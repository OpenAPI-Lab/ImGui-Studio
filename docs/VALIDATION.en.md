# Reproducible validation

[Español](VALIDATION.md)


Record the tested commit, OS, architecture, compiler, graphics backend or export target, commands, and result. Use a copy of an [example project](WORKFLOW.en.md#example-projects) and a separate output directory so others can reproduce it.

## Model and exporter changes

```sh
cmake -S . -B build/check-core -G Ninja -DBUILD_STUDIO_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build/check-core --target verify_core verify_persistence verify_sessions verify_document_edit verify_codegen
ctest --test-dir build/check-core -R 'CoreAndCodegenTest|ProjectPersistenceTest|SessionRecoveryTest|DocumentEditingTest|CppExportGenerationTest' --output-on-failure
```

This selection includes C++, Android, and Apple generation. For format changes, open an earlier project, save a copy, and verify affected fields. The tests use existing fixtures and build-local output directories.

## Service and tool changes

```sh
cmake -S . -B build/check-editor -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/check-editor --target verify_assistant verify_extensions verify_build_profiles verify_build_diagnostics
ctest --test-dir build/check-editor -R 'AssistantApiTest|AssistantLoopbackTest|NativeExtensionCatalogTest|BuildProfilesPersistenceTest|BuildDiagnosticsTest' --output-on-failure
```

Use `ctest --test-dir build/check-editor -N` to see whether Loopback was registered. It requires libcurl and Python and simulates the provider on the local machine.

## Graphics changes

Build `imgui_studio` and run `--smoke-test`. For visible changes, also open the sample project and repeat the affected gesture or state transition. On Ubuntu under WSLg, these commands exercise both paths:

```sh
env -u DISPLAY XDG_SESSION_TYPE=wayland ./build/check-editor/bin/imgui_studio --smoke-test
env -u WAYLAND_DISPLAY XDG_SESSION_TYPE=x11 ./build/check-editor/bin/imgui_studio --smoke-test
```

The second run uses XWayland on WSLg. For Android, specify whether the APK was built and whether it ran. Record the API, architecture, and device or emulator when applicable. Check startup, input, images, rotation, and resume for affected changes. Track source generation and execution separately for macOS and iOS.

On Windows with Visual Studio 2026 and a CMake version that recognizes its generator, run this local graphics check:

```powershell
cmake -S . -B build/check-msvc -G "Visual Studio 18 2026" -A x64 -DBUILD_CORE_TEST=OFF -DSTUDIO_ASSISTANT_NETWORK=OFF -DSTUDIO_CATALOG_NETWORK=OFF
cmake --build build/check-msvc --config Release --target imgui_studio --parallel 1
$editor = Start-Process .\build\check-msvc\bin\Release\imgui_studio.exe -ArgumentList '--smoke-test' -WindowStyle Hidden -Wait -PassThru
$editor.ExitCode
```

A successful check returns exit code `0`. The MSVC executable is under the selected configuration's subdirectory. Optional networking is disabled for this graphics check.

See the [support matrix](CAPABILITIES.en.md) for local evidence and pending checks.

### Project picker and appearance

Run `SessionRecoveryUiTest` in a build with GUI and tests enabled. It covers built-in picker selection, subfolders, filenames with accents, invalid loads, cancellation from an unsaved-changes prompt, and palette persistence. To exercise the native dialog, launch a regular Windows instance and repeat **Open project**, **Save As**, the overwrite prompt, and **Cancel**. Record which picker appeared.

For appearance changes, choose a dark palette and **Porcelain** in [Studio appearance](WORKFLOW.en.md#customize-studio-appearance). Change the radius and reopen the editor to check persistence. Use the same project before and after to check that its own colors and radii stay the same in Preview.

## Run a complete C++ export

In a build with the GUI and tests enabled, run:

```sh
cmake --build build/check-editor --target verify_build generated_window_chrome generated_full_window_chrome
ctest --test-dir build/check-editor -R '^(NativeProjectBuildTest|GeneratedCpp_window_chrome|GeneratedCppWindowChromeSmoke)$' --output-on-failure
```

The first test exercises the Run / Build controller with CMake and Ninja. It
checks compilation, startup, cancellation, design changes, and toolchain
failures. The other tests run the generated title bar and its host. To connect
an application action to a title bar button, see the [Help example](../examples/title-bar-action/README.en.md).

For Vulkan renderer changes, run both checks in the [Vulkan guide](VULKAN.en.md#repository-tests).
