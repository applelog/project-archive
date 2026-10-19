// Tests/RadioAppTests/KoreanRadioOnAirFetcherTests.swift
import XCTest
import Foundation
@testable import RadioApp

final class KoreanRadioOnAirFetcherTests: XCTestCase {
    func testScheduledBroadcastFallbackForKBSCoolFM() {
        let fetcher = KoreanRadioOnAirFetcher()
        let coolFM = KoreanStationPresets.allStations.first { $0.frequency == 89.1 }!
        let info = fetcher.getScheduledBroadcast(for: coolFM)
        
        XCTAssertFalse(info.programTitle.isEmpty)
        XCTAssertFalse(info.djName.isEmpty)
    }
    
    func testScheduledBroadcastFallbackForCBSMusicFM() {
        let fetcher = KoreanRadioOnAirFetcher()
        let cbsMusic = KoreanStationPresets.allStations.first { $0.frequency == 93.9 }!
        let info = fetcher.getScheduledBroadcast(for: cbsMusic)
        
        XCTAssertFalse(info.programTitle.isEmpty)
        XCTAssertFalse(info.djName.isEmpty)
    }
    
    func testSongEnricherDirectSearch() async {
        let enricher = SongMetadataEnricher()
        let song = await enricher.searchTrack(query: "IU 밤편지")
        if let song {
            XCTAssertFalse(song.title.isEmpty)
        }
    }
}
