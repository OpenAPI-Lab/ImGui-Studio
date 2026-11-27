#pragma once

#include "imgui.h"
#include "imguistudio/project/studio_session_store.hpp"

#include <cstring>

namespace ImGuiStudio {

inline ImFont* loadStudioFont(ImFontAtlas& atlas, const std::vector<std::filesystem::path>& candidates,
                              const ImWchar* ranges, bool merge) {
  for (const auto& file : candidates) {
    if (file.empty()) continue;
    const auto bytes = readTextFile(file, 64 * 1024 * 1024);
    if (!bytes || bytes.text.size() < 12) continue;
    const auto* header = bytes.text.data();
    if (std::memcmp(header, "ttcf", 4) && std::memcmp(header, "OTTO", 4) &&
        std::memcmp(header, "\0\1\0\0", 4)) continue;
    ImFontConfig config;
    config.MergeMode = merge;
    config.OversampleH = config.OversampleV = 1;
    config.FontDataOwnedByAtlas = false;
    return atlas.AddFontFromMemoryTTF(const_cast<char*>(bytes.text.data()), static_cast<int>(bytes.text.size()),
      16.0f, &config, ranges);
  }
  return nullptr;
}

inline ImFont* loadStudioFonts(ImFontAtlas& atlas) {
  using Path = std::filesystem::path;
  Path custom, windows;
  try { custom = defaultStudioStateDirectory() / "fonts"; }
  catch (const std::runtime_error&) { /* System fonts remain available. */ }
#if defined(_WIN32)
  if (const auto* directory = _wgetenv(L"WINDIR")) windows = Path(directory) / "Fonts";
#elif defined(__linux__)
  if (std::getenv("WSL_DISTRO_NAME") || std::getenv("WSL_INTEROP")) windows = "/mnt/c/Windows/Fonts";
#endif
  const auto optional = [](const Path& root, const char* file) { return root.empty() ? Path{} : root / file; };
  constexpr static ImWchar latin[] = {0x0020, 0x024F, 0x0370, 0x052F, 0x2000, 0x206F, 0x20A0, 0x20CF, 0};
  constexpr static ImWchar hangul[] = {0x1100, 0x11FF, 0x3130, 0x318F, 0xAC00, 0xD7A3, 0};
  constexpr static ImWchar kana[] = {0x3040, 0x30FF, 0x31F0, 0x31FF, 0xFF61, 0xFF9F, 0};
  atlas.TexDesiredWidth = 4096;
  atlas.Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
  auto* base = loadStudioFont(atlas, {optional(custom, "latin.ttf"), optional(windows, "segoeui.ttf"),
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/System/Library/Fonts/Supplemental/Arial.ttf"}, latin, false);
  if (!base) base = atlas.AddFontDefault();
  loadStudioFont(atlas, {optional(custom, "korean.ttf"), optional(windows, "malgun.ttf"),
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", "/System/Library/Fonts/AppleSDGothicNeo.ttc"}, hangul, true);
  const std::vector<Path> japanese{optional(custom, "japanese.ttf"), optional(windows, "YuGothR.ttc"),
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", "/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc"};
  loadStudioFont(atlas, japanese, kana, true);
  loadStudioFont(atlas, {optional(custom, "chinese.ttf"), optional(windows, "msyh.ttc"),
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", "/System/Library/Fonts/PingFang.ttc"},
    atlas.GetGlyphRangesChineseFull(), true);
  loadStudioFont(atlas, japanese, atlas.GetGlyphRangesJapanese(), true);
  if (atlas.Build()) return base;
  atlas.Clear();
  base = atlas.AddFontDefault();
  atlas.Build();
  return base;
}

}  // namespace ImGuiStudio
