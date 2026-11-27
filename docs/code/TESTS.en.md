# Test code and fixtures

[Español](TESTS.md)

Each executable, script, or generated project checks a distinct contract. The cases in `tests/fixtures/` retain representative format inputs and revisions. Projects in `examples/` support checks through the UI.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [verify_core.cpp](../../tests/verify_core.cpp) | Exercises model types and basic generator paths headlessly. | Pins the rules a minimal build must preserve. |
| [verify_persistence.cpp](../../tests/verify_persistence.cpp) | Opens fixtures, saves designs, and compares recovered data. | Checks compatibility and unknown fields when JSON changes. |
| [verify_sessions.cpp](../../tests/verify_sessions.cpp) | Exercises revisions, locks, and recovery with helper processes. | Concurrent access and abrupt exit need real process boundaries. |
| [verify_document_edit.cpp](../../tests/verify_document_edit.cpp) | Checks move plans, restrictions, and tree results. | Isolates model rules from graphics events. |
| [verify_canvas.cpp](../../tests/verify_canvas.cpp) | Feeds Dear ImGui events and checks design gestures. | Catches drift between geometry, target, and the committed edit. |
| [verify_preview.cpp](../../tests/verify_preview.cpp) | Tests rendering, input, geometry, and assets in preview. | Checks that transient interaction leaves the model intact. |
| [verify_session_ui.cpp](../../tests/verify_session_ui.cpp) | Renders the recovery dialog with isolated state. | Checks UI integration with recoverable data. |
| [verify_codegen.cpp](../../tests/verify_codegen.cpp) | Produces export fixtures and checks content and preservation. | Generated code feeds compilation and runtime tests. |
| [studio_chrome_interactions.hpp](../../tests/studio_chrome_interactions.hpp) | Adds window gestures and assertions to emitted cases. | Exercises actions beyond compiling source text. |
| [verify_status_widgets.cpp](../../tests/verify_status_widgets.cpp) | Verifies state and helper packaging for status widgets. | The helper is included only for designs that need it. |
| [verify_extensions.cpp](../../tests/verify_extensions.cpp) | Checks extension installation, library behavior, and exports. | Checks that changed content cannot activate an extension. |
| [verify_process.cpp](../../tests/verify_process.cpp) | Tests process arguments, output, EOF, and cancellation. | Cases cross the process boundary and exercise host-specific rules. |
| [verify_build.cpp](../../tests/verify_build.cpp) | Drives builds and startup through the real controller. | Checks result validity for the design and actionable failure output. |
| [verify_build_profiles.cpp](../../tests/verify_build_profiles.cpp) | Checks persistent profiles, revisions, and project attachment. | Supports configuration changes without losing formats or concurrent-session data. |
| [verify_build_diagnostics.cpp](../../tests/verify_build_diagnostics.cpp) | Tests compiler formats, fragments, and range-based selection. | Parser changes must preserve text and mappings within bounds. |
| [verify_assistant.cpp](../../tests/verify_assistant.cpp) | Checks request contracts and proposal validation. | Enables checking the code with local inputs without live providers. |
| [verify_assistant_http.py](../../tests/verify_assistant_http.py) | Serves local responses for the assistant's wire formats. | The test verifies request and response handling without real credentials. |
| [verify_python_export.py](../../tests/verify_python_export.py) | Exercises Python artifacts with configured tools. | Checks output with its interpreter and binding. |
| [verify_android_export.cmake](../../tests/verify_android_export.cmake) | Generates a project from the Android pack and inspects its files. | Checks bundling independently of APK installation. |
| [verify_apple_export.cmake](../../tests/verify_apple_export.cmake) | Generates macOS/iOS projects and can build both with Xcode. | Separates generated-file checks from SDK validation. |
| [verify_c_export.cmake](../../tests/verify_c_export.cmake) | Configures, builds, and runs C11 cases with cimgui. | Checks that the C++ bridge supports the C host and interface. |
| [verify_csharp_export.cmake](../../tests/verify_csharp_export.cmake) | Builds .NET projects and uses the emitted test mode. | Runtime checks code and state the emitter cannot run. |
| [verify_java_export.cmake](../../tests/verify_java_export.cmake) | Compiles and starts Java with the configured JDK and bindings. | Checks resources, execution, and target compatibility. |
| [verify_rust_export.cmake](../../tests/verify_rust_export.cmake) | Builds Cargo projects and runs their startup test. | Checks Rust code and any native bridges the project requires. |
| [verify_vulkan_export.cmake](../../tests/verify_vulkan_export.cmake) | Builds and runs the Vulkan fixture. | Checks configuration, presentation, and images through a real backend. |

## Choose a test

Start with the case closest to the change. JSON changes need an older fixture and a rejected input. Rendering changes need a Dear ImGui context and, if emitted code changes, a target runtime check. Installation changes need the actual CPack artifact. Reproduce the documented CTest entries and record the SDK used. [Validation guide](../VALIDATION.en.md) · [Fixture provenance](../../tests/fixtures/manifest.json)

