# Canvas, selection, and layout

[Español](CANVAS.md)

## Choose flow or free positioning

Open [Title bar and free canvas](../examples/barra-y-lienzo-libre.json) from **File → Open JSON project…** and switch to **IDE** mode. In flow layout, order in **Hierarchy** sets the sequence. A row or column sets the direction, while **Flow** wraps to a new row when space runs out. In free layout, each control uses `x` and `y` relative to its container. Use **Alt+drag** or enable **Free position** in the inspector.

| Gesture | Expected result |
| --- | --- |
| Drag to the center of a compatible container | Inserts the node as a child. |
| Drag to the edge of another node | Inserts before or after it at the target's level. |
| Ctrl+click and drag several controls from one container | Moves the group in sibling order. |
| Drag a resize handle when present | Updates supported axes. A single-line input changes width, while a child panel supports both axes. |
| Middle mouse button and Ctrl+wheel | Pan and zoom from 50% to 150%. **Home** resets the view. |
| Esc during a drag | Cancels the edit. Any automatic panning can leave the view shifted. |

The drop marker previews the location. The document updates on release and the whole gesture is undoable. If a widget is hidden, select it in the hierarchy and check whether the parent clips it. Use **Preview** to try interactions. Return to **IDE** to change initial values.

## Tree contract

`planDocumentMove` takes a gap in the original sibling list, from zero through its size. It adjusts the index for items removed before that gap. It checks the destination container, ancestry, depth, and that the selection shares a parent. A tab needs a tab bar. A node cannot move into its own subtree. IDs survive a successful move.

`positionDocumentNodes` rejects non-finite coordinates, enables `useCursorPos`, and clears `sameLine` on commit. `resizeDocumentWidget` validates the type's axes and snaps to eight-unit steps when the grid is active. A rejected edit leaves the input project intact. An unchanged result creates no undo entry.

## Verification

`DocumentEditingTest` covers the model. `NativeCanvasEditingTest` adds mouse and keyboard events, selection, dragging, cancellation, and undo/redo. To reproduce, build both test targets in a [graphical test configuration](BUILD_AND_TEST.en.md#choose-tests) and filter CTest with `DocumentEditingTest|NativeCanvasEditingTest`. For coordinate issues, attach the project, starting zoom, source widget, destination, and a screenshot of the drop marker.

[Document rules](../include/imguistudio/project/studio_document_edit.hpp) · [Canvas gestures](../src/editor/studio_canvas.cpp)
