> 정리 전 문서입니다. 상대 경로와 실행 안내는 이전 위치 기준이며, 기능·검증 주장은 당시 기록입니다. 현재 안내는 프로젝트 README와 루트 README를 참고하세요.

# X Cleaner

프로젝트를 개인용과 배포용으로 분리했습니다.

- `personal/`: Python 화면 앱, 기존 CLI, 테스트, 예시, 작업 기록. [개인용 안내](personal/README.md)
- `deploy/`: React 웹판, Vercel API, 보안 테스트. [웹판 안내](deploy/README.md)

개인용은 Finder에서 `personal/X Cleaner.app`을 열어 실행하세요. API 설정, X 로그인, 파일 선택과 최종 확인을 화면에서 진행합니다.

개인용 실행:

```bash
conda activate min
cd ~/Desktop/x-cleaner/personal
python -m x_cleaner gui
```

선택한 본인 게시물·댓글만 삭제하고, 선택한 리포스트만 취소합니다. 기본 실행은 미리보기입니다. 실제 X 계정에 대한 삭제는 개발·검증 과정에서 실행하지 않았습니다.

웹판 로컬 실행:

```bash
conda activate min
cd ~/Desktop/x-cleaner/deploy
npm run dev
```

REDACTED_CONFIG_VALUE 에 접속합니다. 기본은 실제 실행 비활성화이며 공개 Vercel 배포와 실제 X 인증은 아직 검증하지 않았습니다.

기존 `.venv`는 보관용이며 Python 실행에는 Conda `min`을 사용합니다. Node 의존성은 `deploy/node_modules/`에 설치합니다.
