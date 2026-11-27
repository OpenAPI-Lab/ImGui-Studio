# Connect the Help button to an action

[Español](README.md)

This example uses the [title bar project](../barra-de-titulo.json) and the **C++ / OpenGL** exporter. **Ayuda** means Help and appears in the panel title. The exporter records the click in `state.chrome_help` for that frame. The host uses that pulse to keep a help window open across frames.

## Integrate and run

1. Open `barra-de-titulo.json` in ImGui Studio. In **Generated code**, export the C++ project with OpenGL to a new directory.
2. Copy [title_bar_help.hpp](title_bar_help.hpp) next to `ui.hpp` and `main.cpp`.
3. In `main.cpp`, add `#include "title_bar_help.hpp"` after `#include "ui.hpp"`. Add `bool helpOpen = false;` beside `state`, before the render loop.
4. Call `RenderTitleBarHelp(state, helpOpen);` immediately after `RenderStudioUI(state);` and before `ImGui::Render()`. Split the calls if the export puts them on the same line.
5. Configure and build from the exported directory.

```sh
cmake -S . -B build -G Ninja
cmake --build build --target studio_export
ctest --test-dir build --output-on-failure
```

Run `build/studio_export.exe` on Windows or `./build/studio_export` on Linux. Click **Ayuda**, close the help window, and click the button again. The design emits the event and the host decides what to do with it. The header uses this project's generated type. If you rename the project or control, consult `ui.hpp` and adjust the example.

The exported project's smoke test covers graphics startup. The repository's `GeneratedCpp_window_chrome` and `GeneratedCppWindowChromeSmoke` tests exercise the title bar control and its host separately.
