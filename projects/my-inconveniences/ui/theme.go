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
