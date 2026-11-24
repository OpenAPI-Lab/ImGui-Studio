#pragma once

#include "studio_project_io.hpp"
#include "studio_snapshots.hpp"

#include <cstdlib>
#include <memory>
#include <set>
#if !defined(_WIN32)
#include <sys/file.h>
#endif

namespace ImGuiStudio {

inline constexpr size_t kMaxSessionBytes = 64 * 1024 * 1024;
inline constexpr size_t kMaxUndoStates = 40;

// Locks are held by OS handles, not existence of the lock file. A crash releases
// the lock. Keep lock files to avoid unlink/recreate races between processes.
class SessionFileLease {
 public:
  explicit SessionFileLease(const std::filesystem::path& file) {
#if defined(_WIN32)
    handle_ = CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                          OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#else
    handle_ = ::open(file.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (handle_ >= 0 && ::flock(handle_, LOCK_EX | LOCK_NB) != 0) {
      ::close(handle_); handle_ = -1;
    }
#endif
  }
  ~SessionFileLease() {
#if defined(_WIN32)
    if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
#else
    if (handle_ >= 0) { ::flock(handle_, LOCK_UN); ::close(handle_); }
#endif
  }
  SessionFileLease(const SessionFileLease&) = delete;
  SessionFileLease& operator=(const SessionFileLease&) = delete;
  explicit operator bool() const {
#if defined(_WIN32)
    return handle_ != INVALID_HANDLE_VALUE;
#else
    return handle_ >= 0;
#endif
  }
 private:
#if defined(_WIN32)
  HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
  int handle_ = -1;
#endif
};

inline std::filesystem::path sessionLockPath(std::filesystem::path file) {
  file += ".lock";
  return file;
}

inline std::filesystem::path normalizedProjectPath(const std::filesystem::path& file) {
  return std::filesystem::weakly_canonical(std::filesystem::absolute(file));
}

inline ProjectSaveResult saveProjectIfUnchanged(const StudioProject& project,
    const std::filesystem::path& destination, const std::optional<std::string>& expectedState) {
  try {
    if (destination.empty() || destination.filename().empty() || std::filesystem::is_directory(destination))
      return {false, "Choose a project filename"};
    auto directory = normalizedProjectPath(destination);
    if (!std::filesystem::is_directory(directory.parent_path()))
      return {false, "Project destination directory does not exist"};
    directory += ".imguistudio";
    std::filesystem::create_directories(directory);
    SessionFileLease lease(directory / "project.lock");
    if (!lease) return {false, "Project is being saved by another session"};
    if (expectedState) {
      const auto current = loadProjectFile(destination);
      if (!current || projectToJson(*current.project).dump() != *expectedState)
        return {false, "Project changed outside this session. Use Save As to preserve both versions."};
    }
    return saveProjectFile(project, destination);
  } catch (const std::exception& e) { return {false, e.what()}; }
}

inline Json parseSessionJson(const std::string& text) {
  ProjectJson::require(text.size() <= kMaxSessionBytes, "Session exceeds 64 MiB limit");
  return Json::parse(text, [](int depth, Json::parse_event_t, Json&) {
    ProjectJson::require(depth <= static_cast<int>(ProjectJson::kMaxDepth * 3 + 32),
                         "Session exceeds nesting limit");
    return true;
  });
}

inline const char* snapshotKindId(SnapshotKind kind) {
  switch (kind) {
    case SnapshotKind::Manual: return "manual";
    case SnapshotKind::Auto: return "auto";
    case SnapshotKind::RevertBackup: return "revert_backup";
    case SnapshotKind::Milestone: return "milestone";
  }
  throw std::runtime_error("Unknown snapshot kind");
}

inline Json snapshotsToJson(const SnapshotManager& manager) {
  Json list = Json::array();
  for (const auto& snap : manager.snapshots()) {
    auto value = snap.source.is_object() ? snap.source : Json::object();
    value.update(Json{{"id", snap.id}, {"label", snap.label}, {"kind", snapshotKindId(snap.kind)},
      {"createdAt", snap.createdAtMs}, {"windowCount", snap.windowCount},
      {"widgetCount", snap.widgetCount}, {"fingerprint", computeProjectFingerprint(snap.project)},
      {"project", projectToJson(snap.project)}});
    list.push_back(std::move(value));
  }
  return list;
}

inline SnapshotManager snapshotsFromJson(const Json& list) {
  using namespace ProjectJson;
  require(list.is_array() && list.size() <= SnapshotManager::kMaxSnapshots,
          "Invalid snapshot list or snapshot limit exceeded");
  std::vector<ProjectSnapshot> snapshots;
  std::set<std::string> ids;
  for (const auto& value : list) {
    require(value.is_object(), "Snapshot must be an object");
    ProjectSnapshot snap;
    snap.source = value; snap.source.erase("project");
    snap.id = read(value, "id", std::string());
    require(!snap.id.empty() && ids.insert(snap.id).second, "Missing or duplicate snapshot ID");
    snap.label = read(value, "label", std::string());
    snap.createdAtMs = read(value, "createdAt", int64_t{0});
    require(snap.createdAtMs > 0, "Invalid snapshot timestamp");
    const auto kind = read(value, "kind", std::string());
    if (kind == "manual") snap.kind = SnapshotKind::Manual;
    else if (kind == "auto") snap.kind = SnapshotKind::Auto;
    else if (kind == "revert_backup") snap.kind = SnapshotKind::RevertBackup;
    else if (kind == "milestone") snap.kind = SnapshotKind::Milestone;
    else throw std::runtime_error("Unsupported snapshot kind: " + kind);
    require(value.contains("project"), "Snapshot has no project");
    auto project = projectFromJson(value.at("project"));
    require(bool(project), "Invalid snapshot project: " + project.error);
    snap.project = std::move(*project.project);
    const auto stats = countProjectStats(snap.project);
    snap.windowCount = stats.windowCount; snap.widgetCount = stats.widgetCount;
    snap.fingerprint = computeProjectFingerprint(snap.project);
    snapshots.push_back(std::move(snap));
  }
  SnapshotManager manager;
  manager.replaceSnapshots(std::move(snapshots));
  return manager;
}

// One sidecar per project, with optimistic conflict detection under an OS lock.
// Invalid/future-version sidecars are never silently replaced.
class ProjectHistoryFile {
 public:
  ProjectSaveResult open(const std::filesystem::path& project, SnapshotManager& manager) {
    writable_ = false;
    try {
      project_ = normalizedProjectPath(project);
      file_ = project_; file_ += ".imguistudio"; file_ /= "history.json";
      source_ = Json::object(); expected_.reset();
      if (std::filesystem::exists(file_)) {
        const auto input = readTextFile(file_, kMaxSessionBytes);
        if (!input) return {false, input.error};
        auto value = parseSessionJson(input.text);
        ProjectJson::require(value.is_object() && value.at("schemaVersion") == 1,
                             "Unsupported history schema version");
        ProjectJson::require(value.at("projectPath") == project_.u8string(), "History belongs to a different path");
        auto parsed = snapshotsFromJson(value.at("snapshots"));
        source_ = value; source_.erase("snapshots");
        expected_ = input.text;
        manager = std::move(parsed); // All-or-nothing replacement after validation.
      }
      writable_ = true;
      return {true, {}};
    } catch (const std::exception& e) { return {false, e.what()}; }
  }

  ProjectSaveResult save(const SnapshotManager& manager) {
    if (!writable_) return {false, "History is unavailable; existing metadata was preserved"};
    try {
      std::filesystem::create_directories(file_.parent_path());
      SessionFileLease lease(sessionLockPath(file_));
      if (!lease) return {false, "History is being written by another session"};
      const bool exists = std::filesystem::exists(file_);
      if (exists != expected_.has_value()) return {false, "History changed in another session; save a project copy"};
      if (exists) {
        const auto current = readTextFile(file_, kMaxSessionBytes);
        if (!current || current.text != *expected_)
          return {false, "History changed outside this session; existing metadata was preserved"};
      }
      Json value = source_;
      value.update(Json{{"schemaVersion", 1}, {"projectPath", project_.u8string()},
                         {"snapshots", snapshotsToJson(manager)}});
      // Validate generated state too, before replacing a previous good history.
      snapshotsFromJson(value.at("snapshots"));
      const auto bytes = value.dump(2) + "\n";
      auto result = writeTextFileAtomically(bytes, file_, kMaxSessionBytes);
      if (result) expected_ = bytes;
      return result;
    } catch (const std::exception& e) { return {false, e.what()}; }
  }

  const std::filesystem::path& path() const { return file_; }
  void protectExisting() { writable_ = false; }
 private:
  std::filesystem::path project_, file_;
  std::optional<std::string> expected_;
  Json source_ = Json::object();
  bool writable_ = false;
};

struct RecoveryDocument {
  StudioProject project;
  SnapshotManager snapshots;
  std::vector<StudioProject> past, future;
  std::string originalPath;
  int64_t savedAtMs = 0;
};

inline RecoveryDocument recoveryFromJson(const Json& value) {
  using namespace ProjectJson;
  require(value.is_object() && value.at("schemaVersion") == 1, "Unsupported recovery schema version");
  auto parsed = projectFromJson(value.at("project"));
  require(bool(parsed), "Invalid recovery project: " + parsed.error);
  RecoveryDocument document;
  document.project = std::move(*parsed.project);
  document.originalPath = read(value, "originalPath", std::string());
  document.savedAtMs = read(value, "savedAt", int64_t{0});
  require(document.savedAtMs > 0, "Invalid recovery timestamp");
  document.snapshots = snapshotsFromJson(value.at("snapshots"));
  auto states = [&](const char* key, std::vector<StudioProject>& out) {
    const auto& list = value.at(key);
    require(list.is_array() && list.size() <= kMaxUndoStates, "Invalid undo/redo states");
    for (const auto& item : list) {
      auto p = projectFromJson(item);
      require(bool(p), "Invalid undo/redo project: " + p.error);
      out.push_back(std::move(*p.project));
    }
  };
  states("past", document.past); states("future", document.future);
  return document;
}

inline Json recoveryToJson(const RecoveryDocument& document) {
  Json past = Json::array(), future = Json::array();
  for (const auto& p : document.past) past.push_back(projectToJson(p));
  for (const auto& p : document.future) future.push_back(projectToJson(p));
  Json value{{"schemaVersion", 1}, {"savedAt", document.savedAtMs},
    {"originalPath", document.originalPath}, {"project", projectToJson(document.project)},
    {"snapshots", snapshotsToJson(document.snapshots)}, {"past", past}, {"future", future}};
  recoveryFromJson(value);
  return value;
}

inline std::filesystem::path defaultStudioStateDirectory() {
#if defined(_WIN32)
  if (const auto overridePath = _wgetenv(L"IMGUI_STUDIO_STATE_DIR"))
    if (*overridePath) return std::filesystem::path(overridePath);
  if (const auto local = _wgetenv(L"LOCALAPPDATA"))
    if (*local) return std::filesystem::path(local) / "ImGuiStudio";
#else
  if (const auto overridePath = std::getenv("IMGUI_STUDIO_STATE_DIR"))
    if (*overridePath) return std::filesystem::u8path(overridePath);
#if defined(__APPLE__)
  if (const auto home = std::getenv("HOME"))
    if (*home) return std::filesystem::u8path(home) / "Library" / "Application Support" / "ImGuiStudio";
#else
  if (const auto state = std::getenv("XDG_STATE_HOME"))
    if (*state) return std::filesystem::u8path(state) / "ImGuiStudio";
  if (const auto home = std::getenv("HOME"))
    if (*home) return std::filesystem::u8path(home) / ".local" / "state" / "ImGuiStudio";
#endif
#endif
  throw std::runtime_error("Cannot locate application state directory; set IMGUI_STUDIO_STATE_DIR");
}

struct RecoveryCandidate {
  std::filesystem::path file;
  std::string name, originalPath;
  int64_t savedAtMs = 0;
};

class RecoverySession {
 public:
  ProjectSaveResult initialize(const std::filesystem::path& stateDirectory) {
    try {
      directory_ = stateDirectory / "recovery";
      std::filesystem::create_directories(directory_);
      file_ = directory_ / (nextId("session") + ".json");
      lease_ = std::make_unique<SessionFileLease>(sessionLockPath(file_));
      if (!*lease_) return {false, "Cannot acquire recovery session lock"};
      return {true, {}};
    } catch (const std::exception& e) { return {false, e.what()}; }
  }

  ProjectSaveResult checkpoint(const RecoveryDocument& document) const {
    if (!lease_ || !*lease_) return {false, "Recovery storage is unavailable"};
    try { return writeTextFileAtomically(recoveryToJson(document).dump(2) + "\n", file_, kMaxSessionBytes); }
    catch (const std::exception& e) { return {false, e.what()}; }
  }

  std::vector<RecoveryCandidate> scan(std::vector<std::string>& errors) const {
    std::vector<RecoveryCandidate> candidates;
    try {
      size_t inspected = 0;
      for (const auto& entry : std::filesystem::directory_iterator(directory_)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json" || entry.path() == file_) continue;
        if (++inspected > 200) { errors.push_back("Recovery scan limited to 200 records; remaining files were preserved"); break; }
        SessionFileLease lease(sessionLockPath(entry.path()));
        if (!lease) continue; // Still owned by another running editor.
        try {
          auto input = readTextFile(entry.path(), kMaxSessionBytes);
          if (!input) throw std::runtime_error(input.error);
          auto document = recoveryFromJson(parseSessionJson(input.text));
          candidates.push_back({entry.path(), document.project.name, document.originalPath, document.savedAtMs});
        } catch (const std::exception& e) {
          errors.push_back(entry.path().u8string() + ": " + e.what() + " (preserved)");
        }
      }
      std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.savedAtMs > b.savedAtMs; });
    } catch (const std::exception& e) { errors.push_back(e.what()); }
    return candidates;
  }

  // Always re-read under a lease; a startup candidate may since have changed.
  ProjectSaveResult recover(const RecoveryCandidate& candidate, RecoveryDocument& out) const {
    try {
      if (candidate.file.parent_path() != directory_ || candidate.file == file_)
        return {false, "Invalid recovery candidate path"};
      SessionFileLease lease(sessionLockPath(candidate.file));
      if (!lease) return {false, "Recovery belongs to another active session"};
      auto input = readTextFile(candidate.file, kMaxSessionBytes);
      if (!input) return {false, input.error};
      auto document = recoveryFromJson(parseSessionJson(input.text));
      // Transfer only after a durable replacement exists. The original project is
      // never written here. If checkpoint fails, keep the source candidate intact.
      auto saved = checkpoint(document);
      if (!saved) return saved;
      out = std::move(document);
      std::error_code error;
      std::filesystem::remove(candidate.file, error);
      if (error) return {true, "Recovered copy; could not retire old record: " + error.message()};
      return {true, {}};
    } catch (const std::exception& e) { return {false, e.what()}; }
  }

  ProjectSaveResult discard(const RecoveryCandidate& candidate) const {
    try {
      if (candidate.file.parent_path() != directory_ || candidate.file == file_)
        return {false, "Invalid recovery candidate path"};
      SessionFileLease lease(sessionLockPath(candidate.file));
      if (!lease) return {false, "Recovery belongs to another active session"};
      std::filesystem::remove(candidate.file);
      return {true, {}};
    } catch (const std::exception& e) { return {false, e.what()}; }
  }

  ProjectSaveResult clear() const {
    try {
      if (lease_ && *lease_) std::filesystem::remove(file_);
      return {true, {}};
    } catch (const std::exception& e) { return {false, e.what()}; }
  }
  const std::filesystem::path& path() const { return file_; }
 private:
  std::filesystem::path directory_, file_;
  std::unique_ptr<SessionFileLease> lease_;
};

} // namespace ImGuiStudio
