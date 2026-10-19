// Sources/RadioApp/Views/Components/FrequencyTunerBar.swift
import SwiftUI

public struct FrequencyTunerBar: View {
    @Binding public var frequency: Double
    public let stations: [RadioStation]
    public let onFrequencyChanged: (Double) -> Void
    
    public init(
        frequency: Binding<Double>,
        stations: [RadioStation],
        onFrequencyChanged: @escaping (Double) -> Void
    ) {
        self._frequency = frequency
        self.stations = stations
        self.onFrequencyChanged = onFrequencyChanged
    }
    
    public var body: some View {
        VStack(spacing: 4) {
            GeometryReader { proxy in
                let width = proxy.size.width
                let inset: CGFloat = 16
                let trackWidth = max(1.0, width - (inset * 2))
                
                let minFreq = AutoScanEngine.minFrequency // 87.5
                let maxFreq = AutoScanEngine.maxFrequency // 108.0
                let clampedFreq = max(minFreq, min(maxFreq, frequency))
                let progress = CGFloat((clampedFreq - minFreq) / (maxFreq - minFreq))
                
                ZStack(alignment: .topLeading) {
                    // Track Background Glass Card
                    RoundedRectangle(cornerRadius: 12, style: .continuous)
                        .fill(Color(red: 0.08, green: 0.09, blue: 0.12))
                        .overlay(
                            RoundedRectangle(cornerRadius: 12, style: .continuous)
                                .stroke(Color.white.opacity(0.1), lineWidth: 1)
                        )
                        .frame(height: 52)
                    
                    // Minor & Major Ticks (Calculated precisely by frequency position)
                    ForEach(88...108, id: \.self) { mhz in
                        let mhzDouble = Double(mhz)
                        let xPos = inset + trackWidth * CGFloat((mhzDouble - minFreq) / (maxFreq - minFreq))
                        let isEven = mhz % 2 == 0
                        
                        // Major (even) vs Intermediate (odd) ticks
                        Group {
                            if isEven {
                                // Major 2 MHz mark with label
                                VStack(spacing: 2) {
                                    Rectangle()
                                        .fill(Color.white.opacity(0.7))
                                        .frame(width: 1.5, height: 12)
                                    
                                    Text("\(mhz)")
                                        .font(.system(size: 9, weight: .bold, design: .monospaced))
                                        .foregroundStyle(Color.white.opacity(0.6))
                                }
                                .position(x: xPos, y: 22)
                            } else {
                                // Intermediate 1 MHz mark (tick only, no text collision)
                                Rectangle()
                                    .fill(Color.white.opacity(0.35))
                                    .frame(width: 1.0, height: 7)
                                    .position(x: xPos, y: 11)
                            }
                        }
                    }
                    
                    // Station markers (Orange dots on the scale)
                    ForEach(stations.compactMap { $0.frequency != nil ? $0 : nil }) { st in
                        let stFreq = max(minFreq, min(maxFreq, st.frequency ?? minFreq))
                        let stX = inset + trackWidth * CGFloat((stFreq - minFreq) / (maxFreq - minFreq))
                        Circle()
                            .fill(Color.orange.opacity(0.9))
                            .frame(width: 4, height: 4)
                            .shadow(color: .orange.opacity(0.6), radius: 2)
                            .position(x: stX, y: 38)
                    }
                    
                    // Red Needle Indicator with glowing guide line
                    let needleX = inset + trackWidth * progress
                    VStack(spacing: 0) {
                        Capsule()
                            .fill(Color.red)
                            .frame(width: 2.5, height: 44)
                            .shadow(color: .red.opacity(0.8), radius: 3)
                    }
                    .position(x: needleX, y: 26)
                }
                .contentShape(Rectangle())
                .gesture(
                    DragGesture(minimumDistance: 0)
                        .onChanged { value in
                            let clampedX = max(0, min(trackWidth, value.location.x - inset))
                            let newProgress = clampedX / trackWidth
                            let newFreq = minFreq + (maxFreq - minFreq) * Double(newProgress)
                            let roundedFreq = (newFreq * 10).rounded() / 10
                            if roundedFreq != frequency {
                                frequency = roundedFreq
                                onFrequencyChanged(roundedFreq)
                            }
                        }
                )
            }
            .frame(height: 52)
        }
    }
}
