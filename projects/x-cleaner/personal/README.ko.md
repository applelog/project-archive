# X Cleaner — 선택한 게시물·댓글·리포스트만 정리

Mac에서 실행하는 로컬 Python 터미널 프로그램입니다.

**선택한 내 게시물·댓글(답글)만 삭제하고, 선택한 리포스트만 취소합니다. 선택하지 않은 항목은 대상에 추가하지 않습니다.** 보존 목록을 제외한 나머지 전체 삭제 기능은 없습니다. 좋아요, DM, 팔로우 관계를 변경하는 API도 없습니다.

기본 실행은 오프라인 미리보기입니다. `auth`는 인증, `verify`는 유료일 수 있는 조회만 수행합니다. 삭제는 `run --execute`와 확인 문구 입력이 모두 있어야 시작합니다.

## 화면 앱으로 실행 — 권장

Finder에서 `personal/X Cleaner.app`을 열거나 `개인용 앱 실행.command`를 더블클릭하세요. `conda min`을 자동으로 활성화합니다. 터미널에서는 다음 명령으로 열 수 있습니다.

```bash
conda activate min
cd ~/Desktop/x-cleaner/personal
python -m x_cleaner gui
```

‘API 설정 및 X 로그인’ 탭에서 Client ID, 필요한 경우 Client Secret, 콜백 주소를 저장하고 X 공식 화면에서 로그인합니다. `.env`를 편집하지 않아도 됩니다. 설정은 Git 제외된 `.secrets/app-settings.json`에 권한 600으로 저장하고 기존 CLI도 같은 설정을 사용합니다. 빈 Secret 입력은 저장된 값을 유지하며, 별도 체크로 지울 수 있습니다.

‘기록 선택 및 실행’ 탭에서 파일 선택 창으로 ZIP/폴더를 읽고 목록의 선택 칸 또는 Space로 항목을 고릅니다. 경로와 게시물 ID를 타이핑하지 않아도 됩니다. 보존 목록은 선택 사항입니다.

미리보기 저장 → 계정·대상 검증 → 최종 확인 후 실행 순서입니다. 검증은 유료일 수 있는 조회만 수행하며, 실제 실행은 확인 문구를 정확히 입력해야 시작합니다. 선택·보존 변경 시 이전 미리보기를 사용할 수 없습니다. 실행 중에는 선택과 설정을 잠그며 중단 버튼은 다음 요청을 차단합니다. 이전 작업은 폴더 선택 창으로 열어 이어갑니다. 실패·미확정 항목은 결과를 확인한 뒤 별도 체크로 명시적으로 재시도합니다.

X 개발자 콘솔에서 앱 생성, 콜백 등록, 크레딧 결제는 X 공식 화면에서 해야 합니다. 현재 목록은 아카이브에서 읽으며, 로그인만으로 전체 과거 기록을 가져오는 기능은 아직 없습니다. 화면 생성은 `min`에서 확인했지만 실제 X 인증·삭제는 실행하지 않았습니다. `.app`은 현재 Mac의 Conda를 사용하는 런처이며 다른 Mac에 배포할 서명된 독립 실행 파일은 아닙니다.

## 파일과 실행 순서

```text
x-cleaner/
├── x_cleaner/             아카이브 파싱, 인증, API, 선택·보존 검사, 기록, CLI
├── tests/                 실제 네트워크를 차단한 모의 테스트
├── examples/              가짜 아카이브와 선택 CSV 예시
├── requirements.txt       실행 의존성
├── requirements-dev.txt   테스트 의존성
├── .env.example           비밀값 없는 설정 예시
├── .gitignore
├── archives/              내 실제 아카이브를 넣을 위치(Git 제외)
├── jobs/                  미리보기·실행 기록(Git 제외)
└── .secrets/              인증 토큰(Git 제외)
```

순서: **아카이브 받기 → 목록 검색 → 삭제할 항목 선택 → 미리보기 → 인증 → 온라인 검증 → 최종 목록 확인 → 삭제 실행**.

프로젝트 파일과 출력은 이 폴더 안에만 생성합니다. 입력 아카이브는 다른 폴더의 파일도 읽을 수 있으며 원본을 수정하지 않습니다.

## 1. 개발 환경

현재 실행 환경은 **Conda `min` / Python 3.8.13**입니다. 이 환경에서 기존 111개 모의 테스트가 통과했습니다.

```bash
conda activate min
cd ~/Desktop/x-cleaner/personal
python -m pip install -r requirements-dev.txt
python -m x_cleaner --help
```

실행만 필요하면 `requirements.txt`만 설치해도 됩니다. 사용자 요청에 따라 `conda activate min` 환경을 사용합니다. 기존 `.venv`는 보관되어 있으며 실행에 사용하지 않습니다.

## 2. X 아카이브 받기

X의 **설정 및 개인정보 → 내 계정 → 내 데이터 아카이브 다운로드**에서 본인 확인 후 아카이브를 요청하고 ZIP을 다운로드합니다. 화면 이름은 X에서 변경될 수 있습니다. [공식 다운로드 안내](https://help.x.com/en/managing-your-account/how-to-download-your-x-archive)

본인 확인은 **X 공식 화면에서만** 진행합니다. 이 프로그램에는 비밀번호나 브라우저 쿠키를 입력하지 않습니다.

예를 들어 ZIP을 `archives/my-x-archive.zip`에 저장하거나, 압축을 푼 폴더를 `archives/my-x-archive/`에 놓으세요. 아카이브는 다운로드 당시의 스냅샷이므로 이후 작성한 게시물은 자동으로 삭제 대상이 되지 않습니다.

지원 형식은 `data/account.js`, `data/tweet.js` 또는 `data/tweets.js`, `tweet(s)-part0.js`부터 이어지는 분할 파일 및 같은 이름의 JSON 배열입니다. 파일이 ZIP 최상위 폴더 안에 있어도 읽습니다. JavaScript를 실행하지 않고 JSON 배열만 파싱합니다.

첫 파일이 `tweets.js`이고 다음 파일이 `tweets-part1.js`인 구성도 지원합니다. `summary.json`의 `execution_ready`는 온라인 검증 완료와 차단 사유 없음을 뜻하며, 실제 실행에서는 현재 계정·상태와 확인 입력을 추가로 검사합니다.

- 게시물/계정 ID는 문자열로 유지합니다. JSON 정수도 정확하게 읽고 실수·지수 표기 ID는 거부합니다.
- 발견된 분할 파일 번호의 중간 누락과 중복을 차단합니다. `tweet-headers.js`가 있으면 본문 ID 목록과 비교합니다.
- 헤더/목록 정보가 없는 아카이브에서는 **마지막 분할 파일의 누락까지 증명할 수 없습니다**. 완전한 원본 ZIP을 사용하세요. 선택 ID를 찾지 못하면 삭제를 차단합니다.
- 새로운/지원하지 않는 형식, 충돌하는 ID, 여러 데이터 폴더, 위험한 ZIP 경로와 심볼릭 링크는 차단합니다. 데이터 파일당 256 MiB, 관련 파일 합계 1 GiB 읽기 제한이 있습니다.
- `account.js`가 없어도 미리보기는 가능하지만, 소유 계정 ID를 확인할 수 없어 실제 실행은 차단합니다.

## 3. 목록에서 삭제할 항목 찾기 — API 호출 없음

```bash
python -m x_cleaner list \
  --archive archives/my-x-archive.zip \
  --output jobs/all-posts.csv
```

댓글만, 또는 특정 본문을 검색할 수도 있습니다.

```bash
python -m x_cleaner list \
  --archive archives/my-x-archive.zip \
  --kind reply --search "검색할 문구" \
  --output jobs/replies.csv
```

`--kind`는 `all`, `post`, `reply`, `repost` 중 하나입니다. 아카이브에 참조 정보가 없고 본문이 `RT @...:`로 시작하면 리포스트 **추정**으로 표시합니다. 정확한 종류는 나중에 공식 API로 검증합니다.

CSV의 ID 앞 작은따옴표(`'`)는 스프레드시트에서 큰 숫자의 정밀도를 보호하기 위한 표시입니다. 입력할 때 작은따옴표를 빼고 **숫자 ID 또는 X 링크**를 사용하세요. 원래 ID 문자열은 작업의 JSON 파일에도 저장됩니다. 본문이 수식으로 실행되지 않도록 위험한 첫 문자도 보호합니다.

## 4. 선택한 항목 미리보기 — API 호출 없음

아래 숫자/링크는 예시이며 본인의 항목으로 교체하세요.

```bash
python -m x_cleaner preview \
  --archive archives/my-x-archive.zip \
  --delete-post "https://x.com/myname/status/게시물ID" \
  --delete-reply "댓글ID" \
  --delete-repost "https://x.com/original_author/status/원본게시물ID" \
  --job jobs/my-selection
```

- `--delete-post`: 내 게시물 선택. 내 답글과 인용 게시물도 선택할 수 있습니다.
- `--delete-reply`: 내 댓글/답글만 선택. 일반 게시물이나 다른 사람의 댓글이면 검증을 통과하지 못합니다.
- `--delete-repost`: **내 리포스트 취소** 선택. 원본 링크/ID 또는 아카이브에 기록된 내 리포스트 항목 ID를 입력합니다. 다른 사람의 원본을 삭제하지 않습니다.
- 각 옵션은 여러 번 쓸 수 있습니다. 선택이 비어 있으면 차단합니다.
- `--job`을 생략하면 `jobs/날짜-고유번호` 폴더를 생성합니다. 기존 작업 폴더를 덮어쓰지 않습니다.

리포스트 원본 ID가 아카이브에 없으면 오프라인 단계는 차단/미확정으로 표시할 수 있습니다. `verify`에서 아카이브의 리포스트 항목을 조회해 원본 관계를 확인합니다. 원본 링크로 일치 항목을 찾으려면 아카이브의 다른 리포스트까지 조회할 수 있어 비용이 늘어납니다. 목록에 나온 **내 리포스트 항목 ID**로 선택하면 조회 범위를 줄일 수 있습니다. 관계를 끝내 확인하지 못하면 실행을 차단합니다.

### 여러 항목을 CSV로 선택

`selection.csv` 파일을 프로젝트 안에 만들고 아래 형식으로 입력합니다. 예시는 `examples/selection.example.csv`에도 있습니다.

```csv
kind,value
post,게시물ID
reply,https://x.com/myname/status/댓글ID
repost,https://x.com/author/status/원본ID
```

```bash
python -m x_cleaner preview \
  --archive archives/my-x-archive.zip \
  --selection-file selection.csv \
  --job jobs/my-selection
```

### 선택 실수에 대비한 추가 보존 목록

`--keep 링크또는ID`를 여러 번 지정하거나 `--keep-file keep.txt`로 한 줄당 하나씩 입력할 수 있습니다. **삭제 선택과 보존이 충돌하면 보존이 우선**입니다. 입력한 보존 ID가 아카이브에서 확인되지 않으면 다른 항목의 실제 삭제도 모두 차단합니다.

미리보기 결과는 다음 파일에 저장됩니다.

| 파일 | 내용 |
|---|---|
| `summary.json` | 전체·선택·보존·선택 밖 수, 게시물/댓글 삭제·리포스트 취소 수, 차단 이유, 실행 상태 |
| `targets.json` | 문자열 ID를 보존한 요청 대상, 수정 이력과 상태 |
| `targets.csv` | 사람이 검토할 대상 목록 |
| `input.json` | 고정된 아카이브 스냅샷과 선택·보존 목록 |
| `plan.json` | 검증한 종류·계정·원본·수정 이력 및 최종 요청 계획 |
| `state.sqlite3` | 이어하기의 기준이 되는 실행 기록 |
| `events.jsonl` | 시간순 상태 기록. 토큰/인증 헤더/응답 원문은 저장하지 않음 |

보고서 파일이나 DB를 수동 수정해서 대상을 바꾸지 마세요. 선택 변경은 **새 preview 작업**으로 진행합니다. 입력/계획 지문이 기록과 다르거나 원본 아카이브가 바뀌면 실행을 차단합니다. 지문은 실수/변경 검사용이며, 같은 Mac 계정 권한으로 파일과 DB를 모두 고치는 공격까지 막는 서명은 아닙니다.

## 5. 공식 API 접근·요금·인증 설정

공식 문서 확인일: **2026-10-06**.

X API는 **선불 크레딧 종량제**입니다. 무료라고 가정하지 않습니다. [공식 요금 안내](https://docs.x.com/x-api/getting-started/pricing)

- 게시물 조회 표준 가격은 반환 리소스당 **USD 0.005**, 사용자 조회는 리소스당 **USD 0.010**으로 안내되어 있습니다.
- 일반 삭제와 리포스트 취소의 **정확한 엔드포인트별 단가·접근 가능 여부는 내 Developer Console에서 확인**하세요. 공개 요금표의 작업 분류만으로 둘의 단가를 단정하지 않습니다.
- `verify`뿐 아니라 `run --execute`와 이어하기도 계정과 현재 대상 상태를 조회합니다. 긴 작업 중 토큰 갱신 뒤에도 계정을 확인합니다.
- 같은 날 조회가 중복 과금되지 않을 수 있지만, X 문서는 중복 제거를 완전히 보장하지 않습니다. 프로그램은 과금 조회를 무료라고 취급하지 않습니다.
- Console에서 크레딧 잔액과 지출 상한을 설정하세요. 프로그램이 자동 충전이나 결제 설정을 바꾸지 않습니다.

### 개발자 앱

1. [X Developer Console](https://console.x.com/)에서 개발자 접근을 준비하고 앱을 만듭니다.
2. OAuth 2.0 사용자 인증을 활성화하고 앱을 **Native App/public client**로 설정하는 것을 권장합니다.
3. 콜백 URL을 **`http://127.0.0.1:8765/callback`**으로 정확히 등록합니다. `localhost`와 `127.0.0.1`은 같은 문자열이 아닙니다.
4. OAuth 2.0 **Client ID**를 확인합니다. 앱 전체용 Bearer Token은 이 프로그램의 사용자 인증 토큰을 대신하지 못합니다.
5. 프로젝트 안에 설정을 준비합니다.

```bash
cp .env.example .env
chmod 600 .env
```

`.env`의 `X_CLIENT_ID`를 채우세요. Native App이면 `X_CLIENT_SECRET`은 비워 둡니다. Web App/Automated App처럼 confidential client로 만든 경우에만 Client Secret도 설정합니다. 환경변수로 지정한 값이 `.env`보다 우선합니다.

```bash
python -m x_cleaner auth
```

X 공식 승인 화면에서 **아카이브의 소유 계정**으로 승인하세요. 프로그램은 PKCE S256, 무작위 state와 5분의 콜백 대기 시간을 사용합니다. 로컬 서버는 `127.0.0.1`에만 바인딩합니다. 콜백 주소·승인 코드의 서버 로그를 출력하지 않습니다.

요청 권한은 `tweet.read`, `users.read`, `tweet.write`, `offline.access`입니다. 게시물 쓰기 권한에는 생성 기능도 포함되지만 이 프로그램은 생성 API를 구현하지 않습니다. 좋아요·DM·팔로우 쓰기 권한은 요청하지 않습니다. [공식 인증 방식과 권한](https://docs.x.com/fundamentals/authentication/oauth-2-0/authorization-code), [PKCE 연결 절차](https://docs.x.com/fundamentals/authentication/oauth-2-0/user-access-token)

토큰은 `.secrets/tokens.json`에 저장하며 폴더 권한은 `700`, 파일 권한은 `600`입니다. **파일 권한으로 보호한 평문 저장**이므로 Mac 계정 자체의 보안을 유지하세요. 접근/갱신 토큰, 앱 시크릿을 채팅·Git·공유 폴더에 복사하지 마세요. 토큰은 만료 전에 갱신하고 회전된 갱신 토큰을 원자적으로 저장합니다. 인증 실패 시 반복하지 않고 중단합니다.

사용 후 X 설정의 연결된 앱 화면에서 승인을 취소할 수 있습니다. 앱/계정을 바꾸면 `auth`를 다시 실행하세요.

## 6. 온라인 검증 — 조회만 수행

```bash
python -m x_cleaner verify --job jobs/my-selection
```

이 명령은 비용이 발생할 수 있는 **공식 API 조회**를 사용하지만 삭제 요청은 보내지 않습니다.

- `GET /2/users/me`의 계정 ID와 아카이브의 계정 ID가 같아야 합니다. 사용자명 변경만으로 불일치로 판단하지 않습니다.
- 선택한 항목의 작성자·종류, 리포스트의 원본 ID와 수정 이력을 확인합니다. 다른 사람의 댓글은 삭제할 수 없습니다.
- 리포스트 원본을 찾기 위해 온라인 조회하더라도 **아카이브 밖의 새 항목을 삭제 대상으로 추가하지 않습니다.** 타임라인을 수집하는 기능도 없습니다.
- 수정된 게시물은 어느 버전을 삭제해도 **같은 게시물의 수정 이력 전체가 삭제**됩니다. 이력이 미리보기 대상에 표시되며, 보존 ID가 이력에 포함되면 전체를 보호합니다. [공식 수정 게시물 규칙](https://docs.x.com/x-api/fundamentals/edit-posts)
- 알 수 없는 응답·권한 문제·계정 검증 실패는 차단합니다. 미확정 항목이 있으면 그 작업의 실제 삭제를 시작하지 않습니다.

검증 후 `summary.json`, `targets.json`, `targets.csv`를 다시 확인하세요. 원본 ID가 해소되거나 보존 이력이 연결되면 대상 수가 달라질 수 있습니다.

## 7. 실제 삭제 — 사용자가 나중에 실행할 명령

**개발 및 테스트 과정에서는 이 명령으로 실제 계정의 게시물을 삭제하지 않았습니다.**

```bash
python -m x_cleaner run --job jobs/my-selection
```

위 명령은 계속 **오프라인 미리보기**입니다. 삭제를 원할 때만 다음 명령을 사용합니다.

```bash
python -m x_cleaner run --job jobs/my-selection --execute
```

계정과 현재 대상을 다시 조회한 뒤 `DELETE 계정ID 작업수 계획지문` 형식의 확인 문구를 표시합니다. **화면에 나온 문구를 정확히 입력**해야 시작하며 `yes`로는 시작되지 않습니다. 자동 확인/확인 생략 옵션은 없습니다. 삭제 옵션을 사용하면 확인을 취소하더라도 사전 조회 비용은 발생할 수 있습니다.

매 요청 직전에 선택·보존·계정·현재 대상 상태를 다시 검사합니다. 아직 수정할 수 있는 게시물은 수정 가능 시간이 끝날 때까지 차단하고, 검증 후 이력이 바뀌면 다시 `verify`하도록 중단합니다.

일반 게시물과 댓글은 `DELETE /2/tweets/{id}`, 리포스트는 `DELETE /2/users/{user_id}/retweets/{source_tweet_id}`로 처리합니다. [삭제 API](https://docs.x.com/x-api/posts/delete-post), [리포스트 취소 API](https://docs.x.com/x-api/users/unrepost-post)

리포스트 취소 API는 원본 ID를 사용하며 **특정 리포스트 항목 ID로 조건부 취소하는 API가 아닙니다**. 현재 아카이브 항목을 두 번 검증하고 사라진 항목에는 취소 요청을 보내지 않지만, 검증과 요청 사이의 동시 변경까지 서버에서 원자적으로 막을 수는 없습니다. 실행 중 같은 계정에서 게시물을 수정하거나 동일 원본을 재리포스트하지 마세요.

### 요청 제한과 오류

공개 문서상 일반 삭제와 리포스트 취소는 **각각 사용자당 15분에 50회** 제한입니다. 현재 서버 헤더와 계정 설정이 우선합니다. [공식 요청 제한](https://docs.x.com/x-api/fundamentals/rate-limits)

- `429` 요청 제한이면 `x-rate-limit-reset`과 `Retry-After` 중 더 늦은 시각까지 기다립니다. 안내가 없으면 60초 대기합니다.
- 잔여 요청 수가 0인 성공 응답에도 다음 같은 종류의 요청을 대기시킵니다.
- 최초 요청에 더해 최대 3회만 재시도합니다. 횟수·대기 시각은 이어하기에도 유지합니다.
- `401` 인증, `403` 권한, 결제/크레딧/사용량 한도 오류는 즉시 중단합니다. 사용량 한도로 인한 `429`를 무한 대기하지 않습니다.
- 서버/네트워크 오류는 제한된 재시도를 수행합니다. 삭제 성공을 확정할 수 없는 응답은 `uncertain`으로 기록합니다.
- 리포스트 문서의 현재 예시에는 취소 응답인데 `retweeted:true`인 모순이 있습니다. 프로그램은 이를 성공으로 단정하지 않고, 명시적인 `retweeted:false`만 성공으로 처리합니다.

## 8. 중단 후 이어하기

Ctrl+C로 중단할 수 있습니다. 강제 종료돼도 요청 직전의 상태를 DB에 먼저 저장하므로 복구할 수 있습니다.

```bash
python -m x_cleaner run \
  --job jobs/my-selection --resume --execute
```

계정/현재 대상 검증과 확인 입력을 다시 수행합니다. 기록된 성공 항목은 건너뛰며, 남아 있는 선택 항목만 처리합니다.

| 상태 | 의미 |
|---|---|
| `pending` | 아직 요청하지 않음 |
| `in_flight` | 요청 직전에 기록함. 종료 위치에 따라 요청 결과가 불명확할 수 있음 |
| `retry_wait` | 서버 안내/재시도 대기 중 |
| `success` | 응답에서 삭제/취소 성공을 확인함 |
| `already_absent` | 선택한 아카이브 항목이 이미 삭제되었거나 조회되지 않음. 추가 삭제 요청을 보내지 않음 |
| `failed` | 확정 오류 또는 재시도 상한 도달 |
| `uncertain` | 응답 유실/모호한 응답으로 결과를 확정할 수 없음 |

`already_absent`는 이전에 이 프로그램이 삭제했다는 증거가 아닙니다. 리포스트 항목이 사라졌다고 해서 같은 원본에 대한 **새 리포스트까지 취소하지 않습니다**. 권한 오류와 단순 미발견은 구분합니다. [공식 응답 코드](https://docs.x.com/x-api/fundamentals/response-codes-and-errors)

실패/미확정 항목은 기본 이어하기에서 재요청하지 않습니다. 인증·권한·과금 문제 등을 해결하고 명시적으로 다시 시도할 때만 다음 옵션을 사용합니다.

```bash
python -m x_cleaner run \
  --job jobs/my-selection --resume --retry-failed --execute
```

이 옵션은 해당 항목의 시도 횟수를 새로 시작하며, 다시 확인 입력이 필요합니다. 응답이 유실되면 **정확히 한 번 요청됐음을 보장할 수 없고** 같은 작업의 재시도가 발생할 수 있습니다. 현재 상태 조회로 불필요한 요청을 줄이지만 중복 요청/과금 가능성은 남습니다. 같은 작업 또는 같은 토큰으로 동시에 실행하는 경우 파일 잠금으로 두 번째 실행을 차단합니다.

## 9. 인증·실제 아카이브 없이 모의 실행

제공된 아카이브는 **가짜 데이터**입니다. 아래 명령은 API 호출 없이 실행할 수 있습니다.

```bash
python -m x_cleaner preview \
  --archive examples/archive \
  --selection-file examples/selection.example.csv \
  --job jobs/demo-preview

python -m x_cleaner run --job jobs/demo-preview
python -m pytest
```

현재 작업 폴더에는 `jobs/demo-preview`가 이미 생성되어 있습니다. 바로 `run --job jobs/demo-preview`로 볼 수 있습니다. 예제를 새로 만들려면 `--job jobs/demo-preview-2`처럼 다른 폴더 이름을 쓰세요.

모의 아카이브 6건 중 게시물·댓글·리포스트 각 1건을 선택하여 총 3건만 대상으로 만들고, 다른 3건은 그대로 둡니다. 이 가짜 아카이브로 실제 계정 인증/삭제를 진행하지 마세요.

테스트는 실제 소켓 연결을 차단하고 모의 공식 API로 선택 밖 항목 보호, 큰 ID, ZIP/분할 파싱, 보존 보호, 계정/작성자 검증, PKCE/토큰 갱신, 요청 제한, 재시도 상한, 중간 종료와 이어하기를 검증합니다. 실제 계정과 아카이브가 없어도 구현·모의 검증이 가능합니다. 실제 X 계정 연결 및 실제 아카이브의 형식 호환성은 별도 확인이 필요합니다.

종료 코드는 `0` 정상, `2` 차단/오류/미완료 실패 상태, `130` 사용자 중단입니다.

## 사용자가 준비할 항목

1. 본인 계정의 완전한 X 아카이브 ZIP 또는 폴더.
2. **삭제할 내 게시물·댓글 링크/ID, 취소할 내 리포스트 링크/ID**. 추가 보존 목록은 선택 사항입니다.
3. 개발자 앱의 OAuth 2.0 설정, Client ID, 등록된 로컬 콜백 주소.
4. API 접근 가능 여부, 엔드포인트 단가, 크레딧 잔액과 지출 상한 확인.
5. 실제 실행 전 최종 `targets.json`/CSV 검토.
