import Foundation

struct StationGuide: Codable {
    let source: String
    let reviewedAt: String?
    let routes: [StationRoute]
}

struct StationRoute: Codable, Identifiable {
    let id: String
    let title: String
    let entranceNodeID: String
    let officialRouteID: String
    let steps: [StationStep]
}

struct StationStep: Codable, Identifiable {
    let order: Int
    let text: String
    let fromName: String
    let toName: String

    var id: Int { order }

    enum CodingKeys: String, CodingKey {
        case order, text
        case fromName = "from"
        case toName = "to"
    }
}
