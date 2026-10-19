import SwiftUI

@main
struct DevDeskApp: App {
    @StateObject private var store = DashboardStore()

    var body: some Scene {
        WindowGroup("Dev Desk") {
            RootView()
                .environmentObject(store)
                .frame(minWidth: 720, minHeight: 560)
        }
        .windowStyle(.titleBar)
        .commands {
            CommandGroup(replacing: .newItem) {
                Button("새 지침") {
                    store.screen = .instructions
                    store.newInstructionRequest += 1
                }
                .keyboardShortcut("n", modifiers: .command)
            }
        }
    }
}
