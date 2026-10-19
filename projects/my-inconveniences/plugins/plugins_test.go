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
