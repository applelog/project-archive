// Sources/RadioApp/Services/SongMetadataEnricher.swift
import Foundation

public struct EnrichedSongInfo: Sendable, Equatable {
    public let title: String
    public let artist: String
    public let albumName: String?
    public let albumArtURL: URL?
    
    public init(
        title: String,
        artist: String,
        albumName: String? = nil,
        albumArtURL: URL? = nil
    ) {
        self.title = title
        self.artist = artist
        self.albumName = albumName
        self.albumArtURL = albumArtURL
    }
}

public final class SongMetadataEnricher: Sendable {
    public static let shared = SongMetadataEnricher()
    
    public init() {}
    
    /// Cleans radio stream titles like "[CoolFM] 12:30 아이유 - 밤편지 (Live)" -> "아이유 밤편지"
    public func cleanTitle(_ rawText: String) -> String {
        var text = rawText
        
        // Remove brackets like [KBS], (Live), [CoolFM], [12:30]
        let bracketPattern = #"\[.*?\]|\(.*?\)"#
        text = text.replacingOccurrences(of: bracketPattern, with: "", options: .regularExpression)
        
        // Remove common radio noise words
        let noiseWords = ["생방송", "광고", "CoolFM", "FM4U", "파워FM", "러브FM", "ON AIR", "LIVE"]
        for word in noiseWords {
            text = text.replacingOccurrences(of: word, with: "", options: .caseInsensitive)
        }
        
        return text.trimmingCharacters(in: .whitespacesAndNewlines)
    }
    
    /// Searches exact track & high-res album artwork via zero-auth Apple iTunes Search API
    public func searchTrack(query: String) async -> EnrichedSongInfo? {
        let cleaned = cleanTitle(query)
        guard !cleaned.isEmpty,
              let encoded = cleaned.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed),
              let url = URL(string: "https://itunes.apple.com/search?term=\(encoded)&entity=song&limit=1&country=KR") else {
            return nil
        }
        
        var request = URLRequest(url: url)
        request.timeoutInterval = 3.0
        
        do {
            let (data, response) = try await URLSession.shared.data(for: request)
            guard let httpResp = response as? HTTPURLResponse, httpResp.statusCode == 200 else {
                return nil
            }
            
            struct SearchResponse: Codable {
                let resultCount: Int
                let results: [SongResult]
            }
            struct SongResult: Codable {
                let trackName: String?
                let artistName: String?
                let collectionName: String?
                let artworkUrl100: String?
            }
            
            let decoded = try JSONDecoder().decode(SearchResponse.self, from: data)
            if let first = decoded.results.first, let trackName = first.trackName, let artistName = first.artistName {
                let highResArt = first.artworkUrl100?.replacingOccurrences(of: "100x100bb", with: "600x600bb")
                let artURL = highResArt.flatMap { URL(string: $0) }
                
                return EnrichedSongInfo(
                    title: trackName,
                    artist: artistName,
                    albumName: first.collectionName,
                    albumArtURL: artURL
                )
            }
        } catch {
            print("Song enrich error: \(error)")
        }
        
        // Fallback: parse "Artist - Title" format directly if search fails
        let parts = cleaned.components(separatedBy: "-")
        if parts.count >= 2 {
            let artist = parts[0].trimmingCharacters(in: .whitespaces)
            let title = parts[1].trimmingCharacters(in: .whitespaces)
            return EnrichedSongInfo(title: title, artist: artist)
        }
        
        return EnrichedSongInfo(title: cleaned, artist: "")
    }
}
