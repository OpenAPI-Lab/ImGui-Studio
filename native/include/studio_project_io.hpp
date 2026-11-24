#pragma once

#include "studio_serialization.hpp"

#include <filesystem>
#include <fstream>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ImGuiStudio {

struct TextFileResult {
  std::string text;
  std::string error;
  explicit operator bool() const { return error.empty(); }
};

inline TextFileResult readTextFile(const std::filesystem::path& path, size_t limit) {
  try {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return {{}, "Cannot open file"};
    std::string text;
    std::array<char, 8192> buffer{};
    while (stream) {
      stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      const auto count = static_cast<size_t>(stream.gcount());
      if (text.size() + count > limit) return {{}, "File exceeds size limit"};
      text.append(buffer.data(), count);
    }
    if (stream.bad()) return {{}, "Failed reading file"};
    return {std::move(text), {}};
  } catch (const std::exception& e) { return {{}, e.what()}; }
}

inline ProjectLoadResult loadProjectFile(const std::filesystem::path& path) {
  auto input = readTextFile(path, ProjectJson::kMaxProjectBytes);
  if (!input) return {std::nullopt, {}, input.error};
  return parseProject(input.text);
}

struct ProjectSaveResult {
  bool ok = false;
  std::string error;
  explicit operator bool() const { return ok; }
};

// Only a temporary sibling is written. The destination is replaced after the
// complete file has been flushed; failure leaves the existing destination intact.
inline ProjectSaveResult writeTextFileAtomically(const std::string& bytes,
    const std::filesystem::path& destination, size_t limit = ProjectJson::kMaxProjectBytes) {
  std::filesystem::path temporary;
  bool ownsTemporary = false;
  auto cleanup = [&] {
    if (ownsTemporary) {
      std::error_code ignored;
      std::filesystem::remove(temporary, ignored);
    }
  };
  try {
    if (bytes.size() > limit) return {false, "File exceeds size limit"};
    if (destination.empty() || destination.filename().empty()) return {false, "Choose a project filename"};
    temporary = destination;
    temporary += "." + nextId("saving") + ".tmp";
#if defined(_WIN32)
    const HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
                                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
      return {false, "Cannot create temporary file (Windows error " + std::to_string(GetLastError()) + ")"};
    ownsTemporary = true;
    size_t offset = 0;
    bool written = true;
    DWORD error = ERROR_SUCCESS;
    while (offset < bytes.size()) {
      DWORD count = 0;
      if (!WriteFile(file, bytes.data() + offset, static_cast<DWORD>(bytes.size() - offset), &count, nullptr) || count == 0) {
        error = GetLastError(); written = false; break;
      }
      offset += count;
    }
    if (written && !FlushFileBuffers(file)) { error = GetLastError(); written = false; }
    if (!CloseHandle(file)) { error = GetLastError(); written = false; }
    if (!written) { cleanup(); return {false, "Failed writing project (Windows error " + std::to_string(error) + ")"}; }
    if (!MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
      error = GetLastError(); cleanup();
      return {false, "Cannot replace project file (Windows error " + std::to_string(error) + ")"};
    }
#else
    const int file = ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (file < 0) return {false, std::error_code(errno, std::generic_category()).message()};
    ownsTemporary = true;
    size_t offset = 0;
    int error = 0;
    while (offset < bytes.size()) {
      const auto count = ::write(file, bytes.data() + offset, bytes.size() - offset);
      if (count < 0 && errno == EINTR) continue;
      if (count <= 0) { error = count < 0 ? errno : EIO; break; }
      offset += static_cast<size_t>(count);
    }
    if (!error && ::fsync(file) != 0) error = errno;
    if (::close(file) != 0 && !error) error = errno;
    if (error) { cleanup(); return {false, std::error_code(error, std::generic_category()).message()}; }
    std::error_code renameError;
    std::filesystem::rename(temporary, destination, renameError);
    if (renameError) { cleanup(); return {false, renameError.message()}; }
#endif
    ownsTemporary = false;
    return {true, {}};
  } catch (const std::exception& e) {
    cleanup();
    return {false, e.what()};
  }
}

inline ProjectSaveResult saveProjectFile(const StudioProject& project,
                                         const std::filesystem::path& destination) {
  try {
    const auto json = projectToJson(project);
    const auto check = projectFromJson(json);
    if (!check) return {false, "Cannot save invalid project: " + check.error};
    return writeTextFileAtomically(json.dump(2) + "\n", destination);
  } catch (const std::exception& e) { return {false, e.what()}; }
}

} // namespace ImGuiStudio
