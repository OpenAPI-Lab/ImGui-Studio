# ImGui Studio

[Español](README.md)

ImGui Studio is a native C++17 visual editor and code generator for Dear ImGui. Build a window and its widgets, try the design in preview, and export sources for your application.

## Install a release

For Windows x64, Ubuntu 24.04 x64, and Debian 13 x64, see the
[installation and verification guide](docs/INSTALLATION.en.md). Building
and verifying the macOS DMG is pending.

## Use ImGui Studio

Follow the [editor manual](docs/WORKFLOW.en.md) to learn what each panel
does and build a design step by step. Use the
[widget catalog](docs/WIDGETS.en.md) to choose controls.
The manual also explains [how to open and save with the file picker](docs/WORKFLOW.en.md#open-and-save-with-the-file-picker) and [how to customize Studio's appearance](docs/WORKFLOW.en.md#customize-studio-appearance).

## Build the editor

Use CMake 3.16 or later, Ninja, and a C++17 compiler. The first configure
fetches GLFW, Dear ImGui, and stb from hash-pinned archives.

- On Windows, use GCC in MSYS2 UCRT64 or MSVC in a Visual Studio developer shell. Keep CMake, Ninja, and the compiler on the PATH.
- On Ubuntu or Debian, install `build-essential`, `cmake`, `ninja-build`, `pkg-config`, `libgl-dev`, `xorg-dev`, `libwayland-dev`, `wayland-protocols`, and `libxkbcommon-dev`.
- On macOS, use Clang and the macOS SDK from Xcode Command Line Tools, plus CMake and Ninja.

Install the libcurl 7.85+ development libraries to enable catalog downloads and assistant connections.

```sh
cmake -S . -B build/native -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --target imgui_studio
```

The executable is in `build/native/bin`. Use `imgui_studio.exe` on Windows, `imgui_studio` on Linux, or open the configured `ImGui Studio.app` on macOS. The editor requires a graphical session. On WSL, use WSLg. See the [changelog](CHANGELOG.en.md) for platform verification.

## Exports and packages

- Desktop targets include C++, C, Python, C#, Java, and Rust. C++ supports OpenGL and provides a Vulkan template for Windows and Linux.
- Android, macOS, and iOS have optional C++ export packs with their own build instructions.
- See the [export guide](docs/EXPORTING.en.md) for integrating generated code and the [build guide](docs/BUILD_AND_TEST.en.md) for packaging.

## Documentation

| Guide | What it explains |
| --- | --- |
| [Install ImGui Studio](docs/INSTALLATION.en.md) | Which file to choose and how to verify, install, or remove it. |
| [Build, test, and package](docs/BUILD_AND_TEST.en.md) | Dependencies, CMake flags, test selection, and distribution. |
| [Editor manual](docs/WORKFLOW.en.md) | Start screen, panels, editing, preview, saving, running, and examples. |
| [Canvas and layout](docs/CANVAS.en.md) | Flow, free positioning, gestures, tree rules, and tests. |
| [Title bars and actions](docs/TITLE_BAR.en.md) | Embedded controls, menus, and event handling in the host. |
| [Saving and recovery](docs/RECOVERY.en.md) | Snapshots, restoring, recovery, and save conflicts. |
| [Build diagnostics](docs/BUILD_DIAGNOSTICS.en.md) | Build phases, widget navigation, and what to report. |
| [Widget catalog](docs/WIDGETS.en.md) | What each registered control does and when to use it. |
| [Capabilities](docs/CAPABILITIES.en.md) | Available features, verified platforms, and limitations. |
| [Architecture](docs/ARCHITECTURE.en.md) and [source map](docs/SOURCE_MAP.en.md) | What each part does, why the layers are split, and where to make changes. |
| [How the code works](docs/code/README.en.md) | File contracts, examples, and tests that preserve each behavior. |
| [Project format](docs/PROJECT_FORMAT.en.md) | JSON, history, concurrency, and recovery. |
| [Export and integration](docs/EXPORTING.en.md) | Generated code, toolchains, and optional platform templates. |
| [Vulkan](docs/VULKAN.en.md) | Dependencies, image exports, and presentation tests. |
| [Android](docs/ANDROID.en.md) | Pack setup, APK builds, and emulator checks. |
| [macOS and iOS](docs/APPLE_EXPORTS.en.md) | Metal packs, Xcode, and simulator builds. |
| [Run / Build and diagnostics](docs/RUN_BUILD.en.md) | Profiles, build lifecycle, and widget diagnostics. |
| [Widget library and catalog](docs/WIDGET_LIBRARY.en.md) | Instances, storage, and extension verification. |
| [Assistant and providers](docs/ASSISTANT.en.md) | Connections, credentials, and proposal application. |
| [Reproducible validation](docs/VALIDATION.en.md) | Test commands and what to include in reports. |
| [GitHub checks](docs/CI.en.md) | What CI checks and how to reproduce those checks locally. |
| [Extending the editor](docs/EXTENDING.en.md) and [localization](docs/LOCALIZATION.en.md) | How to add a widget, exporter, or translation. |
| [Technical reference](docs/REFERENCE.en.md) | Detailed editor behavior, diagnostics, providers, and runtime checks. |

## Repository layout

| Directory | Purpose |
| --- | --- |
| `src/app/` and `src/editor/` | Startup, editor UI, canvas, and preview. |
| `src/widgets/` and `src/services/` | Charts, images, catalog, and assistant connections. |
| `include/imguistudio/` | Internal headers grouped by project, widgets, localization, export, build, and services. |
| `templates/` | Standalone exports and platform-specific packs. |
| `tests/` and `examples/` | Automated checks, fixtures, and sample projects. |
| `cmake/` and `packaging/` | Build configuration, distribution, and installer verification. |
| `docs/` | Guides and reference material. |
| `third_party/` | Vendored sources and notices. |
| `tools/` | Repository maintenance tools. |

The [source map](docs/SOURCE_MAP.en.md) shows where to change each feature.

## Contribute

Start with [CONTRIBUTING](CONTRIBUTING.en.md). For sensitive issues, follow [SECURITY](SECURITY.en.md). See the [changelog](CHANGELOG.en.md) for release history.

Project code is licensed under [MIT](LICENSE). Dependencies keep [their own notices](third_party/README.md).
