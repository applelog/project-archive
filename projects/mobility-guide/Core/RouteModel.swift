import Foundation

struct GeoPoint: Codable, Hashable {
    let latitude: Double
    let longitude: Double

    var isValid: Bool {
        latitude.isFinite && longitude.isFinite && (-90...90).contains(latitude) && (-180...180).contains(longitude)
    }

    func distance(to other: GeoPoint) -> Double {
        guard isValid, other.isValid else { return .infinity }
        let first = latitude * .pi / 180, second = other.latitude * .pi / 180
        let latitudeDelta = second - first
        let longitudeDelta = (other.longitude - longitude) * .pi / 180
        let haversine = pow(sin(latitudeDelta / 2), 2)
            + cos(first) * cos(second) * pow(sin(longitudeDelta / 2), 2)
        return 6_371_008.8 * 2 * asin(sqrt(min(1, max(0, haversine))))
    }
}

struct WalkNode: Codable {
    let id: String
    let latitude: Double
    let longitude: Double

    var coordinate: GeoPoint { GeoPoint(latitude: latitude, longitude: longitude) }
}

enum DataEvidence: String, Codable {
    case fieldChecked
    case official
    case estimated
    case unknown
}

enum WalkObstacle: String, Codable {
    case none
    case stairs
    case escalator
    case impassable
    case unverifiedOverpass
    case wheelchairRestricted
}

struct ElevationSample: Codable {
    let distanceMeters: Double
    let elevationMeters: Double

    init(distanceMeters: Double, elevationMeters: Double) {
        self.distanceMeters = distanceMeters
        self.elevationMeters = elevationMeters
    }

    init(from decoder: Decoder) throws {
        var pair = try decoder.unkeyedContainer()
        distanceMeters = try pair.decode(Double.self)
        elevationMeters = try pair.decode(Double.self)
        guard pair.isAtEnd else {
            throw DecodingError.dataCorruptedError(in: pair, debugDescription: "고도 표본은 거리와 높이 쌍이어야 합니다")
        }
    }

    func encode(to encoder: Encoder) throws {
        var pair = encoder.unkeyedContainer()
        try pair.encode(distanceMeters)
        try pair.encode(elevationMeters)
    }
}

struct WalkEdge: Codable {
    let id: String
    let from: String
    let to: String
    let length: Double
    let coordinates: [GeoPoint]
    let gradePercent: Double?
    let gradeEvidence: DataEvidence
    let obstacle: WalkObstacle
    let reverseAllowed: Bool?
    let forwardMaxUphillPercent: Double?
    let forwardMaxDownhillPercent: Double?
    let elevationProfile: [ElevationSample]?

    init(id: String, from: String, to: String, length: Double, coordinates: [GeoPoint],
         gradePercent: Double?, gradeEvidence: DataEvidence, obstacle: WalkObstacle,
         reverseAllowed: Bool? = nil, forwardMaxUphillPercent: Double? = nil,
         forwardMaxDownhillPercent: Double? = nil, elevationProfile: [ElevationSample]? = nil) {
        self.id = id
        self.from = from
        self.to = to
        self.length = length
        self.coordinates = coordinates
        self.gradePercent = gradePercent
        self.gradeEvidence = gradeEvidence
        self.obstacle = obstacle
        self.reverseAllowed = reverseAllowed
        self.forwardMaxUphillPercent = forwardMaxUphillPercent
        self.forwardMaxDownhillPercent = forwardMaxDownhillPercent
        self.elevationProfile = elevationProfile
    }

    // official means a measured value confirmed by the agency for this exact link;
    // values derived from elevation/contours must use estimated.
    var hasConfirmedGrade: Bool {
        (gradeEvidence == .fieldChecked || gradeEvidence == .official)
            && (gradePercent != nil || (forwardMaxUphillPercent != nil && forwardMaxDownhillPercent != nil))
    }
    var needsVerification: Bool { !hasConfirmedGrade }

    func isUsable(maxMeasuredGradePercent: Double, requiresStepFree: Bool = true,
                  allowUnverifiedGrade: Bool = false, forward: Bool = true) -> Bool {
        guard maxMeasuredGradePercent.isFinite, maxMeasuredGradePercent >= 0,
              length.isFinite, length > 0,
              gradePercent.map({ $0.isFinite }) ?? true,
              forwardMaxUphillPercent.map({ $0.isFinite && $0 >= 0 }) ?? true,
              forwardMaxDownhillPercent.map({ $0.isFinite && $0 >= 0 }) ?? true,
              forward || reverseAllowed == true,
              obstacle == .none || (!requiresStepFree &&
                  (obstacle == .stairs || obstacle == .escalator || obstacle == .wheelchairRestricted))
        else { return false }
        if !hasConfirmedGrade {
            if !requiresStepFree { return true }
            guard allowUnverifiedGrade else { return false }
            if gradeEvidence != .estimated { return true }
        }
        let grades = directionalGrades(forward: forward)
        return grades.up <= maxMeasuredGradePercent && grades.down <= maxMeasuredGradePercent
    }

    func confirmedGrades(forward: Bool) -> (up: Double, down: Double)? {
        hasConfirmedGrade ? directionalGrades(forward: forward) : nil
    }

    func availableGrades(forward: Bool) -> (up: Double, down: Double)? {
        guard hasConfirmedGrade || (gradeEvidence == .estimated &&
            (gradePercent != nil || (forwardMaxUphillPercent != nil && forwardMaxDownhillPercent != nil)))
        else { return nil }
        return directionalGrades(forward: forward)
    }

    func cost(for distance: Double, mode: RouteMode, speedMetersPerSecond: Double = 1,
              forward: Bool = true) -> Double {
        guard distance.isFinite, distance >= 0, speedMetersPerSecond.isFinite,
              speedMetersPerSecond > 0 else { return .infinity }
        switch mode {
        case .shortest: return distance
        case .gentle:
            let grades = directionalGrades(forward: forward)
            return distance * (1 + pow(max(grades.up, grades.down) / 10, 2))
        case .fastest:
            return estimatedDuration(for: distance, speedMetersPerSecond: speedMetersPerSecond, forward: forward)
        }
    }

    func estimatedDuration(for distance: Double, speedMetersPerSecond: Double, forward: Bool = true) -> Double {
        guard distance.isFinite, distance >= 0, speedMetersPerSecond.isFinite,
              speedMetersPerSecond > 0 else { return .infinity }
        let grades = directionalGrades(forward: forward)
        // Engineering estimates, not measured travel times: uphill 8%, downhill 12%
        // double level-ground time; stairs x2, escalators x1.25. Calibrate with observations.
        let slopeFactor = 1 + max(pow(grades.up / 8, 2), pow(grades.down / 12, 2))
        let obstacleFactor = obstacle == .stairs ? 2.0 : (obstacle == .escalator ? 1.25 : 1.0)
        return distance / speedMetersPerSecond * slopeFactor * obstacleFactor
    }

    private func directionalGrades(forward: Bool) -> (up: Double, down: Double) {
        guard hasConfirmedGrade || gradeEvidence == .estimated else { return (0, 0) }
        // Legacy signed grade is the maximum absolute grade in either direction.
        let legacy = abs(gradePercent ?? 0)
        let up = forwardMaxUphillPercent ?? legacy
        let down = forwardMaxDownhillPercent ?? legacy
        return forward ? (up, down) : (down, up)
    }

    var hasValidElevationProfile: Bool {
        guard let profile = elevationProfile else { return true }
        return profile.count >= 2 && profile.allSatisfy {
            $0.distanceMeters.isFinite && $0.elevationMeters.isFinite && $0.distanceMeters >= 0
        } && abs(profile[0].distanceMeters) < 0.001
            && abs(profile[profile.count - 1].distanceMeters - length) < 0.001
            && zip(profile, profile.dropFirst()).allSatisfy { $0.distanceMeters < $1.distanceMeters }
    }

    func profile(from startFraction: Double, to endFraction: Double) -> [ElevationSample] {
        guard let source = elevationProfile, hasValidElevationProfile else { return [] }
        let start = length * startFraction, end = length * endFraction
        guard start >= 0, end >= 0, start <= length, end <= length, start != end else { return [] }
        func height(at distance: Double) -> Double {
            for (a, b) in zip(source, source.dropFirst()) where distance <= b.distanceMeters {
                let part = (distance - a.distanceMeters) / (b.distanceMeters - a.distanceMeters)
                return a.elevationMeters + part * (b.elevationMeters - a.elevationMeters)
            }
            return source.last!.elevationMeters
        }
        var result = [ElevationSample(distanceMeters: start, elevationMeters: height(at: start))]
        result += source.filter { $0.distanceMeters > min(start, end) && $0.distanceMeters < max(start, end) }
        result.append(ElevationSample(distanceMeters: end, elevationMeters: height(at: end)))
        result.sort { start < end ? $0.distanceMeters < $1.distanceMeters : $0.distanceMeters > $1.distanceMeters }
        return result.map { ElevationSample(distanceMeters: abs($0.distanceMeters - start), elevationMeters: $0.elevationMeters) }
    }
}

struct StationEntrance: Codable {
    let id: String
    let name: String
    let nodeID: String
    let coordinate: GeoPoint
}

struct WalkGraph: Codable {
    var updatedAt: String? = nil
    var routingAssumptions: [String]? = nil
    var coverageBBox: [Double]? = nil
    var sourceLicense: String? = nil
    var sourceExtractDate: String? = nil
    var gradeSource: String? = nil
    var gradeSourceLicense: String? = nil
    let nodes: [WalkNode]
    let edges: [WalkEdge]
    let entrances: [StationEntrance]

    func covers(_ point: GeoPoint) -> Bool {
        guard let box = coverageBBox, box.count == 4 else { return true }
        return box[0] <= point.longitude && point.longitude <= box[2]
            && box[1] <= point.latitude && point.latitude <= box[3]
    }
}

enum RouteMode: Hashable {
    case shortest
    case gentle
    case fastest
}

struct WalkRoute {
    let edgeIDs: [String]
    let coordinates: [GeoPoint]
    let segments: [RouteSegment]
    let distanceMeters: Double
    let totalCost: Double
    let estimatedDurationSeconds: Double
    let unknownEdgeCount: Int
    let snapDistanceMeters: Double
}

struct RouteSegment {
    let edgeID: String
    let coordinates: [GeoPoint]
    let needsVerification: Bool
    let obstacle: WalkObstacle
    let distanceMeters: Double
    let uphillPercent: Double?
    let downhillPercent: Double?
    var gradeEvidence: DataEvidence = .unknown
    var elevationProfile: [ElevationSample] = []
}

struct RouteInstruction {
    let action: String
    let distanceMeters: Double
    let needsVerification: Bool
    let usesStairs: Bool
    let usesEscalator: Bool
}

extension WalkRoute {
    var instructions: [RouteInstruction] {
        var steps: [RouteInstruction] = []
        var previous: RouteSegment?
        for segment in segments {
            let distance = segment.distanceMeters
            guard distance > 0.1 else { continue }
            let action = previous.map { Self.turn(from: $0.coordinates, to: segment.coordinates) } ?? "출발"
            let stairs = segment.obstacle == .stairs
            let escalator = segment.obstacle == .escalator
            if action == "직진", let last = steps.last,
               last.needsVerification == segment.needsVerification, last.usesStairs == stairs,
               last.usesEscalator == escalator {
                steps[steps.count - 1] = RouteInstruction(action: last.action,
                    distanceMeters: last.distanceMeters + distance,
                    needsVerification: last.needsVerification, usesStairs: stairs, usesEscalator: escalator)
            } else {
                steps.append(RouteInstruction(action: action, distanceMeters: distance,
                    needsVerification: segment.needsVerification, usesStairs: stairs, usesEscalator: escalator))
            }
            previous = segment
        }
        return steps
    }

    private static func turn(from first: [GeoPoint], to second: [GeoPoint]) -> String {
        guard first.count >= 2, second.count >= 2 else { return "직진" }
        let b = first[first.count - 1], c = second[0]
        guard let a = first.dropLast().reversed().first(where: { $0.distance(to: b) > 0.01 }),
              let d = second.dropFirst().first(where: { $0.distance(to: c) > 0.01 }) else { return "직진" }
        let scale = cos(b.latitude * .pi / 180)
        let incoming = atan2((b.longitude - a.longitude) * scale, b.latitude - a.latitude)
        let outgoing = atan2((d.longitude - c.longitude) * scale, d.latitude - c.latitude)
        let angle = atan2(sin(outgoing - incoming), cos(outgoing - incoming)) * 180 / .pi
        if abs(angle) < 35 { return "직진" }
        if abs(angle) > 150 { return "방향 전환" }
        return angle > 0 ? "우회전" : "좌회전"
    }
}
