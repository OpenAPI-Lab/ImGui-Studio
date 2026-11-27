# GitHub checks

[Español](CI.md)

[Repository checks](../.github/workflows/checks.yml) runs on pull requests and pushes to `main` when GitHub Actions is enabled for the repository. It uses Ubuntu 24.04 with Ninja and Windows with Visual Studio 2022.

| Check | What it verifies |
| --- | --- |
| Documentation | Internal link targets and headings. It checks that every file in `src/` and `include/` appears in both source maps. |
| Core | Headless compilation, persistence, tree editing, and session recovery. |
| Export | Generation of C++, Android, and Apple projects from test designs. |

The workflow uses read-only permissions. Generated projects have their own runtime checks. For UI, packaging, or rendering changes, attach relevant evidence using the [validation guide](VALIDATION.en.md).

## On-demand desktop build

In **Actions → Desktop build → Run workflow**, choose one platform to check the GUI editor. [The manual workflow](../.github/workflows/desktop.yml) uses Release and each platform's graphics dependencies.

| Platform | Configured check |
| --- | --- |
| Ubuntu | Build and startup under Xvfb with X11 and software OpenGL. |
| Windows | MSVC build and check for `imgui_studio.exe`. |
| macOS | Build and check the executable and `Info.plist` inside the `.app`. |

To check Wayland, start the editor in a Wayland session as described in [Reproducible validation](VALIDATION.en.md#graphics-changes). macOS window startup still needs verification in a graphical session.

## macOS disk images

Once the workflow is on the default branch, run **Actions → macOS packages → Run workflow**. Download `ImGuiStudio-macOS-arm64` and `ImGuiStudio-macOS-x86_64` from the completed run. Each artifact holds a DMG for macOS 15 or later and its SHA-256 checksum. The workflow mounts the image and checks the editor architecture, libraries, license, and documentation. Developer ID signing, notarization, and graphical startup are verified separately. To publish, attach each DMG and its checksum to the matching GitHub Release. [Packaging workflow](../.github/workflows/macos-package.yml)

## Repeat the documentation check

From the root, run `python tools/check_docs.py` or `python3 tools/check_docs.py`. The script identifies invalid relative link paths and headings. It checks that each Markdown document under the root, `docs/`, and `examples/` has an alternate-language file and a language switch. It also checks links in pack and dependency README files and GitHub templates. When adding or moving first-party code, update both source maps and the [code reference](code/README.en.md) chapters.

See the [build and test steps](VALIDATION.en.md#model-and-exporter-changes) to reproduce the core check on your machine.

Verify the hosted configuration in the **Actions** tab once the workflow runs on GitHub. Local results cover the tools and environment recorded with that run.

The manual [Apple export build workflow](../.github/workflows/apple-exports.yml)
checks [generated macOS and iOS apps](APPLE_EXPORTS.en.md). It builds the
projects with Xcode and checks their bundles.
