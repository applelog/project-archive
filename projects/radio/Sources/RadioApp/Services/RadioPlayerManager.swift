// Sources/RadioApp/Services/RadioPlayerManager.swift
import Foundation
@preconcurrency import AVFoundation
import MediaPlayer
#if canImport(UIKit)
import UIKit
#endif

public enum PlaybackState: Equatable, Sendable {
    case stopped
    case buffering
    case playing
    case failed(String)
}

public protocol RadioPlayerManaging: AnyObject, Sendable {
    var state: PlaybackState { get }
    var currentStation: RadioStation? { get }
    var currentTrackTitle: String? { get }
    var onStateChanged: ((PlaybackState) -> Void)? { get set }
    var onTrackChanged: ((String?) -> Void)? { get set }
    var onNextStationRequested: (() -> Void)? { get set }
    var onPreviousStationRequested: (() -> Void)? { get set }
    func play(station: RadioStation)
    func stop()
    func togglePlayPause(for station: RadioStation)
    func nextStation()
    func previousStation()
}

public final class RadioPlayerManager: NSObject, RadioPlayerManaging, @unchecked Sendable {
    public static let shared = RadioPlayerManager()
    
    public private(set) var state: PlaybackState = .stopped {
        didSet {
            onStateChanged?(state)
        }
    }
    
    public private(set) var currentStation: RadioStation?
    public private(set) var currentTrackTitle: String? {
        didSet {
            onTrackChanged?(currentTrackTitle)
        }
    }
    
    public var onStateChanged: ((PlaybackState) -> Void)?
    public var onTrackChanged: ((String?) -> Void)?
    public var onNextStationRequested: (() -> Void)?
    public var onPreviousStationRequested: (() -> Void)?
    
    private var player: AVPlayer?
    private var playerItemObserver: NSKeyValueObservation?
    private var metadataOutput: AVPlayerItemMetadataOutput?
    
    public override init() {
        super.init()
        setupAudioSession()
        setupRemoteCommands()
    }
    
    public func setupAudioSession() {
        #if os(iOS) || os(tvOS) || os(watchOS) || os(visionOS)
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playback, mode: .default, options: [.allowBluetooth, .allowBluetoothA2DP, .allowAirPlay])
            try session.setActive(true, options: [])
        } catch {
            print("Audio session setup failed: \(error)")
        }
        #endif
    }
    
    public func play(station: RadioStation) {
        currentStation = station
        currentTrackTitle = nil
        state = .buffering
        
        #if os(iOS) || os(tvOS) || os(watchOS) || os(visionOS)
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playback, mode: .default, options: [.allowBluetooth, .allowBluetoothA2DP, .allowAirPlay])
            try session.setActive(true, options: [])
        } catch {
            print("Reactivating audio session error: \(error)")
        }
        #endif
        
        let asset = AVURLAsset(
            url: station.streamURL,
            options: ["AVURLAssetHTTPHeaderFieldsKey": ["User-Agent": "Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X) AppleWebKit/605.1.15"]]
        )
        let playerItem = AVPlayerItem(asset: asset)
        playerItem.preferredForwardBufferDuration = 1.0
        
        let output = AVPlayerItemMetadataOutput(identifiers: nil)
        output.setDelegate(self, queue: .main)
        playerItem.add(output)
        self.metadataOutput = output
        
        playerItemObserver = playerItem.observe(\.status, options: [.new, .initial]) { [weak self] item, _ in
            guard let self else { return }
            switch item.status {
            case .readyToPlay:
                self.player?.volume = 1.0
                self.player?.isMuted = false
                self.player?.playImmediately(atRate: 1.0)
                self.state = .playing
                self.updateNowPlayingInfo(station: station, trackTitle: self.currentTrackTitle)
            case .failed:
                self.state = .failed(item.error?.localizedDescription ?? "스트림 연결 실패")
            default:
                break
            }
        }
        
        if player == nil {
            let p = AVPlayer(playerItem: playerItem)
            p.automaticallyWaitsToMinimizeStalling = false
            p.volume = 1.0
            p.isMuted = false
            player = p
        } else {
            player?.volume = 1.0
            player?.isMuted = false
            player?.automaticallyWaitsToMinimizeStalling = false
            player?.replaceCurrentItem(with: playerItem)
        }
        
        player?.playImmediately(atRate: 1.0)
        updateNowPlayingInfo(station: station, trackTitle: currentTrackTitle)
    }
    
    public func stop() {
        player?.pause()
        player?.replaceCurrentItem(with: nil)
        playerItemObserver?.invalidate()
        playerItemObserver = nil
        metadataOutput = nil
        currentTrackTitle = nil
        state = .stopped
        MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
        #if os(iOS)
        MPNowPlayingInfoCenter.default().playbackState = .stopped
        #endif
    }
    
    public func togglePlayPause(for station: RadioStation) {
        if currentStation?.id == station.id && state == .playing {
            stop()
        } else {
            play(station: station)
        }
    }
    
    public func nextStation() {
        let allStations = KoreanStationPresets.allStations
        guard !allStations.isEmpty else { return }
        
        let next: RadioStation
        if let current = currentStation, let idx = allStations.firstIndex(where: { $0.id == current.id }) {
            let nextIdx = (idx + 1) % allStations.count
            next = allStations[nextIdx]
        } else {
            next = allStations[0]
        }
        
        play(station: next)
        onNextStationRequested?()
    }
    
    public func previousStation() {
        let allStations = KoreanStationPresets.allStations
        guard !allStations.isEmpty else { return }
        
        let prev: RadioStation
        if let current = currentStation, let idx = allStations.firstIndex(where: { $0.id == current.id }) {
            let prevIdx = (idx - 1 + allStations.count) % allStations.count
            prev = allStations[prevIdx]
        } else {
            prev = allStations[allStations.count - 1]
        }
        
        play(station: prev)
        onPreviousStationRequested?()
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
            guard let self else { return .noActionableNowPlayingItem }
            self.nextStation()
            return .success
        }
        
        commandCenter.previousTrackCommand.isEnabled = true
        commandCenter.previousTrackCommand.addTarget { [weak self] _ in
            guard let self else { return .noActionableNowPlayingItem }
            self.previousStation()
            return .success
        }

        commandCenter.skipForwardCommand.isEnabled = true
        commandCenter.skipForwardCommand.addTarget { [weak self] _ in
            guard let self else { return .noActionableNowPlayingItem }
            self.nextStation()
            return .success
        }

        commandCenter.skipBackwardCommand.isEnabled = true
        commandCenter.skipBackwardCommand.addTarget { [weak self] _ in
            guard let self else { return .noActionableNowPlayingItem }
            self.previousStation()
            return .success
        }

        commandCenter.seekForwardCommand.isEnabled = true
        commandCenter.seekForwardCommand.addTarget { [weak self] _ in
            guard let self else { return .noActionableNowPlayingItem }
            self.nextStation()
            return .success
        }

        commandCenter.seekBackwardCommand.isEnabled = true
        commandCenter.seekBackwardCommand.addTarget { [weak self] _ in
            guard let self else { return .noActionableNowPlayingItem }
            self.previousStation()
            return .success
        }
    }
    
    private func updateNowPlayingInfo(station: RadioStation, trackTitle: String? = nil) {
        var info = [String: Any]()
        if let trackTitle, !trackTitle.isEmpty {
            info[MPMediaItemPropertyTitle] = trackTitle
            info[MPMediaItemPropertyArtist] = "\(station.name) • \(station.frequencyLabel)"
        } else {
            info[MPMediaItemPropertyTitle] = station.name
            info[MPMediaItemPropertyArtist] = station.network.displayName
        }
        info[MPMediaItemPropertyAlbumTitle] = station.frequencyLabel
        info[MPNowPlayingInfoPropertyIsLiveStream] = 1.0
        info[MPNowPlayingInfoPropertyPlaybackRate] = (state == .playing ? 1.0 : 0.0)
        info[MPNowPlayingInfoPropertyDefaultPlaybackRate] = 1.0
        info[MPNowPlayingInfoPropertyElapsedPlaybackTime] = 0.0
        
        #if canImport(UIKit)
        let artworkSize = CGSize(width: 512, height: 512)
        let artwork = MPMediaItemArtwork(boundsSize: artworkSize) { size in
            let renderer = UIGraphicsImageRenderer(size: size)
            return renderer.image { ctx in
                let rect = CGRect(origin: .zero, size: size)
                
                // Dark background
                UIColor(red: 0.08, green: 0.09, blue: 0.12, alpha: 1.0).setFill()
                ctx.fill(rect)
                
                // Rounded inner card
                let cardRect = CGRect(x: 32, y: 32, width: size.width - 64, height: size.height - 64)
                let cardPath = UIBezierPath(roundedRect: cardRect, cornerRadius: 28)
                UIColor(white: 0.16, alpha: 1.0).setFill()
                cardPath.fill()
                
                // Accent border
                UIColor(white: 0.28, alpha: 1.0).setStroke()
                cardPath.lineWidth = 2
                cardPath.stroke()
                
                // Station name string
                let nameAttrs: [NSAttributedString.Key: Any] = [
                    .font: UIFont.systemFont(ofSize: 40, weight: .bold),
                    .foregroundColor: UIColor.white
                ]
                let nameStr = NSAttributedString(string: station.name, attributes: nameAttrs)
                let nameSize = nameStr.size()
                nameStr.draw(at: CGPoint(x: (size.width - nameSize.width) / 2, y: size.height / 2 - 44))
                
                // Frequency string
                let freqAttrs: [NSAttributedString.Key: Any] = [
                    .font: UIFont.monospacedDigitSystemFont(ofSize: 34, weight: .bold),
                    .foregroundColor: UIColor.systemOrange
                ]
                let freqStr = NSAttributedString(string: station.frequencyLabel, attributes: freqAttrs)
                let freqSize = freqStr.size()
                freqStr.draw(at: CGPoint(x: (size.width - freqSize.width) / 2, y: size.height / 2 + 16))
            }
        }
        info[MPMediaItemPropertyArtwork] = artwork
        #endif
        
        MPNowPlayingInfoCenter.default().nowPlayingInfo = info
        #if os(iOS)
        MPNowPlayingInfoCenter.default().playbackState = (state == .playing ? .playing : (state == .buffering ? .interrupted : .stopped))
        #endif
    }
}

extension RadioPlayerManager: AVPlayerItemMetadataOutputPushDelegate {
    public func metadataOutput(
        _ output: AVPlayerItemMetadataOutput,
        didOutputTimedMetadataGroups groups: [AVTimedMetadataGroup],
        from track: AVPlayerItemTrack?
    ) {
        for group in groups {
            for item in group.items {
                processMetadataItem(item)
            }
        }
    }
    
    private func processMetadataItem(_ item: AVMetadataItem) {
        Task {
            var extracted: String?
            if #available(iOS 16.0, macOS 13.0, *) {
                extracted = try? await item.load(.stringValue)
            }
            #if !os(macOS)
            if extracted == nil {
                extracted = item.stringValue
            }
            #endif
            
            if let str = extracted?.trimmingCharacters(in: .whitespacesAndNewlines), !str.isEmpty {
                await MainActor.run { [weak self] in
                    guard let self else { return }
                    self.currentTrackTitle = str
                    if let station = self.currentStation {
                        self.updateNowPlayingInfo(station: station, trackTitle: str)
                    }
                }
            }
        }
    }
}

