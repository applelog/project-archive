import Foundation

let loadedAt = Date()
let graph = try JSONDecoder().decode(WalkGraph.self,
    from: Data(contentsOf: URL(fileURLWithPath: "Data/network.json")))
print(String(format: "decode: %.3fs", Date().timeIntervalSince(loadedAt)))
let preparedAt = Date()
let router = WheelchairRouter(graph: graph)
print(String(format: "prepare: %.3fs", Date().timeIntervalSince(preparedAt)))
for (label, start, destination) in [
    ("short", GeoPoint(latitude: 37.4947, longitude: 126.8585), GeoPoint(latitude: 37.4917, longitude: 126.8549)),
    ("city", GeoPoint(latitude: 37.5665, longitude: 126.978), GeoPoint(latitude: 37.5703, longitude: 126.983)),
    ("long", GeoPoint(latitude: 37.5445, longitude: 127.037), GeoPoint(latitude: 37.5404, longitude: 127.0693)),
] {
    var costs: [Double] = []
    let began = Date()
    for mode in [RouteMode.fastest, .shortest, .gentle] {
        let found = router.route(from: start, to: destination,
            maxMeasuredGradePercent: 8.33, mode: mode, allowUnverifiedGrade: true)
        costs.append(found?.route.totalCost ?? -1)
    }
    print(String(format: "%@: three modes %.3fs, costs %@", label,
        Date().timeIntervalSince(began), costs.map { String(format: "%.3f", $0) }.joined(separator: ",")))
}
