// Sources/RadioApp/Views/NowPlayingCard.swift
import SwiftUI

public struct NowPlayingCard: View {
    public let station: RadioStation?
    public let state: PlaybackState
    public let onTogglePlay: () -> Void
    public let onNext: () -> Void
    public let onPrev: () -> Void
    public let onAutoScan: () -> Void
    public let isScanning: Bool
    
    public init(
        station: RadioStation?,
        state: PlaybackState,
        isScanning: Bool,
        onTogglePlay: @escaping () -> Void,
        onNext: @escaping () -> Void,
        onPrev: @escaping () -> Void,
        onAutoScan: @escaping () -> Void
    ) {
        self.station = station
        self.state = state
        self.isScanning = isScanning
        self.onTogglePlay = onTogglePlay
        self.onNext = onNext
        self.onPrev = onPrev
        self.onAutoScan = onAutoScan
    }
    
    public var body: some View {
        HStack(spacing: 16) {
            // Auto Scan button (Old-school MP3 style)
            Button(action: onAutoScan) {
                VStack(spacing: 4) {
                    Image(systemName: isScanning ? "antenna.radiowaves.left.and.right" : "sparkle.magnifyingglass")
                        .font(.system(size: 20))
                    Text("AUTO")
                        .font(.system(size: 10, weight: .bold, design: .monospaced))
                }
                .foregroundStyle(isScanning ? .yellow : .white)
                .frame(width: 52, height: 52)
                .background(Circle().fill(Color.white.opacity(0.12)))
            }
            .disabled(isScanning)
            
            Spacer()
            
            // Previous Button
            Button(action: onPrev) {
                Image(systemName: "backward.fill")
                    .font(.system(size: 24))
                    .foregroundStyle(.white)
            }
            
            // Main Play/Pause Button
            Button(action: onTogglePlay) {
                ZStack {
                    Circle()
                        .fill(Color.orange)
                        .frame(width: 64, height: 64)
                    
                    if state == .buffering {
                        ProgressView()
                            .tint(.black)
                    } else {
                        Image(systemName: state == .playing ? "pause.fill" : "play.fill")
                            .font(.system(size: 28))
                            .foregroundStyle(.black)
                    }
                }
            }
            
            // Next Button
            Button(action: onNext) {
                Image(systemName: "forward.fill")
                    .font(.system(size: 24))
                    .foregroundStyle(.white)
            }
            
            Spacer()
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 8)
    }
}
