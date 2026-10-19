// Sources/RadioApp/Models/RadioActivityAttributes.swift
import Foundation

#if os(iOS)
import ActivityKit

public struct RadioActivityAttributes: ActivityAttributes, Sendable {
    public struct ContentState: Codable, Hashable, Sendable {
        public var stationName: String
        public var networkName: String
        public var frequencyLabel: String
        public var trackTitle: String?
        public var isPlaying: Bool
        
        public init(
            stationName: String,
            networkName: String,
            frequencyLabel: String,
            trackTitle: String? = nil,
            isPlaying: Bool = true
        ) {
            self.stationName = stationName
            self.networkName = networkName
            self.frequencyLabel = frequencyLabel
            self.trackTitle = trackTitle
            self.isPlaying = isPlaying
        }
    }
    
    public var stationId: String
    
    public init(stationId: String) {
        self.stationId = stationId
    }
}
#endif
