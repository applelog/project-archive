// Tests/RadioAppTests/AutoScanEngineTests.swift
import XCTest
import Foundation
@testable import RadioApp

private final class ThreadSafeStationList: @unchecked Sendable {
    private var list: [RadioStation] = []
    private let lock = NSLock()
    
    func append(_ station: RadioStation) {
        lock.lock()
        defer { lock.unlock() }
        list.append(station)
    }
    
    var stations: [RadioStation] {
        lock.lock()
        defer { lock.unlock() }
        return list
    }
}

final class AutoScanEngineTests: XCTestCase {
    func testFrequencyBoundaryConstants() {
        XCTAssertEqual(AutoScanEngine.minFrequency, 87.5)
        XCTAssertEqual(AutoScanEngine.maxFrequency, 108.0)
        XCTAssertEqual(AutoScanEngine.stepFrequency, 0.1)
    }

    func testNextAndPreviousStationNavigation() {
        let engine = AutoScanEngine()
        let stations = KoreanStationPresets.allStations
        
        let next = engine.nextStation(from: 89.1, in: stations)
        XCTAssertEqual(next?.frequency, 91.9)
        
        let prev = engine.previousStation(from: 91.9, in: stations)
        XCTAssertEqual(prev?.frequency, 89.1)
        
        let wrapNext = engine.nextStation(from: 107.7, in: stations)
        XCTAssertEqual(wrapNext?.frequency, 89.1)
    }

    func testAutoScanEngineDiscoversStations() async {
        let engine = AutoScanEngine()
        let collector = ThreadSafeStationList()
        
        await engine.scanFrequencies(
            speedMs: 1,
            onTick: { freq, station in
                if let station {
                    collector.append(station)
                }
            },
            onComplete: { stations in
                XCTAssertFalse(stations.isEmpty)
            }
        )
        
        let discovered = collector.stations
        XCTAssertFalse(discovered.isEmpty)
        XCTAssertTrue(discovered.contains(where: { $0.frequency == 107.7 }))
    }
}
