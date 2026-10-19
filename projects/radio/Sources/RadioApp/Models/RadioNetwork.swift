// Sources/RadioApp/Models/RadioNetwork.swift
import Foundation

public enum RadioNetwork: String, CaseIterable, Codable, Sendable {
    case kbs = "KBS"
    case mbc = "MBC"
    case sbs = "SBS"
    case cbs = "CBS"
    case tbs = "TBS"
    case ebs = "EBS"
    case ytn = "YTN"
    case custom = "MY"
    
    public var displayName: String {
        switch self {
        case .kbs: return "KBS 한국방송"
        case .mbc: return "MBC 문화방송"
        case .sbs: return "SBS"
        case .cbs: return "CBS 기독교방송"
        case .tbs: return "TBS 교통방송"
        case .ebs: return "EBS 교육방송"
        case .ytn: return "YTN 뉴스"
        case .custom: return "내 채널"
        }
    }
}

#if canImport(SwiftUI)
import SwiftUI

extension RadioNetwork {
    public var accentColor: Color {
        switch self {
        case .kbs: return Color(red: 0.18, green: 0.49, blue: 0.96) // Apple Royal Blue
        case .mbc: return Color(red: 0.62, green: 0.28, blue: 0.88) // Apple Purple
        case .sbs: return Color(red: 0.12, green: 0.73, blue: 0.53) // Apple Emerald Teal
        case .cbs: return Color(red: 0.38, green: 0.36, blue: 0.88) // Apple Indigo
        case .tbs: return Color(red: 0.98, green: 0.52, blue: 0.15) // Apple Warm Amber
        case .ebs: return Color(red: 0.00, green: 0.75, blue: 0.75) // Apple Mint Cyan
        case .ytn: return Color(red: 0.94, green: 0.27, blue: 0.24) // Apple Coral Red
        case .custom: return Color(red: 1.00, green: 0.60, blue: 0.00) // Apple Orange
        }
    }
}
#endif
