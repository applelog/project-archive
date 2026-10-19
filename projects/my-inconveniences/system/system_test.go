package system

import (
	"testing"
)

func TestParseBatteryOutput(t *testing.T) {
	// 1. AC Power, charging
	sampleOutput1 := `Now drawing from 'AC Power'
 -InternalBattery-0 (id=1234567)	98%; charging; 0:12 remaining present: true`
	info, err := parseBatteryOutput(sampleOutput1)
	if err != nil {
		t.Fatalf("parseBatteryOutput failed: %v", err)
	}
	if info.Percent != 98 {
		t.Errorf("Expected 98%%, got %d%%", info.Percent)
	}
	if info.PowerSource != "AC" {
		t.Errorf("Expected PowerSource to be AC, got %s", info.PowerSource)
	}
	if info.State != "charging" {
		t.Errorf("Expected State to be charging, got %s", info.State)
	}

	// 2. AC Power, fully charged
	sampleOutput2 := `Now drawing from 'AC Power'
 -InternalBattery-0 (id=1234567)	100%; charged; 0:00 remaining present: true`
	info, err = parseBatteryOutput(sampleOutput2)
	if err != nil {
		t.Fatalf("parseBatteryOutput failed: %v", err)
	}
	if info.Percent != 100 {
		t.Errorf("Expected 100%%, got %d%%", info.Percent)
	}
	if info.PowerSource != "AC" {
		t.Errorf("Expected PowerSource to be AC, got %s", info.PowerSource)
	}
	if info.State != "charged" {
		t.Errorf("Expected State to be charged, got %s", info.State)
	}

	// 3. Battery Power, discharging
	sampleOutput3 := `Now drawing from 'Battery Power'
 -InternalBattery-0 (id=1234567)	85%; discharging; 3:15 remaining present: true`
	info, err = parseBatteryOutput(sampleOutput3)
	if err != nil {
		t.Fatalf("parseBatteryOutput failed: %v", err)
	}
	if info.Percent != 85 {
		t.Errorf("Expected 85%%, got %d%%", info.Percent)
	}
	if info.PowerSource != "Battery" {
		t.Errorf("Expected PowerSource to be Battery, got %s", info.PowerSource)
	}
	if info.State != "discharging" {
		t.Errorf("Expected State to be discharging, got %s", info.State)
	}
}

func TestParseWattageOutput(t *testing.T) {
	sampleIoreg := `"AppleRawAdapterVoltage" = 20000
"AppleRawAdapterCurrent" = 3250`
	watt := parseWattage(sampleIoreg)
	expectedWatt := 20.0 * 3.25 // = 65W
	if watt != expectedWatt {
		t.Errorf("Expected %fW, got %fW", expectedWatt, watt)
	}
}
