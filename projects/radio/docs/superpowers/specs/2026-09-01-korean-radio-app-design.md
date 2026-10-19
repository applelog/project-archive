# Design Spec: Korean Radio iOS App (No-Ad, CarPlay, MP3 Auto-Scan)

- **Date**: 2026-09-01
- **Platform**: iOS 17.0+ (Optimized for iPhone 17, SwiftUI, Native Swift)
- **Target Audience**: Personal in-car & everyday radio listener

---

## 1. Overview & Goals

- **Problem**: Commercial Korean radio apps (KBS Kong, MBC mini, SBS Gorilla) contain annoying dynamic audio/visual advertisements, slow startup times, and separate fragmented apps.
- **Solution**: A lightweight, unified, zero-ad Korean radio streaming app built with native SwiftUI, featuring:
  1. Direct HLS live streams (`.m3u8` / AAC) for all major Korean networks.
  2. Full CarPlay dashboard & steering wheel remote command center integration.
  3. Retro MP3 player-style full frequency sweep (87.5 MHz ~ 108.0 MHz) auto-scan & manual frequency tuner dial with haptic feedback.
  4. Audio session ducking during navigation voice prompts (T-Map, KakaoNavi).

---

## 2. Architecture & Tech Stack

```
┌─────────────────────────────────────────────────────────────┐
│                      UI Presentation                        │
│  ┌───────────────────────────────┐ ┌──────────────────────┐ │
│  │ SwiftUI Main View             │ │ CarPlay Scene        │ │
│  │ - Retro Frequency Tuner Bar   │ │ - CPListTemplate     │ │
│  │ - Auto-Scan (87.5~108.0 MHz)  │ │ - CPNowPlaying       │ │
│  │ - Minimal Now Playing Player  │ │   Template           │ │
│  │ - Preset Grid / Channel List  │ │ - Steering Wheel Seek│ │
│  └───────────────────────────────┘ └──────────────────────┘ │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                    RadioPlayerManager                       │
│  - AVPlayer (HLS Live Stream Playback)                      │
│  - AVAudioSession (.playback, mode: .spokenAudio/default)    │
│  - MPNowPlayingInfoCenter (Metadata, Station Art/Title)     │
│  - MPRemoteCommandCenter (Play/Pause, Next/Prev Station)    │
│  - HapticFeedbackManager (Taptic Engine ticks & snap)       │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                       RadioDataStore                        │
│  - Built-in Korean Radio Presets (Sorted by Frequency MHz)  │
│  - Auto-Scan Channel Scanner Engine                         │
│  - Custom Stream URL Persistence (UserDefaults / JSON)      │
└─────────────────────────────────────────────────────────────┘
```

### Dependencies
- **External Dependencies**: 0% (Pure Apple Native Frameworks: `SwiftUI`, `AVFoundation`, `MediaPlayer`, `CarPlay`).

---

## 3. Korean Station Frequency Map (Default Presets)

| Frequency | Network | Channel Name | Category | Stream Format |
|---|---|---|---|---|
| **89.1 MHz** | KBS | 2FM 쿨FM (Cool FM) | K-Pop / Variety | HLS (m3u8) |
| **91.9 MHz** | MBC | FM4U | Music / Entertainment | HLS (m3u8) |
| **93.1 MHz** | KBS | 1FM 클래식FM | Classical / Traditional | HLS (m3u8) |
| **93.9 MHz** | CBS | 음악FM (Joy4U) | Pop / Classical / Ballad | HLS (m3u8) |
| **95.1 MHz** | TBS | 교통방송 FM | Traffic / News / Culture | HLS (m3u8) |
| **95.9 MHz** | MBC | 표준FM | News / Current Affairs | HLS (m3u8) |
| **97.3 MHz** | KBS | 1라디오 | News / Current Affairs | HLS (m3u8) |
| **98.1 MHz** | CBS | 표준FM | News / Religion | HLS (m3u8) |
| **103.5 MHz** | SBS | 러브FM (Love FM) | News / Entertainment | HLS (m3u8) |
| **104.5 MHz** | EBS | FM | Education / Culture | HLS (m3u8) |
| **106.1 MHz** | KBS | 2라디오 해피FM | Adult Contemporary | HLS (m3u8) |
| **107.7 MHz** | SBS | 파워FM (Power FM) | Variety / K-Pop | HLS (m3u8) |
| **N/A (Online)**| YTN | YTN 라디오 | 24h News | HLS (m3u8) |

---

## 4. Key Feature Specifications

### 4.1. MP3-Style Auto Frequency Scan (87.5 MHz ~ 108.0 MHz)
- **Auto-Scan Trigger**: User taps `[AUTO SCAN]` button.
- **Visual Animation**: Frequency needle sweeps from `87.5 MHz` to `108.0 MHz` with a smooth animated display and sound/haptic ticks.
- **Channel Detection**: As the needle hits a valid preset frequency, the station is highlighted, registered into the active preset list (CH 01, CH 02...), and played briefly or added.
- **Haptic Feedback**: High-frequency light impact haptics as the counter increments, with a rigid impact when catching an active station.

### 4.2. Interactive Frequency Tuner Dial
- A horizontal draggable frequency scale (87.5 ~ 108.0 MHz) with minor (0.1 MHz) and major (1.0 MHz) tick marks.
- **Magnetic Snap**: When dragged near an active radio frequency (e.g. 91.9, 107.7), it snaps smoothly with a tactile haptic click and tunes in.
- Quick step buttons (`<<` and `>>`) to step by 0.1 MHz or jump to previous/next preset.

### 4.3. CarPlay & In-Car Control
- **CarPlay Template (`CPListTemplate`)**: Displays channel list categorized by preset and frequency.
- **CarPlay Now Playing (`CPNowPlayingTemplate`)**: Shows station name, frequency badge (e.g. "107.7 MHz"), and network branding.
- **Steering Wheel Controls (`MPRemoteCommandCenter`)**:
  - Next Track / Previous Track buttons ➔ Jump to Next/Previous tuned station.
  - Play / Pause button ➔ Toggle live stream.
- **Navigation Voice Ducking**: Configured with `AVAudioSession.CategoryOptions.duckOthers` so navigation directions automatically lower radio volume without pausing.

### 4.4. Audio Resilience & Zero-Ad Guarantee
- **Direct Stream Buffering**: Directly buffers official HLS stream playlists without third-party ad interceptors.
- **Auto-Reconnect**: Automatically reconnects if cellular network switches between 5G/LTE/Wi-Fi or drives through a tunnel.

---

## 5. File & Project Structure

```
Radio/
├── App/
│   ├── RadioApp.swift                  # App Entry Point & CarPlay Scene Delegate setup
│   └── CarPlaySceneDelegate.swift      # CarPlay Interface & Template Management
├── Models/
│   └── RadioStation.swift              # Station data model (id, name, network, frequency, streamURL, logo)
├── Services/
│   ├── RadioPlayerManager.swift        # AVPlayer, AudioSession, RemoteCommand, NowPlaying
│   ├── AutoScanEngine.swift            # 87.5 ~ 108.0 MHz Scan & Frequency stepper logic
│   └── HapticManager.swift             # Sensory feedback coordinator
├── ViewModels/
│   └── RadioViewModel.swift            # Main UI state, active station, tuning frequency
└── Views/
    ├── MainRadioView.swift             # Root view with player and station list
    ├── FrequencyTunerBar.swift         # MP3-style frequency slider & ticks
    ├── NowPlayingCard.swift            # Channel badge, animated waveform / status
    ├── StationListView.swift           # Grid/List of stations
    └── Components/
        └── RetroDisplayView.swift      # Digital frequency LCD style readout
```

---

## 6. Verification & Testing Strategy

1. **Audio Streaming Test**: Verify all preset stream URLs play cleanly through `AVPlayer` with low latency (< 1 sec startup).
2. **Auto-Scan Simulation**: Verify scan loop traverses 87.5 to 108.0 MHz, properly hits all active stations, and generates valid presets.
3. **Background & Lock Screen Test**: Ensure audio continues in background and lock screen media widget responds to play/pause and skip commands.
4. **CarPlay & Steering Wheel Test**: Verify `MPRemoteCommandCenter` responds to remote commands and CarPlay scene initializes properly.
