// Sources/RadioApp/Services/KoreanRadioOnAirFetcher.swift
import Foundation

public struct OnAirBroadcastInfo: Sendable, Equatable {
    public let programTitle: String
    public let djName: String
    public let scheduleTime: String
    public let programPhotoURL: URL?
    public let currentSongTitle: String?
    public let currentSongArtist: String?
    
    public init(
        programTitle: String,
        djName: String = "",
        scheduleTime: String = "",
        programPhotoURL: URL? = nil,
        currentSongTitle: String? = nil,
        currentSongArtist: String? = nil
    ) {
        self.programTitle = programTitle
        self.djName = djName
        self.scheduleTime = scheduleTime
        self.programPhotoURL = programPhotoURL
        self.currentSongTitle = currentSongTitle
        self.currentSongArtist = currentSongArtist
    }
}

public final class KoreanRadioOnAirFetcher: Sendable {
    public static let shared = KoreanRadioOnAirFetcher()
    
    public init() {}
    
    /// Fetches real-time on-air program info and currently playing song
    public func fetchCurrentBroadcast(for station: RadioStation) async -> OnAirBroadcastInfo {
        switch station.network {
        case .mbc:
            if let info = await fetchMBCOnAir(station: station) {
                return info
            }
        case .sbs:
            if let info = await fetchSBSOnAir(station: station) {
                return info
            }
        default:
            break
        }
        
        // Fallback to accurate program schedule based on current Korean standard time
        return getScheduledBroadcast(for: station)
    }
    
    // MARK: - MBC Live On-Air API
    private func fetchMBCOnAir(station: RadioStation) async -> OnAirBroadcastInfo? {
        let isFM4U = station.frequency == 91.9 || station.name.contains("FM4U")
        let channelParam = isFM4U ? "mfm" : "sfm"
        
        guard let url = URL(string: "https://control.imbc.com/Schedule/Radio?channel=\(channelParam)") else {
            return nil
        }
        
        var request = URLRequest(url: url)
        request.timeoutInterval = 3.0
        request.setValue("Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X)", forHTTPHeaderField: "User-Agent")
        
        do {
            let (data, response) = try await URLSession.shared.data(for: request)
            guard let httpResp = response as? HTTPURLResponse, httpResp.statusCode == 200 else {
                return nil
            }
            
            struct MBCScheduleItem: Decodable {
                let Title: String?
                let SubTitle: String?
                let Players: String?
                let Photo: String?
                let StartTime: String?
                let EndTime: String?
                let IsOnAirNow: String?
            }
            
            let items = try JSONDecoder().decode([MBCScheduleItem].self, from: data)
            let currentHHmm = currentKoreanTimeHHmm()
            
            for item in items {
                let isOnAir = item.IsOnAirNow == "Y"
                let matchesTime: Bool
                if let st = item.StartTime, let et = item.EndTime {
                    if st <= et {
                        matchesTime = currentHHmm >= st && currentHHmm < et
                    } else { // Overnight spanning midnight e.g. 2300 ~ 0100
                        matchesTime = currentHHmm >= st || currentHHmm < et
                    }
                } else {
                    matchesTime = false
                }
                
                if isOnAir || matchesTime {
                    let title = item.Title ?? station.name
                    let dj = item.Players ?? ""
                    let photoURL = item.Photo.flatMap { URL(string: $0) }
                    
                    // Check if a real-time song is currently on air via somitem API
                    let activeSong = await fetchMBCCurrentSong(station: station)
                    
                    return OnAirBroadcastInfo(
                        programTitle: title,
                        djName: dj,
                        scheduleTime: "\(item.StartTime ?? "") ~ \(item.EndTime ?? "")",
                        programPhotoURL: photoURL,
                        currentSongTitle: activeSong?.title,
                        currentSongArtist: activeSong?.artist
                    )
                }
            }
        } catch {
            // Silently fall back to schedule timetable
        }
        
        return nil
    }
    
    private func fetchMBCCurrentSong(station: RadioStation) async -> (title: String, artist: String)? {
        let targetChannel = (station.frequency == 91.9 || station.name.contains("FM4U")) ? "FM4U" : "STFM"
        guard let url = URL(string: "https://miniapi.imbc.com/music/somitem") else { return nil }
        
        var request = URLRequest(url: url)
        request.timeoutInterval = 2.0
        request.setValue("Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X)", forHTTPHeaderField: "User-Agent")
        
        do {
            let (data, response) = try await URLSession.shared.data(for: request)
            guard let httpResp = response as? HTTPURLResponse, httpResp.statusCode == 200 else { return nil }
            
            struct SomItemResponse: Decodable {
                let Channel: String?
                let SomItem: String?
            }
            let items = try JSONDecoder().decode([SomItemResponse].self, from: data)
            if let matched = items.first(where: { $0.Channel == targetChannel }),
               let raw = matched.SomItem?.trimmingCharacters(in: .whitespacesAndNewlines),
               !raw.isEmpty {
                let parts = raw.components(separatedBy: "-")
                if parts.count >= 2 {
                    return (
                        title: parts[1].trimmingCharacters(in: .whitespaces),
                        artist: parts[0].trimmingCharacters(in: .whitespaces)
                    )
                } else {
                    return (title: raw, artist: "")
                }
            }
        } catch {}
        return nil
    }
    
    // MARK: - SBS Live On-Air API
    private func fetchSBSOnAir(station: RadioStation) async -> OnAirBroadcastInfo? {
        guard let url = URL(string: "https://apis.sbs.co.kr/play-api/1.0/onair/channels") else {
            return nil
        }
        
        var request = URLRequest(url: url)
        request.timeoutInterval = 3.0
        request.setValue("Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X)", forHTTPHeaderField: "User-Agent")
        
        do {
            let (data, response) = try await URLSession.shared.data(for: request)
            guard let httpResp = response as? HTTPURLResponse, httpResp.statusCode == 200 else {
                return nil
            }
            
            struct SBSResponse: Decodable {
                let list: [SBSChannelItem]?
            }
            struct SBSChannelItem: Decodable {
                let channelname: String?
                let channelid: String?
                let title: String?
                let subtitle: String?
                let thumbimg: String?
                let type: String?
            }
            
            let sbsData = try JSONDecoder().decode(SBSResponse.self, from: data)
            guard let list = sbsData.list else { return nil }
            
            let isPower = station.frequency == 107.7 || station.name.contains("파워")
            let targetId = isPower ? "S07" : "S08"
            
            if let matched = list.first(where: { $0.channelid == targetId || ($0.type == "Radio" && (isPower ? ($0.channelname?.contains("POWER") == true) : ($0.channelname?.contains("LOVE") == true))) }) {
                let title = matched.title ?? station.name
                let dj = isPower ? "SBS 파워FM" : "SBS 러브FM"
                let photoURL = matched.thumbimg.flatMap { URL(string: $0) }
                
                return OnAirBroadcastInfo(
                    programTitle: title,
                    djName: dj,
                    scheduleTime: matched.subtitle ?? "",
                    programPhotoURL: photoURL
                )
            }
        } catch {
            // Silently fall back to schedule timetable
        }
        
        return nil
    }
    
    // MARK: - Korean Standard Time Timetable Fallback
    public func getScheduledBroadcast(for station: RadioStation) -> OnAirBroadcastInfo {
        let hour = currentKoreanHour()
        
        switch station.network {
        case .kbs:
            if station.frequency == 89.1 || station.name.contains("쿨") {
                return kbsCoolFMProgram(forHour: hour)
            } else if station.frequency == 93.1 || station.name.contains("클래식") {
                return kbsClassicFMProgram(forHour: hour)
            } else if station.frequency == 97.3 || station.name.contains("1라디오") {
                return kbs1RadioProgram(forHour: hour)
            } else {
                return kbs2RadioProgram(forHour: hour)
            }
            
        case .cbs:
            if station.frequency == 93.9 || station.name.contains("음악") {
                return cbsMusicFMProgram(forHour: hour)
            } else {
                return cbsStandardFMProgram(forHour: hour)
            }
            
        case .tbs:
            return tbsFMProgram(forHour: hour)
            
        case .ebs:
            return ebsFMProgram(forHour: hour)
            
        case .ytn:
            return OnAirBroadcastInfo(programTitle: "YTN 24시간 뉴스 생방송", djName: "YTN FM 94.5")
            
        default:
            return OnAirBroadcastInfo(programTitle: station.name, djName: station.network.displayName)
        }
    }
    
    private func kbsCoolFMProgram(forHour hour: Int) -> OnAirBroadcastInfo {
        switch hour {
        case 6:
            return OnAirBroadcastInfo(programTitle: "상쾌한 아침", djName: "KBS Cool FM 89.1")
        case 7, 8:
            return OnAirBroadcastInfo(programTitle: "조우종의 FM대행진", djName: "조우종")
        case 9, 10:
            return OnAirBroadcastInfo(programTitle: "이현우의 음악앨범", djName: "이현우")
        case 11:
            return OnAirBroadcastInfo(programTitle: "박명수의 라디오쇼", djName: "박명수")
        case 12, 13:
            return OnAirBroadcastInfo(programTitle: "이은지의 가요광장", djName: "이은지")
        case 14, 15:
            return OnAirBroadcastInfo(programTitle: "하하의 슈퍼라디오", djName: "하하")
        case 16, 17:
            return OnAirBroadcastInfo(programTitle: "윤정수 남창희의 미스터 라디오", djName: "윤정수, 남창희")
        case 18, 19:
            return OnAirBroadcastInfo(programTitle: "사랑하기 좋은 날 이금희입니다", djName: "이금희")
        case 20, 21:
            return OnAirBroadcastInfo(programTitle: "청하의 볼륨을 높여요", djName: "청하")
        case 22, 23:
            return OnAirBroadcastInfo(programTitle: "몬스타엑스 아이엠의 키스 더 라디오", djName: "I.M")
        case 0, 1:
            return OnAirBroadcastInfo(programTitle: "스테이션 Z", djName: "KBS Cool FM")
        default:
            return OnAirBroadcastInfo(programTitle: "쿨FM 심야 음악방송", djName: "KBS Cool FM 89.1")
        }
    }
    
    private func kbsClassicFMProgram(forHour hour: Int) -> OnAirBroadcastInfo {
        switch hour {
        case 7, 8:
            return OnAirBroadcastInfo(programTitle: "출발 FM과 함께", djName: "KBS Classic FM")
        case 9, 10:
            return OnAirBroadcastInfo(programTitle: "명연주 명음반", djName: "정만섭")
        case 11:
            return OnAirBroadcastInfo(programTitle: "KBS 음악실", djName: "KBS Classic FM")
        case 12, 13:
            return OnAirBroadcastInfo(programTitle: "생생 클래식", djName: "KBS Classic FM")
        case 14, 15:
            return OnAirBroadcastInfo(programTitle: "FM 실황음악", djName: "정세진")
        case 16:
            return OnAirBroadcastInfo(programTitle: "노래의 날개 위에", djName: "KBS Classic FM")
        case 17:
            return OnAirBroadcastInfo(programTitle: "FM POPS", djName: "한동준")
        case 18, 19:
            return OnAirBroadcastInfo(programTitle: "세상의 모든 음악", djName: "전기현")
        case 20, 21:
            return OnAirBroadcastInfo(programTitle: "당신의 밤과 음악", djName: "이상협")
        case 22, 23:
            return OnAirBroadcastInfo(programTitle: "재즈수첩", djName: "황덕호")
        default:
            return OnAirBroadcastInfo(programTitle: "KBS 클래식FM 24시간 음악", djName: "93.1 MHz")
        }
    }
    
    private func kbs1RadioProgram(forHour hour: Int) -> OnAirBroadcastInfo {
        switch hour {
        case 7, 8:
            return OnAirBroadcastInfo(programTitle: "고성국의 전격시사", djName: "고성국")
        case 9, 10:
            return OnAirBroadcastInfo(programTitle: "성공예감 이대호입니다", djName: "이대호")
        case 11:
            return OnAirBroadcastInfo(programTitle: "라디오 전국일주", djName: "KBS 1Radio")
        case 12, 13:
            return OnAirBroadcastInfo(programTitle: "뉴스레터 K", djName: "KBS 1Radio")
        case 14, 15:
            return OnAirBroadcastInfo(programTitle: "정관용의 시사본부", djName: "정관용")
        case 17, 18:
            return OnAirBroadcastInfo(programTitle: "생방송 주진우 라이브", djName: "KBS 1Radio")
        default:
            return OnAirBroadcastInfo(programTitle: "KBS 제1라디오 뉴스 & 시사", djName: "97.3 MHz")
        }
    }
    
    private func kbs2RadioProgram(forHour hour: Int) -> OnAirBroadcastInfo {
        switch hour {
        case 7, 8:
            return OnAirBroadcastInfo(programTitle: "김태훈의 프리웨이", djName: "김태훈")
        case 9, 10:
            return OnAirBroadcastInfo(programTitle: "주현미의 러브레터", djName: "주현미")
        case 11:
            return OnAirBroadcastInfo(programTitle: "박수홍의 기분 좋은 날", djName: "박수홍")
        case 12, 13:
            return OnAirBroadcastInfo(programTitle: "이각경의 해피타임 4시", djName: "이각경")
        case 18, 19:
            return OnAirBroadcastInfo(programTitle: "김혜영과 함께", djName: "김혜영")
        default:
            return OnAirBroadcastInfo(programTitle: "KBS 해피FM 2라디오", djName: "106.1 MHz")
        }
    }
    
    private func cbsMusicFMProgram(forHour hour: Int) -> OnAirBroadcastInfo {
        switch hour {
        case 7, 8:
            return OnAirBroadcastInfo(programTitle: "김용신의 그대와 여는 아침", djName: "김용신")
        case 9, 10:
            return OnAirBroadcastInfo(programTitle: "강석우의 아름다운 당신에게", djName: "강석우")
        case 11:
            return OnAirBroadcastInfo(programTitle: "신지혜의 영화음악", djName: "신지혜")
        case 12, 13:
            return OnAirBroadcastInfo(programTitle: "이수영의 12시에 만납시다", djName: "이수영")
        case 14, 15:
            return OnAirBroadcastInfo(programTitle: "박승화의 가요속으로", djName: "박승화")
        case 16, 17:
            return OnAirBroadcastInfo(programTitle: "유영석의 FM FM", djName: "유영석")
        case 18, 19:
            return OnAirBroadcastInfo(programTitle: "배미향의 저녁스케치", djName: "배미향")
        case 20, 21:
            return OnAirBroadcastInfo(programTitle: "김현주의 행복한 동행", djName: "김현주")
        case 22, 23:
            return OnAirBroadcastInfo(programTitle: "허윤희의 꿈과 음악 사이에", djName: "허윤희")
        default:
            return OnAirBroadcastInfo(programTitle: "CBS 음악FM 24시간 음악", djName: "93.9 MHz")
        }
    }
    
    private func cbsStandardFMProgram(forHour hour: Int) -> OnAirBroadcastInfo {
        switch hour {
        case 7, 8:
            return OnAirBroadcastInfo(programTitle: "김현정의 뉴스쇼", djName: "김현정")
        case 18, 19:
            return OnAirBroadcastInfo(programTitle: "박재홍의 한판승부", djName: "박재홍")
        default:
            return OnAirBroadcastInfo(programTitle: "CBS 표준FM 뉴스 & 시사", djName: "98.1 MHz")
        }
    }
    
    private func tbsFMProgram(forHour hour: Int) -> OnAirBroadcastInfo {
        switch hour {
        case 7, 8:
            return OnAirBroadcastInfo(programTitle: "TBS 출근길 생방송", djName: "TBS FM 95.1")
        case 12, 13:
            return OnAirBroadcastInfo(programTitle: "TBS 가요광장", djName: "TBS FM")
        case 18, 19:
            return OnAirBroadcastInfo(programTitle: "TBS 퇴근길 교통방송", djName: "TBS FM 95.1")
        default:
            return OnAirBroadcastInfo(programTitle: "TBS FM 교통 & 음악 방송", djName: "95.1 MHz")
        }
    }
    
    private func ebsFMProgram(forHour hour: Int) -> OnAirBroadcastInfo {
        return OnAirBroadcastInfo(programTitle: "EBS 책 읽어주는 라디오 / 모닝스페셜", djName: "EBS FM 104.5")
    }
    
    // MARK: - Helpers
    private func currentKoreanHour() -> Int {
        var calendar = Calendar(identifier: .gregorian)
        if let kst = TimeZone(identifier: "Asia/Seoul") {
            calendar.timeZone = kst
        }
        return calendar.component(.hour, from: Date())
    }
    
    private func currentKoreanTimeHHmm() -> String {
        let formatter = DateFormatter()
        formatter.dateFormat = "HHmm"
        if let kst = TimeZone(identifier: "Asia/Seoul") {
            formatter.timeZone = kst
        }
        return formatter.string(from: Date())
    }
}
