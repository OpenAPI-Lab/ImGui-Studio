# Generated control helpers

[Español](EMITTED_CONTROLS.md)

These headers contain emitted implementations and helpers that translate nodes to target types. Compiling ImGui Studio validates the C++ container. A changed emitted block also needs compilation or interpretation in its target language.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [studio_codegen_titlebar.hpp](../../include/imguistudio/export/detail/studio_codegen_titlebar.hpp) | Contains emitted title-bar helpers for native targets. | Preserve placement, style stack balance, and per-frame button pulses. |
| [studio_managed_titlebar.hpp](../../include/imguistudio/export/detail/studio_managed_titlebar.hpp) | Converts the tree to elements such as `Button` and `WindowTitle`. | Centralizes visible-child selection and title data so bindings receive consistent input. |
| [studio_titlebar_csharp.hpp](../../include/imguistudio/export/detail/studio_titlebar_csharp.hpp) | Implements emitted window controls with ImGui.NET. | Embedded fields and dedicated buttons have different visibility when collapsed. |
| [studio_titlebar_java.hpp](../../include/imguistudio/export/detail/studio_titlebar_java.hpp) | Implements the emitted header with imgui-java. | Check clicks, labels, and IDs within the window's scope. |
| [studio_titlebar_python.hpp](../../include/imguistudio/export/detail/studio_titlebar_python.hpp) | Emits icon and header helpers for imgui-bundle. | Preserve per-frame return values and binding call order. |
| [studio_titlebar_rust.hpp](../../include/imguistudio/export/detail/studio_titlebar_rust.hpp) | Emits `TitleButton`, `TitleAppearance`, `WindowMode`, and `TitleContext`. | Separates configuration, state, and frame context. Compile the host when changing borrows or callbacks. |
| [studio_codegen_image.hpp](../../include/imguistudio/export/detail/studio_codegen_image.hpp) | Describes assets and emits C/C++ drawing adapters. | Preserve resource independence from the backend as well as size and click state. |
| [studio_managed_images.hpp](../../include/imguistudio/export/detail/studio_managed_images.hpp) | Contains emitted loaders and drawing for Python and C#. | The backend supplies live textures for drawing and releases them during shutdown. |
| [studio_images_java_rust.hpp](../../include/imguistudio/export/detail/studio_images_java_rust.hpp) | Composes loading, texture, and drawing helpers for Java and Rust. | Java uses classpath resources and Rust includes assets at build time. Check both packaging paths. |
| [studio_codegen_animation.hpp](../../include/imguistudio/export/detail/studio_codegen_animation.hpp) | Emits C/C++ bindings and native animation helpers. | Preserve cycle calculation and style restoration after drawing the children. |
| [studio_managed_animation.hpp](../../include/imguistudio/export/detail/studio_managed_animation.hpp) | Collects scaled children and emits animation logic for Python and C#. | Preserving units and dimensions keeps layout and animation phase consistent. |
| [studio_animation_java.hpp](../../include/imguistudio/export/detail/studio_animation_java.hpp) | Emits imgui-java animation helpers. | The emitted layer must balance style changes when leaving the container. |
| [studio_animation_rust.hpp](../../include/imguistudio/export/detail/studio_animation_rust.hpp) | Emits `AnimationStyle` and `Animation` for applying effects. | Check style scope and measurement used with auto-height children. |
| [studio_codegen_chart2d.hpp](../../include/imguistudio/export/detail/studio_codegen_chart2d.hpp) | Emits bindings and shared functions plus C/C++ 2D modes. | C and C++ use different binding signatures. Test both and the empty-data modes. |
| [studio_codegen_chart3d.hpp](../../include/imguistudio/export/detail/studio_codegen_chart3d.hpp) | Emits projection and 3D drawing for C/C++. | Preserve initial orientation, orbit controls, and draw order. |
| [studio_chart2d_csharp.hpp](../../include/imguistudio/export/detail/studio_chart2d_csharp.hpp) | Emits sampling and 2D modes for ImGui.NET. | Check series size and draw coordinates in the binding. |
| [studio_chart2d_java.hpp](../../include/imguistudio/export/detail/studio_chart2d_java.hpp) | Emits sampling and 2D modes for imgui-java. | Check array bounds and draw-list overloads. |
| [studio_chart2d_python.hpp](../../include/imguistudio/export/detail/studio_chart2d_python.hpp) | Emits Python sampling and 2D helpers, including the pie path. | Check automatic scale and imgui-bundle argument shapes. |
| [studio_chart2d_rust.hpp](../../include/imguistudio/export/detail/studio_chart2d_rust.hpp) | Emits buffer-backed `chart_samples` and the `Chart2d` renderer. | The buffer size bounds sampling. Keep slices valid for short and empty series. |
| [studio_chart3d_csharp.hpp](../../include/imguistudio/export/detail/studio_chart3d_csharp.hpp) | Emits state, projection, and 3D rendering for C#. | Check that input changes only the generated view's state. |
| [studio_chart3d_java.hpp](../../include/imguistudio/export/detail/studio_chart3d_java.hpp) | Emits projection, faces, and 3D rendering through imgui-java. | Verify color, face order, and emitted orbit state. |
| [studio_chart3d_python.hpp](../../include/imguistudio/export/detail/studio_chart3d_python.hpp) | Emits Python 3D math and drawing functions. | Check projection and invalid data in the target as well as syntax. |
| [studio_chart3d_rust.hpp](../../include/imguistudio/export/detail/studio_chart3d_rust.hpp) | Emits `Chart3dState` and point, face, and view types. | The series length is part of the type. Keep emitted state and the helper consistent. |

## Verification

For a new property, check the inspector and compile the target that consumes the helper. Repeat [Title bars](../TITLE_BAR.en.md), [Status and progress](../../examples/estado-y-progreso.json), or [Images and input](../../examples/vulkan-imagenes.json) as applicable. Include the emitted file and node range when reporting a compiler failure.

