# Title bars and actions

[Español](TITLE_BAR.md)

## Design the title bar

Open the [title bar example](../examples/barra-de-titulo.json) and select **Panel de actividad**. The inspector's **Title bar** section controls header visibility and the collapse, minimize, maximize, and close buttons. Those buttons affect the designed window in Preview. Use **Reset preview** to restore geometry and reopen closed windows. Under **Window appearance**, set height, margins, icons, colors, and the separator. Drag either edge of the bar to edit its width.

| To place | How to configure it |
| --- | --- |
| A menu inside the header | Select the child menu and choose **Menu bar placement → Title bar**. |
| A dedicated button | Add a **Title bar button** as a direct child. Choose a side or free position and an icon. Drag horizontally to place it. |
| Text, a checkbox, or a single-line input | Select a supported child and enable **In the title bar**. |
| A bar with fewer controls | Turn off individual controls in the inspector. **Hide title bar** hides the header and its embedded controls. |

## Connect a click

The example's **Ayuda** button emits a one-frame pulse in `state.chrome_help`. An open Help window needs host-owned state because the next frame clears the pulse. In exported C++, keep a boolean outside the render loop and handle the pulse immediately after `RenderStudioUI(state)`. The [integration example](../examples/title-bar-action/README.en.md) provides a header to copy, the call site, and build commands.

Check `codegen_report.txt` when switching languages. C# and Java draw embedded controls such as checkboxes and inputs while the body is open. Dedicated title buttons and window controls stay available when it collapses. The host supplies its own menu actions.

## Verification and contributions

`GeneratedCpp_window_chrome` checks exported state and `GeneratedCppWindowChromeSmoke` starts the OpenGL host. For placement or style changes, run both and open the example in **Preview** with its body open and collapsed. Include the title-bar size and target language when reporting a difference.

[Native rendering](../src/editor/studio_preview.cpp) · [Title control generation](../include/imguistudio/export/detail/studio_codegen_titlebar.hpp)
