#!/bin/zsh
set -euo pipefail
cd "${0:A:h}"
mkdir -p .build/module-cache .build/Dev\ Desk.app/Contents/MacOS
cp Resources/Info.plist .build/Dev\ Desk.app/Contents/Info.plist
/usr/libexec/PlistBuddy -c "Add :DevDeskWorkspacePath string $PWD" .build/Dev\ Desk.app/Contents/Info.plist
swiftc -swift-version 5 -O \
  -module-cache-path "$PWD/.build/module-cache" \
  -sdk "$(xcrun --sdk macosx --show-sdk-path)" \
  -framework SwiftUI -framework Charts -framework IOKit \
  Sources/*.swift \
  -o .build/Dev\ Desk.app/Contents/MacOS/DevDesk
echo "Built: $PWD/.build/Dev Desk.app"
