#pragma once
#include "studio_types.hpp"

namespace ImGuiStudio::WindowChrome {

struct NumericProperty {
  const char* key;
  std::optional<float> WindowAppearance::* member;
};

inline constexpr NumericProperty numbers[] = {
  {"titleBarHeight", &WindowAppearance::titleHeight},
  {"titleBarOffsetLeft", &WindowAppearance::titleOffsetLeft},
  {"titleBarOffsetRight", &WindowAppearance::titleOffsetRight},
  {"windowRoundingOverride", &WindowAppearance::rounding},
  {"windowBorderSizeOverride", &WindowAppearance::borderSize},
  {"windowPaddingXOverride", &WindowAppearance::paddingX},
  {"windowPaddingYOverride", &WindowAppearance::paddingY},
  {"windowAlphaOverride", &WindowAppearance::alpha},
};

struct ColorProperty {
  const char* key;
  std::optional<Color4> WindowAppearance::* member;
};

inline constexpr ColorProperty colors[] = {
  {"windowBgOverride", &WindowAppearance::background},
  {"titleBgOverride", &WindowAppearance::titleBackground},
  {"windowBorderColorOverride", &WindowAppearance::borderColor},
  {"textColorOverride", &WindowAppearance::textColor},
  {"titleBarSeparatorColor", &WindowAppearance::separatorColor},
};

struct Preset {
  const char* id;
  const char* name;
  const char* variant;
  const char* side;
  bool minimize, maximize;
  float titleHeight;
  bool separator;
  const char* shadow;
  const char* icon;
  const char* alignment;
  const char* background;
  const char* titleBackground;
  const char* border;
  const char* text;
  const char* separatorColor;
  float rounding, borderSize;
};

// Values from WINDOW_CHROME_PRESETS in src/data/widgetCatalog.ts.
inline constexpr Preset presets[] = {
  {"imgui", "ImGui", "classic", "right", false, false, 28, false, "deep", "none", "left",
   nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0},
  {"bionic", "Bionic", "icons", "right", true, true, 40, true, "deep", "bot", "left",
   "#111318", "#14171f", "#252a36", "#eef2f7", "#1f2430", 10, 1},
  {"macos", "macOS", "traffic_lights", "left", true, true, 36, true, "deep", "none", "center",
   "#181b22", "#1f232c", "#313847", "#f1f5f9", "#2a303c", 12, 1},
  {"minimal", "Minimal", "icons", "right", true, false, 34, false, "soft", "none", "left",
   "#151820", "#151820", "#232834", "#e2e8f0", nullptr, 8, 0},
  {"cyber", "Cyber", "pills", "right", true, true, 34, true, "glow", "terminal", "left",
   "#0b101b", "#0f172a", "#0ea5e9", "#e0f2fe", "#0284c7", 2, 1},
};

inline bool hasTitleInsets(const WindowAppearance& appearance) {
  return appearance.titleOffsetLeft.value_or(0) > 0 || appearance.titleOffsetRight.value_or(0) > 0;
}

inline bool supportsTitleWidget(WidgetType type) {
  switch (type) {
    case WidgetType::Text: case WidgetType::TextColored:
    case WidgetType::Button: case WidgetType::SmallButton: case WidgetType::ArrowButton:
    case WidgetType::Checkbox: case WidgetType::InputText: return true;
    default: return false;
  }
}

inline bool isTitleMenu(const ImGuiNode& node) {
  return node.type == WidgetType::MenuBar && node.props.menuBarEdge == MenuBarEdge::TitleBar;
}

inline bool isTitleItem(const ImGuiNode& node) {
  return node.type == WidgetType::TitleBarButton || isTitleMenu(node) ||
    (node.props.inTitleBar && supportsTitleWidget(node.type));
}

inline bool usesCustomTitle(const ImGuiNode& window) {
  const auto& p = window.props;
  const auto& a = p.windowAppearance;
  return p.closable || p.showMinimizeButton || p.showMaximizeButton || !p.showTitleText ||
    p.titleAlign != "left" || a.titleSeparator || a.titleIcon != "none" || hasTitleInsets(a) ||
    std::any_of(window.children.begin(), window.children.end(),
      [](const ImGuiNode& node) { return node.visible && isTitleItem(node); });
}

} // namespace ImGuiStudio::WindowChrome
