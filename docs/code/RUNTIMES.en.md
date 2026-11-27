# Export hosts and templates

[Español](RUNTIMES.md)

Generated hosts connect the interface to a window, graphics backend, and platform input. This code runs in the user's application.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [studio_codegen_vulkan.hpp](../../include/imguistudio/export/detail/studio_codegen_vulkan.hpp) | `vulkanMain` emits the entry point with its state type and image path when needed. | Keeps the host separate from the UI so export can choose the renderer. |
| [studio_vulkan.hpp](../../templates/studio_vulkan.hpp) | Owns GLFW, the Vulkan device, swapchain, and presentation loop. | Resize can require rebuilding presentation resources. Verify their lifetimes extend through use. |
| [studio_vulkan_images.hpp](../../templates/studio_vulkan_images.hpp) | Uploads textures and manages the views and descriptors used by ImGui. | A texture depends on the device and backend synchronization. Free it before backend teardown. |
| [main.cpp](../../templates/android/main.cpp) | Hosts NativeActivity, EGL, input, and OpenGL ES rendering. | Android surfaces change during the lifecycle. Coordinate creation and release with ImGui resources. |
| [AndroidManifest.xml](../../templates/android/AndroidManifest.xml) | Declares NativeActivity and Android application configuration. | The library and activity names must match the built code. |
| [build.gradle](../../templates/android/build.gradle) | Sets the plugin, SDK levels, NDK, architectures, and CMake build. | Coordinates Gradle and native compilation. Update the guide when requirements change. |
| [settings.gradle](../../templates/android/settings.gradle) | Configures repositories and Gradle project name. | Lets the export configure independently of the editor. |
| [CMakeLists.txt](../../templates/android/CMakeLists.txt) | Configures Dear ImGui, the Android backend, and the native library. | Images can change dependency requirements. Keep pinned versions and hashes consistent. |
| [template.version](../../templates/android/template.version) | Identifies the Android pack format. | The exporter validates it before using pack files. |
| [CMakeLists.txt](../../templates/apple/CMakeLists.txt) | Selects SDK, frameworks, and macOS or iOS bundle properties. | Shared code requires the target's environment and frameworks. |
| [main.mm](../../templates/apple/main.mm) | Hosts the UI using AppKit or UIKit and the Metal frame loop. | Keeps platform input and rendering outside the design code. |
| [studio_metal_images.hpp](../../templates/apple/studio_metal_images.hpp) | Loads and holds Metal textures for generated assets. | Texture lifetime must cover all draw commands that use it. |
| [Info.plist](../../templates/apple/macos/Info.plist) | Describes the macOS bundle. | Identifier and executable must match generated configuration. |
| [Info.plist](../../templates/apple/ios/Info.plist) | Describes the iOS bundle. | Preserve startup and configuration fields for the UIKit target. |
| [template.version](../../templates/apple/macos/template.version) | Identifies the macOS pack format. | Lets the exporter verify the pack target before copying it. |
| [template.version](../../templates/apple/ios/template.version) | Identifies the iOS pack format. | Prevents desktop metadata from being selected for a mobile project. |
| [title_bar_help.hpp](../../examples/title-bar-action/title_bar_help.hpp) | Connects the title-button pulse to a host-owned Help window. | The pulse lasts one frame. Keep the window's boolean across render calls. |

## Verification

Run `GeneratedVulkanRuntime` and `NativeVulkanProjectBuildTest` for Vulkan. `AndroidCppExportGenerationTest` and `AppleCppExportGenerationTest` cover generation. Repeat startup, input, and lifecycle checks with the applicable SDK and device or simulator when changing a host. [Vulkan](../VULKAN.en.md) · [Android](../ANDROID.en.md) · [Apple](../APPLE_EXPORTS.en.md)

