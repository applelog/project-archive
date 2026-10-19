package plugins

import (
	"os"
	"path/filepath"
)

// CheckPluginStatus checks if a plugin directory exists under the baseDir/plugins path.
func CheckPluginStatus(baseDir string, name string) bool {
	activePath := filepath.Join(baseDir, "plugins", name)
	info, err := os.Stat(activePath)
	return err == nil && info.IsDir()
}

// TogglePlugin moves a plugin directory from plugins to plugins_disabled, or vice-versa.
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
			// If it doesn't exist in either, create it under active to initialize
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
