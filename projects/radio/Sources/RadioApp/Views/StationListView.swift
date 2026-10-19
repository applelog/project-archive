// Sources/RadioApp/Views/StationListView.swift
import SwiftUI

public struct StationListView: View {
    public let stations: [RadioStation]
    public let currentStation: RadioStation?
    public let isPlaying: Bool
    public let onSelect: (RadioStation) -> Void
    
    public init(
        stations: [RadioStation],
        currentStation: RadioStation?,
        isPlaying: Bool,
        onSelect: @escaping (RadioStation) -> Void
    ) {
        self.stations = stations
        self.currentStation = currentStation
        self.isPlaying = isPlaying
        self.onSelect = onSelect
    }
    
    public var body: some View {
        LazyVStack(spacing: 8) {
            ForEach(stations) { station in
                let isSelected = currentStation?.id == station.id
                Button {
                    onSelect(station)
                } label: {
                    HStack(spacing: 12) {
                        // Network badge with signature Apple network color
                        Text(station.network.rawValue)
                            .font(.system(size: 11, weight: .bold, design: .rounded))
                            .foregroundStyle(.white)
                            .frame(width: 44, height: 26)
                            .background(
                                RoundedRectangle(cornerRadius: 7, style: .continuous)
                                    .fill(station.network.accentColor)
                            )
                            .fixedSize()
                        
                        VStack(alignment: .leading, spacing: 2) {
                            Text(station.name)
                                .font(.system(size: 15, weight: isSelected ? .bold : .medium))
                                .foregroundStyle(isSelected ? .orange : .white)
                                .lineLimit(1)
                                .minimumScaleFactor(0.85)
                            
                            Text(station.category)
                                .font(.system(size: 12))
                                .foregroundStyle(Color.gray)
                                .lineLimit(1)
                        }
                        
                        Spacer(minLength: 8)
                        
                        HStack(spacing: 6) {
                            Text(station.frequencyLabel)
                                .font(.system(size: 13, weight: .bold, design: .monospaced))
                                .foregroundStyle(isSelected ? .orange : .gray)
                                .fixedSize()
                            
                            if isSelected && isPlaying {
                                Image(systemName: "waveform")
                                    .symbolEffect(.variableColor.iterative)
                                    .foregroundStyle(.orange)
                                    .fixedSize()
                            }
                        }
                    }
                    .padding(.horizontal, 14)
                    .padding(.vertical, 11)
                    .background(
                        RoundedRectangle(cornerRadius: 12, style: .continuous)
                            .fill(isSelected ? Color.white.opacity(0.08) : Color.white.opacity(0.03))
                    )
                }
                .buttonStyle(.plain)
            }
        }
    }
}
