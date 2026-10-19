import Foundation

struct WheelchairRouter {
    private let graph: WalkGraph
    private let nodes: [String: WalkNode]
    private let graphIsValid: Bool
    private struct EdgeReference {
        let index: Int
        let forward: Bool
    }
    private let outgoingEdges: [String: [EdgeReference]]
    private let edgeLatitudeRanges: [ClosedRange<Double>]
    private let minimumDistanceRatio: Double

    init(graph: WalkGraph) {
        self.graph = graph
        var lookup: [String: WalkNode] = [:]
        var valid = true
        for node in graph.nodes {
            if Task.isCancelled { valid = false; break }
            if node.id.isEmpty || !node.coordinate.isValid || lookup[node.id] != nil { valid = false }
            lookup[node.id] = node
        }
        var edgeIDs = Set<String>()
        var outgoing: [String: [EdgeReference]] = [:]
        var latitudeRanges: [ClosedRange<Double>] = []
        var distanceRatio = 1.0
        for (index, edge) in graph.edges.enumerated() {
            if Task.isCancelled { valid = false; break }
            let lineLength = Self.lineLength(edge.coordinates)
            guard !edge.id.isEmpty, edgeIDs.insert(edge.id).inserted,
                  let from = lookup[edge.from], let to = lookup[edge.to],
                  edge.length.isFinite, edge.length > 0, edge.coordinates.count >= 2,
                  edge.coordinates.allSatisfy(\.isValid),
                  edge.coordinates.first!.distance(to: from.coordinate) < 0.01,
                  edge.coordinates.last!.distance(to: to.coordinate) < 0.01,
                  lineLength > 0, lineLength.isFinite,
                  edge.gradePercent.map({ $0.isFinite }) ?? true,
                  edge.forwardMaxUphillPercent.map({ $0.isFinite && $0 >= 0 }) ?? true,
                  edge.forwardMaxDownhillPercent.map({ $0.isFinite && $0 >= 0 }) ?? true,
                  edge.hasValidElevationProfile
            else { valid = false; continue }
            outgoing[edge.from, default: []].append(EdgeReference(index: index, forward: true))
            if edge.reverseAllowed == true {
                outgoing[edge.to, default: []].append(EdgeReference(index: index, forward: false))
            }
            let latitudes = edge.coordinates.map(\.latitude)
            latitudeRanges.append(latitudes.min()!...latitudes.max()!)
            let chord = max(edge.coordinates.first!.distance(to: edge.coordinates.last!),
                            from.coordinate.distance(to: to.coordinate))
            if chord > 0 { distanceRatio = min(distanceRatio, edge.length / chord) }
        }
        self.nodes = lookup
        self.graphIsValid = valid
        self.outgoingEdges = outgoing
        self.edgeLatitudeRanges = latitudeRanges
        self.minimumDistanceRatio = distanceRatio
    }

    func route(
        from start: GeoPoint,
        to destinationNodeID: String,
        maxMeasuredGradePercent: Double,
        mode: RouteMode,
        requiresStepFree: Bool = true,
        allowUnverifiedGrade: Bool = false,
        maxSnapMeters: Double = 35,
        speedMetersPerSecond: Double = 1,
        useHeuristic: Bool = true
    ) -> WalkRoute? {
        guard let destination = nodes[destinationNodeID] else { return nil }
        let target = Attachment(nodes: [destinationNodeID], edges: [], gap: 0,
                                goalPoints: [destination.coordinate], goalRadius: 0)
        return search(from: start, target: target, maxMeasuredGradePercent: maxMeasuredGradePercent,
                      mode: mode, requiresStepFree: requiresStepFree,
                      allowUnverifiedGrade: allowUnverifiedGrade, maxSnapMeters: maxSnapMeters,
                      speedMetersPerSecond: speedMetersPerSecond, useHeuristic: useHeuristic)?.route
    }

    func route(
        from start: GeoPoint,
        to destination: GeoPoint,
        maxMeasuredGradePercent: Double,
        mode: RouteMode,
        requiresStepFree: Bool = true,
        allowUnverifiedGrade: Bool = false,
        maxSnapMeters: Double = 35,
        maxDestinationSnapMeters: Double = 35,
        speedMetersPerSecond: Double = 1,
        useHeuristic: Bool = true
    ) -> (route: WalkRoute, destinationGapMeters: Double)? {
        guard graphIsValid,
              let target = attachment(at: destination, maxSnapMeters: maxDestinationSnapMeters,
                                      includeNearbyAlternatives: !requiresStepFree || allowUnverifiedGrade) else { return nil }
        return search(from: start, target: target, maxMeasuredGradePercent: maxMeasuredGradePercent,
                      mode: mode, requiresStepFree: requiresStepFree,
                      allowUnverifiedGrade: allowUnverifiedGrade, maxSnapMeters: maxSnapMeters,
                      speedMetersPerSecond: speedMetersPerSecond, useHeuristic: useHeuristic)
    }

    private enum Vertex: Hashable { case start, node(String), destination }
    private struct Arc {
        let to: Vertex
        let edge: WalkEdge?
        let distance: Double
        let points: [GeoPoint]
        let cost: Double
        let duration: Double
        let forward: Bool
        var startGap: Double = 0
        var destinationGap: Double = 0
    }
    private struct Attachment {
        let nodes: [String]
        let edges: [(WalkEdge, Projection)]
        let gap: Double
        let goalPoints: [GeoPoint]
        let goalRadius: Double
    }

    private func search(
        from start: GeoPoint, target: Attachment, maxMeasuredGradePercent: Double,
        mode: RouteMode, requiresStepFree: Bool, allowUnverifiedGrade: Bool, maxSnapMeters: Double,
        speedMetersPerSecond: Double, useHeuristic: Bool
    ) -> (route: WalkRoute, destinationGapMeters: Double)? {
        guard graphIsValid, maxMeasuredGradePercent.isFinite, maxMeasuredGradePercent >= 0,
              speedMetersPerSecond.isFinite, speedMetersPerSecond > 0,
              let origin = attachment(at: start, maxSnapMeters: maxSnapMeters,
                                      includeNearbyAlternatives: allowUnverifiedGrade) else { return nil }

        var adjacency: [Vertex: [Arc]] = [:]
        func usable(_ edge: WalkEdge, forward: Bool) -> Bool {
            edge.isUsable(maxMeasuredGradePercent: maxMeasuredGradePercent,
                          requiresStepFree: requiresStepFree,
                          allowUnverifiedGrade: allowUnverifiedGrade, forward: forward)
        }
        func arc(to: Vertex, edge: WalkEdge, distance: Double, points: [GeoPoint], forward: Bool,
                 startGap: Double = 0, destinationGap: Double = 0) -> Arc {
            Arc(to: to, edge: edge, distance: distance, points: points,
                cost: edge.cost(for: distance, mode: mode, speedMetersPerSecond: speedMetersPerSecond, forward: forward),
                duration: edge.estimatedDuration(for: distance, speedMetersPerSecond: speedMetersPerSecond, forward: forward),
                forward: forward,
                startGap: startGap, destinationGap: destinationGap)
        }
        // Validate prices without allocating every city's arc and reversed geometry.
        // The immutable topology is prepared once; only expanded nodes materialize arcs.
        for edge in graph.edges {
            guard !Task.isCancelled else { return nil }
            for forward in [true, false] where usable(edge, forward: forward) {
                let cost = edge.cost(for: edge.length, mode: mode,
                                    speedMetersPerSecond: speedMetersPerSecond, forward: forward)
                let duration = edge.estimatedDuration(for: edge.length,
                                    speedMetersPerSecond: speedMetersPerSecond, forward: forward)
                guard cost.isFinite, cost >= 0, duration.isFinite, duration >= 0 else { return nil }
            }
        }
        func outgoing(_ vertex: Vertex) -> [Arc] {
            var result = adjacency[vertex] ?? []
            if case let .node(id) = vertex {
                for reference in outgoingEdges[id, default: []] {
                    let edge = graph.edges[reference.index]
                    guard usable(edge, forward: reference.forward) else { continue }
                    result.append(arc(to: .node(reference.forward ? edge.to : edge.from), edge: edge,
                        distance: edge.length,
                        points: reference.forward ? edge.coordinates : edge.coordinates.reversed(),
                        forward: reference.forward))
                }
            }
            return result
        }
        // A coordinate at a junction can enter every incident usable link through that node.
        for id in origin.nodes {
            adjacency[.start, default: []].append(Arc(to: .node(id), edge: nil, distance: 0,
                points: [nodes[id]!.coordinate], cost: 0, duration: 0, forward: true, startGap: origin.gap))
        }
        for (edge, projection) in origin.edges {
            if usable(edge, forward: true) {
                adjacency[.start, default: []].append(arc(to: .node(edge.to), edge: edge,
                    distance: edge.length * (1 - projection.fraction), points: projection.toEnd,
                    forward: true, startGap: projection.distance))
            }
            if usable(edge, forward: false) {
                adjacency[.start, default: []].append(arc(to: .node(edge.from), edge: edge,
                    distance: edge.length * projection.fraction, points: projection.toStart,
                    forward: false, startGap: projection.distance))
            }
        }
        for id in target.nodes {
            adjacency[.node(id), default: []].append(Arc(to: .destination, edge: nil, distance: 0,
                points: [nodes[id]!.coordinate], cost: 0, duration: 0, forward: true, destinationGap: target.gap))
        }
        for (edge, projection) in target.edges {
            if usable(edge, forward: true) {
                adjacency[.node(edge.from), default: []].append(arc(to: .destination, edge: edge,
                    distance: edge.length * projection.fraction, points: projection.toStart.reversed(),
                    forward: true, destinationGap: projection.distance))
            }
            if usable(edge, forward: false) {
                adjacency[.node(edge.to), default: []].append(arc(to: .destination, edge: edge,
                    distance: edge.length * (1 - projection.fraction), points: projection.toEnd.reversed(),
                    forward: false, destinationGap: projection.distance))
            }
        }
        // Connect two interior positions directly, preserving the link's travel direction.
        for (edge, first) in origin.edges {
            for (lastEdge, last) in target.edges where lastEdge.id == edge.id {
                let forward = last.fraction >= first.fraction
                guard usable(edge, forward: forward) else { continue }
                let points = forward ? Self.slice(edge.coordinates, from: first, to: last)
                                     : Self.slice(edge.coordinates, from: last, to: first).reversed()
                adjacency[.start, default: []].append(arc(to: .destination, edge: edge,
                    distance: edge.length * abs(last.fraction - first.fraction), points: Array(points),
                    forward: forward, startGap: first.distance, destinationGap: last.distance))
            }
        }

        let arcs = adjacency.values.flatMap { $0 }
        guard arcs.allSatisfy({ $0.cost.isFinite && $0.cost >= 0 && $0.duration.isFinite && $0.duration >= 0
            && $0.distance.isFinite && $0.distance >= 0 }) else { return nil }
        var lowerBound = minimumDistanceRatio * (mode == .fastest ? 1 / speedMetersPerSecond : 1.0)
        // Scale the metric lower bound to the cheapest actual arc, including partial links
        // and tiny node/geometry discrepancies. A zero bound safely becomes Dijkstra.
        for (vertex, outgoing) in adjacency {
            guard !Task.isCancelled else { return nil }
            for arc in outgoing {
                guard let first = arc.points.first, let last = arc.points.last else {
                    lowerBound = 0
                    continue
                }
                let from: GeoPoint
                let to: GeoPoint
                if case let .node(id) = vertex { from = nodes[id]!.coordinate } else { from = first }
                if case let .node(id) = arc.to { to = nodes[id]!.coordinate } else { to = last }
                let chord = max(first.distance(to: last), from.distance(to: to))
                if chord > 0 { lowerBound = min(lowerBound, arc.cost / chord) }
            }
        }
        lowerBound = useHeuristic && lowerBound.isFinite ? max(0, lowerBound * (1 - 1e-12)) : 0
        func heuristic(_ vertex: Vertex) -> Double {
            guard case let .node(id) = vertex else { return 0 }
            return max(0, (target.goalPoints.map { nodes[id]!.coordinate.distance(to: $0) }.min() ?? 0)
                       - target.goalRadius) * lowerBound
        }

        var best: [Vertex: Double] = [.start: 0]
        var previous: [Vertex: (Vertex, Arc)] = [:]
        var queue = MinQueue<Vertex>()
        queue.push(.start, g: 0, f: 0)
        while let item = queue.pop() {
            guard !Task.isCancelled else { return nil }
            // Stale entries compare g, never the heuristic-bearing queue priority f.
            guard item.g == best[item.vertex] else { continue }
            if item.vertex == .destination { break }
            for arc in outgoing(item.vertex) {
                let g = item.g + arc.cost
                let f = g + heuristic(arc.to)
                guard g.isFinite, f.isFinite else { return nil }
                if g < (best[arc.to] ?? .infinity) {
                    best[arc.to] = g
                    previous[arc.to] = (item.vertex, arc)
                    queue.push(arc.to, g: g, f: f)
                }
            }
        }
        guard let totalCost = best[.destination] else { return nil }
        var path: [Arc] = []
        var cursor = Vertex.destination
        while cursor != .start {
            guard let (parent, arc) = previous[cursor] else { return nil }
            path.append(arc)
            cursor = parent
        }
        path.reverse()
        var coordinates: [GeoPoint] = []
        var segments: [RouteSegment] = []
        for arc in path {
            for point in arc.points where point != coordinates.last { coordinates.append(point) }
            if let edge = arc.edge, arc.distance > 0 {
                let grades = edge.availableGrades(forward: arc.forward)
                let first = arc.points.first.flatMap { Self.project($0, onto: edge.coordinates)?.fraction }
                let last = arc.points.last.flatMap { Self.project($0, onto: edge.coordinates)?.fraction }
                let profile = first.flatMap { first in last.map { edge.profile(from: first, to: $0) } } ?? []
                segments.append(RouteSegment(edgeID: edge.id, coordinates: arc.points,
                    needsVerification: edge.needsVerification, obstacle: edge.obstacle,
                    distanceMeters: arc.distance, uphillPercent: grades?.up,
                    downhillPercent: grades?.down, gradeEvidence: edge.gradeEvidence,
                    elevationProfile: profile))
            }
        }
        let distance = path.reduce(0) { $0 + $1.distance }
        let duration = path.reduce(0) { $0 + $1.duration }
        guard distance.isFinite, duration.isFinite else { return nil }
        return (WalkRoute(edgeIDs: segments.map(\.edgeID), coordinates: coordinates, segments: segments,
                          distanceMeters: distance, totalCost: totalCost, estimatedDurationSeconds: duration,
                          unknownEdgeCount: segments.filter(\.needsVerification).count,
                          snapDistanceMeters: path.first?.startGap ?? 0),
                path.last?.destinationGap ?? 0)
    }

    private func attachment(at point: GeoPoint, maxSnapMeters: Double,
                            includeNearbyAlternatives: Bool = false) -> Attachment? {
        guard !Task.isCancelled, point.isValid, maxSnapMeters.isFinite, maxSnapMeters >= 0 else { return nil }
        // Great-circle distance is at least the meridional latitude distance.
        // This conservative prefilter works across longitude wrap and at the poles.
        let latitudeRadius = maxSnapMeters / 6_371_008.8 * 180 / .pi + 1e-10
        let latitudeRange = (point.latitude - latitudeRadius)...(point.latitude + latitudeRadius)
        let exact = graph.nodes.filter {
            latitudeRange.contains($0.latitude) && $0.coordinate.distance(to: point) <= min(0.01, maxSnapMeters)
        }
        if !exact.isEmpty && !includeNearbyAlternatives {
            return Attachment(nodes: exact.map(\.id), edges: [],
                gap: exact.map { $0.coordinate.distance(to: point) }.min()!,
                goalPoints: exact.map(\.coordinate), goalRadius: 0)
        }
        let projections = graph.edges.enumerated().compactMap { index, edge -> (WalkEdge, Projection)? in
            guard latitudeRange.overlaps(edgeLatitudeRanges[index]) else { return nil }
            guard let projection = Self.project(point, onto: edge.coordinates) else { return nil }
            return (edge, projection)
        }
        let nearest = projections.map { $0.1.distance }.min() ?? .infinity
        guard !exact.isEmpty || nearest <= maxSnapMeters else { return nil }
        // An origin stays on its nearest geometry. A destination may end at a farther reachable
        // sidewalk, with the unsurveyed remaining gap reported to the user.
        let candidates = projections.filter {
            $0.1.distance <= maxSnapMeters &&
            (includeNearbyAlternatives || $0.1.distance <= nearest + 0.01)
        }
        return Attachment(nodes: exact.map(\.id), edges: candidates,
                          gap: min(nearest, exact.map { $0.coordinate.distance(to: point) }.min() ?? .infinity),
                          goalPoints: includeNearbyAlternatives ? [point] : candidates.map { $0.1.point },
                          goalRadius: includeNearbyAlternatives ? maxSnapMeters : 0)
    }

    static func distance(from point: GeoPoint, to polyline: [GeoPoint]) -> Double {
        project(point, onto: polyline)?.distance ?? .infinity
    }

    private struct Projection {
        let distance: Double
        let fraction: Double
        let index: Int
        let point: GeoPoint
        let toStart: [GeoPoint]
        let toEnd: [GeoPoint]
    }
    private static func lineLength(_ line: [GeoPoint]) -> Double {
        zip(line, line.dropFirst()).reduce(0) { $0 + $1.0.distance(to: $1.1) }
    }
    private static func slice(_ line: [GeoPoint], from first: Projection, to last: Projection) -> [GeoPoint] {
        guard first.index < last.index else { return [first.point, last.point] }
        return [first.point] + Array(line[(first.index + 1)...last.index]) + [last.point]
    }
    private static func longitudeDelta(_ delta: Double) -> Double {
        let wrapped = delta.truncatingRemainder(dividingBy: 360)
        return wrapped > 180 ? wrapped - 360 : (wrapped < -180 ? wrapped + 360 : wrapped)
    }
    private static func project(_ point: GeoPoint, onto line: [GeoPoint]) -> Projection? {
        guard point.isValid, line.count >= 2, line.allSatisfy(\.isValid) else { return nil }
        let lengths = zip(line, line.dropFirst()).map { $0.distance(to: $1) }
        let total = lengths.reduce(0, +)
        guard total.isFinite, total > 0 else { return nil }
        // ponytail: local planar projection for street-sized segments; densify long survey links.
        let longitudeScale = cos(point.latitude * .pi / 180)
        var best: (distance: Double, index: Int, point: GeoPoint, walked: Double)?
        var walked = 0.0
        for index in lengths.indices {
            let a = line[index], b = line[index + 1]
            let longitude = longitudeDelta(b.longitude - a.longitude)
            let dx = longitude * longitudeScale, dy = b.latitude - a.latitude
            let denominator = dx * dx + dy * dy
            guard denominator > 0, denominator.isFinite else { walked += lengths[index]; continue }
            let px = longitudeDelta(point.longitude - a.longitude) * longitudeScale
            let py = point.latitude - a.latitude
            let part = min(1, max(0, (px * dx + py * dy) / denominator))
            let projected = GeoPoint(latitude: a.latitude + dy * part,
                                     longitude: longitudeDelta(a.longitude + longitude * part))
            let distance = point.distance(to: projected)
            if distance < (best?.distance ?? .infinity) {
                best = (distance, index, projected, walked + lengths[index] * part)
            }
            walked += lengths[index]
        }
        guard let best else { return nil }
        return Projection(distance: best.distance, fraction: best.walked / total,
                          index: best.index, point: best.point,
                          toStart: [best.point] + Array(line[0...best.index].reversed()),
                          toEnd: [best.point] + Array(line[(best.index + 1)...]))
    }
}

private struct MinQueue<Vertex> {
    struct Item {
        let vertex: Vertex
        let g: Double
        let f: Double
    }
    private var heap: [Item] = []
    mutating func push(_ vertex: Vertex, g: Double, f: Double) {
        heap.append(Item(vertex: vertex, g: g, f: f))
        var index = heap.count - 1
        while index > 0 {
            let parent = (index - 1) / 2
            if heap[parent].f <= heap[index].f { break }
            heap.swapAt(parent, index)
            index = parent
        }
    }
    mutating func pop() -> Item? {
        guard !heap.isEmpty else { return nil }
        if heap.count == 1 { return heap.removeLast() }
        let first = heap[0]
        heap[0] = heap.removeLast()
        var index = 0
        while true {
            let left = index * 2 + 1
            guard left < heap.count else { break }
            let right = left + 1
            let child = right < heap.count && heap[right].f < heap[left].f ? right : left
            if heap[index].f <= heap[child].f { break }
            heap.swapAt(index, child)
            index = child
        }
        return first
    }
}
