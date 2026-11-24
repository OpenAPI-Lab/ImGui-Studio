#pragma once
#include "studio_codegen.hpp"

namespace ImGuiStudio::ManagedTitleBar {
inline std::string caption(const std::string& label) {
  const auto text = label.substr(0, label.find('\0'));
  return text.substr(0, text.find("##"));
}

struct Button {
  int side, variant, icon;
  float rounding;
  std::optional<Color4> background, foreground, hovered, active;
};

template<class Warn>
Button describe(const ImGuiNode& node, Warn warn) {
  const auto& p = node.props;
  const auto index = [](const std::string& value, std::initializer_list<std::string_view> choices, int fallback = 0) {
    const auto choice = std::find(choices.begin(), choices.end(), value);
    return choice == choices.end() ? fallback : static_cast<int>(choice - choices.begin());
  };
  const auto readColor = [&](const char* field) -> std::optional<Color4> {
    if (!p.source.is_object() || !p.source.contains(field) || !p.source.at(field).is_string()) return std::nullopt;
    try { return ProjectJson::colorFromHex(p.source.at(field).get<std::string>()); }
    catch (const std::exception&) { warn(std::string("Color incompatible: ") + field); return std::nullopt; }
  };
  const int variant = index(p.titleBarBtnVariant, {"icon","pill","outline","solid"}, 1);
  if (!p.buttonAction.empty()) warn("La accion de boton requiere integracion de la aplicacion");
  return {index(p.titleBarSide, {"left","center","right","free"}), variant,
    index(p.titleBarIcon, {"none","panel_left","panel_right","sparkles","edit","plus","settings","search","terminal","bot","pin","bell"}),
    p.titleBarBtnRounding.value_or(variant == 1 ? 999.0f : 6.0f),
    readColor("buttonColorOverride"), readColor("textColorOverride"),
    readColor("buttonHoveredColorOverride"), readColor("buttonActiveColorOverride")};
}

inline bool visibleItem(const ImGuiNode& child) {
  return child.visible && WindowChrome::isTitleItem(child);
}

struct WindowTitle {
  float left, right, alignment;
  int side, variant, icon, shadow;
  bool visible, custom, inset, collapse, modes;
};

inline WindowTitle describeWindow(const ImGuiNode& window) {
  using CppCodegen::titleOption;
  const auto& p = window.props;
  const auto& a = p.windowAppearance;
  const bool visible = !CppCodegen::hidesTitle(window);
  constexpr int icons[] = {0,9,12,8,3,13,14};
  return {
    std::clamp(a.titleOffsetLeft.value_or(0), 0.0f, 4096.0f),
    std::clamp(a.titleOffsetRight.value_or(0), 0.0f, 4096.0f),
    titleOption(p.titleAlign, {"left","center","right"}) * 0.5f,
    titleOption(p.windowControlsSide, {"left","center","right"}, 2),
    titleOption(p.windowControlsVariant, {"icons","traffic_lights","pills","classic"}),
    icons[titleOption(a.titleIcon, {"none","bot","app","terminal","sparkles","folder","layers"})],
    titleOption(a.shadow, {"none","soft","deep","glow"}),
    visible, visible && WindowChrome::usesCustomTitle(window), visible && WindowChrome::hasTitleInsets(a),
    p.showCollapseArrow && !p.noCollapse, visible && CppCodegen::hasWindowModes(p)
  };
}

template<class Emit>
void eachWindowControl(const WidgetProps& props, const WindowTitle& title, Emit emit) {
  if (title.variant == 1 && props.closable) emit("close", 1, title.side);
  if (props.showMinimizeButton) emit("minimize", 2, title.side);
  if (props.showMaximizeButton) emit("maximize", 3, title.side);
  if (title.variant != 1 && props.closable) emit("close", 1, title.side);
}
} // namespace ImGuiStudio::ManagedTitleBar
