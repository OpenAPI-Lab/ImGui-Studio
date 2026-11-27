# Install ImGui Studio

[Español](INSTALLATION.md)

## Choose a download

Choose your operating system's installer from the release's **Assets** section. Use the matching `.sha256` file to verify the binary. On Linux, choose the DEB for your distribution. [GitHub Releases](https://github.com/OpenAPI-Lab/ImGui-Studio/releases)

| System | Version 1.0.0 file |
| --- | --- |
| Windows x64 | `ImGuiStudio-1.0.0-Windows-AMD64.exe` |
| Ubuntu 24.04 x64 | `imgui-studio_1.0.0_ubuntu24.04_amd64.deb` |
| Debian 13 x64 | `imgui-studio_1.0.0_debian13_amd64.deb` |

The Windows installer is unsigned. Building and verifying the macOS editor DMG is still pending. To build from source, follow the [build guide](BUILD_AND_TEST.en.md).

## Windows

Open the EXE and follow the setup wizard. Launch **ImGui Studio** from the Start menu. To uninstall it, use **Installed apps** in Windows Settings or the Start menu's uninstall shortcut.

In PowerShell, compute the checksum and compare it with the first value in the `.sha256` file:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath '.\ImGuiStudio-1.0.0-Windows-AMD64.exe'
```

## Ubuntu and Debian

Open a terminal in your downloads directory. Run the pair of commands for your distribution to verify and install the package:

```sh
# Ubuntu 24.04
sha256sum -c imgui-studio_1.0.0_ubuntu24.04_amd64.deb.sha256 &&
  sudo apt install ./imgui-studio_1.0.0_ubuntu24.04_amd64.deb

# Debian 13
sha256sum -c imgui-studio_1.0.0_debian13_amd64.deb.sha256 &&
  sudo apt install ./imgui-studio_1.0.0_debian13_amd64.deb
```

APT resolves the package's declared libraries. Open **ImGui Studio** from the applications menu or run `imgui_studio` from a graphical session. The package supports Wayland and X11. On WSL, use WSLg. To remove it, run `sudo apt remove imgui-studio`.

## Open your first project

On the start screen, choose **New project**, **Open project**, or a recent project. The recovery prompt appears when a recoverable copy from an earlier session exists. Use the [first-project guide](WORKFLOW.en.md) to edit, preview, and export a design.

The Android, macOS, and iOS C++ export packs are separate downloads. They generate projects for those platforms from the desktop editor. [Export guide](EXPORTING.en.md#optional-destinations)
