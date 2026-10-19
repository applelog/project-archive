// Sources/RadioApp/Models/SongHistoryItem.swift
import Foundation

public struct SongHistoryItem: Identifiable, Codable, Sendable, Equatable {
    public let id: UUID
    public let title: String
    public let artist: String
    public let albumArtURL: URL?
    public let playedAt: Date
    public let stationName: String
    public let frequencyLabel: String
    
    public init(
        id: UUID = UUID(),
        title: String,
        artist: String,
        albumArtURL: URL? = nil,
        playedAt: Date = Date(),
        stationName: String,
        frequencyLabel: String
    ) {
        self.id = id
        self.title = title
        self.artist = artist
        self.albumArtURL = albumArtURL
        self.playedAt = playedAt
        self.stationName = stationName
        self.frequencyLabel = frequencyLabel
    }
    
    public var formattedTime: String {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm"
        formatter.timeZone = TimeZone(identifier: "Asia/Seoul") ?? .current
        return formatter.string(from: playedAt)
    }
}
