// Tests/RadioAppTests/KoreanStationPresetsTests.swift
import XCTest
import Foundation
@testable import RadioApp

final class KoreanStationPresetsTests: XCTestCase {
    func testPresetsAreOrderedByFrequency() {
        let stations = KoreanStationPresets.allStations
        XCTAssertFalse(stations.isEmpty)
        
        let frequencies = stations.compactMap(\.frequency)
        XCTAssertEqual(frequencies, frequencies.sorted())
    }
    
    func testContainsMajorKoreanStations() {
        let stations = KoreanStationPresets.allStations
        let names = stations.map(\.name)
        
        XCTAssertTrue(names.contains(where: { $0.contains("쿨FM") }))
        XCTAssertTrue(names.contains(where: { $0.contains("FM4U") }))
        XCTAssertTrue(names.contains(where: { $0.contains("파워FM") }))
        XCTAssertTrue(names.contains(where: { $0.contains("음악FM") }))
    }

    func testFindByFrequencyMatchesClosestStation() {
        let station = KoreanStationPresets.findByFrequency(107.7)
        XCTAssertTrue(station?.name.contains("파워FM") == true)
        
        let closeMatch = KoreanStationPresets.findByFrequency(91.92, tolerance: 0.1)
        XCTAssertTrue(closeMatch?.name.contains("FM4U") == true)
    }
}
