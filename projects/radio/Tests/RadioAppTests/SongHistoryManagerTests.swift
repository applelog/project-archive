// Tests/RadioAppTests/SongHistoryManagerTests.swift
import XCTest
import Foundation
@testable import RadioApp

@MainActor
final class SongHistoryManagerTests: XCTestCase {
    func testAddAndRetrieveSongHistory() {
        let manager = SongHistoryManager.shared
        manager.clearHistory()
        
        let station = KoreanStationPresets.allStations.first!
        manager.addSong(
            title: "밤편지",
            artist: "아이유",
            albumArtURL: URL(string: "https://example.com/art.jpg"),
            station: station
        )
        
        XCTAssertEqual(manager.recentSongs.count, 1)
        XCTAssertEqual(manager.recentSongs.first?.title, "밤편지")
        XCTAssertEqual(manager.recentSongs.first?.artist, "아이유")
        XCTAssertEqual(manager.recentSongs.first?.stationName, station.name)
    }
    
    func testDuplicateSuppressionWithinShortInterval() {
        let manager = SongHistoryManager.shared
        manager.clearHistory()
        
        let station = KoreanStationPresets.allStations.first!
        manager.addSong(title: "밤편지", artist: "아이유", albumArtURL: nil, station: station)
        manager.addSong(title: "밤편지", artist: "아이유", albumArtURL: nil, station: station)
        
        XCTAssertEqual(manager.recentSongs.count, 1)
    }
}
