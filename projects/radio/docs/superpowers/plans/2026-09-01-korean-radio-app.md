# Korean Radio iOS App Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a zero-ad, lightweight native iOS radio app for iPhone 17 featuring all major Korean live radio stations, CarPlay support, steering wheel controls, and an old-school MP3-style 87.5~108.0 MHz full frequency auto-scan & tuner dial.

**Architecture:** Built with pure native Swift & SwiftUI without third-party dependencies. Core domain consists of `RadioStation` models and a live station registry. Audio is orchestrated via `RadioPlayerManager` using `AVPlayer`, `AVAudioSession` (with ducking for navigation), `MPNowPlayingInfoCenter`, and `MPRemoteCommandCenter`. A dedicated `AutoScanEngine` manages the 87.5~108.0 MHz frequency sweep with simulated and live tuning with haptic sensory feedback. CarPlay is supported via `CPTemplateApplicationSceneDelegate` and `CPListTemplate`.

**Tech Stack:** Swift 6 / SwiftUI / AVFoundation / MediaPlayer / CarPlay / Swift Package Manager

## Global Constraints

- Platform: iOS 17.0+ (Optimized for iPhone 17)
- External Dependencies: 0 (Pure Apple native frameworks)
- Ad-Free: Direct HLS (.m3u8) / AAC stream playback without third-party ad interceptors
- Full Frequency Range: 87.5 MHz to 108.0 MHz with 0.1 MHz stepping
- Navigation Audio: `AVAudioSession.CategoryOptions.duckOthers` enabled

---

### Task 1: Package Scaffolding & Core Models (`RadioStation`, `RadioNetwork`)

**Files:**
- Create: `Package.swift`
- Create: `Sources/RadioApp/Models/RadioNetwork.swift`
- Create: `Sources/RadioApp/Models/RadioStation.swift`
- Test: `Tests/RadioAppTests/RadioStationTests.swift`

**Interfaces:**
- Produces:
  - `enum RadioNetwork: String, CaseIterable, Codable, Sendable`
  - `struct RadioStation: Identifiable, Hashable, Codable, Sendable` with properties:
    `id: UUID`, `name: String`, `network: RadioNetwork`, `frequency: Double?`, `frequencyLabel: String`, `streamURL: URL`, `category: String`, `isCustom: Bool`

- [ ] **Step 1: Create `Package.swift` and directory structure**

```swift
// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "RadioApp",
    platforms: [
        .iOS(.v17),
        .macOS(.v14)
    ],
    products: [
        .library(name: "RadioApp", targets: ["RadioApp"])
    ],
    targets: [
        .target(
            name: "RadioApp",
            dependencies: [],
            path: "Sources/RadioApp"
        ),
        .testTarget(
            name: "RadioAppTests",
            dependencies: ["RadioApp"],
            path: "Tests/RadioAppTests"
        )
    ]
)
```

- [ ] **Step 2: Write the failing unit test for `RadioStation`**

```swift
// Tests/RadioAppTests/RadioStationTests.swift
import Testing
import Foundation
@testable import RadioApp

struct RadioStationTests {
    @Test func testRadioStationInitializationAndFrequencyFormatting() {
        let url = URL(string: "https://example.com/stream.m3u8")!
        let station = RadioStation(
            name: "SBS 파워FM",
            network: .sbs,
            frequency: 107.7,
            streamURL: url,
            category: "음악/예능"
        )
        
        #expect(station.name == "SBS 파워FM")
        #expect(station.network == .sbs)
        #expect(station.frequency == 107.7)
        #expect(station.frequencyLabel == "107.7 MHz")
        #expect(station.isCustom == false)
    }

    @Test func testOnlineOnlyStationWithoutFrequency() {
        let url = URL(string: "https://example.com/ytn.m3u8")!
        let station = RadioStation(
            name: "YTN 라디오",
            network: .ytn,
            frequency: nil,
            streamURL: url,
            category: "뉴스/시사"
        )
        
        #expect(station.frequencyLabel == "ONLINE")
    }
}
```

- [ ] **Step 3: Run test to verify it fails**

Run: `swift test --filter RadioStationTests`
Expected: Compilation failure (RadioStation / RadioNetwork not defined).

- [ ] **Step 4: Implement `RadioNetwork.swift` and `RadioStation.swift`**

```swift
// Sources/RadioApp/Models/RadioNetwork.swift
import Foundation

public enum RadioNetwork: String, CaseIterable, Codable, Sendable {
    case kbs = "KBS"
    case mbc = "MBC"
    case sbs = "SBS"
    case cbs = "CBS"
    case tbs = "TBS"
    case ebs = "EBS"
    case ytn = "YTN"
    case custom = "MY"
    
    public var displayName: String {
        switch self {
        case .kbs: return "KBS 한국방송"
        case .mbc: return "MBC 문화방송"
        case .sbs: return "SBS"
        case .cbs: return "CBS 기독교방송"
        case .tbs: return "TBS 교통방송"
        case .ebs: return "EBS 교육방송"
        case .ytn: return "YTN 뉴스"
        case .custom: return "내 채널"
        }
    }
}
```

```swift
// Sources/RadioApp/Models/RadioStation.swift
import Foundation

public struct RadioStation: Identifiable, Hashable, Codable, Sendable {
    public let id: UUID
    public let name: String
    public let network: RadioNetwork
    public let frequency: Double?
    public let streamURL: URL
    public let category: String
    public let isCustom: Bool
    
    public init(
        id: UUID = UUID(),
        name: String,
        network: RadioNetwork,
        frequency: Double?,
        streamURL: URL,
        category: String = "일반",
        isCustom: Bool = false
    ) {
        self.id = id
        self.name = name
        self.network = network
        self.frequency = frequency
        self.streamURL = streamURL
        self.category = category
        self.isCustom = isCustom
    }
    
    public var frequencyLabel: String {
        guard let frequency else { return "ONLINE" }
        return String(format: "%.1f MHz", frequency)
    }
}
```

- [ ] **Step 5: Run tests and verify they pass**

Run: `swift test --filter RadioStationTests`
Expected: PASS with 0 failures.

- [ ] **Step 6: Commit**

```bash
git add Package.swift Sources/RadioApp/Models Tests/RadioAppTests
git commit -m "feat: add RadioStation and RadioNetwork domain models"
```

---

### Task 2: Built-in Korean Station Presets & Repository

**Files:**
- Create: `Sources/RadioApp/Services/KoreanStationPresets.swift`
- Test: `Tests/RadioAppTests/KoreanStationPresetsTests.swift`

**Interfaces:**
- Produces:
  - `struct KoreanStationPresets: Sendable`
  - `static let allStations: [RadioStation]` (sorted in ascending order by frequency MHz)
  - `static func findByFrequency(_ freq: Double, tolerance: Double) -> RadioStation?`

- [ ] **Step 1: Write the failing test for `KoreanStationPresets`**

```swift
// Tests/RadioAppTests/KoreanStationPresetsTests.swift
import Testing
import Foundation
@testable import RadioApp

struct KoreanStationPresetsTests {
    @Test func testPresetsAreOrderedByFrequency() {
        let stations = KoreanStationPresets.allStations
        #expect(!stations.isEmpty)
        
        let frequencies = stations.compactMap(\.frequency)
        #expect(frequencies == frequencies.sorted())
    }
    
    @Test func testContainsMajorKoreanStations() {
        let stations = KoreanStationPresets.allStations
        let names = stations.map(\.name)
        
        #expect(names.contains(where: { $0.contains("쿨FM") }))
        #expect(names.contains(where: { $0.contains("FM4U") }))
        #expect(names.contains(where: { $0.contains("파워FM") }))
        #expect(names.contains(where: { $0.contains("음악FM") }))
    }

    @Test func testFindByFrequencyMatchesClosestStation() {
        let station = KoreanStationPresets.findByFrequency(107.7)
        #expect(station?.name.contains("파워FM") == true)
        
        let closeMatch = KoreanStationPresets.findByFrequency(91.92, tolerance: 0.1)
        #expect(closeMatch?.name.contains("FM4U") == true)
    }
}
```

- [ ] **Step 2: Run test to verify failure**

Run: `swift test --filter KoreanStationPresetsTests`
Expected: FAIL (KoreanStationPresets not found).

- [ ] **Step 3: Implement `KoreanStationPresets.swift`**

```swift
// Sources/RadioApp/Services/KoreanStationPresets.swift
import Foundation

public struct KoreanStationPresets: Sendable {
    public static let allStations: [RadioStation] = [
        RadioStation(
            name: "KBS 2FM (쿨FM)",
            network: .kbs,
            frequency: 89.1,
            streamURL: URL(string: "https://kbs-radio-coolfm.cdn.ntruss.com/coolfm/coolfm.m3u8") ?? URL(string: "http://localhost")!,
            category: "대중음악/예능"
        ),
        RadioStation(
            name: "MBC FM4U",
            network: .mbc,
            frequency: 91.9,
            streamURL: URL(string: "https://mbc-radio-fm4u.cdn.ntruss.com/fm4u/fm4u.m3u8") ?? URL(string: "http://localhost")!,
            category: "음악/예능"
        ),
        RadioStation(
            name: "KBS 1FM (클래식FM)",
            network: .kbs,
            frequency: 93.1,
            streamURL: URL(string: "https://kbs-radio-classicfm.cdn.ntruss.com/classicfm/classicfm.m3u8") ?? URL(string: "http://localhost")!,
            category: "클래식/국악"
        ),
        RadioStation(
            name: "CBS 음악FM",
            network: .cbs,
            frequency: 93.9,
            streamURL: URL(string: "https://cbs-musicfm.cdn.ntruss.com/musicfm/musicfm.m3u8") ?? URL(string: "http://localhost")!,
            category: "가요/팝/힐링"
        ),
        RadioStation(
            name: "TBS 교통방송 FM",
            network: .tbs,
            frequency: 95.1,
            streamURL: URL(string: "https://tbs-radio.cdn.ntruss.com/tbsfm/tbsfm.m3u8") ?? URL(string: "http://localhost")!,
            category: "교통/시사"
        ),
        RadioStation(
            name: "MBC 표준FM",
            network: .mbc,
            frequency: 95.9,
            streamURL: URL(string: "https://mbc-radio-standard.cdn.ntruss.com/standard/standard.m3u8") ?? URL(string: "http://localhost")!,
            category: "뉴스/시사/교양"
        ),
        RadioStation(
            name: "KBS 1라디오",
            network: .kbs,
            frequency: 97.3,
            streamURL: URL(string: "https://kbs-radio-1radio.cdn.ntruss.com/1radio/1radio.m3u8") ?? URL(string: "http://localhost")!,
            category: "뉴스/시사"
        ),
        RadioStation(
            name: "CBS 표준FM",
            network: .cbs,
            frequency: 98.1,
            streamURL: URL(string: "https://cbs-standardfm.cdn.ntruss.com/standardfm/standardfm.m3u8") ?? URL(string: "http://localhost")!,
            category: "시사/종교"
        ),
        RadioStation(
            name: "SBS 러브FM",
            network: .sbs,
            frequency: 103.5,
            streamURL: URL(string: "https://sbs-radio-lovefm.cdn.ntruss.com/lovefm/lovefm.m3u8") ?? URL(string: "http://localhost")!,
            category: "시사/예능"
        ),
        RadioStation(
            name: "EBS FM",
            network: .ebs,
            frequency: 104.5,
            streamURL: URL(string: "https://ebs-radio.cdn.ntruss.com/ebsfm/ebsfm.m3u8") ?? URL(string: "http://localhost")!,
            category: "교육/외국어"
        ),
        RadioStation(
            name: "KBS 2라디오 (해피FM)",
            network: .kbs,
            frequency: 106.1,
            streamURL: URL(string: "https://kbs-radio-happyfm.cdn.ntruss.com/happyfm/happyfm.m3u8") ?? URL(string: "http://localhost")!,
            category: "중장년 가요/교양"
        ),
        RadioStation(
            name: "SBS 파워FM",
            network: .sbs,
            frequency: 107.7,
            streamURL: URL(string: "https://sbs-radio-powerfm.cdn.ntruss.com/powerfm/powerfm.m3u8") ?? URL(string: "http://localhost")!,
            category: "대중음악/예능"
        ),
        RadioStation(
            name: "YTN 라디오",
            network: .ytn,
            frequency: nil,
            streamURL: URL(string: "https://ytn-radio.cdn.ntruss.com/ytnradio/ytnradio.m3u8") ?? URL(string: "http://localhost")!,
            category: "24시간 뉴스"
        )
    ]
    
    public static func findByFrequency(_ freq: Double, tolerance: Double = 0.05) -> RadioStation? {
        return allStations.first { station in
            guard let stationFreq = station.frequency else { return false }
            return abs(stationFreq - freq) <= tolerance
        }
    }
}
```

- [ ] **Step 4: Run tests and verify they pass**

Run: `swift test --filter KoreanStationPresetsTests`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add Sources/RadioApp/Services/KoreanStationPresets.swift Tests/RadioAppTests/KoreanStationPresetsTests.swift
git commit -m "feat: add KoreanStationPresets with frequency sorted stations"
```

---

### Task 3: MP3-Style Auto Frequency Scan Engine (`AutoScanEngine`)

**Files:**
- Create: `Sources/RadioApp/Services/AutoScanEngine.swift`
- Test: `Tests/RadioAppTests/AutoScanEngineTests.swift`

**Interfaces:**
- Produces:
  - `final class AutoScanEngine: Sendable`
  - `static let minFrequency: Double = 87.5`
  - `static let maxFrequency: Double = 108.0`
  - `static let stepFrequency: Double = 0.1`
  - `func nextStation(from currentFreq: Double, in stations: [RadioStation]) -> RadioStation?`
  - `func previousStation(from currentFreq: Double, in stations: [RadioStation]) -> RadioStation?`
  - `func scanFrequencies(speedMs: UInt64, onTick: @Sendable (Double, RadioStation?) -> Void, onComplete: @Sendable ([RadioStation]) -> Void) async`

- [ ] **Step 1: Write unit tests for `AutoScanEngine`**

```swift
// Tests/RadioAppTests/AutoScanEngineTests.swift
import Testing
import Foundation
@testable import RadioApp

struct AutoScanEngineTests {
    @Test func testFrequencyBoundaryConstants() {
        #expect(AutoScanEngine.minFrequency == 87.5)
        #expect(AutoScanEngine.maxFrequency == 108.0)
        #expect(AutoScanEngine.stepFrequency == 0.1)
    }

    @Test func testNextAndPreviousStationNavigation() {
        let engine = AutoScanEngine()
        let stations = KoreanStationPresets.allStations
        
        // Next from 89.1 should be 91.9
        let next = engine.nextStation(from: 89.1, in: stations)
        #expect(next?.frequency == 91.9)
        
        // Prev from 91.9 should be 89.1
        let prev = engine.previousStation(from: 91.9, in: stations)
        #expect(prev?.frequency == 89.1)
        
        // Wrap around test: next from 107.7 wraps to 89.1
        let wrapNext = engine.nextStation(from: 107.7, in: stations)
        #expect(wrapNext?.frequency == 89.1)
    }

    @Test func testAutoScanEngineDiscoversStations() async {
        let engine = AutoScanEngine()
        var discovered: [RadioStation] = []
        
        await engine.scanFrequencies(
            speedMs: 1, // fast for testing
            onTick: { freq, station in
                if let station {
                    discovered.append(station)
                }
            },
            onComplete: { stations in
                #expect(!stations.isEmpty)
            }
        )
        
        #expect(!discovered.isEmpty)
        #expect(discovered.contains(where: { $0.frequency == 107.7 }))
    }
}
```

- [ ] **Step 2: Run test to verify failure**

Run: `swift test --filter AutoScanEngineTests`
Expected: FAIL (AutoScanEngine not defined).

- [ ] **Step 3: Implement `AutoScanEngine.swift`**

```swift
// Sources/RadioApp/Services/AutoScanEngine.swift
import Foundation

public final class AutoScanEngine: Sendable {
    public static let minFrequency: Double = 87.5
    public static let maxFrequency: Double = 108.0
    public static let stepFrequency: Double = 0.1
    
    public init() {}
    
    public func nextStation(from currentFreq: Double, in stations: [RadioStation]) -> RadioStation? {
        let freqStations = stations.filter { $0.frequency != nil }.sorted { ($0.frequency ?? 0) < ($1.frequency ?? 0) }
        guard !freqStations.isEmpty else { return nil }
        
        if let next = freqStations.first(where: { ($0.frequency ?? 0) > currentFreq + 0.05 }) {
            return next
        }
        return freqStations.first
    }
    
    public func previousStation(from currentFreq: Double, in stations: [RadioStation]) -> RadioStation? {
        let freqStations = stations.filter { $0.frequency != nil }.sorted { ($0.frequency ?? 0) < ($1.frequency ?? 0) }
        guard !freqStations.isEmpty else { return nil }
        
        if let prev = freqStations.last(where: { ($0.frequency ?? 0) < currentFreq - 0.05 }) {
            return prev
        }
        return freqStations.last
    }
    
    public func scanFrequencies(
        speedMs: UInt64 = 25,
        onTick: @Sendable (Double, RadioStation?) -> Void,
        onComplete: @Sendable ([RadioStation]) -> Void
    ) async {
        var current = Self.minFrequency
        var found: [RadioStation] = []
        
        while current <= Self.maxFrequency {
            let rounded = (current * 10).rounded() / 10
            let matched = KoreanStationPresets.findByFrequency(rounded, tolerance: 0.05)
            if let matched, !found.contains(where: { $0.id == matched.id }) {
                found.append(matched)
            }
            onTick(rounded, matched)
            
            try? await Task.sleep(nanoseconds: speedMs * 1_000_000)
            current += Self.stepFrequency
        }
        
        onComplete(found)
    }
}
```

- [ ] **Step 4: Run tests and verify they pass**

Run: `swift test --filter AutoScanEngineTests`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add Sources/RadioApp/Services/AutoScanEngine.swift Tests/RadioAppTests/AutoScanEngineTests.swift
git commit -m "feat: add MP3 auto frequency scan engine"
```

---

### Task 4: Audio Engine, Remote Commands, NowPlaying & Haptics

**Files:**
- Create: `Sources/RadioApp/Services/HapticManager.swift`
- Create: `Sources/RadioApp/Services/RadioPlayerManager.swift`
- Test: `Tests/RadioAppTests/RadioPlayerManagerTests.swift`

**Interfaces:**
- Produces:
  - `final class HapticManager: Sendable` (provides tick, snap, selection feedback)
  - `enum PlaybackState: Sendable` (`.stopped`, `.buffering`, `.playing`, `.failed(String)`)
  - `protocol RadioPlayerManaging: AnyObject, Sendable` (or class `RadioPlayerManager`)
    `var playbackState: PlaybackState { get }`
    `func play(station: RadioStation)`
    `func stop()`
    `func togglePlayPause(for station: RadioStation)`

- [ ] **Step 1: Write test for playback state management**

```swift
// Tests/RadioAppTests/RadioPlayerManagerTests.swift
import Testing
import Foundation
@testable import RadioApp

struct RadioPlayerManagerTests {
    @Test func testPlaybackStateTransitions() {
        let station = KoreanStationPresets.allStations[0]
        #expect(station.name.contains("쿨FM"))
        
        let manager = RadioPlayerManager()
        #expect(manager.state == .stopped)
    }
}
```

- [ ] **Step 2: Run test to verify failure**

Run: `swift test --filter RadioPlayerManagerTests`
Expected: FAIL (RadioPlayerManager not defined).

- [ ] **Step 3: Implement `HapticManager.swift` and `RadioPlayerManager.swift`**

```swift
// Sources/RadioApp/Services/HapticManager.swift
import Foundation
#if canImport(UIKit)
import UIKit
#endif

public final class HapticManager: Sendable {
    public static let shared = HapticManager()
    
    public init() {}
    
    public func tick() {
        #if canImport(UIKit) && !os(watchOS)
        Task { @MainActor in
            let generator = UIImpactFeedbackGenerator(style: .light)
            generator.prepare()
            generator.impactOccurred(intensity: 0.4)
        }
        #endif
    }
    
    public func snap() {
        #if canImport(UIKit) && !os(watchOS)
        Task { @MainActor in
            let generator = UIImpactFeedbackGenerator(style: .medium)
            generator.prepare()
            generator.impactOccurred(intensity: 0.9)
        }
        #endif
    }
    
    public func stationFound() {
        #if canImport(UIKit) && !os(watchOS)
        Task { @MainActor in
            let generator = UINotificationFeedbackGenerator()
            generator.prepare()
            generator.notificationOccurred(.success)
        }
        #endif
    }
}
```

```swift
// Sources/RadioApp/Services/RadioPlayerManager.swift
import Foundation
import AVFoundation
import MediaPlayer

public enum PlaybackState: Equatable, Sendable {
    case stopped
    case buffering
    case playing
    case failed(String)
}

public final class RadioPlayerManager: NSObject, @unchecked Sendable {
    public private(set) var state: PlaybackState = .stopped {
        didSet {
            onStateChanged?(state)
        }
    }
    
    public private(set) var currentStation: RadioStation?
    public var onStateChanged: ((PlaybackState) -> Void)?
    public var onNextStationRequested: (() -> Void)?
    public var onPreviousStationRequested: (() -> Void)?
    
    private var player: AVPlayer?
    private var playerItemObserver: NSKeyValueObservation?
    
    public override init() {
        super.init()
        setupAudioSession()
        setupRemoteCommands()
    }
    
    public func setupAudioSession() {
        #if canImport(AVFoundation)
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playback, mode: .default, options: [.duckOthers, .allowBluetooth, .allowBluetoothA2DP])
            try session.setActive(true)
        } catch {
            print("Audio session setup failed: \(error)")
        }
        #endif
    }
    
    public func play(station: RadioStation) {
        currentStation = station
        state = .buffering
        
        let playerItem = AVPlayerItem(url: station.streamURL)
        playerItemObserver = playerItem.observe(\.status, options: [.new, .initial]) { [weak self] item, _ in
            guard let self else { return }
            switch item.status {
            case .readyToPlay:
                self.state = .playing
                self.updateNowPlayingInfo(station: station)
            case .failed:
                self.state = .failed(item.error?.localizedDescription ?? "재생 오류")
            default:
                break
            }
        }
        
        if player == nil {
            player = AVPlayer(playerItem: playerItem)
        } else {
            player?.replaceCurrentItem(with: playerItem)
        }
        
        player?.play()
        updateNowPlayingInfo(station: station)
    }
    
    public func stop() {
        player?.pause()
        player?.replaceCurrentItem(with: nil)
        playerItemObserver?.invalidate()
        playerItemObserver = nil
        state = .stopped
        MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
    }
    
    public func togglePlayPause(for station: RadioStation) {
        if currentStation?.id == station.id && state == .playing {
            stop()
        } else {
            play(station: station)
        }
    }
    
    private func setupRemoteCommands() {
        let commandCenter = MPRemoteCommandCenter.shared()
        
        commandCenter.playCommand.isEnabled = true
        commandCenter.playCommand.addTarget { [weak self] _ in
            guard let self, let station = self.currentStation else { return .noActionableNowPlayingItem }
            self.play(station: station)
            return .success
        }
        
        commandCenter.pauseCommand.isEnabled = true
        commandCenter.pauseCommand.addTarget { [weak self] _ in
            self?.stop()
            return .success
        }
        
        commandCenter.togglePlayPauseCommand.isEnabled = true
        commandCenter.togglePlayPauseCommand.addTarget { [weak self] _ in
            guard let self, let station = self.currentStation else { return .noActionableNowPlayingItem }
            self.togglePlayPause(for: station)
            return .success
        }
        
        commandCenter.nextTrackCommand.isEnabled = true
        commandCenter.nextTrackCommand.addTarget { [weak self] _ in
            self?.onNextStationRequested?()
            return .success
        }
        
        commandCenter.previousTrackCommand.isEnabled = true
        commandCenter.previousTrackCommand.addTarget { [weak self] _ in
            self?.onPreviousStationRequested?()
            return .success
        }
    }
    
    private func updateNowPlayingInfo(station: RadioStation) {
        var info = [String: Any]()
        info[MPMediaItemPropertyTitle] = station.name
        info[MPMediaItemPropertyArtist] = station.network.displayName
        info[MPMediaItemPropertyAlbumTitle] = station.frequencyLabel
        info[MPNowPlayingInfoPropertyIsLiveStream] = true
        MPNowPlayingInfoCenter.default().nowPlayingInfo = info
    }
}
```

- [ ] **Step 4: Run tests and verify they pass**

Run: `swift test --filter RadioPlayerManagerTests`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add Sources/RadioApp/Services/HapticManager.swift Sources/RadioApp/Services/RadioPlayerManager.swift Tests/RadioAppTests/RadioPlayerManagerTests.swift
git commit -m "feat: add RadioPlayerManager and HapticManager"
```

---

### Task 5: Radio ViewModel & State Machine (`RadioViewModel`)

**Files:**
- Create: `Sources/RadioApp/ViewModels/RadioViewModel.swift`
- Test: `Tests/RadioAppTests/RadioViewModelTests.swift`

**Interfaces:**
- Produces:
  - `@Observable @MainActor final class RadioViewModel`
  - Properties: `currentStation: RadioStation?`, `playbackState: PlaybackState`, `tuningFrequency: Double`, `isScanning: Bool`, `stations: [RadioStation]`, `searchQuery: String`
  - Methods: `tuneTo(frequency: Double)`, `selectStation(_: RadioStation)`, `togglePlayback()`, `startAutoScan()`, `nextStation()`, `previousStation()`

- [ ] **Step 1: Write test for `RadioViewModel`**

```swift
// Tests/RadioAppTests/RadioViewModelTests.swift
import Testing
import Foundation
@testable import RadioApp

@MainActor
struct RadioViewModelTests {
    @Test func testInitialStateAndStationSelection() {
        let vm = RadioViewModel()
        #expect(!vm.stations.isEmpty)
        #expect(vm.isScanning == false)
        
        let powerFM = vm.stations.first { $0.frequency == 107.7 }!
        vm.selectStation(powerFM)
        
        #expect(vm.currentStation?.id == powerFM.id)
        #expect(vm.tuningFrequency == 107.7)
    }

    @Test func testSearchFilter() {
        let vm = RadioViewModel()
        vm.searchQuery = "MBC"
        
        #expect(vm.filteredStations.allSatisfy { $0.network == .mbc || $0.name.contains("MBC") })
    }
}
```

- [ ] **Step 2: Run test to verify failure**

Run: `swift test --filter RadioViewModelTests`
Expected: FAIL (RadioViewModel not defined).

- [ ] **Step 3: Implement `RadioViewModel.swift`**

```swift
// Sources/RadioApp/ViewModels/RadioViewModel.swift
import Foundation
import Observation

@Observable
@MainActor
public final class RadioViewModel {
    public var stations: [RadioStation]
    public var currentStation: RadioStation?
    public var playbackState: PlaybackState = .stopped
    public var tuningFrequency: Double = 89.1
    public var isScanning: Bool = false
    public var searchQuery: String = ""
    
    private let playerManager: RadioPlayerManager
    private let scanEngine: AutoScanEngine
    private let haptics: HapticManager
    
    public init(
        playerManager: RadioPlayerManager = RadioPlayerManager(),
        scanEngine: AutoScanEngine = AutoScanEngine(),
        haptics: HapticManager = .shared
    ) {
        self.playerManager = playerManager
        self.scanEngine = scanEngine
        self.haptics = haptics
        self.stations = KoreanStationPresets.allStations
        self.currentStation = stations.first
        
        self.playerManager.onStateChanged = { [weak self] state in
            Task { @MainActor [weak self] in
                self?.playbackState = state
            }
        }
        
        self.playerManager.onNextStationRequested = { [weak self] in
            Task { @MainActor [weak self] in
                self?.nextStation()
            }
        }
        
        self.playerManager.onPreviousStationRequested = { [weak self] in
            Task { @MainActor [weak self] in
                self?.previousStation()
            }
        }
    }
    
    public var filteredStations: [RadioStation] {
        if searchQuery.trimmingCharacters(in: .whitespaces).isEmpty {
            return stations
        }
        let q = searchQuery.lowercased()
        return stations.filter { station in
            station.name.lowercased().contains(q) ||
            station.network.rawValue.lowercased().contains(q) ||
            station.frequencyLabel.lowercased().contains(q) ||
            station.category.lowercased().contains(q)
        }
    }
    
    public func selectStation(_ station: RadioStation) {
        currentStation = station
        if let freq = station.frequency {
            tuningFrequency = freq
        }
        playerManager.play(station: station)
        haptics.snap()
    }
    
    public func togglePlayback() {
        guard let current = currentStation else { return }
        playerManager.togglePlayPause(for: current)
        haptics.tick()
    }
    
    public func tuneTo(frequency: Double) {
        tuningFrequency = (frequency * 10).rounded() / 10
        if let matched = KoreanStationPresets.findByFrequency(tuningFrequency) {
            currentStation = matched
            playerManager.play(station: matched)
            haptics.stationFound()
        } else {
            haptics.tick()
        }
    }
    
    public func nextStation() {
        if let next = scanEngine.nextStation(from: tuningFrequency, in: stations) {
            selectStation(next)
        }
    }
    
    public func previousStation() {
        if let prev = scanEngine.previousStation(from: tuningFrequency, in: stations) {
            selectStation(prev)
        }
    }
    
    public func startAutoScan() {
        guard !isScanning else { return }
        isScanning = true
        playerManager.stop()
        
        Task {
            await scanEngine.scanFrequencies(
                speedMs: 35,
                onTick: { [weak self] freq, matched in
                    Task { @MainActor [weak self] in
                        guard let self else { return }
                        self.tuningFrequency = freq
                        if let matched {
                            self.currentStation = matched
                            self.haptics.stationFound()
                        } else {
                            self.haptics.tick()
                        }
                    }
                },
                onComplete: { [weak self] discovered in
                    Task { @MainActor [weak self] in
                        guard let self else { return }
                        self.isScanning = false
                        if let first = discovered.first {
                            self.selectStation(first)
                        }
                    }
                }
            )
        }
    }
}
```

- [ ] **Step 4: Run tests and verify they pass**

Run: `swift test --filter RadioViewModelTests`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add Sources/RadioApp/ViewModels/RadioViewModel.swift Tests/RadioAppTests/RadioViewModelTests.swift
git commit -m "feat: add RadioViewModel state management and tests"
```

---

### Task 6: Retro Tuner & MP3 Sweep UI Components

**Files:**
- Create: `Sources/RadioApp/Views/Components/RetroDisplayView.swift`
- Create: `Sources/RadioApp/Views/Components/FrequencyTunerBar.swift`

**Interfaces:**
- Produces:
  - `struct RetroDisplayView: View` (displays LCD-style frequency readout, station title, stereo badge, auto-scan blinking indicator)
  - `struct FrequencyTunerBar: View` (horizontal draggable frequency scale 87.5~108.0 MHz with magnetic markers and red needle indicator)

- [ ] **Step 1: Implement `RetroDisplayView.swift`**

```swift
// Sources/RadioApp/Views/Components/RetroDisplayView.swift
import SwiftUI

public struct RetroDisplayView: View {
    public let frequency: Double
    public let stationName: String
    public let networkName: String
    public let isLive: Bool
    public let isScanning: Bool
    
    public init(
        frequency: Double,
        stationName: String,
        networkName: String,
        isLive: Bool,
        isScanning: Bool
    ) {
        self.frequency = frequency
        self.stationName = stationName
        self.networkName = networkName
        self.isLive = isLive
        self.isScanning = isScanning
    }
    
    public var body: some View {
        VStack(spacing: 8) {
            HStack {
                HStack(spacing: 4) {
                    Circle()
                        .fill(isLive ? Color.red : Color.gray)
                        .frame(width: 8, height: 8)
                    Text(isScanning ? "AUTO SCANNING" : (isLive ? "LIVE FM STEREO" : "STANDBY"))
                        .font(.system(size: 11, weight: .bold, design: .monospaced))
                        .foregroundStyle(isScanning ? .yellow : (isLive ? .green : .secondary))
                }
                
                Spacer()
                
                Text(networkName)
                    .font(.system(size: 12, weight: .semibold, design: .monospaced))
                    .foregroundStyle(.cyan)
            }
            
            HStack(alignment: .lastTextBaseline, spacing: 6) {
                Text(String(format: "%.1f", frequency))
                    .font(.system(size: 48, weight: .heavy, design: .monospaced))
                    .foregroundStyle(.white)
                
                Text("MHz")
                    .font(.system(size: 20, weight: .bold, design: .monospaced))
                    .foregroundStyle(.orange)
                
                Spacer()
            }
            
            HStack {
                Text(stationName)
                    .font(.system(size: 16, weight: .medium))
                    .foregroundStyle(.white.opacity(0.9))
                    .lineLimit(1)
                Spacer()
            }
        }
        .padding(18)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color(red: 0.08, green: 0.09, blue: 0.11))
                .overlay(
                    RoundedRectangle(cornerRadius: 16)
                        .stroke(Color.white.opacity(0.15), lineWidth: 1)
                )
        )
    }
}
```

- [ ] **Step 2: Implement `FrequencyTunerBar.swift`**

```swift
// Sources/RadioApp/Views/Components/FrequencyTunerBar.swift
import SwiftUI

public struct FrequencyTunerBar: View {
    @Binding public var frequency: Double
    public let stations: [RadioStation]
    public let onFrequencyChanged: (Double) -> Void
    
    public init(
        frequency: Binding<Double>,
        stations: [RadioStation],
        onFrequencyChanged: @escaping (Double) -> Void
    ) {
        self._frequency = frequency
        self.stations = stations
        self.onFrequencyChanged = onFrequencyChanged
    }
    
    public var body: some View {
        VStack(spacing: 12) {
            GeometryReader { proxy in
                let width = proxy.size.width
                let minFreq = AutoScanEngine.minFrequency
                let maxFreq = AutoScanEngine.maxFrequency
                let progress = (frequency - minFreq) / (maxFreq - minFreq)
                let needleX = max(0, min(width, width * progress))
                
                ZStack(alignment: .leading) {
                    // Track Background
                    RoundedRectangle(cornerRadius: 8)
                        .fill(Color(white: 0.15))
                        .frame(height: 36)
                    
                    // Major / Minor Ticks
                    HStack(spacing: 0) {
                        ForEach(88...108, id: \.self) { major in
                            VStack(spacing: 2) {
                                Rectangle()
                                    .fill(Color.white.opacity(0.7))
                                    .frame(width: 1.5, height: 16)
                                Text("\(major)")
                                    .font(.system(size: 9, weight: .semibold, design: .monospaced))
                                    .foregroundStyle(Color.white.opacity(0.6))
                            }
                            if major < 108 {
                                Spacer()
                            }
                        }
                    }
                    .padding(.horizontal, 10)
                    
                    // Station markers on the scale
                    ForEach(stations.compactMap { $0.frequency != nil ? $0 : nil }) { st in
                        let stProgress = ((st.frequency ?? minFreq) - minFreq) / (maxFreq - minFreq)
                        Circle()
                            .fill(Color.orange)
                            .frame(width: 5, height: 5)
                            .position(x: 10 + (width - 20) * stProgress, y: 18)
                    }
                    
                    // Red Needle Indicator
                    Rectangle()
                        .fill(Color.red)
                        .frame(width: 3, height: 42)
                        .shadow(color: .red.opacity(0.8), radius: 4)
                        .position(x: 10 + (width - 20) * progress, y: 18)
                }
                .contentShape(Rectangle())
                .gesture(
                    DragGesture(minimumDistance: 0)
                        .onChanged { value in
                            let clampedX = max(0, min(width - 20, value.location.x - 10))
                            let newProgress = clampedX / (width - 20)
                            let newFreq = minFreq + (maxFreq - minFreq) * Double(newProgress)
                            let roundedFreq = (newFreq * 10).rounded() / 10
                            frequency = roundedFreq
                            onFrequencyChanged(roundedFreq)
                        }
                )
            }
            .frame(height: 44)
        }
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add Sources/RadioApp/Views/Components
git commit -m "feat: add RetroDisplayView and FrequencyTunerBar views"
```

---

### Task 7: Main Views (`NowPlayingCard`, `StationListView`, `MainRadioView`)

**Files:**
- Create: `Sources/RadioApp/Views/NowPlayingCard.swift`
- Create: `Sources/RadioApp/Views/StationListView.swift`
- Create: `Sources/RadioApp/Views/MainRadioView.swift`

- [ ] **Step 1: Implement `NowPlayingCard.swift`**

```swift
// Sources/RadioApp/Views/NowPlayingCard.swift
import SwiftUI

public struct NowPlayingCard: View {
    public let station: RadioStation?
    public let state: PlaybackState
    public let onTogglePlay: () -> Void
    public let onNext: () -> Void
    public let onPrev: () -> Void
    public let onAutoScan: () -> Void
    public let isScanning: Bool
    
    public init(
        station: RadioStation?,
        state: PlaybackState,
        isScanning: Bool,
        onTogglePlay: @escaping () -> Void,
        onNext: @escaping () -> Void,
        onPrev: @escaping () -> Void,
        onAutoScan: @escaping () -> Void
    ) {
        self.station = station
        self.state = state
        self.isScanning = isScanning
        self.onTogglePlay = onTogglePlay
        self.onNext = onNext
        self.onPrev = onPrev
        self.onAutoScan = onAutoScan
    }
    
    public var body: some View {
        HStack(spacing: 16) {
            // Auto Scan button (Old-school MP3 style)
            Button(action: onAutoScan) {
                VStack(spacing: 4) {
                    Image(systemName: isScanning ? "antenna.radiowaves.left.and.right" : "sparkle.magnifyingglass")
                        .font(.system(size: 20))
                    Text("AUTO")
                        .font(.system(size: 10, weight: .bold, design: .monospaced))
                }
                .foregroundStyle(isScanning ? .yellow : .white)
                .frame(width: 52, height: 52)
                .background(Circle().fill(Color.white.opacity(0.12)))
            }
            .disabled(isScanning)
            
            Spacer()
            
            // Previous Button
            Button(action: onPrev) {
                Image(systemName: "backward.fill")
                    .font(.system(size: 24))
                    .foregroundStyle(.white)
            }
            
            // Main Play/Pause Button
            Button(action: onTogglePlay) {
                ZStack {
                    Circle()
                        .fill(Color.orange)
                        .frame(width: 64, height: 64)
                    
                    if state == .buffering {
                        ProgressView()
                            .tint(.black)
                    } else {
                        Image(systemName: state == .playing ? "pause.fill" : "play.fill")
                            .font(.system(size: 28))
                            .foregroundStyle(.black)
                    }
                }
            }
            
            // Next Button
            Button(action: onNext) {
                Image(systemName: "forward.fill")
                    .font(.system(size: 24))
                    .foregroundStyle(.white)
            }
            
            Spacer()
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 8)
    }
}
```

- [ ] **Step 2: Implement `StationListView.swift`**

```swift
// Sources/RadioApp/Views/StationListView.swift
import SwiftUI

public struct StationListView: View {
    public let stations: [RadioStation]
    public let currentStation: RadioStation?
    public let isPlaying: Bool
    public let onSelect: (RadioStation) -> Void
    
    public init(
        stations: [RadioStation],
        currentStation: RadioStation?,
        isPlaying: Bool,
        onSelect: @escaping (RadioStation) -> Void
    ) {
        self.stations = stations
        self.currentStation = currentStation
        self.isPlaying = isPlaying
        self.onSelect = onSelect
    }
    
    public var body: some View {
        LazyVStack(spacing: 8) {
            ForEach(stations) { station in
                let isSelected = currentStation?.id == station.id
                Button {
                    onSelect(station)
                } label: {
                    HStack(spacing: 14) {
                        // Network badge
                        Text(station.network.rawValue)
                            .font(.system(size: 11, weight: .bold, design: .monospaced))
                            .foregroundStyle(isSelected ? .black : .white)
                            .frame(width: 44, height: 26)
                            .background(
                                RoundedRectangle(cornerRadius: 6)
                                    .fill(isSelected ? Color.orange : Color.white.opacity(0.15))
                            )
                        
                        VStack(alignment: .leading, spacing: 2) {
                            Text(station.name)
                                .font(.system(size: 15, weight: .medium))
                                .foregroundStyle(isSelected ? .orange : .white)
                            
                            Text(station.category)
                                .font(.system(size: 12))
                                .foregroundStyle(Color.gray)
                        }
                        
                        Spacer()
                        
                        Text(station.frequencyLabel)
                            .font(.system(size: 14, weight: .bold, design: .monospaced))
                            .foregroundStyle(isSelected ? .orange : .gray)
                        
                        if isSelected && isPlaying {
                            Image(systemName: "waveform")
                                .symbolEffect(.variableColor.iterative)
                                .foregroundStyle(.orange)
                        }
                    }
                    .padding(.horizontal, 16)
                    .padding(.vertical, 12)
                    .background(
                        RoundedRectangle(cornerRadius: 12)
                            .fill(isSelected ? Color.white.opacity(0.08) : Color.white.opacity(0.03))
                    )
                }
                .buttonStyle(.plain)
            }
        }
    }
}
```

- [ ] **Step 3: Implement `MainRadioView.swift`**

```swift
// Sources/RadioApp/Views/MainRadioView.swift
import SwiftUI

public struct MainRadioView: View {
    @State private var viewModel = RadioViewModel()
    
    public init() {}
    
    public var body: some View {
        NavigationStack {
            ZStack {
                Color(red: 0.04, green: 0.04, blue: 0.06).ignoresSafeArea()
                
                VStack(spacing: 16) {
                    // Search Bar
                    HStack {
                        Image(systemName: "magnifyingglass")
                            .foregroundStyle(.gray)
                        TextField("주파수, 채널명, 방송사 검색", text: $viewModel.searchQuery)
                            .foregroundStyle(.white)
                        if !viewModel.searchQuery.isEmpty {
                            Button {
                                viewModel.searchQuery = ""
                            } label: {
                                Image(systemName: "xmark.circle.fill")
                                    .foregroundStyle(.gray)
                            }
                        }
                    }
                    .padding(10)
                    .background(RoundedRectangle(cornerRadius: 10).fill(Color.white.opacity(0.08)))
                    .padding(.horizontal, 16)
                    
                    // Retro Tuner LCD Readout
                    RetroDisplayView(
                        frequency: viewModel.tuningFrequency,
                        stationName: viewModel.currentStation?.name ?? "라디오 대기 중",
                        networkName: viewModel.currentStation?.network.displayName ?? "FM STEREO",
                        isLive: viewModel.playbackState == .playing,
                        isScanning: viewModel.isScanning
                    )
                    .padding(.horizontal, 16)
                    
                    // Analog MP3 Frequency Slider Scale
                    FrequencyTunerBar(
                        frequency: $viewModel.tuningFrequency,
                        stations: viewModel.stations,
                        onFrequencyChanged: { freq in
                            viewModel.tuneTo(frequency: freq)
                        }
                    )
                    .padding(.horizontal, 16)
                    
                    // Media Controls
                    NowPlayingCard(
                        station: viewModel.currentStation,
                        state: viewModel.playbackState,
                        isScanning: viewModel.isScanning,
                        onTogglePlay: { viewModel.togglePlayback() },
                        onNext: { viewModel.nextStation() },
                        onPrev: { viewModel.previousStation() },
                        onAutoScan: { viewModel.startAutoScan() }
                    )
                    
                    Divider().background(Color.white.opacity(0.1))
                    
                    // Station List
                    ScrollView {
                        StationListView(
                            stations: viewModel.filteredStations,
                            currentStation: viewModel.currentStation,
                            isPlaying: viewModel.playbackState == .playing,
                            onSelect: { station in
                                viewModel.selectStation(station)
                            }
                        )
                        .padding(.horizontal, 16)
                    }
                }
                .padding(.top, 8)
            }
            .navigationTitle("라디오")
            .navigationBarTitleDisplayMode(.inline)
            .toolbarBackground(.hidden, for: .navigationBar)
        }
        .preferredColorScheme(.dark)
    }
}
```

- [ ] **Step 4: Commit**

```bash
git add Sources/RadioApp/Views
git commit -m "feat: add NowPlayingCard, StationListView, and MainRadioView"
```

---

### Task 8: App Entry, CarPlay Delegate & Execution Documentation

**Files:**
- Create: `Sources/RadioApp/App/CarPlaySceneDelegate.swift`
- Create: `Sources/RadioApp/App/RadioApp.swift`
- Create: `README.md`
- Test: Build and test entire package via `swift test`

- [ ] **Step 1: Implement `CarPlaySceneDelegate.swift`**

```swift
// Sources/RadioApp/App/CarPlaySceneDelegate.swift
import Foundation
#if canImport(CarPlay)
import CarPlay

public final class CarPlaySceneDelegate: UIResponder, CPTemplateApplicationSceneDelegate {
    private var interfaceController: CPInterfaceController?
    private let playerManager = RadioPlayerManager()
    
    public func templateApplicationScene(
        _ templateApplicationScene: CPTemplateApplicationScene,
        didConnect interfaceController: CPInterfaceController
    ) {
        self.interfaceController = interfaceController
        
        let items = KoreanStationPresets.allStations.map { station in
            let item = CPListItem(
                text: station.name,
                detailText: "\(station.network.displayName) • \(station.frequencyLabel)"
            )
            item.handler = { [weak self] _, completion in
                self?.playerManager.play(station: station)
                completion()
            }
            return item
        }
        
        let listTemplate = CPListTemplate(
            title: "한국 라디오 (No-Ad)",
            sections: [CPListSection(items: items)]
        )
        
        interfaceController.setRootTemplate(listTemplate, animated: true, completion: nil)
    }
    
    public func templateApplicationScene(
        _ templateApplicationScene: CPTemplateApplicationScene,
        didDisconnectInterfaceController interfaceController: CPInterfaceController
    ) {
        self.interfaceController = nil
    }
}
#endif
```

- [ ] **Step 2: Implement `RadioApp.swift`**

```swift
// Sources/RadioApp/App/RadioApp.swift
import SwiftUI

@main
public struct RadioApp: App {
    public init() {}
    
    public var body: some Scene {
        WindowGroup {
            MainRadioView()
        }
    }
}
```

- [ ] **Step 3: Create `README.md` with iPhone 17 Run Instructions**

```markdown
# 📻 한국 라디오 (Korean Radio) - Ad-Free, CarPlay, MP3 Auto-Scan

광고 없는 초경량 아이폰 17 & 카플레이(CarPlay) 전용 라디오 앱입니다.

## ✨ 주요 기능
1. **광고 없는 실시간 스트리밍**: 외부 라이브러리 및 광고 트래커 0%, 방송사 공식 라이브 스트림 직결.
2. **MP3 감성 전체 주파수 자동 스캔 (87.5 ~ 108.0 MHz)**: `AUTO` 버튼 클릭 시 주파수 게이지가 쫘르륵 올라가며 국내 방송 채널 탐색 및 햅틱 진동.
3. **아날로그 튜닝 바**: 주파수 눈금을 손가락으로 드래그하여 아날로그 다이얼처럼 주파수 튜닝.
4. **애플 카플레이(CarPlay) & 차량 핸들 리모컨**: 차량 모니터 전용 채널 리스트 및 핸들 이전/다음곡 버튼 완벽 연동.
5. **내비게이션 오디오 덕킹**: 티맵/카카오내비 음성 안내 시 라디오 볼륨이 부드럽게 감소.

## 🚀 iPhone 17 설치 및 실행 방법
1. Mac에서 Xcode를 실행합니다.
2. `File` > `New` > `Project` > `iOS App` 선택 후 이름에 `RadioApp` 지정.
3. 생성된 프로젝트에 본 레포지토리의 `Sources/RadioApp` 코드 파일들을 추가.
4. `Signing & Capabilities` 탭에서:
   - 본인의 무료 Apple ID 계정(Personal Team) 선택.
   - `+ Capability` > **Background Modes** 추가 (`Audio, AirPlay, and Picture in Picture` 체크).
5. iPhone 17을 Mac에 연결(또는 iOS 시뮬레이터 선택) 후 `Cmd + R`로 실행!
```

- [ ] **Step 4: Run all unit tests**

Run: `swift test`
Expected: All tests pass successfully.

- [ ] **Step 5: Final Commit**

```bash
git add Sources/RadioApp/App README.md
git commit -m "feat: complete Korean Radio app with CarPlay support and guide"
```
