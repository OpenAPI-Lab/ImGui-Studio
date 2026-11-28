#pragma once

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ImGuiStudio {

inline std::filesystem::path projectFileHome() {
#ifdef _WIN32
  if (const auto* home = _wgetenv(L"USERPROFILE"); home && *home) return home;
#else
  if (const auto* home = std::getenv("HOME"); home && *home) return home;
#endif
  std::error_code error;
  return std::filesystem::current_path(error);
}

struct ProjectFileBrowser {
  struct Entry {
    std::filesystem::path path;
    bool directory;
  };

  std::filesystem::path folder;
  std::string filename;
  std::vector<Entry> entries;

  std::error_code selectFolder(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::directory_iterator entry(path, error), end;
    std::vector<Entry> listed;
    for (; !error && entry != end; entry.increment(error)) {
      std::error_code typeError;
      const auto& item = *entry;
      if (item.is_directory(typeError)) listed.push_back({item.path(), true});
      else if (item.is_regular_file(typeError)) {
        auto extension = item.path().extension().u8string();
        for (auto& c : extension) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (extension == ".json") listed.push_back({item.path(), false});
      }
    }
    if (error) return error;
    std::sort(listed.begin(), listed.end(), [](const Entry& a, const Entry& b) {
      if (a.directory != b.directory) return a.directory;
      return a.path.filename() < b.path.filename();
    });
    entries = std::move(listed);
    folder = path;
    return {};
  }

  std::optional<std::filesystem::path> destination() const {
    if (folder.empty() || filename.empty() || filename == "." || filename == ".." ||
        filename.find_first_of("/\\") != std::string::npos ||
        filename.find('\0') != std::string::npos) return std::nullopt;
    auto name = std::filesystem::u8path(filename);
    if (name.has_root_path()) return std::nullopt;
    if (name.extension().empty()) name += ".json";
    return folder / name;
  }
};

} // namespace ImGuiStudio
