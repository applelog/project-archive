<div align="center">

# SIDE PROJECT LAB

### 작은 시도들의 기록 · 구현과 한계를 함께 남기는 개발 포트폴리오

![Projects](https://img.shields.io/badge/PROJECTS-09-58a6ff?style=for-the-badge)
![Focus](https://img.shields.io/badge/FOCUS-BUILD%20%26%20REFLECT-a371f7?style=for-the-badge)
![Status](https://img.shields.io/badge/STATUS-EXPERIMENTAL-d29922?style=for-the-badge)

![Swift](https://img.shields.io/badge/Swift-F05138?style=flat-square&logo=swift&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?style=flat-square&logo=python&logoColor=white)
![TypeScript](https://img.shields.io/badge/TypeScript-3178C6?style=flat-square&logo=typescript&logoColor=white)
![React](https://img.shields.io/badge/React-61DAFB?style=flat-square&logo=react&logoColor=20232A)
![Go](https://img.shields.io/badge/Go-00ADD8?style=flat-square&logo=go&logoColor=white)
![C](https://img.shields.io/badge/C-00599C?style=flat-square&logo=c&logoColor=white)

[프로젝트 인덱스](#프로젝트-한눈에-보기) · [구현과 한계](#프로젝트별-기록) · [기술적 회고](#여러-프로젝트에서-드러난-과제) · [출처와 권리](THIRD_PARTY_NOTICES.md)

</div>

[English](README.md) · **한국어**

---

직접 겪은 불편함과 관심사에서 출발한 서브 프로젝트를 모은 개발 포트폴리오입니다. 완성된 서비스만 전시하기보다, 무엇을 시도했고 어디까지 구현했으며 어떤 한계가 남았는지를 정리합니다.

> 정리 기준: 2026-10-19 로컬 소스와 개발 기록. 아래의 “구현”은 코드가 존재한다는 뜻이며, 실제 서비스 운영이나 모든 기능의 실행 성공을 뜻하지 않습니다. 이번 작업은 공개용 사본 정리이며 전체 앱을 다시 실행한 검증은 아닙니다.

## Codex와 보낸 9월

2026년 9월 한 달 동안 Codex를 사용해 보았습니다. 아래 애니메이션은 첨부한 토큰 활동 화면의 색상 패턴을 재구성한 것입니다. 사용 기록의 시각화이며 정확한 토큰 수나 생산성 지표는 아닙니다.

![One month with Codex: animated activity illustration](assets/codex-september.svg)

[정적 이미지](assets/codex-september-static.svg) · [시각화 데이터](data/codex-activity-visual.json)

## 프로젝트 한눈에 보기

| 프로젝트 | 주요 기술 | 핵심 시도 |
|---|---|---|
| [이동권 길찾기](projects/mobility-guide/) | Swift · SwiftUI · Python | 이동 조건에 따라 보행 경로를 선택할 수 있는 길찾기를 만들고자 했다. |
| [음악 분리와 악보 변환](projects/music-paper/) | Python · Flask · Demucs | 음악에서 반주를 분리하고 조성을 바꾸거나 악보로 옮기는 작업을 돕고자 했다. |
| [선택 항목 기반 X Cleaner](projects/x-cleaner/) | Python · React · Node.js | 본인이 선택한 게시물과 리포스트만 정리할 수 있는 도구를 만들고자 했다. |
| [시간표 마법사](projects/school-timetable/) | React · TypeScript · Vite | 강의 정보를 조합해 겹치지 않는 시간표 후보를 비교하고자 했다. |
| [한국 라디오](projects/radio/) | Swift · SwiftUI · AVPlayer | 라디오 채널을 아날로그 튜너 형태로 탐색하는 앱을 만들고자 했다. |
| [Dev Desk](projects/dev-desk/) | SwiftUI · Charts · IOKit | 개발 지침과 도구 사용 상태, 기기 정보를 한 화면에 모으고자 했다. |
| [나의 불편함](projects/my-inconveniences/) | Go · Bubble Tea · Lip Gloss | 개발 환경 제어와 시스템 상태 확인을 터미널에서 처리하고자 했다. |
| [위기 지표 분석 실험](projects/korea-crisis-watch-ai/) | React · TypeScript · Gemini | 여러 경제 지표와 AI 요약을 한 화면에서 살펴보는 실험이다. |
| [1,474,560 byte 게임 실험](projects/life-in-144mb/) | C · raylib · Python | 작은 실행 파일 크기 제약 안에서 기억과 삶을 소재로 게임을 구성하고자 했다. |

## 프로젝트별 기록

### 이동권 길찾기

**문제와 목적** — 이동 조건에 따라 보행 경로를 선택할 수 있는 길찾기를 만들고자 했다.

**구현한 내용** — 보행망 변환, A* 탐색, 계단·경사 제약, 지도와 경로 상세 화면을 구현했다.

**한계** — 경사 자료는 지형 추정값이며 현장 검증과 다르다. 보행·버스·지하철을 잇는 전체 여정은 아직 연결되지 않았다. 대용량 network.json은 이 사본에서 제외했다.

**다음에 개선할 점** — 접근성은 경로를 계산하는 것만으로 증명할 수 없다. 다음 단계는 현장 자료와 역내 연결, 실제 저상 차량 정보를 확인하는 것이다.

[코드와 실행 안내](projects/mobility-guide/README.md) · [대표 구현](projects/mobility-guide/Core/Router.swift)

### 음악 분리와 악보 변환

**문제와 목적** — 음악에서 반주를 분리하고 조성을 바꾸거나 악보로 옮기는 작업을 돕고자 했다.

**구현한 내용** — 보컬·반주 분리, 피치 조절, 음원 미리듣기, MIDI·PDF 변환 경로와 Gemini 보정 연결 코드를 구현했다.

**한계** — 악보 정확도와 분리 음질의 정량 평가는 없다. 모델 실행 비용과 외부 도구 의존성이 크며 공개 서버 운영을 검증하지 않았다.

**다음에 개선할 점** — 여러 모델을 연결하는 일과 결과의 음악적 정확도를 보장하는 일은 다르다. 평가 음원과 비교 기준을 먼저 마련해야 한다.

[코드와 실행 안내](projects/music-paper/README.md) · [대표 구현](projects/music-paper/app.py)

### 선택 항목 기반 X Cleaner

**문제와 목적** — 본인이 선택한 게시물과 리포스트만 정리할 수 있는 도구를 만들고자 했다.

**구현한 내용** — 개인용 GUI·CLI와 웹판, 아카이브 읽기, 선택 검증, 미리보기, OAuth 및 실행 요청 로직을 구현했다.

**한계** — 기존 검증 기록은 모의 API 중심이다. 실제 X 인증·삭제와 공개 배포가 성공했다고 볼 수 없다.

**다음에 개선할 점** — 삭제 도구에서는 기능 수보다 선택 범위와 오류 후 상태를 명확히 하는 일이 중요하다. 실계정 검증은 별도의 명시적 승인 아래 수행해야 한다.

[코드와 실행 안내](projects/x-cleaner/README.md) · [대표 구현](projects/x-cleaner/personal/x_cleaner/workflow.py)

### 시간표 마법사

**문제와 목적** — 강의 정보를 조합해 겹치지 않는 시간표 후보를 비교하고자 했다.

**구현한 내용** — 텍스트 파서, 충돌 없는 조합 탐색, 필터와 시간표 화면, 파서·탐색 테스트를 작성했다.

**한계** — 입력 형식 변화와 조합 수 증가에 대한 추가 검증이 필요하다. 원래 README의 기술 버전·테스트 설명 일부는 현재 package.json과 달라 정리했다.

**다음에 개선할 점** — 정확한 입력 해석과 탐색 비용을 함께 다뤄야 한다. 다양한 실제 입력을 익명화한 회귀 사례가 다음 과제다.

[코드와 실행 안내](projects/school-timetable/README.md) · [대표 구현](projects/school-timetable/src/utils/scheduler.ts)

### 한국 라디오

**문제와 목적** — 라디오 채널을 아날로그 튜너 형태로 탐색하는 앱을 만들고자 했다.

**구현한 내용** — 채널 프리셋, 탐색·재생 상태 관리, 곡 정보와 기록, CarPlay 연결 코드를 작성했다.

**한계** — 방송 스트림의 현재 가용성, 실제 CarPlay 환경과 장시간 재생은 이번에 검증하지 않았다. 방송 콘텐츠의 권리는 별도다.

**다음에 개선할 점** — 화면과 재생 코드를 갖추는 것 외에 외부 스트림 변화와 차량 환경에서의 검증이 필요하다.

[코드와 실행 안내](projects/radio/README.md) · [대표 구현](projects/radio/Sources/RadioApp/Services/RadioPlayerManager.swift)

### Dev Desk

**문제와 목적** — 개발 지침과 도구 사용 상태, 기기 정보를 한 화면에 모으고자 했다.

**구현한 내용** — 지침 저장·편집, 묶음 관리, 로컬 사용 기록 읽기와 시스템 정보 표시 기능을 구현했다.

**한계** — 로컬 기록에 나타나는 값과 계정의 공식 전체 사용량은 다르다. 기기·운영체제 환경에 따른 수집 차이가 있다.

**다음에 개선할 점** — 수치의 출처와 관측 범위를 함께 보여주는 것이 중요하다. 개인 데이터는 공개용 사본에서 제외했다.

[코드와 실행 안내](projects/dev-desk/README.md) · [대표 구현](projects/dev-desk/Sources/Services.swift)

### 나의 불편함

**문제와 목적** — 개발 환경 제어와 시스템 상태 확인을 터미널에서 처리하고자 했다.

**구현한 내용** — 터미널 화면, 시스템 정보 수집, 플러그인 관리 모듈과 테스트를 작성했다.

**한계** — macOS 명령과 로컬 플러그인 경로에 의존한다. 설계 문서의 기능 전체를 실제 연동 성공으로 간주할 수 없다.

**다음에 개선할 점** — 개인 환경에 맞춘 도구를 다른 사람도 쓰게 하려면 경로·설정과 측정 방식을 분리해야 한다.

[코드와 실행 안내](projects/my-inconveniences/README.md) · [대표 구현](projects/my-inconveniences/main.go)

### 위기 지표 분석 실험

**문제와 목적** — 여러 경제 지표와 AI 요약을 한 화면에서 살펴보는 실험이다.

**구현한 내용** — Gemini 검색 도구 호출, JSON 응답 처리, 위험 지표 카드와 브라우저 기록 저장을 구현했다.

**한계** — 위험 점수의 검증된 산식이 없고 모델 생성 결과에 의존한다. 브라우저 API 키 사용 구조는 공개 서비스 전에 변경해야 한다. 응답 스키마 검증도 보강해야 한다.

**다음에 개선할 점** — AI가 수치를 생성하는 것과 믿을 수 있는 지표 서비스를 제공하는 것은 다르다. 공식 데이터 수집과 산식·출처 검증이 선행 과제다.

[코드와 실행 안내](projects/korea-crisis-watch-ai/README.md) · [대표 구현](projects/korea-crisis-watch-ai/services/geminiService.ts)

### 1,474,560 byte 게임 실험

**문제와 목적** — 작은 실행 파일 크기 제약 안에서 기억과 삶을 소재로 게임을 구성하고자 했다.

**구현한 내용** — 두 개발 버전의 장면·시나리오, 렌더링, 오디오 합성, 글꼴 생성과 크기 확인 스크립트를 남겼다.

**한계** — 실행 파일의 최신 크기와 완주 경험은 이번에 재검증하지 않았다. 포함된 글꼴 데이터의 출처·배포 조건 확인이 필요하다.

**다음에 개선할 점** — 용량 제약은 표현 방식에 영향을 준다. 글꼴 권리 정리와 재현 가능한 빌드, 플레이 검증을 함께 해야 한다.

[코드와 실행 안내](projects/life-in-144mb/README.md)

## 여러 프로젝트에서 드러난 과제

- **기능 구현과 실제 검증의 차이:** UI, 테스트, 외부 API, 실제 기기·이용 환경은 각각 따로 확인해야 합니다.
- **데이터의 출처와 한계:** 경사 추정값, AI가 만든 위험 점수, 로컬 사용 기록을 검증된 사실처럼 표시하지 않아야 합니다.
- **개인 도구에서 공유 가능한 프로젝트로:** 로컬 경로, 인증정보, 환경 의존성, 실행 안내를 정리해야 재현할 수 있습니다.

이 항목은 소스와 기록을 검토해 정리한 기술적 회고입니다. 개인의 실제 경험이나 성과를 새로 만들어 넣지 않았습니다.

## 저장소 구성과 범위

`projects/` 아래 각 프로젝트를 독립적으로 보관합니다. 한 번에 모든 앱을 빌드하는 통합 서비스가 아닙니다. 기존 Git 이력은 포함하지 않으며 현재 작업 파일의 사본입니다. 하위 `.github/workflows/`는 참고용으로 보관하며 이 저장소의 루트 워크플로로 활성화하지 않았습니다.

인증 설정, 개인 기록·시간표·사진, 음원과 생성 결과, 설치 패키지·캐시, 대용량 GIS와 보행망은 제외했습니다. 따라서 모든 프로젝트가 복제 즉시 실행되는 구성은 아닙니다. 특히 이동권 길찾기는 보행망 데이터를 별도로 준비해야 합니다. 게임의 글꼴 데이터와 출처 미확인 이미지는 제외했으며, 게임 빌드와 라디오 아이콘 자산은 복원 또는 교체가 필요합니다.

## 저작권과 출처

Copyright (c) 2026 Kim Kangwoo (applelog). 본인에게 권리가 있는 코드 및 문서에 한합니다. 외부 라이브러리·글꼴·데이터·방송 콘텐츠의 권리는 각 권리자에게 있습니다.

기존 `school-timetable/LICENSE`의 MIT 고지는 유지합니다. 나머지 프로젝트에 새로운 오픈소스 라이선스를 일괄 적용하지 않았습니다. [외부 자료 및 공개 전 확인 사항](THIRD_PARTY_NOTICES.md)을 확인하세요.
