# Startup, editor UI, and canvas

[Español](EDITOR.md)

The host owns GLFW and the ImGui context. `StudioApp` coordinates the document and uses renderer observations for geometry. Canvas interactions produce edit proposals that are validated before entering undo history.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
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

## Verification

Use `NativeCanvasEditingTest`, `NativeWidgetPreviewTest`, `NativeImagePreviewTest`, and `SessionRecoveryUiTest`. For geometry changes, repeat a manual check with clipping, zoom, and overlapping windows. [Canvas](../CANVAS.en.md)

