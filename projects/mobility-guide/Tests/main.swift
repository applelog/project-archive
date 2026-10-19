import Foundation

var failures = 0
func check(_ condition: @autoclosure () -> Bool, _ message: String) {
    if !condition() {
        fputs("FAIL: \(message)\n", stderr)
        failures += 1
    }
}
func close(_ first: Double?, _ second: Double, tolerance: Double = 0.01) -> Bool {
    guard let first else { return false }
    return abs(first - second) <= tolerance
}
func node(_ id: String, _ latitude: Double, _ longitude: Double) -> WalkNode {
    WalkNode(id: id, latitude: latitude, longitude: longitude)
}
func edge(_ id: String, _ from: WalkNode, _ to: WalkNode, length: Double? = nil,
          grade: Double? = 0, evidence: DataEvidence = .fieldChecked,
          obstacle: WalkObstacle = .none, reverseAllowed: Bool? = true,
          uphill: Double? = nil, downhill: Double? = nil) -> WalkEdge {
    WalkEdge(id: id, from: from.id, to: to.id,
             length: length ?? from.coordinate.distance(to: to.coordinate),
             coordinates: [from.coordinate, to.coordinate], gradePercent: grade,
             gradeEvidence: evidence, obstacle: obstacle, reverseAllowed: reverseAllowed,
             forwardMaxUphillPercent: uphill, forwardMaxDownhillPercent: downhill)
}
func router(_ nodes: [WalkNode], _ edges: [WalkEdge]) -> WheelchairRouter {
    WheelchairRouter(graph: WalkGraph(nodes: nodes, edges: edges, entrances: []))
}

let a = node("A", 37, 127), b = node("B", 37, 127.002)
let c = node("C", 37.001, 127), d = node("D", 37.001, 127.002)
let detour = [edge("AC", a, c, length: 130, grade: 4), edge("CD", c, d, length: 150)]
let safety = router([a, b, c, d], detour + [
    edge("AB", a, b, length: 90, grade: 12), edge("BD", b, d, length: 90),
    edge("AD", a, d, length: 50, obstacle: .stairs),
])
check(safety.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 8.33,
                   mode: .shortest)?.edgeIDs == ["AC", "CD"], "계단과 경사 한도 초과를 제외합니다")
check(safety.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 12.5,
                   mode: .shortest)?.edgeIDs == ["AB", "BD"], "한도 내 최단 경로를 선택합니다")
check(safety.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 12.5,
                   mode: .gentle)?.edgeIDs == ["AC", "CD"], "완만 모드는 경사 부담을 반영합니다")
check(safety.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 12.5,
                   mode: .shortest, requiresStepFree: false)?.instructions.first?.usesStairs == true,
      "일반 보행 계단은 안내에 표시합니다")
for obstacle in [WalkObstacle.stairs, .escalator, .impassable, .unverifiedOverpass, .wheelchairRestricted] {
    let r = router([a, b], [edge("AB", a, b, obstacle: obstacle)])
    check(r.route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33,
                  mode: .shortest) == nil, "단차 없는 경로에서 \(obstacle)을 제외합니다")
}
let wheelchairRestricted = router([a, b], [edge("AB", a, b, obstacle: .wheelchairRestricted)])
check(wheelchairRestricted.route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33,
    mode: .shortest, requiresStepFree: false) != nil, "휠체어 제한 표기는 일반 보행을 막지 않습니다")
for (grade, evidence) in [(Optional(-12.0), DataEvidence.fieldChecked), (nil, .unknown), (Optional(2.0), .estimated)] {
    let r = router([a, b], [edge("AB", a, b, grade: grade, evidence: evidence)])
    check(r.route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33,
                  mode: .shortest) == nil, "엄격 모드에서 음수 급경사/미확인/추정 경사를 제외합니다")
}
let uncertain = router([a, b], [edge("AB", a, b, grade: nil, evidence: .unknown)])
let standard = uncertain.route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33,
                               mode: .shortest, requiresStepFree: false)
check(standard?.unknownEdgeCount == 1 && standard?.segments.first?.needsVerification == true,
      "일반 보행 미확인 경사는 허용하되 경고합니다")
let preview = uncertain.route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33,
                              mode: .shortest, allowUnverifiedGrade: true)
check(preview?.unknownEdgeCount == 1, "휠체어 미확인 경사 후보를 별도로 계산합니다")
let previewBlocked = router([a, b, c], [
    edge("AB", a, b, grade: nil, evidence: .unknown, obstacle: .stairs),
    edge("BC", b, c, grade: nil, evidence: .unknown),
])
check(previewBlocked.route(from: a.coordinate, to: "C", maxMeasuredGradePercent: 8.33,
    mode: .shortest, allowUnverifiedGrade: true) == nil,
    "휠체어 미확인 후보에서도 계단을 허용하지 않습니다")
check(safety.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 8.33,
    mode: .shortest, allowUnverifiedGrade: true)?.edgeIDs == ["AC", "CD"],
    "미확인 후보도 실측 한도 초과 경사를 허용하지 않습니다")
let line = router([a, b], [edge("AB", a, b)])
let estimatedHill = edge("estimated", a, b, length: 100, grade: nil, evidence: .estimated,
                         uphill: 10, downhill: 2)
check(!estimatedHill.isUsable(maxMeasuredGradePercent: 8.33, allowUnverifiedGrade: true),
      "휠체어 참고 후보에서도 추정 한도 초과 경사를 제외합니다")
check(estimatedHill.isUsable(maxMeasuredGradePercent: 8.33, requiresStepFree: false),
      "일반 보행에는 추정 급경사를 비용과 함께 허용합니다")
check(estimatedHill.confirmedGrades(forward: true) == nil && estimatedHill.needsVerification,
      "추정 값을 확인된 경사로 승격하지 않습니다")
check(close(estimatedHill.availableGrades(forward: false)?.up, 2), "역방향 추정 오르막/내리막을 교환합니다")
check(estimatedHill.estimatedDuration(for: 100, speedMetersPerSecond: 1) > 100 &&
      estimatedHill.cost(for: 100, mode: .gentle) > 100, "추정 경사를 ETA와 완만 비용에 반영합니다")
let estimatedChoice = router([a, b, c, d], [
    edge("AB", a, b, length: 100, grade: 10, evidence: .estimated),
    edge("BD", b, d, length: 100, grade: 10, evidence: .estimated),
    edge("AC", a, c, length: 150, grade: 1, evidence: .estimated),
    edge("CD", c, d, length: 150, grade: 1, evidence: .estimated),
])
for mode in [RouteMode.fastest, .shortest, .gentle] {
    let found = estimatedChoice.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 12,
        mode: mode, allowUnverifiedGrade: true)
    let reference = estimatedChoice.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 12,
        mode: mode, allowUnverifiedGrade: true, useHeuristic: false)
    check(found?.edgeIDs == (mode == .shortest ? ["AB", "BD"] : ["AC", "CD"]),
          "추정 경사로 짧은 길과 완만/빠른 우회가 달라집니다")
    check(close(found?.totalCost, reference?.totalCost ?? -1), "추정 경사 A*와 Dijkstra 비용이 일치합니다")
}
let profiled = WalkEdge(id: "profiled", from: a.id, to: b.id, length: 100,
    coordinates: [a.coordinate, b.coordinate], gradePercent: nil, gradeEvidence: .estimated,
    obstacle: .none, reverseAllowed: true, forwardMaxUphillPercent: 2, forwardMaxDownhillPercent: 0,
    elevationProfile: [ElevationSample(distanceMeters: 0, elevationMeters: 10),
                       ElevationSample(distanceMeters: 50, elevationMeters: 11),
                       ElevationSample(distanceMeters: 100, elevationMeters: 12)])
let slicedProfile = profiled.profile(from: 0.25, to: 0.75)
check(close(slicedProfile.first?.elevationMeters, 10.5) && close(slicedProfile.last?.distanceMeters, 50),
      "중간 출발/도착 고도 단면은 해당 부분만 보간합니다")
let reversedProfile = profiled.profile(from: 0.75, to: 0.25)
check(close(reversedProfile.first?.elevationMeters, 11.5) && close(reversedProfile.last?.elevationMeters, 10.5),
      "역방향 고도 단면을 이동 방향으로 표시합니다")
let decodedProfiled = try JSONDecoder().decode(WalkEdge.self, from: JSONEncoder().encode(profiled))
check(decodedProfiled.hasValidElevationProfile, "간결한 고도 쌍을 JSON으로 왕복합니다")
let profiledRoute = router([a, b], [profiled]).route(
    from: GeoPoint(latitude: 37, longitude: 127.0015),
    to: GeoPoint(latitude: 37, longitude: 127.0005), maxMeasuredGradePercent: 8.33,
    mode: .shortest, allowUnverifiedGrade: true, maxSnapMeters: 0.1, maxDestinationSnapMeters: 0.1)?.route
check(close(profiledRoute?.segments.first?.elevationProfile.first?.elevationMeters, 11.5) &&
      close(profiledRoute?.segments.first?.elevationProfile.last?.elevationMeters, 10.5),
      "라우터의 부분 역방향 경로에도 올바른 고도 단면을 연결합니다")
let brokenProfile = WalkEdge(id: "broken", from: a.id, to: b.id, length: 100,
    coordinates: [a.coordinate, b.coordinate], gradePercent: 2, gradeEvidence: .estimated,
    obstacle: .none, reverseAllowed: true,
    elevationProfile: [ElevationSample(distanceMeters: 0, elevationMeters: Double.infinity),
                       ElevationSample(distanceMeters: 100, elevationMeters: 12)])
check(router([a, b], [brokenProfile]).route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33,
      mode: .shortest, allowUnverifiedGrade: true) == nil, "비유한 고도 단면을 가진 자료는 라우팅 전에 거부합니다")
let p = GeoPoint(latitude: 37, longitude: 127.0004)
let q = GeoPoint(latitude: 37, longitude: 127.0014)
let partial = line.route(from: p, to: q, maxMeasuredGradePercent: 8.33, mode: .shortest)
check(close(partial?.route.distanceMeters, p.distance(to: q), tolerance: 0.03),
      "같은 링크 중간 출발/도착은 부분구간만 이동합니다")
check((partial?.destinationGapMeters ?? .infinity) < 0.01, "목적지도 링크에 투영합니다")
check(line.route(from: GeoPoint(latitude: 38, longitude: 128), to: "B",
                  maxMeasuredGradePercent: 8.33, mode: .shortest) == nil, "먼 위치에 임의 경로를 만들지 않습니다")
let blocked = router([a, b, c, d], [edge("AB", a, b, obstacle: .stairs), edge("CD", c, d)])
check(blocked.route(from: p, to: "D", maxMeasuredGradePercent: 8.33,
                    mode: .shortest, maxSnapMeters: 200) == nil, "가까운 불가 링크를 건너 먼 링크에 연결하지 않습니다")
let nearA = node("nearA", 37, 127), nearB = node("nearB", 37, 127.002)
let islandA = node("islandA", 37.00015, 127.001), islandB = node("islandB", 37.00015, 127.0011)
let nearDestination = GeoPoint(latitude: 37.00015, longitude: 127.00105)
let isolatedNearest = router([nearA, nearB, islandA, islandB], [
    edge("main", nearA, nearB), edge("island", islandA, islandB)
])
let reachableNearDestination = isolatedNearest.route(from: nearA.coordinate, to: nearDestination,
    maxMeasuredGradePercent: 8.33, mode: .shortest, requiresStepFree: false)
check(reachableNearDestination?.route.edgeIDs == ["main"] &&
      (reachableNearDestination?.destinationGapMeters ?? 0) > 10,
      "가장 가까운 고립 링크 대신 연결된 목적지 인근 보행로로 안내하고 남은 간격을 표시합니다")
check(isolatedNearest.route(from: nearA.coordinate, to: nearDestination,
    maxMeasuredGradePercent: 8.33, mode: .shortest) == nil,
    "계단 없는 경로에는 고립된 가장 가까운 지점을 건너뛰어 도착했다고 표시하지 않습니다")
let distant = node("Z", 60, -120)
check(close(a.coordinate.distance(to: distant.coordinate), distant.coordinate.distance(to: a.coordinate), tolerance: 0.000001),
      "위도와 지역에 관계없이 거리는 대칭입니다")
let otherA = node("A", -33.86, 151.20), otherB = node("B", -33.86, 151.202)
check(router([otherA, otherB], [edge("AB", otherA, otherB)]).route(from: otherA.coordinate,
    to: "B", maxMeasuredGradePercent: 8.33, mode: .shortest) != nil, "다른 나라 그래프도 같은 규칙으로 탐색합니다")
var bounded = WalkGraph(nodes: [a, b], edges: [edge("AB", a, b)], entrances: [])
bounded.coverageBBox = [126.9, 36.9, 127.1, 37.1]
check(bounded.covers(a.coordinate) && !bounded.covers(otherA.coordinate), "가져온 자료의 지역 범위를 구분합니다")
check(safety.route(from: a.coordinate, to: "missing", maxMeasuredGradePercent: 8.33,
                   mode: .shortest) == nil, "없는 목적지에는 경로를 만들지 않습니다")


// Cost validation and ETA use the same confirmed slope/obstacle model as routing.
let flatEdge = edge("flat", a, b, length: 100)
check(!flatEdge.cost(for: -1, mode: .shortest).isFinite, "비정상 부분 거리의 비용을 허용하지 않습니다")
check(!flatEdge.cost(for: 100, mode: .fastest, speedMetersPerSecond: 0).isFinite,
      "0 또는 비정상 속도로 ETA를 생성하지 않습니다")
let fastest = safety.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 12.5, mode: .fastest)
check(fastest?.edgeIDs == ["AC", "CD"], "빠른 모드는 짧아도 급경사인 길의 시간 부담을 반영합니다")
check(close(fastest?.totalCost, fastest?.estimatedDurationSeconds ?? -1), "빠른 모드 최적화 비용은 ETA와 일치합니다")
let slow = line.route(from: p, to: q, maxMeasuredGradePercent: 8.33, mode: .fastest, speedMetersPerSecond: 0.5)
let quick = line.route(from: p, to: q, maxMeasuredGradePercent: 8.33, mode: .fastest, speedMetersPerSecond: 1)
check(close(slow?.route.estimatedDurationSeconds, (quick?.route.estimatedDurationSeconds ?? -1) * 2),
      "평지 속도를 절반으로 줄이면 같은 경로 ETA가 두 배가 됩니다")
check((quick?.route.estimatedDurationSeconds ?? 0) > 80 && (quick?.route.estimatedDurationSeconds ?? .infinity).isFinite,
      "부분 경로 ETA는 실제 거리와 속도에 맞는 유한 시간입니다")
for speed in [0.0, -1, Double.nan, Double.infinity] {
    check(line.route(from: p, to: "B", maxMeasuredGradePercent: 8.33, mode: .fastest,
                     speedMetersPerSecond: speed) == nil, "유효하지 않은 속도를 거부합니다")
}

let oneWay = router([a, b], [edge("AB", a, b, reverseAllowed: false)])
check(oneWay.route(from: p, to: q, maxMeasuredGradePercent: 8.33, mode: .shortest) != nil,
      "일방통행 링크 부분구간 정방향은 허용합니다")
check(oneWay.route(from: q, to: p, maxMeasuredGradePercent: 8.33, mode: .shortest) == nil,
      "일방통행 링크 부분구간 역방향은 금지합니다")
check(oneWay.route(from: b.coordinate, to: "A", maxMeasuredGradePercent: 8.33, mode: .shortest) == nil,
      "일방통행 링크 전체 역방향도 금지합니다")
check(close(line.route(from: q, to: p, maxMeasuredGradePercent: 8.33, mode: .shortest)?.route.distanceMeters,
            p.distance(to: q), tolerance: 0.03), "명시적 양방향 링크 역방향 부분구간은 허용합니다")
let legacyJSON = """
{"id":"AB","from":"A","to":"B","length":180,"coordinates":[{"latitude":37,"longitude":127},{"latitude":37,"longitude":127.002}],"gradePercent":0,"gradeEvidence":"fieldChecked","obstacle":"none"}
"""
let decodedEdge = try JSONDecoder().decode(WalkEdge.self, from: Data(legacyJSON.utf8))
check(decodedEdge.reverseAllowed == nil && !decodedEdge.isUsable(maxMeasuredGradePercent: 8.33, forward: false),
      "역방향 필드가 없는 파일은 역방향 확인을 추정하지 않습니다")
let defaultEdge = WalkEdge(id: "default", from: "A", to: "B", length: 180,
                          coordinates: [a.coordinate, b.coordinate], gradePercent: 0,
                          gradeEvidence: .fieldChecked, obstacle: .none)
check(defaultEdge.reverseAllowed == nil, "직접 생성과 JSON 누락의 역방향 기본값이 같습니다")
let directedSlope = edge("AB", a, b, length: 100, grade: nil, uphill: 10, downhill: 1)
let directedRouter = router([a, b], [directedSlope])
check(directedRouter.route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33, mode: .shortest) == nil,
      "확인된 방향별 최대 오르막도 경사 한도를 검사합니다")
check(directedRouter.route(from: b.coordinate, to: "A", maxMeasuredGradePercent: 8.33, mode: .shortest) == nil,
      "같은 최대 기준을 역방향 내리막에도 검사합니다")
let uphillRoute = directedRouter.route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 12,
                                      mode: .fastest)
let downhillRoute = directedRouter.route(from: b.coordinate, to: "A", maxMeasuredGradePercent: 12,
                                        mode: .fastest)
check((uphillRoute?.estimatedDurationSeconds ?? 0) > (downhillRoute?.estimatedDurationSeconds ?? .infinity),
      "역방향은 오르막/내리막 데이터를 교환하여 ETA를 계산합니다")
let official = router([a, b], [edge("AB", a, b, grade: 3, evidence: .official)])
check(official.route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33,
                     mode: .shortest)?.unknownEdgeCount == 0, "구간별 공식 확인 경사 수치를 인정합니다")
let estimatedSteep = edge("AB", a, b, length: 100, grade: 40, evidence: .estimated)
check(estimatedSteep.estimatedDuration(for: 100, speedMetersPerSecond: 1) > 100 &&
      estimatedSteep.confirmedGrades(forward: true) == nil,
      "추정 경사는 시간 비용에 사용하되 확인된 경사로 표시하지 않습니다")
let standardEscalator = router([a, b], [edge("AB", a, b, obstacle: .escalator)])
check(standardEscalator.route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33,
                            mode: .shortest, requiresStepFree: false) != nil,
      "일반 보행에서 에스컬레이터를 명시적으로 허용합니다")
for obstacle in [WalkObstacle.impassable, .unverifiedOverpass] {
    check(router([a, b], [edge("AB", a, b, obstacle: obstacle)]).route(from: a.coordinate,
        to: "B", maxMeasuredGradePercent: 8.33, mode: .shortest, requiresStepFree: false) == nil,
        "폐쇄 및 미확인 고가도로는 일반 보행에서도 제외합니다")
}
let junction = router([a, b, c], [edge("bad", a, b, grade: nil, evidence: .unknown), edge("good", a, c)])
check(junction.route(from: a.coordinate, to: "C", maxMeasuredGradePercent: 8.33,
                     mode: .shortest)?.edgeIDs == ["good"], "교차점에서는 모든 합당한 연결 후보를 검사합니다")
let curved = WalkEdge(id: "curve", from: "A", to: "D", length: 400,
    coordinates: [a.coordinate, c.coordinate, d.coordinate], gradePercent: 0,
    gradeEvidence: .fieldChecked, obstacle: .none, reverseAllowed: true)
let curvePoint = GeoPoint(latitude: 37.0005, longitude: 127)
let curveEnd = GeoPoint(latitude: 37.001, longitude: 127.001)
let curveRoute = router([a, d], [curved]).route(from: curvePoint, to: curveEnd,
    maxMeasuredGradePercent: 8.33, mode: .shortest)
check(curveRoute?.route.coordinates.contains(c.coordinate) == true,
      "굽은 링크 부분구간은 중간 굴절점을 보존합니다")
check(close(curveRoute?.route.distanceMeters, 200, tolerance: 0.02),
      "부분 거리 비율은 전체 폴리라인 길이에 따라 계산합니다")

let crossingA = node("crossingA", 36.99, 127)
let crossingB = node("crossingB", 37.01, 127)
let crossingRoute = router([crossingA, crossingB], [edge("crossing", crossingA, crossingB)])
    .route(from: GeoPoint(latitude: 37, longitude: 127),
           to: GeoPoint(latitude: 37.0001, longitude: 127),
           maxMeasuredGradePercent: 8.33, mode: .shortest, maxSnapMeters: 1,
           maxDestinationSnapMeters: 1)
check(close(crossingRoute?.route.distanceMeters, 11.12, tolerance: 0.02),
      "공간 사전 필터는 끝점이 멀어도 출발지·도착지를 지나는 긴 링크를 보존합니다")
let dateLineA = node("dateLineA", 0, 179.999), dateLineB = node("dateLineB", 0, -179.999)
let dateLineRoute = router([dateLineA, dateLineB], [edge("dateLine", dateLineA, dateLineB)])
    .route(from: GeoPoint(latitude: 0, longitude: 180), to: dateLineB.coordinate,
           maxMeasuredGradePercent: 8.33, mode: .shortest)
check(close(dateLineRoute?.route.distanceMeters, 111.195, tolerance: 0.02),
      "공간 사전 필터와 부분 링크는 날짜 변경선을 넘는 그래프에서도 작동합니다")

// Both metric-consistent networks and inconsistent supplied lengths must match Dijkstra.
var gridNodes: [WalkNode] = []
for row in 0..<5 {
    for column in 0..<5 { gridNodes.append(node("\(row)-\(column)", 35 + Double(row) * 0.001, -120 + Double(column) * 0.001)) }
}
var gridEdges: [WalkEdge] = []
for row in 0..<5 {
    for column in 0..<5 {
        let first = gridNodes[row * 5 + column]
        for (nextRow, nextColumn) in [(row + 1, column), (row, column + 1)] where nextRow < 5 && nextColumn < 5 {
            let last = gridNodes[nextRow * 5 + nextColumn]
            gridEdges.append(edge("\(first.id)-\(last.id)", first, last,
                length: first.coordinate.distance(to: last.coordinate) * 1.05,
                grade: Double((row + column) % 4) * 2))
        }
    }
}
let grid = router(gridNodes, gridEdges)
for mode in [RouteMode.shortest, .gentle, .fastest] {
    let gridA = grid.route(from: gridNodes[0].coordinate, to: "4-4", maxMeasuredGradePercent: 12.5, mode: mode)
    let gridD = grid.route(from: gridNodes[0].coordinate, to: "4-4", maxMeasuredGradePercent: 12.5,
                           mode: mode, useHeuristic: false)
    check(close(gridA?.totalCost, gridD?.totalCost ?? -1, tolerance: 0.000001),
          "A*와 Dijkstra의 모드별 최적 비용이 같습니다")
    let start = safety.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 12.5, mode: mode)
    let reference = safety.route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 12.5,
                                 mode: mode, useHeuristic: false)
    check(close(start?.totalCost, reference?.totalCost ?? -1, tolerance: 0.000001),
          "metric보다 짧은 외부 거리 자료에서도 조정한 하한으로 최적 비용이 같습니다")
    let partialA = grid.route(from: GeoPoint(latitude: 35, longitude: -119.9996),
        to: GeoPoint(latitude: 35.004, longitude: -119.9964), maxMeasuredGradePercent: 12.5, mode: mode)
    let partialD = grid.route(from: GeoPoint(latitude: 35, longitude: -119.9996),
        to: GeoPoint(latitude: 35.004, longitude: -119.9964), maxMeasuredGradePercent: 12.5,
        mode: mode, useHeuristic: false)
    check(close(partialA?.route.totalCost, partialD?.route.totalCost ?? -1, tolerance: 0.000001),
          "출발/도착 부분구간에서도 A* 최적 비용이 같습니다")
}
check(router([a, a, b], [edge("AB", a, b)]).route(from: a.coordinate, to: "B",
    maxMeasuredGradePercent: 8.33, mode: .shortest) == nil, "중복 노드가 있는 잘못된 그래프를 거부합니다")
check(router([a, b], [edge("AB", a, b, length: Double.nan)]).route(from: a.coordinate, to: "B",
    maxMeasuredGradePercent: 8.33, mode: .shortest) == nil, "비유한 링크 길이를 거부합니다")
check(router([a, b], [edge("AB", a, b, grade: Double.infinity)]).route(from: a.coordinate, to: "B",
    maxMeasuredGradePercent: 8.33, mode: .shortest) == nil, "비유한 확인 경사를 거부합니다")
let offsetA = node("offsetA", 0, 0), offsetB = node("offsetB", 0, 0.001), offsetD = node("offsetD", 0, 0.002)
let offsetFirst = GeoPoint(latitude: 0, longitude: 0.00100008)
let offsetLast = GeoPoint(latitude: 0, longitude: 0.00199992)
let offsetBD = WalkEdge(id: "offsetBD", from: offsetB.id, to: offsetD.id,
    length: offsetFirst.distance(to: offsetLast), coordinates: [offsetFirst, offsetLast],
    gradePercent: 0, gradeEvidence: .fieldChecked, obstacle: .none, reverseAllowed: true)
let offsetGraph = router([offsetA, offsetB, offsetD], [edge("offsetAB", offsetA, offsetB, length: 200), offsetBD,
    edge("offsetAD", offsetA, offsetD, length: 200 + offsetBD.length + 0.008)])
let offsetASTAR = offsetGraph.route(from: offsetA.coordinate, to: offsetD.id, maxMeasuredGradePercent: 8.33, mode: .shortest)
let offsetDijkstra = offsetGraph.route(from: offsetA.coordinate, to: offsetD.id, maxMeasuredGradePercent: 8.33,
                                      mode: .shortest, useHeuristic: false)
check(close(offsetASTAR?.totalCost, offsetDijkstra?.totalCost ?? -1, tolerance: 0.000001),
      "노드/geometry의 허용 미세 오차까지 포함해 heuristic 하한을 검사합니다")

let repeatedAB = WalkEdge(id: "repeatedAB", from: a.id, to: b.id, length: 100,
    coordinates: [a.coordinate, b.coordinate, b.coordinate], gradePercent: 0,
    gradeEvidence: .fieldChecked, obstacle: .none, reverseAllowed: true)
let repeatedTurn = router([a, b, d], [repeatedAB, edge("BD", b, d, length: 110)])
    .route(from: a.coordinate, to: "D", maxMeasuredGradePercent: 8.33, mode: .fastest)
check(repeatedTurn?.instructions.map(\.action) == ["출발", "좌회전"],
      "중복 geometry 좌표 뒤의 회전도 안내합니다")
check(close(repeatedTurn?.instructions.reduce(0) { $0 + $1.distanceMeters }, 210),
      "상세 안내의 거리 합계는 공식 링크 길이로 계산한 경로 거리와 같습니다")
let escalatorRoute = router([a, b], [edge("escalator", a, b, obstacle: .escalator)])
    .route(from: a.coordinate, to: "B", maxMeasuredGradePercent: 8.33,
           mode: .shortest, requiresStepFree: false)
check(escalatorRoute?.instructions.first?.usesEscalator == true,
      "일반 보행 상세 안내에 에스컬레이터를 표시합니다")

let emptyGuide = try JSONDecoder().decode(StationGuide.self, from: Data("{\"source\":\"KRIC\",\"reviewedAt\":null,\"routes\":[]}".utf8))
check(emptyGuide.routes.isEmpty, "공식 자료가 없으면 실내 안내가 비어 있어야 합니다")
let guideJSON = """
{"source":"KRIC","reviewedAt":"2026-09-25","routes":[{"id":"guil","title":"구일 방면","entranceNodeID":"D","officialRouteID":"7","steps":[{"order":1,"text":"엘리베이터를 이용합니다","from":"출입구","to":"승강장"}]}]}
"""
let decodedGuide = try JSONDecoder().decode(StationGuide.self, from: Data(guideJSON.utf8))
check(decodedGuide.routes.first?.steps.first?.toName == "승강장", "철도 변환 파일의 단계 형식을 읽어야 합니다")
// Optional live-data check: only general walking is expected until surveyed grades exist.
if CommandLine.arguments.count > 1 {
    let graph = try JSONDecoder().decode(WalkGraph.self,
        from: Data(contentsOf: URL(fileURLWithPath: CommandLine.arguments[1])))
    let allowedNodes = Set(graph.edges.filter { $0.obstacle == .none }.flatMap { [$0.from, $0.to] })
    let connectedNodes = graph.nodes.filter { allowedNodes.contains($0.id) }
    let cityRouter = WheelchairRouter(graph: graph)
    let preview = cityRouter.route(
        from: GeoPoint(latitude: 37.5665, longitude: 126.978),
        to: GeoPoint(latitude: 37.5703, longitude: 126.983),
        maxMeasuredGradePercent: 8.33, mode: .fastest,
        allowUnverifiedGrade: true)?.route
    if let preview {
        check(preview.unknownEdgeCount > 0 && preview.segments.allSatisfy {
            $0.obstacle == .none && ($0.uphillPercent ?? 0) <= 8.33 && ($0.downhillPercent ?? 0) <= 8.33
        }, "실제 서울 보행망 후보에서 계단과 추정 한도 초과 경사를 제외합니다")
        print("시청-종각 미확인 후보: \(Int(preview.distanceMeters.rounded()))m, ETA \(Int(ceil(preview.estimatedDurationSeconds / 60)))min, 시작 연결 \(Int(preview.snapDistanceMeters.rounded()))m")
    } else {
        print("시청-종각: 휠체어 경사 한도 내 연결 후보 없음")
    }
    for (label, first, last) in [
        ("시청-종각", GeoPoint(latitude: 37.5665, longitude: 126.978), GeoPoint(latitude: 37.5703, longitude: 126.983)),
        ("홍대-합정", GeoPoint(latitude: 37.557, longitude: 126.9237), GeoPoint(latitude: 37.5496, longitude: 126.9138)),
        ("강남-역삼", GeoPoint(latitude: 37.4982, longitude: 127.0276), GeoPoint(latitude: 37.5006, longitude: 127.0365)),
        ("서울숲-건대", GeoPoint(latitude: 37.5445, longitude: 127.037), GeoPoint(latitude: 37.5404, longitude: 127.0693)),
    ] {
        let began = Date()
        let found = cityRouter.route(from: first, to: last,
            maxMeasuredGradePercent: 8.33, mode: .fastest, requiresStepFree: false)?.route
        let reference = cityRouter.route(from: first, to: last,
            maxMeasuredGradePercent: 8.33, mode: .fastest, requiresStepFree: false, useHeuristic: false)?.route
        check(found != nil, "실제 서울 보행망의 \(label) 노드를 연결합니다")
        check(close(found?.totalCost, reference?.totalCost ?? -1, tolerance: 0.000001),
              "실제 \(label) A*와 Dijkstra 비용이 같습니다")
        if let found {
            check(close(found.instructions.reduce(0) { $0 + $1.distanceMeters }, found.distanceMeters),
                  "실제 \(label) 상세 안내의 거리 합계가 같습니다")
            print("\(label): \(Int(found.distanceMeters.rounded()))m, ETA \(Int(ceil(found.estimatedDurationSeconds / 60)))min, A*/Dijkstra check \(String(format: "%.2f", Date().timeIntervalSince(began)))s")
        }
    }
    let first = connectedNodes.first!
    check(cityRouter.route(from: first.coordinate, to: first.id,
        maxMeasuredGradePercent: 8.33, mode: .fastest) != nil, "이미 도착한 위치는 이동 링크가 필요 없습니다")
    let unchecked = graph.edges.first { $0.gradeEvidence == .unknown && $0.obstacle == .none }!
    let nodeByID = Dictionary(uniqueKeysWithValues: graph.nodes.map { ($0.id, $0) })
    check(cityRouter.route(from: nodeByID[unchecked.from]!.coordinate, to: unchecked.to,
        maxMeasuredGradePercent: 8.33, mode: .fastest) == nil,
        "실제 미확인 경사 보행망을 휠체어 경로로 허용하지 않습니다")
    print("Live network: \(graph.nodes.count) nodes, \(graph.edges.count) links")
}
if failures > 0 { exit(1) }
print("Core route checks passed")
