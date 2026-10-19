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

// BatteryInfo holds parsed battery status details.
type BatteryInfo struct {
	Percent     int
	Wattage     float64
	PowerSource string // "AC" or "Battery"
	State       string // "charged", "charging", "discharging", "finishing charge", etc.
}

// GetBatteryInfo executes pmset and ioreg to determine battery capacity and power source status.
func GetBatteryInfo() (BatteryInfo, error) {
	// 1. Get battery percentage, power source, and state
	cmdBatt := exec.Command("pmset", "-g", "batt")
	var outBatt bytes.Buffer
	cmdBatt.Stdout = &outBatt
	if err := cmdBatt.Run(); err != nil {
		return BatteryInfo{}, err
	}
	info, err := parseBatteryOutput(outBatt.String())
	if err != nil {
		return BatteryInfo{}, err
	}

	// 2. Get charging wattage
	cmdIoreg := exec.Command("ioreg", "-rw0", "-r", "-c", "AppleSmartBattery")
	var outIoreg bytes.Buffer
	cmdIoreg.Stdout = &outIoreg
	_ = cmdIoreg.Run() // If it fails, default to 0W without erroring out

	info.Wattage = parseWattage(outIoreg.String())

	return info, nil
}

func parseBatteryOutput(output string) (BatteryInfo, error) {
	var info BatteryInfo

	// Parse Power Source
	if strings.Contains(output, "drawing from 'AC Power'") {
		info.PowerSource = "AC"
	} else if strings.Contains(output, "drawing from 'Battery Power'") {
		info.PowerSource = "Battery"
	} else {
		info.PowerSource = "Unknown"
	}

	// Parse Percent and State
	re := regexp.MustCompile(`(\d+)%;\s*([^;]+);`)
	matches := re.FindStringSubmatch(output)
	if len(matches) < 3 {
		return info, fmt.Errorf("could not parse battery output")
	}

	pct, err := strconv.Atoi(matches[1])
	if err != nil {
		return info, err
	}
	info.Percent = pct
	info.State = strings.TrimSpace(matches[2])

	return info, nil
}

func parseWattage(output string) float64 {
	// Parse AppleRawAdapterVoltage(mV) and AppleRawAdapterCurrent(mA)
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

// GetPingLatency measures the connection delay to Google's public DNS.
func GetPingLatency() (string, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, "ping", "-c", "1", "-t", "1", "8.8.8.8")
	var out bytes.Buffer
	cmd.Stdout = &out
	if err := cmd.Run(); err != nil {
		return "오프라인", err
	}

	// Parse ping result: time=24.5 ms
	re := regexp.MustCompile(`time=([\d\.]+)\s*ms`)
	match := re.FindStringSubmatch(out.String())
	if len(match) < 2 {
		return "오프라인", fmt.Errorf("parse ping error")
	}
	return match[1] + " ms", nil
}

// GetDownloadSpeed measures real download bandwidth in Mbps using Cloudflare speed test CDN.
func GetDownloadSpeed() (float64, error) {
	// 1MB test file
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
