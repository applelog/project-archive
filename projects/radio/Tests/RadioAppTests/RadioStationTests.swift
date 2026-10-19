// Tests/RadioAppTests/RadioStationTests.swift
import XCTest
import Foundation
@testable import RadioApp

final class RadioStationTests: XCTestCase {
    func testRadioStationInitializationAndFrequencyFormatting() {
        let url = URL(string: "https://example.com/stream.m3u8")!
        let station = RadioStation(
            name: "SBS 파워FM",
            network: .sbs,
            frequency: 107.7,
            streamURL: url,
            category: "음악/예능"
        )
        
        XCTAssertEqual(station.name, "SBS 파워FM")
        XCTAssertEqual(station.network, .sbs)
        XCTAssertEqual(station.frequency, 107.7)
        XCTAssertEqual(station.frequencyLabel, "107.7 MHz")
        XCTAssertFalse(station.isCustom)
    }

    func testOnlineOnlyStationWithoutFrequency() {
        let url = URL(string: "https://example.com/ytn.m3u8")!
        let station = RadioStation(
            name: "YTN 라디오",
            network: .ytn,
            frequency: nil,
            streamURL: url,
            category: "뉴스/시사"
        )
        
        XCTAssertEqual(station.frequencyLabel, "ONLINE")
    }
}
