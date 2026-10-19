import CoreLocation
import MapKit
import SwiftUI

private enum GradePreference: String, CaseIterable, Identifiable {
    case conservative = "1:12 · 약 8.3%"
    case moderate = "1:10 · 10%"
    case flexible = "1:8 · 12.5%"

    var id: String { rawValue }
    var percent: Double {
        switch self {
        case .conservative: return 100.0 / 12.0
        case .moderate: return 10
        case .flexible: return 12.5
        }
    }
}

private enum MobilityProfile: String, CaseIterable, Identifiable {
    case wheelchair = "휠체어"
    case walkingAid = "보행 보조"
    case stepFree = "계단 회피"
    case standard = "일반 보행"

    var id: String { rawValue }
    var requiresStepFree: Bool { self != .standard }
}

private enum PanelPage {
    case home, search, pin, selected, route
}

private enum SearchTarget {
    case origin, destination
}

private struct SelectedPlace {
    let name: String
    let detail: String
    let coordinate: GeoPoint
}

private struct LocalRouteCandidate {
    let route: WalkRoute
    let destinationGapMeters: Double
    let mode: RouteMode
    let isUnverifiedPreview: Bool
}

private struct RouteLine {
    let needsVerification: Bool
    var coordinates: [GeoPoint]
}

private enum AppFiles {
    static func load<T: Decodable>(_ name: String, as type: T.Type) -> T? {
        guard let url = Bundle.main.url(forResource: name, withExtension: "json"),
              let data = try? Data(contentsOf: url) else { return nil }
        return try? JSONDecoder().decode(T.self, from: data)
    }
}

struct ContentView: View {
    @StateObject private var locationService = LocationService()
    @State private var graph = WalkGraph(nodes: [], edges: [], entrances: [])
    @State private var preparedRouter: WheelchairRouter?
    @State private var stationGuide = StationGuide(source: "", reviewedAt: nil, routes: [])
    @State private var isLoadingGraph = true
    @State private var dataMessage: String?
    @State private var confirmedGradeCount = 0
    @State private var estimatedGradeCount = 0
    @State private var camera: MapCameraPosition = .region(MKCoordinateRegion(
        center: CLLocationCoordinate2D(latitude: 37.5665, longitude: 126.978),
        span: MKCoordinateSpan(latitudeDelta: 0.06, longitudeDelta: 0.06)
    ))
    @State private var page: PanelPage = .home
    @State private var selectedPlace: SelectedPlace?
    @State private var pinPreview: SelectedPlace?
    @State private var pinReturnPage: PanelPage = .home
    @State private var selectionFromSearch = false
    @State private var searchTarget: SearchTarget = .destination
    @State private var searchReturnPage: PanelPage = .home
    @State private var searchQuery = ""
    @State private var searchResults: [MKMapItem] = []
    @State private var searchMessage: String?
    @State private var searchGeneration = 0
    @State private var isSearching = false
    @State private var activeSearch: MKLocalSearch?
    @FocusState private var searchFocused: Bool
    @State private var routeMessage: String?
    @State private var mobility: MobilityProfile = .wheelchair
    @State private var grade: GradePreference = .conservative
    @State private var speedMetersPerSecond = 1.0
    @State private var mode: RouteMode = .fastest
    @State private var startOverride: GeoPoint?
    @State private var startName = "지도에서 선택한 출발지"
    @State private var routeGeneration = 0
    @State private var routeTask: Task<Void, Never>?
    @State private var isRouting = false
    @State private var departureTime = Date()
    @State private var localRoutes: [LocalRouteCandidate] = []
    @State private var appleRoutes: [MKRoute] = []
    @State private var selectedLocalIndex = 0
    @State private var selectedAppleIndex: Int?
    @State private var activeDirections: MKDirections?
    @State private var activeTransitDirections: MKDirections?
    @State private var transitTask: Task<Void, Never>?
    @State private var transitETA: TimeInterval?
    @State private var isCheckingTransit = false
    @State private var selectedEntrance: StationEntrance?
    @State private var offRouteReadings = 0
    @State private var showingStationSteps = false
    @State private var stationRouteID = ""
    @State private var focusOnNextLocation = false
    @State private var showingPanel = true
    @State private var showingProfile = false
    @State private var panelDetent: PresentationDetent = .height(320)

    private let teal = Color(red: 0.02, green: 0.43, blue: 0.41)
    private let amber = Color(red: 0.63, green: 0.34, blue: 0.01)
    private let routeGreen = Color(uiColor: UIColor { traits in
        traits.userInterfaceStyle == .dark
            ? UIColor(red: 0.30, green: 0.83, blue: 0.53, alpha: 1)
            : UIColor(red: 0.04, green: 0.42, blue: 0.24, alpha: 1)
    })

    private var activeGuide: StationRoute? {
        guard stationGuide.reviewedAt != nil else { return nil }
        return availableStationRoutes.first { $0.id == stationRouteID } ?? availableStationRoutes.first
    }

    private var availableStationRoutes: [StationRoute] {
        stationGuide.routes.filter { $0.entranceNodeID == selectedEntrance?.nodeID && !$0.steps.isEmpty }
    }

    private var hasOutdoorData: Bool { !graph.edges.isEmpty }
    private var hasRoute: Bool { !localRoutes.isEmpty || !appleRoutes.isEmpty }
    private var selectedLocalRoute: LocalRouteCandidate? {
        guard selectedAppleIndex == nil, localRoutes.indices.contains(selectedLocalIndex) else { return nil }
        return localRoutes[selectedLocalIndex]
    }
    private var selectedAppleRoute: MKRoute? {
        guard let index = selectedAppleIndex, appleRoutes.indices.contains(index) else { return nil }
        return appleRoutes[index]
    }
    private var selectedRouteLines: [RouteLine] {
        var lines: [RouteLine] = []
        for segment in selectedLocalRoute?.route.segments ?? [] {
            if let last = lines.last, last.needsVerification == segment.needsVerification,
               last.coordinates.last == segment.coordinates.first {
                lines[lines.count - 1].coordinates.append(contentsOf: segment.coordinates.dropFirst())
            } else {
                lines.append(RouteLine(needsVerification: segment.needsVerification,
                                       coordinates: segment.coordinates))
            }
        }
        return lines
    }
    private var accurateLocation: CLLocation? { validLocation(locationService.location) }
    private var startPoint: GeoPoint? {
        startOverride ?? accurateLocation.map {
            GeoPoint(latitude: $0.coordinate.latitude, longitude: $0.coordinate.longitude)
        }
    }

    private func validLocation(_ candidate: CLLocation?) -> CLLocation? {
        guard let location = candidate,
              location.horizontalAccuracy >= 0,
              location.horizontalAccuracy <= 30,
              abs(location.timestamp.timeIntervalSinceNow) <= 15 else { return nil }
        return location
    }

    var body: some View {
        mapView
        .safeAreaInset(edge: .top, spacing: 0) {
            mapToolbar
                .padding(.horizontal, 16)
                .padding(.top, 8)
                .padding(.bottom, 12)
        }
        // Reserve the collapsed sheet's space when MapKit fits a route.
        .safeAreaInset(edge: .bottom, spacing: 0) {
            Color.clear.frame(height: 320).allowsHitTesting(false)
        }
        .tint(teal)
        .sheet(isPresented: $showingPanel) {
            panelContent
                .presentationDetents([.height(320), .medium, .large], selection: $panelDetent)
                .presentationDragIndicator(.visible)
                .presentationCornerRadius(28)
                .presentationBackground(.regularMaterial)
                .presentationBackgroundInteraction(.enabled(upThrough: .height(320)))
                .interactiveDismissDisabled()
        }
        .task {
            // Decode the bundled walking graph away from the UI thread.
            let loaded = await Task.detached(priority: .userInitiated) {
                let network = AppFiles.load("network", as: WalkGraph.self)
                return (network, AppFiles.load("station_steps", as: StationGuide.self),
                        network?.edges.filter(\.hasConfirmedGrade).count ?? 0,
                        network.map { WheelchairRouter(graph: $0) },
                        network?.edges.filter { $0.gradeEvidence == .estimated }.count ?? 0)
            }.value
            if let loadedGraph = loaded.0 { graph = loadedGraph }
            else { dataMessage = "보행망 파일을 읽지 못했습니다. 자료 형식을 확인해 주세요." }
            if let loadedGuide = loaded.1 { stationGuide = loadedGuide }
            confirmedGradeCount = loaded.2
            preparedRouter = loaded.3
            estimatedGradeCount = loaded.4
            isLoadingGraph = false
        }
        .onReceive(locationService.$location) { newLocation in
            guard startOverride == nil else { return }
            guard let newLocation = validLocation(newLocation) else {
                if isRouting || (hasRoute && selectedLocalRoute?.isUnverifiedPreview != true) {
                    clearRoute()
                    routeMessage = "현재 위치가 정확하지 않아 경로 안내를 멈췄습니다."
                }
                return
            }
            if focusOnNextLocation {
                focusMap(on: newLocation)
                focusOnNextLocation = false
            }
            guard page == .route, !isRouting, selectedLocalRoute?.isUnverifiedPreview != true else { return }
            let current = GeoPoint(latitude: newLocation.coordinate.latitude,
                                   longitude: newLocation.coordinate.longitude)
            let coordinates: [GeoPoint]
            if let local = selectedLocalRoute { coordinates = local.route.coordinates }
            else if let apple = selectedAppleRoute {
                let points = apple.polyline.points()
                coordinates = (0..<apple.polyline.pointCount).map {
                    GeoPoint(latitude: points[$0].coordinate.latitude, longitude: points[$0].coordinate.longitude)
                }
            } else { return }
            let distance = WheelchairRouter.distance(from: current, to: coordinates)
            if distance > max(25, newLocation.horizontalAccuracy * 2) {
                offRouteReadings += 1
                if offRouteReadings >= 2 {
                    recalculate(from: newLocation)
                    offRouteReadings = 0
                }
            } else {
                offRouteReadings = 0
            }
        }
        .onReceive(Timer.publish(every: 5, on: .main, in: .common).autoconnect()) { _ in
            if startOverride == nil &&
                (isRouting || (hasRoute && selectedLocalRoute?.isUnverifiedPreview != true)) &&
                accurateLocation == nil {
                clearRoute()
                routeMessage = "현재 위치가 정확하지 않아 경로 안내를 멈췄습니다."
            }
        }
        .onChange(of: mobility) { _, _ in if page == .route || isRouting { recalculate() } }
        .onChange(of: grade) { _, _ in if page == .route || isRouting { recalculate() } }
        .onChange(of: mode) { _, newMode in
            if (page == .route || isRouting) && selectedLocalRoute?.mode != newMode { recalculate() }
        }
        .onChange(of: speedMetersPerSecond) { _, _ in if page == .route || isRouting { recalculate() } }
        .onDisappear { clearRoute(); cancelSearch() }
    }

    private var mapView: some View {
        MapReader { proxy in
            Map(position: $camera) {
                UserAnnotation()
                if let start = startOverride {
                    Marker(startName, systemImage: "flag.fill", coordinate: CLLocationCoordinate2D(
                        latitude: start.latitude, longitude: start.longitude
                    ))
                    .tint(teal)
                }
                if let place = selectedPlace {
                    Marker(place.name, systemImage: "mappin", coordinate: CLLocationCoordinate2D(
                        latitude: place.coordinate.latitude, longitude: place.coordinate.longitude
                    ))
                    .tint(.red)
                }
                if let pin = pinPreview {
                    Marker("선택한 위치", systemImage: "mappin", coordinate: CLLocationCoordinate2D(
                        latitude: pin.coordinate.latitude, longitude: pin.coordinate.longitude
                    ))
                    .tint(amber)
                }
                if let entrance = selectedEntrance {
                    Marker(entrance.name, systemImage: "figure.roll", coordinate: CLLocationCoordinate2D(
                        latitude: entrance.coordinate.latitude, longitude: entrance.coordinate.longitude
                    ))
                    .tint(teal)
                }
                ForEach(Array(localRoutes.enumerated()), id: \.offset) { index, candidate in
                    if selectedAppleIndex != nil || index != selectedLocalIndex {
                        MapPolyline(coordinates: candidate.route.coordinates.map {
                            CLLocationCoordinate2D(latitude: $0.latitude, longitude: $0.longitude)
                        })
                        .stroke(.gray.opacity(0.7), lineWidth: 3)
                    }
                }
                ForEach(Array(appleRoutes.enumerated()), id: \.offset) { index, candidate in
                    if index != selectedAppleIndex {
                        MapPolyline(candidate).stroke(.gray.opacity(0.6), lineWidth: 3)
                    }
                }
                ForEach(Array(selectedRouteLines.enumerated()), id: \.offset) { _, line in
                        MapPolyline(coordinates: line.coordinates.map {
                            CLLocationCoordinate2D(latitude: $0.latitude, longitude: $0.longitude)
                        })
                        .stroke(.white.opacity(0.95), style: StrokeStyle(lineWidth: 7, lineCap: .round, lineJoin: .round))
                        MapPolyline(coordinates: line.coordinates.map {
                            CLLocationCoordinate2D(latitude: $0.latitude, longitude: $0.longitude)
                        })
                        .stroke(routeGreen,
                                style: StrokeStyle(lineWidth: 4,
                                                   lineCap: line.needsVerification ? .butt : .round,
                                                   lineJoin: .round,
                                                   dash: line.needsVerification ? [9, 5] : []))
                }
                if let apple = selectedAppleRoute {
                    MapPolyline(apple)
                        .stroke(.white.opacity(0.95), style: StrokeStyle(lineWidth: 7, lineCap: .round, lineJoin: .round))
                    MapPolyline(apple)
                        .stroke(routeGreen, style: StrokeStyle(lineWidth: 4, lineCap: .round, lineJoin: .round))
                }
            }
            .mapControls { MapCompass(); MapScaleView() }
            .onTapGesture { position in
                guard let coordinate = proxy.convert(position, from: .local) else { return }
                let point = GeoPoint(latitude: coordinate.latitude, longitude: coordinate.longitude)
                guard point.isValid else { return }
                pinPreview = SelectedPlace(name: "지도에서 선택한 위치", detail: "지도 핀", coordinate: point)
                if page != .pin { pinReturnPage = page }
                page = .pin
                panelDetent = .height(320)
            }
        }
        .ignoresSafeArea()
    }

    private var mapToolbar: some View {
        VStack(spacing: 10) {
            HStack(spacing: 10) {
                Text("이동권 길찾기")
                    .font(.headline.weight(.bold))
                    .padding(.horizontal, 16)
                    .frame(minHeight: 46)
                    .background(.regularMaterial, in: Capsule())
                Spacer()
                Button { showingProfile = true } label: {
                    Image(systemName: "person.crop.circle")
                        .font(.title3)
                        .frame(width: 46, height: 46)
                }
                .background(.regularMaterial, in: Circle())
                .accessibilityLabel("나의 이동 설정, 현재 \(mobility.rawValue)")
                Button(action: locateMe) {
                    Image(systemName: "location.fill")
                        .font(.headline)
                        .frame(width: 46, height: 46)
                }
                .background(.regularMaterial, in: Circle())
                .accessibilityLabel("내 위치로 지도 이동")
            }
            VStack(spacing: 0) {
                routeEndpointButton(title: "출발", value: startOverride == nil ? "내 위치" : startName,
                                    symbol: "circle.fill") {
                    openSearch(for: .origin)
                }
                Divider().padding(.leading, 50)
                routeEndpointButton(title: "도착", value: selectedPlace?.name ?? "도착지 검색",
                                    symbol: "mappin.circle.fill") {
                    openSearch(for: .destination)
                }
            }
            .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 18))
        }
    }

    private func routeEndpointButton(title: String, value: String, symbol: String,
                                     action: @escaping () -> Void) -> some View {
        Button(action: action) {
            HStack(spacing: 12) {
                Image(systemName: symbol)
                    .font(.subheadline)
                    .foregroundStyle(title == "출발" ? teal : .red)
                    .frame(width: 22)
                Text(title).font(.caption.weight(.semibold)).foregroundStyle(.secondary)
                Text(value).font(.subheadline.weight(.semibold))
                    .foregroundStyle(.primary).lineLimit(1)
                Spacer(minLength: 0)
                Image(systemName: "magnifyingglass").font(.caption).foregroundStyle(.secondary)
            }
            .padding(.horizontal, 14)
            .frame(minHeight: 48)
            .contentShape(Rectangle())
        }
        .buttonStyle(.plain)
        .accessibilityLabel("\(title): \(value), 변경")
    }

    private var panelContent: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 18) {
                switch page {
                case .home: homePage
                case .search: searchPage
                case .pin: pinPage
                case .selected: selectedPage
                case .route: routePage
                }
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(.horizontal, 20)
            .padding(.top, 28)
            .padding(.bottom, 36)
        }
        .scrollIndicators(.hidden)
        .sheet(isPresented: $showingStationSteps) {
            if let guide = activeGuide, selectedEntrance?.nodeID == guide.entranceNodeID {
                StationStepsView(route: guide, reviewedAt: stationGuide.reviewedAt)
            }
        }
        .sheet(isPresented: $showingProfile) { profileSheet }
    }

    private var homePage: some View {
        VStack(alignment: .leading, spacing: 18) {
            Text("어디로 가세요?").font(.title2.bold())
            Text("상단에서 출발지·도착지를 검색하거나 지도를 눌러 선택하세요.")
                .font(.subheadline).foregroundStyle(.secondary)
            dataStatus
        }
    }

    private var pinPage: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                backButton
                Spacer(minLength: 8)
                Text("지도에서 선택한 위치").font(.headline)
            }
            Text("출발지 또는 도착지로 지정하세요.")
                .font(.subheadline).foregroundStyle(.secondary)
            if let pinPreview {
                VStack(spacing: 10) {
                    Button { setStart(pinPreview) } label: {
                        Text("출발지로 설정").frame(maxWidth: .infinity, minHeight: 44)
                    }
                        .buttonStyle(.bordered)
                    Button { select(pinPreview, fromSearch: false) } label: {
                        Text("도착지로 설정").frame(maxWidth: .infinity, minHeight: 44)
                    }
                        .buttonStyle(.borderedProminent)
                }
                .controlSize(.large)
                .frame(maxWidth: .infinity)
            }
        }
    }

    private var searchPage: some View {
        VStack(alignment: .leading, spacing: 16) {
            backButton
            Text(searchTarget == .origin ? "출발지 검색" : "도착지 검색").font(.title2.bold())
            if searchTarget == .origin {
                Button("내 위치에서 출발", systemImage: "location.fill") {
                    clearRoute()
                    routeMessage = nil
                    startOverride = nil
                    cancelSearch()
                    page = selectedPlace == nil ? .home : .selected
                    panelDetent = .height(320)
                    locateMe()
                }
                .frame(minHeight: 44)
            }
            HStack(spacing: 10) {
                TextField("장소 이름 또는 주소", text: $searchQuery)
                    .textInputAutocapitalization(.never)
                    .submitLabel(.search)
                    .focused($searchFocused)
                    .onSubmit { Task { await searchPlaces() } }
                    .accessibilityLabel("장소 이름 또는 주소")
                Button("검색") { Task { await searchPlaces() } }
                    .disabled(isSearching)
                    .frame(minHeight: 44)
            }
            .padding(.leading, 16)
            .padding(.trailing, 8)
            .background(Color(uiColor: .secondarySystemBackground),
                        in: RoundedRectangle(cornerRadius: 16))
            Text("현재 지도 주변을 우선 검색합니다. 검색어는 Apple 지도 서비스로 전송됩니다.")
                .font(.footnote).foregroundStyle(.secondary)
            if isSearching { ProgressView("검색 중") }
            if let searchMessage {
                Text(searchMessage).font(.subheadline).foregroundStyle(.secondary)
            }
            ForEach(Array(searchResults.enumerated()), id: \.offset) { _, item in
                Button {
                    let point = GeoPoint(latitude: item.placemark.coordinate.latitude,
                                         longitude: item.placemark.coordinate.longitude)
                    let place = SelectedPlace(name: item.name ?? "검색한 장소",
                                              detail: item.placemark.title ?? "검색 결과",
                                              coordinate: point)
                    if searchTarget == .origin { setStart(place) }
                    else { select(place, fromSearch: true) }
                } label: {
                    HStack(spacing: 12) {
                        Image(systemName: "mappin.circle.fill").foregroundStyle(teal)
                        VStack(alignment: .leading, spacing: 3) {
                            Text(item.name ?? "검색한 장소").font(.headline)
                            Text(item.placemark.title ?? "")
                                .font(.subheadline).foregroundStyle(.secondary).lineLimit(2)
                        }
                        Spacer(minLength: 0)
                    }
                    .frame(maxWidth: .infinity, minHeight: 56, alignment: .leading)
                }
                .buttonStyle(.plain)
                Divider()
            }
        }
    }

    private var selectedPage: some View {
        VStack(alignment: .leading, spacing: 18) {
            backButton
            destinationHeader
            if !mobility.requiresStepFree {
                Text("경로 조회 시 출발지·목적지 좌표를 Apple 지도에 전송합니다. Apple 보행·대중교통 경로의 접근성은 미확인입니다.")
                    .font(.footnote).foregroundStyle(.secondary)
            }
            Button(action: findRoute) {
                VStack(spacing: 4) {
                    Label(isRouting ? "경로 계산 중" : "경로 찾기", systemImage: "arrow.triangle.turn.up.right.diamond.fill")
                        .font(.headline)
                    Text("\(mobility.rawValue) · \(routeModeName(mode))")
                        .font(.caption)
                }
                .frame(maxWidth: .infinity, minHeight: 54)
            }
            .buttonStyle(.borderedProminent)
            .disabled(isRouting || (mobility.requiresStepFree && isLoadingGraph))
            if isRouting { ProgressView("이동 조건에 맞는 경로를 찾고 있습니다") }
            if let routeMessage {
                Label(routeMessage, systemImage: "exclamationmark.circle")
                    .font(.subheadline)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(16)
                    .background(Color(uiColor: .secondarySystemBackground),
                                in: RoundedRectangle(cornerRadius: 16))
            } else if !isRouting {
                dataStatus
            }
            sourceNote
        }
    }

    private var routePage: some View {
        VStack(alignment: .leading, spacing: 18) {
            HStack {
                backButton
                Spacer(minLength: 8)
                if hasRoute {
                    Button("전체 경로", systemImage: "arrow.up.left.and.arrow.down.right") {
                        panelDetent = .height(320)
                        focusOnSelectedRoute()
                    }
                    .frame(minHeight: 44)
                }
            }
            if isRouting { ProgressView("경로를 다시 계산하고 있습니다") }
            if hasRoute {
                routeSummary
                routeOptions
                if let startPoint, let selectedPlace,
                   startPoint.distance(to: selectedPlace.coordinate) > 1_500 {
                    if mobility.requiresStepFree { stepFreeTransitNote }
                    else { transitSection }
                }
                if let local = selectedLocalRoute {
                    routeSection(local)
                    instructionSection(local)
                } else if let apple = selectedAppleRoute {
                    appleRouteSection(apple)
                }
            } else if let routeMessage {
                Label(routeMessage, systemImage: "exclamationmark.circle")
                    .font(.subheadline).foregroundStyle(amber)
            } else {
                dataStatus
            }
            if selectedEntrance != nil { stationSection }
            sourceNote
        }
    }

    @ViewBuilder private var routeSummary: some View {
        if let local = selectedLocalRoute {
            VStack(alignment: .leading, spacing: 6) {
                Text("\(timeLabel(local.route)) · 약 \(Int(local.route.distanceMeters.rounded()))m")
                    .font(.title2.bold())
                Text("예상 도착 \(departureTime.addingTimeInterval(local.route.estimatedDurationSeconds).formatted(date: .omitted, time: .shortened)) · \(mobility.rawValue)")
                    .font(.subheadline).foregroundStyle(.secondary)
                if local.isUnverifiedPreview {
                    Label("경사·접근성 미확인 · 참고용 경로", systemImage: "info.circle")
                        .font(.subheadline.weight(.semibold)).foregroundStyle(amber)
                }
            }
            .accessibilityElement(children: .combine)
        } else if let apple = selectedAppleRoute {
            VStack(alignment: .leading, spacing: 6) {
                Text("\(timeLabel(apple.expectedTravelTime)) · 약 \(Int(apple.distance.rounded()))m")
                    .font(.title2.bold())
                Text("예상 도착 \(departureTime.addingTimeInterval(apple.expectedTravelTime).formatted(date: .omitted, time: .shortened)) · Apple 지도 보행")
                    .font(.subheadline).foregroundStyle(.secondary)
                Label("계단·경사·출입구 접근성 미확인", systemImage: "exclamationmark.triangle")
                    .font(.subheadline).foregroundStyle(amber)
            }
            .accessibilityElement(children: .combine)
        }
    }

    private var backButton: some View {
        Button(action: goBack) {
            Label("뒤로 가기", systemImage: "chevron.left")
                .font(.subheadline.weight(.semibold))
                .frame(minHeight: 44)
        }
        .buttonStyle(.plain)
    }

    private var destinationHeader: some View {
        HStack(spacing: 14) {
            Image(systemName: "mappin")
                .font(.title2.weight(.semibold))
                .foregroundStyle(teal)
                .frame(width: 48, height: 48)
                .background(teal.opacity(0.12), in: RoundedRectangle(cornerRadius: 14))
            VStack(alignment: .leading, spacing: 3) {
                Text(selectedPlace?.name ?? "목적지").font(.title2.bold())
                Text(selectedPlace?.detail ?? "")
                    .font(.subheadline).foregroundStyle(.secondary).lineLimit(2)
            }
            Spacer(minLength: 0)
        }
        .accessibilityElement(children: .combine)
    }

    private var dataStatus: some View {
        VStack(alignment: .leading, spacing: 8) {
            if isLoadingGraph {
                ProgressView("보행망을 불러오고 있습니다")
            } else if let dataMessage {
                Label("보행 자료 확인 필요", systemImage: "map").font(.headline)
                Text(dataMessage)
            } else if let routeMessage {
                Label("경로 안내 중지", systemImage: "exclamationmark.circle").font(.headline)
                Text(routeMessage)
            } else if let message = locationService.message, startOverride == nil {
                Label("위치 확인 필요", systemImage: "location.slash").font(.headline)
                Text(message)
                Button("다시 확인", action: locateMe).buttonStyle(.bordered)
                    .frame(minHeight: 44)
            } else if !hasOutdoorData {
                Label("보행 경로 자료 준비 중", systemImage: "map").font(.headline)
                Text("장소를 검색하고 지도를 볼 수 있습니다. 검증된 보행망 자료가 없어 아직 접근 가능한 경로를 계산하지 않습니다.")
            } else if mobility.requiresStepFree && confirmedGradeCount == 0 {
                Label("경로를 미리 살펴볼 수 있어요", systemImage: "map").font(.headline)
                Text(estimatedGradeCount > 0
                     ? "계단·에스컬레이터와 추정 경사가 설정 한도를 넘는 길은 피합니다. 실제 보도 접근성은 미검증이라 참고용 경로로 표시합니다."
                     : "지도에 표시된 계단·에스컬레이터는 피합니다. 보도 경사와 턱은 미확인이라 참고용 경로로 표시합니다.")
                missingAccessDataExplanation
            } else if startPoint == nil {
                Label("현재 위치 확인 필요", systemImage: "location.slash").font(.headline)
                Text("현재 위치를 확인하거나 목적지를 선택한 뒤 지도에서 출발지를 지정하세요.")
                Button("내 위치 확인", action: locateMe).buttonStyle(.bordered)
                    .frame(minHeight: 44)
            } else {
                Label("경로를 찾을 준비가 됐습니다", systemImage: "point.topleft.down.curvedto.point.bottomright.up")
                    .font(.headline)
                Text("경로 찾기를 누르면 선택한 이동 설정으로 계산합니다.")
            }
        }
        .font(.subheadline)
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(16)
        .background(Color(uiColor: .secondarySystemBackground),
                    in: RoundedRectangle(cornerRadius: 18))
    }

    private var routeOptions: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("경로 선택").font(.headline)
            ForEach(Array(localRoutes.enumerated()), id: \.offset) { index, candidate in
                Button {
                    selectedAppleIndex = nil
                    selectedLocalIndex = index
                    mode = candidate.mode
                    selectedEntrance = graph.entrances.first { entrance in
                        candidate.route.coordinates.last.map { $0.distance(to: entrance.coordinate) < 0.01 } ?? false
                    }
                    stationRouteID = availableStationRoutes.first?.id ?? ""
                    panelDetent = .height(320)
                    focusOnSelectedRoute()
                } label: {
                    routeOptionLabel(title: routeModeName(candidate.mode),
                        detail: "\(timeLabel(candidate.route.estimatedDurationSeconds)) · \(Int(candidate.route.distanceMeters.rounded()))m · \(candidate.isUnverifiedPreview ? "접근성 미확인" : "확인된 경사 기준")",
                        selected: selectedAppleIndex == nil && selectedLocalIndex == index)
                }
                .buttonStyle(.plain)
            }
            ForEach(Array(appleRoutes.enumerated()), id: \.offset) { index, candidate in
                Button {
                    selectedAppleIndex = index
                    selectedEntrance = nil
                    panelDetent = .height(320)
                    focusOnSelectedRoute()
                } label: {
                    routeOptionLabel(title: "Apple 지도 · 보행 \(index + 1)",
                        detail: "\(timeLabel(candidate.expectedTravelTime)) · \(Int(candidate.distance.rounded()))m · 접근성 미확인",
                        selected: selectedAppleIndex == index)
                }
                .buttonStyle(.plain)
            }
            if localRoutes.isEmpty {
                Text("자체 보행망에서 연결된 후보를 찾지 못했습니다.")
                    .font(.footnote).foregroundStyle(.secondary)
            }
            if appleRoutes.isEmpty && !mobility.requiresStepFree {
                Text("Apple 지도 대안은 현재 제공되지 않습니다.")
                    .font(.footnote).foregroundStyle(.secondary)
            }
        }
    }

    private func routeOptionLabel(title: String, detail: String, selected: Bool) -> some View {
        HStack(spacing: 12) {
            Image(systemName: selected ? "checkmark.circle.fill" : "circle")
                .foregroundStyle(selected ? routeGreen : .secondary)
            VStack(alignment: .leading, spacing: 4) {
                Text(title).font(.subheadline.weight(.semibold)).foregroundStyle(.primary)
                Text(detail).font(.footnote).foregroundStyle(.secondary)
            }
            Spacer(minLength: 0)
        }
        .frame(maxWidth: .infinity, minHeight: 56, alignment: .leading)
        .padding(.horizontal, 14)
        .background(selected ? routeGreen.opacity(0.08) : Color(uiColor: .secondarySystemBackground),
                    in: RoundedRectangle(cornerRadius: 16))
        .accessibilityElement(children: .combine)
        .accessibilityAddTraits(selected ? .isSelected : [])
    }

    private func routeSection(_ candidate: LocalRouteCandidate) -> some View {
        let route = candidate.route
        return VStack(alignment: .leading, spacing: 12) {
            Text("경로 확인 사항").font(.headline)
            if candidate.isUnverifiedPreview {
                Text("실제 보도의 경사·턱·폭을 확인한 경로가 아닙니다. 안전 경로 추천과 안내 시작은 제공하지 않습니다.")
                    .font(.subheadline).foregroundStyle(.secondary)
            }
            if route.snapDistanceMeters > 5 {
                Text("출발지와 보행로 사이 약 \(Int(route.snapDistanceMeters.rounded()))m는 연결 상태가 미확인입니다.")
                    .font(.subheadline).foregroundStyle(.secondary)
            }
            if candidate.destinationGapMeters > 5 {
                Text("보행망 끝에서 목적지까지 약 \(Int(candidate.destinationGapMeters.rounded()))m는 연결 상태가 확인되지 않았습니다.")
                    .font(.subheadline).foregroundStyle(.secondary)
            }
            if route.unknownEdgeCount > 0 {
                Label("초록 점선: 실측 경사가 미검증인 \(route.unknownEdgeCount)개 구간", systemImage: "line.diagonal")
                    .font(.footnote).foregroundStyle(.secondary)
            }
            slopeSection(route)
            routeModePicker
            Text("설정한 평지 속도와 확인·추정 경사로 계산한 예상시간입니다. 경사가 없는 구간은 평지 속도로 계산하며, 신호·대기 시간과 보행망 밖 연결 구간은 제외합니다.")
                .font(.footnote).foregroundStyle(.secondary)
            Text("턱·공사·엘리베이터 운행 여부는 현장에서 확인해 주세요.")
                .font(.footnote).foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private func slopeSection(_ route: WalkRoute) -> some View {
        let confirmedMeters = route.segments.filter { !$0.needsVerification }
            .reduce(0) { $0 + $1.distanceMeters }
        let estimatedMeters = route.segments.filter { $0.gradeEvidence == .estimated }
            .reduce(0) { $0 + $1.distanceMeters }
        let unknownMeters = route.segments.filter { $0.uphillPercent == nil || $0.downhillPercent == nil }
            .reduce(0) { $0 + $1.distanceMeters }
        let confirmed = route.segments.filter { !$0.needsVerification }
        let estimated = route.segments.filter { $0.gradeEvidence == .estimated }
        return VStack(alignment: .leading, spacing: 8) {
            Text("경사 정보").font(.headline)
            elevationSection(route)
            Canvas { context, size in
                var offset = 0.0
                let total = max(1, route.segments.reduce(0) { $0 + $1.distanceMeters })
                for segment in route.segments {
                    let width = size.width * segment.distanceMeters / total
                    let grade = max(segment.uphillPercent ?? 0, segment.downhillPercent ?? 0)
                    let color: Color = segment.uphillPercent == nil || segment.downhillPercent == nil ? .secondary.opacity(0.3)
                        : grade > 8 ? .red : (grade > 5 ? .orange : routeGreen)
                    context.fill(Path(CGRect(x: offset, y: 0, width: width, height: size.height)),
                                 with: .color(color))
                    offset += width
                }
            }
            .frame(height: 6)
            .clipShape(Capsule())
            .accessibilityLabel("실측 확인 \(Int(confirmedMeters.rounded()))미터, 지형 추정 \(Int(estimatedMeters.rounded()))미터, 경사 자료 없음 \(Int(unknownMeters.rounded()))미터")
            HStack {
                Text("출발")
                Spacer()
                Text("도착")
            }
            .font(.caption).foregroundStyle(.secondary)
            Text("실측 확인 \(Int(confirmedMeters.rounded()))m · 지형 추정 \(Int(estimatedMeters.rounded()))m · 자료 없음 \(Int(unknownMeters.rounded()))m")
                .font(.subheadline)
            if let up = confirmed.compactMap(\.uphillPercent).max(),
               let down = confirmed.compactMap(\.downhillPercent).max() {
                Text("실측 최대 오르막 \(up, specifier: "%.1f")% · 내리막 \(down, specifier: "%.1f")%")
                    .font(.subheadline)
            }
            if let up = estimated.compactMap(\.uphillPercent).max(),
               let down = estimated.compactMap(\.downhillPercent).max() {
                Text("추정 최대 오르막 \(up, specifier: "%.1f")% · 내리막 \(down, specifier: "%.1f")%")
                    .font(.subheadline)
                Text("지형 높이에서 계산한 참고값입니다. 작은 턱·짧은 경사로·보도 횡경사는 포함하지 않습니다.")
                    .font(.footnote).foregroundStyle(.secondary)
            }
            if confirmedMeters + estimatedMeters == 0 {
                Text("이 경로의 실제 보도 경사는 아직 확인되지 않았어요.")
                    .font(.footnote).foregroundStyle(.secondary)
            }
            if confirmedMeters + estimatedMeters > 0 {
                Text("경사율: 초록 5% 이하 · 주황 5~8% · 빨강 8% 초과 · 회색 자료 없음. 색은 접근성 검증을 뜻하지 않습니다.")
                    .font(.caption).foregroundStyle(.secondary)
            }
            missingAccessDataExplanation
        }
        .padding(14)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color(uiColor: .secondarySystemBackground),
                    in: RoundedRectangle(cornerRadius: 16))
    }

    private func elevationSection(_ route: WalkRoute) -> some View {
        let heights = route.segments.flatMap { $0.elevationProfile.map(\.elevationMeters) }
        let low = heights.min() ?? 0
        let high = heights.max() ?? 0
        let span = max(10, high - low)
        return Group {
            if !heights.isEmpty {
                Text("길의 높낮이 · 지형 추정").font(.subheadline.weight(.medium))
                Canvas { context, size in
                    let base = size.height - 8
                    let usableHeight = size.height - 16
                    let total = max(1, route.distanceMeters)
                    for fraction in [0.0, 0.5, 1.0] {
                        var grid = Path()
                        let y = 8 + usableHeight * fraction
                        grid.move(to: CGPoint(x: 0, y: y))
                        grid.addLine(to: CGPoint(x: size.width, y: y))
                        context.stroke(grid, with: .color(.secondary.opacity(0.15)), lineWidth: 1)
                    }
                    var offset = 0.0
                    for segment in route.segments {
                        var line = Path()
                        for (index, sample) in segment.elevationProfile.enumerated() {
                            let point = CGPoint(x: size.width * (offset + sample.distanceMeters) / total,
                                                y: base - usableHeight * (sample.elevationMeters - low) / span)
                            if index == 0 { line.move(to: point) } else { line.addLine(to: point) }
                        }
                        context.stroke(line, with: .color(routeGreen),
                                       style: StrokeStyle(lineWidth: 2.5, lineCap: .round, lineJoin: .round))
                        offset += segment.distanceMeters
                    }
                }
                .frame(height: 110)
                .accessibilityLabel("지형 추정 고도 단면. 최저 \(Int(low.rounded()))미터, 최고 \(Int(high.rounded()))미터. 경사 자료가 없는 구간은 선을 표시하지 않습니다.")
                Text("추정 고도 \(low, specifier: "%.1f")~\(high, specifier: "%.1f")m · 끊긴 구간은 자료 없음")
                    .font(.caption).foregroundStyle(.secondary)
            }
        }
    }

    private var missingAccessDataExplanation: some View {
        DisclosureGroup("왜 경사가 미확인인가요?") {
            Text(graph.gradeSource != nil
                 ? "서울 표고점·등고선에서 계산한 지형 경사를 길에 연결했습니다. 지형 추정과 경사 자료가 없는 구간을 구분하지만, 실제 보도의 경사·턱·폭을 측정한 자료는 아닙니다. 따라서 휠체어 통행 가능 여부를 확정하지 않습니다. 추가 자료 제출 없이 경로 검색은 가능합니다."
                 : "현재 경로는 OpenStreetMap의 길 연결과 시설 표기로 계산합니다. 실제 보도의 경사·턱·폭을 측정한 자료는 없어 통행 가능 여부를 확정하지 않습니다. 추가 자료 제출 없이 경로 검색은 가능합니다.")
                .font(.footnote).foregroundStyle(.secondary)
                .padding(.top, 6)
        }
        .font(.footnote)
        .frame(minHeight: 44)
    }

    private var transitSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Label("대중교통 비교", systemImage: "tram.fill").font(.headline)
            if isCheckingTransit { ProgressView("대중교통 시간을 확인하고 있습니다") }
            else if let transitETA {
                let walkETA = ([selectedLocalRoute?.route.estimatedDurationSeconds,
                                selectedAppleRoute?.expectedTravelTime].compactMap { $0 }).min()
                Text("Apple 지도 예상 \(timeLabel(transitETA))\(walkETA.map { transitETA + 180 < $0 ? " · 보행보다 빠를 수 있음" : "" } ?? "")")
                    .font(.subheadline.weight(.semibold))
            } else {
                Text("앱에서 대중교통 예상 시간을 확인하지 못했습니다.")
                    .font(.subheadline).foregroundStyle(.secondary)
            }
            Text("지하철 포함 여부, 환승·역 출입구 접근성은 Apple 지도에서 확인하세요.")
                .font(.footnote).foregroundStyle(.secondary)
            Button("Apple 지도에서 대중교통 경로 보기", systemImage: "arrow.up.right.square") {
                openTransitInMaps()
            }
            .frame(minHeight: 44)
        }
        .padding(14)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color(uiColor: .secondarySystemBackground),
                    in: RoundedRectangle(cornerRadius: 16))
    }

    private var stepFreeTransitNote: some View {
        VStack(alignment: .leading, spacing: 8) {
            Label("지하철 이동은 확인 중", systemImage: "tram.fill").font(.headline)
            Text("역 출입구·승강장까지의 엘리베이터 동선과 운행 상태를 확인하지 못해 휠체어 경로로 추천할 수 없습니다.")
                .font(.subheadline).foregroundStyle(.secondary)
        }
        .padding(14)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color(uiColor: .secondarySystemBackground),
                    in: RoundedRectangle(cornerRadius: 16))
    }

    private func timeLabel(_ route: WalkRoute) -> String {
        timeLabel(route.estimatedDurationSeconds)
    }

    private func timeLabel(_ seconds: TimeInterval) -> String {
        let minutes = Int(ceil(seconds / 60))
        if minutes < 1 { return "1분 미만" }
        return minutes < 60 ? "약 \(minutes)분" : "약 \(minutes / 60)시간 \(minutes % 60)분"
    }

    private func routeModeName(_ value: RouteMode) -> String {
        switch value {
        case .fastest: return "빠르게"
        case .shortest: return "짧게"
        case .gentle: return "경사 부담"
        }
    }

    private var routeModePicker: some View {
        Picker("경로 우선순위", selection: $mode) {
            Text("빠르게").tag(RouteMode.fastest)
            Text("짧게").tag(RouteMode.shortest)
            Text("경사 부담").tag(RouteMode.gentle)
        }
        .pickerStyle(.segmented)
    }

    private func instructionSection(_ candidate: LocalRouteCandidate) -> some View {
        let route = candidate.route
        return VStack(alignment: .leading, spacing: 14) {
            Divider()
            Text("이동 경로 상세").font(.headline)
            Text("보행망의 선형으로 계산한 방향과 거리입니다. 도로명과 횡단보도 위치는 확인되지 않았습니다.")
                .font(.footnote).foregroundStyle(.secondary)
            ForEach(Array(route.instructions.enumerated()), id: \.offset) { index, step in
                HStack(alignment: .top, spacing: 14) {
                    Text("\(index + 1)")
                        .font(.headline)
                        .foregroundStyle(.white)
                        .frame(width: 32, height: 32)
                        .background(teal, in: Circle())
                    VStack(alignment: .leading, spacing: 4) {
                        Text("\(step.action) · 약 \(Int(step.distanceMeters.rounded()))m 이동")
                            .font(.body.weight(.semibold))
                        if step.usesStairs {
                            Text("계단 포함").font(.subheadline).foregroundStyle(amber)
                        }
                        if step.usesEscalator {
                            Text("에스컬레이터 포함").font(.subheadline).foregroundStyle(amber)
                        }
                        if step.needsVerification {
                            Text("경사 미확인").font(.subheadline).foregroundStyle(amber)
                        }
                    }
                    Spacer(minLength: 0)
                }
                .frame(maxWidth: .infinity, minHeight: 52, alignment: .leading)
                .accessibilityElement(children: .combine)
            }
            Label(candidate.destinationGapMeters > 5 ? "보행망 끝에 도착 · 목적지까지 연결 상태 확인 필요" : "목적지 위치에 도착", systemImage: "mappin")
                .font(.subheadline.weight(.medium))
        }
    }

    private func appleRouteSection(_ route: MKRoute) -> some View {
        VStack(alignment: .leading, spacing: 14) {
            Text("Apple 지도 보행 경로").font(.headline)
            Text("\(timeLabel(route.expectedTravelTime)) · 약 \(Int(route.distance.rounded()))m")
                .font(.title3.weight(.semibold))
            Text("예상 도착 \(departureTime.addingTimeInterval(route.expectedTravelTime).formatted(date: .omitted, time: .shortened))")
                .font(.subheadline).foregroundStyle(.secondary)
            Label("계단·경사·턱·건물 출입구의 접근성은 확인되지 않았습니다. 목적지 핀은 출입구가 아닐 수 있습니다.",
                  systemImage: "exclamationmark.triangle")
                .font(.subheadline).foregroundStyle(amber)
            Divider()
            Text("이동 경로 상세").font(.headline)
            ForEach(Array(route.steps.filter { !$0.instructions.isEmpty }.enumerated()), id: \.offset) { index, step in
                HStack(alignment: .top, spacing: 14) {
                    Text("\(index + 1)").font(.headline).foregroundStyle(.white)
                        .frame(width: 32, height: 32).background(.blue, in: Circle())
                    VStack(alignment: .leading, spacing: 4) {
                        Text(step.instructions).font(.body.weight(.semibold))
                        if step.distance > 0 {
                            Text("약 \(Int(step.distance.rounded()))m")
                                .font(.subheadline).foregroundStyle(.secondary)
                        }
                    }
                    Spacer(minLength: 0)
                }
                .frame(maxWidth: .infinity, minHeight: 52, alignment: .leading)
                .accessibilityElement(children: .combine)
            }
            Text("안내와 시간은 Apple 지도에서 제공합니다. 개인 속도 설정은 이 시간에 반영되지 않으며 실제 통행 가능 여부를 확인해야 합니다.")
                .font(.footnote).foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var settingsSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("나의 이동 설정").font(.headline)
            Picker("이동 방식", selection: $mobility) {
                ForEach(MobilityProfile.allCases) { Text($0.rawValue).tag($0) }
            }
            .pickerStyle(.menu)
            Picker("개인 최대 경사", selection: $grade) {
                ForEach(GradePreference.allCases) { Text($0.rawValue).tag($0) }
            }
            .pickerStyle(.menu)
            Picker("평지 이동 속도", selection: $speedMetersPerSecond) {
                ForEach([0.5, 0.8, 1.0, 1.2, 1.5], id: \.self) { speed in
                    Text("\(speed, specifier: "%.1f")m/s").tag(speed)
                }
            }
            .pickerStyle(.menu)
            routeModePicker
            Text(mobility.requiresStepFree
                 ? "계단·에스컬레이터와 확인·추정 경사가 한도를 넘는 길은 제외합니다. 실측 경사가 검증되지 않은 길은 참고 후보로만 표시합니다."
                 : "계단·에스컬레이터를 허용합니다. 미확인 경사를 포함할 수 있어 시간과 경사 부담은 정확하지 않을 수 있습니다.")
                .font(.footnote).foregroundStyle(.secondary)
            Text("이동 설정은 이 기기의 앱 메모리에만 보관하며 계정에 전송하지 않습니다.")
                .font(.footnote).foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var profileSheet: some View {
        NavigationStack {
            ScrollView {
                settingsSection
                    .padding(20)
            }
            .navigationTitle("나의 이동 설정")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar { ToolbarItem(placement: .topBarTrailing) {
                Button("완료") { showingProfile = false }
            } }
        }
        .presentationDetents([.medium, .large])
    }

    private var stationSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Divider()
            Text("역내 이동 안내").font(.headline)
            if availableStationRoutes.count > 1 {
                Picker("역내 목적지", selection: $stationRouteID) {
                    ForEach(availableStationRoutes) { Text($0.title).tag($0.id) }
                }
            }
            if let guide = activeGuide, selectedEntrance?.nodeID == guide.entranceNodeID {
                Text("공식 출입구→승강장 동선 \(guide.steps.count)단계")
                    .font(.subheadline)
                Button("역내 단계 안내 보기", systemImage: "arrow.right") {
                    showingStationSteps = true
                }
                .buttonStyle(.borderedProminent)
                .frame(minHeight: 44)
            } else {
                Text("이 출입구의 공식 역내 동선이 확인되지 않았습니다. 승강장까지는 안내하지 않습니다.")
                    .font(.subheadline).foregroundStyle(.secondary)
            }
            Text("엘리베이터 운행 상태는 실시간으로 검증되지 않았습니다.")
                .font(.footnote).foregroundStyle(amber)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var sourceNote: some View {
        VStack(alignment: .leading, spacing: 4) {
            if graph.sourceLicense != nil {
                Text("보행 연결: OpenStreetMap 한국 원자료 \(graph.sourceExtractDate ?? "기준일 미확인") · 변환일 \(graph.updatedAt ?? "미확인")")
                Link("© OpenStreetMap contributors · ODbL 1.0",
                     destination: URL(string: "https://www.openstreetmap.org/copyright")!)
            } else {
                Text("자료: 서울특별시 보행 네트워크·역 엘리베이터 위치, 국가철도공단 역내 동선")
                Text("실외 자료 변환일 \(graph.updatedAt ?? "미확인") · 역내 확인일 \(stationGuide.reviewedAt ?? "미확인")")
            }
            if let assumptions = graph.routingAssumptions {
                ForEach(assumptions, id: \.self) { Text($0) }
            }
            if let license = graph.gradeSourceLicense {
                Text("지형 경사: \(license)")
                Link("서울시 경사도·등고선 자료 출처",
                     destination: URL(string: "https://data.seoul.go.kr/dataList/OA-22241/F/1/datasetView.do")!)
            }
        }
        .font(.caption)
        .foregroundStyle(.secondary)
    }

    private func goBack() {
        cancelSearch()
        switch page {
        case .home: break
        case .search:
            page = searchReturnPage
            panelDetent = .height(320)
        case .pin:
            pinPreview = nil
            page = pinReturnPage
            panelDetent = .height(320)
        case .selected:
            clearRoute()
            selectedPlace = nil
            routeMessage = nil
            page = selectionFromSearch ? .search : .home
            panelDetent = selectionFromSearch ? .large : .height(320)
        case .route:
            clearRoute()
            routeMessage = nil
            page = .selected
            panelDetent = .medium
        }
    }

    private func select(_ place: SelectedPlace, fromSearch: Bool) {
        guard place.coordinate.isValid else { return }
        cancelSearch()
        pinPreview = nil
        clearRoute()
        routeMessage = nil
        selectedPlace = place
        selectionFromSearch = fromSearch
        page = .selected
        panelDetent = .height(320)
        camera = .region(MKCoordinateRegion(
            center: CLLocationCoordinate2D(latitude: place.coordinate.latitude,
                                           longitude: place.coordinate.longitude),
            span: MKCoordinateSpan(latitudeDelta: 0.006, longitudeDelta: 0.006)
        ))
    }

    private func setStart(_ place: SelectedPlace) {
        guard place.coordinate.isValid else { return }
        cancelSearch()
        pinPreview = nil
        clearRoute()
        routeMessage = nil
        startOverride = place.coordinate
        startName = place.name
        page = selectedPlace == nil ? .home : .selected
        panelDetent = .height(320)
        camera = .region(MKCoordinateRegion(
            center: CLLocationCoordinate2D(latitude: place.coordinate.latitude,
                                           longitude: place.coordinate.longitude),
            span: MKCoordinateSpan(latitudeDelta: 0.006, longitudeDelta: 0.006)
        ))
    }

    private func openSearch(for target: SearchTarget) {
        cancelSearch()
        searchTarget = target
        searchReturnPage = page
        searchQuery = ""
        searchResults = []
        searchMessage = nil
        page = .search
        panelDetent = .large
        searchFocused = true
    }

    @MainActor private func searchPlaces() async {
        guard !isSearching else { return }
        let query = searchQuery.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !query.isEmpty, query.count <= 80 else {
            searchMessage = "검색어를 1~80자로 입력해 주세요."
            return
        }
        searchFocused = false
        searchGeneration += 1
        let generation = searchGeneration
        isSearching = true
        searchMessage = nil
        searchResults = []
        defer {
            if generation == searchGeneration {
                isSearching = false
                activeSearch = nil
            }
        }
        let request = MKLocalSearch.Request()
        request.naturalLanguageQuery = query
        if let visibleRegion = camera.region { request.region = visibleRegion }
        let search = MKLocalSearch(request: request)
        activeSearch = search
        do {
            let response = try await search.start()
            guard generation == searchGeneration, page == .search else { return }
            searchResults = Array(response.mapItems.filter {
                GeoPoint(latitude: $0.placemark.coordinate.latitude,
                         longitude: $0.placemark.coordinate.longitude).isValid
            }.prefix(12))
            if searchResults.isEmpty { searchMessage = "검색 결과가 없습니다. 다른 장소 이름을 입력해 주세요." }
        } catch {
            if generation == searchGeneration, page == .search {
                searchMessage = "장소를 검색하지 못했습니다. 네트워크를 확인하고 다시 시도해 주세요."
            }
        }
    }

    private func cancelSearch() {
        searchGeneration += 1
        activeSearch?.cancel()
        activeSearch = nil
        isSearching = false
        searchFocused = false
    }

    private func clearRoute() {
        routeGeneration += 1
        routeTask?.cancel()
        transitTask?.cancel()
        activeDirections?.cancel()
        activeTransitDirections?.cancel()
        activeDirections = nil
        activeTransitDirections = nil
        routeTask = nil
        transitTask = nil
        isRouting = false
        isCheckingTransit = false
        transitETA = nil
        localRoutes = []
        appleRoutes = []
        selectedLocalIndex = 0
        selectedAppleIndex = nil
        selectedEntrance = nil
        offRouteReadings = 0
    }

    private func findRoute() {
        routeMessage = nil
        guard selectedPlace != nil else { return }
        guard !mobility.requiresStepFree || hasOutdoorData else {
            routeMessage = "검증된 보행망 자료가 없어 경로를 계산할 수 없습니다."
            return
        }
        guard startPoint != nil else {
            routeMessage = "현재 위치를 확인하거나 지도에서 출발지를 선택해 주세요."
            locateMe()
            return
        }
        recalculate()
    }

    private func locateMe() {
        locationService.start()
        if let location = accurateLocation { focusMap(on: location) }
        else { focusOnNextLocation = true }
    }

    private func compareTransit(from start: GeoPoint, to destination: GeoPoint, generation: Int) {
        let request = MKDirections.Request()
        request.source = MKMapItem(placemark: MKPlacemark(coordinate: CLLocationCoordinate2D(
            latitude: start.latitude, longitude: start.longitude)))
        request.destination = MKMapItem(placemark: MKPlacemark(coordinate: CLLocationCoordinate2D(
            latitude: destination.latitude, longitude: destination.longitude)))
        request.transportType = .transit
        request.departureDate = Date()
        let directions = MKDirections(request: request)
        activeTransitDirections = directions
        isCheckingTransit = true
        transitTask = Task { @MainActor in
            let estimate = try? await directions.calculateETA()
            guard !Task.isCancelled, generation == routeGeneration else { return }
            transitETA = estimate?.expectedTravelTime
            isCheckingTransit = false
            activeTransitDirections = nil
            transitTask = nil
        }
    }

    private func openTransitInMaps() {
        guard let start = startPoint, let destination = selectedPlace?.coordinate else { return }
        let origin = MKMapItem(placemark: MKPlacemark(coordinate: CLLocationCoordinate2D(
            latitude: start.latitude, longitude: start.longitude)))
        let goal = MKMapItem(placemark: MKPlacemark(coordinate: CLLocationCoordinate2D(
            latitude: destination.latitude, longitude: destination.longitude)))
        MKMapItem.openMaps(with: [origin, goal], launchOptions: [
            MKLaunchOptionsDirectionsModeKey: MKLaunchOptionsDirectionsModeTransit
        ])
    }

    private func focusMap(on location: CLLocation) {
        camera = .region(MKCoordinateRegion(
            center: location.coordinate,
            span: MKCoordinateSpan(latitudeDelta: 0.006, longitudeDelta: 0.006)
        ))
    }

    private func focusOnSelectedRoute() {
        var coordinates: [CLLocationCoordinate2D] = []
        if let local = selectedLocalRoute {
            coordinates = local.route.coordinates.map {
                CLLocationCoordinate2D(latitude: $0.latitude, longitude: $0.longitude)
            }
        } else if let apple = selectedAppleRoute {
            let points = apple.polyline.points()
            coordinates = (0..<apple.polyline.pointCount).map { points[$0].coordinate }
        }
        // Include the actual endpoints, without drawing an unverified connection.
        for point in [startPoint, selectedPlace?.coordinate].compactMap({ $0 }) {
            coordinates.append(CLLocationCoordinate2D(latitude: point.latitude, longitude: point.longitude))
        }
        guard let first = coordinates.first else { return }
        var bounds = MKMapRect(origin: MKMapPoint(first), size: MKMapSize(width: 0, height: 0))
        for coordinate in coordinates.dropFirst() {
            bounds = bounds.union(MKMapRect(origin: MKMapPoint(coordinate),
                                            size: MKMapSize(width: 0, height: 0)))
        }
        let minimumPadding = MKMapPointsPerMeterAtLatitude(first.latitude) * 80
        camera = .rect(bounds.insetBy(dx: -max(bounds.width * 0.15, minimumPadding),
                                      dy: -max(bounds.height * 0.15, minimumPadding)))
    }

    private func recalculate(from currentLocation: CLLocation? = nil) {
        let current = validLocation(currentLocation).map {
            GeoPoint(latitude: $0.coordinate.latitude, longitude: $0.coordinate.longitude)
        }
        guard let place = selectedPlace, let start = startOverride ?? current ?? startPoint else {
            clearRoute()
            routeMessage = "현재 위치 또는 목적지를 확인할 수 없어 경로 안내를 멈췄습니다."
            return
        }
        let canUseLocalGraph = preparedRouter != nil && hasOutdoorData && graph.covers(start) && graph.covers(place.coordinate)
        guard !mobility.requiresStepFree || canUseLocalGraph else {
            clearRoute()
            routeMessage = "현재 제공된 보행망 범위 밖입니다. 이 지역의 경로 자료가 필요합니다."
            return
        }
        clearRoute()
        let generation = routeGeneration
        let network = graph, limit = grade.percent, routeMode = mode, cachedRouter = preparedRouter
        let hasConfirmedGrades = confirmedGradeCount > 0
        let stepFree = mobility.requiresStepFree, speed = speedMetersPerSecond
        isRouting = true
        routeMessage = nil
        let calculation = Task.detached(priority: .userInitiated) { () -> (candidates: [LocalRouteCandidate], message: String?) in
            guard !Task.isCancelled, canUseLocalGraph, let router = cachedRouter else { return ([], nil) }
            var preview = stepFree && !hasConfirmedGrades
            var chosen = preview ? nil : router.route(from: start, to: place.coordinate,
                maxMeasuredGradePercent: limit, mode: routeMode, requiresStepFree: stepFree,
                speedMetersPerSecond: speed)
            if chosen == nil && stepFree {
                preview = true
                chosen = router.route(from: start, to: place.coordinate,
                    maxMeasuredGradePercent: limit, mode: routeMode, requiresStepFree: true,
                    allowUnverifiedGrade: true, speedMetersPerSecond: speed)
            }
            guard let chosen else {
                var gaps: [String] = []
                for (label, point) in [("출발지", start), ("도착지", place.coordinate)] {
                    guard !Task.isCancelled else { return ([], nil) }
                    let gap = network.edges.reduce(Double.infinity) {
                        min($0, WheelchairRouter.distance(from: point, to: $1.coordinates))
                    }
                    if gap.isFinite && gap > 35 {
                        gaps.append("\(label)가 보행망에서 약 \(Int(gap.rounded()))m 떨어져 있습니다")
                    }
                }
                return ([], gaps.isEmpty ? nil : gaps.joined(separator: ". ") + ". 실제 출입구를 지도에서 선택해 주세요. 접속 범위는 35m입니다.")
            }
            var candidates = [LocalRouteCandidate(route: chosen.route,
                destinationGapMeters: chosen.destinationGapMeters, mode: routeMode,
                isUnverifiedPreview: preview || chosen.route.unknownEdgeCount > 0
                    || chosen.route.snapDistanceMeters > 5 || chosen.destinationGapMeters > 5)]
            for candidateMode in [RouteMode.fastest, .shortest, .gentle] where candidateMode != routeMode {
                guard !Task.isCancelled else { return ([], nil) }
                if let other = router.route(from: start, to: place.coordinate,
                    maxMeasuredGradePercent: limit, mode: candidateMode, requiresStepFree: stepFree,
                    allowUnverifiedGrade: preview,
                    speedMetersPerSecond: speed),
                    !candidates.contains(where: { $0.route.edgeIDs == other.route.edgeIDs }) {
                    candidates.append(LocalRouteCandidate(route: other.route,
                        destinationGapMeters: other.destinationGapMeters, mode: candidateMode,
                        isUnverifiedPreview: preview || other.route.unknownEdgeCount > 0
                            || other.route.snapDistanceMeters > 5 || other.destinationGapMeters > 5))
                }
            }
            return (candidates, nil)
        }
        let directions: MKDirections?
        if stepFree {
            directions = nil
        } else {
            let request = MKDirections.Request()
            request.source = MKMapItem(placemark: MKPlacemark(coordinate: CLLocationCoordinate2D(
                latitude: start.latitude, longitude: start.longitude)))
            request.destination = MKMapItem(placemark: MKPlacemark(coordinate: CLLocationCoordinate2D(
                latitude: place.coordinate.latitude, longitude: place.coordinate.longitude)))
            request.transportType = .walking
            request.requestsAlternateRoutes = true
            directions = MKDirections(request: request)
        }
        activeDirections = directions
        routeTask = Task { @MainActor in
            let (own, apple, localMessage) = await withTaskCancellationHandler {
                let apple: [MKRoute]
                if let directions { apple = (try? await directions.calculate().routes) ?? [] }
                else { apple = [] }
                let own = await calculation.value
                return (own.candidates, apple, own.message)
            } onCancel: {
                calculation.cancel()
            }
            guard !Task.isCancelled, generation == routeGeneration else { return }
            isRouting = false
            routeTask = nil
            activeDirections = nil
            guard startOverride != nil || accurateLocation != nil else {
                routeMessage = "현재 위치가 오래되어 경로 안내를 멈췄습니다. 위치를 다시 확인해 주세요."
                return
            }
            if startOverride == nil, let latest = accurateLocation {
                let latestPoint = GeoPoint(latitude: latest.coordinate.latitude,
                                           longitude: latest.coordinate.longitude)
                if latestPoint.distance(to: start) > max(25, latest.horizontalAccuracy * 2) {
                    recalculate(from: latest)
                    return
                }
            }
            guard !own.isEmpty || !apple.isEmpty else {
                routeMessage = localMessage ?? (stepFree
                    ? "이동 조건을 만족하는 경로가 없습니다. 미확인 경사·계단·단절 구간 또는 보행망 밖 위치일 수 있습니다."
                    : "자체 보행망과 Apple 지도에서 경로를 찾지 못했습니다. 검색 장소의 출입구나 네트워크 상태를 확인해 주세요.")
                return
            }
            localRoutes = own
            appleRoutes = apple
            if let quickestApple = apple.indices.min(by: {
                apple[$0].expectedTravelTime < apple[$1].expectedTravelTime
            }), own.isEmpty || apple[quickestApple].expectedTravelTime < own[0].route.estimatedDurationSeconds {
                selectedAppleIndex = quickestApple
            } else {
                selectedAppleIndex = nil
            }
            selectedEntrance = selectedAppleIndex == nil ? own.first.flatMap { best in
                network.entrances.first { entrance in
                    best.route.coordinates.last.map { $0.distance(to: entrance.coordinate) < 0.01 } ?? false
                }
            } : nil
            stationRouteID = availableStationRoutes.first?.id ?? ""
            departureTime = Date()
            page = .route
            panelDetent = .height(320)
            if !stepFree && start.distance(to: place.coordinate) > 1_500 {
                compareTransit(from: start, to: place.coordinate, generation: generation)
            }
            focusOnSelectedRoute()
        }
    }
}

private struct StationStepsView: View {
    let route: StationRoute
    let reviewedAt: String?
    @Environment(\.dismiss) private var dismiss
    @State private var index = 0

    var body: some View {
        NavigationStack {
            VStack(alignment: .leading, spacing: 24) {
                Text(route.title)
                    .font(.title2.bold())
                Text("\(index + 1) / \(route.steps.count) 단계")
                    .font(.headline).foregroundStyle(.secondary)
                Text(route.steps[index].text)
                    .font(.title3)
                    .frame(maxWidth: .infinity, alignment: .leading)
                if !route.steps[index].fromName.isEmpty || !route.steps[index].toName.isEmpty {
                    Text("\(route.steps[index].fromName) → \(route.steps[index].toName)")
                        .font(.subheadline).foregroundStyle(.secondary)
                }
                Spacer()
                Text("역내 위치는 자동 추적하지 않습니다. 단계를 확인한 뒤 직접 넘겨 주세요.")
                    .font(.subheadline).foregroundStyle(.secondary)
                HStack(spacing: 12) {
                    Button("이전") { index = max(0, index - 1) }
                        .disabled(index == 0)
                    Button(index == route.steps.count - 1 ? "완료" : "다음") {
                        if index == route.steps.count - 1 { dismiss() }
                        else { index += 1 }
                    }
                    .buttonStyle(.borderedProminent)
                }
                .frame(maxWidth: .infinity)
                .controlSize(.large)
                Text("공식 자료 대조일 \(reviewedAt ?? "미확인") · 운행 여부 현장 확인")
                    .font(.caption).foregroundStyle(.secondary)
            }
            .padding(24)
            .navigationTitle("역내 단계 안내")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar { ToolbarItem(placement: .topBarTrailing) { Button("닫기") { dismiss() } } }
        }
        .presentationDetents([.large])
    }
}
