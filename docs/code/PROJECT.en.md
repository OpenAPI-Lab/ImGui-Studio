# Model, serialization, and persistence

[Español](PROJECT.md)

`StudioProject` is the shared model used by the editor and exporters. JSON interpretation, tree edits, file replacement, and history each return their own result so callers can handle each failure at the right boundary.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [studio_types.hpp](../../include/imguistudio/project/studio_types.hpp) | Defines `StudioProject`, `ImGuiNode`, `WidgetProps`, and `canAcceptChild`. | It owns hierarchy rules. A moved node keeps its ID and retains source data for serialization. |
| [studio_widget_registry.inc](../../include/imguistudio/project/studio_widget_registry.inc) | The `STUDIO_WIDGET` macro defines the enum, JSON ID, and container flag. | The shared registry prevents divergent mappings. Preserve persistent IDs and existing enum order. |
| [studio_serialization.hpp](../../include/imguistudio/project/studio_serialization.hpp) | `parseProject` bounds input, `projectFromJson` validates it, and `projectToJson` rebuilds the file. | An unknown type is preserved with a warning. Duplicate IDs, invalid hierarchy, and size limits produce errors. Comparison is semantic. |
| [studio_document_edit.hpp](../../include/imguistudio/project/studio_document_edit.hpp) | Plans insertion and movement, applies position and size, and returns `DocumentEditResult`. | The candidate is validated before commit. An invalid edit preserves the input and an identical result creates no undo entry. |
| [studio_project_io.hpp](../../include/imguistudio/project/studio_project_io.hpp) | `readTextFile` bounds reads and `writeTextFileAtomically` writes and replaces through an adjacent temporary file. | The temporary file is written and flushed before replacement. Session code owns history and conflict detection. |
| [studio_snapshots.hpp](../../include/imguistudio/project/studio_snapshots.hpp) | `SnapshotManager` captures and compares designs, limits retention, and returns a copy on restore. | Versions live outside the main file. Restoring can retain the state being left behind. |
| [studio_session_store.hpp](../../include/imguistudio/project/studio_session_store.hpp) | `SessionFileLease`, `ProjectHistoryFile`, and `RecoverySession` coordinate revisions, locks, and checkpoints. | The revision is checked under the lock. Failed recovery and future formats require preserving data and surfacing the error. |

## Contract example

`moveDocumentNode(project, "job_progress", "status_window", 0)` proposes moving an existing node to the first gap in a container. Use the returned project only when the result contains one. A result without a project or `error` means no edit was applied. See the [editing fixture](../../tests/verify_document_edit.cpp) for accepted and rejected inputs.

### C++ example

Pass `examples/estado-y-progreso.json` as the argument. The example compares the proposed tree with the original and leaves the file untouched.

```cpp
#include "imguistudio/project/studio_document_edit.hpp"
#include "imguistudio/project/studio_project_io.hpp"
#include <cassert>

int main(int argc, char** argv) {
    using namespace ImGuiStudio;
    if (argc != 2) return 2;
    const auto loaded = loadProjectFile(std::filesystem::u8path(argv[1]));
    if (!loaded) return 1;

    const auto result = moveDocumentNode(*loaded.project, "job_progress",
                                         "status_window", 0);
    if (!result || !result.error.empty()) return 1;
    const auto* window = findNodeById(result.project->nodes, "status_window");
    assert(window && window->children.front().id == "job_progress");
    assert(loaded.project->nodes.front().children.front().id == "auto_toggle");
}
```

## Verification

Run `CoreAndCodegenTest`, `ProjectPersistenceTest`, `DocumentEditingTest`, and `SessionRecoveryTest`. For format changes, include an older design and check that unknown data survives the round trip. [Project format](../PROJECT_FORMAT.en.md) · [Recovery](../RECOVERY.en.md)

