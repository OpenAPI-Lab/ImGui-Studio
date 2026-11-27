# Saving, version history, and recovery

[Español](RECOVERY.md)

## What each action restores

| Action | Result |
| --- | --- |
| **Save project** | Writes the current design to the JSON file. |
| **Save snapshot** | Captures a version in the accompanying history. |
| **Restore** in version history | Loads a version into the editor. History can retain a backup of the previous state. Save the project to update the JSON file. |
| **Recover copy** at startup | Opens pending work as a project without a path. Use **Save project as…** to write it to disk. |
| **Later** | Keeps the copy for **File → Recover previous sessions…**. |
| **Discard… → Yes, discard this copy** | Deletes that recovery copy. |

## Try it with a sample file

Open [Status and progress](../examples/estado-y-progreso.json) and save it under a new name. Open **View → Version history** and save a snapshot called “Before change.” Edit the **Procesado** label in the inspector and save the project. Restore “Before change” and observe the label in the editor. Choose **Save project** to persist that state.

If the editor says the file changed after you opened it, use **Save project as…** to keep your edits in a separate file. Compare the two versions before replacing the original. Keep a copy of the history file before repairing corrupt metadata by hand.

## How persistence protects the design

`saveProjectIfUnchanged` takes a lock and compares the project contents on save to detect conflicts. Replacement writes and flushes a temporary file in the same directory. The companion history keeps up to 35 snapshots. Recovery includes the design and up to 40 states in each undo and redo stack. The editor spaces checkpoint attempts two seconds apart. An abrupt stop can lose the last edits. Locks are held by the operating system. A `.lock` file by itself does not indicate an active session.

## Verification

`SessionRecoveryTest` covers conflicts, live locks, interrupted sessions, and malformed records. `SessionRecoveryUiTest` renders the recovery dialog in a Dear ImGui context. When reporting a problem, include the open, save, recover, or discard result on a sample project and the exact message. Set `IMGUI_STUDIO_STATE_DIR` to isolate an experiment.

[Project format](PROJECT_FORMAT.en.md) · [Locking and recovery](../include/imguistudio/project/studio_session_store.hpp) · [File writes](../include/imguistudio/project/studio_project_io.hpp)
