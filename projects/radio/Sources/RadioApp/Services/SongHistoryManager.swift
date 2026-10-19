// Sources/RadioApp/Services/SongHistoryManager.swift
import Foundation
import Observation

@Observable
@MainActor
public final class SongHistoryManager {
    public static let shared = SongHistoryManager()
    
    public var recentSongs: [SongHistoryItem] = []
    
    private let userDefaultsKey = "KoreanRadio_SongHistory_v1"
    private let maxHistoryCount = 50
    
    public init() {
        loadHistory()
    }
    
    public func addSong(
        title: String,
        artist: String,
        albumArtURL: URL?,
        station: RadioStation
    ) {
        let cleanTitle = title.trimmingCharacters(in: .whitespacesAndNewlines)
        let cleanArtist = artist.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !cleanTitle.isEmpty else { return }
        
        // Prevent duplicate entries if the same song is already at the top of history
        if let first = recentSongs.first,
           first.title.lowercased() == cleanTitle.lowercased() &&
           first.stationName == station.name &&
           Date().timeIntervalSince(first.playedAt) < 300 {
            return
        }
        
        let newItem = SongHistoryItem(
            title: cleanTitle,
            artist: cleanArtist,
            albumArtURL: albumArtURL,
            playedAt: Date(),
            stationName: station.name,
            frequencyLabel: station.frequencyLabel
        )
        
        recentSongs.insert(newItem, at: 0)
        if recentSongs.count > maxHistoryCount {
            recentSongs = Array(recentSongs.prefix(maxHistoryCount))
        }
        
        saveHistory()
    }
    
    public func clearHistory() {
        recentSongs.removeAll()
        saveHistory()
    }
    
    private func saveHistory() {
        do {
            let data = try JSONEncoder().encode(recentSongs)
            UserDefaults.standard.set(data, forKey: userDefaultsKey)
        } catch {
            print("Failed to save song history: \(error)")
        }
    }
    
    private func loadHistory() {
        guard let data = UserDefaults.standard.data(forKey: userDefaultsKey) else { return }
        do {
            recentSongs = try JSONDecoder().decode([SongHistoryItem].self, from: data)
        } catch {
            print("Failed to load song history: \(error)")
        }
    }
}
