#pragma once

#include "studio_serialization.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace ImGuiStudio {

enum class SnapshotKind {
  Manual,
  Auto,
  RevertBackup,
  Milestone
};

inline std::string snapshotKindLabel(SnapshotKind kind) {
  switch (kind) {
    case SnapshotKind::Manual: return "Manual";
    case SnapshotKind::Auto: return "Auto";
    case SnapshotKind::RevertBackup: return "Respaldo";
    case SnapshotKind::Milestone: return "Hito";
  }
  return "Manual";
}

struct ProjectSnapshot {
  Json source;
  std::string id;
  std::string label;
  SnapshotKind kind = SnapshotKind::Manual;
  int64_t createdAtMs = 0;
  int windowCount = 0;
  int widgetCount = 0;
  std::string fingerprint;
  StudioProject project;
};

inline int64_t nowUnixMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

inline std::string computeProjectFingerprint(const StudioProject& proj) {
  const std::string serialized = projectToJson(proj).dump();
  uint64_t hash = 0xcbf29ce484222325ULL;
  for (unsigned char ch : serialized) {
    hash ^= ch;
    hash *= 0x100000001b3ULL;
  }
  std::ostringstream result;
  result << std::hex << hash << '_' << std::dec << serialized.size();
  return result.str();
}

class SnapshotManager {
 public:
  static constexpr size_t kMaxSnapshots = 35;

  const std::vector<ProjectSnapshot>& snapshots() const { return snapshots_; }

  // Callers validate a complete persisted history before replacing live state.
  void replaceSnapshots(std::vector<ProjectSnapshot> snapshots) {
    snapshots_ = std::move(snapshots);
    prune();
  }

  bool saveSnapshot(const StudioProject& proj,
                    const std::string& label,
                    SnapshotKind kind,
                    bool skipIfSameAsLatest = false) {
    const std::string fp = computeProjectFingerprint(proj);
    if (skipIfSameAsLatest && !snapshots_.empty() && snapshots_.front().fingerprint == fp) {
      return false;
    }

    const ProjectStats stats = countProjectStats(proj);
    ProjectSnapshot snap;
    snap.id = nextId("snap");
    snap.label = label.empty() ? ("Versión #" + std::to_string(snapshots_.size() + 1)) : label;
    snap.kind = kind;
    snap.createdAtMs = nowUnixMs();
    snap.windowCount = stats.windowCount;
    snap.widgetCount = stats.widgetCount;
    snap.fingerprint = fp;
    snap.project = proj;

    snapshots_.insert(snapshots_.begin(), std::move(snap));
    prune();
    return true;
  }

  std::optional<StudioProject> revertToSnapshot(const std::string& snapshotId,
                                                const StudioProject& currentLiveProject) {
    const ProjectSnapshot* target = nullptr;
    for (const auto& s : snapshots_) {
      if (s.id == snapshotId) {
        target = &s;
        break;
      }
    }
    if (!target) return std::nullopt;

    // Copy target project before mutating snapshots_
    StudioProject restored = target->project;
    std::string targetLabel = target->label;

    const std::string currentFp = computeProjectFingerprint(currentLiveProject);
    if (snapshots_.empty() || snapshots_.front().fingerprint != currentFp) {
      saveSnapshot(currentLiveProject,
                   "Respaldo previo a revertir (" + targetLabel + ")",
                   SnapshotKind::RevertBackup,
                   true);
    }
    return restored;
  }

  bool renameSnapshot(const std::string& snapshotId, const std::string& newLabel) {
    if (newLabel.empty()) return false;
    for (auto& s : snapshots_) {
      if (s.id == snapshotId) {
        s.label = newLabel;
        return true;
      }
    }
    return false;
  }

  bool deleteSnapshot(const std::string& snapshotId) {
    for (auto it = snapshots_.begin(); it != snapshots_.end(); ++it) {
      if (it->id == snapshotId) {
        snapshots_.erase(it);
        return true;
      }
    }
    return false;
  }

  void clearAutoSnapshots() {
    snapshots_.erase(
        std::remove_if(snapshots_.begin(), snapshots_.end(),
                       [](const ProjectSnapshot& s) { return s.kind == SnapshotKind::Auto; }),
        snapshots_.end());
  }

  std::string describeDiff(const ProjectSnapshot& snap, const StudioProject& current) const {
    if (snap.fingerprint == computeProjectFingerprint(current)) {
      return "Idéntico al diseño actual";
    }
    const ProjectStats cur = countProjectStats(current);
    std::ostringstream os;
    int dw = snap.windowCount - cur.windowCount;
    int dg = snap.widgetCount - cur.widgetCount;
    bool needSep = false;
    if (dw != 0) {
      os << (dw > 0 ? "+" : "") << dw << " vent.";
      needSep = true;
    }
    if (dg != 0) {
      if (needSep) os << " · ";
      os << (dg > 0 ? "+" : "") << dg << " widget" << (std::abs(dg) == 1 ? "" : "s");
      needSep = true;
    }
    if (snap.project.style.preset != current.style.preset) {
      if (needSep) os << " · ";
      os << "estilo " << stylePresetName(snap.project.style.preset);
      needSep = true;
    }
    if (!needSep) {
      os << "propiedades o posiciones distintas";
    }
    return os.str();
  }

 private:
  std::vector<ProjectSnapshot> snapshots_;

  void prune() {
    while (snapshots_.size() > kMaxSnapshots) {
      // Prefer dropping oldest Auto snapshot first
      auto it = std::find_if(snapshots_.rbegin(), snapshots_.rend(),
                             [](const ProjectSnapshot& s) { return s.kind == SnapshotKind::Auto; });
      if (it != snapshots_.rend()) {
        snapshots_.erase(std::next(it).base());
      } else {
        snapshots_.pop_back();
      }
    }
  }
};

}  // namespace ImGuiStudio
