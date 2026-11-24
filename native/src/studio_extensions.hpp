#pragma once

#include "studio_knobs_sources.hpp"
#include "studio_project_io.hpp"

#include <atomic>
#include <future>

namespace ImGuiStudio {

using KnobsPackage = std::array<std::string, KnobsSource::files.size()>;

bool catalogHttpAvailable();
bool hasInstalledKnobs(const std::filesystem::path& state);
ProjectSaveResult installKnobsPackage(const std::filesystem::path& state, const KnobsPackage& package);
ProjectSaveResult downloadAndInstallKnobs(const std::filesystem::path& state, const std::atomic<bool>& cancel);
ProjectSaveResult removeKnobsPackage(const std::filesystem::path& state);

class StudioExtensionCatalog {
 public:
  ~StudioExtensionCatalog();
  void load(const std::filesystem::path& state);
  bool installed() const { return installed_; }
  bool busy() const { return job_.valid(); }
  bool canInstall() const { return !state_.empty() && catalogHttpAvailable() && !busy(); }
  void install();
  void cancel() { cancellation_.store(true); }
  void remove();
  void poll();
  const std::string& error() const { return error_; }

 private:
  std::filesystem::path state_;
  std::string error_;
  bool installed_ = false;
  std::atomic<bool> cancellation_{false};
  std::future<ProjectSaveResult> job_;
};

} // namespace ImGuiStudio
