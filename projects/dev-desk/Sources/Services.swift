import Foundation
import IOKit
import IOKit.ps
import Darwin

enum StoreError: LocalizedError {
    case conflict
    case missingInstruction
    case invalidTitle

    var errorDescription: String? {
        switch self {
        case .conflict: "다른 곳에서 지침이 수정됐습니다. 최신 내용을 확인한 뒤 다시 저장하세요."
        case .missingInstruction: "지침 파일을 찾을 수 없습니다."
        case .invalidTitle: "제목을 입력하세요."
        }
    }
}

struct LibraryStore {
    let root: URL
    var folder: URL { root.appendingPathComponent("data/library", isDirectory: true) }
    private var history: URL { root.appendingPathComponent("data/history", isDirectory: true) }
    private var bundlesFile: URL { root.appendingPathComponent("data/bundles.json") }
    private var subscriptionsFile: URL { root.appendingPathComponent("data/subscriptions.json") }
    private var batteryFile: URL { root.appendingPathComponent("data/battery-history.json") }

    func prepare() throws {
        let fm = FileManager.default
        try fm.createDirectory(at: folder, withIntermediateDirectories: true)
        try fm.createDirectory(at: history, withIntermediateDirectories: true)
    }

    func instructions() throws -> [Instruction] {
        try prepare()
        return try FileManager.default.contentsOfDirectory(at: folder, includingPropertiesForKeys: [.contentModificationDateKey])
            .compactMap { url in
                guard url.pathExtension == "md", let id = UUID(uuidString: url.deletingPathExtension().lastPathComponent) else { return nil }
                let text = try String(contentsOf: url, encoding: .utf8)
                let date = (try url.resourceValues(forKeys: [.contentModificationDateKey])).contentModificationDate ?? .distantPast
                return Instruction.parse(id: id, text: text, modifiedAt: date)
            }
            .sorted { $0.title.localizedStandardCompare($1.title) == .orderedAscending }
    }

    func save(id: UUID?, title: String, body: String, expectedText: String?) throws -> Instruction {
        let cleanTitle = title.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !cleanTitle.isEmpty, !cleanTitle.contains("\n") else { throw StoreError.invalidTitle }
        try prepare()
        let actualID = id ?? UUID()
        let url = folder.appendingPathComponent("\(actualID.uuidString).md")
        if let expectedText {
            guard let current = try? String(contentsOf: url, encoding: .utf8) else { throw StoreError.missingInstruction }
            guard current == expectedText else { throw StoreError.conflict }
            let stamp = ISO8601DateFormatter().string(from: Date()).replacingOccurrences(of: ":", with: "-")
            try current.write(to: history.appendingPathComponent("\(actualID.uuidString)-\(stamp).md"), atomically: true, encoding: .utf8)
        } else if FileManager.default.fileExists(atPath: url.path) {
            throw StoreError.conflict
        }
        let item = Instruction(id: actualID, title: cleanTitle, body: body, modifiedAt: Date())
        try item.markdown.write(to: url, atomically: true, encoding: .utf8)
        return item
    }

    func delete(_ item: Instruction) throws {
        let url = folder.appendingPathComponent("\(item.id.uuidString).md")
        guard FileManager.default.fileExists(atPath: url.path) else { throw StoreError.missingInstruction }
        let stamp = ISO8601DateFormatter().string(from: Date()).replacingOccurrences(of: ":", with: "-")
        try FileManager.default.moveItem(at: url, to: history.appendingPathComponent("\(item.id.uuidString)-deleted-\(stamp).md"))
    }

    func text(for item: Instruction) -> String? {
        try? String(contentsOf: folder.appendingPathComponent("\(item.id.uuidString).md"), encoding: .utf8)
    }

    func bundles() -> [InstructionBundle] {
        guard let data = try? Data(contentsOf: bundlesFile) else { return [] }
        return (try? JSONDecoder().decode([InstructionBundle].self, from: data)) ?? []
    }

    func saveBundles(_ bundles: [InstructionBundle]) throws {
        try prepare()
        let data = try JSONEncoder().encode(bundles)
        try data.write(to: bundlesFile, options: .atomic)
    }

    func subscriptions() -> [Subscription] {
        if let data = try? Data(contentsOf: subscriptionsFile),
           let saved = try? JSONDecoder().decode([Subscription].self, from: data) { return saved }
        let formatter = ISO8601DateFormatter()
        formatter.formatOptions = [.withFullDate]
        return [
            Subscription(id: "antigravity", name: "Google AI Pro (5 TB)", nextPayment: formatter.date(from: "2026-10-12"), nextAmountKRW: 7_300, laterPayment: formatter.date(from: "2027-08-12"), laterAmountKRW: 29_000, note: "2026-09-24 사용자 제공 Google Play 화면"),
            Subscription(id: "codex", name: "ChatGPT Plus", nextPayment: nil, nextAmountKRW: nil, laterPayment: nil, laterAmountKRW: nil, note: "결제 금액과 날짜를 입력하세요")
        ]
    }

    func saveSubscriptions(_ items: [Subscription]) throws {
        try prepare()
        try JSONEncoder().encode(items).write(to: subscriptionsFile, options: .atomic)
    }

    func batteryHistory() -> [BatteryPoint] {
        guard let data = try? Data(contentsOf: batteryFile) else { return [] }
        return (try? JSONDecoder().decode([BatteryPoint].self, from: data)) ?? []
    }

    func saveBatteryHistory(_ points: [BatteryPoint]) throws {
        try prepare()
        try JSONEncoder().encode(points).write(to: batteryFile, options: .atomic)
    }
}

enum SkillScanner {
    static func scan() -> [SkillInfo] {
        let home = FileManager.default.homeDirectoryForCurrentUser
        let roots = [home.appendingPathComponent(".agents/skills"), home.appendingPathComponent(".codex/skills"), home.appendingPathComponent(".codex/.tmp/plugins/plugins/superpowers/skills")]
        var seen = Set<String>()
        var results: [SkillInfo] = []
        for root in roots where FileManager.default.fileExists(atPath: root.path) {
            guard let files = FileManager.default.enumerator(at: root, includingPropertiesForKeys: nil) else { continue }
            for case let url as URL in files where url.lastPathComponent == "SKILL.md" {
                guard seen.insert(url.path).inserted, let content = try? String(contentsOf: url, encoding: .utf8) else { continue }
                let lines = content.components(separatedBy: .newlines)
                let name = lines.first { $0.hasPrefix("name:") }?.dropFirst(5).trimmingCharacters(in: .whitespacesAndNewlines).replacingOccurrences(of: "\"", with: "") ?? url.deletingLastPathComponent().lastPathComponent
                let summary = lines.first { $0.hasPrefix("description:") }?.dropFirst(12).trimmingCharacters(in: .whitespacesAndNewlines).replacingOccurrences(of: "\"", with: "") ?? "사용법 보기"
                results.append(SkillInfo(name: name, summary: summary, path: url, content: content))
            }
        }
        return results.sorted { $0.name.localizedStandardCompare($1.name) == .orderedAscending }
    }
}

struct AntigravitySnapshot {
    var conversationCount = 0
    var latestActivity: Date?
    var statusLineConfigured = false
    var lastRead = Date()
}

enum AntigravityReader {
    static func read() -> AntigravitySnapshot {
        let base = FileManager.default.homeDirectoryForCurrentUser.appendingPathComponent(".gemini/antigravity-cli")
        let files = (try? FileManager.default.contentsOfDirectory(at: base.appendingPathComponent("conversations"), includingPropertiesForKeys: [.contentModificationDateKey])) ?? []
        let dbs = files.filter { $0.pathExtension == "db" }
        let latest = dbs.compactMap { try? $0.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate }.max()
        let settingsData = try? Data(contentsOf: base.appendingPathComponent("settings.json"))
        let settings = settingsData.flatMap { try? JSONSerialization.jsonObject(with: $0) as? [String: Any] }
        let status = settings?["statusLine"] as? [String: Any]
        let command = (status?["command"] as? String)?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
        return AntigravitySnapshot(conversationCount: dbs.count, latestActivity: latest, statusLineConfigured: !command.isEmpty && (status?["enabled"] as? Bool != false))
    }
}

enum CodexReader {
    static func read() -> CodexSnapshot {
        var result = CodexSnapshot()
        let root = FileManager.default.homeDirectoryForCurrentUser.appendingPathComponent(".codex/sessions")
        guard let walker = FileManager.default.enumerator(at: root, includingPropertiesForKeys: [.contentModificationDateKey, .fileSizeKey]) else {
            result.error = "Codex 세션 기록을 찾지 못했습니다."
            return result
        }
        let cutoff = Date().addingTimeInterval(-30 * 24 * 3600)
        var files: [(URL, Date)] = []
        for case let url as URL in walker where url.pathExtension == "jsonl" {
            guard let attr = try? url.resourceValues(forKeys: [.contentModificationDateKey, .fileSizeKey]),
                  let date = attr.contentModificationDate, date >= cutoff,
                  (attr.fileSize ?? 0) < 50_000_000 else { continue }
            files.append((url, date))
        }
        let calendar = Calendar.current
        let fractional = ISO8601DateFormatter()
        fractional.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
        let basic = ISO8601DateFormatter()
        var daily: [Date: Int] = [:]
        var latestQuotaDate = Date.distantPast
        for (url, modifiedAt) in files.sorted(by: { $0.1 > $1.1 }).prefix(120) {
            guard let text = try? String(contentsOf: url, encoding: .utf8) else { continue }
            var previousTotal = 0
            var latestInput = 0
            var latestOutput = 0
            var hadUsage = false
            for line in text.split(separator: "\n") {
                guard let data = line.data(using: .utf8),
                      let obj = (try? JSONSerialization.jsonObject(with: data)) as? [String: Any],
                      obj["type"] as? String == "event_msg",
                      let payload = obj["payload"] as? [String: Any],
                      payload["type"] as? String == "token_count" else { continue }
                if let info = payload["info"] as? [String: Any],
                   let usage = info["total_token_usage"] as? [String: Any] {
                    let total = usage["total_tokens"] as? Int ?? 0
                    let delta = total >= previousTotal ? total - previousTotal : total
                    previousTotal = total
                    latestInput = usage["input_tokens"] as? Int ?? 0
                    latestOutput = usage["output_tokens"] as? Int ?? 0
                    hadUsage = true
                    let stamp = (obj["timestamp"] as? String).flatMap { fractional.date(from: $0) ?? basic.date(from: $0) } ?? modifiedAt
                    daily[calendar.startOfDay(for: stamp), default: 0] += delta
                }
                if let rate = payload["rate_limits"] as? [String: Any] {
                    let stamp = (obj["timestamp"] as? String).flatMap { fractional.date(from: $0) ?? basic.date(from: $0) } ?? modifiedAt
                    if stamp > latestQuotaDate {
                        latestQuotaDate = stamp
                        result.quotaObservedAt = stamp
                        result.quota = window(rate["primary"])
                        result.weeklyQuota = window(rate["secondary"])
                        result.plan = rate["plan_type"] as? String
                    }
                }
            }
            if hadUsage {
                result.sessions += 1
                result.inputTokens += latestInput
                result.outputTokens += latestOutput
                result.latestActivity = max(result.latestActivity ?? .distantPast, modifiedAt)
            }
        }
        if let reset = result.quota?.resetsAt, reset < Date() { result.quota = nil }
        if let reset = result.weeklyQuota?.resetsAt, reset < Date() { result.weeklyQuota = nil }
        result.dailyTokens = daily.map { ($0.key, $0.value) }.sorted { $0.0 < $1.0 }
        if result.sessions == 0 { result.error = "최근 30일간 읽을 수 있는 토큰 기록이 없습니다." }
        return result
    }

    private static func window(_ value: Any?) -> QuotaWindow? {
        guard let item = value as? [String: Any], let percent = item["used_percent"] as? Double else { return nil }
        return QuotaWindow(usedPercent: percent, resetsAt: (item["resets_at"] as? TimeInterval).map(Date.init(timeIntervalSince1970:)), windowMinutes: item["window_minutes"] as? Int)
    }
}

final class SystemReader {
    private var lastCPU: (active: UInt64, idle: UInt64)?

    func read() -> SystemSnapshot {
        var state = SystemSnapshot()
        let info = IOPSCopyPowerSourcesInfo().takeRetainedValue()
        let sources = IOPSCopyPowerSourcesList(info).takeRetainedValue() as [AnyObject]
        if let source = sources.first,
           let detail = IOPSGetPowerSourceDescription(info, source).takeUnretainedValue() as? [String: Any] {
            let current = (detail[kIOPSCurrentCapacityKey as String] as? NSNumber)?.doubleValue
            let maximum = (detail[kIOPSMaxCapacityKey as String] as? NSNumber)?.doubleValue
            if let current, let maximum, maximum > 0 { state.batteryPercent = 100 * current / maximum }
            state.charging = (detail[kIOPSIsChargingKey as String] as? Bool) ?? false
            state.onAC = (detail[kIOPSPowerSourceStateKey as String] as? String) == (kIOPSACPowerValue as String)
            let rawMinutes = (detail[kIOPSTimeToEmptyKey as String] as? NSNumber)?.intValue ?? -1
            if !state.onAC && rawMinutes > 0 { state.remainingMinutes = rawMinutes }
        }

        let battery = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("AppleSmartBattery"))
        if battery != 0 {
            defer { IOObjectRelease(battery) }
            func number(_ key: String) -> Double? {
                (IORegistryEntryCreateCFProperty(battery, key as CFString, kCFAllocatorDefault, 0)?.takeRetainedValue() as? NSNumber)?.doubleValue
            }
            if let amps = number("InstantAmperage"), let millivolts = number("Voltage"), !state.onAC {
                let watts = abs(amps * millivolts) / 1_000_000
                if watts > 0 && watts < 200 { state.batteryWatts = watts }
            }
        }

        var load = host_cpu_load_info_data_t()
        var loadCount = mach_msg_type_number_t(MemoryLayout.size(ofValue: load) / MemoryLayout<integer_t>.size)
        let cpuResult = withUnsafeMutablePointer(to: &load) {
            $0.withMemoryRebound(to: integer_t.self, capacity: Int(loadCount)) {
                host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, $0, &loadCount)
            }
        }
        if cpuResult == KERN_SUCCESS {
            let active = UInt64(load.cpu_ticks.0) + UInt64(load.cpu_ticks.1) + UInt64(load.cpu_ticks.2)
            let idle = UInt64(load.cpu_ticks.3)
            if let previous = lastCPU {
                let deltaActive = active >= previous.active ? active - previous.active : 0
                let deltaIdle = idle >= previous.idle ? idle - previous.idle : 0
                if deltaActive + deltaIdle > 0 { state.cpuPercent = 100 * Double(deltaActive) / Double(deltaActive + deltaIdle) }
            }
            lastCPU = (active, idle)
        }

        let totalBytes = ProcessInfo.processInfo.physicalMemory
        state.memoryTotalGB = Double(totalBytes) / 1_073_741_824
        var vm = vm_statistics64_data_t()
        var vmCount = mach_msg_type_number_t(MemoryLayout.size(ofValue: vm) / MemoryLayout<integer_t>.size)
        let vmResult = withUnsafeMutablePointer(to: &vm) {
            $0.withMemoryRebound(to: integer_t.self, capacity: Int(vmCount)) {
                host_statistics64(mach_host_self(), HOST_VM_INFO64, $0, &vmCount)
            }
        }
        if vmResult == KERN_SUCCESS {
            let freePages = UInt64(vm.free_count) + UInt64(vm.inactive_count)
            state.memoryUsedGB = Double(totalBytes - min(totalBytes, freePages * UInt64(vm_page_size))) / 1_073_741_824
        }
        return state
    }
}
