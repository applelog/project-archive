// Sources/RadioApp/Services/SpotifyLinkManager.swift
import Foundation
#if canImport(UIKit)
import UIKit
#endif

public struct SpotifyLinkManager: Sendable {
    public static func openInSpotify(title: String, artist: String? = nil) {
        let cleanTitle = title.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !cleanTitle.isEmpty else { return }
        
        var query = cleanTitle
        if let artist, !artist.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            query = "\(artist) \(cleanTitle)"
        }
        
        guard let encoded = query.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed) else {
            return
        }
        
        #if os(iOS)
        let appURL = URL(string: "spotify:search:\(encoded)")
        let webURL = URL(string: "https://open.spotify.com/search/\(encoded)")
        
        Task { @MainActor in
            if let appURL, UIApplication.shared.canOpenURL(appURL) {
                UIApplication.shared.open(appURL)
            } else if let webURL {
                UIApplication.shared.open(webURL)
            }
        }
        #endif
    }
}
