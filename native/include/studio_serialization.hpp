#pragma once

#include "studio_types.hpp"
#include "studio_window_appearance.hpp"

#include <array>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace ImGuiStudio {

using Json = nlohmann::json;

namespace ProjectJson {

inline constexpr size_t kMaxProjectBytes = 32 * 1024 * 1024;
inline constexpr size_t kMaxNodes = 100000;
inline constexpr size_t kMaxDepth = 128;

inline void require(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

template <class T>
inline T read(const Json& obj, const char* key, T fallback) {
  if (!obj.contains(key)) return fallback;
  const auto& value = obj.at(key);
  if constexpr (std::is_same_v<T, std::string>) {
    require(value.is_string(), std::string(key) + ": expected a string");
  } else if constexpr (std::is_same_v<T, bool>) {
    require(value.is_boolean(), std::string(key) + ": expected a boolean");
  } else if constexpr (std::is_arithmetic_v<T>) {
    require(value.is_number(), std::string(key) + ": expected a number");
    const double n = value.get<double>();
    require(std::isfinite(n) && n >= std::numeric_limits<T>::lowest() &&
                n <= std::numeric_limits<T>::max(), std::string(key) + ": number out of range");
    if constexpr (std::is_integral_v<T>) {
      require(std::trunc(n) == n, std::string(key) + ": expected an integer");
    }
  } else {
    require(value.is_array(), std::string(key) + ": expected an array");
  }
  return value.get<T>();
}

template <class T>
inline void update(Json& out, const char* key, const T& value, const T& baseline, bool fresh) {
  if (fresh || value != baseline) out[key] = value;
}

inline std::vector<std::string> strings(const Json& obj, const char* key,
                                        const std::vector<std::string>& fallback) {
  auto result = read(obj, key, fallback);
  if (obj.contains(key)) {
    for (const auto& v : obj.at(key)) require(v.is_string(), std::string(key) + ": expected strings");
  }
  return result;
}

inline Color4 colorFromHex(const std::string& text) {
  require(text.size() == 7 || text.size() == 9, "color: expected #RRGGBB or #RRGGBBAA");
  require(text[0] == '#', "color: expected # prefix");
  auto part = [&](size_t index) {
    auto digit = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return -1;
    };
    const int a = digit(text[index]), b = digit(text[index + 1]);
    require(a >= 0 && b >= 0, "color: invalid hexadecimal digits");
    return static_cast<float>(a * 16 + b) / 255.0f;
  };
  return {part(1), part(3), part(5), text.size() == 9 ? part(7) : 1.0f};
}

inline std::array<float, 4> colorArray(const Color4& color) {
  return {color.r, color.g, color.b, color.a};
}

inline std::string colorHex(const Color4& color) {
  char buffer[10];
  auto byte = [](float v) { return static_cast<unsigned>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255)); };
  std::snprintf(buffer, sizeof(buffer), "#%02x%02x%02x%02x", byte(color.r), byte(color.g),
                byte(color.b), byte(color.a));
  return buffer;
}

inline MenuBarEdge readEdge(const Json& j) {
  const auto edge = read(j, "menuBarEdge", std::string("top"));
  if (edge == "bottom") return MenuBarEdge::Bottom;
  if (edge == "left") return MenuBarEdge::Left;
  if (edge == "right") return MenuBarEdge::Right;
  if (edge == "titlebar") return MenuBarEdge::TitleBar;
  if (edge == "free") return MenuBarEdge::Free;
  return MenuBarEdge::Top; // Future enum strings remain in source until explicitly edited.
}

inline bool integerWidget(WidgetType type) {
  return type == WidgetType::SliderInt || type == WidgetType::InputInt || type == WidgetType::DragInt;
}

inline std::optional<float> titleBarNumber(const Json& j, const char* key) {
  const auto value = j.find(key);
  if (value == j.end() || !value->is_number()) return std::nullopt;
  const auto number = value->get<double>();
  if (!std::isfinite(number) || std::abs(number) > std::numeric_limits<float>::max()) return std::nullopt;
  return static_cast<float>(number);
}

inline WidgetProps readProps(const Json& j, WidgetType type) {
  require(j.is_object(), "props: expected an object");
  WidgetProps p;
  p.source = j;
  if (isImageWidget(type)) {
    const auto asset = p.source.find("imageAsset");
    if (asset != p.source.end()) {
      p.imageAsset = std::make_shared<const Json>(std::move(*asset));
      p.source.erase(asset);
    }
  }
  p.text = read(j, "text", p.text);
  if (type == WidgetType::InputText || type == WidgetType::InputMultiline)
    p.text = read(j, "defaultString", p.text);
  p.hint = read(j, "hint", p.hint);
  p.buttonAction = read(j, "buttonAction", p.buttonAction);
  p.checked = read(j, "defaultBool", p.checked);
  if (integerWidget(type)) {
    p.intValue = read(j, "defaultNumber", p.intValue);
    p.minInt = read(j, "min", p.minInt);
    p.maxInt = read(j, "max", p.maxInt);
  } else {
    p.floatValue = read(j, "defaultNumber", p.floatValue);
    p.minFloat = read(j, "min", p.minFloat);
    p.maxFloat = read(j, "max", p.maxFloat);
  }
  p.progress = read(j, "fraction", p.progress);
  p.speed = read(j, "speed", p.speed);
  p.format = read(j, "format", p.format);
  p.selected = read(j, "selected", p.selected);
  p.overlay = read(j, "overlay", p.overlay);
  if (type == WidgetType::Chart2d) {
    if (j.contains("chart2dType") && j.at("chart2dType").is_string())
      p.chart2dType = read(j, "chart2dType", p.chart2dType);
    if (j.contains("chartLabels") && j.at("chartLabels").is_array() &&
        std::all_of(j.at("chartLabels").begin(), j.at("chartLabels").end(), [](const Json& v) { return v.is_string(); }))
      p.chartLabels = strings(j, "chartLabels", {});
    p.chartAutoMin = !j.contains("min");
    p.chartAutoMax = !j.contains("max");
    if (p.chartAutoMax) p.maxFloat = 100;
    p.plotValues = {18, 32, 27, 54, 49, 72, 66, 84};
  }
  if (type == WidgetType::Chart3d) {
    if (j.contains("chart3dType") && j.at("chart3dType").is_string())
      p.chart3dType = read(j, "chart3dType", p.chart3dType);
    const auto readRotation = [&](const char* key, float fallback) {
      const auto value = j.find(key);
      if (value == j.end() || !value->is_number()) return fallback;
      const double angle = value->get<double>();
      return std::isfinite(angle) && std::abs(angle) <= std::numeric_limits<float>::max()
        ? static_cast<float>(angle) : fallback;
    };
    p.chart3dRotX = readRotation("chart3dRotX", p.chart3dRotX);
    p.chart3dRotY = readRotation("chart3dRotY", p.chart3dRotY);
    p.plotValues = Chart3d::defaultSeries();
  }
  p.arrowDir = read(j, "arrowDir", p.arrowDir);
  p.spacing = read(j, "spacing", type == WidgetType::Repeater ? 4.0f : p.spacing);
  if ((type == WidgetType::Loader || type == WidgetType::Component) &&
      j.contains("sourceComponent") && j.at("sourceComponent").is_string())
    p.sourceComponent = read(j, "sourceComponent", p.sourceComponent);
  if (type == WidgetType::Loader && j.contains("loaderActive") && j.at("loaderActive").is_boolean())
    p.loaderActive = read(j, "loaderActive", p.loaderActive);
  if (type == WidgetType::Animation) {
    if (j.contains("animType") && j.at("animType").is_string())
      p.animType = read(j, "animType", p.animType);
    if (j.contains("animEasing") && j.at("animEasing").is_string())
      p.animEasing = read(j, "animEasing", p.animEasing);
    if (j.contains("animDuration") && j.at("animDuration").is_number())
      p.animDuration = read(j, "animDuration", p.animDuration);
    if (j.contains("animLoop") && j.at("animLoop").is_boolean())
      p.animLoop = read(j, "animLoop", p.animLoop);
    if (j.contains("animRunning") && j.at("animRunning").is_boolean())
      p.animRunning = read(j, "animRunning", p.animRunning);
  }
  if (type == WidgetType::Repeater && j.contains("repeatCount") && j.at("repeatCount").is_number()) {
    const auto count = j.at("repeatCount").get<double>();
    if (std::isfinite(count))
      p.repeatCount = static_cast<int>(std::clamp(count, static_cast<double>(std::numeric_limits<int>::min()),
                                                static_cast<double>(std::numeric_limits<int>::max())));
  }
  if (type == WidgetType::RadioButton) {
    const double initial = std::round(static_cast<double>(p.floatValue));
    p.intValue = static_cast<int>(std::clamp(initial, static_cast<double>(INT32_MIN), static_cast<double>(INT32_MAX)));
  }
  if (j.contains("defaultVec")) {
    require(j.at("defaultVec").is_array(), "defaultVec: expected an array");
    p.defaultVec.clear();
    for (const auto& v : j.at("defaultVec"))
      p.defaultVec.push_back(read(Json{{"value", v}}, "value", 0.0f));
  }
  p.items = strings(j, "items", p.items);
  p.selectedIndex = read(j, "selectedIndex", p.selectedIndex);
  p.radioValue = read(j, "radioValue", p.radioValue);
  p.border = read(j, "border", p.border);
  p.defaultOpen = read(j, "defaultOpen", p.defaultOpen);
  p.menuBarEdge = readEdge(j);
  if (j.contains("color")) p.color = colorFromHex(read(j, "color", std::string()));
  if (j.contains("defaultColor")) {
    const auto& a = j.at("defaultColor");
    require(a.is_array() && a.size() == 4, "defaultColor: expected four channels");
    for (const auto& v : a) require(v.is_number() && v.get<double>() >= 0 && v.get<double>() <= 1,
                                   "defaultColor: channels must be between 0 and 1");
    p.color = {a[0].get<float>(), a[1].get<float>(), a[2].get<float>(), a[3].get<float>()};
  }
  if (j.contains("columns")) {
    require(j.at("columns").is_array(), "columns: expected column labels");
    p.columnLabels = strings(j, "columns", {});
    p.columns = static_cast<int>(j.at("columns").size());
  } else {
    p.columns = read(j, "gridColumns", type == WidgetType::GridView ? 3 : p.columns);
  }
  if (j.contains("plotValues")) {
    require(j.at("plotValues").is_array(), "plotValues: expected an array");
    p.plotValues.clear();
    for (const auto& v : j.at("plotValues")) {
      Json item = {{"value", v}};
      p.plotValues.push_back(read(item, "value", 0.0f));
    }
  }
  const auto flags = strings(j, "windowFlags", {});
  auto hasFlag = [&](const char* name) { return std::find(flags.begin(), flags.end(), name) != flags.end(); };
  p.noTitleBar = hasFlag("ImGuiWindowFlags_NoTitleBar");
  p.noResize = hasFlag("ImGuiWindowFlags_NoResize");
  p.noMove = hasFlag("ImGuiWindowFlags_NoMove");
  p.noCollapse = hasFlag("ImGuiWindowFlags_NoCollapse");
  p.noBackground = hasFlag("ImGuiWindowFlags_NoBackground");
  const auto readString = [&](const char* key, std::string& field) {
    const auto value = j.find(key);
    if (value != j.end() && value->is_string()) field = value->get<std::string>();
  };
  if (type == WidgetType::Window) {
    const auto readBool = [&](const char* key, bool& field) {
      const auto value = j.find(key);
      if (value != j.end() && value->is_boolean()) field = value->get<bool>();
    };
    readBool("closable", p.closable);
    readBool("showCollapseArrow", p.showCollapseArrow);
    readBool("showMinimizeButton", p.showMinimizeButton);
    readBool("showMaximizeButton", p.showMaximizeButton);
    readBool("showTitleText", p.showTitleText);
    readString("windowControlsSide", p.windowControlsSide);
    readString("windowControlsVariant", p.windowControlsVariant);
    readString("titleAlign", p.titleAlign);
    auto& appearance = p.windowAppearance;
    readBool("titleBarSeparator", appearance.titleSeparator);
    readBool("titleBarTransparent", appearance.transparentTitle);
    readBool("showResizeGrip", appearance.showResizeGrip);
    readString("windowChromeStyle", appearance.preset);
    readString("windowShadow", appearance.shadow);
    readString("windowTitleIcon", appearance.titleIcon);
    for (const auto& field : WindowChrome::numbers)
      appearance.*field.member = titleBarNumber(j, field.key);
    for (const auto& field : WindowChrome::colors) {
      const auto value = j.find(field.key);
      if (value == j.end() || !value->is_string()) continue;
      try { appearance.*field.member = colorFromHex(value->get_ref<const std::string&>()); }
      catch (const std::runtime_error&) { /* Retain unrecognized colors in source. */ }
    }
  }
  if (j.contains("inTitleBar") && j.at("inTitleBar").is_boolean())
    p.inTitleBar = j.at("inTitleBar").get<bool>();
  if (isFreeLayoutUtility(type) && j.contains("utilityCustomSize") && j.at("utilityCustomSize").is_boolean())
    p.utilityCustomSize = j.at("utilityCustomSize").get<bool>();
  if (type == WidgetType::TitleBarButton || p.inTitleBar || j.contains("titleBarSide") ||
      j.contains("titleBarX") || j.contains("titleBarY")) {
    readString("titleBarSide", p.titleBarSide);
    p.titleBarX = titleBarNumber(j, "titleBarX").value_or(p.titleBarX);
    p.titleBarY = titleBarNumber(j, "titleBarY");
  }
  if (type == WidgetType::TitleBarButton) {
    readString("titleBarBtnVariant", p.titleBarBtnVariant);
    readString("titleBarIcon", p.titleBarIcon);
    p.titleBarBtnRounding = titleBarNumber(j, "titleBarBtnRounding");
  }
  return p;
}

inline Json propsJson(const WidgetProps& p, WidgetType type) {
  const bool fresh = p.source.is_null();
  Json out = fresh ? Json::object() : p.source;
  const WidgetProps base = fresh ? WidgetProps{} : readProps(p.source, type);
  const auto updateOptional = [&](const char* key, std::optional<float> value, std::optional<float> original) {
    if (value == original) return;
    if (value) out[key] = *value;
    else out.erase(key);
  };
  if (isImageWidget(type) && p.imageAsset) out["imageAsset"] = *p.imageAsset;
  if (type == WidgetType::Window) {
    update(out, "closable", p.closable, base.closable, false);
    update(out, "showCollapseArrow", p.showCollapseArrow, base.showCollapseArrow, false);
    update(out, "showMinimizeButton", p.showMinimizeButton, base.showMinimizeButton, false);
    update(out, "showMaximizeButton", p.showMaximizeButton, base.showMaximizeButton, false);
    update(out, "showTitleText", p.showTitleText, base.showTitleText, false);
    update(out, "windowControlsSide", p.windowControlsSide, base.windowControlsSide, false);
    update(out, "windowControlsVariant", p.windowControlsVariant, base.windowControlsVariant, false);
    update(out, "titleAlign", p.titleAlign, base.titleAlign, false);
    const auto& a = p.windowAppearance;
    const auto& original = base.windowAppearance;
    update(out, "windowChromeStyle", a.preset, original.preset, false);
    update(out, "windowShadow", a.shadow, original.shadow, false);
    update(out, "windowTitleIcon", a.titleIcon, original.titleIcon, false);
    update(out, "titleBarSeparator", a.titleSeparator, original.titleSeparator, false);
    update(out, "titleBarTransparent", a.transparentTitle, original.transparentTitle, false);
    update(out, "showResizeGrip", a.showResizeGrip, original.showResizeGrip, false);
    for (const auto& field : WindowChrome::numbers)
      updateOptional(field.key, a.*field.member, original.*field.member);
    for (const auto& field : WindowChrome::colors) {
      const auto& value = a.*field.member;
      const auto& old = original.*field.member;
      if (value && (!old || colorArray(*value) != colorArray(*old))) out[field.key] = colorHex(*value);
      if (!value && old) out.erase(field.key);
    }
  }
  update(out, "inTitleBar", p.inTitleBar, base.inTitleBar, false);
  if (isFreeLayoutUtility(type)) update(out, "utilityCustomSize", p.utilityCustomSize, base.utilityCustomSize, false);
  update(out, "titleBarSide", p.titleBarSide, base.titleBarSide, fresh && type == WidgetType::TitleBarButton);
  update(out, "titleBarX", p.titleBarX, base.titleBarX, false);
  updateOptional("titleBarY", p.titleBarY, base.titleBarY);
  if (type == WidgetType::TitleBarButton) {
    update(out, "titleBarBtnVariant", p.titleBarBtnVariant, base.titleBarBtnVariant, fresh);
    update(out, "titleBarIcon", p.titleBarIcon, base.titleBarIcon, fresh);
    updateOptional("titleBarBtnRounding", p.titleBarBtnRounding, base.titleBarBtnRounding);
  }
  const char* textKey = type == WidgetType::InputText || type == WidgetType::InputMultiline ? "defaultString" : "text";
  update(out, textKey, p.text, base.text, fresh);
  update(out, "hint", p.hint, base.hint, fresh);
  if (!p.buttonAction.empty() || out.contains("buttonAction"))
    update(out, "buttonAction", p.buttonAction, base.buttonAction, fresh);
  update(out, "defaultBool", p.checked, base.checked, fresh);
  if (integerWidget(type)) {
    update(out, "defaultNumber", p.intValue, base.intValue, fresh);
    update(out, "min", p.minInt, base.minInt, fresh);
    update(out, "max", p.maxInt, base.maxInt, fresh);
  } else {
    update(out, "defaultNumber", p.floatValue, base.floatValue, fresh);
    if (type == WidgetType::Chart2d) {
      if (p.chartAutoMin) out.erase("min");
      else update(out, "min", p.minFloat, base.minFloat, fresh || base.chartAutoMin);
      if (p.chartAutoMax) out.erase("max");
      else update(out, "max", p.maxFloat, base.maxFloat, fresh || base.chartAutoMax);
      update(out, "chart2dType", p.chart2dType, base.chart2dType, fresh);
      update(out, "chartLabels", p.chartLabels, base.chartLabels, fresh && !p.chartLabels.empty());
    } else {
      update(out, "min", p.minFloat, base.minFloat, fresh);
      update(out, "max", p.maxFloat, base.maxFloat, fresh);
    }
  }
  update(out, "fraction", p.progress, base.progress, fresh);
  update(out, "speed", p.speed, base.speed, fresh);
  update(out, "format", p.format, base.format, fresh && !p.format.empty());
  update(out, "selected", p.selected, base.selected, fresh);
  update(out, "overlay", p.overlay, base.overlay, fresh && !p.overlay.empty());
  update(out, "arrowDir", p.arrowDir, base.arrowDir, fresh && type == WidgetType::ArrowButton);
  update(out, "spacing", p.spacing, base.spacing, fresh);
  if (type == WidgetType::Loader || type == WidgetType::Component)
    update(out, "sourceComponent", p.sourceComponent, base.sourceComponent, fresh);
  if (type == WidgetType::Loader) update(out, "loaderActive", p.loaderActive, base.loaderActive, fresh);
  if (type == WidgetType::Repeater) update(out, "repeatCount", p.repeatCount, base.repeatCount, fresh);
  const bool vectorWidget = type == WidgetType::SliderFloat2 || type == WidgetType::SliderFloat3 || type == WidgetType::DragFloat3;
  update(out, "defaultVec", p.defaultVec, base.defaultVec, fresh && vectorWidget);
  update(out, "items", p.items, base.items, fresh);
  update(out, "selectedIndex", p.selectedIndex, base.selectedIndex, fresh);
  update(out, "radioValue", p.radioValue, base.radioValue, fresh);
  update(out, "border", p.border, base.border, fresh);
  update(out, "defaultOpen", p.defaultOpen, base.defaultOpen, fresh);
  if (fresh || p.menuBarEdge != base.menuBarEdge) out["menuBarEdge"] = menuBarEdgeName(p.menuBarEdge);
  if (fresh || colorArray(p.color) != colorArray(base.color)) {
    if (type == WidgetType::ColorEdit4 || type == WidgetType::ColorEdit3 || type == WidgetType::ColorButton || out.contains("defaultColor"))
      out["defaultColor"] = colorArray(p.color);
    else out["color"] = colorHex(p.color);
  }
  if (fresh || p.columns != base.columns || p.columnLabels != base.columnLabels) {
    if (type == WidgetType::Table) {
      auto labels = p.columnLabels;
      labels.resize(static_cast<size_t>(std::max(1, p.columns)), "Column");
      out["columns"] = labels;
    } else out["gridColumns"] = p.columns;
  }
  update(out, "plotValues", p.plotValues, base.plotValues, fresh);
  if (type == WidgetType::Animation) {
    update(out, "animType", p.animType, base.animType, fresh);
    update(out, "animEasing", p.animEasing, base.animEasing, fresh);
    update(out, "animDuration", p.animDuration, base.animDuration, fresh);
    update(out, "animLoop", p.animLoop, base.animLoop, fresh);
    update(out, "animRunning", p.animRunning, base.animRunning, fresh);
  }
  if (type == WidgetType::Chart3d) {
    update(out, "chart3dType", p.chart3dType, base.chart3dType, fresh);
    update(out, "chart3dRotX", p.chart3dRotX, base.chart3dRotX, fresh);
    update(out, "chart3dRotY", p.chart3dRotY, base.chart3dRotY, fresh);
  }
  if (fresh || p.noTitleBar != base.noTitleBar || p.noResize != base.noResize || p.noMove != base.noMove ||
      p.noCollapse != base.noCollapse || p.noBackground != base.noBackground) {
    auto flags = strings(out, "windowFlags", {});
    auto set = [&](const char* name, bool enabled) {
      flags.erase(std::remove(flags.begin(), flags.end(), name), flags.end());
      if (enabled) flags.emplace_back(name);
    };
    set("ImGuiWindowFlags_NoTitleBar", p.noTitleBar);
    set("ImGuiWindowFlags_NoResize", p.noResize);
    set("ImGuiWindowFlags_NoMove", p.noMove);
    if (p.noCollapse != base.noCollapse || (fresh && p.noCollapse)) set("ImGuiWindowFlags_NoCollapse", p.noCollapse);
    if (p.noBackground != base.noBackground || (fresh && p.noBackground)) set("ImGuiWindowFlags_NoBackground", p.noBackground);
    out["windowFlags"] = flags;
  }
  return out;
}

inline ImGuiStyleConfig readStyle(const Json& j) {
  require(j.is_object(), "style: expected an object");
  ImGuiStyleConfig s;
  s.source = j;
  const auto preset = read(j, "preset", std::string("dark"));
  if (preset == "light") s.preset = StylePresetId::ImGuiLight;
  else if (preset == "classic") s.preset = StylePresetId::ImGuiClassic;
  else if (preset == "slate_pro") s.preset = StylePresetId::CyberCyan;
  else if (preset == "nordic") s.preset = StylePresetId::NordFrost;
#define READ_STYLE(field) s.field = read(j, #field, s.field)
  READ_STYLE(windowRounding); READ_STYLE(frameRounding);
  READ_STYLE(windowPaddingX); READ_STYLE(windowPaddingY);
  READ_STYLE(itemSpacingX); READ_STYLE(itemSpacingY);
#undef READ_STYLE
  if (j.contains("colors")) {
    const auto& colors = j.at("colors");
    require(colors.is_object(), "style.colors: expected an object");
    auto get = [&](const char* key, Color4& color) {
      if (colors.contains(key)) color = colorFromHex(read(colors, key, std::string()));
    };
    get("windowBg", s.windowBg); get("titleBgActive", s.titleBg);
    get("button", s.buttonColor); get("frameBg", s.frameBg); get("text", s.textColor);
  }
  return s;
}

inline Json styleJson(const ImGuiStyleConfig& s) {
  const bool fresh = s.source.is_null();
  Json out = fresh ? Json::object() : s.source;
  const auto base = fresh ? ImGuiStyleConfig{} : readStyle(s.source);
  if (fresh || s.preset != base.preset) {
    const char* presets[] = {"dark", "classic", "light", "slate_pro", "nordic"};
    out["preset"] = presets[static_cast<int>(s.preset)];
  }
#define WRITE_STYLE(field) update(out, #field, s.field, base.field, fresh)
  WRITE_STYLE(windowRounding); WRITE_STYLE(frameRounding);
  WRITE_STYLE(windowPaddingX); WRITE_STYLE(windowPaddingY);
  WRITE_STYLE(itemSpacingX); WRITE_STYLE(itemSpacingY);
#undef WRITE_STYLE
  if (fresh) {
    out.update(Json{{"childRounding", 6}, {"grabRounding", 4}, {"tabRounding", 4},
      {"scrollbarRounding", 6}, {"framePaddingX", 4}, {"framePaddingY", 3},
      {"windowBorderSize", 1}, {"frameBorderSize", 0}, {"alpha", 1}});
    out["colors"] = {{"childBg", "#10131b"}, {"titleBg", "#1c2433"},
      {"frameBgHovered", "#26364b"}, {"buttonHovered", "#337cce"},
      {"buttonActive", "#2867ad"}, {"accent", "#38bdf8"}, {"checkMark", "#38bdf8"},
      {"sliderGrab", "#38bdf8"}, {"header", "#26364b"}, {"headerHovered", "#334966"},
      {"textDisabled", "#8892a0"}, {"border", "#334155"}, {"separator", "#334155"}};
  }
  auto put = [&](const char* key, const Color4& color, const Color4& original) {
    if (fresh || colorArray(color) != colorArray(original)) out["colors"][key] = colorHex(color);
  };
  put("windowBg", s.windowBg, base.windowBg); put("titleBgActive", s.titleBg, base.titleBg);
  put("button", s.buttonColor, base.buttonColor); put("frameBg", s.frameBg, base.frameBg);
  put("text", s.textColor, base.textColor);
  return out;
}

inline CanvasConfig readCanvas(const Json& j) {
  require(j.is_object(), "canvas: expected an object");
  CanvasConfig c;
  c.source = j;
  c.orientation = read(j, "orientation", c.orientation);
  c.landscape = c.orientation != "portrait";
  c.width = read(j, "canvasWidth", c.width);
  c.height = read(j, "canvasHeight", c.height);
  require(c.width > 0 && c.height > 0, "canvas: dimensions must be positive");
  return c;
}

inline Json canvasJson(const CanvasConfig& c) {
  const bool fresh = c.source.is_null();
  const auto base = fresh ? CanvasConfig{} : readCanvas(c.source);
  Json out = fresh ? Json::object() : c.source;
  update(out, "canvasWidth", c.width, base.width, fresh);
  update(out, "canvasHeight", c.height, base.height, fresh);
  std::string orientation = c.orientation;
  if (c.landscape != base.landscape && orientation == base.orientation)
    orientation = c.landscape ? "landscape" : "portrait";
  update(out, "orientation", orientation, base.orientation, fresh);
  return out;
}

inline ImGuiNode readNodeFields(const Json& j) {
  require(j.is_object(), "node: expected an object");
  ImGuiNode n;
  n.source = j;
  n.source.erase("props");
  if (n.source.contains("children")) n.source["children"] = Json::array();
  for (const char* key : {"id", "type", "label", "varName"})
    require(j.contains(key) && j.at(key).is_string(), std::string("node.") + key + ": expected a string");
  n.id = j.at("id").get<std::string>();
  require(!n.id.empty(), "node.id: must not be empty");
  const auto id = j.at("type").get<std::string>();
  require(!id.empty(), "node.type: must not be empty");
  n.type = widgetTypeFromId(id);
  n.label = j.at("label").get<std::string>();
  n.varName = j.at("varName").get<std::string>();
  n.imguiId = read(j, "imguiId", n.imguiId);
  n.x = read(j, "x", n.x); n.y = read(j, "y", n.y);
  n.width = read(j, "width", n.width); n.height = read(j, "height", n.height);
  n.visible = read(j, "visible", n.visible);
  n.sameLine = read(j, "sameLine", n.sameLine);
  n.useCursorPos = read(j, "useCursorPos", n.useCursorPos);
  n.sameLineSpacing = read(j, "sameLineSpacing", n.sameLineSpacing);
  n.indent = read(j, "indent", n.indent);
  return n;
}

inline ImGuiNode readNode(const Json& j, std::unordered_set<std::string>& ids,
                          std::vector<std::string>& warnings, size_t depth) {
  require(depth <= kMaxDepth, "node hierarchy exceeds depth limit");
  auto n = readNodeFields(j);
  require(ids.size() < kMaxNodes, "project exceeds node limit");
  require(ids.insert(n.id).second, "duplicate node ID: " + n.id);
  if (n.type == WidgetType::Unknown)
    warnings.push_back("Unknown widget '" + j.at("type").get<std::string>() + "' preserved (" + n.id + ")");
  require(j.contains("props"), "node " + n.id + ": missing props");
  n.props = readProps(j.at("props"), n.type);
  if (j.contains("children")) {
    require(j.at("children").is_array(), "node.children: expected an array");
    for (const auto& child : j.at("children")) {
      auto c = readNode(child, ids, warnings, depth + 1);
      if (n.type != WidgetType::Unknown && c.type != WidgetType::Unknown)
        require(canAcceptChild(n.type, c.type), "invalid hierarchy: " + n.id + " -> " + c.id);
      n.children.push_back(std::move(c));
    }
  }
  return n;
}

inline Json nodeJson(const ImGuiNode& n) {
  const bool fresh = n.source.is_null();
  const auto base = fresh ? ImGuiNode{} : readNodeFields(n.source);
  Json out = fresh ? Json::object() : n.source;
  update(out, "id", n.id, base.id, fresh);
  if (fresh || n.type != base.type) out["type"] = widgetTypeId(n.type);
  update(out, "label", n.label, base.label, fresh);
  update(out, "varName", n.varName, base.varName, fresh);
  if (!n.imguiId.empty() || out.contains("imguiId")) update(out, "imguiId", n.imguiId, base.imguiId, fresh);
#define WRITE_NODE(field) update(out, #field, n.field, base.field, fresh)
  WRITE_NODE(x); WRITE_NODE(y); WRITE_NODE(width); WRITE_NODE(height);
  WRITE_NODE(visible); WRITE_NODE(sameLine); WRITE_NODE(useCursorPos);
  WRITE_NODE(sameLineSpacing); WRITE_NODE(indent);
#undef WRITE_NODE
  out["props"] = propsJson(n.props, n.type);
  if (!n.children.empty() || n.props.source.is_null() || out.contains("children")) out["children"] = Json::array();
  for (const auto& c : n.children) out["children"].push_back(nodeJson(c));
  return out;
}

} // namespace ProjectJson

struct ProjectLoadResult {
  std::optional<StudioProject> project;
  std::vector<std::string> warnings;
  std::string error;
  explicit operator bool() const { return project.has_value(); }
};

inline Json projectToJson(const StudioProject& p) {
  const bool fresh = p.source.is_null();
  Json out = fresh ? Json::object() : p.source;
  if (fresh || p.name != out.value("name", std::string())) out["name"] = p.name;
  out["style"] = ProjectJson::styleJson(p.style);
  if (fresh || out.contains("canvas") || p.canvas.width != 1280 || p.canvas.height != 720 ||
      p.canvas.orientation != "landscape" || !p.canvas.landscape)
    out["canvas"] = ProjectJson::canvasJson(p.canvas);
  out["nodes"] = Json::array();
  for (const auto& n : p.nodes) out["nodes"].push_back(ProjectJson::nodeJson(n));
  return out;
}

inline ProjectLoadResult projectFromJson(const Json& j) {
  ProjectLoadResult result;
  try {
    using namespace ProjectJson;
    require(j.is_object(), "project: expected an object");
    require(j.contains("name") && j.at("name").is_string(), "project.name: expected a string");
    require(j.contains("nodes") && j.at("nodes").is_array(), "project.nodes: expected an array");
    require(j.contains("style"), "project.style: missing object");
    StudioProject p;
    p.source = j; p.source.erase("nodes");
    p.name = j.at("name").get<std::string>();
    p.style = readStyle(j.at("style"));
    if (j.contains("canvas")) p.canvas = readCanvas(j.at("canvas"));
    std::unordered_set<std::string> ids;
    for (const auto& node : j.at("nodes")) {
      auto n = readNode(node, ids, result.warnings, 0);
      require(n.type == WidgetType::Window || n.type == WidgetType::Unknown,
              "root nodes must be windows: " + n.id);
      p.nodes.push_back(std::move(n));
    }
    result.project = std::move(p);
  } catch (const std::exception& e) {
    result.error = e.what();
  }
  return result;
}

inline ProjectLoadResult parseProject(const std::string& text) {
  if (text.size() > ProjectJson::kMaxProjectBytes) return {std::nullopt, {}, "Project exceeds 32 MiB limit"};
  try {
    // Bound JSON nesting before allocating a recursively nested DOM.
    const auto j = Json::parse(text, [](int depth, Json::parse_event_t, Json&) {
      ProjectJson::require(depth <= static_cast<int>(ProjectJson::kMaxDepth * 3 + 16),
                           "JSON exceeds nesting limit");
      return true;
    });
    return projectFromJson(j);
  } catch (const std::exception& e) {
    return {std::nullopt, {}, e.what()};
  }
}

} // namespace ImGuiStudio
