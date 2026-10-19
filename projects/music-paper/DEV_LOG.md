# 개발일지 — AI MR 분리기

> CLAUDE.md 준수: 단순하게, 필요한 것만, 검증하며 진행

---

## 프로젝트 개요

| 항목 | 내용 |
|------|------|
| 목적 | 음악 파일에서 MR(반주) 분리 + 키 조절 웹앱 |
| 환경 | conda `min` (Python 3.8.13) |
| 핵심 라이브러리 | Demucs (Meta AI), librosa, Flask |
| 작업 폴더 | `/Users/YOUR_USERNAME/Desktop/무제 폴더 6` |

---

## 파이프라인

```
음악 파일 (MP3/MP4/WAV 등)
    ↓
[1] Demucs htdemucs 모델로 보컬/반주 분리
    ├── vocals.wav  → 보컬 트랙
    └── no_vocals.wav → MR 트랙
    ↓
[2] librosa.effects.pitch_shift 로 반음 단위 키 조절
    ↓
[3] Flask 웹앱으로 브라우저에서 업로드/다운로드
```

---

## 기술 결정 사항

| 결정 | 이유 |
|------|------|
| Demucs `--two-stems vocals` | 보컬/반주 2트랙만 분리 → 속도↑, 불필요한 복잡도 제거 |
| Flask (FastAPI 아님) | 단순 파일 업/다운로드 → Flask로 충분 |
| librosa pitch_shift | soundstretch 대비 설치 간단, Python 3.8 호환 |
| threading (Celery 아님) | 단일 사용자 → Celery 오버킬 |

---

## 파일 구조

```
무제 폴더 6/
├── CLAUDE.md           # 개발 가이드라인
├── DEV_LOG.md          # 이 파일
├── app.py              # Flask 서버
├── requirements.txt    # 의존성
├── start.sh            # conda min으로 서버 실행
├── templates/
│   └── index.html      # 프론트엔드
├── uploads/            # 업로드 파일 (자동 생성)
└── outputs/            # 분리 결과 (자동 생성)
```

---

## 개발 로그

### 2026-06-25 — 초기 구현

**구현 계획** (CLAUDE.md §4 Goal-Driven)
```
1. app.py + index.html 작성  → verify: Flask 서버 실행
2. conda min 패키지 설치      → verify: import demucs, librosa, flask
3. 실제 파일 분리 테스트      → verify: 보컬/반주 파일 생성 확인
```

**완료 항목**
- [x] `app.py` — Flask 서버, Demucs Python API 연동, pitch_shift
- [x] `templates/index.html` — 업로드 UI, 진행률, 미리듣기, 키 조절
- [x] `requirements.txt` — 의존성 정의
- [x] `start.sh` — conda min 환경으로 실행
- [x] flask, numpy, werkzeug, soundfile, librosa 설치 완료
- [x] demucs 4.0.1 + torch 2.2.2 설치 완료
- [x] 서버 실행 확인 (port 5001, macOS 5000 AirPlay 충돌로 변경)
- [ ] 실제 음악 파일로 MR 분리 테스트

**주의점**
- Python 3.8.13 → demucs 4.x 호환성 필요
- Demucs 첫 실행 시 모델 자동 다운로드 (~1.5GB)
- ffmpeg 없을 경우 wav 폴백 처리 완료

---

### 2026-06-26 — 기능 추가

- [x] **Pumping 억제**: pedalboard Compressor + Limiter 후처리 추가
  - 보컬 제거 후 볼륨이 올라갔다 내려갔다 하는 현상 완화
- [x] **MP3 다운로드**: pedalboard AudioFile로 WAV→MP3 변환 추가
  - MR/보컬 각각 WAV·MP3 버튼 분리
  - 키 조절 다운로드에 WAV/MP3 포맷 선택 토글 추가
- [x] **속도 최적화**: shifts=0, overlap=0.1, MPS GPU 자동 감지

---

## TODO

- [x] conda min에 패키지 설치 완료
- [x] 서버 실행 및 UI 확인
- [x] 실제 음악 파일로 MR 분리 테스트
- [x] 키 조절 다운로드 동작 확인
- [x] Pumping 아티팩트 억제 (컴프레서)
- [x] MP3 다운로드 지원
- [ ] 실제 음악으로 pumping 개선 효과 검증

---

### 2026-07-14 — AI 악보 보정 및 하이브리드 파이프라인 설계

**구현 계획** (CLAUDE.md §4 Goal-Driven)
```
1. DEV_LOG.md 업데이트 및 설계 완료       → verify: 커밋 완료
2. 프론트엔드 UI 수정 (옵션 체크박스 및 API Key 설정창) → verify: index.html 렌더링 확인
3. 백엔드 app.py 수정 (컴프레서 완화, 하이브리드 분석, Gemini 보정) → verify: MR 분리 및 악보 생성 테스트
4. 예외 처리 검증 (API Key 없을 때 폴백, MuseScore 없을 때 VexFlow 렌더링) → verify: 동작성 검증
```

**완료 항목**
- [x] 악보 품질 이슈 분석 완료 (보컬에서 코드 추출하는 병목, 컴프레서 볼륨 펌핑)
- [x] Gemini AI 보정 및 하이브리드 파이프라인 설계 완료
- [x] 설계 문서 작성 및 커밋 완료 (`docs/superpowers/specs/2026-07-14-ai-music-sheet-calibration-design.md`)

---

### 2026-09-02 — 전수 검사 및 코드 최적화/안정화

**완료 항목**
- [x] **전체 파일 점검 & 의존성 동기화**: `requirements.txt` 및 `install.sh`에 누락되었던 `pedalboard`, `basic-pitch`, `music21`, `google-generativeai` 추가
- [x] **app.py 라우팅 상태 버그 수정**: 악보 생성 후 (`sheet_done` 상태)에도 MR/보컬 다운로드 및 키 조절이 정상 작동하도록 허용 상태 (`done`, `sheet_done`) 개선
- [x] **Gemini AI 응답 파싱 안정화**: 마크다운 코드블록(````json ... ````) 포맷팅 유입 시 안전하게 JSON만 추출/파싱하는 방어 로직 추가
- [x] **미사용 및 중복 코드 정리**: 미사용 `build_two_part_score` 정리 및 중복 except 블록 제거, `index.html` 내 중복 CSS 제거
- [x] **악보 메타데이터 및 클라이언트 PDF 내보내기 강화**: 악보 생성 시 감지된 조성(Key), BPM, 코드 진행(Chords) UI 뱃지 표시 및 MuseScore 미설치 환경에서도 웹 뷰어에서 PDF를 바로 내보낼 수 있도록 jsPDF 연동 지원
- [x] **conda min 환경 검증 완료**: 전체 구문 및 모듈 로딩 0 에러 검증 완료

