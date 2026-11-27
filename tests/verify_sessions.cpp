#include "imguistudio/project/studio_session_store.hpp"

#include <cassert>
#include <iostream>

using namespace ImGuiStudio;

static StudioProject example() {
  auto project = createDefaultProject("Sesion de prueba");
  auto checkbox = createDefaultNode(WidgetType::Checkbox);
  checkbox.props.source = Json{{"extension", {{"future", {1, 2, 3}}}}};
  project.nodes[0].children.push_back(checkbox);
  return project;
}

int main(int argc, char** argv) {
  if (argc == 3 && std::string(argv[1]) == "--leave-recovery") {
    RecoverySession session;
    assert(session.initialize(std::filesystem::u8path(argv[2])));
    RecoveryDocument draft;
    draft.project = example(); draft.project.name = "Process interruption";
    draft.past = {example()}; draft.savedAtMs = nowUnixMs();
    draft.snapshots.saveSnapshot(draft.project, "Before interruption", SnapshotKind::Manual);
    assert(session.checkpoint(draft));
    std::_Exit(0); // No C++ cleanup: the OS must release the lease.
  }
  if (argc == 3 && std::string(argv[1]) == "--check-recovery") {
    RecoverySession session;
    assert(session.initialize(std::filesystem::u8path(argv[2])));
    std::vector<std::string> errors;
    auto candidates = session.scan(errors);
    assert(errors.empty() && candidates.size() == 1);
    RecoveryDocument draft;
    assert(session.recover(candidates[0], draft));
    assert(draft.project.name == "Process interruption");
    assert(draft.past.size() == 1 && draft.snapshots.snapshots().size() == 1);
    assert(session.clear());
    std::cout << "[OK] Recovery after process termination without C++ cleanup.\n";
    return 0;
  }
  const auto directory = std::filesystem::temp_directory_path() / nextId("studio_sessions_test");
  std::filesystem::create_directory(directory);
  const auto projectFile = directory / std::filesystem::u8path(u8"proyecto_日本.json");
  auto project = example();
  assert(saveProjectFile(project, projectFile));
  const auto conflictFile = directory / "conflict.json";
  assert(saveProjectFile(project, conflictFile));
  const auto expectedState = projectToJson(project).dump();
  auto external = project;
  external.name = "Guardado por otra instancia";
  assert(saveProjectIfUnchanged(external, conflictFile, expectedState));
  assert(!saveProjectIfUnchanged(project, conflictFile, expectedState));
  assert(loadProjectFile(conflictFile).project->name == external.name);
  {
    auto folder = conflictFile; folder += ".imguistudio";
    SessionFileLease lease(folder / "project.lock");
    assert(lease);
    assert(!saveProjectIfUnchanged(project, conflictFile, std::nullopt));
  }

  SnapshotManager manager;
  assert(manager.saveSnapshot(project, "Base", SnapshotKind::Milestone));
  ProjectHistoryFile first;
  assert(first.open(projectFile, manager));
  assert(first.save(manager));
  const auto initialHistory = readTextFile(first.path(), kMaxSessionBytes).text;
  SnapshotManager reopened;
  ProjectHistoryFile second;
  assert(second.open(projectFile, reopened));
  assert(snapshotsToJson(reopened) == snapshotsToJson(manager));
  assert(reopened.renameSnapshot(reopened.snapshots()[0].id, "Renombrada"));
  assert(second.save(reopened));
  assert(!first.save(manager)); // A stale instance must not overwrite newer metadata.
  assert(readTextFile(first.path(), kMaxSessionBytes).text != initialHistory);
  {
    SessionFileLease lease(sessionLockPath(second.path()));
    assert(lease);
    assert(!second.save(reopened));
  }
  assert(second.save(reopened));
  ProjectHistoryFile protectedHistory;
  SnapshotManager targetHistory;
  assert(protectedHistory.open(projectFile, targetHistory));
  const auto targetBytes = readTextFile(second.path(), kMaxSessionBytes).text;
  protectedHistory.protectExisting();
  assert(!protectedHistory.save(manager));
  assert(readTextFile(second.path(), kMaxSessionBytes).text == targetBytes);

  // An invalid import never clears the existing in-memory history or file.
  assert(writeTextFileAtomically("{broken history", second.path()));
  ProjectHistoryFile corrupt;
  const auto before = snapshotsToJson(reopened);
  assert(!corrupt.open(projectFile, reopened));
  assert(snapshotsToJson(reopened) == before);
  assert(!corrupt.save(reopened));
  assert(readTextFile(second.path(), kMaxSessionBytes).text == "{broken history");
  assert(writeTextFileAtomically(Json{{"schemaVersion", 99}}.dump(), second.path()));
  assert(!corrupt.open(projectFile, reopened));
  assert(!corrupt.save(reopened));
  assert(parseSessionJson(readTextFile(second.path(), kMaxSessionBytes).text)["schemaVersion"] == 99);

  auto modified = project;
  modified.name = "Cambios sin guardar";
  RecoveryDocument draft;
  draft.project = modified; draft.snapshots = manager;
  draft.past = {project}; draft.future = {modified};
  draft.originalPath = projectFile.u8string(); draft.savedAtMs = nowUnixMs();
  assert(projectToJson(recoveryFromJson(recoveryToJson(draft)).project) == projectToJson(modified));
  auto invalidSnapshots = snapshotsToJson(manager);
  invalidSnapshots.push_back(invalidSnapshots[0]);
  bool rejected = false;
  try { snapshotsFromJson(invalidSnapshots); } catch (const std::exception&) { rejected = true; }
  assert(rejected);
  auto invalidRecovery = recoveryToJson(draft);
  for (size_t i = 0; i <= kMaxUndoStates; ++i) invalidRecovery["past"].push_back(projectToJson(project));
  rejected = false;
  try { recoveryFromJson(invalidRecovery); } catch (const std::exception&) { rejected = true; }
  assert(rejected);
  invalidRecovery = recoveryToJson(draft);
  invalidRecovery["future"] = "invalid";
  rejected = false;
  try { recoveryFromJson(invalidRecovery); } catch (const std::exception&) { rejected = true; }
  assert(rejected);

  RecoverySession reader;
  assert(reader.initialize(directory / "state"));
  std::filesystem::path orphan;
  {
    RecoverySession writer;
    assert(writer.initialize(directory / "state"));
    assert(writer.checkpoint(draft));
    orphan = writer.path();
    std::vector<std::string> errors;
    assert(reader.scan(errors).empty()); // The lease hides a live session.
    assert(errors.empty());
    // Release the lease without cleanup, simulating a terminated editor.
  }
  std::vector<std::string> errors;
  auto candidates = reader.scan(errors);
  assert(errors.empty() && candidates.size() == 1);
  assert(candidates[0].name == "Cambios sin guardar");
  RecoveryDocument recovered;
  assert(reader.recover(candidates[0], recovered));
  assert(!std::filesystem::exists(orphan));
  assert(std::filesystem::exists(reader.path()));
  assert(projectToJson(recovered.project) == projectToJson(modified));
  assert(projectToJson(recovered.past[0]) == projectToJson(project));
  assert(projectToJson(recovered.future[0]) == projectToJson(modified));
  assert(snapshotsToJson(recovered.snapshots) == snapshotsToJson(manager));
  assert(projectToJson(*loadProjectFile(projectFile).project) == projectToJson(project));
  assert(reader.clear());
  assert(!std::filesystem::exists(reader.path()));

  // Failed transfer preserves both the candidate and the caller's current state.
  RecoverySession noStorage;
  {
    RecoverySession writer;
    assert(writer.initialize(directory / "state"));
    assert(writer.checkpoint(draft));
    orphan = writer.path();
  }
  candidates = reader.scan(errors);
  assert(candidates.size() == 1);
  // Put an inaccessible destination (a directory) at the active checkpoint path.
  std::filesystem::create_directory(reader.path());
  const auto previouslyRecovered = projectToJson(recovered.project);
  assert(!reader.recover(candidates[0], recovered));
  assert(std::filesystem::exists(orphan));
  assert(projectToJson(recovered.project) == previouslyRecovered);
  std::filesystem::remove(reader.path());
  assert(reader.discard(candidates[0]));
  assert(!std::filesystem::exists(orphan));

  // Corrupt or future-version recovery files remain intact and are reported.
  const auto damaged = reader.path().parent_path() / "damaged.json";
  assert(writeTextFileAtomically("{damaged", damaged));
  const auto future = reader.path().parent_path() / "future.json";
  assert(writeTextFileAtomically("{\"schemaVersion\":99}", future));
  errors.clear();
  assert(reader.scan(errors).empty());
  assert(errors.size() == 2);
  assert(readTextFile(damaged, kMaxSessionBytes).text == "{damaged");
  assert(std::filesystem::exists(future));
  assert(!reader.discard({projectFile, {}, {}, 0})); // Never remove unrelated paths.

  // Only this test-owned directory is removed. Release handles before cleanup.
  assert(reader.clear());
  reader = RecoverySession{};
  noStorage = RecoverySession{};
  assert(std::filesystem::weakly_canonical(directory).parent_path() ==
         std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()));
  assert(directory.filename().string().find("studio_sessions_test_") == 0);
  std::filesystem::remove_all(directory);
  std::cout << "[OK] Durable snapshots, conflict detection, live leases, crash recovery and corruption preservation.\n";
}
