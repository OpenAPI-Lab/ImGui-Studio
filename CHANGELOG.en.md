# Changelog

[Español](CHANGELOG.md)

## Unreleased

- **Open project**, the first save, and **Save project as…** let you browse folders and select a file. Windows uses the system picker when available.
- **Studio appearance** offers six palettes and adjustable editor panel rounding. Studio remembers these settings across sessions.

## 1.0.0 — 2026-11-24

- Native visual editor with a start screen, recent projects, and recovery of unsaved changes.
- Free positioning on the canvas, an editable hierarchy, an inspector, configurable title bars, and preview.
- Built-in widgets, a blank canvas for designing custom widgets, and an extension catalog. The first integrated extension is imgui-knobs.
- English, Spanish, French, Korean, Japanese, and Simplified Chinese interface catalogs, with support for community language packs.
- Desktop export to C++, C, Python, C#, Java, and Rust. Coverage varies by widget, language, and platform.
- C++ Vulkan output for Windows and Linux alongside OpenGL output.
- Optional C++ packs for Android with OpenGL ES and for macOS and iOS with Metal.

### Verification and limitations

- The editor and portable ZIP were tested on Windows. The EXE installation lifecycle was verified in an isolated Wine environment. Authenticode signing and the native Windows elevation prompt remain pending.
- The Ubuntu 24.04 x64 DEB was installed, launched, and removed under WSL with both direct Wayland and XWayland. The Debian 13 x64 DEB was installed and removed with APT and started under virtual X11 in a Debian 13 container.
- The sample APK was built for arm64 and x86-64. Startup, images, touch, rotation, and resume were tested on an x86-64 Android 15 emulator. A physical-device test and on-screen keyboard checks remain pending.
- macOS and iOS project generation was verified on Windows and WSL. Xcode builds, runtime checks, and desktop DMG verification are pending.

See [installers and SHA-256 checksums](docs/INSTALLATION.en.md).
