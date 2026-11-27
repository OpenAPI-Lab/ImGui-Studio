# Export to macOS and iOS

[Español](APPLE_EXPORTS.md)

ImGui Studio generates C++ projects for macOS and iOS using their optional
packs. The macOS host uses AppKit. The iOS host uses UIKit scenes with
single-finger input. Both use Metal to render.

## Prepare the packs

```sh
cmake -S . -B build/packs -G Ninja -DBUILD_STUDIO_GUI=OFF -DBUILD_CORE_TEST=OFF
cmake --build build/packs --target studio_macos_pack studio_ios_pack
```

Look under `build/packs/optional-packs/` for the ZIPs. Extract the target you
want to use. Under **Generated code → C++**, select **Export macOS** or
**Export iOS**, point to the directory with `template.version`, and choose a
new output directory.

The generated project contains `ui.hpp`, the Objective-C++ host in
`main.mm`, CMake, `Info.plist`, and `codegen_report.txt`. The host loads
design textures and releases them during renderer teardown. Connect your
application actions after `RenderStudioUI`.

## Build with Xcode

Building the generated project requires a Mac with Xcode, the target SDKs,
and CMake 3.24 or later. From the macOS export directory:

```sh
cmake -S . -B build-macos -G Xcode
cmake --build build-macos --config Debug --target studio_export
open build-macos/Debug/studio_export.app
```

From the iOS export directory, configure the simulator SDK:

```sh
cmake -S . -B build-simulator -G Xcode -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build build-simulator --config Debug --target studio_export
open build-simulator/StudioExport.xcodeproj
```

Select the `studio_export` scheme and an iOS simulator in Xcode. For a
device build, use a separate directory with `CMAKE_OSX_SYSROOT=iphoneos`,
a unique `STUDIO_BUNDLE_ID`, and a signing team in Xcode. On-screen
keyboard and IME support require additional iOS integration.
[Apple toolchains in CMake](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-ios-tvos-visionos-or-watchos)

## What each check covers

`AppleCppExportGenerationTest` extracts both ZIPs and verifies generation
from the same design. Set `-DSTUDIO_TEST_APPLE_BUILD=ON` on a Mac for
`AppleCppExportBuildTest`. It uses Xcode to compile both the macOS project
and iOS Simulator project and checks for the executables and valid
`Info.plist` files.

```sh
cmake -S . -B build/check-apple -G Ninja -DBUILD_STUDIO_GUI=OFF -DSTUDIO_TEST_APPLE_BUILD=ON
cmake --build build/check-apple --target verify_codegen
ctest --test-dir build/check-apple -R '^AppleCppExportBuildTest$' --output-on-failure
```

Run these commands from the ImGui Studio root to build a test export for
each platform.

The [manual Apple export build workflow](../.github/workflows/apple-exports.yml)
configures those builds on a macOS runner. Generation has been verified on
Windows and WSL. The first Xcode build, GUI startup, touch, and device
signing remain pending.
