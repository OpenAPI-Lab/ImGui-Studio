# Project, history, and recovery

[Español](PROJECT_FORMAT.md)


## File contract

Project JSON stores design data, including the name, node tree, properties, style, and canvas settings. Node IDs support editing and diagnostics. The tree is validated on load and edit. Files over 32 MiB, 100,000 nodes, or 128 levels fail to load and leave the current document intact. Readers and writers preserve unknown data where the model retains the corresponding source. JSON comparison is semantic and whitespace can change.

[Example project](../examples/barra-de-titulo.json) · [Validator and serializer](../include/imguistudio/project/studio_serialization.hpp)

## Three persistence layers

| File | What it holds | Why it exists |
| --- | --- | --- |
| `project.json` | Design and persistent properties | Makes the design shareable and exportable again. |
| `project.json.imguistudio/history.json` | Project versions | Keeps snapshots separate from the portable design. Stores up to 35 versions. |
| `recovery/` in the state directory | Session checkpoints, history, and undo/redo | Recovers pending changes after an interrupted session. |

Saving checks for external changes and uses temporary-file replacement. Locks coordinate concurrent sessions. A `.lock` file's existence alone does not identify an active owner.

**Recover copy** opens a document without an assigned path. Choose a destination with **Save As**. **Discard** removes the confirmed copy and **Later** keeps it. Changes since the most recent checkpoint may be lost. History conflicts and failures are reported separately from project saves.

## State directory

Windows uses `%LOCALAPPDATA%/ImGuiStudio`. Linux uses `$XDG_STATE_HOME/ImGuiStudio` or `~/.local/state/ImGuiStudio`. macOS uses `~/Library/Application Support/ImGuiStudio`. Set `IMGUI_STUDIO_STATE_DIR` to isolate a test. This directory also holds recent projects, language settings, the library, extensions, and build profiles.

## Verification

`ProjectPersistenceTest` exercises fixtures and writes. `DocumentEditingTest` covers valid and invalid tree operations. `SessionRecoveryTest` covers checkpoints, concurrency, and corrupt files. See the [test map](BUILD_AND_TEST.en.md).
