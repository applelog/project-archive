// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "RadioApp",
    platforms: [
        .iOS(.v17),
        .macOS(.v14)
    ],
    products: [
        .library(name: "RadioApp", targets: ["RadioApp"])
    ],
    targets: [
        .target(
            name: "RadioApp",
            dependencies: [],
            path: "Sources/RadioApp",
            exclude: ["Info.plist"]
        ),
        .testTarget(
            name: "RadioAppTests",
            dependencies: ["RadioApp"],
            path: "Tests/RadioAppTests",
            swiftSettings: [
                .unsafeFlags([
                    "-F", "/Library/Developer/CommandLineTools/Library/Developer/Frameworks",
                    "-Xfrontend", "-disable-cross-import-overlays"
                ])
            ],
            linkerSettings: [
                .unsafeFlags([
                    "-F", "/Library/Developer/CommandLineTools/Library/Developer/Frameworks",
                    "-framework", "Testing",
                    "-Xlinker", "-rpath",
                    "-Xlinker", "/Library/Developer/CommandLineTools/Library/Developer/Frameworks"
                ])
            ]
        )
    ]
)
