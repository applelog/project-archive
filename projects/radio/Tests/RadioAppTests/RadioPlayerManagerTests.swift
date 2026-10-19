// Tests/RadioAppTests/RadioPlayerManagerTests.swift
import XCTest
import Foundation
@testable import RadioApp

final class RadioPlayerManagerTests: XCTestCase {
    func testPlaybackStateTransitions() {
        let station = KoreanStationPresets.allStations[0]
        XCTAssertTrue(station.name.contains("쿨FM"))
        
        let manager = RadioPlayerManager()
        XCTAssertEqual(manager.state, .stopped)
    }
}
