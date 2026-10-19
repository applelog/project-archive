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
	isProdMode    bool
	theme         ui.Theme
	cursorIndex   int // 0 to 5 for interactive menu items
	plugins       []string

	// System Stats
	batteryPct   int
	batteryWatt  float64
	batterySrc   string
	batteryState string
	ping         string
	speedMbps    float64
	speedTesting bool
}

func initSandbox(sandboxDir string, pluginsList []string) {
	_ = os.MkdirAll(filepath.Join(sandboxDir, "plugins"), 0755)
	_ = os.MkdirAll(filepath.Join(sandboxDir, "plugins_disabled"), 0755)
	for _, p := range pluginsList {
		activePath := filepath.Join(sandboxDir, "plugins", p)
		disabledPath := filepath.Join(sandboxDir, "plugins_disabled", p)
		_, errA := os.Stat(activePath)
		_, errD := os.Stat(disabledPath)
		if os.IsNotExist(errA) && os.IsNotExist(errD) {
			_ = os.MkdirAll(activePath, 0755)
		}
	}
}

func initialModel() model {
	isProd := false
	for _, arg := range os.Args {
		if arg == "--prod" || arg == "--production" {
			isProd = true
		}
	}

	var configDir string
	pluginList := []string{"frontend-design", "karpathy-guidelines", "superpowers"}

	if isProd {
		home, _ := os.UserHomeDir()
		configDir = filepath.Join(home, ".gemini", "config")
	} else {
		configDir = "./.sandbox_plugins"
		initSandbox(configDir, pluginList)
	}

	return model{
		baseConfigDir: configDir,
		isProdMode:    isProd,
		theme:         ui.DefaultTheme(),
		cursorIndex:   0,
		plugins:       pluginList,
		batteryPct:    0,
		batteryWatt:   0.0,
		batterySrc:    "Unknown",
		batteryState:  "Unknown",
		ping:          "측정 중...",
		speedMbps:     -1.0,
		speedTesting:  false,
	}
}

func (m model) Init() tea.Cmd {
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

type systemUpdateMsg struct {
	battInfo system.BatteryInfo
	ping     string
}

func (m model) measureSystem() tea.Cmd {
	return func() tea.Msg {
		battInfo, _ := system.GetBatteryInfo()
		pingVal, _ := system.GetPingLatency()
		return systemUpdateMsg{battInfo: battInfo, ping: pingVal}
	}
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

		case "up", "k":
			m.cursorIndex--
			if m.cursorIndex < 0 {
				m.cursorIndex = 5 // Wrap to bottom (6 items total)
			}

		case "down", "j":
			m.cursorIndex++
			if m.cursorIndex > 5 {
				m.cursorIndex = 0 // Wrap to top
			}

		case "enter", " ":
			// Trigger action based on cursor position
			switch m.cursorIndex {
			case 0:
				_ = plugins.TogglePlugin(m.baseConfigDir, m.plugins[0])
			case 1:
				_ = plugins.TogglePlugin(m.baseConfigDir, m.plugins[1])
			case 2:
				_ = plugins.TogglePlugin(m.baseConfigDir, m.plugins[2])
			case 3:
				_ = exec.Command("open", "https://github.com").Start()
			case 4:
				_ = exec.Command("open", ".").Start()
			case 5:
				// Clean Sandbox and Quit
				_ = os.RemoveAll("./.sandbox_plugins")
				_ = os.Remove("./my-dashboard")
				return m, tea.Quit
			}

		// Direct keyboard hotkeys
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
		case "c", "C":
			_ = os.RemoveAll("./.sandbox_plugins")
			_ = os.Remove("./my-dashboard")
			return m, tea.Quit
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
		return m, tea.Batch(
			doTick(),
			m.measureSystem(),
		)

	case systemUpdateMsg:
		m.batteryPct = msg.battInfo.Percent
		m.batteryWatt = msg.battInfo.Wattage
		m.batterySrc = msg.battInfo.PowerSource
		m.batteryState = msg.battInfo.State
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
	// Color styling definitions
	greenText := lipgloss.NewStyle().Foreground(lipgloss.Color("#00FF00"))
	cyanText := lipgloss.NewStyle().Foreground(lipgloss.Color("#00FFFF")).Bold(true)
	grayText := lipgloss.NewStyle().Foreground(lipgloss.Color("#7F7F7F"))
	whiteBold := lipgloss.NewStyle().Bold(true)

	// 1. ASCII Art Header (noi / 나의)
	logoStr := greenText.Render("  _  _       _ \n | \\| | ___ | |\n |    |(   )| | \n |_|\\_|\\___/|_| ")

	headerInfo := fmt.Sprintf(
		" %s\n %s\n %s",
		cyanText.Render("https://github.com/kimtaemin/my-inconveniences"),
		greenText.Render("나의 불편함 TUI 대시보드 - Deep clean and optimize your workflow."),
		grayText.Render("Mode: Sandbox (Default) | Use --prod for global settings"),
	)
	if m.isProdMode {
		headerInfo = fmt.Sprintf(
			" %s\n %s\n %s",
			cyanText.Render("https://github.com/kimtaemin/my-inconveniences"),
			greenText.Render("나의 불편함 TUI 대시보드 - Deep clean and optimize your workflow."),
			whiteBold.Render("Mode: Production (Global ~/.gemini/config)"),
		)
	}

	// Join logo and description horizontally
	header := lipgloss.JoinHorizontal(lipgloss.Center, logoStr, headerInfo) + "\n\n"

	// Helper to format menu item with cursor
	formatMenuItem := func(index int, numStr, text, desc string) string {
		prefix := "   "
		if m.cursorIndex == index {
			prefix = cyanText.Render(" ➤ ")
			text = cyanText.Render(text)
		}
		return fmt.Sprintf("%s%s %-20s %s\n", prefix, numStr, text, desc)
	}

	// 2. Plugins Section
	pluginSection := whiteBold.Render(" 🔌 Antigravity Plugins (Press 1, 2, 3 or arrow keys + Enter to toggle)\n")
	for i, name := range m.plugins {
		status := "[○] OFF"
		if plugins.CheckPluginStatus(m.baseConfigDir, name) {
			status = greenText.Render("[●] ON ")
		} else {
			status = grayText.Render(status)
		}
		pluginSection += formatMenuItem(i, fmt.Sprintf("%d.", i+1), name, status)
	}

	// 3. System Status Section
	var battStatusStr string
	if m.batterySrc == "AC" {
		if m.batteryState == "charged" {
			battStatusStr = "AC 전원 (완충됨)"
		} else if m.batteryState == "charging" {
			battStatusStr = fmt.Sprintf("AC 전원 (충전 중 - %.1fW)", m.batteryWatt)
		} else {
			battStatusStr = fmt.Sprintf("AC 전원 (대기 중 - %.1fW)", m.batteryWatt)
		}
	} else if m.batterySrc == "Battery" {
		if m.batteryWatt > 0 {
			battStatusStr = fmt.Sprintf("배터리 사용 중 (방전 중 - %.1fW)", m.batteryWatt)
		} else {
			battStatusStr = "배터리 사용 중"
		}
	} else {
		battStatusStr = "확인 불가"
	}

	speedStr := "측정 대기 (R키로 측정)"
	if m.speedTesting {
		speedStr = "속도 측정 중..."
	} else if m.speedMbps >= 0 {
		speedStr = fmt.Sprintf("%.2f Mbps", m.speedMbps)
	}

	systemSection := fmt.Sprintf(
		"\n %s\n   - Power:   %s\n   - Battery: %d%%\n   - Ping:    %s\n   - Speed:   %s\n",
		whiteBold.Render("🖥️ System Status"),
		battStatusStr, m.batteryPct, m.ping, speedStr,
	)

	// 4. Quick Launchers Section
	launcherSection := whiteBold.Render("\n 📂 Quick Launchers (Select or use hotkeys G, O, C)\n")
	launcherSection += formatMenuItem(3, "G.", "Open GitHub Link", cyanText.Render("https://github.com"))
	launcherSection += formatMenuItem(4, "O.", "Open Workspace", grayText.Render("Open Finder in current directory"))
	launcherSection += formatMenuItem(5, "C.", "Clean Sandbox", grayText.Render("Delete all temp files and quit"))

	// 5. Help Footer
	footer := grayText.Render(
		"\n ↑↓ | Enter Toggle/Run | G GitHub | O Folder | R Speedtest | C Clean | Q Quit\n",
	)

	return header + pluginSection + systemSection + launcherSection + footer
}

func main() {
	p := tea.NewProgram(initialModel())
	if _, err := p.Run(); err != nil {
		fmt.Printf("Error: %v\n", err)
		os.Exit(1)
	}
}
