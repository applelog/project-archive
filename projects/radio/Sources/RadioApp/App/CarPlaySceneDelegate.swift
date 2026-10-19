// Sources/RadioApp/App/CarPlaySceneDelegate.swift
import Foundation
#if canImport(CarPlay) && canImport(UIKit)
import UIKit
import CarPlay

public final class CarPlaySceneDelegate: UIResponder, CPTemplateApplicationSceneDelegate {
    private var interfaceController: CPInterfaceController?
    private let playerManager = RadioPlayerManager.shared
    
    public func templateApplicationScene(
        _ templateApplicationScene: CPTemplateApplicationScene,
        didConnect interfaceController: CPInterfaceController
    ) {
        self.interfaceController = interfaceController
        
        let nowPlayingBarButton = CPBarButton(title: "재생 중") { [weak self] _ in
            self?.interfaceController?.pushTemplate(CPNowPlayingTemplate.shared, animated: true, completion: nil)
        }
        
        let stations = KoreanStationPresets.allStations
        
        let items = stations.map { station in
            let item = CPListItem(
                text: station.name,
                detailText: "\(station.network.displayName) • \(station.frequencyLabel)"
            )
            item.handler = { [weak self] _, completion in
                guard let self else { completion(); return }
                self.playerManager.play(station: station)
                self.interfaceController?.pushTemplate(CPNowPlayingTemplate.shared, animated: true, completion: nil)
                completion()
            }
            return item
        }
        
        let listTemplate = CPListTemplate(
            title: "한국 라디오 (No-Ad)",
            sections: [CPListSection(items: items, header: "실시간 FM 라디오 채널", sectionIndexTitle: nil)]
        )
        listTemplate.trailingNavigationBarButtons = [nowPlayingBarButton]
        
        interfaceController.setRootTemplate(listTemplate, animated: true, completion: nil)
    }
    
    public func templateApplicationScene(
        _ templateApplicationScene: CPTemplateApplicationScene,
        didDisconnectInterfaceController interfaceController: CPInterfaceController
    ) {
        self.interfaceController = nil
    }
}
#endif
