import SwiftUI
import Charts

private enum Palette {
    static let tint = Color(red: 0.24, green: 0.40, blue: 0.78)
    static let muted = Color.secondary
    static let inset = Color(nsColor: .controlBackgroundColor)
}

struct RootView: View {
    @EnvironmentObject private var store: DashboardStore
    @FocusState private var searchFocused: Bool

    var body: some View {
        NavigationSplitView {
            List(Screen.allCases, selection: $store.screen) { screen in
                Label(screen.rawValue, systemImage: screen.icon)
                    .tag(screen)
                    .padding(.vertical, 3)
            }
            .navigationSplitViewColumnWidth(min: 190, ideal: 210)
            .safeAreaInset(edge: .bottom) {
                VStack(alignment: .leading, spacing: 5) {
                    Divider()
                    Label("로컬 데이터 · 이 Mac", systemImage: "lock.laptopcomputer")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .padding(.horizontal, 14)
                        .padding(.bottom, 12)
                }
            }
        } detail: {
            VStack(spacing: 0) {
                HStack(spacing: 12) {
                    Image(systemName: "magnifyingglass").foregroundStyle(.secondary)
                    TextField("지침과 스킬 검색", text: $store.search)
                        .textFieldStyle(.plain)
                        .focused($searchFocused)
                    if !store.search.isEmpty {
                        Button { store.search = "" } label: { Image(systemName: "xmark.circle.fill") }
                            .buttonStyle(.plain).foregroundStyle(.secondary)
                            .accessibilityLabel("검색어 지우기")
                    }
                    Text("⌘ K").font(.caption.monospaced()).foregroundStyle(.tertiary)
                }
                .padding(.horizontal, 14)
                .frame(height: 38)
                .background(Palette.inset, in: RoundedRectangle(cornerRadius: 11))
                .padding(.horizontal, 28)
                .padding(.top, 20)
                .padding(.bottom, 14)

                if !store.search.isEmpty {
                    VStack(alignment: .leading, spacing: 4) {
                        ForEach(store.filteredInstructions.prefix(5)) { item in
                            Button {
                                store.screen = .instructions
                                store.selectedInstructionID = item.id
                                store.search = ""
                            } label: {
                                Label(item.title, systemImage: "text.book.closed")
                                    .frame(maxWidth: .infinity, alignment: .leading)
                                    .padding(6)
                            }.buttonStyle(.plain)
                        }
                        ForEach(store.filteredSkills.prefix(5)) { skill in
                            Button {
                                store.screen = .skills
                                store.selectedSkillID = skill.id
                                store.search = ""
                            } label: {
                                Label(skill.name, systemImage: "sparkles.rectangle.stack")
                                    .frame(maxWidth: .infinity, alignment: .leading)
                                    .padding(6)
                            }.buttonStyle(.plain)
                        }
                        if store.filteredInstructions.isEmpty && store.filteredSkills.isEmpty {
                            Text("검색 결과가 없습니다.").foregroundStyle(.secondary).padding(8)
                        }
                    }
                    .padding(8)
                    .frame(maxWidth: 650)
                    .background(Palette.inset, in: RoundedRectangle(cornerRadius: 11))
                    .padding(.bottom, 10)
                }

                Group {
                    switch store.screen {
                    case .overview: OverviewView()
                    case .instructions: InstructionsView()
                    case .bundles: BundlesView()
                    case .skills: SkillsView()
                    case .activity: ActivityView()
                    case .usage: UsageView()
                    case .system: SystemView()
                    }
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
            .toolbar {
                ToolbarItem {
                    Button { store.refresh() } label: { Image(systemName: "arrow.clockwise") }
                        .help("현재 데이터 다시 읽기")
                }
            }
            .background(Color(nsColor: .windowBackgroundColor))
        }
        .tint(Palette.tint)
        .onReceive(Timer.publish(every: 10, on: .main, in: .common).autoconnect()) { _ in store.refresh() }
        .alert("Dev Desk", isPresented: Binding(get: { store.notice != nil }, set: { if !$0 { store.notice = nil } })) {
            Button("확인", role: .cancel) { store.notice = nil }
        } message: { Text(store.notice ?? "") }
        .background {
            Button("") { searchFocused = true }
                .keyboardShortcut("k", modifiers: .command)
                .hidden()
        }
    }
}

private struct PageHeading: View {
    let title: String
    let subtitle: String
    var body: some View {
        VStack(alignment: .leading, spacing: 5) {
            Text(title).font(.system(size: 30, weight: .bold, design: .rounded)).tracking(-0.5)
            Text(subtitle).font(.subheadline).foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.bottom, 12)
    }
}

private struct Surface<Content: View>: View {
    let content: Content
    init(@ViewBuilder content: () -> Content) { self.content = content() }
    var body: some View {
        content
            .padding(18)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(Color(nsColor: .controlBackgroundColor), in: RoundedRectangle(cornerRadius: 17))
            .overlay(RoundedRectangle(cornerRadius: 17).strokeBorder(Color.primary.opacity(0.05)))
    }
}

private struct MarkdownReader: View {
    let source: String

    var body: some View {
        VStack(alignment: .leading, spacing: 13) {
            ForEach(Array(MarkdownParser.blocks(source).enumerated()), id: \.offset) { _, block in
                switch block {
                case let .heading(level, value):
                    inline(value)
                        .font(.system(size: level == 1 ? 24 : level == 2 ? 19 : 16, weight: .semibold))
                        .padding(.top, level == 1 ? 5 : 10)
                case let .paragraph(value):
                    inline(value)
                case let .bullet(value):
                    HStack(alignment: .firstTextBaseline, spacing: 9) {
                        Text("•")
                        inline(value)
                    }.padding(.leading, 8)
                case let .numbered(number, value):
                    HStack(alignment: .firstTextBaseline, spacing: 9) {
                        Text(number).monospacedDigit()
                        inline(value)
                    }.padding(.leading, 8)
                case let .code(value):
                    Text(value)
                        .font(.system(.body, design: .monospaced))
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .padding(12)
                        .background(Palette.inset, in: RoundedRectangle(cornerRadius: 9))
                case let .quote(value):
                    HStack(spacing: 10) {
                        Rectangle().fill(Palette.tint.opacity(0.5)).frame(width: 3)
                        inline(value).foregroundStyle(.secondary)
                    }.fixedSize(horizontal: false, vertical: true)
                case let .table(headers, rows):
                    ScrollView(.horizontal) {
                        Grid(alignment: .leading, horizontalSpacing: 16, verticalSpacing: 9) {
                            GridRow {
                                ForEach(headers.indices, id: \.self) { column in
                                    inline(headers[column]).fontWeight(.semibold).frame(minWidth: 110, maxWidth: 220, alignment: .leading)
                                }
                            }
                            Divider()
                            ForEach(rows.indices, id: \.self) { row in
                                GridRow {
                                    ForEach(headers.indices, id: \.self) { column in
                                        inline(column < rows[row].count ? rows[row][column] : "")
                                            .frame(minWidth: 110, maxWidth: 220, alignment: .leading)
                                    }
                                }
                                Divider()
                            }
                        }
                        .padding(12)
                        .background(Palette.inset, in: RoundedRectangle(cornerRadius: 9))
                    }
                case .divider:
                    Divider().padding(.vertical, 3)
                }
            }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .textSelection(.enabled)
    }

    private func inline(_ value: String) -> Text {
        guard let styled = try? AttributedString(markdown: value, options: .init(interpretedSyntax: .inlineOnlyPreservingWhitespace)) else {
            return Text(verbatim: value)
        }
        return Text(styled)
    }
}

private struct ResponsivePage<Content: View>: View {
    let content: Content
    init(@ViewBuilder content: () -> Content) { self.content = content() }
    var body: some View {
        GeometryReader { geometry in
            ScrollView {
                content
                    .padding(.vertical, 28)
                    .frame(width: max(0, min(1260, geometry.size.width - 56)))
                    .frame(maxWidth: .infinity)
            }
        }
    }
}

private struct Metric: View {
    let icon: String
    let title: String
    let value: String
    let detail: String
    var body: some View {
        Surface {
            Label(title, systemImage: icon).font(.subheadline.weight(.medium)).foregroundStyle(.secondary)
            Text(value).font(.system(size: 29, weight: .semibold, design: .rounded)).contentTransition(.numericText())
                .padding(.top, 9)
            Text(detail).font(.caption).foregroundStyle(.secondary).lineLimit(2)
        }
    }
}

private struct QuotaView: View {
    let title: String
    let quota: QuotaWindow?
    var body: some View {
        VStack(alignment: .leading, spacing: 9) {
            HStack {
                Text(title).font(.subheadline.weight(.medium))
                Spacer()
                Text(quota.map { String(format: "%.0f%% 사용", $0.usedPercent) } ?? "확인 불가")
                    .font(.subheadline.weight(.semibold))
            }
            ProgressView(value: min(max(quota?.usedPercent ?? 0, 0), 100), total: 100)
                .tint((quota?.usedPercent ?? 0) > 85 ? .orange : Palette.tint)
                .accessibilityLabel("\(title) 사용량")
            if let reset = quota?.resetsAt {
                Text("초기화 \(reset.formatted(date: .abbreviated, time: .shortened))")
                    .font(.caption).foregroundStyle(.secondary)
            } else {
                Text("관측 가능한 한도 기록이 없습니다.").font(.caption).foregroundStyle(.secondary)
            }
        }
    }
}

struct OverviewView: View {
    @EnvironmentObject private var store: DashboardStore

    var body: some View {
        ResponsivePage {
            VStack(alignment: .leading, spacing: 20) {
                PageHeading(title: "오늘의 작업 공간", subtitle: "개발에 필요한 상태와 지침을 한눈에 봅니다.")
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 230), spacing: 14)], spacing: 14) {
                    Metric(icon: "battery.75percent", title: "배터리", value: store.system.batteryPercent.map { "\(Int($0))%" } ?? "—", detail: batteryDetail)
                    Metric(icon: "circle.hexagongrid", title: "Codex", value: store.codex.quota.map { "\(Int(max(0, 100 - $0.usedPercent)))%" } ?? "—", detail: "5시간 한도 잔여 · 마지막 관측 기준")
                    Metric(icon: "sparkles", title: "Antigravity", value: "\(store.antigravity.conversationCount)개", detail: "로컬 대화 파일 · 잔여 한도 미연동")
                }
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 360), spacing: 14)], alignment: .leading, spacing: 14) {
                    Surface {
                        Text("AI 사용 한도").font(.headline)
                        Text("Codex · ChatGPT Plus").font(.caption).foregroundStyle(.secondary).padding(.bottom, 9)
                        QuotaView(title: "5시간", quota: store.codex.quota)
                        Divider().padding(.vertical, 5)
                        QuotaView(title: "7일", quota: store.codex.weeklyQuota)
                        Text("마지막 한도 기록: \(store.codex.quotaObservedAt?.formatted(date: .abbreviated, time: .shortened) ?? "없음")")
                            .font(.caption2).foregroundStyle(.tertiary).padding(.top, 6)
                    }
                    Surface {
                        Text("빠른 지침").font(.headline)
                        Text("반복해서 쓰는 지침을 바로 찾고 복사하세요.").font(.caption).foregroundStyle(.secondary).padding(.bottom, 9)
                        if store.instructions.isEmpty {
                            Text("아직 저장한 지침이 없습니다.").foregroundStyle(.secondary).padding(.vertical, 20)
                        } else {
                            ForEach(store.instructions.prefix(4)) { item in
                                HStack {
                                    Text(item.title).lineLimit(1)
                                    Spacer()
                                    Button { store.copy(item.body) } label: { Image(systemName: "doc.on.doc") }
                                        .buttonStyle(.borderless).help("지침 복사")
                                }.padding(.vertical, 5)
                            }
                        }
                        Button("지침 보관함 열기") { store.screen = .instructions }
                            .buttonStyle(.link).padding(.top, 5)
                    }
                }
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 360), spacing: 14)], alignment: .leading, spacing: 14) {
                    Surface {
                        Text("최근 7일 Codex 토큰 · 백만 단위").font(.headline)
                        TokenChart(points: store.codex.dailyTokens)
                            .frame(height: 170)
                    }
                    Surface {
                        Text("배터리 추이").font(.headline)
                        BatteryChart(points: store.batteryHistory)
                            .frame(height: 170)
                    }
                }
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 230), spacing: 14)], spacing: 14) {
                    Metric(icon: "cpu", title: "CPU", value: store.system.cpuPercent.map { String(format: "%.0f%%", $0) } ?? "측정 중", detail: "현재 사용률")
                    Metric(icon: "memorychip", title: "메모리", value: store.system.memoryUsedGB.map { String(format: "%.1f GB", $0) } ?? "—", detail: "전체 \(String(format: "%.1f", store.system.memoryTotalGB ?? 0)) GB")
                    Metric(icon: "bolt", title: "배터리 방전 전력", value: store.system.batteryWatts.map { String(format: "%.1f W", $0) } ?? "—", detail: store.system.onAC ? "전원 연결 중" : "지원되는 센서값 기준 추정")
                }
            }
        }
    }

    private var batteryDetail: String {
        if store.system.onAC { return store.system.charging ? "충전 중" : "전원 연결 · 완충 또는 충전 대기" }
        if let minutes = store.system.remainingMinutes { return "예상 \(minutes / 60)시간 \(minutes % 60)분 사용 가능" }
        return "배터리 사용 중 · 시간 추정 불가"
    }
}

private struct TokenChart: View {
    let points: [(Date, Int)]
    var body: some View {
        let cutoff = Calendar.current.startOfDay(for: Date().addingTimeInterval(-6 * 24 * 3600))
        let visible = points.filter { $0.0 >= cutoff }
        if visible.isEmpty {
            ContentUnavailableView("사용 기록 없음", systemImage: "chart.bar", description: Text("최근 7일 기록이 수집되면 표시됩니다."))
        } else {
            Chart(visible, id: \.0) { point in
                BarMark(x: .value("날짜", point.0, unit: .day), y: .value("백만 토큰", Double(point.1) / 1_000_000))
                    .foregroundStyle(Palette.tint.gradient)
            }
            .chartXAxis { AxisMarks(values: .stride(by: .day)) { AxisGridLine(); AxisValueLabel(format: .dateTime.month().day()) } }
            .accessibilityLabel("최근 7일 Codex 토큰 사용량")
        }
    }
}

private struct BatteryChart: View {
    let points: [BatteryPoint]
    var body: some View {
        if points.count < 2 {
            ContentUnavailableView("측정 시작 전", systemImage: "battery.100percent", description: Text("앱을 사용하는 동안 추이를 기록합니다."))
        } else {
            Chart(points) { point in
                LineMark(x: .value("시간", point.date), y: .value("잔량", point.percent))
                    .foregroundStyle(Palette.tint)
                AreaMark(x: .value("시간", point.date), y: .value("잔량", point.percent))
                    .foregroundStyle(Palette.tint.opacity(0.08))
            }
            .chartYScale(domain: 0...100)
            .accessibilityLabel("최근 배터리 잔량 추이")
        }
    }
}

struct InstructionsView: View {
    @EnvironmentObject private var store: DashboardStore
    @State private var creating = false
    @State private var isEditing = false
    @State private var title = ""
    @State private var bodyText = ""
    @State private var expectedText: String?

    var body: some View {
        GeometryReader { geometry in
        HStack(spacing: 0) {
            VStack(spacing: 0) {
                HStack {
                    Text("지침 보관함").font(.headline)
                    Spacer()
                    Button { newItem() } label: { Image(systemName: "plus") }
                        .help("새 지침")
                }.padding(18)
                List(selection: $store.selectedInstructionID) {
                    ForEach(store.filteredInstructions) { item in
                        VStack(alignment: .leading, spacing: 4) {
                            Text(item.title).font(.subheadline.weight(.medium))
                            Text(item.body.replacingOccurrences(of: "\n", with: " "))
                                .font(.caption).foregroundStyle(.secondary).lineLimit(2)
                        }
                        .padding(.vertical, 4)
                        .tag(item.id)
                    }
                }
                .listStyle(.inset)
            }
            .frame(width: min(290, max(180, geometry.size.width * 0.38)))
            Divider()
            if creating || isEditing {
                editor
            } else if let item = selectedItem {
                reader(item)
            } else {
                ContentUnavailableView("지침을 선택하세요", systemImage: "text.book.closed", description: Text("새 지침을 만들거나 AI가 data/library에 추가한 파일을 선택할 수 있습니다."))
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        }
        .onChange(of: store.selectedInstructionID) { _, id in
            if let id, let item = store.instructions.first(where: { $0.id == id }) { load(item) }
        }
        .onChange(of: store.newInstructionRequest) { _, _ in newItem() }
    }

    private func reader(_ item: Instruction) -> some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                HStack {
                    Text(item.title).font(.title2.bold())
                    Spacer()
                    Button("본문 복사", systemImage: "doc.on.doc") { store.copy(item.body) }
                    Button("수정") { load(item); isEditing = true }
                        .buttonStyle(.borderedProminent)
                }
                MarkdownReader(source: item.body)
            }
            .padding(24)
            .frame(maxWidth: 800, alignment: .leading)
            .frame(maxWidth: .infinity)
        }
    }

    private var editor: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                HStack {
                    Text(creating ? "새 지침" : "지침 편집").font(.title2.bold())
                    Spacer()
                    if !creating, let item = selectedItem {
                        Button("삭제", role: .destructive) { store.deleteInstruction(item); isEditing = false }
                    }
                    Button("취소") { creating = false; isEditing = false }
                    Button("저장") {
                        if store.saveInstruction(id: creating ? nil : store.selectedInstructionID, title: title, body: bodyText, expectedText: creating ? nil : expectedText) {
                            creating = false
                            isEditing = false
                            if let item = selectedItem { expectedText = store.library.text(for: item) }
                        }
                    }
                    .buttonStyle(.borderedProminent)
                    .keyboardShortcut("s", modifiers: .command)
                }
                TextField("제목", text: $title).font(.title3).textFieldStyle(.roundedBorder)
                Text("사용 지침 · Markdown").font(.subheadline.weight(.medium))
                TextEditor(text: $bodyText)
                    .font(.system(.body, design: .monospaced))
                    .scrollContentBackground(.hidden)
                    .padding(9)
                    .frame(minHeight: 300)
                    .background(Palette.inset, in: RoundedRectangle(cornerRadius: 12))
                Text("미리보기").font(.subheadline.weight(.medium))
                Surface {
                    if bodyText.isEmpty { Text("내용을 입력하면 여기에 보입니다.").foregroundStyle(.secondary) }
                    else { MarkdownReader(source: bodyText) }
                }
                Text("AI도 이 폴더에 Markdown 파일을 추가할 수 있습니다: \(store.library.folder.path)")
                    .font(.caption).foregroundStyle(.secondary).textSelection(.enabled)
                Button("AI에게 지침 추가 요청 복사", systemImage: "sparkles") {
                    store.copy("다음 지침을 \(store.library.folder.path)에 새 UTF-8 Markdown 파일로 저장해줘. 파일 이름은 새 UUID와 .md 확장자로 만들고, 첫 줄은 '# 제목', 빈 줄 다음에는 지침 본문을 써줘. 기존 파일은 덮어쓰지 말아줘. 저장할 지침: ")
                }
                .buttonStyle(.link)
            }
            .padding(24)
            .frame(maxWidth: 800, alignment: .leading)
            .frame(maxWidth: .infinity)
        }
    }

    private var selectedItem: Instruction? {
        store.instructions.first { $0.id == store.selectedInstructionID }
    }

    private func load(_ item: Instruction) {
        creating = false
        isEditing = false
        title = item.title
        bodyText = item.body
        expectedText = store.library.text(for: item)
    }

    private func newItem() {
        store.selectedInstructionID = nil
        creating = true
        isEditing = true
        title = ""
        bodyText = ""
        expectedText = nil
    }
}

struct BundlesView: View {
    @EnvironmentObject private var store: DashboardStore
    @State private var draft = InstructionBundle(title: "")
    @State private var editing = false

    var body: some View {
        GeometryReader { geometry in
        HStack(spacing: 0) {
            VStack {
                HStack {
                    Text("지침 묶음").font(.headline)
                    Spacer()
                    Button { draft = InstructionBundle(title: ""); store.selectedBundleID = nil; editing = true } label: { Image(systemName: "plus") }
                        .help("새 묶음")
                }.padding(18)
                List(selection: $store.selectedBundleID) {
                    ForEach(store.bundles) { item in
                        VStack(alignment: .leading, spacing: 3) {
                            Text(item.title)
                            Text("\(item.instructionIDs.count)개 지침").font(.caption).foregroundStyle(.secondary)
                        }.tag(item.id)
                    }
                }.listStyle(.inset)
            }.frame(width: min(290, max(180, geometry.size.width * 0.38)))
            Divider()
            if editing {
                ScrollView {
                    VStack(alignment: .leading, spacing: 18) {
                        HStack {
                            Text("묶음 편집").font(.title2.bold())
                            Spacer()
                            if store.bundles.contains(where: { $0.id == draft.id }) {
                                Button("삭제", role: .destructive) { store.deleteBundle(draft); editing = false }
                            }
                            Button("저장") { store.saveBundle(draft) }
                                .buttonStyle(.borderedProminent)
                        }
                        TextField("묶음 이름", text: $draft.title).textFieldStyle(.roundedBorder)
                        HStack {
                            Text("순서대로 합쳐집니다").font(.headline)
                            Spacer()
                            Menu("지침 추가", systemImage: "plus") {
                                ForEach(store.instructions.filter { !draft.instructionIDs.contains($0.id) }) { item in
                                    Button(item.title) { draft.instructionIDs.append(item.id) }
                                }
                            }
                        }
                        if draft.instructionIDs.isEmpty { Text("지침을 추가하세요.").foregroundStyle(.secondary) }
                        ForEach(Array(draft.instructionIDs.enumerated()), id: \.element) { index, id in
                            HStack {
                                Text(store.instructions.first(where: { $0.id == id })?.title ?? "삭제된 지침")
                                Spacer()
                                Button { draft.instructionIDs.swapAt(index, index - 1) } label: { Image(systemName: "arrow.up") }
                                    .disabled(index == 0).help("위로")
                                Button { draft.instructionIDs.swapAt(index, index + 1) } label: { Image(systemName: "arrow.down") }
                                    .disabled(index == draft.instructionIDs.count - 1).help("아래로")
                                Button { draft.instructionIDs.remove(at: index) } label: { Image(systemName: "minus.circle") }
                                    .help("묶음에서 제거")
                            }.padding(10).background(Palette.inset, in: RoundedRectangle(cornerRadius: 8))
                        }
                        HStack {
                            Text("합친 지침 미리보기").font(.headline)
                            Spacer()
                            Button("전체 복사", systemImage: "doc.on.doc") { store.copy(composedText) }
                                .disabled(composedText.isEmpty)
                        }
                        Surface {
                            if composedText.isEmpty { Text("지침을 추가하면 결과가 보입니다.").foregroundStyle(.secondary) }
                            else { MarkdownReader(source: composedText) }
                        }
                    }
                    .padding(24)
                    .frame(maxWidth: 800, alignment: .leading)
                    .frame(maxWidth: .infinity)
                }
            } else {
                ContentUnavailableView("묶음을 선택하세요", systemImage: "square.stack", description: Text("자주 쓰는 지침을 순서대로 모아 한 번에 복사할 수 있습니다."))
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        }
        .onChange(of: store.selectedBundleID) { _, id in
            if let item = store.bundles.first(where: { $0.id == id }) { draft = item; editing = true }
        }
    }

    private var composedText: String {
        draft.instructionIDs.compactMap { id in
            store.instructions.first(where: { $0.id == id })?.body
        }.joined(separator: "\n\n---\n\n")
    }
}

struct SkillsView: View {
    @EnvironmentObject private var store: DashboardStore
    var body: some View {
        GeometryReader { geometry in
        HStack(spacing: 0) {
            VStack {
                HStack {
                    Text("로컬 스킬").font(.headline)
                    Spacer()
                    Text("\(store.filteredSkills.count)").font(.caption).foregroundStyle(.secondary)
                }.padding(18)
                List(selection: $store.selectedSkillID) {
                    ForEach(store.filteredSkills) { skill in
                        VStack(alignment: .leading, spacing: 3) {
                            Text(skill.name).font(.subheadline.weight(.medium))
                            Text(skill.summary).font(.caption).foregroundStyle(.secondary).lineLimit(2)
                        }.padding(.vertical, 3).tag(skill.id)
                    }
                }.listStyle(.inset)
            }.frame(width: min(300, max(180, geometry.size.width * 0.38)))
            Divider()
            if let skill = store.skills.first(where: { $0.id == store.selectedSkillID }) {
                ScrollView {
                    VStack(alignment: .leading, spacing: 14) {
                        Text(skill.name).font(.title2.bold())
                        Text(skill.summary).foregroundStyle(.secondary)
                        Text(skill.path.path).font(.caption.monospaced()).foregroundStyle(.secondary).textSelection(.enabled)
                        HStack {
                            Label("파일 발견", systemImage: "checkmark.circle.fill").foregroundStyle(.green)
                            Text("실행과 지침 준수 여부는 이 화면만으로 확인할 수 없습니다.").font(.caption).foregroundStyle(.secondary)
                        }
                        Button("사용법 복사", systemImage: "doc.on.doc") { store.copy(skill.content) }
                        Divider()
                        MarkdownReader(source: skill.content)
                    }
                    .padding(24).frame(maxWidth: 850, alignment: .leading).frame(maxWidth: .infinity)
                }
            } else {
                ContentUnavailableView("스킬을 선택하세요", systemImage: "sparkles.rectangle.stack", description: Text("설치된 SKILL.md의 원문과 사용법을 바로 볼 수 있습니다."))
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        }
    }
}

struct ActivityView: View {
    @EnvironmentObject private var store: DashboardStore
    var body: some View {
        ResponsivePage {
            VStack(alignment: .leading, spacing: 18) {
                PageHeading(title: "AI 활동", subtitle: "로컬 CLI 기록을 약 30초 간격으로 다시 읽습니다. 계정 실시간 상태와는 다를 수 있습니다.")
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 250), spacing: 14)], spacing: 14) {
                    Metric(icon: "circle.hexagongrid", title: "Codex 세션", value: "\(store.codex.sessions)", detail: "최근 30일 토큰 기록이 있는 세션")
                    Metric(icon: "sparkles", title: "Antigravity 대화", value: "\(store.antigravity.conversationCount)", detail: "로컬에 저장된 대화 파일")
                }
                Surface {
                    Text("최근 관측").font(.headline)
                    HStack {
                        Label("Codex", systemImage: "circle.hexagongrid")
                        Spacer()
                        Text(store.codex.latestActivity?.formatted(date: .abbreviated, time: .shortened) ?? "기록 없음")
                    }.padding(.top, 12)
                    Divider()
                    HStack {
                        Label("Antigravity", systemImage: "sparkles")
                        Spacer()
                        Text(store.antigravity.latestActivity?.formatted(date: .abbreviated, time: .shortened) ?? "기록 없음")
                    }
                }
                Surface {
                    Text("데이터 연결 상태").font(.headline)
                    Text("맥 상태: 약 10초 간격 · 이 Mac에서 직접 측정")
                    Text("Codex: 로컬 세션 파일 · 마지막 읽기 \(store.codex.lastRead.formatted(date: .omitted, time: .shortened))")
                    Text("Antigravity: 로컬 대화 파일 · 마지막 읽기 \(store.antigravity.lastRead.formatted(date: .omitted, time: .shortened))")
                    Text("Antigravity 잔여 한도와 스킬 실행 성공은 현재 연결되지 않았습니다.")
                        .foregroundStyle(.secondary)
                }
                Surface {
                    Text("스킬 상태의 의미").font(.headline)
                    Text("스킬 화면의 ‘파일 발견’은 설치 파일이 있다는 뜻입니다. 실제 호출 여부와 결과 품질은 도구가 남긴 실행 기록을 확인해야 하며, 현재 관측되지 않은 항목은 미확인으로 유지합니다.")
                        .foregroundStyle(.secondary).padding(.top, 5)
                }
            }
        }
    }
}

struct UsageView: View {
    @EnvironmentObject private var store: DashboardStore
    var body: some View {
        ResponsivePage {
            VStack(alignment: .leading, spacing: 18) {
                PageHeading(title: "사용량과 비용", subtitle: "측정한 사용량과 실제 구독 결제 정보를 구분해서 봅니다.")
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 230), spacing: 14)], spacing: 14) {
                    Metric(icon: "text.word.spacing", title: "Codex 입력", value: store.codex.inputTokens.formatted(), detail: "최근 30일 · 로컬 기록")
                    Metric(icon: "text.bubble", title: "Codex 출력", value: store.codex.outputTokens.formatted(), detail: "최근 30일 · 로컬 기록")
                    Metric(icon: "sparkles", title: "Antigravity 토큰", value: "수집 전", detail: "구독 한도와 세션 토큰은 연결 후 표시")
                }
                Surface {
                    Text("Codex 한도").font(.headline)
                    QuotaView(title: "5시간", quota: store.codex.quota).padding(.top, 12)
                    Divider().padding(.vertical, 8)
                    QuotaView(title: "7일", quota: store.codex.weeklyQuota)
                    Text("마지막 한도 기록: \(store.codex.quotaObservedAt?.formatted(date: .abbreviated, time: .shortened) ?? "없음")")
                        .font(.caption).foregroundStyle(.secondary).padding(.top, 8)
                }
                Surface {
                    Text("최근 7일 Codex 사용 추이").font(.headline)
                    TokenChart(points: store.codex.dailyTokens).frame(height: 230).padding(.top, 10)
                }
                Surface {
                    Text("Antigravity 한도").font(.headline)
                    Text("현재 앱으로 전달되는 한도 데이터가 없습니다. Antigravity CLI의 /usage 화면에서 확인할 수 있습니다. 공식 상태 데이터 연결 전까지 숫자를 추정하지 않습니다.")
                        .foregroundStyle(.secondary).padding(.top, 6)
                }
                ForEach(store.subscriptions) { item in
                    SubscriptionCard(item: item)
                }
                Surface {
                    Text("API 요금 기준 환산액").font(.headline)
                    Text("현재 기록에는 모델별 입력·출력·캐시 토큰과 적용 단가가 모두 확인되지 않아 계산하지 않습니다. 구독료를 사용량 기반 청구액으로 표시하지 않습니다.")
                        .foregroundStyle(.secondary).padding(.top, 6)
                }
            }
        }
    }
}

private struct SubscriptionCard: View {
    @EnvironmentObject private var store: DashboardStore
    let item: Subscription
    @State private var editing = false
    @State private var amount = ""
    @State private var date = ""
    @State private var laterAmount = ""
    @State private var laterDate = ""

    var body: some View {
        Surface {
            HStack {
                Text(item.name).font(.headline)
                Spacer()
                Button(editing ? "저장" : "수정") {
                    if editing { save() } else { beginEdit() }
                }
            }
            if editing {
                VStack(alignment: .leading, spacing: 8) {
                    Text("다음 결제일")
                    TextField("YYYY-MM-DD", text: $date)
                    Text("다음 금액")
                    TextField("원", text: $amount)
                    Text("이후 변경일")
                    TextField("YYYY-MM-DD", text: $laterDate)
                    Text("변경 금액")
                    TextField("원", text: $laterAmount)
                }.textFieldStyle(.roundedBorder).padding(.top, 12)
            } else {
                HStack {
                    Text("다음 결제").foregroundStyle(.secondary)
                    Spacer()
                    Text(dateText(item.nextPayment))
                    Text(item.nextAmountKRW.map { "\($0.formatted())원" } ?? "금액 미입력").fontWeight(.semibold)
                }.padding(.top, 12)
                if item.laterPayment != nil || item.laterAmountKRW != nil {
                    HStack {
                        Text("이후 요금").foregroundStyle(.secondary)
                        Spacer()
                        Text(dateText(item.laterPayment))
                        Text(item.laterAmountKRW.map { "월 \($0.formatted())원" } ?? "미입력")
                    }.padding(.top, 5)
                }
                Text(item.note).font(.caption).foregroundStyle(.secondary).padding(.top, 8)
            }
        }
    }

    private func dateText(_ value: Date?) -> String { value?.formatted(date: .abbreviated, time: .omitted) ?? "날짜 미입력" }
    private func iso(_ value: Date?) -> String {
        guard let value else { return "" }
        let formatter = ISO8601DateFormatter()
        formatter.formatOptions = [.withFullDate]
        return formatter.string(from: value)
    }
    private func parse(_ value: String) -> Date? {
        let formatter = ISO8601DateFormatter()
        formatter.formatOptions = [.withFullDate]
        return formatter.date(from: value)
    }
    private func beginEdit() {
        amount = item.nextAmountKRW.map(String.init) ?? ""
        date = iso(item.nextPayment)
        laterAmount = item.laterAmountKRW.map(String.init) ?? ""
        laterDate = iso(item.laterPayment)
        editing = true
    }
    private func save() {
        guard (date.isEmpty || parse(date) != nil), (laterDate.isEmpty || parse(laterDate) != nil),
              (amount.isEmpty || Int(amount) != nil), (laterAmount.isEmpty || Int(laterAmount) != nil) else {
            store.notice = "날짜는 YYYY-MM-DD, 금액은 숫자로 입력하세요."
            return
        }
        var changed = item
        changed.nextPayment = parse(date)
        changed.nextAmountKRW = Int(amount)
        changed.laterPayment = parse(laterDate)
        changed.laterAmountKRW = Int(laterAmount)
        changed.note = "앱에서 직접 수정 · \(Date().formatted(date: .abbreviated, time: .omitted))"
        store.saveSubscription(changed)
        editing = false
    }
}

struct SystemView: View {
    @EnvironmentObject private var store: DashboardStore
    var body: some View {
        ResponsivePage {
            VStack(alignment: .leading, spacing: 18) {
                PageHeading(title: "맥 상태", subtitle: "이 Mac에서 읽은 현재 상태와 최근 추이입니다.")
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 230), spacing: 14)], spacing: 14) {
                    Metric(icon: "battery.75percent", title: "배터리", value: store.system.batteryPercent.map { "\(Int($0))%" } ?? "—", detail: store.system.onAC ? "전원 연결" : "배터리 사용 중")
                    Metric(icon: "clock", title: "예상 사용 시간", value: store.system.remainingMinutes.map { "\($0 / 60)시간 \($0 % 60)분" } ?? "—", detail: store.system.onAC ? "전원 연결 중" : "운영체제 추정값 · 변동 가능")
                    Metric(icon: "bolt", title: "방전 전력", value: store.system.batteryWatts.map { String(format: "%.1f W", $0) } ?? "—", detail: "지원되는 배터리 센서 기준")
                }
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 230), spacing: 14)], spacing: 14) {
                    Metric(icon: "cpu", title: "CPU 사용률", value: store.system.cpuPercent.map { String(format: "%.0f%%", $0) } ?? "측정 중", detail: "최근 두 표본 기준")
                    Metric(icon: "memorychip", title: "메모리", value: store.system.memoryUsedGB.map { String(format: "%.1f GB", $0) } ?? "—", detail: "전체 \(String(format: "%.1f GB", store.system.memoryTotalGB ?? 0))")
                }
                Surface {
                    Text("배터리 잔량 · 최근 24시간").font(.headline)
                    BatteryChart(points: store.batteryHistory).frame(height: 260).padding(.top, 12)
                }
                Text("마지막 측정: \(store.system.sampledAt.formatted(date: .abbreviated, time: .shortened)) · 종료 시각은 부하 변화에 따라 달라집니다.")
                    .font(.caption).foregroundStyle(.secondary)
            }
        }
    }
}
