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
