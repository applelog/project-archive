> 정리 전 문서입니다. 상대 경로와 실행 안내는 이전 위치 기준이며, 기능·검증 주장은 당시 기록입니다. 현재 안내는 프로젝트 README와 루트 README를 참고하세요.

# 이동권 길찾기

지도에서 출발지·목적지를 선택하거나 장소를 검색해 이동 조건에 맞는 보행 경로를 계산하는 iPhone 앱입니다. 특정 동네·역에 종속된 경로 분기를 제거했습니다. 경로 자료가 제공되는 지역에 같은 엔진을 적용합니다.

## 현재 구현

- 제약 필터 → 비음수 비용 → A* → 서로 다른 후보 최대 3개.
- 일반 보행은 자체 보행망 후보와 Apple 지도 보행 대안을 선택할 수 있습니다. Apple 대안은 가능한 경우에만 여러 개 반환되며, 계단·경사·출입구 접근성은 미확인입니다. 휠체어·보행 보조·계단 회피에는 표시하지 않습니다. 일반 보행 경로 조회 시 출발지·목적지 좌표가 Apple 지도에 전송됩니다.
- 휠체어·보행 보조·계단 회피: 계단, 에스컬레이터, 통행 불가, 미확인 구조물과 확인·추정 경사가 개인 한도를 넘는 링크를 제외합니다. 실측 확인 경사만 이용하는 경로가 없으면 추정·미확인 경사를 포함한 후보를 별도로 표시합니다. 이 후보는 안전 경로로 추천하거나 안내를 시작하지 않습니다.
- 빠르게: 추정 이동시간 최소화. 짧게: 거리 최소화. 경사 부담: 거리와 경사 부담의 가중 비용 최소화.
- 보행로 중간의 출발·도착도 부분 링크로 연결합니다. 일반 보행과 접근성 미확인 후보에서는 35m 이내의 연결된 다른 보행로를 살피고, 출발·도착 접속 간격을 별도로 표시합니다. 확인된 경사만 이용하는 엄격 경로는 가장 가까운 링크만 사용합니다. 역방향은 명시적 허용이 필요합니다. 연결되지 않은 위치를 직선 경로로 대신하지 않습니다.
- 지도 상단 출발지·도착지 검색, 지도 핀의 출발·도착 지정, 별도 이동 설정 시트, 경로별 경사 확인 상태 표시, 평지 이동 속도, 거리, 예상 시간·도착 시각, 회전·계단·에스컬레이터 상세 안내, 뒤로 가기.
- 선택한 경로의 전체 선형과 출발·도착점을 기준으로 지도를 맞춥니다. 경로 선택 시 다시 맞추며, `전체 경로` 버튼으로 돌아볼 수 있습니다. 상단 검색창과 접힌 하단 패널의 공간을 지도에 확보했습니다. 경로 결과 상단에는 예상 시간·거리·도착 시각과 접근성 확인 상태를 표시합니다.
- 일반 보행의 장거리 경로에는 Apple 지도 대중교통 예상 시간을 비교하고 Apple 지도에서 상세 경로를 엽니다. 이 결과가 지하철인지와 역내 접근성은 앱에서 확인하지 않습니다.
- 데이터 읽기·경로 계산은 UI 스레드 밖에서 수행하고 취소된 검색·계산 결과는 화면에 반영하지 않습니다.
- 보행망의 검증·방향별 연결표는 로딩할 때 한 번 준비해 재사용합니다. 출발·도착 투영은 위도 범위로 먼저 걸러내며, A*가 실제 확장하는 노드의 링크만 구체화합니다. 경로선은 초록색과 흰 테두리로 표시하며, 자체 보행망의 미확인 구간은 점선으로 구분합니다. 경사 확인 막대의 회색은 자료 미확인이고 평지를 뜻하지 않습니다.

예상 시간은 설정한 속도에 확인·추정 경사와 시설별 계수를 적용한 **공학적 추정**입니다. 경사 자료가 없는 링크는 평지로 가정하므로 오차가 커질 수 있습니다. 계수는 이용 기록으로 보정하지 않았습니다. 신호·대기 및 보행망 밖 연결 구간은 제외합니다. 경사 부담 모드는 최대 경사를 최소화하는 별도 최적화가 아닙니다. 링크 중간에서 출발·도착해도 한도 검사와 비용에는 원래 링크의 최대 경사를 사용하므로 보수적인 제외·시간 과대 추정이 생길 수 있습니다.

## 다음 핵심 작업: 보행·버스·지하철 연결

2026-10-09 사용자 정정에 따라 보행망 보강과 대중교통 전체 여정을 병행합니다. 출발지→정류장/역 출입구→승강장→탑승·환승→역 밖/정류장→목적지를 연결하고 전체 예상시간·보행량·환승 부담을 비교합니다. 걷기 힘든 사용자는 설정에 따라 에스컬레이터/엘리베이터를, 휠체어는 계단·에스컬레이터를 제외한 엘리베이터 동선을 사용합니다. 버스는 실제 저상 차량과 승하차 접근성을 별도로 확인합니다.

이 기능은 아직 앱에 연결되지 않았습니다. [ODsay 대중교통 길찾기 v1.8](https://lab.odsay.com/guide/releaseReference?platform=web)을 후보 공급자로 검토하고 있으며 서비스 선택·승인·요금·표시/가공 조건과 HTTPS 서버 연결을 확인해야 합니다. 제안된 키 이름은 `.env`의 `ODSAY_API_KEY`이며 앱 번들에는 포함하지 않습니다. 기존 서울 API는 역간 열차 후보와 시설 정보를 보완하는 자료로 사용합니다. 상세 연결·제약·검증 기준은 [설계 문서 7절](docs/superpowers/specs/2026-09-25-accessible-routing-design.md)에 기록했습니다.

## 실제 자료와 한계

2026-09-27자 [Geofabrik 한국 OpenStreetMap 원자료](https://download.geofabrik.de/asia/south-korea.html)를 공식 MD5와 대조해 확보했습니다. `Data/network.json`은 이 자료에서 서울·광명시를 포함하는 경위도 범위 `126.73, 37.39, 127.20, 37.71`의 보행 연결을 변환한 **285,684개 노드·361,221개 링크**입니다. 범위 밖 장소는 별도 자료가 필요하다고 표시합니다. 알고리즘과 변환기는 다른 지역에도 그대로 적용하며, 현재 앱에 포함한 데이터의 범위만 제한됩니다. 원자료와 변환된 보행망은 [ODbL 1.0 및 OpenStreetMap 저작자 표시](https://www.openstreetmap.org/copyright) 조건을 따릅니다.

OSM의 같은 노드를 공유하는 링크만 연결합니다. `highway=steps` 4,967개, `wheelchair=no/limited` 50개, 방향이 표시된 에스컬레이터 5개를 각각 분류했습니다. OSM 표기 누락, 보도와 실제 출입구의 연결, 도로 중심선과 보도 위치 차이는 남아 있습니다. **299,562개 링크는 지형 추정 경사, 61,659개는 경사 자료 없음**입니다. 실제 보도 경사·턱·폭의 현장 확인 표 `Data/field_checks.csv`는 비어 있어 확인된 휠체어 경로는 계산하지 않습니다. 앱의 휠체어 결과는 지도에서만 살펴보는 미확인 후보입니다. 계단 없는 것으로 증명되었다고 표시하지 않습니다. 일반 보행 결과도 현장 대조 전의 후보입니다.

2026-10-09 지형 연결 후 일반 보행 빠른 후보에서 시청→종각 약 2,306m/40분, 홍대→합정 약 1,269m/23분, 강남→역삼 약 857m/23분, 서울숲→건대 약 4,807m/86분을 A*와 Dijkstra가 같은 비용으로 찾았습니다. 평지 속도 1m/s와 추정 경사를 적용했습니다. 시청→종각은 휠체어 한도 8.33% 내 연결 후보가 없습니다. **자체 보행망의 큰 우회는 현실의 최단 경로를 뜻하지 않습니다.** 2026-09-29 macOS MapKit 실조회에서는 ‘종각역’ 장소 1개와 810~852m 보행 경로 3개를 받았습니다. 앱은 일반 보행에서 예상 시간이 더 짧은 후보를 처음 선택하지만 Apple 시간에는 개인 속도 설정이 반영되지 않습니다. 보행망에서 검색 좌표까지 남은 거리를 실제 통행 가능한 길로 확인한 것은 아닙니다.

기존 [서울시 보행 네트워크](https://data.seoul.go.kr/dataList/OA-21208/A/1/datasetView.do) CSV 전체 491,082건에서 변환한 212,066개 노드·263,264개 링크는 `.private/seoul_network_2026-09-26.json`에 별도 보관합니다. 서울시 자료의 네 실구간이 모두 단절되어 앱의 일반 보행망은 OSM으로 교체했습니다. 두 자료를 섞어 근거 없는 연결을 만들지 않았습니다. 원본 CSV에는 보도 실측 경사·턱·폭·계단·통행 방향이 없고 공식 도로 기준 시점은 2020년입니다.

2026-10-09: [서울시 경사도·등고선 OA-22241](https://data.seoul.go.kr/dataList/OA-22241/F/1/datasetView.do)의 **2023년 기준 지형 자료**를 보행 링크에 연결했습니다. 제공된 두 폴더 중 범위가 넓은 `서울시 경사도/`의 표고점·등고선 한 쌍만 사용했으며, `서울시 등고선/`의 다른 판본과 섞지 않았습니다. 포털의 공공누리 1유형 출처표시를 앱에 추가했습니다. 파일 자료이므로 API 키는 필요 없습니다.

미터 투영 좌표에서 등고선 20m 간격 표본과 표고점으로 삼각망을 만들고, 길의 10m 간격 높이에서 최대 오르막·내리막을 계산합니다. 30m 창을 사용하며 30m보다 짧은 링크는 전체 길이를 사용합니다. 최근접 지형 표본 125m 초과, 삼각형 최대 변 300m 초과, 자료 범위 밖은 추정하지 않습니다. 이 값들은 보간 지원 범위를 제한하는 공학적 기준이며 오차 보장이 아닙니다. 구조물·장애물 13,216개와 범위 밖·희박한 지형 48,443개는 미확인으로 유지했습니다. OSM에 교량·터널·비지상 층이 표시된 길에 지표 높이를 적용하지 않습니다. 구조물 표기 누락, 짧은 경사로·턱·횡경사와 지형 오차는 남아 있습니다. 경로 상세에서 추정 고도 단면과 실측/추정/자료 없음의 거리를 구분합니다.

추정 경사는 A* 비용·ETA와 참고 후보의 개인 한도 검사에 사용하며 실측값으로 승격하지 않습니다. 한도를 초과하면 우회하거나 후보가 없다고 반환합니다. Slopemap/FABDEM 자료나 타 서비스 경로를 가져와 휠체어 안전 경로로 대체하지 않았습니다. 역내 동선·엘리베이터 운행 상태와 실제 이용자 대조는 추가 검증이 필요합니다.

## 폴더와 실행

```text
App/                      SwiftUI 지도, 검색, 위치와 경로 화면
Core/                     A*, 제약, 비용, 시간·상세 안내
Data/                     앱 보행망, 현장 확인 표, 역내 단계
Tools/                    OSM·공식 CSV·철도 자료 변환
Tests/                    경로·자료 입력 검증
docs/superpowers/specs/    설계와 구현 상태
.private/                 공유하지 않는 원본 PBF·CSV·변환 검증 자료
.env                      로컬 API 키; 앱에 포함하지 않음
MobilityGuide.xcodeproj/   iPhone 프로젝트
```

Python 작업은 `conda activate min`에서 실행합니다. OSM 변환에만 `osmium==4.3.1`이 필요합니다. 현재 원본은 `.private/south-korea-260927.osm.pbf`, 서울시 CSV 원본은 `.private/seoul_walk_2026-09-26.csv`에 있습니다.

```sh
python -m pip install osmium==4.3.1
python Tools/import_osm.py --pbf .private/south-korea-260927.osm.pbf --output Data/network.json --bbox 126.73 37.39 127.20 37.71 --extract-date 2026-09-27
```

`--bbox`를 바꾸면 같은 변환기로 다른 지역 자료를 만들 수 있습니다. 경계 밖의 도로는 자르므로 경계를 넘는 경로를 계산하려면 더 넓은 범위의 자료가 필요합니다. 기존 서울시 자료를 다시 변환할 때는 다음 명령을 사용합니다.

지형 추정 경사를 다시 연결할 때는 `conda min`에서 `python -m pip install -r Tools/requirements-terrain.txt`로 필요한 패키지를 설치합니다. 원본을 보존하고 별도 출력 파일을 검증한 뒤 앱 자료로 교체합니다.

```sh
python Tools/import_terrain.py --network Data/network.json --contours '서울시 경사도/등고선 5000/N3L_F001.shp' --spot-heights '서울시 경사도/표고 5000/N3P_F002.shp' --osm-pbf .private/south-korea-260927.osm.pbf --output .private/network-terrain.json
```

```sh
python Tools/import_seoul.py --network-csv .private/seoul_walk_2026-09-26.csv --field-checks Data/field_checks.csv --bidirectional --output .private/seoul_network_2026-09-26.json
```

`--bidirectional`은 그 공식 CSV의 일반 보행용 근사 옵션입니다. 현장 확인 표의 형식:

```csv
edge_id,grade_percent,obstacle,checked_at,reverse_allowed,forward_max_uphill_percent,forward_max_downhill_percent
```

최대 절대 경사를 `grade_percent`로 기록하거나, 정방향 최대 오르막·내리막 두 값을 함께 기록합니다. `obstacle`은 `none`, `stairs`, `escalator`, `impassable`, `unverifiedOverpass`; `reverse_allowed`는 확인한 `true/false`; 날짜는 `YYYY-MM-DD`입니다. 빈 값은 미확인입니다. 경사 확인에는 장애물 확인도 필요합니다. 가상 측정값을 실제 자료에 넣지 않습니다.

Xcode에서 프로젝트를 열어 Signing Team을 설정한 뒤 iPhone에서 실행합니다. iPhone 17 시뮬레이터에서 첫 화면을 실행·확인했습니다. GPS 없이 확인하려면 지도를 눌러 핀의 출발지·도착지 버튼을 차례로 선택한 뒤 경로 찾기를 사용합니다. 휠체어 모드에서는 미확인 후보만 표시될 수 있습니다.

2026-10-08: 시뮬레이터 빌드·설치·실행과 첫 화면 캡처를 확인했습니다. 핵심 경로 검사는 실제 보행망의 네 구간에서 통과했습니다. 화면 조작 도구가 `Computer Use permissions are not granted`를 반환하므로 검색→핀→프로필→경로 선택을 직접 누르는 전체 흐름과 우회 경로의 화면 배치는 아직 확인하지 못했습니다. 빌드 성공을 이 흐름의 검증 완료로 취급하지 않습니다.

2026-10-08 성능 비교: 같은 Mac, 같은 보행망, `swiftc -O`로 세 비용 모드를 연속 계산했습니다. 시청→종각 5.364초→0.056초, 서울숲→건대의 휠체어 미확인 후보 4.723초→0.057초였고 각 모드의 이전/이후 비용은 동일했습니다. 초기 JSON 읽기(이 실험 약 4~6초), 라우터 준비, Apple 네트워크 응답과 iPhone UI 시간은 이 수치에 포함하지 않습니다. 새로운 알고리즘이 더 짧은 현실 경로를 증명한 것은 아닙니다.

2026-10-09 새 자료(약 215MiB)의 Mac JSON 읽기·라우터 준비는 약 6~7초였습니다. Python 자료 변환 검사 23개(선택 입력 1개 제외), Swift 합성·실제 네 구간 검사, 시뮬레이터 빌드·설치·실행과 키 유출 검사를 통과했습니다. 개봉성당→서울역 본관은 일반 보행 최단 후보 약 30.9km, 휠체어 후보 없음으로, 지형 연결 전에도 큰 우회가 동일하게 발생했습니다. 같은 좌표의 macOS MapKit 실조회는 Apple 보행 후보 13,105m/225분을 반환했으며 대중교통 ETA는 조회할 수 없었습니다. 이 구간의 실제 보행 연결·접근 가능한 역 출입구를 보강해야 합니다. 앱 화면을 직접 누르는 전체 흐름은 여전히 미검증입니다.

## 검증 명령

```sh
conda activate min
python -m unittest discover -s Tests -p 'test_import_*.py' -v
mkdir -p .build
swiftc -O -module-cache-path .build/mobility-swift-cache Core/RouteModel.swift Core/Router.swift Core/StationModel.swift Tests/main.swift -o .build/mobility-core-check
.build/mobility-core-check
.build/mobility-core-check Data/network.json
swiftc -O -module-cache-path .build/benchmark-module-cache Core/RouteModel.swift Core/Router.swift Core/StationModel.swift Tests/RouteBenchmark/main.swift -o .build/route-benchmark
.build/route-benchmark
xcodebuild -quiet -project MobilityGuide.xcodeproj -scheme MobilityGuide -sdk iphoneos -destination 'generic/platform=iOS' -derivedDataPath ./DerivedData CODE_SIGNING_ALLOWED=NO build
```

마지막 Swift 실행은 네 실제 구간의 연결과 A*·Dijkstra 비용 일치를 검사합니다. 합성 그래프는 고립된 목적지 인근 링크의 대안 선택과 엄격 모드의 대안 금지도 검사합니다. Apple 지도 경로의 실시간 응답과 실기기 안내는 별도 검증이 필요합니다.

서울교통공사 `getShtrmPath`는 역간 열차 후보, `getWksnEsctr`는 에스컬레이터 시설 자료입니다. 명세·공개 샘플은 확인했고 상세 내용은 [설계 문서](docs/superpowers/specs/2026-09-25-accessible-routing-design.md)에 기록했습니다. 두 API는 아직 앱의 대중교통 경로에 연결하지 않았습니다. 키는 `.env`에만 저장하며 공식 HTTP 예시로 전송하지 않습니다. AWS Lambda·API Gateway 중계는 가능하지만 서울 API의 키 전송 구간에 검증된 HTTPS 주소가 확인돼야 연결합니다.
