import Foundation

struct Instruction: Identifiable, Equatable {
    let id: UUID
    var title: String
    var body: String
    var modifiedAt: Date

    var markdown: String { "# \(title)\n\n\(body.trimmingCharacters(in: .whitespacesAndNewlines))\n" }

    static func parse(id: UUID, text: String, modifiedAt: Date) -> Instruction? {
        let lines = text.components(separatedBy: .newlines)
        guard let first = lines.first, first.hasPrefix("# ") else { return nil }
        let title = String(first.dropFirst(2)).trimmingCharacters(in: .whitespaces)
        guard !title.isEmpty else { return nil }
        let body = lines.dropFirst().joined(separator: "\n").trimmingCharacters(in: .whitespacesAndNewlines)
        return Instruction(id: id, title: title, body: body, modifiedAt: modifiedAt)
    }
}

enum MarkdownBlock: Equatable {
    case heading(Int, String)
    case paragraph(String)
    case bullet(String)
    case numbered(String, String)
    case code(String)
    case quote(String)
    case table([String], [[String]])
    case divider
}

enum MarkdownParser {
    static func blocks(_ source: String) -> [MarkdownBlock] {
        var result: [MarkdownBlock] = []
        var paragraph: [String] = []
        var code: [String] = []
        var inCode = false

        func flushParagraph() {
            if !paragraph.isEmpty {
                result.append(.paragraph(paragraph.joined(separator: " ")))
                paragraph.removeAll()
            }
        }

        let lines = source.components(separatedBy: .newlines)
        var index = 0
        if lines.first?.trimmingCharacters(in: .whitespaces) == "---",
           let close = lines.dropFirst().firstIndex(where: { $0.trimmingCharacters(in: .whitespaces) == "---" }) {
            index = close + 1
        }
        while index < lines.count {
            let raw = lines[index]
            let line = raw.trimmingCharacters(in: .whitespaces)
            if !inCode, line.hasPrefix("|"), index + 1 < lines.count,
               isTableDivider(lines[index + 1]) {
                flushParagraph()
                let headers = cells(line)
                index += 2
                var rows: [[String]] = []
                while index < lines.count, lines[index].trimmingCharacters(in: .whitespaces).hasPrefix("|") {
                    rows.append(cells(lines[index]))
                    index += 1
                }
                result.append(.table(headers, rows))
                continue
            }
            index += 1
            if line.hasPrefix("```") {
                flushParagraph()
                if inCode {
                    result.append(.code(code.joined(separator: "\n")))
                    code.removeAll()
                }
                inCode.toggle()
                continue
            }
            if inCode { code.append(raw); continue }
            if line.isEmpty { flushParagraph(); continue }
            if line == "---" || line == "***" {
                flushParagraph()
                result.append(.divider)
            } else if let space = line.firstIndex(of: " "),
                      !line[..<space].isEmpty,
                      line[..<space].allSatisfy({ $0 == "#" }) {
                flushParagraph()
                result.append(.heading(min(line.distance(from: line.startIndex, to: space), 6), String(line[line.index(after: space)...])))
            } else if line.hasPrefix("- ") || line.hasPrefix("* ") {
                flushParagraph()
                result.append(.bullet(String(line.dropFirst(2))))
            } else if let dot = line.firstIndex(of: "."),
                      line[..<dot].allSatisfy(\.isNumber),
                      !line[..<dot].isEmpty,
                      line[line.index(after: dot)...].hasPrefix(" ") {
                flushParagraph()
                result.append(.numbered(String(line[..<dot]) + ".", String(line[line.index(dot, offsetBy: 2)...])))
            } else if line.hasPrefix("> ") {
                flushParagraph()
                result.append(.quote(String(line.dropFirst(2))))
            } else {
                paragraph.append(line)
            }
        }
        flushParagraph()
        if inCode { result.append(.code(code.joined(separator: "\n"))) }
        return result
    }

    private static func cells(_ line: String) -> [String] {
        var text = line.trimmingCharacters(in: .whitespaces)
        if text.hasPrefix("|") { text.removeFirst() }
        if text.hasSuffix("|") { text.removeLast() }
        // ponytail: simple pipe tables only; escaped pipes need a full GFM parser if they become common.
        return text.split(separator: "|", omittingEmptySubsequences: false).map { $0.trimmingCharacters(in: .whitespaces) }
    }

    private static func isTableDivider(_ line: String) -> Bool {
        let values = cells(line)
        return !values.isEmpty && values.allSatisfy {
            let dashes = $0.replacingOccurrences(of: ":", with: "")
            return !dashes.isEmpty && dashes.allSatisfy { $0 == "-" }
        }
    }
}

struct InstructionBundle: Codable, Identifiable, Equatable {
    var id: UUID
    var title: String
    var instructionIDs: [UUID]

    init(id: UUID = UUID(), title: String, instructionIDs: [UUID] = []) {
        self.id = id
        self.title = title
        self.instructionIDs = instructionIDs
    }
}

struct SkillInfo: Identifiable {
    var id: String { path.path }
    let name: String
    let summary: String
    let path: URL
    let content: String
}

struct QuotaWindow {
    let usedPercent: Double
    let resetsAt: Date?
    let windowMinutes: Int?
}

struct CodexSnapshot {
    var inputTokens = 0
    var outputTokens = 0
    var sessions = 0
    var latestActivity: Date?
    var quota: QuotaWindow?
    var weeklyQuota: QuotaWindow?
    var quotaObservedAt: Date?
    var plan: String?
    var lastRead = Date()
    var error: String?
    var dailyTokens: [(Date, Int)] = []
}

struct SystemSnapshot {
    var batteryPercent: Double?
    var charging = false
    var onAC = false
    var remainingMinutes: Int?
    var batteryWatts: Double?
    var cpuPercent: Double?
    var memoryUsedGB: Double?
    var memoryTotalGB: Double?
    var sampledAt = Date()
}

struct BatteryPoint: Identifiable, Codable {
    var id: Date { date }
    let date: Date
    let percent: Double
}

struct Subscription: Codable, Identifiable {
    let id: String
    var name: String
    var nextPayment: Date?
    var nextAmountKRW: Int?
    var laterPayment: Date?
    var laterAmountKRW: Int?
    var note: String
}

enum Screen: String, CaseIterable, Identifiable {
    case overview = "개요"
    case instructions = "지침"
    case bundles = "지침 묶음"
    case skills = "스킬"
    case activity = "AI 활동"
    case usage = "사용량 · 비용"
    case system = "맥 상태"

    var id: String { rawValue }

    var icon: String {
        switch self {
        case .overview: "square.grid.2x2"
        case .instructions: "text.book.closed"
        case .bundles: "square.stack"
        case .skills: "sparkles.rectangle.stack"
        case .activity: "waveform.path"
        case .usage: "chart.bar.xaxis"
        case .system: "laptopcomputer"
        }
    }
}
