# Technical reference

[Español](REFERENCE.md)

The desktop editor uses C++17, Dear ImGui, GLFW, and OpenGL. The [source map](SOURCE_MAP.en.md) identifies the files responsible for the model, serialization, preview, editor gestures, exporters, processes, and packaging. See the [changelog](../CHANGELOG.en.md) for platform verification.

## Projects and recovery

On the start screen, create a project, enter a path to open an existing JSON file, or choose a recent project. Windows also offers the system file picker. Use **Ctrl+O** to open, **Ctrl+S** to save, and **Ctrl+Shift+S** to save as. New, Open, and Close prompt when the document has unsaved changes. Confirm replacement when **Save As** points at an existing file.

Saving writes an exclusive temporary file in the same directory, flushes it, and replaces the target. Project files have a 32 MiB limit, up to 100,000 nodes, and a maximum widget depth of 128. Imported data retains unknown fields and widget types. JSON round trips preserve values and structure while allowing formatting and key order to change.

An accompanying `<project>.imguistudio/history.json` stores up to 35 snapshots. Manual snapshots are written when taken. Automatic snapshots wait for 15 seconds of inactivity. The editor keeps recovery checkpoints separately, with up to 40 states per undo or redo stack and a two-second interval between checkpoint attempts. Edits made after the last checkpoint may be lost if the process stops abruptly.

When a checkpoint from an interrupted session exists, the recovery prompt offers to reopen it as an untitled project. **Save As** chooses where to write it. **Later** keeps the checkpoint. Confirming **Discard** removes that checkpoint. Saving or discarding a session also clears its temporary recovery copy. System locks distinguish active editors from interrupted sessions. An external change to the original project triggers a **Save As** prompt. For metadata limits, revision checks, and history conflicts, see [Project format and recovery](PROJECT_FORMAT.en.md).

## Editor and canvas

Design mode uses the Dear ImGui controls that preview renders. Selecting a
control in design mode leaves its design value in the document. Tabs, trees,
and collapsible sections can still be navigated to reach their children.
The editor uses visible geometry and clipping when choosing nested widgets.

- Drag a flow-layout widget to reorder it or move it into a container. A
  container's center targets its children. Its edges target before or after
  the container. The hierarchy presents the same tree in vertical order.
- **Alt+drag** switches a widget to free positioning. A widget with
  `useCursorPos` already enabled can be moved directly. Its coordinates
  are relative to the containing window or child. Grid snapping uses steps
  of eight when it is enabled.
- **Ctrl+click** toggles a widget in the selection. Single-node commands and
  inspector properties use the most recently added widget.
- Drag a selection from a common container to move it as a block. A completed
  drag creates one undo entry. **Esc** cancels a drag and preserves the document.
- Pan with the middle mouse button and zoom between 50% and 150% with
  **Ctrl+wheel**. Press **Home** over the canvas to reset the viewport.
  Preview opens at 100% and the editor restores the design viewport on return.
- Separators, spacers, and fixed spacing can be placed freely. Drag a selected
  control's handle or use the inspector. Width or height adjustments depend on
  the control. Checkbox, radio, and slider height follows the font and theme.
- Title bar controls, layouts, style overrides, and images can be saved with
  a project. Export coverage varies by property and widget. See the
  [capability matrix](CAPABILITIES.en.md) and the generated report.

### Widget properties and runtime behavior

The preview stores interactive values separately from design properties. **Reset
preview** restores initial values, tab selection, collapsed sections, and window
geometry. Radios with the same `varName` share a selection. Numeric controls
fall back to a safe preview format when a saved format is incompatible, and the
original format stays in the document until you edit it.

| Widget or group | Design, preview, and export behavior |
| --- | --- |
| Buttons, labels, separators, and selection | Includes regular, small, arrow, and color buttons, selectable items, checkboxes, radios, and formatted or wrapped text. Application actions are connected in the host. |
| Text and numeric inputs | Edit initial values, bounds, steps, speeds, formats, and vector components. Native text uses the Dear ImGui `std::string` adapter. Generated bindings can impose their own capacities. |
| Child panels, groups, trees, tabs, tables, and grids | Edit layout, item spacing, columns, and initial selection. Tables and grids accept 1–511 columns in the native preview. A larger value produces a diagnostic and stays in the document. |
| Window styles | Set inherited or per-window color, opacity, borders, rounding, and padding. The inspector offers ImGui, Bionic, macOS, Minimal, and Cyber profiles. The lower resize-handle indicator can be hidden while the corner remains usable. |
| Title bars and menus | Set visibility, alignment, icon, bar dimensions, and minimize, maximize, and close controls. Place supported child controls or a menu in the title bar. Drag the title edges to set its width. Dedicated title buttons expose a per-frame activation in all six desktop exporters. Read the report for property and binding limits. |
| Image and image button | Import local PNG or JPEG files up to 2 MiB and 4096 × 4096 pixels. Assets are embedded in the project. C++, C11, and Python embed the bytes in code. C# and Rust include them during the build, and Java uses classpath resources. The generated host loads and frees textures. An image button reports activation for the current frame. |
| Animation | Set duration, curve, loop, and initial playback. Preview supports fade, bounce, horizontal slide, scale, and theme hue rotation. The six desktop exporters retain the effects and playback state. |
| Loader, Component, and Repeater | Loader shows or hides its children. Component groups a named template. Repeater uses one editable template with configurable instance count and spacing. Generated instances have distinct IDs and share values from the template. |
| List, grid, and traversal views | Edit the item model, initial selection, border, and dimensions. Grid also exposes column count and spacing. Preview input changes temporary selection, and exported apps keep their own selection at runtime. |
| Flow layout | Wraps visible children to the next row based on available width. Configure spacing and the border. All six desktop exporters implement row wrapping. |
| 2D charts | Line, area, bar, doughnut, scatter, and smooth curve. Edit the title, series, color, labels, and automatic or manual bounds. In cartesian previews, hover to inspect a value. All six desktop exports expose the data series as runtime state. |
| 3D charts | Bars, scatter, and surface. Edit color, title, elevation, and azimuth. **Shift+drag** rotates the chart in design mode, and **Esc** cancels the gesture. Preview rotation is temporary. The six desktop exports expose both the series and orientation. |
| Toggle, spinner, and circular progress | Configure the initial state, revolutions per second, or progress fraction and center text. Zero speed stops the spinner. A missing progress label shows the percentage. C++ shares the drawing code with preview. Other desktop exporters report the unsupported controls and emit placeholders. |

For charts, **Edit values** accepts comma- or whitespace-separated numbers
with a decimal point. **Apply values** replaces the series in one undo step.
The [title-bar example](../examples/barra-de-titulo.json), [free-placement
example](../examples/barra-y-lienzo-libre.json), and [status
widgets](../examples/estado-y-progreso.json) illustrate the editor controls.

### Interface language and fonts

Choose **Language** on the start screen or under **View**. The setting takes
effect immediately and is written to `language.json`. To add a language, put
a UTF-8 file in `locales/` under the editor's state directory, for example
`locales/en-gb.json`, then reopen the editor. Use lowercase letters, digits,
and hyphens in the file name. Missing or invalid translations fall back to
English. Valid keys are listed in [studio_locale_strings.inc](../include/imguistudio/localization/studio_locale_strings.inc).

```json
{
  "name": "English (UK)",
  "strings": {
    "WelcomeNew": "Create a project",
    "Recents": "Recently opened projects"
  }
}
```

The state directory is `%LOCALAPPDATA%/ImGuiStudio` on Windows,
`$XDG_STATE_HOME/ImGuiStudio` or `~/.local/state/ImGuiStudio` on Linux, and
`~/Library/Application Support/ImGuiStudio` on macOS.
`IMGUI_STUDIO_STATE_DIR` overrides it. The editor uses installed fonts and
can also read `fonts/latin.ttf`, `fonts/korean.ttf`, `fonts/japanese.ttf`,
and `fonts/chinese.ttf` from the state directory. See the [localization
coverage and glyph tests](LOCALIZATION.en.md).

## Run / Build

Build creates a fresh attempt directory from the live design and invokes the
selected target's toolchain. Run uses the result of a successful build for the
current design and profile. A change to the design or target requires a new
build. The smoke action draws three hidden frames where supported.

Windows launches through CreateProcessW and uses a Job Object for cancellation.
POSIX uses an exec call and a process group. Arguments are passed individually.
Output combines stdout and stderr, and the Build pane keeps up to 1 MiB of
recent output. Attempt directories stay on disk for inspection.

The diagnostics panel parses common compiler formats such as
`file:line:column` and `file(line,column)`, as well as CMake messages.
The source viewer shows the generated snapshot taken when the build started.
When a line falls inside a generator's node range, the panel can locate the
widget while the document still matches that build. Dependency and startup
errors can have a file and line without a corresponding widget.

### Build profiles and process boundaries

The editor can keep up to 32 profiles. Profile changes stay in memory until
**Save profiles**. If **Reload profiles** fails, the editor keeps the current
profiles in memory and blocks writes until a valid file is loaded. Updating a
profile's settings or workspace invalidates the build result. Renaming a
profile by itself preserves it. After restarting, build again with the
restored profile.

`build_profiles.json` uses schema version 1 and stores `schemaVersion`,
`activeProfile`, and `profiles`. Each profile contains `id`, `name`,
`workspace`, and `settings`. The file limit is 1 MiB, maximum depth is
64, and each known text field is limited to 8,192 bytes. Saving compares
the file read by this instance against the current file under an OS lock.
If another instance changed it, reload before saving.

**Include profile in project** writes the versioned profile under
`buildConfig.nativeCpp` as an undoable design edit. The JSON contains
the selected profile's paths after you save. **Import project profile**
copies it to a new local profile for review. Build uses recognized fields
from the selected profile. Unknown fields are retained during round trips.

Child stdin is closed. The editor reads up to 64 KiB of combined stdout and
stderr per frame and retains 1 MiB of recent output. **Stop** and editor
shutdown cancel the Windows Job Object or POSIX process group. A POSIX child
that creates a separate group or session can outlive cancellation.
See [Run / Build](RUN_BUILD.en.md) for the controls and source files.

### Diagnostic parsing and source ranges

The parser handles GCC and Clang colon-separated locations, MSVC and Roslyn
parenthesized locations, and CMake messages with continuation lines. It reads
incremental output, including CRLF and a final line without a newline, and
strips terminal styling from parsed messages. The pane accepts up to 1,000
entries and input lines up to 16 KiB. When the pane is full, newer errors can
displace warnings or notes. Unrecognized and localized messages may appear only
in the log.

Generated node ranges are available for `ui.hpp`, `studio_ui.h`,
`StudioUI.cs`, `main.py`, `StudioUI.java`, and `ui.rs`. The editor uses
the narrowest matching node range in the attempt snapshot. Symlink aliases
can prevent a path match. **View exported code** uses the snapshot rather
than opening arbitrary paths found in tool output. Runtime output stays in
the log. [Parser tests](../tests/verify_build_diagnostics.cpp) ·
[Build integration tests](../tests/verify_build.cpp)

## Catalog and assistant

**Widgets → My Library** keeps built-in widgets and user-defined widgets
accessible from the palette. Create a blank draft, start from a built-in
control, or copy the selected widget. A blank draft can save multiple
controls as a group. Each inserted copy has its own IDs and state. Editing
the library definition affects future insertions. Import or export a widget
JSON to share its tree and embedded images. See [Widget library and catalog](WIDGET_LIBRARY.en.md).

The catalog downloads the pinned imgui-knobs header, implementation, and MIT
license, then checks their bytes against the reviewed sources. Preview uses
the adapter that was compiled into the editor. Removing the extension hides
its palette entry. Saved nodes retain their properties.

The assistant offers Ollama, LM Studio, OpenAI, Anthropic, and Gemini.
Sending a request includes the prompt, recent conversation context, and a
summary of the current design. A proposed edit appears for review. Applying
it adds an undoable edit if the document still matches the proposal's base.

### Provider settings and credentials

Open **View → AI Assistant**, choose a provider and model, and set the base URL
if required. Saving settings stores provider names, URLs, and model names
separately from the project. Use **Temporary key** for the current provider
and URL. For the official endpoints, the editor accepts `OPENAI_API_KEY`,
`ANTHROPIC_API_KEY`, and `GEMINI_API_KEY`. Changing provider or URL clears a
key entered into the panel. Project files, exported code, and provider settings
exclude credentials.

The system vault uses WinCred, Linux Secret Service, or macOS Keychain.
On Linux, CMake detects libsecret through pkg-config. Runtime storage needs
a D-Bus session with an available Secret Service. Use
`STUDIO_ASSISTANT_SECRET_SERVICE=OFF` to build without libsecret. Keychain
runtime validation is pending. `STUDIO_ASSISTANT_NETWORK=OFF` disables
assistant connectivity at build time. See [Assistant and providers](ASSISTANT.en.md).

## Desktop exporters

Choose a language and an empty destination path in **Generated code**. The
exporters generate a sample host and a `codegen_report.txt` with per-node
diagnostics and source ranges. Review the report for fallbacks and unsupported
properties. Exporting leaves the in-memory project and its saved file as they
were. See [Export and integrate a UI](EXPORTING.en.md) for integration code.

| Target | Contract and checks |
| --- | --- |
| C++ | The CMake project includes `ui.hpp` with state, `ApplyStudioStyle()`, and `RenderStudioUI(state)`, plus the host. Keep one generated interface per translation unit. The export checks compile both header and render modes, check linkage across translation units, and run frames with Dear ImGui. |
| C++ with Vulkan | Select Vulkan in the export dialog or Build profile for Windows or Linux. OpenGL is the default. A Vulkan driver, loader, and headers are required. Use `Vulkan_ROOT` for a custom SDK. Enable `STUDIO_TEST_VULKAN_EXPORT` for texture, input, and presentation checks. |
| C11 | `studio_ui.h` and `main.c` compile as C11. `backend.cpp` and cimgui require a C++ compiler. Build with CMake and run `studio_export_c`. The generated input buffers have fixed sizes. Enable `STUDIO_TEST_C_EXPORT` to exercise the emitted projects. |
| Python | Run `main.py` with CPython 3.10+ and the imgui-bundle version in `requirements.txt`. Build checks generated syntax and the binding. The editor starts the selected interpreter and uses packages from that interpreter or its environment. The Linux wheel tested under WSL used X11, which requires XWayland in a Wayland session. Configure `STUDIO_PYTHON_TEST_EXECUTABLE` to enable its Build / Run checks. |
| C# | Uses the .NET 8 SDK, ImGui.NET, and Silk.NET. Use `dotnet run -- --headless-test` for three in-memory frames or `dotnet run -- --smoke-test` for its hidden OpenGL host. Keep one `UIState` instance per interface. Configure `STUDIO_DOTNET_TEST_EXECUTABLE` and optionally `STUDIO_TEST_CSHARP_WINDOW`. |
| Java | Targets Java 17 and uses `StudioUI.java`, `Main.java`, imgui-java, and LWJGL. The POM supports Maven. Run / Build uses local JARs and native libraries with the selected JDK. Configure `STUDIO_JAVA_TEST_JDK` and `STUDIO_JAVA_TEST_BINDING_DIR` to run the exporter tests. |
| Rust | Cargo builds `ui.rs` and the GLFW/OpenGL host. Use `cargo run -- --smoke-test` or `cargo build --no-default-features` for library integration. Native bridges require a compatible C++ toolchain. Configure `STUDIO_RUST_TEST_CARGO` for generated-project tests. |

## Desktop packages

Configure a separate Release build with packaging enabled:

```sh
cmake -S . -B build/package -DSTUDIO_PACKAGING=ON -DBUILD_CORE_TEST=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/package --config Release --target imgui_studio
cpack --config build/package/CPackConfig.cmake -C Release
```

CPack writes packages and their SHA-256 files to `build/package/packages`.
The packaging build uses Python 3 for icon generation.

| Platform | Generator and dependencies |
| --- | --- |
| Windows | The default ZIP contains the executable and runtime DLLs. Runtime dependency staging needs CMake 3.21+. For the EXE, add `-G NSIS` to the CPack command and put `makensis` on the PATH. |
| Ubuntu and Debian | Use `-G DEB`, `dpkg-dev`, and `file` so CPack can calculate dependencies. The package also declares dynamically loaded GLFW libraries for each enabled backend. Build on the intended distribution to use its dependency names and runtime versions. Set `STUDIO_PACKAGE_MAINTAINER` when packaging for distribution. |
| macOS | Use `-G DragNDrop` on a Mac. The configured workflow builds and checks disk images for Intel and Apple Silicon on macOS 15 or later. The workflow and DMGs await hosted validation. |

With packaging enabled, select the host's package test:

```sh
ctest --test-dir build/package -C Release -R 'WindowsZipPackageTest|LinuxDebPackageTest' --output-on-failure
```

To check a prebuilt ZIP or DEB, run `python packaging/verify_package.py
--format ZIP --artifact <file>` or use `--format DEB` and pass the
`--dpkg`, `--dpkg-deb`, and `--fakeroot` paths. Keep the `.sha256` file
next to the package. Add `--require-libsecret` to verify that dependency in a
build with Linux credential storage. The script compares licenses, guides,
and examples, then starts the packaged editor. The DEB test uses an isolated
dpkg root to exercise installation and removal.

For the EXE, `packaging/verify_windows_installer.ps1 -Installer <file>
-PreflightOnly` verifies the artifact and checks for conflicts. The Windows
ZIP and the Wine installer lifecycle, the Ubuntu WSL DEB lifecycle, and the
Debian APT install and removal were verified. See [Install ImGui Studio](INSTALLATION.en.md)
for the download names and [GitHub checks](CI.en.md#macos-disk-images) for the DMG workflow.

## Additional contracts

- [Project format and recovery](PROJECT_FORMAT.en.md)
- [Exported code and the renderer lifecycle](EXPORTING.en.md)
- [Compilation, tests, and distribution](BUILD_AND_TEST.en.md)
