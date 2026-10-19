import CoreLocation
import Foundation

final class LocationService: NSObject, ObservableObject, CLLocationManagerDelegate {
    @Published private(set) var location: CLLocation?
    @Published private(set) var authorization: CLAuthorizationStatus = .notDetermined
    @Published private(set) var message: String?

    private let manager = CLLocationManager()

    override init() {
        super.init()
        manager.delegate = self
        manager.desiredAccuracy = kCLLocationAccuracyBest
        manager.distanceFilter = 5
        authorization = manager.authorizationStatus
    }

    func start() {
        switch manager.authorizationStatus {
        case .notDetermined:
            manager.requestWhenInUseAuthorization()
        case .authorizedAlways, .authorizedWhenInUse:
            message = nil
            manager.startUpdatingLocation()
        case .denied, .restricted:
            message = "위치 권한이 꺼져 있습니다. iPhone 설정에서 이 앱의 사용 중 위치 접근을 허용해 주세요."
        @unknown default:
            message = "위치 권한 상태를 확인할 수 없습니다."
        }
    }

    func locationManagerDidChangeAuthorization(_ manager: CLLocationManager) {
        DispatchQueue.main.async {
            self.authorization = manager.authorizationStatus
            if self.authorization == .authorizedAlways || self.authorization == .authorizedWhenInUse {
                self.manager.startUpdatingLocation()
            } else if self.authorization == .denied || self.authorization == .restricted {
                self.location = nil
                self.message = "위치 권한이 없어 경로를 계산할 수 없습니다."
            }
        }
    }

    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        guard let latest = locations.last else { return }
        DispatchQueue.main.async {
            self.location = latest
            self.message = nil
        }
    }

    func locationManager(_ manager: CLLocationManager, didFailWithError error: Error) {
        DispatchQueue.main.async {
            self.message = "현재 위치를 확인하지 못했습니다. 잠시 후 다시 시도해 주세요."
        }
    }
}
