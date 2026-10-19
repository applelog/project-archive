// Tests/RadioAppTests/RadioViewModelTests.swift
import XCTest
import Foundation
@testable import RadioApp

@MainActor
final class RadioViewModelTests: XCTestCase {
    func testInitialStateAndStationSelection() {
        let vm = RadioViewModel()
        XCTAssertFalse(vm.stations.isEmpty)
        XCTAssertFalse(vm.isScanning)
        
        let powerFM = vm.stations.first { $0.frequency == 107.7 }!
        vm.selectStation(powerFM)
        
        XCTAssertEqual(vm.currentStation?.id, powerFM.id)
        XCTAssertEqual(vm.tuningFrequency, 107.7)
    }

    func testSearchFilter() {
        let vm = RadioViewModel()
        vm.searchQuery = "MBC"
        
        XCTAssertTrue(vm.filteredStations.allSatisfy { $0.network == .mbc || $0.name.contains("MBC") })
    }
}
