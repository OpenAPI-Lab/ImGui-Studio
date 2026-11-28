#pragma once

#include "imgui.h"
#include "imguistudio/project/studio_project_io.hpp"

#include <array>

namespace ImGuiStudio {

struct EditorPalette {
  const char* id;
  const char* name;
  unsigned workspace, panel, surface, border, accent;
  bool light = false;
};

inline constexpr std::array<EditorPalette, 6> editorPalettes{{
  {"midnight", "Midnight", 0x10151e, 0x171f2c, 0x243247, 0x394961, 0x78aef4},
  {"graphite", "Graphite", 0x16181c, 0x202329, 0x30353f, 0x444b58, 0xe6ad68},
  {"ocean", "Ocean", 0x101d27, 0x172a37, 0x243e4f, 0x38586b, 0x66d8e9},
  {"forest", "Forest", 0x131d18, 0x1c2a22, 0x2c4033, 0x415a48, 0x91d7a3},
  {"plum", "Plum", 0x1c1625, 0x281f35, 0x392c4a, 0x564167, 0xcda4f5},
  {"porcelain", "Porcelain", 0xe5eaf1, 0xf7f9fc, 0xe3e9f1, 0xb5c3d7, 0x265fab, true},
}};

struct EditorAppearance {
  size_t palette = 0;
  int rounding = 10;
};

inline EditorAppearance loadEditorAppearance(const std::filesystem::path& state) {
  EditorAppearance appearance;
  const auto file = readTextFile(state / "appearance.json", 4096);
  if (!file) return appearance;
  const auto parsed = Json::parse(file.text, nullptr, false);
  if (!parsed.is_object()) return appearance;
  if (const auto name = parsed.find("palette"); name != parsed.end() && name->is_string()) {
    for (size_t i = 0; i < editorPalettes.size(); ++i)
      if (*name == editorPalettes[i].id) appearance.palette = i;
  }
  if (const auto radius = parsed.find("rounding"); radius != parsed.end() && radius->is_number_integer()) {
    const auto number = radius->get<double>();
    if (number >= 0 && number <= 18) appearance.rounding = static_cast<int>(number);
  }
  return appearance;
}

inline ProjectSaveResult saveEditorAppearance(const std::filesystem::path& state, const EditorAppearance& appearance) {
  const Json contents{{"palette", editorPalettes[appearance.palette].id}, {"rounding", appearance.rounding}};
  return writeTextFileAtomically(contents.dump(2) + "\n", state / "appearance.json", 4096);
}

inline ImVec4 editorColor(unsigned rgb) {
  return ImVec4(((rgb >> 16) & 255) / 255.0f, ((rgb >> 8) & 255) / 255.0f,
                (rgb & 255) / 255.0f, 1.0f);
}

inline void applyEditorAppearance(const EditorAppearance& appearance) {
  const auto& palette = editorPalettes[appearance.palette];
  auto& style = ImGui::GetStyle();
  if (palette.light) ImGui::StyleColorsLight(&style);
  else ImGui::StyleColorsDark(&style);

  const float radius = static_cast<float>(appearance.rounding);
  style.WindowRounding = radius;
  style.PopupRounding = radius;
  style.ChildRounding = radius * 0.8f;
  style.FrameRounding = radius * 0.55f;
  style.TabRounding = radius * 0.55f;
  style.GrabRounding = radius * 0.55f;
  style.ScrollbarRounding = radius * 0.65f;
  style.WindowBorderSize = 1.0f;

  const auto panel = editorColor(palette.panel);
  const auto surface = editorColor(palette.surface);
  const auto accent = editorColor(palette.accent);
  const auto mix = [&](float amount) {
    return ImVec4(surface.x + (accent.x - surface.x) * amount,
      surface.y + (accent.y - surface.y) * amount, surface.z + (accent.z - surface.z) * amount, 1.0f);
  };
  auto* colors = style.Colors;
  colors[ImGuiCol_WindowBg] = colors[ImGuiCol_PopupBg] = panel;
  colors[ImGuiCol_MenuBarBg] = surface;
  colors[ImGuiCol_Border] = editorColor(palette.border);
  colors[ImGuiCol_TitleBg] = colors[ImGuiCol_TitleBgCollapsed] = surface;
  colors[ImGuiCol_TitleBgActive] = mix(0.18f);
  colors[ImGuiCol_FrameBg] = colors[ImGuiCol_Button] = colors[ImGuiCol_Header] = surface;
  colors[ImGuiCol_FrameBgHovered] = colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_HeaderHovered] = mix(0.24f);
  colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_HeaderActive] = mix(0.38f);
  colors[ImGuiCol_Tab] = colors[ImGuiCol_TabDimmed] = surface;
  colors[ImGuiCol_TabHovered] = colors[ImGuiCol_TabSelected] = colors[ImGuiCol_TabDimmedSelected] = mix(0.24f);
  colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_CheckMark] = colors[ImGuiCol_SliderGrab] = accent;
  colors[ImGuiCol_ScrollbarBg] = panel;
  colors[ImGuiCol_ScrollbarGrab] = mix(0.17f);
  colors[ImGuiCol_ScrollbarGrabHovered] = mix(0.30f);
  colors[ImGuiCol_ScrollbarGrabActive] = mix(0.42f);
}

} // namespace ImGuiStudio
