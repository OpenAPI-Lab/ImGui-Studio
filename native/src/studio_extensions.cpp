#include "studio_extensions.hpp"
#include "studio_assistant_http.hpp"
#include "studio_session_store.hpp"

#include <chrono>

namespace ImGuiStudio {
namespace {
constexpr size_t packageLimit = 64 * 1024;

std::filesystem::path packagePath(const std::filesystem::path& state) {
  return state / "extensions" / "imgui-knobs.json";
}
}

bool hasInstalledKnobs(const std::filesystem::path& state) {
  if (state.empty()) return false;
  try {
    const auto installed = readTextFile(packagePath(state), packageLimit);
    if (!installed) return false;
    const auto manifest = parseSessionJson(installed.text);
    if (!manifest.is_object() || manifest.value("revision", std::string{}) != KnobsSource::revision) return false;
    const auto& files = manifest.at("files");
    if (!files.is_object() || files.size() != KnobsSource::files.size()) return false;
    for (const auto& file : KnobsSource::files)
      if (files.value(file.name, std::string{}) != file.contents) return false;
    return true;
  } catch (const std::exception&) { return false; }
}

ProjectSaveResult installKnobsPackage(const std::filesystem::path& state, const KnobsPackage& package) {
  if (state.empty()) return {false, "The catalog state directory is unavailable."};
  try {
    Json files = Json::object();
    for (size_t i = 0; i < package.size(); ++i) {
      const auto& source = KnobsSource::files[i];
      if (package[i] != source.contents)
        return {false, std::string("Downloaded file differs from the reviewed revision: ") + source.name};
      files[source.name] = package[i];
    }
    const auto path = packagePath(state);
    std::filesystem::create_directories(path.parent_path());
    SessionFileLease lease(sessionLockPath(path));
    if (!lease) return {false, "The catalog is being updated by another session."};
    const Json manifest{{"revision", KnobsSource::revision}, {"files", std::move(files)}};
    return writeTextFileAtomically(manifest.dump(), path, packageLimit);
  } catch (const std::exception& error) { return {false, error.what()}; }
}

ProjectSaveResult downloadAndInstallKnobs(const std::filesystem::path& state, const std::atomic<bool>& cancel) {
  KnobsPackage package;
  for (size_t i = 0; i < package.size(); ++i) {
    const auto& expected = KnobsSource::files[i];
    auto response = downloadCatalogFile(std::string(KnobsSource::sourceBase) + expected.name, cancel);
    if (!response.error.empty()) return {false, std::move(response.error)};
    if (response.body != expected.contents)
      return {false, std::string("Downloaded file differs from the reviewed revision: ") + expected.name};
    package[i] = std::move(response.body);
  }
  if (cancel.load()) return {false, "Download cancelled."};
  return installKnobsPackage(state, package);
}

ProjectSaveResult removeKnobsPackage(const std::filesystem::path& state) {
  if (state.empty()) return {false, "The catalog state directory is unavailable."};
  try {
    const auto path = packagePath(state);
    if (!std::filesystem::exists(path.parent_path())) return {true, {}};
    SessionFileLease lease(sessionLockPath(path));
    if (!lease) return {false, "The catalog is being updated by another session."};
    std::filesystem::remove(path);
    return {true, {}};
  } catch (const std::exception& error) { return {false, error.what()}; }
}

StudioExtensionCatalog::~StudioExtensionCatalog() {
  cancel();
  if (job_.valid()) job_.wait();
}

void StudioExtensionCatalog::load(const std::filesystem::path& state) {
  state_ = state;
  installed_ = hasInstalledKnobs(state_);
}

void StudioExtensionCatalog::install() {
  if (!canInstall()) return;
  error_.clear();
  cancellation_.store(false);
  try {
    job_ = std::async(std::launch::async, [this, state = state_] {
      return downloadAndInstallKnobs(state, cancellation_);
    });
  } catch (const std::exception& error) { error_ = error.what(); }
}

void StudioExtensionCatalog::remove() {
  if (busy()) return;
  const auto result = removeKnobsPackage(state_);
  installed_ = hasInstalledKnobs(state_);
  error_ = result.error;
}

void StudioExtensionCatalog::poll() {
  if (!busy() || job_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
  try { error_ = job_.get().error; }
  catch (const std::exception& error) { error_ = error.what(); }
  installed_ = hasInstalledKnobs(state_);
}

} // namespace ImGuiStudio
