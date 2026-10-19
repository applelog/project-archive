// Sources/RadioApp/App/RadioApp.swift
import SwiftUI

#if os(iOS)
@main
public struct RadioApp: App {
    public init() {}
    
    public var body: some Scene {
        WindowGroup {
            MainRadioView()
        }
    }
}
#else
public struct RadioApp: App {
    public init() {}
    
    public var body: some Scene {
        WindowGroup {
            MainRadioView()
        }
    }
}
#endif
