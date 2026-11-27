# Build and packaging code

[Español](DISTRIBUTION.md)

CMake wires sources, dependencies, and tests together. CPack adds distribution files. Platform verifiers exercise artifacts from their installed locations.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [CMakeLists.txt](../../CMakeLists.txt) | Declares project version, CMake requirement, and entry point. | Makes the repository configurable from its root. |
| [ImGuiStudio.cmake](../../cmake/ImGuiStudio.cmake) | Registers targets, pinned dependencies, packs, and CTest. | The GUI-off configuration keeps core independent of graphics dependencies. |
| [StudioPackaging.cmake](../../packaging/StudioPackaging.cmake) | Installs the executable, licenses, guides, and resources and configures CPack. | Declares runtime-loaded GLFW libraries so the DEB can resolve them. |
| [make_icons.py](../../packaging/make_icons.py) | Generates the icons expected by each package type. | The build prepares reproducible assets before installation. |
| [imgui-studio.desktop](../../packaging/imgui-studio.desktop) | Declares the Linux application menu entry. | The command and icon must match the installed names. |
| [verify_package.py](../../packaging/verify_package.py) | Checks contents, checksums, and packaged ZIP or DEB startup. | Compares packaged documentation to source. The DEB is installed and removed under an isolated root. |
| [verify_windows_installer.ps1](../../packaging/verify_windows_installer.ps1) | Verifies preflight and, in full mode, installation, shortcuts, registry, and removal. | Preflight detects conflicts before touching an installation. The full cycle needs the appropriate environment. |
| [studio_status_source.hpp.in](../../packaging/studio_status_source.hpp.in) | Embeds the implementation of status widgets. | Export can copy the helper without relying on editor source files at runtime. |
| [studio_knobs_sources.hpp.in](../../packaging/studio_knobs_sources.hpp.in) | Embeds the pinned knob header, implementation, and license. | Catalog checks and generated exports use the reviewed revision. |
| [studio_vulkan_source.hpp.in](../../packaging/studio_vulkan_source.hpp.in) | Embeds the Vulkan host and image adapter. | Emitted projects can build independently. |
| [check_docs.py](../../tools/check_docs.py) | Checks links, language pairs, and source inventory. | Detects moved paths and missing documentation in CI. |
| [checks.yml](../../.github/workflows/checks.yml) | Runs repository verification and core tests. | Contributions can be checked without a graphics session. |
| [desktop.yml](../../.github/workflows/desktop.yml) | Builds the graphical editor on a selected host. | Provides a controlled check of platform build configuration. |
| [apple-exports.yml](../../.github/workflows/apple-exports.yml) | Builds macOS and iOS exports with Xcode. | Checks the application Studio generates and its bundle. |
| [macos-package.yml](../../.github/workflows/macos-package.yml) | Builds and verifies the Intel and Apple Silicon editor DMGs. | Checks actual disk image contents by mounting it and validating the bundle. |

## Verification

Run `python tools/check_docs.py` and the appropriate host [package test](../BUILD_AND_TEST.en.md#distribution). When changing an install layout, inspect the generated artifact and confirm the editor can launch from it. [GitHub workflows](../CI.en.md)

