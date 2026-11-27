# Export and integrate a UI

[Español](EXPORTING.md)


In **Generated code**, choose a language and a new directory. The editor emits design code, a sample host, and `codegen_report.txt`. Report line ranges map nodes to code. Review diagnostics before integrating a control. The [capability matrix](CAPABILITIES.en.md) distinguishes generation from verified execution.

## C++ in an existing application

Exports include `ui.hpp`, a state type derived from the project name, `ApplyStudioStyle()`, and `RenderStudioUI(state)`. The host creates the Dear ImGui context and backends. For a project named `Monitor`, the integration pattern is:

```cpp
#include "ui.hpp"

struct MonitorView {
    MonitorState state{};

    void initializeStyle() { ApplyStudioStyle(); }
    void drawFrame() { RenderStudioUI(state); }
};
```

Call `initializeStyle()` after creating the context. Call `drawFrame()`
between `ImGui::NewFrame()` and `ImGui::Render()`. Generated `main.cpp`
contains the complete order. Keep resources and helpers together. If you
replace the backend, adapt texture loading and release to its lifecycle.
Connect actions by reading the generated state after drawing.
The [Help button example](../examples/title-bar-action/README.en.md) uses
this pattern to add an action to the exported C++ host.

Desktop C++ offers GLFW/OpenGL and Vulkan on Windows and Linux. Vulkan requires headers, a loader, and a driver that supports presentation. Set `Vulkan_ROOT` for a custom SDK location.
The [Vulkan guide](VULKAN.en.md) covers images, resizing, and input checks.

## Other desktop destinations

| Language | Entry point and toolchain |
| --- | --- |
| C11 | CMake, cimgui, and the `studio_export_c` executable. |
| Python | `main.py` and `requirements.txt`. Use CPython and the emitted imgui-bundle version. |
| C# | `StudioExport.csproj`, the .NET 8 SDK, and ImGui.NET. |
| Java | `StudioUI.java` and the generated Java project with its declared dependencies. |
| Rust | `Cargo.toml`, Cargo, and imgui-rs. Native bridges require a C/C++ toolchain when the export includes them. |

## Optional destinations

The `studio_android_pack`, `studio_macos_pack`, and `studio_ios_pack` targets build separate ZIPs. Extract the desired pack and point the exporter at the directory containing `template.version`. Build the generated project with the platform toolchain.

- [Android](ANDROID.en.md) uses C++, Gradle, the NDK, and OpenGL ES 3.
- [macOS](../templates/apple/macos/README.md) uses C++, AppKit, Metal, and CMake/Xcode.
- [iOS](../templates/apple/ios/README.md) uses C++, UIKit, Metal, and CMake/Xcode.

The [macOS and iOS guide](APPLE_EXPORTS.en.md) covers Xcode, the simulator,
and checks for both generated projects.

The pack README files include Spanish and English instructions.
