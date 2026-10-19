# 이동권 길찾기 — 개봉역 시범 Implementation Plan

> **For agentic workers:** Use the existing approved product plan and implement inline. No subagents are used in this workspace.

**Goal:** iPhone에서 개봉동 현재 위치부터 개봉역 엘리베이터까지 경로를 계산하고, 공식 자료가 있을 때만 1호선 승강장 단계를 안내한다.

**Architecture:** 서울시 공식 보행망과 엘리베이터 위치를 개발 시점에 JSON으로 변환한다. 앱은 SwiftUI/MapKit/Core Location으로 표시하고 경로는 로컬 그래프에서 계산한다. 철도 공식 단계 자료가 없으면 실내 안내를 비활성화한다.

**Tech Stack:** Swift 6, SwiftUI, MapKit, Core Location, Python 표준 라이브러리, Xcode.

---

### Task 1: 경로 계산

**Files:** `Core/RouteModel.swift`, `Core/Router.swift`, `Tests/main.swift`

- [ ] 실측 경사 초과와 계단/에스컬레이터를 배제하고 미확인 구간을 표시하는 실패 테스트를 실행한다.
- [ ] 가장 가까운 보행 링크에서 출발하는 최소 경로 계산을 구현한다.
- [ ] 테스트를 다시 실행해 통과를 확인한다.

### Task 2: 공식 데이터 가져오기

**Files:** `Tools/import_seoul.py`, `Tools/import_kric.py`, `Data/network.json`, `Data/station_steps.json`

- [ ] 서울 API의 링크/노드/엘리베이터 필드를 검증하고 연결된 그래프만 저장한다.
- [ ] 철도 API의 출입구→승강장 단계를 방향별로 정렬하고 자료가 없으면 빈 상태를 유지한다.
- [ ] 키 누락, 잘못된 응답, 연결되지 않은 엘리베이터를 검사한다.

### Task 3: iPhone 앱

**Files:** `App/MobilityGuideApp.swift`, `App/ContentView.swift`, `App/LocationService.swift`, `MobilityGuide.xcodeproj/project.pbxproj`

- [ ] 위치 권한, 경사 선호, 열차 방향, 지도 경로 및 대안, 출처/확인 상태를 표시한다.
- [ ] 실외 경로 이탈 시 재계산하고 GPS가 부정확하면 안내를 중지한다.
- [ ] 공식 철도 자료가 있을 때만 실내 단계 안내를 표시한다.
- [ ] iOS 시뮬레이터 빌드와 핵심 로직 테스트를 실행한다.

### Task 4: 사용 및 검증 문서

**Files:** `README.md`, `programming_instructor.md`

- [ ] API 키 주입, 데이터 갱신, 현장 확인, 실제 휠체어 사용자 검증 전 공개 제한을 문서화한다.
- [ ] 적용한 프로젝트 지침과 확인되지 않은 사항을 구분해 기록한다.
