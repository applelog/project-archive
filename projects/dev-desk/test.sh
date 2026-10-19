#!/bin/zsh
set -euo pipefail
cd "${0:A:h}"
mkdir -p .build/module-cache
swiftc -swift-version 5 -module-cache-path "$PWD/.build/module-cache" \
  -sdk "$(xcrun --sdk macosx --show-sdk-path)" \
  -framework IOKit Sources/Models.swift Sources/Services.swift Tests/StorageChecks.swift \
  -o .build/storage-checks
.build/storage-checks "$PWD/.build/self-check-$(date +%s)"
swiftc -swift-version 5 -module-cache-path "$PWD/.build/module-cache" \
  -sdk "$(xcrun --sdk macosx --show-sdk-path)" \
  -framework IOKit Sources/Models.swift Sources/Services.swift Tests/IntegrationChecks.swift \
  -o .build/integration-checks
.build/integration-checks
