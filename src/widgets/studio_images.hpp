#pragma once

#include "imguistudio/project/studio_types.hpp"
#include "imgui.h"

#include <filesystem>

namespace ImGuiStudio {

struct ImageImport {
  std::shared_ptr<const nlohmann::json> asset;
  std::string error;
};
ImageImport importImage(const std::filesystem::path& path);

class NativeImageCache {
 public:
  struct Texture {
    ImTextureID id = 0;
    int width = 0, height = 0;
    std::string error;
  };

  NativeImageCache() = default;
  NativeImageCache(const NativeImageCache&) = delete;
  NativeImageCache& operator=(const NativeImageCache&) = delete;
  ~NativeImageCache();
  void beginFrame();
  Texture texture(const std::shared_ptr<const nlohmann::json>& asset);

 private:
  struct Entry {
    Texture texture;
    int lastFrame;
  };
  using Entries = std::map<std::shared_ptr<const nlohmann::json>, Entry>;
  Entries entries_;
  size_t bytes_ = 0;
  void erase(Entries::iterator entry);
};

} // namespace ImGuiStudio
