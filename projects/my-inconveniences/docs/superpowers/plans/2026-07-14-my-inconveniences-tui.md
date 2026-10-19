# '나의 불편함' TUI 대시보드 구현 계획 (Implementation Plan)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** macOS 환경에서 동작하며 Antigravity 플러그인 폴더를 활성화/비활성화하고, 배터리 충전 와트 및 인터넷 핑/속도를 실시간으로 모니터링하여 보여주는 Go 언어 기반의 Bubble Tea TUI 대시보드 구축.

**Architecture:** Model-Update-View 패턴을 강제하는 Bubble Tea를 바탕으로 비동기 백그라운드 고루틴 처리를 통해 UI 멈춤 없이 핑(3초 주기)과 인터넷 속도(10분 주기 / R키 수동)를 수집하는 아키텍처.

**Tech Stack:** Go (1.26.5), Charm CLI (github.com/charmbracelet/bubbletea, github.com/charmbracelet/lipgloss, github.com/charmbracelet/bubbles)

## Global Constraints
- 모든 Go 코드는 macOS darwin/arm64 아키텍처에서 동작해야 함.
- 플러그인 활성 경로는 `~/.gemini/config/plugins/`이고 비활성 경로는 `~/.gemini/config/plugins_disabled/`임.
- 외부 라이브러리는 github.com/charmbracelet 의 공식 패키지만 사용하며 추가 의존성을 최소화함.
- 테스트 시 실제 파일 시스템이나 네트워크를 오염시키지 않도록 Mocking 또는 임시 디렉토리를 활용함.

---

### Task 1: 프로젝트 초기화 및 기본 Bubble Tea 앱 구축

**Files:**
- Create: `go.mod`
- Create: `main.go`
- Test: `main_test.go`

**Interfaces:**
- Produces: `main.go` 내의 기본 `tea.Model` 정의 및 빌드/실행 환경 구성.

- [ ] **Step 1: Go 모듈 생성 및 의존성 다운로드**

Run:
```bash
go mod init my-inconveniences
go get github.com/charmbracelet/bubbletea
go get github.com/charmbracelet/lipgloss
go get github.com/charmbracelet/bubbles
```
Expected: `go.mod` 및 `go.sum`이 생성되고 의존성 라이브러리가 올바르게 다운로드됨.

- [ ] **Step 2: 기본 TUI 렌더링 검증용 테스트 작성**

Create: `main_test.go`
```go
package main

import (
	"testing"
	tea "github.com/charmbracelet/bubbletea"
)

func TestInitialModel(t *testing.T) {
	m := initialModel()
	view := m.View()
	expected := "나의 불편함 Dashboard"
	if !contains(view, expected) {
		t.Errorf("Expected view to contain %q, got %q", expected, view)
	}
}

func contains(s, substr string) bool {
	return len(s) >= len(substr) && (s == substr || s[0:len(substr)] == substr || s[len(s)-len(substr):] == substr)
}
```

- [ ] **Step 3: 테스트 실패 확인**

Run: `go test -v ./...`
Expected: 컴파일 에러 혹은 `initialModel` 미정의 오류로 실패.

- [ ] **Step 4: minimal main.go 구현**

Create: `main.go`
```go
package main

import (
	"fmt"
	"os"
	tea "github.com/charmbracelet/bubbletea"
)

type model struct {
	title string
}

func initialModel() model {
	return model{title: "나의 불편함 Dashboard"}
}

func (m model) Init() tea.Cmd {
	return nil
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "q", "ctrl+c":
			return m, tea.Quit
		}
	}
	return m, nil
}

func (m model) View() string {
	return fmt.Sprintf("=== %s ===\nPress 'q' to quit.\n", m.title)
}

func main() {
	p := tea.NewProgram(initialModel())
	if _, err := p.Run(); err != nil {
		fmt.Printf("Error running program: %v", err)
		os.Exit(1)
	}
}
```

- [ ] **Step 5: 테스트 수행 및 통과 확인**

Run: `go test -v ./...`
Expected: PASS

- [ ] **Step 6: Git 커밋**

Run:
```bash
git add go.mod go.sum main.go main_test.go
git commit -m "feat: initialize Go project and set up basic bubbletea application"
```

---

### Task 2: Antigravity 플러그인 제어 모듈 개발

**Files:**
- Create: `plugins/plugins.go`
- Create: `plugins/plugins_test.go`

**Interfaces:**
- Produces: `plugins.CheckPluginStatus(baseDir string, name string) bool`
- Produces: `plugins.TogglePlugin(baseDir string, name string) error`

- [ ] **Step 1: 플러그인 제어 테스트 케이스 작성**

Create: `plugins/plugins_test.go`
```go
package plugins

import (
	"os"
	"path/filepath"
	"testing"
)

func TestPluginToggle(t *testing.T) {
	tmpDir, err := os.MkdirTemp("", "gemini_config_test")
	if err != nil {
		t.Fatalf("Failed to create temp dir: %v", err)
	}
	defer os.RemoveAll(tmpDir)

	pluginsDir := filepath.Join(tmpDir, "plugins")
	disabledDir := filepath.Join(tmpDir, "plugins_disabled")

	err = os.MkdirAll(pluginsDir, 0755)
	if err != nil {
		t.Fatalf("Failed to create plugins dir: %v", err)
	}

	pluginName := "superpowers"
	pluginPath := filepath.Join(pluginsDir, pluginName)
	err = os.Mkdir(pluginPath, 0755)
	if err != nil {
		t.Fatalf("Failed to create dummy plugin: %v", err)
	}

	// 1. Initial status check
	if !CheckPluginStatus(tmpDir, pluginName) {
		t.Errorf("Expected plugin %s to be active", pluginName)
	}

	// 2. Toggle off (active -> disabled)
	err = TogglePlugin(tmpDir, pluginName)
	if err != nil {
		t.Fatalf("TogglePlugin failed: %v", err)
	}

	if CheckPluginStatus(tmpDir, pluginName) {
		t.Errorf("Expected plugin %s to be inactive after toggle off", pluginName)
	}

	// 3. Toggle on (disabled -> active)
	err = TogglePlugin(tmpDir, pluginName)
	if err != nil {
		t.Fatalf("TogglePlugin failed: %v", err)
	}

	if !CheckPluginStatus(tmpDir, pluginName) {
		t.Errorf("Expected plugin %s to be active after toggle on", pluginName)
	}
}
```

- [ ] **Step 2: 테스트 실패 확인**

Run: `go test -v ./plugins`
Expected: FAIL (CheckPluginStatus 및 TogglePlugin 미구현으로 컴파일 에러)

- [ ] **Step 3: 플러그인 관리 유틸 구현**

Create: `plugins/plugins.go`
```go
package plugins

import (
	"errors"
	"os"
	"path/filepath"
)

func CheckPluginStatus(baseDir string, name string) bool {
	activePath := filepath.Join(baseDir, "plugins", name)
	info, err := os.Stat(activePath)
	return err == nil && info.IsDir()
}

func TogglePlugin(baseDir string, name string) error {
	pluginsDir := filepath.Join(baseDir, "plugins")
	disabledDir := filepath.Join(baseDir, "plugins_disabled")

	activePath := filepath.Join(pluginsDir, name)
	inactivePath := filepath.Join(disabledDir, name)

	if CheckPluginStatus(baseDir, name) {
		// Active -> Disabled
		if err := os.MkdirAll(disabledDir, 0755); err != nil {
			return err
		}
		return os.Rename(activePath, inactivePath)
	} else {
		// Disabled -> Active
		disabledPath := filepath.Join(disabledDir, name)
		info, err := os.Stat(disabledPath)
		if err != nil || !info.IsDir() {
			// 만약 둘 다 없으면 새로 생성하여 활성화 처리
			if err := os.MkdirAll(pluginsDir, 0755); err != nil {
				return err
			}
			return os.Mkdir(activePath, 0755)
		}
		if err := os.MkdirAll(pluginsDir, 0755); err != nil {
			return err
		}
		return os.Rename(inactivePath, activePath)
	}
}
```

- [ ] **Step 4: 테스트 수행 및 통과 확인**

Run: `go test -v ./plugins`
Expected: PASS

- [ ] **Step 5: Git 커밋**

Run:
```bash
git add plugins/plugins.go plugins/plugins_test.go
git commit -m "feat: implement Antigravity plugin status checker and directory-based toggler"
```

---

### Task 3: macOS 시스템 및 환경 정보 모니터링 모듈 개발

**Files:**
- Create: `system/system.go`
- Create: `system/system_test.go`

**Interfaces:**
- Produces: `system.GetBatteryInfo() (pct int, watt float64, charging bool, err error)`
- Produces: `system.GetPingLatency() (string, error)`
- Produces: `system.GetDownloadSpeed() (float64, error)`

- [ ] **Step 1: 시스템 모니터링 데이터 파서 테스트 작성**

Create: `system/system_test.go`
```go
package system

import (
	"testing"
)

func TestParseBatteryOutput(t *testing.T) {
	// pmset -g batt 예시 출력 모킹 테스트
	sampleOutput := `Now drawing from 'AC Power'
 -InternalBattery-0 (id=1234567)	98%; charging; 0:12 remaining present: true`
	pct, charging, err := parseBatteryPercent(sampleOutput)
	if err != nil {
		t.Fatalf("parseBatteryPercent failed: %v", err)
	}
	if pct != 98 {
		t.Errorf("Expected 98%%, got %d%%", pct)
	}
	if !charging {
		t.Errorf("Expected charging to be true")
	}
}

func TestParseWattageOutput(t *testing.T) {
	// ioreg 모킹 테스트
	sampleIoreg := `"AppleRawAdapterVoltage" = 20000
"AppleRawAdapterCurrent" = 3250`
	watt := parseWattage(sampleIoreg)
	expectedWatt := 20.0 * 3.25 // = 65W
	if watt != expectedWatt {
		t.Errorf("Expected %fW, got %fW", expectedWatt, watt)
	}
}
```

- [ ] **Step 2: 테스트 실패 확인**

Run: `go test -v ./system`
Expected: FAIL (파싱 함수 미정의 에러)

- [ ] **Step 3: 시스템 상태 모니터 구현**

Create: `system/system.go`
```go
package system

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"net/http"
	"os/exec"
	"regexp"
	"strconv"
	"strings"
	"time"
)

func GetBatteryInfo() (int, float64, bool, error) {
	// 1. 배터리 퍼센트 및 충전 여부 구하기
	cmdBatt := exec.Command("pmset", "-g", "batt")
	var outBatt bytes.Buffer
	cmdBatt.Stdout = &outBatt
	if err := cmdBatt.Run(); err != nil {
		return 0, 0, false, err
	}
	pct, charging, err := parseBatteryPercent(outBatt.String())
	if err != nil {
		return 0, 0, false, err
	}

	// 2. 충전 와트(Wattage) 구하기
	cmdIoreg := exec.Command("ioreg", "-rw0", "-r", "-c", "AppleSmartBattery")
	var outIoreg bytes.Buffer
	cmdIoreg.Stdout = &outIoreg
	_ = cmdIoreg.Run() // 실패해도 0W 처리

	watt := parseWattage(outIoreg.String())

	return pct, watt, charging, nil
}

func parseBatteryPercent(output string) (int, bool, error) {
	re := regexp.MustCompile(`(\d+)%;\s*([^;]+);`)
	matches := re.FindStringSubmatch(output)
	if len(matches) < 3 {
		return 0, false, fmt.Errorf("could not parse battery output")
	}
	pct, err := strconv.Atoi(matches[1])
	if err != nil {
		return 0, false, err
	}
	status := matches[2]
	charging := strings.Contains(status, "charging")
	return pct, charging, nil
}

func parseWattage(output string) float64 {
	// AppleRawAdapterVoltage(mV) 와 AppleRawAdapterCurrent(mA) 파싱
	reVolt := regexp.MustCompile(`"AppleRawAdapterVoltage"\s*=\s*(\d+)`)
	reCurr := regexp.MustCompile(`"AppleRawAdapterCurrent"\s*=\s*(\d+)`)

	voltMatch := reVolt.FindStringSubmatch(output)
	currMatch := reCurr.FindStringSubmatch(output)

	if len(voltMatch) < 2 || len(currMatch) < 2 {
		return 0.0
	}

	voltMV, _ := strconv.ParseFloat(voltMatch[1], 64)
	currMA, _ := strconv.ParseFloat(currMatch[1], 64)

	// mV * mA / 1,000,000 = Watt
	return (voltMV * currMA) / 1000000.0
}

func GetPingLatency() (string, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, "ping", "-c", "1", "-t", "1", "8.8.8.8")
	var out bytes.Buffer
	cmd.Stdout = &out
	if err := cmd.Run(); err != nil {
		return "오프라인", err
	}

	// 핑 결과 파싱: time=24.5 ms
	re := regexp.MustCompile(`time=([\d\.]+)\s*ms`)
	match := re.FindStringSubmatch(out.String())
	if len(match) < 2 {
		return "오프라인", fmt.Errorf("parse ping error")
	}
	return match[1] + " ms", nil
}

func GetDownloadSpeed() (float64, error) {
	// Cloudflare 속도 측정용 임시 파일 다운로드 (1MB)
	url := "https://speed.cloudflare.com/__down?bytes=1048576"
	ctx, cancel := context.WithTimeout(context.Background(), 8*time.Second)
	defer cancel()

	req, err := http.NewRequestWithContext(ctx, "GET", url, nil)
	if err != nil {
		return 0, err
	}

	start := time.Now()
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return 0, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return 0, fmt.Errorf("http error: %d", resp.StatusCode)
	}

	written, err := io.Copy(io.Discard, resp.Body)
	if err != nil {
		return 0, err
	}

	duration := time.Since(start).Seconds()
	if duration <= 0 {
		return 0, fmt.Errorf("duration too short")
	}

	// Megabits per second = (bytes * 8) / (1024 * 1024) / seconds
	mbps := (float64(written) * 8.0) / (1024.0 * 1024.0) / duration
	return mbps, nil
}
```

- [ ] **Step 4: 테스트 수행 및 통과 확인**

Run: `go test -v ./system`
Expected: PASS (모킹 파서 테스트가 통과되고, 로컬에 물리적으로 장치가 없더라도 로직 에러가 발생하지 않음)

- [ ] **Step 5: Git 커밋**

Run:
```bash
git add system/system.go system/system_test.go
git commit -m "feat: implement macOS battery charging status parser and network ping/download speed testers"
```

---

### Task 4: UI 디자인 및 Bubble Tea 통합 대시보드 구현

**Files:**
- Modify: `main.go`
- Create: `ui/theme.go`

**Interfaces:**
- Consumes: `plugins.CheckPluginStatus`
- Consumes: `plugins.TogglePlugin`
- Consumes: `system.GetBatteryInfo`
- Consumes: `system.GetPingLatency`
- Consumes: `system.GetDownloadSpeed`

- [ ] **Step 1: Lipgloss 기반 스타일 테마 생성**

Create: `ui/theme.go`
```go
package ui

import "github.com/charmbracelet/lipgloss"

type Theme struct {
	Title        lipgloss.Style
	Box          lipgloss.Style
	FocusBox     lipgloss.Style
	Footer       lipgloss.Style
	ActiveText   lipgloss.Style
	InactiveText lipgloss.Style
}

func DefaultTheme() Theme {
	return Theme{
		Title: lipgloss.NewStyle().
			Bold(true).
			Foreground(lipgloss.Color("#7D56F4")). // 보라색
			Padding(0, 1),
		Box: lipgloss.NewStyle().
			Border(lipgloss.RoundedBorder()).
			BorderForeground(lipgloss.Color("#3C3C3C")).
			Padding(1, 2).
			Width(38).
			Height(10),
		FocusBox: lipgloss.NewStyle().
			Border(lipgloss.RoundedBorder()).
			BorderForeground(lipgloss.Color("#7D56F4")).
			Padding(1, 2).
			Width(38).
			Height(10),
		Footer: lipgloss.NewStyle().
			Foreground(lipgloss.Color("#8E8E8E")).
			Italic(true),
		ActiveText: lipgloss.NewStyle().
			Foreground(lipgloss.Color("#00FF00")).
			Bold(true),
		InactiveText: lipgloss.NewStyle().
			Foreground(lipgloss.Color("#8E8E8E")),
	}
}
```

- [ ] **Step 2: main.go에서 TUI 대시보드 로직 구현 및 백그라운드 고루틴 연동**

Modify: `main.go`
```go
package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
	"my-inconveniences/plugins"
	"my-inconveniences/system"
	"my-inconveniences/ui"
)

type tickMsg time.Time
type speedTestResultMsg float64
type speedTestErrorMsg error
type speedTestStartMsg struct{}

type model struct {
	baseConfigDir string
	theme         ui.Theme
	activeTab     int // 0: Plugins, 1: Monitor, 2: Links
	plugins       []string

	// 시스템 정보
	batteryPct  int
	batteryWatt float64
	isCharging  bool
	ping        string
	speedMbps   float64
	speedTesting bool
}

func initialModel() model {
	home, _ := os.UserHomeDir()
	configDir := filepath.Join(home, ".gemini")

	return model{
		baseConfigDir: configDir,
		theme:         ui.DefaultTheme(),
		activeTab:     0,
		plugins:       []string{"frontend-design", "karpathy-guidelines", "superpowers"},
		batteryPct:    0,
		batteryWatt:   0.0,
		isCharging:    false,
		ping:          "측정 중...",
		speedMbps:     -1.0,
		speedTesting:  false,
	}
}

func (m model) Init() tea.Cmd {
	// 초기 로드 시 3초 타이머 및 초기 시스템 측정 시작
	return tea.Batch(
		doTick(),
		m.measureSystem(),
	)
}

func doTick() tea.Cmd {
	return tea.Tick(3*time.Second, func(t time.Time) tea.Msg {
		return tickMsg(t)
	})
}

func (m model) measureSystem() tea.Cmd {
	return func() tea.Msg {
		pct, watt, charging, _ := system.GetBatteryInfo()
		pingVal, _ := system.GetPingLatency()
		return systemUpdateMsg{pct: pct, watt: watt, charging: charging, ping: pingVal}
	}
}

type systemUpdateMsg struct {
	pct      int
	watt     float64
	charging bool
	ping     string
}

func (m model) runSpeedTest() tea.Cmd {
	return func() tea.Msg {
		speed, err := system.GetDownloadSpeed()
		if err != nil {
			return speedTestErrorMsg(err)
		}
		return speedTestResultMsg(speed)
	}
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "q", "ctrl+c":
			return m, tea.Quit
		case "tab":
			m.activeTab = (m.activeTab + 1) % 3
		case "1":
			_ = plugins.TogglePlugin(m.baseConfigDir, m.plugins[0])
		case "2":
			_ = plugins.TogglePlugin(m.baseConfigDir, m.plugins[1])
		case "3":
			_ = plugins.TogglePlugin(m.baseConfigDir, m.plugins[2])
		case "g", "G":
			_ = exec.Command("open", "https://github.com").Start()
		case "o", "O":
			_ = exec.Command("open", ".").Start()
		case "r", "R":
			if !m.speedTesting {
				m.speedTesting = true
				return m, tea.Batch(
					func() tea.Msg { return speedTestStartMsg{} },
					m.runSpeedTest(),
				)
			}
		}

	case tickMsg:
		// 3초 주기로 배터리 및 Ping 갱신
		return m, tea.Batch(
			doTick(),
			m.measureSystem(),
		)

	case systemUpdateMsg:
		m.batteryPct = msg.pct
		m.batteryWatt = msg.watt
		m.isCharging = msg.charging
		m.ping = msg.ping

	case speedTestStartMsg:
		m.speedTesting = true

	case speedTestResultMsg:
		m.speedMbps = float64(msg)
		m.speedTesting = false

	case speedTestErrorMsg:
		m.speedTesting = false
	}

	return m, nil
}

func (m model) View() string {
	// 플러그인 뷰 빌드
	var pluginList string
	for i, name := range m.plugins {
		status := "[○] OFF"
		if plugins.CheckPluginStatus(m.baseConfigDir, name) {
			status = m.theme.ActiveText.Render("[●] ON ")
		} else {
			status = m.theme.InactiveText.Render(status)
		}
		pluginList += fmt.Sprintf(" %d. %-20s %s\n", i+1, name, status)
	}

	// 박스 포커스 스타일 적용
	pluginStyle := m.theme.Box
	monitorStyle := m.theme.Box
	if m.activeTab == 0 {
		pluginStyle = m.theme.FocusBox
	} else if m.activeTab == 1 {
		monitorStyle = m.theme.FocusBox
	}

	pluginBox := pluginStyle.Render(
		fmt.Sprintf("🔌 **Antigravity 플러그인 제어**\n\n%s\n*(단축키 1,2,3으로 즉시 ON/OFF)*", pluginList),
	)

	// 모니터 뷰 빌드
	chargingStr := "사용 중"
	if m.isCharging {
		chargingStr = fmt.Sprintf("충전 중 (%.1fW)", m.batteryWatt)
	}
	speedStr := "측정 대기"
	if m.speedTesting {
		speedStr = "속도 측정 중..."
	} else if m.speedMbps >= 0 {
		speedStr = fmt.Sprintf("%.2f Mbps", m.speedMbps)
	}

	monitorContent := fmt.Sprintf(
		"🖥️ **시스템 상태 모니터**\n\n- 배터리: %d%% (%s)\n- 핑 지연: %s\n- 인터넷 속도: %s\n\n*(R키로 속도 즉시 측정)*",
		m.batteryPct, chargingStr, m.ping, speedStr,
	)
	monitorBox := monitorStyle.Render(monitorContent)

	// 상단 헤더
	header := m.theme.Title.Render("⚡ 나의 불편함 TUI Dashboard v1.0.0") + "\n"

	// 레이아웃 가로 배치
	mainContent := lipgloss.JoinHorizontal(lipgloss.Top, pluginBox, monitorBox)

	// 하단 도움말
	footer := m.theme.Footer.Render(
		"\n [Tab] 포커스 전환 | [1,2,3] 플러그인 토글 | [G] GitHub 열기 | [O] 작업폴더 열기 | [q] 종료",
	)

	return header + mainContent + footer + "\n"
}

func main() {
	p := tea.NewProgram(initialModel())
	if _, err := p.Run(); err != nil {
		fmt.Printf("Error: %v\n", err)
		os.Exit(1)
	}
}
```

- [ ] **Step 3: 컴파일 및 빌드 테스트 수행**

Run: `go build -o my-dashboard`
Expected: 컴파일 오류 없이 `my-dashboard` 바이너리가 정상적으로 생성됨.

- [ ] **Step 4: 로컬에서 테스트 커버리지 및 정적 분석 통과 확인**

Run: `go test -v ./...`
Expected: 모든 단위 테스트 통과 (PASS)

- [ ] **Step 5: Git 최종 커밋 및 머지 준비**

Run:
```bash
git add ui/theme.go main.go
git commit -m "feat: complete modern TUI layout design and integrate background system speed monitoring"
```
