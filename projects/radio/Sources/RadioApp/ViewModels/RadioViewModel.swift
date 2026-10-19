// Sources/RadioApp/ViewModels/RadioViewModel.swift
import Foundation
import Observation

@Observable
@MainActor
public final class RadioViewModel {
    public var stations: [RadioStation]
    public var currentStation: RadioStation?
    
    // Distinct Radio Broadcast Info
    public var currentProgramTitle: String?
    public var currentDJName: String?
    public var currentProgramPhotoURL: URL?
    
    // Distinct Music Track & Spotify Link (Only populated when a song is playing)
    public var currentSong: EnrichedSongInfo?
    public var currentTrackTitle: String?
    
    public var playbackState: PlaybackState = .stopped
    public var tuningFrequency: Double = 89.1
    public var isScanning: Bool = false
    public var searchQuery: String = ""
    public var scannedSignalDetected: Bool = false
    
    private let playerManager: any RadioPlayerManaging
    private let scanEngine: AutoScanEngine
    private let haptics: HapticManager
    private let enricher: SongMetadataEnricher
    private let onAirFetcher: KoreanRadioOnAirFetcher
    private var metadataTask: Task<Void, Never>?
    
    public init(
        playerManager: any RadioPlayerManaging = RadioPlayerManager.shared,
        scanEngine: AutoScanEngine = AutoScanEngine(),
        haptics: HapticManager = .shared,
        enricher: SongMetadataEnricher = .shared,
        onAirFetcher: KoreanRadioOnAirFetcher = .shared
    ) {
        self.playerManager = playerManager
        self.scanEngine = scanEngine
        self.haptics = haptics
        self.enricher = enricher
        self.onAirFetcher = onAirFetcher
        self.stations = KoreanStationPresets.allStations
        self.currentStation = stations.first
        if let initialFreq = stations.first?.frequency {
            self.tuningFrequency = initialFreq
        }
        
        self.playerManager.onStateChanged = { [weak self] state in
            Task { @MainActor [weak self] in
                guard let self else { return }
                self.playbackState = state
                if let current = self.playerManager.currentStation, self.currentStation?.id != current.id {
                    self.currentStation = current
                    if let freq = current.frequency {
                        self.tuningFrequency = freq
                    }
                    self.fetchOnAirBroadcast(for: current)
                }
                if state == .playing {
                    self.startMetadataRefreshLoop()
                } else if state == .stopped {
                    self.stopMetadataRefreshLoop()
                }
                #if os(iOS)
                if let station = self.currentStation {
                    RadioActivityManager.shared.startOrUpdateActivity(
                        station: station,
                        trackTitle: self.currentSong?.title ?? self.currentProgramTitle,
                        isPlaying: state == .playing
                    )
                }
                #endif
            }
        }
        
        self.playerManager.onTrackChanged = { [weak self] streamTrack in
            Task { @MainActor [weak self] in
                guard let self else { return }
                if let streamTrack, !streamTrack.isEmpty {
                    self.currentTrackTitle = streamTrack
                    let enriched = await self.enricher.searchTrack(query: streamTrack)
                    Task { @MainActor [weak self] in
                        guard let self else { return }
                        self.currentSong = enriched
                        if let enriched, let station = self.currentStation {
                            SongHistoryManager.shared.addSong(
                                title: enriched.title,
                                artist: enriched.artist,
                                albumArtURL: enriched.albumArtURL,
                                station: station
                            )
                        }
                    }
                }
                
                #if os(iOS)
                if let station = self.currentStation {
                    RadioActivityManager.shared.startOrUpdateActivity(
                        station: station,
                        trackTitle: self.currentSong?.title ?? self.currentProgramTitle,
                        isPlaying: self.playbackState == .playing
                    )
                }
                #endif
            }
        }
        
        self.playerManager.onNextStationRequested = { [weak self] in
            Task { @MainActor [weak self] in
                guard let self else { return }
                if let current = self.playerManager.currentStation {
                    self.currentStation = current
                    if let freq = current.frequency {
                        self.tuningFrequency = freq
                    }
                    self.fetchOnAirBroadcast(for: current)
                    self.haptics.snap()
                }
            }
        }
        
        self.playerManager.onPreviousStationRequested = { [weak self] in
            Task { @MainActor [weak self] in
                guard let self else { return }
                if let current = self.playerManager.currentStation {
                    self.currentStation = current
                    if let freq = current.frequency {
                        self.tuningFrequency = freq
                    }
                    self.fetchOnAirBroadcast(for: current)
                    self.haptics.snap()
                }
            }
        }
        
        // Initial metadata fetch for default station
        if let currentStation {
            fetchOnAirBroadcast(for: currentStation)
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
        fetchOnAirBroadcast(for: station)
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
            fetchOnAirBroadcast(for: matched)
            playerManager.play(station: matched)
            haptics.stationFound()
        } else {
            currentProgramTitle = nil
            currentDJName = nil
            currentSong = nil
            currentTrackTitle = nil
            haptics.tick()
        }
    }
    
    public func nextStation() {
        playerManager.nextStation()
        if let next = playerManager.currentStation {
            currentStation = next
            if let freq = next.frequency {
                tuningFrequency = freq
            }
            fetchOnAirBroadcast(for: next)
            haptics.snap()
        }
    }
    
    public func previousStation() {
        playerManager.previousStation()
        if let prev = playerManager.currentStation {
            currentStation = prev
            if let freq = prev.frequency {
                tuningFrequency = freq
            }
            fetchOnAirBroadcast(for: prev)
            haptics.snap()
        }
    }
    
    public func startAutoScan(speedMs: UInt64 = 25, verifyStreams: Bool = false) {
        guard !isScanning else { return }
        isScanning = true
        scannedSignalDetected = false
        stopMetadataRefreshLoop()
        playerManager.stop()
        
        Task {
            await scanEngine.scanFrequencies(
                speedMs: speedMs,
                verifyStreams: verifyStreams,
                onTick: { [weak self] freq, matched in
                    Task { @MainActor [weak self] in
                        guard let self else { return }
                        self.tuningFrequency = freq
                        if let matched {
                            self.currentStation = matched
                            self.scannedSignalDetected = true
                            self.haptics.stationFound()
                        } else {
                            self.scannedSignalDetected = false
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
    
    // MARK: - Metadata & On-Air Refresh
    public func fetchOnAirBroadcast(for station: RadioStation) {
        Task {
            let broadcast = await onAirFetcher.fetchCurrentBroadcast(for: station)
            
            var songInfo: EnrichedSongInfo? = nil
            if let songTitle = broadcast.currentSongTitle, !songTitle.isEmpty {
                let query = "\(broadcast.currentSongArtist ?? "") \(songTitle)".trimmingCharacters(in: .whitespaces)
                songInfo = await enricher.searchTrack(query: query.isEmpty ? songTitle : query)
            }
            
            Task { @MainActor [weak self] in
                guard let self, self.currentStation?.id == station.id else { return }
                self.currentProgramTitle = broadcast.programTitle
                self.currentDJName = broadcast.djName
                self.currentProgramPhotoURL = broadcast.programPhotoURL
                self.currentSong = songInfo
                self.currentTrackTitle = songInfo?.title ?? broadcast.programTitle
                
                if let song = songInfo {
                    SongHistoryManager.shared.addSong(
                        title: song.title,
                        artist: song.artist,
                        albumArtURL: song.albumArtURL,
                        station: station
                    )
                }
                
                #if os(iOS)
                RadioActivityManager.shared.startOrUpdateActivity(
                    station: station,
                    trackTitle: self.currentTrackTitle,
                    isPlaying: self.playbackState == .playing
                )
                #endif
            }
        }
    }
    
    private func startMetadataRefreshLoop() {
        metadataTask?.cancel()
        metadataTask = Task { [weak self] in
            while !Task.isCancelled {
                try? await Task.sleep(nanoseconds: 30_000_000_000) // 30 seconds
                if Task.isCancelled { break }
                guard let self else { break }
                await MainActor.run {
                    if let station = self.currentStation, self.playbackState == .playing {
                        self.fetchOnAirBroadcast(for: station)
                    }
                }
            }
        }
    }
    
    private func stopMetadataRefreshLoop() {
        metadataTask?.cancel()
        metadataTask = nil
    }
}
