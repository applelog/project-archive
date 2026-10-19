// Sources/RadioApp/Views/Components/RetroDisplayView.swift
import SwiftUI

public struct RetroDisplayView: View {
    public let frequency: Double
    public let stationName: String
    public let networkName: String
    public let programTitle: String?
    public let djName: String?
    public let programPhotoURL: URL?
    public let songInfo: EnrichedSongInfo?
    public let isLive: Bool
    public let isScanning: Bool
    
    public init(
        frequency: Double,
        stationName: String,
        networkName: String,
        programTitle: String? = nil,
        djName: String? = nil,
        programPhotoURL: URL? = nil,
        songInfo: EnrichedSongInfo? = nil,
        isLive: Bool,
        isScanning: Bool
    ) {
        self.frequency = frequency
        self.stationName = stationName
        self.networkName = networkName
        self.programTitle = programTitle
        self.djName = djName
        self.programPhotoURL = programPhotoURL
        self.songInfo = songInfo
        self.isLive = isLive
        self.isScanning = isScanning
    }
    
    public var body: some View {
        VStack(spacing: 12) {
            // 1. Top Status Bar
            HStack {
                HStack(spacing: 6) {
                    Circle()
                        .fill(isScanning ? Color.yellow : (isLive ? Color.green : Color.gray))
                        .frame(width: 8, height: 8)
                        .shadow(color: isLive ? .green.opacity(0.8) : .clear, radius: 4)
                    
                    Text(isScanning ? "AUTO SCANNING..." : (isLive ? "LIVE FM STEREO" : "STANDBY"))
                        .font(.system(size: 11, weight: .bold, design: .monospaced))
                        .foregroundStyle(isScanning ? .yellow : (isLive ? .green : .secondary))
                        .lineLimit(1)
                }
                
                Spacer(minLength: 8)
                
                Text(networkName)
                    .font(.system(size: 11, weight: .bold, design: .monospaced))
                    .foregroundStyle(.cyan)
                    .lineLimit(1)
            }
            
            // 2. Frequency Large Number Display
            HStack(alignment: .lastTextBaseline, spacing: 6) {
                Text(String(format: "%.1f", frequency))
                    .font(.system(size: 46, weight: .heavy, design: .monospaced))
                    .monospacedDigit()
                    .foregroundStyle(.white)
                
                Text("MHz")
                    .font(.system(size: 16, weight: .bold, design: .monospaced))
                    .foregroundStyle(.orange)
                    .padding(.bottom, 2)
                
                Spacer()
                
                // Signal Indicator Bars
                HStack(alignment: .bottom, spacing: 3) {
                    ForEach(1...4, id: \.self) { bar in
                        RoundedRectangle(cornerRadius: 1)
                            .fill(isLive ? Color.green : (isScanning ? Color.yellow : Color.white.opacity(0.2)))
                            .frame(width: 4, height: CGFloat(bar * 5))
                    }
                }
                .padding(.bottom, 6)
            }
            
            // 3. Station & On-Air Broadcast Program Card (라디오 방송 제목 유지)
            HStack(spacing: 10) {
                if let photoURL = programPhotoURL {
                    AsyncImage(url: photoURL) { phase in
                        switch phase {
                        case .success(let image):
                            image
                                .resizable()
                                .aspectRatio(contentMode: .fill)
                                .frame(width: 38, height: 38)
                                .clipShape(RoundedRectangle(cornerRadius: 8))
                        default:
                            placeholderRadioIcon
                        }
                    }
                } else {
                    placeholderRadioIcon
                }
                
                VStack(alignment: .leading, spacing: 2) {
                    HStack(spacing: 6) {
                        Text(stationName)
                            .font(.system(size: 14, weight: .bold))
                            .foregroundStyle(.white)
                            .lineLimit(1)
                            .minimumScaleFactor(0.85)
                        
                        if isLive {
                            Text("ON AIR")
                                .font(.system(size: 9, weight: .heavy, design: .monospaced))
                                .foregroundStyle(.red)
                                .padding(.horizontal, 5)
                                .padding(.vertical, 1.5)
                                .background(Capsule().fill(Color.red.opacity(0.18)))
                                .fixedSize()
                        }
                    }
                    
                    if let program = programTitle, !program.isEmpty {
                        Text(djName?.isEmpty == false ? "\(program) • \(djName!)" : program)
                            .font(.system(size: 12))
                            .foregroundStyle(.gray)
                            .lineLimit(1)
                            .truncationMode(.tail)
                    }
                }
                
                Spacer(minLength: 4)
            }
            .padding(9)
            .background(
                RoundedRectangle(cornerRadius: 10, style: .continuous)
                    .fill(Color.white.opacity(0.04))
            )
            
            // 4. Current Playing Music & Spotify Link (오직 실제 노래가 나올 때만 스포티파이 버튼 활성화!)
            if let song = songInfo {
                Button {
                    SpotifyLinkManager.openInSpotify(title: song.title, artist: song.artist)
                } label: {
                    HStack(spacing: 10) {
                        // Album Cover Art
                        if let artURL = song.albumArtURL {
                            AsyncImage(url: artURL) { phase in
                                switch phase {
                                case .success(let image):
                                    image
                                        .resizable()
                                        .aspectRatio(contentMode: .fill)
                                        .frame(width: 42, height: 42)
                                        .clipShape(RoundedRectangle(cornerRadius: 8))
                                default:
                                    placeholderMusicIcon
                                }
                            }
                        } else {
                            placeholderMusicIcon
                        }
                        
                        VStack(alignment: .leading, spacing: 2) {
                            HStack(spacing: 4) {
                                Image(systemName: "music.note")
                                    .font(.system(size: 9))
                                    .foregroundStyle(.orange)
                                Text("NOW PLAYING")
                                    .font(.system(size: 9, weight: .bold, design: .monospaced))
                                    .foregroundStyle(.orange)
                            }
                            
                            Text(song.title)
                                .font(.system(size: 13, weight: .bold))
                                .foregroundStyle(.white)
                                .lineLimit(1)
                                .truncationMode(.tail)
                            
                            if !song.artist.isEmpty {
                                Text(song.artist)
                                    .font(.system(size: 11))
                                    .foregroundStyle(.gray)
                                    .lineLimit(1)
                                    .truncationMode(.tail)
                            }
                        }
                        
                        Spacer(minLength: 8)
                        
                        // Spotify Open Button (노래만 검색)
                        HStack(spacing: 4) {
                            Image(systemName: "arrow.up.right.circle.fill")
                                .font(.system(size: 12))
                            Text("Spotify")
                                .font(.system(size: 11, weight: .bold, design: .rounded))
                        }
                        .foregroundStyle(.black)
                        .padding(.horizontal, 9)
                        .padding(.vertical, 5)
                        .background(
                            Capsule().fill(Color(red: 0.11, green: 0.84, blue: 0.38)) // Spotify Green
                        )
                        .fixedSize()
                        .layoutPriority(2)
                    }
                    .padding(10)
                    .background(
                        RoundedRectangle(cornerRadius: 12, style: .continuous)
                            .fill(Color.white.opacity(0.06))
                    )
                }
                .buttonStyle(.plain)
            } else if isLive {
                // When in talk / DJ commentary / story reading (No song active)
                HStack(spacing: 6) {
                    Image(systemName: "waveform")
                        .font(.system(size: 11))
                        .foregroundStyle(.cyan)
                    Text("실시간 생방송 사연 / 토크 진행 중")
                        .font(.system(size: 11, weight: .medium))
                        .foregroundStyle(.gray)
                        .lineLimit(1)
                    Spacer()
                }
                .padding(.horizontal, 8)
                .padding(.top, 2)
            }
        }
        .padding(16)
        .background(
            RoundedRectangle(cornerRadius: 16, style: .continuous)
                .fill(Color(red: 0.08, green: 0.09, blue: 0.12))
                .overlay(
                    RoundedRectangle(cornerRadius: 16, style: .continuous)
                        .stroke(Color.white.opacity(0.12), lineWidth: 1)
                )
        )
    }
    
    private var placeholderRadioIcon: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 8, style: .continuous)
                .fill(Color.white.opacity(0.08))
                .frame(width: 38, height: 38)
            Image(systemName: "mic.fill")
                .font(.system(size: 14))
                .foregroundStyle(.orange)
        }
    }
    
    private var placeholderMusicIcon: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 8, style: .continuous)
                .fill(Color.white.opacity(0.1))
                .frame(width: 42, height: 42)
            Image(systemName: "music.note")
                .font(.system(size: 14))
                .foregroundStyle(.orange)
        }
    }
}
