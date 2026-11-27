# Architecture

[Español](ARCHITECTURE.md)


| Layer | Responsibility and rationale |
| --- | --- |
| Graphics host | [src/app/main.cpp](../src/app/main.cpp) starts GLFW, OpenGL, Dear ImGui, and the fonts. It owns the event loop and hands each frame to `StudioApp`. |
| Editor | [studio_ui.cpp](../src/editor/studio_ui.cpp) coordinates dialogs, panels, and the session. [studio_canvas.cpp](../src/editor/studio_canvas.cpp) handles selection, geometry, and dragging. |
| Document | [studio_types.hpp](../include/imguistudio/project/studio_types.hpp) defines nodes, properties, style, and canvas settings. [studio_document_edit.hpp](../include/imguistudio/project/studio_document_edit.hpp) validates structural operations so callers share rules. |
| Preview | [studio_preview.cpp](../src/editor/studio_preview.cpp) draws controls and holds transient state. Preview interactions are separated from saved properties. |
| Disk | [studio_serialization.hpp](../include/imguistudio/project/studio_serialization.hpp) interprets JSON. [studio_project_io.hpp](../include/imguistudio/project/studio_project_io.hpp) reads and replaces files. [studio_session_store.hpp](../include/imguistudio/project/studio_session_store.hpp) handles history, recovery, and concurrency. |
| Export | Generators visit the document and produce code, node line ranges, and diagnostics. Bundling adds the host and applicable dependencies or resources. |
| Build | [studio_build.hpp](../include/imguistudio/build/studio_build.hpp) exports the live design, configures the toolchain, and manages the artifact. [studio_process.hpp](../include/imguistudio/build/studio_process.hpp) captures output and controls child processes. |
| Services | `studio_assistant_*` separates provider HTTP, credential storage, and proposal validation. `studio_extensions.*` manages verified catalog installation. |

## Trace a change

Dragging starts from geometry observed in preview. The canvas computes the destination and the editor commits a tree edit. The resulting document can be saved or exported. Recovery writes separate checkpoints while editing. A build takes the live design and writes its own output directory.

The distinction between model, rendering, and execution explains why JSON can retain a node when a particular exporter emits a placeholder. See the [format](PROJECT_FORMAT.en.md), [exports](EXPORTING.en.md), and [source map](SOURCE_MAP.en.md).
