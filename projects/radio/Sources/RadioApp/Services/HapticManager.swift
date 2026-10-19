// Sources/RadioApp/Services/HapticManager.swift
import Foundation
#if canImport(UIKit)
import UIKit
#endif

public final class HapticManager: Sendable {
    public static let shared = HapticManager()
    
    #if canImport(UIKit) && !os(watchOS)
    @MainActor
    private final class GeneratorHolder {
        static let shared = GeneratorHolder()
        let light = UIImpactFeedbackGenerator(style: .light)
        let medium = UIImpactFeedbackGenerator(style: .medium)
        let notify = UINotificationFeedbackGenerator()
        var lastTickTime: TimeInterval = 0
        
        init() {
            light.prepare()
            medium.prepare()
            notify.prepare()
        }
    }
    #endif
    
    public init() {}
    
    public func tick() {
        #if canImport(UIKit) && !os(watchOS)
        Task { @MainActor in
            let holder = GeneratorHolder.shared
            let now = CACurrentMediaTime()
            guard now - holder.lastTickTime >= 0.05 else { return } // Max 20Hz (within 32Hz OS limit)
            holder.lastTickTime = now
            holder.light.impactOccurred(intensity: 0.4)
        }
        #endif
    }
    
    public func snap() {
        #if canImport(UIKit) && !os(watchOS)
        Task { @MainActor in
            let holder = GeneratorHolder.shared
            holder.medium.impactOccurred(intensity: 0.9)
        }
        #endif
    }
    
    public func stationFound() {
        #if canImport(UIKit) && !os(watchOS)
        Task { @MainActor in
            let holder = GeneratorHolder.shared
            holder.notify.notificationOccurred(.success)
        }
        #endif
    }
}
