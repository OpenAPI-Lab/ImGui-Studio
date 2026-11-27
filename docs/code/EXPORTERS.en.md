# Code generation and bundling

[Español](EXPORTERS.md)

Generators produce code, diagnostics, and line ranges. Bundles add a host, assets, and instructions and are written to a new destination. The code panel can produce source text without invoking a compiler.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [studio_codegen.hpp](../../include/imguistudio/export/studio_codegen.hpp) | Exposes the C++ generator through one include. | Keeps a stable entry point even though implementation lives in another header. |
| [studio_codegen_native.hpp](../../include/imguistudio/export/studio_codegen_native.hpp) | `generateCppDetailed` walks nodes and builds source, state, diagnostics, and ranges in `GeneratedCpp`. | The viewer, bundles, and diagnostic navigation depend on these results. Check escaped text, names, and ranges when changing the emitter. |
| [studio_cpp_export.hpp](../../include/imguistudio/export/studio_cpp_export.hpp) | `CppBundle` collects files. `generateCppBundle` selects a renderer and `writeNewExportDirectory` writes the output. | The renderer controls the host and texture path. A new destination protects existing projects. |
| [studio_c_export.hpp](../../include/imguistudio/export/studio_c_export.hpp) | Emits the C11 interface, C host, and C++ cimgui bridge. | UI source must compile as C11. The bridge owns C++ dependencies and preserves the ABI boundary. |
| [studio_python_export.hpp](../../include/imguistudio/export/studio_python_export.hpp) | Emits `main.py`, state, and the imgui-bundle requirement. | The runtime loads the expected binding. Check syntax and execution as well as emitted text. |
| [studio_csharp_export.hpp](../../include/imguistudio/export/studio_csharp_export.hpp) | Creates the ImGui.NET interface, .NET project and host, and any required assets. | The host loads resources and owns the context. Check headless and window paths for binding changes. |
| [studio_java_export.hpp](../../include/imguistudio/export/studio_java_export.hpp) | Emits `StudioUI.java`, `Main.java`, a POM, and classpath resources. | Execution needs the correct JDK, JARs, and natives. Preserve resource-to-output-path mapping. |
| [studio_rust_export.hpp](../../include/imguistudio/export/studio_rust_export.hpp) | Emits `ui.rs`, `main.rs`, Cargo files, and native bridges when needed. | The interface keeps state and resource lifetimes separate from the host. Optional features must compile in their intended configuration. |
| [studio_android_export.hpp](../../include/imguistudio/export/studio_android_export.hpp) | `generateAndroidCppBundle` validates the pack and integrates the design with Gradle and NativeActivity. | Required files come from the descriptor. Validation precedes writing the destination. |
| [studio_apple_export.hpp](../../include/imguistudio/export/studio_apple_export.hpp) | `generateAppleCppBundle` combines UI, the Metal pack, and macOS or iOS target data. | Preserve generated fields and each platform's contract. Bundle generation and Xcode compilation are separate checks. |

## Example

`generateCppBundle(project, CppRenderer::OpenGL)` returns the set of files. Show diagnostics before calling `writeGeneratedBundle(bundle, destination)`. The report includes IDs and ranges for investigating coverage gaps. For integrating `RenderStudioUI(state)` into another host, see the [buildable example](../../examples/title-bar-action/README.en.md).

## Verification

Run `CppExportGenerationTest`, the `GeneratedCpp_*` tests, and Android/Apple generation tests. For each language, select its runtime test from [validation](../VALIDATION.en.md).

