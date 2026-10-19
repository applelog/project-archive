> 정리 전 문서입니다. 상대 경로와 실행 안내는 이전 위치 기준이며, 기능·검증 주장은 당시 기록입니다. 현재 안내는 프로젝트 README와 루트 README를 참고하세요.

# Dev Desk

Codex·Antigravity CLI를 사용하는 개인용 macOS 개발 대시보드. 지침 검색·편집·묶음 복사, 로컬 스킬 열람, Codex 사용량·한도, 배터리·CPU·메모리, 구독 일정을 한 창에서 확인합니다.

## 실행

macOS 26과 Xcode가 필요합니다. 이 폴더에서 `zsh build.sh`를 실행한 뒤 `.build/Dev Desk.app`을 엽니다. 앱이 이미 열려 있다면 종료한 후 다시 열어야 새 빌드가 적용됩니다. 빌드와 앱이 직접 저장하는 파일은 모두 이 폴더 안에 생성됩니다.

```text
.
├── Sources/                 SwiftUI 화면, 저장 및 수집 코드
├── Resources/Info.plist     앱 번들 설정
├── Tests/StorageChecks.swift 저장·충돌 감지 자체 검사
├── build.sh                 앱 빌드
├── data/library/            지침 Markdown (첫 실행 후 생성)
├── data/history/            수정 전 버전과 삭제 항목
├── data/bundles.json        지침 묶음 (저장 후 생성)
├── data/subscriptions.json  구독 정보 (수정 후 생성)
└── .build/                  앱과 컴파일 결과
```

지침은 앱의 **지침 → +**에서 만들 수 있습니다. **AI에게 지침 추가 요청 복사** 버튼으로 저장 형식과 경로를 전달할 수 있습니다. AI가 추가할 때에는 이 폴더의 `data/library/`에 UUID를 파일 이름으로 한 UTF-8 Markdown 파일을 작성하도록 요청하세요. 첫 줄은 `# 제목`, 그 아래는 지침 본문입니다. 앱은 새 파일을 자동으로 다시 읽습니다.

Codex 수치는 기존 `~/.codex/sessions` 기록을 읽어 표시하며 최근 30일 로컬 세션에 한정됩니다. Antigravity는 현재 로컬 대화 파일 수와 최근 변경 시각을 표시합니다. 구독 한도는 공식 CLI의 `/usage`에서 확인할 수 있고, 앱으로 전달되는 연결을 설정하기 전까지 앱에는 숫자를 표시하지 않습니다. 스킬의 ‘파일 발견’은 실행 성공을 의미하지 않습니다.

Google AI Pro 결제 일정은 사용자 제공 화면의 2026-09-24 기준 정보입니다. 구독 카드에서 수정할 수 있습니다. ChatGPT Plus 결제액과 날짜는 입력 전까지 공란입니다. 배터리 잔여 시간은 운영체제 추정값이며, 방전 전력은 지원되는 센서에서만 표시됩니다.

저장 로직과 현재 맥의 수집 연동 확인: `zsh test.sh`
