# Source map

[Español](SOURCE_MAP.md)

The [architecture](ARCHITECTURE.en.md) explains why these layers are arranged this way. The tables identify where to make a change and which code owns the behavior.

## Root and distribution

| File or directory | Responsibility |
| --- | --- |
| [`CMakeLists.txt`](../CMakeLists.txt) | Public entry point for configuring from the root. |
| [`tools/check_docs.py`](../tools/check_docs.py) | Checks documentation links and source inventory. |
| [`.github/workflows/checks.yml`](../.github/workflows/checks.yml) | Runs repository checks on Windows and Linux. |
| [`.github/workflows/desktop.yml`](../.github/workflows/desktop.yml) | Builds and checks the GUI editor on demand for a selected platform. |
| [`.github/workflows/apple-exports.yml`](../.github/workflows/apple-exports.yml) | Builds test exports for macOS and iOS Simulator with Xcode. |
| [`cmake/ImGuiStudio.cmake`](../cmake/ImGuiStudio.cmake) | Defines targets, pinned dependencies, templates, tests, and executable settings. |
| [`packaging/StudioPackaging.cmake`](../packaging/StudioPackaging.cmake) | Installs binaries, docs, licenses, and icons and configures CPack. |
| [`packaging/make_icons.py`](../packaging/make_icons.py) | Generates packaging icons. |
| [`packaging/imgui-studio.desktop`](../packaging/imgui-studio.desktop) | Editor entry for Linux desktop application menus. |
| [`packaging/verify_package.py`](../packaging/verify_package.py) | Checks SHA-256, licenses, ZIP startup, and isolated DEB lifecycle. |
| [`packaging/verify_windows_installer.ps1`](../packaging/verify_windows_installer.ps1) | Checks the EXE, shortcuts, registry, and removal through an install lifecycle. |
| [`packaging/studio_knobs_sources.hpp.in`](../packaging/studio_knobs_sources.hpp.in) | Embeds pinned knob sources and license. |
| [`packaging/studio_status_source.hpp.in`](../packaging/studio_status_source.hpp.in) | Embeds status controls for generated projects. |
| [`packaging/studio_vulkan_source.hpp.in`](../packaging/studio_vulkan_source.hpp.in) | Embeds the Vulkan host and image implementation. |

## Editor, resources, and services

| File or directory | Responsibility |
| --- | --- |
| [`src/app/main.cpp`](../src/app/main.cpp) | Startup, frame loop, and window and font smoke modes. |
| [`src/editor/studio_ui.hpp`](../src/editor/studio_ui.hpp) | StudioApp state, editing, windows, and editor services. |
| [`src/editor/studio_ui.cpp`](../src/editor/studio_ui.cpp) | Navigation, dialogs, files, library, export, and Build panel. |
| [`src/editor/studio_canvas.hpp`](../src/editor/studio_canvas.hpp) | Canvas observations and gesture contracts. |
| [`src/editor/studio_canvas.cpp`](../src/editor/studio_canvas.cpp) | Coordinates, dragging, selection, hierarchy, and edit commits. |
| [`src/editor/studio_preview.hpp`](../src/editor/studio_preview.hpp) | Preview state and observable data. |
| [`src/editor/studio_preview.cpp`](../src/editor/studio_preview.cpp) | Draws controls, exposes properties, and manages transient interaction. |
| [`src/app/studio_fonts.hpp`](../src/app/studio_fonts.hpp) | Loads host or state-directory fonts and builds the atlas. |
| [`src/widgets/studio_images.hpp`](../src/widgets/studio_images.hpp) | Preview texture cache contract. |
| [`src/widgets/studio_images.cpp`](../src/widgets/studio_images.cpp) | Decodes assets and manages editor textures. |
| [`src/widgets/studio_chart2d.hpp`](../src/widgets/studio_chart2d.hpp) | Renders 2D charts in preview. |
| [`src/widgets/studio_chart3d.hpp`](../src/widgets/studio_chart3d.hpp) | Renders 3D charts in preview. |
| [`src/services/studio_extensions.hpp`](../src/services/studio_extensions.hpp) | Catalog types and entries consumed by the editor. |
| [`src/services/studio_extensions.cpp`](../src/services/studio_extensions.cpp) | Verifies pinned content and persists installation state. |
| [`src/services/studio_assistant_panel.hpp`](../src/services/studio_assistant_panel.hpp) | Assistant panel and conversation state and contract. |
| [`src/services/studio_assistant_panel.cpp`](../src/services/studio_assistant_panel.cpp) | Requests, settings, credential dialogs, and proposal review. |
| [`src/services/studio_assistant_http.hpp`](../src/services/studio_assistant_http.hpp) | Transport interface used by the assistant. |
| [`src/services/studio_assistant_http.cpp`](../src/services/studio_assistant_http.cpp) | libcurl HTTP connection and response handling. |
| [`src/services/studio_assistant_vault.hpp`](../src/services/studio_assistant_vault.hpp) | Credential vault interface. |
| [`src/services/studio_assistant_vault.cpp`](../src/services/studio_assistant_vault.cpp) | WinCred, Secret Service, and Keychain adapters. |

## Model, files, and operations

| File or directory | Responsibility |
| --- | --- |
| [`include/imguistudio/project/studio_types.hpp`](../include/imguistudio/project/studio_types.hpp) | Defines project, node, properties, type, and persistent state. |
| [`include/imguistudio/project/studio_widget_registry.inc`](../include/imguistudio/project/studio_widget_registry.inc) | Maps types to names and internal catalog metadata. |
| [`include/imguistudio/project/studio_serialization.hpp`](../include/imguistudio/project/studio_serialization.hpp) | Converts JSON and model, validates bounds, and preserves extra fields. |
| [`include/imguistudio/project/studio_document_edit.hpp`](../include/imguistudio/project/studio_document_edit.hpp) | Validates insert, move, reorder, and position operations. |
| [`include/imguistudio/project/studio_project_io.hpp`](../include/imguistudio/project/studio_project_io.hpp) | Bounded reads, temporary-file saves, and change checks. |
| [`include/imguistudio/project/studio_snapshots.hpp`](../include/imguistudio/project/studio_snapshots.hpp) | Versions, comparison, and retention policy. |
| [`include/imguistudio/project/studio_session_store.hpp`](../include/imguistudio/project/studio_session_store.hpp) | Checkpoints, durable history, locks, and state directory. |
| [`include/imguistudio/widgets/studio_widget_library.hpp`](../include/imguistudio/widgets/studio_widget_library.hpp) | Reusable widget format and storage. |
| [`include/imguistudio/widgets/studio_window_appearance.hpp`](../include/imguistudio/widgets/studio_window_appearance.hpp) | Fields and values for window customization. |
| [`include/imguistudio/widgets/studio_image_data.hpp`](../include/imguistudio/widgets/studio_image_data.hpp) | Validates and handles images embedded in the document. |
| [`include/imguistudio/widgets/studio_numeric_format.hpp`](../include/imguistudio/widgets/studio_numeric_format.hpp) | Constrains numeric formats used by controls. |
| [`include/imguistudio/widgets/studio_knob.hpp`](../include/imguistudio/widgets/studio_knob.hpp) | Knob configuration shared by editor and generator. |
| [`include/imguistudio/widgets/studio_status_widgets.hpp`](../include/imguistudio/widgets/studio_status_widgets.hpp) | C++ toggle, spinner, and circular progress primitives. |
| [`include/imguistudio/localization/studio_locale.hpp`](../include/imguistudio/localization/studio_locale.hpp) | Loads catalogs, selects languages, and resolves keys. |
| [`include/imguistudio/localization/studio_locale_strings.inc`](../include/imguistudio/localization/studio_locale_strings.inc) | Base keys and Spanish and English text. |
| [`include/imguistudio/localization/studio_locale_extra.inc`](../include/imguistudio/localization/studio_locale_extra.inc) | French, Korean, Japanese, and Simplified Chinese translations. |
| [`include/imguistudio/services/studio_assistant_api.hpp`](../include/imguistudio/services/studio_assistant_api.hpp) | Models, URL rules, and provider-specific formats. |
| [`include/imguistudio/services/studio_assistant_proposal.hpp`](../include/imguistudio/services/studio_assistant_proposal.hpp) | Validates a proposal and applies it as a document edit. |
| [`include/imguistudio/build/studio_build_profiles.hpp`](../include/imguistudio/build/studio_build_profiles.hpp) | Format and persistence for project build profiles. |
| [`include/imguistudio/build/studio_build.hpp`](../include/imguistudio/build/studio_build.hpp) | Coordinates temporary export, Build, Run, and the startup check. |
| [`include/imguistudio/build/studio_build_diagnostics.hpp`](../include/imguistudio/build/studio_build_diagnostics.hpp) | Parses diagnostics and maps files, lines, and nodes. |
| [`include/imguistudio/build/studio_process.hpp`](../include/imguistudio/build/studio_process.hpp) | Child processes with individual arguments, captured output, and cancellation. |

## Generators and templates

| File or directory | Responsibility |
| --- | --- |
| [`include/imguistudio/export/studio_codegen.hpp`](../include/imguistudio/export/studio_codegen.hpp) | Include entry point for the main generator. |
| [`include/imguistudio/export/studio_codegen_native.hpp`](../include/imguistudio/export/studio_codegen_native.hpp) | Emits native UI, state structs, and diagnostics. |
| [`include/imguistudio/export/studio_cpp_export.hpp`](../include/imguistudio/export/studio_cpp_export.hpp) | Assembles C++ projects and writes files to a new destination. |
| [`include/imguistudio/export/studio_c_export.hpp`](../include/imguistudio/export/studio_c_export.hpp) | C11 scaffold, cimgui dependencies, and example runtime. |
| [`include/imguistudio/export/studio_python_export.hpp`](../include/imguistudio/export/studio_python_export.hpp) | CPython/imgui-bundle program, assets, and dependencies. |
| [`include/imguistudio/export/studio_csharp_export.hpp`](../include/imguistudio/export/studio_csharp_export.hpp) | .NET 8 project, ImGui.NET interface, and assets. |
| [`include/imguistudio/export/studio_java_export.hpp`](../include/imguistudio/export/studio_java_export.hpp) | Java classes and generated build project, bindings, and assets. |
| [`include/imguistudio/export/studio_rust_export.hpp`](../include/imguistudio/export/studio_rust_export.hpp) | Cargo project, Rust state, and bridges where needed. |
| [`include/imguistudio/export/detail/studio_codegen_vulkan.hpp`](../include/imguistudio/export/detail/studio_codegen_vulkan.hpp) | Emits the GLFW and Vulkan application entry point. |
| [`templates/studio_vulkan.hpp`](../templates/studio_vulkan.hpp) | Vulkan host, surface lifecycle, and presentation. |
| [`templates/studio_vulkan_images.hpp`](../templates/studio_vulkan_images.hpp) | Vulkan texture upload and lifetime. |
| [`include/imguistudio/export/studio_android_export.hpp`](../include/imguistudio/export/studio_android_export.hpp) | Validates and combines the Android pack with a design. |
| [`templates/android`](../templates/android) | Descriptor, activity, NDK, Gradle, and OpenGL ES host. |
| [`include/imguistudio/export/studio_apple_export.hpp`](../include/imguistudio/export/studio_apple_export.hpp) | Validates the pack and assembles macOS/iOS projects. |
| [`templates/apple`](../templates/apple) | Shared Metal host, texture adapter, and platform descriptors. |
| [`include/imguistudio/export/detail/studio_codegen_titlebar.hpp`](../include/imguistudio/export/detail/studio_codegen_titlebar.hpp) | Title bar helpers for native exports. |
| [`include/imguistudio/export/detail/studio_managed_titlebar.hpp`](../include/imguistudio/export/detail/studio_managed_titlebar.hpp) | Shared representation of title bar controls for bindings. |
| [`include/imguistudio/export/detail/studio_codegen_image.hpp`](../include/imguistudio/export/detail/studio_codegen_image.hpp) | Image helpers and adapters for generated sources. |
| [`include/imguistudio/export/detail/studio_managed_images.hpp`](../include/imguistudio/export/detail/studio_managed_images.hpp) | Shared image support for bindings. |
| [`include/imguistudio/export/detail/studio_images_java_rust.hpp`](../include/imguistudio/export/detail/studio_images_java_rust.hpp) | Image adapters for Java and Rust targets. |
| [`include/imguistudio/export/detail/studio_codegen_animation.hpp`](../include/imguistudio/export/detail/studio_codegen_animation.hpp) | Emits native animation helpers. |
| [`include/imguistudio/export/detail/studio_managed_animation.hpp`](../include/imguistudio/export/detail/studio_managed_animation.hpp) | Shared animation parameters and logic across bindings. |
| [`include/imguistudio/export/detail/studio_animation_java.hpp`](../include/imguistudio/export/detail/studio_animation_java.hpp) | Generated Java animation implementation. |
| [`include/imguistudio/export/detail/studio_animation_rust.hpp`](../include/imguistudio/export/detail/studio_animation_rust.hpp) | Generated Rust animation implementation. |
| [`include/imguistudio/export/detail/studio_codegen_chart2d.hpp`](../include/imguistudio/export/detail/studio_codegen_chart2d.hpp) | Generated C/C++ helpers for 2D charts. |
| [`include/imguistudio/export/detail/studio_codegen_chart3d.hpp`](../include/imguistudio/export/detail/studio_codegen_chart3d.hpp) | Generated C/C++ helpers for 3D charts. |
| [`include/imguistudio/export/detail/studio_chart2d_csharp.hpp`](../include/imguistudio/export/detail/studio_chart2d_csharp.hpp) | Generated 2D chart rendering for C#. |
| [`include/imguistudio/export/detail/studio_chart3d_csharp.hpp`](../include/imguistudio/export/detail/studio_chart3d_csharp.hpp) | Generated 3D chart rendering for C#. |
| [`include/imguistudio/export/detail/studio_titlebar_csharp.hpp`](../include/imguistudio/export/detail/studio_titlebar_csharp.hpp) | Generated title bar support for C#. |
| [`include/imguistudio/export/detail/studio_chart2d_java.hpp`](../include/imguistudio/export/detail/studio_chart2d_java.hpp) | Generated 2D chart rendering for Java. |
| [`include/imguistudio/export/detail/studio_chart3d_java.hpp`](../include/imguistudio/export/detail/studio_chart3d_java.hpp) | Generated 3D chart rendering for Java. |
| [`include/imguistudio/export/detail/studio_titlebar_java.hpp`](../include/imguistudio/export/detail/studio_titlebar_java.hpp) | Generated title bar support for Java. |
| [`include/imguistudio/export/detail/studio_chart2d_python.hpp`](../include/imguistudio/export/detail/studio_chart2d_python.hpp) | Generated 2D chart rendering for Python. |
| [`include/imguistudio/export/detail/studio_chart3d_python.hpp`](../include/imguistudio/export/detail/studio_chart3d_python.hpp) | Generated 3D chart rendering for Python. |
| [`include/imguistudio/export/detail/studio_titlebar_python.hpp`](../include/imguistudio/export/detail/studio_titlebar_python.hpp) | Generated title bar support for Python. |
| [`include/imguistudio/export/detail/studio_chart2d_rust.hpp`](../include/imguistudio/export/detail/studio_chart2d_rust.hpp) | Generated 2D chart rendering for Rust. |
| [`include/imguistudio/export/detail/studio_chart3d_rust.hpp`](../include/imguistudio/export/detail/studio_chart3d_rust.hpp) | Generated 3D chart rendering for Rust. |
| [`include/imguistudio/export/detail/studio_titlebar_rust.hpp`](../include/imguistudio/export/detail/studio_titlebar_rust.hpp) | Generated title bar support for Rust. |

## Tests and reference data

| File or directory | Responsibility |
| --- | --- |
| [`tests/verify_core.cpp`](../tests/verify_core.cpp) | Model and basic generation contracts. |
| [`tests/verify_persistence.cpp`](../tests/verify_persistence.cpp) | Fixtures, reads, writes, and unknown data. |
| [`tests/verify_sessions.cpp`](../tests/verify_sessions.cpp) | Recovery, history revisions, and concurrent sessions. |
| [`tests/verify_document_edit.cpp`](../tests/verify_document_edit.cpp) | Tree edits and validation. |
| [`tests/verify_canvas.cpp`](../tests/verify_canvas.cpp) | Canvas gestures, selection, and editing. |
| [`tests/verify_preview.cpp`](../tests/verify_preview.cpp) | Interaction, geometry, and image rendering. |
| [`tests/verify_session_ui.cpp`](../tests/verify_session_ui.cpp) | Recovery dialog behavior. |
| [`tests/verify_codegen.cpp`](../tests/verify_codegen.cpp) | Language generation, fixtures, and test outputs. |
| [`tests/studio_chrome_interactions.hpp`](../tests/studio_chrome_interactions.hpp) | Helps verify custom title bars in generated projects. |
| [`tests/verify_status_widgets.cpp`](../tests/verify_status_widgets.cpp) | Projects with toggles, spinners, and circular progress. |
| [`tests/verify_extensions.cpp`](../tests/verify_extensions.cpp) | Verified extension installation, library, and export. |
| [`tests/verify_process.cpp`](../tests/verify_process.cpp) | Child-process arguments, output, and cancellation. |
| [`tests/verify_build.cpp`](../tests/verify_build.cpp) | Build and run through the editor controller. |
| [`tests/verify_build_profiles.cpp`](../tests/verify_build_profiles.cpp) | Profile persistence, destinations, and compatibility. |
| [`tests/verify_build_diagnostics.cpp`](../tests/verify_build_diagnostics.cpp) | Error parsing and navigation. |
| [`tests/verify_assistant.cpp`](../tests/verify_assistant.cpp) | Assistant contracts and proposal validation. |
| [`tests/verify_assistant_http.py`](../tests/verify_assistant_http.py) | Loopback test server and provider wire formats. |
| [`tests/verify_python_export.py`](../tests/verify_python_export.py) | Python artifact tests. |
| [`tests/fixtures`](../tests/fixtures) | Compatibility cases and regression input data. |
| [`examples`](../examples) | Sample projects for manually trying the editor. |
| [`third_party/README.md`](../third_party/README.md) | Origin and notices for vendored dependencies. |
| [`tests/verify_android_export.cmake`](../tests/verify_android_export.cmake) | Orchestrates the Android pack generation check. |
| [`tests/verify_apple_export.cmake`](../tests/verify_apple_export.cmake) | Orchestrates the macOS/iOS pack generation check. |
| [`tests/verify_c_export.cmake`](../tests/verify_c_export.cmake) | Orchestrates the C11 runtime check. |
| [`tests/verify_csharp_export.cmake`](../tests/verify_csharp_export.cmake) | Orchestrates the C# runtime check. |
| [`tests/verify_java_export.cmake`](../tests/verify_java_export.cmake) | Orchestrates the Java runtime check. |
| [`tests/verify_rust_export.cmake`](../tests/verify_rust_export.cmake) | Orchestrates the Rust runtime check. |
| [`tests/verify_vulkan_export.cmake`](../tests/verify_vulkan_export.cmake) | Orchestrates the C++ Vulkan runtime check. |
