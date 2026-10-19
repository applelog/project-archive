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
    
    /// Verifies whether a live stream URL is actually reachable and responding
    public func verifyLiveStream(url: URL, timeoutSeconds: Double = 1.5) async -> Bool {
        var request = URLRequest(url: url)
        request.httpMethod = "HEAD"
        request.timeoutInterval = timeoutSeconds
        
        do {
            let (_, response) = try await URLSession.shared.data(for: request)
            if let httpResponse = response as? HTTPURLResponse {
                return (200...399).contains(httpResponse.statusCode)
            }
            return false
        } catch {
            var getRequest = URLRequest(url: url)
            getRequest.httpMethod = "GET"
            getRequest.setValue("bytes=0-512", forHTTPHeaderField: "Range")
            getRequest.timeoutInterval = timeoutSeconds
            guard let (_, getResponse) = try? await URLSession.shared.data(for: getRequest),
                  let httpGetResp = getResponse as? HTTPURLResponse else {
                return false
            }
            return (200...399).contains(httpGetResp.statusCode)
        }
    }
    
    public func scanFrequencies(
        speedMs: UInt64 = 25,
        verifyStreams: Bool = false,
        onTick: @Sendable (Double, RadioStation?) -> Void,
        onComplete: @Sendable ([RadioStation]) -> Void
    ) async {
        let totalSteps = Int(round((Self.maxFrequency - Self.minFrequency) / Self.stepFrequency))
        var found: [RadioStation] = []
        
        for step in 0...totalSteps {
            let current = Self.minFrequency + (Double(step) * Self.stepFrequency)
            let rounded = (current * 10).rounded() / 10
            let candidate = KoreanStationPresets.findByFrequency(rounded, tolerance: 0.05)
            
            var verifiedStation: RadioStation? = nil
            if let candidate {
                if verifyStreams {
                    let isAlive = await verifyLiveStream(url: candidate.streamURL)
                    if isAlive {
                        verifiedStation = candidate
                    }
                } else {
                    verifiedStation = candidate
                }
            }
            
            if let verifiedStation, !found.contains(where: { $0.id == verifiedStation.id }) {
                found.append(verifiedStation)
            }
            
            onTick(rounded, verifiedStation)
            
            if speedMs > 0 {
                try? await Task.sleep(nanoseconds: speedMs * 1_000_000)
            }
        }
        
        onComplete(found)
    }
}
