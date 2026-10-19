import Foundation
import SwiftUI

@MainActor
final class DashboardStore: ObservableObject {
    @Published var screen: Screen = .overview
    @Published var search = ""
    @Published var instructions: [Instruction] = []
    @Published var bundles: [InstructionBundle] = []
    @Published var skills: [SkillInfo] = []
    @Published var subscriptions: [Subscription] = []
    @Published var codex = CodexSnapshot()
    @Published var antigravity = AntigravitySnapshot()
    @Published var system = SystemSnapshot()
    @Published var batteryHistory: [BatteryPoint] = []
    @Published var selectedInstructionID: UUID?
    @Published var newInstructionRequest = 0
    @Published var selectedSkillID: String?
    @Published var selectedBundleID: UUID?
    @Published var notice: String?

    let workspace: URL
    let library: LibraryStore
    private let systemReader = SystemReader()
    private var lastBatterySave = Date.distantPast
    private var lastCLIRead = Date.distantPast
    private var lastSkillRead = Date.distantPast

    init() {
        let path = Bundle.main.object(forInfoDictionaryKey: "DevDeskWorkspacePath") as? String ?? FileManager.default.currentDirectoryPath
        workspace = URL(fileURLWithPath: path, isDirectory: true)
        library = LibraryStore(root: workspace)
        reloadLibrary()
        subscriptions = library.subscriptions()
        batteryHistory = library.batteryHistory().filter { $0.date > Date().addingTimeInterval(-24 * 3600) }
        refresh()
    }

    var filteredInstructions: [Instruction] {
        guard !search.isEmpty else { return instructions }
        return instructions.filter { $0.title.localizedCaseInsensitiveContains(search) || $0.body.localizedCaseInsensitiveContains(search) }
    }

    var filteredSkills: [SkillInfo] {
        guard !search.isEmpty else { return skills }
        return skills.filter { $0.name.localizedCaseInsensitiveContains(search) || $0.summary.localizedCaseInsensitiveContains(search) }
    }

    func refresh() {
        reloadLibrary()
        system = systemReader.read()
        if let percent = system.batteryPercent {
            if batteryHistory.last.map({ system.sampledAt.timeIntervalSince($0.date) >= 50 }) ?? true {
                batteryHistory.append(BatteryPoint(date: system.sampledAt, percent: percent))
                batteryHistory.removeAll { $0.date < Date().addingTimeInterval(-24 * 3600) }
            }
        }
        if Date().timeIntervalSince(lastBatterySave) > 60 {
            lastBatterySave = Date()
            try? library.saveBatteryHistory(batteryHistory)
        }
        if Date().timeIntervalSince(lastCLIRead) > 25 {
            lastCLIRead = Date()
            Task.detached {
                let codex = CodexReader.read()
                let agy = AntigravityReader.read()
                await MainActor.run {
                    self.codex = codex
                    self.antigravity = agy
                }
            }
        }
        if Date().timeIntervalSince(lastSkillRead) > 120 {
            lastSkillRead = Date()
            Task.detached {
                let items = SkillScanner.scan()
                await MainActor.run { self.skills = items }
            }
        }
    }

    func reloadLibrary() {
        do {
            instructions = try library.instructions()
            bundles = library.bundles()
        } catch {
            notice = error.localizedDescription
        }
    }

    func saveInstruction(id: UUID?, title: String, body: String, expectedText: String?) -> Bool {
        do {
            let item = try library.save(id: id, title: title, body: body, expectedText: expectedText)
            reloadLibrary()
            selectedInstructionID = item.id
            notice = "지침을 저장했습니다."
            return true
        } catch {
            notice = error.localizedDescription
            return false
        }
    }

    func deleteInstruction(_ item: Instruction) {
        do {
            try library.delete(item)
            if selectedInstructionID == item.id { selectedInstructionID = nil }
            reloadLibrary()
            notice = "지침을 보관함의 data/history로 옮겼습니다."
        } catch { notice = error.localizedDescription }
    }

    func saveBundle(_ item: InstructionBundle) {
        var updated = bundles
        if let index = updated.firstIndex(where: { $0.id == item.id }) { updated[index] = item }
        else { updated.append(item) }
        do {
            try library.saveBundles(updated)
            bundles = updated
            selectedBundleID = item.id
            notice = "지침 묶음을 저장했습니다."
        } catch { notice = error.localizedDescription }
    }

    func deleteBundle(_ item: InstructionBundle) {
        let updated = bundles.filter { $0.id != item.id }
        do {
            try library.saveBundles(updated)
            bundles = updated
            selectedBundleID = nil
            notice = "지침 묶음을 삭제했습니다."
        } catch { notice = error.localizedDescription }
    }

    func saveSubscription(_ item: Subscription) {
        var updated = subscriptions
        guard let index = updated.firstIndex(where: { $0.id == item.id }) else { return }
        updated[index] = item
        do {
            try library.saveSubscriptions(updated)
            subscriptions = updated
            notice = "구독 정보를 저장했습니다."
        } catch { notice = error.localizedDescription }
    }

    func copy(_ text: String) {
        NSPasteboard.general.clearContents()
        if NSPasteboard.general.setString(text, forType: .string) { notice = "클립보드에 복사했습니다." }
        else { notice = "복사에 실패했습니다. 텍스트를 직접 선택해 주세요." }
    }
}
