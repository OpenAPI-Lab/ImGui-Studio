# Build, test, and package

[Español](BUILD_AND_TEST.md)


## Desktop editor

Use CMake 3.16+, Ninja, and a C++17 toolchain that can also build the C dependencies. Run from the repository root:

```sh
cmake -S . -B build/editor -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_CORE_TEST=OFF
cmake --build build/editor --target imgui_studio
```

| Host | Tools and output |
| --- | --- |
| Windows | MSYS2 UCRT64 with GCC or a Visual Studio developer shell with MSVC. Output is `build/editor/bin/imgui_studio.exe`. |
| Ubuntu/Debian | `build-essential`, `cmake`, `ninja-build`, `pkg-config`, `libgl-dev`, `xorg-dev`, `libwayland-dev`, `wayland-protocols`, `libxkbcommon-dev`. Output is `build/editor/bin/imgui_studio`. |
| macOS | Clang and the macOS SDK from Xcode Command Line Tools, CMake, and Ninja. Configured output is `build/editor/bin/ImGui Studio.app`. Mac validation is pending. |

CMake fetches GLFW, Dear ImGui, and stb from hash-pinned archives. To reuse local sources, set `FETCHCONTENT_SOURCE_DIR_GLFW`, `FETCHCONTENT_SOURCE_DIR_IMGUI`, or `FETCHCONTENT_SOURCE_DIR_STB`. Local paths are user-supplied inputs. On Linux, install graphics development libraries for the enabled backends. WSL uses WSLg for the application window.

## Options

| Option | Purpose |
| --- | --- |
| `BUILD_STUDIO_GUI` | Enables the executable and its graphics dependencies. |
| `BUILD_CORE_TEST` | Enables core tests and other tests supported by the build configuration. |
| `STUDIO_ASSISTANT_NETWORK`, `STUDIO_CATALOG_NETWORK` | Enable the respective connectivity features when libcurl 7.85+ is available. |
| `STUDIO_ASSISTANT_SECRET_SERVICE` | Enables Linux credential storage when libsecret and a session secret service are available. |
| `STUDIO_PACKAGING` | Enables install rules, CPack, and verifiers. Uses Python 3. |

## Choose tests

```sh
cmake -S . -B build/core -G Ninja -DBUILD_STUDIO_GUI=OFF
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

The core tests run without a display. For GUI builds, run `ctest --test-dir build/editor -N` to discover registered tests. Configure another directory with `BUILD_CORE_TEST=ON` if you started with the Release command. `--smoke-test` draws three frames in a hidden window and `--font-smoke-test` checks interface glyphs.

| Area | Representative tests |
| --- | --- |
| Save, history, and tree edits | `ProjectPersistenceTest`, `SessionRecoveryTest`, `DocumentEditingTest`. |
| Canvas, images, and recovery UI | `NativeCanvasEditingTest`, `NativeImagePreviewTest`, `SessionRecoveryUiTest`. |
| Export | `CppExportGenerationTest`, `AndroidCppExportGenerationTest`, `AppleCppExportGenerationTest`. Optional runtime checks require their respective toolchains. |
| Services | `NativeExtensionCatalogTest`, `AssistantApiTest`, and `AssistantLoopbackTest` when enabled. |
| Packages | `WindowsZipPackageTest` or `LinuxDebPackageTest`, depending on host and tools. |

## Distribution

Configure a separate build with `-DSTUDIO_PACKAGING=ON`, build `imgui_studio`, and run `cpack --config build/package/CPackConfig.cmake -C Release` if you configured in `build/package`. The [desktop packaging reference](REFERENCE.en.md#desktop-packages) lists the generators, build dependencies, and artifact verification commands. To install a release, see [Install ImGui Studio](INSTALLATION.en.md).
