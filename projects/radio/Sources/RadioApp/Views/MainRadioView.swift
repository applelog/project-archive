// Sources/RadioApp/Views/MainRadioView.swift
import SwiftUI

public struct MainRadioView: View {
    @State private var viewModel = RadioViewModel()
    @State private var showHistorySheet: Bool = false
    @FocusState private var isSearchFocused: Bool
    
    public init() {}
    
    public var body: some View {
        NavigationStack {
            ZStack {
                Color(red: 0.04, green: 0.04, blue: 0.06).ignoresSafeArea()
                    .onTapGesture {
                        if isSearchFocused {
                            isSearchFocused = false
                        }
                    }
                
                ScrollView(.vertical, showsIndicators: true) {
                    VStack(spacing: 16) {
                        // Search Bar with Apple standard cancel button & focus management
                        HStack(spacing: 10) {
                            HStack {
                                Image(systemName: "magnifyingglass")
                                    .foregroundStyle(.gray)
                                TextField("주파수, 채널명, 방송사 검색", text: $viewModel.searchQuery)
                                    .foregroundStyle(.white)
                                    .focused($isSearchFocused)
                                    .submitLabel(.search)
                                    .onSubmit {
                                        isSearchFocused = false
                                    }
                                
                                if !viewModel.searchQuery.isEmpty {
                                    Button {
                                        viewModel.searchQuery = ""
                                    } label: {
                                        Image(systemName: "xmark.circle.fill")
                                            .foregroundStyle(.gray)
                                    }
                                }
                            }
                            .padding(10)
                            .background(RoundedRectangle(cornerRadius: 12, style: .continuous).fill(Color.white.opacity(0.08)))
                            
                            if isSearchFocused {
                                Button("취소") {
                                    withAnimation(.spring(response: 0.3, dampingFraction: 0.8)) {
                                        isSearchFocused = false
                                        viewModel.searchQuery = ""
                                    }
                                }
                                .font(.system(size: 15, weight: .medium))
                                .foregroundStyle(.orange)
                                .transition(.move(edge: .trailing).combined(with: .opacity))
                            }
                        }
                        .padding(.horizontal, 16)
                        
                        // Retro Tuner LCD Readout with Live Song / Broadcast Info
                        RetroDisplayView(
                            frequency: viewModel.tuningFrequency,
                            stationName: viewModel.currentStation?.name ?? "라디오 대기 중",
                            networkName: viewModel.currentStation?.network.displayName ?? "FM STEREO",
                            programTitle: viewModel.currentProgramTitle,
                            djName: viewModel.currentDJName,
                            programPhotoURL: viewModel.currentProgramPhotoURL,
                            songInfo: viewModel.currentSong,
                            isLive: viewModel.playbackState == .playing,
                            isScanning: viewModel.isScanning
                        )
                        .padding(.horizontal, 16)
                        
                        // Analog MP3 Frequency Slider Scale
                        FrequencyTunerBar(
                            frequency: $viewModel.tuningFrequency,
                            stations: viewModel.stations,
                            onFrequencyChanged: { freq in
                                viewModel.tuneTo(frequency: freq)
                            }
                        )
                        .padding(.horizontal, 16)
                        
                        // Media Controls
                        NowPlayingCard(
                            station: viewModel.currentStation,
                            state: viewModel.playbackState,
                            isScanning: viewModel.isScanning,
                            onTogglePlay: { viewModel.togglePlayback() },
                            onNext: { viewModel.nextStation() },
                            onPrev: { viewModel.previousStation() },
                            onAutoScan: { viewModel.startAutoScan() }
                        )
                        .padding(.horizontal, 8)
                        
                        // Section Header
                        HStack {
                            Text("채널 목록 (\(viewModel.filteredStations.count))")
                                .font(.system(size: 14, weight: .bold))
                                .foregroundStyle(.gray)
                            Spacer()
                            if viewModel.scannedSignalDetected {
                                HStack(spacing: 4) {
                                    Circle().fill(Color.green).frame(width: 6, height: 6)
                                    Text("신호 수신됨")
                                        .font(.system(size: 11, weight: .medium))
                                        .foregroundStyle(.green)
                                }
                            }
                        }
                        .padding(.horizontal, 18)
                        .padding(.top, 4)
                        
                        // Station List
                        StationListView(
                            stations: viewModel.filteredStations,
                            currentStation: viewModel.currentStation,
                            isPlaying: viewModel.playbackState == .playing,
                            onSelect: { station in
                                viewModel.selectStation(station)
                            }
                        )
                        .padding(.horizontal, 16)
                        .padding(.bottom, 24)
                    }
                    .padding(.top, 8)
                }
                .scrollDismissesKeyboard(.interactively)
            }
            .navigationTitle("라디오")
            #if os(iOS)
            .navigationBarTitleDisplayMode(.inline)
            .toolbarBackground(.hidden, for: .navigationBar)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button {
                        showHistorySheet = true
                    } label: {
                        HStack(spacing: 4) {
                            Image(systemName: "music.note.list")
                                .font(.system(size: 16, weight: .semibold))
                            Text("선곡표")
                                .font(.system(size: 13, weight: .bold))
                        }
                        .foregroundStyle(.orange)
                        .padding(.horizontal, 10)
                        .padding(.vertical, 5)
                        .background(Capsule().fill(Color.orange.opacity(0.15)))
                    }
                }
                
                ToolbarItemGroup(placement: .keyboard) {
                    Spacer()
                    Button("완료") {
                        isSearchFocused = false
                    }
                    .fontWeight(.bold)
                    .foregroundStyle(.orange)
                }
            }
            .sheet(isPresented: $showHistorySheet) {
                SongHistoryView()
            }
            #endif
        }
        .preferredColorScheme(.dark)
    }
}
