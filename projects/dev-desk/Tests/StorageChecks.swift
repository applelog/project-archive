import Foundation

@main
struct StorageChecks {
    static func main() throws {
        let root = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)
        let store = LibraryStore(root: root)
        let item = try store.save(id: nil, title: "시험 지침", body: "첫 내용", expectedText: nil)
        let titles = try store.instructions().map(\.title)
        assert(titles == ["시험 지침"])
        let original = store.text(for: item)!
        let edited = try store.save(id: item.id, title: item.title, body: "고친 내용", expectedText: original)
        assert(edited.body == "고친 내용")
        do {
            _ = try store.save(id: item.id, title: item.title, body: "덮어쓰기", expectedText: original)
            fatalError("동시 수정 감지가 실패했습니다")
        } catch StoreError.conflict { }
        let bundle = InstructionBundle(title: "묶음", instructionIDs: [item.id])
        try store.saveBundles([bundle])
        assert(store.bundles() == [bundle])
        try store.delete(edited)
        let remaining = try store.instructions()
        assert(remaining.isEmpty)
        let blocks = MarkdownParser.blocks("## 사용법\n\n- 항목\n1. 순서\n\n```bash\necho '# 그대로'\n```\n\n[공식 문서](https://example.com)")
        assert(blocks == [.heading(2, "사용법"), .bullet("항목"), .numbered("1.", "순서"), .code("echo '# 그대로'"), .paragraph("[공식 문서](https://example.com)")])
        let skillBlocks = MarkdownParser.blocks("---\nname: sample\n---\n# 제목\n\n| 항목 | 설명 |\n| --- | --- |\n| A | **강조** |")
        assert(skillBlocks == [.heading(1, "제목"), .table(["항목", "설명"], [["A", "**강조**"]])])
        print("StorageChecks passed")
    }
}
