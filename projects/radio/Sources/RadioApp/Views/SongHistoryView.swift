// Sources/RadioApp/Views/SongHistoryView.swift
import SwiftUI

public struct SongHistoryView: View {
    @Bindable private var historyManager = SongHistoryManager.shared
    @Environment(\.dismiss) private var dismiss
    
    public init() {}
    
    public var body: some View {
        NavigationStack {
            ZStack {
                Color(red: 0.04, green: 0.04, blue: 0.06).ignoresSafeArea()
                
                if historyManager.recentSongs.isEmpty {
                    VStack(spacing: 16) {
                        Image(systemName: "music.note.list")
                            .font(.system(size: 48))
                            .foregroundStyle(.gray.opacity(0.6))
                        
                        Text("선곡 기록이 없습니다")
                            .font(.system(size: 18, weight: .bold))
                            .foregroundStyle(.white)
                        
                        Text("라디오를 청취하는 동안 흘러나온 노래들이\n여기에 자동으로 기록되어 언제든 스포티파이로 들을 수 있습니다.")
                            .font(.system(size: 14))
                            .foregroundStyle(.gray)
                            .multilineTextAlignment(.center)
                            .padding(.horizontal, 32)
                    }
                } else {
                    List {
                        ForEach(historyManager.recentSongs) { item in
                            SongHistoryRow(item: item)
                                .listRowInsets(EdgeInsets(top: 8, leading: 16, bottom: 8, trailing: 16))
                                .listRowBackground(Color.clear)
                                .listRowSeparator(.hidden)
                        }
                        .onDelete { indexSet in
                            historyManager.recentSongs.remove(atOffsets: indexSet)
                        }
                    }
                    .listStyle(.plain)
                    .scrollContentBackground(.hidden)
                }
            }
            .navigationTitle("방금 나온 노래")
            #if os(iOS)
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    if !historyManager.recentSongs.isEmpty {
                        Button("전체 삭제", role: .destructive) {
                            withAnimation {
                                historyManager.clearHistory()
                            }
                        }
                        .foregroundStyle(.red.opacity(0.8))
                        .font(.system(size: 14))
                    }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("닫기") {
                        dismiss()
                    }
                    .fontWeight(.bold)
                    .foregroundStyle(.orange)
                }
            }
            #endif
        }
        .preferredColorScheme(.dark)
    }
}

private struct SongHistoryRow: View {
    let item: SongHistoryItem
    
    var body: some View {
        HStack(spacing: 12) {
            // Album Art
            if let artURL = item.albumArtURL {
                AsyncImage(url: artURL) { phase in
                    switch phase {
                    case .success(let image):
                        image
                            .resizable()
                            .aspectRatio(contentMode: .fill)
                            .frame(width: 50, height: 50)
                            .clipShape(RoundedRectangle(cornerRadius: 10))
                    default:
                        placeholderImage
                    }
                }
            } else {
                placeholderImage
            }
            
            // Title & Artist & Time
            VStack(alignment: .leading, spacing: 3) {
                Text(item.title)
                    .font(.system(size: 15, weight: .bold))
                    .foregroundStyle(.white)
                    .lineLimit(1)
                    .truncationMode(.tail)
                
                Text(item.artist.isEmpty ? item.stationName : item.artist)
                    .font(.system(size: 13))
                    .foregroundStyle(.gray)
                    .lineLimit(1)
                    .truncationMode(.tail)
                
                HStack(spacing: 6) {
                    Text(item.formattedTime)
                        .font(.system(size: 11, weight: .medium, design: .monospaced))
                        .foregroundStyle(.orange.opacity(0.9))
                        .fixedSize()
                    
                    Text("•")
                        .font(.system(size: 11))
                        .foregroundStyle(.gray.opacity(0.5))
                        .fixedSize()
                    
                    Text("\(item.stationName) (\(item.frequencyLabel))")
                        .font(.system(size: 11))
                        .foregroundStyle(.cyan.opacity(0.9))
                        .lineLimit(1)
                        .truncationMode(.tail)
                }
            }
            
            Spacer(minLength: 8)
            
            // Spotify 1-Click Jump Button
            Button {
                SpotifyLinkManager.openInSpotify(title: item.title, artist: item.artist)
            } label: {
                HStack(spacing: 4) {
                    Image(systemName: "arrow.up.right.circle.fill")
                        .font(.system(size: 13))
                    Text("Spotify")
                        .font(.system(size: 11, weight: .bold, design: .rounded))
                }
                .foregroundStyle(.black)
                .padding(.horizontal, 10)
                .padding(.vertical, 6)
                .background(
                    Capsule().fill(Color(red: 0.11, green: 0.84, blue: 0.38)) // Spotify Green
                )
            }
            .buttonStyle(.plain)
            .fixedSize()
            .layoutPriority(2)
        }
        .padding(12)
        .background(
            RoundedRectangle(cornerRadius: 14, style: .continuous)
                .fill(Color.white.opacity(0.05))
                .overlay(
                    RoundedRectangle(cornerRadius: 14, style: .continuous)
                        .stroke(Color.white.opacity(0.08), lineWidth: 1)
                )
        )
    }
    
    private var placeholderImage: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 10)
                .fill(Color.white.opacity(0.08))
                .frame(width: 50, height: 50)
            Image(systemName: "music.note")
                .font(.system(size: 18))
                .foregroundStyle(.orange)
        }
    }
}
