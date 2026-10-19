> 정리 전 문서입니다. 상대 경로와 실행 안내는 이전 위치 기준이며, 기능·검증 주장은 당시 기록입니다. 현재 안내는 프로젝트 README와 루트 README를 참고하세요.

# 📻 한국 라디오 (Korean Radio) - No-Ad, CarPlay, MP3 Auto-Scan

광고 없는 초경량 아이폰 17 & 카플레이(CarPlay) 전용 한국 라디오 앱입니다.

---

## ✨ 주요 기능

1. **광고 없는 실시간 스트리밍 (0-Ad)**
   - 외부 광고 SDK 및 트래커 0%, 배터리/데이터 낭비 제로
   - KBS, MBC, SBS, CBS, TBS, EBS, YTN 공식 HLS (`.m3u8` / AAC) 라이브 스트림 직결

2. **MP3 감성 전체 주파수 자동 스캔 (87.5 ~ 108.0 MHz)**
   - `AUTO` 버튼을 누르면 주파수 바늘이 87.5MHz부터 108.0MHz까지 쫘르륵 올라가며 수신 가능한 방송국을 순차 탐색
   - 아날로그 튜너 틱 소리 및 아이폰 햅틱(Taptic Engine) 진동 연동

3. **아날로그 튜닝 바 (Manual Frequency Tuner)**
   - 87.5 ~ 108.0 MHz 주파수 눈금자를 손가락으로 드래그하여 아날로그 다이얼처럼 주파수 튜닝
   - 방송국 주파수 근처 도달 시 자석처럼 '딸깍' 달라붙는 햅틱 피드백

4. **애플 카플레이(CarPlay) & 차량 핸들 리모컨**
   - 차량 모니터 전용 `CPListTemplate` 채널 탐색 UI 제공
   - 차량 핸들의 `이전곡 / 다음곡` 버튼으로 다음/이전 주파수 채널 즉시 전환
   - 제어센터 및 잠금화면 `MPNowPlayingInfoCenter` 미디어 위젯 완벽 지원

5. **내비게이션 오디오 덕킹 (Audio Ducking)**
   - 티맵(T-Map), 카카오내비, 네이버지도 등 길안내 음성이 나올 때 라디오 볼륨이 부드럽게 감소했다가 복귀

---

## 📡 내장 채널 주파수 맵 (Default Presets)

| 주파수 | 방송사 | 채널명 | 장르 |
|---|---|---|---|
| **89.1 MHz** | KBS | 2FM 쿨FM (Cool FM) | 대중음악 / 예능 |
| **91.9 MHz** | MBC | FM4U | 대중음악 / 종합예능 |
| **93.1 MHz** | KBS | 1FM 클래식FM | 클래식 / 국악 |
| **93.9 MHz** | CBS | 음악FM (Joy4U) | 팝 / 가요 / 힐링 |
| **95.1 MHz** | TBS | 교통방송 FM | 교통정보 / 시사 / 음악 |
| **95.9 MHz** | MBC | 표준FM | 종합뉴스 / 시사 |
| **97.3 MHz** | KBS | 1라디오 | 24시간 뉴스 / 시사 |
| **98.1 MHz** | CBS | 표준FM | 시사 / 교양 / 종교 |
| **103.5 MHz** | SBS | 러브FM (Love FM) | 종합예능 / 가요 |
| **104.5 MHz** | EBS | FM 교육방송 | 어학 / 교육 / 교양 |
| **106.1 MHz** | KBS | 2라디오 해피FM | 중장년 가요 / 교양 |
| **107.7 MHz** | SBS | 파워FM (Power FM) | 대중가요 / 예능 / 토크 |
| **ONLINE** | YTN | YTN 라디오 | 24시간 뉴스 전문 |

---

## 🚀 iPhone 17 빌드 및 실행 방법

### 방법 A: Xcode에서 바로 열기
1. Mac에 Xcode가 설치되어 있다면 본 폴더를 Xcode에서 엽니다 (`File` > `Open` > `/Users/YOUR_USERNAME/Desktop/radio`).
2. 상단 타겟에서 **iPhone 17** (또는 본인의 실제 아이폰)을 선택합니다.
3. `Signing & Capabilities` 설정:
   - **Team**: 본인의 개인 Apple ID (무료 개발자 계정) 선택
   - **Background Modes**: `Audio, AirPlay, and Picture in Picture` 체크
4. `Cmd + R`을 눌러 실행합니다!

### 방법 B: Swift Package CLI 테스트
```bash
# 전체 테스트 실행
swift test
```

---

## 📁 프로젝트 구조

```
radio/
├── Package.swift                       # Swift Package 매니페스트 (iOS 17+, macOS 14+)
├── Sources/
│   └── RadioApp/
│       ├── App/
│       │   ├── RadioApp.swift          # 앱 엔트리포인트 (@main)
│       │   └── CarPlaySceneDelegate.swift # 카플레이 템플릿 씬 델리게이트
│       ├── Models/
│       │   ├── RadioNetwork.swift      # 방송사 열거형 (KBS, MBC, SBS 등)
│       │   └── RadioStation.swift      # 라디오 채널 데이터 모델
│       ├── Services/
│       │   ├── KoreanStationPresets.swift # 주파수 정렬 국내 방송사 프리셋 & 조회
│       │   ├── AutoScanEngine.swift    # 87.5~108.0MHz 풀 스캔 & 스텝 점프 엔진
│       │   ├── RadioPlayerManager.swift # AVPlayer, 잠금화면, 핸들 리모컨, 오디오 세션
│       │   └── HapticManager.swift     # 햅틱 진동 피드백 매니저
│       ├── ViewModels/
│       │   └── RadioViewModel.swift    # 메인 라디오 뷰모델 & 상태 머신
│       └── Views/
│           ├── MainRadioView.swift     # 메인 화면
│           ├── NowPlayingCard.swift    # 재생/정지, 이전/다음, AUTO 스캔 컨트롤러
│           ├── StationListView.swift   # 채널 리스트 및 파형 애니메이션
│           └── Components/
│               ├── RetroDisplayView.swift # 레트로 LCD 주파수 디스플레이
│               └── FrequencyTunerBar.swift# 아날로그 87.5~108.0MHz 튜닝 바
└── Tests/
    └── RadioAppTests/                  # 단위 테스트 스위트
```
