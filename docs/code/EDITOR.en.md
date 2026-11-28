# Startup, editor UI, and canvas

[Español](EDITOR.md)

The host owns GLFW and the ImGui context. `StudioApp` coordinates the document and uses renderer observations for geometry. Canvas interactions produce edit proposals that are validated before entering undo history.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [studio_editor_appearance.hpp](../../src/editor/studio_editor_appearance.hpp) | Defines palettes, applies editor rounding, and reads and writes appearance preferences. | The editor stores these preferences in `appearance.json`; project render styles remain independent. |
| [studio_project_file_picker.hpp](../../src/editor/studio_project_file_picker.hpp) | Lists folders and JSON files. For Save, it builds a destination from the folder and filename. | Both flows use the same navigation in the built-in picker. Save adds `.json` when the filename has no extension. |
| [main.cpp](../../src/app/main.cpp) | Initializes the window, OpenGL, ImGui, fonts, and frame loop. Implements startup and glyph smoke modes. | Creates and destroys resources in order. Smoke mode draws hidden frames with persistence disabled. |
| [studio_fonts.hpp](../../src/app/studio_fonts.hpp) | `loadStudioFonts` finds fonts and builds the interface atlas. | Coverage depends on available files. Test atlas range or lookup changes against all six catalogs. |
| [studio_ui.hpp](../../src/editor/studio_ui.hpp) | Declares `StudioApp`, selection, project state, dialogs, and history. | Owns state that spans frames. Keep services and document state with their appropriate owner. |
| [studio_ui.cpp](../../src/editor/studio_ui.cpp) | `commitChange`, `undo`, and `redo` coordinate design and session effects. Implements panels and actions. | Centralized commits keep history, exports, and builds in sync with the document. |
| [studio_canvas.hpp](../../src/editor/studio_canvas.hpp) | Declares `NativeCanvas` and `CanvasUpdate` for gestures. | Proposals communicate a selection or edit to the editor without the gesture owning the project file. |
| [studio_canvas.cpp](../../src/editor/studio_canvas.cpp) | `hitTest` and `process` use visible geometry to resolve drags, resizing, and drop destinations. | An in-progress gesture previews the destination. Commit updates the document in one undo step. |
| [studio_preview.hpp](../../src/editor/studio_preview.hpp) | Defines `PreviewItem`, `PreviewReport`, window state, and `NativePreview`. | Keeps observed geometry, limitations, and transient values separate from design properties. |
| [studio_preview.cpp](../../src/editor/studio_preview.cpp) | Renders controls and inspector properties, records geometry, and restores customized style. | Preview can change transient values. Local styling and design constraints keep the rest of the editor isolated. |

## Trace an edit

Open the [canvas example](../../examples/barra-y-lienzo-libre.json) and drag a child. The renderer publishes `PreviewItem`, the canvas resolves the target, and `StudioApp` commits the resulting project. **Esc** cancels before commit. Trace these steps separately to locate a hit-test, validation, or persistence regression.

## Project file picker

`requestProjectPicker` prepares **Open** or **Save As**. It starts in the current project's folder when available. Opening from the start screen uses the most recent project's folder if known. The first save of a recovered copy can use the original folder. The home directory is the fallback.

`renderProjectFilePicker` calls `GetOpenFileNameW` or `GetSaveFileNameW` when a Windows window is active. The built-in picker uses `ProjectFileBrowser` to list a folder and appends `.json` when saving a filename without an extension. Open passes the path to `openProject`, which loads and validates the file before replacing the design. Save passes the path to `saveProjectTo`, which keeps the external-change check when saving over the current file. An invalid result leaves the selection open and displays its error. Cancellation leaves the current project intact.

## Appearance preferences

`EditorAppearance` stores the palette identifier and rounding in `appearance.json` in the [state directory](../PROJECT_FORMAT.en.md#state-directory). An invalid preference falls back to its default. `applyEditorAppearance` configures application surfaces and the viewport takes its background from the palette. While drawing the project, the canvas saves and restores that style so design properties apply in their own scope.

## Verification

Use `NativeCanvasEditingTest`, `NativeWidgetPreviewTest`, `NativeImagePreviewTest`, and `SessionRecoveryUiTest`. The UI test covers the built-in picker, filenames with accents, rejected JSON, cancellation, and appearance persistence. To check the system picker, open and save from a normal window on Windows. For geometry changes, repeat a manual check with clipping, zoom, and overlapping windows. [Canvas](../CANVAS.en.md)

