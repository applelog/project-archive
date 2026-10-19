// Sources/RadioApp/Services/KoreanStationPresets.swift
import Foundation

public struct KoreanStationPresets: Sendable {
    public static let allStations: [RadioStation] = [
        RadioStation(
            name: "KBS 2FM (쿨FM)",
            network: .kbs,
            frequency: 89.1,
            streamURL: URL(string: "https://radio.bsod.kr/stream?stn=kbs&ch=2fm")!,
            category: "대중음악/예능"
        ),
        RadioStation(
            name: "MBC FM4U",
            network: .mbc,
            frequency: 91.9,
            streamURL: URL(string: "https://radio.bsod.kr/stream?stn=mbc&ch=mfm")!,
            category: "음악/예능"
        ),
        RadioStation(
            name: "KBS 1FM (클래식FM)",
            network: .kbs,
            frequency: 93.1,
            streamURL: URL(string: "https://radio.bsod.kr/stream?stn=kbs&ch=1fm")!,
            category: "클래식/국악"
        ),
        RadioStation(
            name: "CBS 음악FM",
            network: .cbs,
            frequency: 93.9,
            streamURL: URL(string: "https://m-aac.cbs.co.kr/cbs939/_definst_/cbs939.stream/playlist.m3u8")!,
            category: "가요/팝/힐링"
        ),
        RadioStation(
            name: "YTN 라디오",
            network: .ytn,
            frequency: 94.5,
            streamURL: URL(string: "https://radiolive.ytn.co.kr/radio/_definst_/20211118_fmlive/playlist.m3u8")!,
            category: "24시간 뉴스"
        ),
        RadioStation(
            name: "TBS 교통방송 FM",
            network: .tbs,
            frequency: 95.1,
            streamURL: URL(string: "https://cdnfm.tbs.seoul.kr/tbs/_definst_/tbs_fm_web_360.smil/chunklist.m3u8")!,
            category: "교통/시사"
        ),
        RadioStation(
            name: "MBC 표준FM",
            network: .mbc,
            frequency: 95.9,
            streamURL: URL(string: "https://radio.bsod.kr/stream?stn=mbc&ch=sfm")!,
            category: "뉴스/시사/교양"
        ),
        RadioStation(
            name: "KBS 1라디오",
            network: .kbs,
            frequency: 97.3,
            streamURL: URL(string: "https://radio.bsod.kr/stream?stn=kbs&ch=1radio")!,
            category: "뉴스/시사"
        ),
        RadioStation(
            name: "CBS 표준FM",
            network: .cbs,
            frequency: 98.1,
            streamURL: URL(string: "https://m-aac.cbs.co.kr/mweb_cbs981/_definst_/cbs981.stream/playlist.m3u8")!,
            category: "시사/종교"
        ),
        RadioStation(
            name: "TBS eFM",
            network: .tbs,
            frequency: 101.3,
            streamURL: URL(string: "https://cdnefm.tbs.seoul.kr/tbs/_definst_/tbs_efm_web_360.smil/chunklist.m3u8")!,
            category: "영어/외국어 방송"
        ),
        RadioStation(
            name: "SBS 러브FM",
            network: .sbs,
            frequency: 103.5,
            streamURL: URL(string: "https://radio.bsod.kr/stream?stn=sbs&ch=lovefm")!,
            category: "시사/예능"
        ),
        RadioStation(
            name: "EBS FM",
            network: .ebs,
            frequency: 104.5,
            streamURL: URL(string: "http://ebsonairiosaod.ebs.co.kr/fmradiobandiaod/bandiappaac/playlist.m3u8")!,
            category: "교육/외국어"
        ),
        RadioStation(
            name: "KBS 2라디오 (해피FM)",
            network: .kbs,
            frequency: 106.1,
            streamURL: URL(string: "https://radio.bsod.kr/stream?stn=kbs&ch=2radio")!,
            category: "중장년 가요/교양"
        ),
        RadioStation(
            name: "SBS 파워FM",
            network: .sbs,
            frequency: 107.7,
            streamURL: URL(string: "https://radio.bsod.kr/stream?stn=sbs&ch=powerfm")!,
            category: "대중음악/예능"
        )
    ]
    
    public static func findByFrequency(_ freq: Double, tolerance: Double = 0.08) -> RadioStation? {
        return allStations.first { station in
            guard let stationFreq = station.frequency else { return false }
            return abs(stationFreq - freq) <= tolerance
        }
    }
}
