import Foundation

@main
struct IntegrationChecks {
    static func main() {
        let codex = CodexReader.read()
        if let quota = codex.quota {
            precondition((0...100).contains(quota.usedPercent))
        }
        precondition(codex.inputTokens >= 0 && codex.outputTokens >= 0)
        precondition(codex.dailyTokens.allSatisfy { $0.1 >= 0 })
        let agy = AntigravityReader.read()
        precondition(agy.conversationCount >= 0)
        let reader = SystemReader()
        _ = reader.read()
        let system = reader.read()
        if let battery = system.batteryPercent { precondition((0...100).contains(battery)) }
        if let cpu = system.cpuPercent { precondition((0...100).contains(cpu)) }
        print("IntegrationChecks passed: Codex \(codex.sessions) sessions, Antigravity \(agy.conversationCount) conversations, battery \(system.batteryPercent.map { String(Int($0)) } ?? "unavailable")%")
    }
}
