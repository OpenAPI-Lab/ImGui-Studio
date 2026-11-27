# Capabilities and validation

[Español](CAPABILITIES.md)

| Area | Available | Verification and scope |
| --- | --- | --- |
| Desktop editor | Dear ImGui, GLFW, and OpenGL. | Built and started on Windows with GCC and MSVC. Native Wayland and XWayland verified on Ubuntu 24.04 under WSL. macOS runtime pending. |
| Project | Start screen, recent files, saving, history, undo/redo, and recovery. | Tests cover persistence, edits, conflicts, and sessions. Save to a different file when a conflict is detected. |
| Design | Free canvas, hierarchy, inspector, configurable title bar, preview, images, and charts. | Coverage varies by control and property. The editing reference lists supported types. |
| Library and extensions | Custom widgets, import/export, and curated imgui-knobs catalog. | Knobs have preview and C++ output. Other exporters emit a placeholder for that type. |
| C++ OpenGL | Sources and a GLFW CMake project. | Desktop generation and runtime tests. |
| C++ Vulkan | GLFW and Vulkan CMake project. | Presentation, resize, input, and images on Windows and Ubuntu under WSL. |
| C11, Python, C#, Java, and Rust | Desktop projects using their respective toolchains. | Runtime results exist on Windows and WSL. Each project report identifies partial and unsupported output. |
| Android C++ | Gradle/CMake pack with NativeActivity and OpenGL ES 3. | APK for arm64 and x86-64. Startup, touch, textures, rotation, and resume on an Android 15 x86-64 emulator. Physical ARM64 and on-screen keyboard pending. |
| macOS/iOS C++ | Optional Metal templates with AppKit and UIKit. | Generation verified on Windows and WSL. Apple SDK builds and runtime pending. |
| Languages | English, Spanish, French, Korean, Japanese, and Simplified Chinese. | External packs are supported. See [remaining strings and glyph constraints](LOCALIZATION.en.md). |
| Assistant | Adapters for Ollama, LM Studio, OpenAI, Anthropic, and Gemini. | Requests verified with a simulated server. Mac Keychain and live providers still require validation. |
| Packages | Windows portable ZIP and EXE, Ubuntu 24.04 and Debian 13 DEBs, and a macOS DMG workflow. | Native Windows ZIP, isolated Wine installer lifecycle, Ubuntu WSL DEB lifecycle, and Debian 13 APT installation and virtual X11 launch. Native UAC, signing, and DMG validation pending. |
