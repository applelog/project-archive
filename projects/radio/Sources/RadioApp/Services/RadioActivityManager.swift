// Sources/RadioApp/Services/RadioActivityManager.swift
import Foundation

#if os(iOS)
import ActivityKit

public final class RadioActivityManager: @unchecked Sendable {
    public static let shared = RadioActivityManager()
    
    private var currentActivity: Any?
    
    public init() {}
    
    public func startOrUpdateActivity(station: RadioStation, trackTitle: String?, isPlaying: Bool) {
        guard ActivityAuthorizationInfo().areActivitiesEnabled else { return }
        
        let state = RadioActivityAttributes.ContentState(
            stationName: station.name,
            networkName: station.network.displayName,
            frequencyLabel: station.frequencyLabel,
            trackTitle: trackTitle,
            isPlaying: isPlaying
        )
        
        if let activity = currentActivity as? Activity<RadioActivityAttributes> {
            Task {
                await activity.update(ActivityContent(state: state, staleDate: nil))
            }
        } else {
            let attributes = RadioActivityAttributes(stationId: station.id.uuidString)
            do {
                let activity = try Activity.request(
                    attributes: attributes,
                    content: ActivityContent(state: state, staleDate: nil),
                    pushType: nil
                )
                self.currentActivity = activity
            } catch {
                // Silently ignore if Dynamic Island widget extension is not configured for target
            }
        }
    }
    
    public func endActivity() {
        if let activity = currentActivity as? Activity<RadioActivityAttributes> {
            Task {
                await activity.end(nil, dismissalPolicy: .immediate)
            }
            currentActivity = nil
        }
    }
}
#endif
