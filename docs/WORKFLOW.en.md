# First project and editor workflow

[Español](WORKFLOW.md)


1. Open the editor and select **New project**. To explore a saved design, open the [title bar and free canvas example](../examples/barra-y-lienzo-libre.json).
2. Insert a window and controls from **Widgets**. Use **Hierarchy** to choose their container and reorganize the tree.
3. Select a node and change its properties in the **Inspector**. Available options depend on the type. Windows expose their title, menu, title controls, and appearance.
4. Use **Preview** to try clicks and values. The editor stores design values and keeps transient preview interactions separate.
5. Save the project as JSON. In **Generated code**, select a language and export to a new directory. Open the generated README for build instructions.
6. To build during design, open **Run / Build**, choose the destination, and configure its tools. Build and run the startup check. Rebuild after changing the design.

## Start screen, saving, and history

| Action | What it does and when to use it |
| --- | --- |
| **New project** | Starts a new design. If the canvas is empty, use **Edit → New window** to create the root container. |
| **Open project** | Opens the file picker. Navigate to the JSON file, select it, and choose **Open**. **Recent projects** opens designs directly from the list. |
| **Save project** / **Save project as…** | The first save and **Ctrl+Shift+S** open a folder and filename picker. Enter a filename, and the editor adds `.json` if it has no extension. Confirm before replacing another file. **Ctrl+S** saves directly after the project has a destination. |
| **Save snapshot** | Stores a version in history. Use **Save project** to update the main file. |
| **View → Version history** | Shows snapshots, their differences, and controls to restore or delete a version. After restoring, save the design to apply that state to the main JSON file. |
| **Recover previous session** | When a recovery copy exists, **Recover copy** opens it as a new unsaved project. Use **Save project as…** to save it. **Later** keeps the copy. Confirming **Discard…** deletes it. |

**Ctrl+O** opens the project dialog. Use **Ctrl+Z** to undo and **Ctrl+Y** or **Ctrl+Shift+Z** to redo. The [project contract](PROJECT_FORMAT.en.md) describes what each file contains.

### Open and save with the file picker

1. To load a design, choose **Open project** on the start screen or **File → Open JSON project…** (**Ctrl+O**). Browse to the design's folder, select the file, and choose **Open**.
2. To save for the first time or make a copy, choose a folder in the picker, enter the filename, and select **Save**. **Save project as…** (**Ctrl+Shift+S**) opens the picker again. The first save and Save As ask before replacing another file.
3. Cancel the picker to keep working on your current project. If loading fails, the editor reports the error and keeps the current design open.

Windows uses the system picker when available. In the built-in picker, click a folder to enter it, choose **Up one level** to go to its parent, or select **Home folder** to return to your home directory. Open mode lists `.json` and `.JSON` files.

## What each panel does

| Panel or control | Purpose |
| --- | --- |
| **Widgets** | Browse controls in the palette. Select a compatible container or window and click a widget to insert it. You can also drag a widget onto its destination. |
| **Hierarchy** | Shows the full tree, including nodes outside the visible canvas. Select a node, use **Move up/Move down**, or drag it to a compatible container. |
| **Canvas** | Design with native windows and controls. Click to select in **IDE** mode and switch to **Preview** to interact with the controls. |
| **Properties inspector** | Set label, variable, ImGui ID, visibility, size, and position where supported. It also edits initial values, choices, colors, and widget-specific settings. **Duplicate** copies the node and **Delete** removes it. |
| **Generated code** | Shows output for the selected language. **Copy** copies what you see and **Export** writes a project with the files needed to build it. |
| **Run / Build** | Uses tools configured in the profile to generate and test a build of the design. **Test startup** exercises the generated host where supported. |
| **View → AI assistant** | Opens provider, model, and connection controls. Review a proposal before applying it as an edit. [Configuration and data sent](ASSISTANT.en.md) |
| **View → Language** | Changes the editor language and saves the selection for the next launch. [Languages and community packs](LOCALIZATION.en.md) |
| **View → Studio appearance** | Changes the editor palette and panel rounding. Also available from the start screen. Studio remembers your selection and the design keeps its own colors. |

## Practice with a design

For a closer look at a step, see [Canvas and layout](CANVAS.en.md), [Title bars](TITLE_BAR.en.md), [Saving and recovery](RECOVERY.en.md), or [Build diagnostics](BUILD_DIAGNOSTICS.en.md).

1. Create a window and select it in the hierarchy. Insert a text control and a button, then set their labels to **System status** and **Refresh** in the inspector.
2. Select and drag the button on the canvas to reorder it. Use **Alt+drag** for free positioning. Drag the resize handle or set **Width** and **Height** in the inspector when available. **View → Snap to grid** snaps gestures to 8-pixel steps.
3. Use **Ctrl+click** to toggle items in the selection. If the controls share a container, drag the selection to move them together. Hold the middle mouse button to pan and use **Ctrl+wheel** to zoom. **Home** resets the view. **Esc** cancels the current drag.
4. Select the window. Under **Title bar**, choose whether to show the title, collapse options, and window buttons. To embed a menu, add a menu as a child of the window and set **Menu bar placement → Title bar**. Drag the side handles of the title bar to adjust its width. Open the [title bar example](../examples/barra-de-titulo.json) to see a complete setup.
5. Select a control and open **Widget colors**. Enabling a custom color overrides the inherited color. Select a window to edit its appearance. Available properties vary by control and exporter. [Widget reference](REFERENCE.en.md#widget-properties-and-runtime-behavior)
6. Insert a checkbox, set its **Initial value**, and switch to **Preview**. Toggle the checkbox and then select **Reset preview** to restore the initial state. **IDE** returns to design mode. Interacting with a checkbox in preview leaves its design value intact.
7. Save the project as JSON. In **Generated code**, choose C++ and **Export CMake…**. Choose a new directory under an existing parent. Read the generated README and `codegen_report.txt` for build steps and target limitations. Connect the **Refresh** action in the application hosting the generated code. [C++ action example](../examples/title-bar-action/README.en.md)

## Customize Studio appearance

1. Choose **Studio appearance** on the start screen or open **View → Studio appearance** from the editor.
2. Under **Editor palette**, choose **Midnight**, **Graphite**, **Ocean**, **Forest**, **Plum**, or the light **Porcelain** theme.
3. Use **Panel rounding** to adjust the radius. The default is 10 and the available range is 0 to 18.

The palette applies to the Studio interface and is saved across sessions. To change the colors of a window or widget in your project, select it on the canvas and use the **Properties inspector**.

## Choose the output

| Goal | Action |
| --- | --- |
| Keep an editable design | **File → Save project**. |
| Try control values and placement | **Preview**. It runs inside ImGui Studio. |
| Integrate code into another application | **Generated code → Export**. For C++, `.hpp` shows the header, `Render()` shows the rendering portion, and the full export includes a host. |
| Launch a separate application | **Native project → Run / Build…**. Select a target and working directory, detect the tools, then select **Build**. Use **Run** once it succeeds. Rebuild when the design or its build settings change. |
| Generate an Android, macOS, or iOS project | Select C++ in **Generated code** and use the appropriate export pack. [Optional targets](EXPORTING.en.md#optional-destinations) |

## Example projects

| File | What to try |
| --- | --- |
| [barra-de-titulo.json](../examples/barra-de-titulo.json) | Menus, buttons, and title appearance. |
| [barra-y-lienzo-libre.json](../examples/barra-y-lienzo-libre.json) | Control positions, sizes, and hierarchy. |
| [estado-y-progreso.json](../examples/estado-y-progreso.json) | Toggle, loading spinner, and circular progress. |
| [vulkan-imagenes.json](../examples/vulkan-imagenes.json) | Checkbox and PNG/JPEG resources in C++ exports. |
| [tarjeta-de-proceso.widget.json](../examples/tarjeta-de-proceso.widget.json) | Import through **Widgets → My library → Add from file**. |

## Library

**Create widget** opens an independent draft. A blank canvas can save several controls as a group. **Insert** creates a copy with its own identifiers, so editing the definition affects future insertions. The **Catalog** activates verified extensions from the editor's curated selection. [Create, import, and install widgets](WIDGET_LIBRARY.en.md)

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| A widget cannot be inserted or is missing | Select a compatible window or container. Find the widget in **Hierarchy** and check visibility, position, size, and clipping by its parent. |
| A checkbox does not change when clicked | Switch to **Preview** to try it. To change its starting state, return to **IDE** and edit the initial value. |
| Title buttons or the menu disappeared | Check that the window shows its title and that the controls appear under that window in the hierarchy. |
| **Run** is disabled | Check whether **Build** succeeded and whether the design, target, or settings changed. Build again with the current profile. [Run / Build diagnostics](RUN_BUILD.en.md) |
| A control appears as a placeholder in the export | Open `codegen_report.txt` and find the node ID. Check whether the selected language supports the feature. [Capabilities](CAPABILITIES.en.md) |
