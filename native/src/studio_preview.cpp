#include "studio_preview.hpp"
#include "studio_locale.hpp"
#include "studio_knob.hpp"
#include "studio_status_widgets.hpp"
#include "imgui-knobs.h"
#include "studio_chart2d.hpp"
#include "studio_chart3d.hpp"
#include "studio_document_edit.hpp"
#include "misc/cpp/imgui_stdlib.h"
#include "imgui_internal.h"

#include <cfloat>
#include <cctype>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace ImGuiStudio {
namespace {

bool vectorControl(WidgetType type) {
  return type == WidgetType::SliderFloat2 || type == WidgetType::SliderFloat3 || type == WidgetType::DragFloat3;
}
bool floatControl(WidgetType type) {
  return type == WidgetType::InputFloat || type == WidgetType::SliderFloat || type == WidgetType::SliderAngle ||
    type == WidgetType::DragFloat || type == WidgetType::Knob || vectorControl(type);
}
bool itemList(WidgetType type) { return type == WidgetType::Combo || type == WidgetType::Listbox; }
void drawModelLabel(const std::string& text, bool centered) {
  const auto minimum = ImGui::GetItemRectMin(), maximum = ImGui::GetItemRectMax();
  const auto padding = ImGui::GetStyle().FramePadding;
  const auto extent = ImGui::CalcTextSize(text.c_str());
  const float x = centered ? std::max(padding.x, (maximum.x - minimum.x - extent.x) * 0.5f) : padding.x;
  const float y = std::max(0.0f, (maximum.y - minimum.y - extent.y) * 0.5f);
  auto* draw = ImGui::GetWindowDrawList();
  draw->PushClipRect(ImVec2(minimum.x + padding.x, minimum.y), ImVec2(maximum.x - padding.x, maximum.y), true);
  draw->AddText(ImVec2(minimum.x + x, minimum.y + y), ImGui::GetColorU32(ImGuiCol_Text), text.c_str());
  draw->PopClipRect();
}
bool plotWidget(WidgetType type) {
  return type == WidgetType::PlotLines || type == WidgetType::PlotHistogram ||
    type == WidgetType::Chart2d || type == WidgetType::Chart3d;
}
bool finiteInput(const char* label, float& value) {
  float edited = value;
  if (ImGui::InputFloat(label, &edited) && std::isfinite(edited)) { value = edited; return true; }
  return false;
}

constexpr const char* kTitleSides[] = {"left", "center", "right", "free"};
constexpr const char* kTitleVariants[] = {"icon", "pill", "outline", "solid"};
constexpr const char* kTitleIcons[] = {"none", "panel_left", "panel_right", "sparkles", "edit",
                                      "plus", "settings", "search", "terminal", "bot", "pin", "bell"};
constexpr const char* kWindowControlSides[] = {"left", "center", "right"};
constexpr const char* kWindowControlVariants[] = {"icons", "traffic_lights", "pills", "classic"};
constexpr const char* kTitleAlignments[] = {"left", "center", "right"};
constexpr const char* kWindowTitleIcons[] = {"none", "bot", "app", "terminal", "sparkles", "folder", "layers"};
constexpr const char* kWindowShadows[] = {"none", "soft", "deep", "glow"};
constexpr ImGuiCol kWindowTitleColors[] = {ImGuiCol_TitleBg, ImGuiCol_TitleBgActive, ImGuiCol_TitleBgCollapsed};

ImRect insetTitleBar(ImRect bar, const WindowAppearance& style, float scale) {
  const float left = std::clamp(style.titleOffsetLeft.value_or(0.0f), 0.0f, 4096.0f) * scale;
  const float right = std::clamp(style.titleOffsetRight.value_or(0.0f), 0.0f, 4096.0f) * scale;
  if (left + right > 0) {
    const float fraction = std::min(1.0f, std::max(0.0f, bar.GetWidth() - 1.0f) / (left + right));
    bar.Min.x += left * fraction;
    bar.Max.x -= right * fraction;
  }
  return bar;
}

bool isTitleMenu(const ImGuiNode& node) {
  return node.type == WidgetType::MenuBar && node.props.menuBarEdge == MenuBarEdge::TitleBar;
}

void renderMenuLabels(const WidgetProps& props) {
  for (size_t i = 0; i < props.items.size(); ++i) {
    ImGui::PushID(static_cast<int>(i));
    if (ImGui::BeginMenu(props.items[i].c_str())) {
      ImGui::MenuItem(surfaceLabel(StudioText::MenuNoActions).c_str(), nullptr, false, false);
      ImGui::EndMenu();
    }
    ImGui::PopID();
  }
}

ImVec4 imColor(const Color4& color) {
  return ImVec4(color.r, color.g, color.b, color.a);
}

template <size_t N>
size_t titleChoice(const std::string& value, const char* const (&choices)[N], size_t fallback = 0) {
  for (size_t i = 0; i < N; ++i) if (value == choices[i]) return i;
  return fallback;
}

template <size_t N>
void titleCombo(const char* label, std::string& value, const char* const (&choices)[N],
                const char* const (&labels)[N], size_t fallback = 0) {
  int index = static_cast<int>(titleChoice(value, choices, fallback));
  if (ImGui::Combo(label, &index, labels, static_cast<int>(N))) value = choices[index];
}

void drawTitleIcon(ImDrawList& draw, size_t icon, ImVec2 center, float radius, ImU32 color) {
  const auto point = [&](float x, float y) { return ImVec2(center.x + x * radius, center.y + y * radius); };
  const float stroke = std::max(1.0f, radius * 0.22f);
  const auto line = [&](float x1, float y1, float x2, float y2) {
    draw.AddLine(point(x1, y1), point(x2, y2), color, stroke);
  };
  const auto box = [&] { draw.AddRect(point(-0.95f, -0.75f), point(0.95f, 0.75f), color, 1, 0, stroke); };
  switch (icon) {
    case 1: case 2: {
      box();
      const float edge = icon == 1 ? -0.35f : 0.35f;
      line(edge, -0.75f, edge, 0.75f);
      break;
    }
    case 3:
      draw.AddQuadFilled(point(0, -1), point(0.57f, 0), point(0, 1), point(-0.57f, 0), color);
      draw.AddCircleFilled(point(0.8f, -0.56f), stroke, color);
      break;
    case 4:
      draw.AddRect(point(-0.85f, -0.48f), point(0.5f, 0.85f), color, 1, 0, stroke);
      line(-0.35f, 0.35f, 0.9f, -0.9f);
      break;
    case 5:
      line(-0.9f, 0, 0.9f, 0); line(0, -0.9f, 0, 0.9f);
      break;
    case 6:
      draw.AddCircle(center, radius * 0.75f, color, 8, stroke);
      draw.AddCircleFilled(center, stroke, color);
      line(-1, 0, -0.75f, 0); line(0.75f, 0, 1, 0);
      line(0, -1, 0, -0.75f); line(0, 0.75f, 0, 1);
      break;
    case 7:
      draw.AddCircle(point(-0.25f, -0.25f), radius * 0.6f, color, 12, stroke);
      line(0.2f, 0.2f, 0.9f, 0.9f);
      break;
    case 8:
      box();
      line(-0.61f, -0.32f, -0.22f, 0); line(-0.22f, 0, -0.61f, 0.32f);
      line(0.04f, 0.32f, 0.59f, 0.32f);
      break;
    case 9:
      draw.AddRect(point(-0.88f, -0.5f), point(0.88f, 0.75f), color, 2, 0, stroke);
      line(0, -0.95f, 0, -0.5f);
      draw.AddCircleFilled(point(-0.36f, 0.07f), stroke, color);
      draw.AddCircleFilled(point(0.36f, 0.07f), stroke, color);
      break;
    case 10:
      draw.AddQuadFilled(point(-0.52f, -0.9f), point(0.52f, -0.9f),
                         point(0.78f, 0.12f), point(-0.78f, 0.12f), color);
      line(0, 0.1f, 0, 0.93f);
      break;
    case 11:
      draw.AddCircleFilled(point(0, -0.16f), radius * 0.6f, color);
      line(-0.78f, 0.42f, 0.78f, 0.42f);
      draw.AddCircleFilled(point(0, 0.79f), stroke, color);
      break;
    default: break;
  }
}

void drawWindowTitleIcon(ImDrawList& draw, size_t icon, ImVec2 center, float radius, ImU32 color) {
  if (icon == 1 || icon == 3 || icon == 4) {
    drawTitleIcon(draw, icon == 1 ? 9 : icon == 3 ? 8 : 3, center, radius, color);
    return;
  }
  const auto pt = [&](float x, float y) { return ImVec2(center.x + x * radius, center.y + y * radius); };
  const float stroke = std::max(1.0f, radius * 0.22f);
  if (icon == 2) {
    draw.AddRect(pt(-1, -0.77f), pt(1, 0.77f), color, stroke, 0, stroke);
    draw.AddLine(pt(-1, -0.27f), pt(1, -0.27f), color, stroke);
  } else if (icon == 5) {
    draw.AddRect(pt(-1, -0.45f), pt(1, 0.72f), color, stroke, 0, stroke);
    draw.AddLine(pt(-0.96f, -0.45f), pt(-0.68f, -0.76f), color, stroke);
    draw.AddLine(pt(-0.68f, -0.76f), pt(-0.18f, -0.76f), color, stroke);
    draw.AddLine(pt(-0.18f, -0.76f), pt(0.10f, -0.45f), color, stroke);
  } else {
    draw.AddQuad(pt(-0.92f, -0.38f), pt(0, -0.9f), pt(0.92f, -0.38f), pt(0, 0.16f), color, stroke);
    for (const float y : {0.0f, 0.39f}) {
      draw.AddLine(pt(-0.92f, y), pt(0, y + 0.53f), color, stroke);
      draw.AddLine(pt(0, y + 0.53f), pt(0.92f, y), color, stroke);
    }
  }
}
ImVec2 sizeOf(const ImGuiNode& node, float scale) {
  return {node.width < 0 ? -FLT_MIN : node.width * scale, std::max(0.0f, node.height) * scale};
}

bool drawImageWidget(const ImGuiNode& node, const NativeImageCache::Texture& texture, float scale) {
  const bool button = node.type == WidgetType::ImageButton;
  const float maxExtent = std::max(1.0f, 16000.0f * scale);
  const float naturalWidth = texture.id ? static_cast<float>(texture.width) : (button ? 64.0f : 160.0f);
  const float naturalHeight = texture.id ? static_cast<float>(texture.height) : (button ? 48.0f : 96.0f);
  float width = node.width < 0 ? ImGui::GetContentRegionAvail().x : node.width * scale;
  float height = node.height * scale;
  if (node.width == 0) width = height > 0 && texture.id ? height * naturalWidth / naturalHeight : naturalWidth * scale;
  width = std::clamp(width, 1.0f, maxExtent);
  if (height <= 0) height = texture.id ? width * naturalHeight / naturalWidth : naturalHeight * scale;
  height = std::clamp(height, 1.0f, maxExtent);
  bool activated = false;
  if (button) activated = ImGui::Button("##image_button", ImVec2(width, height));
  else ImGui::Dummy(ImVec2(width, height));
  if (!ImGui::IsItemVisible()) return activated;

  auto* draw = ImGui::GetWindowDrawList();
  const auto lo = ImGui::GetItemRectMin(), hi = ImGui::GetItemRectMax();
  const auto padding = button ? ImGui::GetStyle().FramePadding : ImVec2();
  if (!button) draw->AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_FrameBg), ImGui::GetStyle().FrameRounding);
  const float innerWidth = width - 2 * padding.x, innerHeight = height - 2 * padding.y;
  if (texture.id && innerWidth > 0 && innerHeight > 0) {
    const float fit = std::min(innerWidth / naturalWidth, innerHeight / naturalHeight);
    const ImVec2 extent(naturalWidth * fit, naturalHeight * fit);
    const ImVec2 start(lo.x + (width - extent.x) * 0.5f, lo.y + (height - extent.y) * 0.5f);
    draw->AddImage(texture.id, start, ImVec2(start.x + extent.x, start.y + extent.y),
                   ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
  } else if (!texture.id) {
    const char* text = texture.error.empty() ? "Sin imagen" : "Imagen no disponible";
    const auto extent = ImGui::CalcTextSize(text);
    draw->PushClipRect(lo, hi, true);
    draw->AddText(ImVec2(lo.x + std::max(0.0f, (width - extent.x) * 0.5f),
                         lo.y + std::max(0.0f, (height - extent.y) * 0.5f)),
                  ImGui::GetColorU32(ImGuiCol_TextDisabled), text);
    draw->PopClipRect();
  }
  if (button && ImGui::IsItemHovered() && !node.label.empty()) ImGui::SetItemTooltip("%s", node.label.c_str());
  return activated;
}
int integerStep(float speed) {
  return static_cast<int>(std::clamp(static_cast<double>(speed), 1.0,
                                   static_cast<double>(std::numeric_limits<int>::max() / 100)));
}
enum class ColorUse { All, Button, Frame, FrameHover, FrameActive, Selection, TextSelection, Check, Slider, Progress };

struct WidgetColorSlot {
  const char* key;
  StudioText label;
  ImGuiCol color;
  ColorUse use;
};

constexpr WidgetColorSlot widgetColors[] = {
  {"textColorOverride", StudioText::PaletteText, ImGuiCol_Text, ColorUse::All},
  {"buttonColorOverride", StudioText::WidgetButtonBg, ImGuiCol_Button, ColorUse::Button},
  {"buttonHoveredColorOverride", StudioText::HoverButton, ImGuiCol_ButtonHovered, ColorUse::Button},
  {"buttonActiveColorOverride", StudioText::PressedButton, ImGuiCol_ButtonActive, ColorUse::Button},
  {"frameBgOverride", StudioText::FrameBgColor, ImGuiCol_FrameBg, ColorUse::Frame},
  {"frameBgHoveredOverride", StudioText::HoverBgColor, ImGuiCol_FrameBgHovered, ColorUse::FrameHover},
  {"frameBgActiveOverride", StudioText::ActiveBgColor, ImGuiCol_FrameBgActive, ColorUse::FrameActive},
  {"headerColorOverride", StudioText::SelectionBg, ImGuiCol_Header, ColorUse::Selection},
  {"headerHoveredColorOverride", StudioText::HoverOption, ImGuiCol_HeaderHovered, ColorUse::Selection},
  {"headerActiveColorOverride", StudioText::PressedOption, ImGuiCol_HeaderActive, ColorUse::Selection},
  {"textSelectedBgOverride", StudioText::SelectedTextBg, ImGuiCol_TextSelectedBg, ColorUse::TextSelection},
  {"checkMarkColorOverride", StudioText::CheckMarkColor, ImGuiCol_CheckMark, ColorUse::Check},
  {"sliderGrabColorOverride", StudioText::SliderGrab, ImGuiCol_SliderGrab, ColorUse::Slider},
  {"sliderGrabActiveColorOverride", StudioText::ActiveSliderGrab, ImGuiCol_SliderGrabActive, ColorUse::Slider},
  {"progressFillColorOverride", StudioText::ProgressFill, ImGuiCol_PlotHistogram, ColorUse::Progress},
};

bool showsWidgetColor(WidgetType type, ColorUse use) {
  const bool check = type == WidgetType::Checkbox || type == WidgetType::RadioButton;
  const bool textInput = type == WidgetType::InputText || type == WidgetType::InputMultiline;
  const bool numeric = floatControl(type) || ProjectJson::integerWidget(type);
  const bool slider = type == WidgetType::SliderFloat || type == WidgetType::SliderFloat2 ||
    type == WidgetType::SliderFloat3 || type == WidgetType::SliderAngle || type == WidgetType::SliderInt;
  const bool frame = textInput || numeric || check || itemList(type) ||
    type == WidgetType::ColorEdit3 || type == WidgetType::ColorEdit4;
  switch (use) {
    case ColorUse::All: return true;
    case ColorUse::Button:
      return type == WidgetType::Toggle || type == WidgetType::Button || type == WidgetType::SmallButton || type == WidgetType::ArrowButton ||
        type == WidgetType::TitleBarButton || type == WidgetType::ImageButton;
    case ColorUse::Frame: return frame || type == WidgetType::ProgressBar || isRoundStatusWidget(type);
    case ColorUse::FrameHover: return check || slider || type == WidgetType::Combo ||
      type == WidgetType::DragFloat || type == WidgetType::DragFloat3 || type == WidgetType::DragInt;
    case ColorUse::FrameActive: return check || slider ||
      type == WidgetType::DragFloat || type == WidgetType::DragFloat3 || type == WidgetType::DragInt;
    case ColorUse::Selection:
      return type == WidgetType::Selectable || isModelView(type) || itemList(type) ||
        type == WidgetType::TreeNode || type == WidgetType::CollapsingHeader || type == WidgetType::MenuBar;
    case ColorUse::TextSelection: return textInput || numeric;
    case ColorUse::Check: return check;
    case ColorUse::Slider: return slider;
    case ColorUse::Progress: return type == WidgetType::ProgressBar || isRoundStatusWidget(type);
  }
  return false;
}

std::optional<Color4> widgetColor(const Json& props, const char* key) {
  if (!props.is_object()) return std::nullopt;
  const auto found = props.find(key);
  if (found == props.end() || !found->is_string()) return std::nullopt;
  try { return ProjectJson::colorFromHex(found->get_ref<const std::string&>()); }
  catch (const std::runtime_error&) { return std::nullopt; }
}

struct NodeColors {
  int count = 0;
  explicit NodeColors(const WidgetProps& p) {
    if (!p.source.is_object()) return;
    for (const auto& slot : widgetColors) {
      if (const auto c = widgetColor(p.source, slot.key)) {
        ImGui::PushStyleColor(slot.color, ImVec4(c->r, c->g, c->b, c->a));
        ++count;
      }
    }
  }
  ~NodeColors() { if (count) ImGui::PopStyleColor(count); }
};

enum AnimationEffect { PulseAlpha, BounceY, SlideX, ScalePulse, ColorShift };
struct AnimationOption { const char* id; const char* label; };
constexpr AnimationOption animationEffects[] = {
  {"pulse_alpha", "Pulso de opacidad"}, {"bounce_y", "Rebote vertical"},
  {"slide_x", "Deslizamiento horizontal"}, {"scale_pulse", "Escala pulsante"},
  {"color_shift", "Cambio cromático"},
};
constexpr AnimationOption animationCurves[] = {
  {"linear", "Lineal"}, {"sine", "Senoidal"}, {"ease_in_out", "Aceleración y frenado"},
  {"bounce", "Rebote"},
};
constexpr double minAnimationDuration = 200;

template<size_t N>
size_t animationOption(const AnimationOption (&options)[N], const std::string& id, size_t fallback) {
  for (size_t i = 0; i < N; ++i) if (id == options[i].id) return i;
  return fallback;
}

template<size_t N>
void animationCombo(const char* label, const AnimationOption (&options)[N], std::string& id, size_t fallback) {
  const auto selected = animationOption(options, id, fallback);
  if (ImGui::BeginCombo(label, options[selected].label)) {
    for (const auto& option : options)
      if (ImGui::Selectable(option.label, id == option.id)) id = option.id;
    ImGui::EndCombo();
  }
}

float animationAmount(const WidgetProps& p, double elapsed) {
  const double duration = std::isfinite(p.animDuration) ? std::max(minAnimationDuration, p.animDuration) : 1400;
  const double cycles = elapsed / (duration / 1000);
  const float phase = static_cast<float>(p.animLoop ? std::fmod(cycles, 1.0) : std::min(cycles, 1.0));
  const float radians = phase * (2 * IM_PI);
  switch (animationOption(animationCurves, p.animEasing, 1)) {
    case 0: return phase;
    case 2: {
      const float p = 2 * std::min(phase, 1 - phase);
      if (p < 0.5f) return 2 * p * p;
      return 1 - 2 * (1 - p) * (1 - p);
    }
    case 3: return std::pow(std::abs(std::sin(radians)), 0.7f);
    default: return 0.5f - 0.5f * std::cos(radians);
  }
}
} // namespace

void applyPreviewStyle(const ImGuiStyleConfig& config) {
  if (config.preset == StylePresetId::ImGuiLight) ImGui::StyleColorsLight();
  else if (config.preset == StylePresetId::ImGuiClassic) ImGui::StyleColorsClassic();
  else ImGui::StyleColorsDark();
  auto& s = ImGui::GetStyle();
  s.WindowRounding = config.windowRounding; s.FrameRounding = config.frameRounding;
  s.WindowPadding = ImVec2(config.windowPaddingX, config.windowPaddingY);
  s.ItemSpacing = ImVec2(config.itemSpacingX, config.itemSpacingY);
  s.Colors[ImGuiCol_WindowBg] = ImVec4(config.windowBg.r, config.windowBg.g, config.windowBg.b, config.windowBg.a);
  s.Colors[ImGuiCol_TitleBgActive] = ImVec4(config.titleBg.r, config.titleBg.g, config.titleBg.b, config.titleBg.a);
  s.Colors[ImGuiCol_Button] = ImVec4(config.buttonColor.r, config.buttonColor.g, config.buttonColor.b, config.buttonColor.a);
  s.Colors[ImGuiCol_FrameBg] = ImVec4(config.frameBg.r, config.frameBg.g, config.frameBg.b, config.frameBg.a);
  s.Colors[ImGuiCol_Text] = ImVec4(config.textColor.r, config.textColor.g, config.textColor.b, config.textColor.a);
  if (!config.source.is_object()) return;
  auto number = [&](const char* key, float& target) {
    if (!config.source.contains(key) || !config.source.at(key).is_number()) return;
    const double value = config.source.at(key).get<double>();
    if (std::isfinite(value) && value >= 0 && value <= 4096) target = static_cast<float>(value);
  };
  number("childRounding", s.ChildRounding); number("grabRounding", s.GrabRounding);
  number("tabRounding", s.TabRounding); number("scrollbarRounding", s.ScrollbarRounding);
  number("framePaddingX", s.FramePadding.x); number("framePaddingY", s.FramePadding.y);
  number("windowBorderSize", s.WindowBorderSize); number("frameBorderSize", s.FrameBorderSize);
  number("alpha", s.Alpha); s.Alpha = std::clamp(s.Alpha, 0.0f, 1.0f);
  if (!config.source.contains("colors") || !config.source.at("colors").is_object()) return;
  const auto& colors = config.source.at("colors");
  const std::pair<const char*, ImGuiCol> mapping[] = {
    {"childBg", ImGuiCol_ChildBg}, {"titleBg", ImGuiCol_TitleBg},
    {"frameBgHovered", ImGuiCol_FrameBgHovered}, {"buttonHovered", ImGuiCol_ButtonHovered},
    {"buttonActive", ImGuiCol_ButtonActive}, {"checkMark", ImGuiCol_CheckMark},
    {"sliderGrab", ImGuiCol_SliderGrab}, {"header", ImGuiCol_Header},
    {"headerHovered", ImGuiCol_HeaderHovered}, {"textDisabled", ImGuiCol_TextDisabled},
    {"border", ImGuiCol_Border}, {"separator", ImGuiCol_Separator}};
  for (const auto& entry : mapping) {
    if (!colors.contains(entry.first) || !colors.at(entry.first).is_string()) continue;
    try {
      const auto c = ProjectJson::colorFromHex(colors.at(entry.first).get<std::string>());
      s.Colors[entry.second] = ImVec4(c.r, c.g, c.b, c.a);
    } catch (const std::exception&) { /* Unknown invalid style data remains in the document. */ }
  }
}

ScopedPreviewWindowStyle::ScopedPreviewWindowStyle(const ImGuiNode& node, float scale, bool minimized)
    : original_(ImGui::GetStyle()) {
  auto& style = ImGui::GetStyle();
  const auto& p = node.props;
  const auto& a = p.windowAppearance;
  if (a.alpha) style.Alpha = std::clamp(*a.alpha, 0.0f, 1.0f);
  if (a.rounding) style.WindowRounding = std::clamp(*a.rounding, 0.0f, 72.0f) * scale;
  if (a.borderSize) style.WindowBorderSize = std::clamp(*a.borderSize, 0.0f, 12.0f) * scale;
  if (p.noBackground) style.WindowBorderSize = 0;
  if (a.paddingX) style.WindowPadding.x = std::clamp(*a.paddingX, 0.0f, 256.0f) * scale;
  if (a.paddingY) style.WindowPadding.y = std::clamp(*a.paddingY, 0.0f, 256.0f) * scale;
  if (a.background) style.Colors[ImGuiCol_WindowBg] = imColor(*a.background);
  if (a.borderColor) style.Colors[ImGuiCol_Border] = imColor(*a.borderColor);
  if (a.textColor) style.Colors[ImGuiCol_Text] = imColor(*a.textColor);
  if (!a.showResizeGrip) {
    for (const auto color : {ImGuiCol_ResizeGrip, ImGuiCol_ResizeGripHovered, ImGuiCol_ResizeGripActive})
      style.Colors[color].w = 0.0f;
  }
  if (minimized) style.WindowMinSize.y = 1.0f;
  if (!p.noTitleBar && a.titleHeight) {
    const float desired = std::clamp(*a.titleHeight, 20.0f, 72.0f) * scale;
    style.FramePadding.y = std::max(0.0f, (desired - ImGui::GetFontSize()) * 0.5f);
  }
  const bool inset = !p.noTitleBar && WindowChrome::hasTitleInsets(a);
  for (size_t i = 0; i < std::size(kWindowTitleColors); ++i) {
    auto& color = style.Colors[kWindowTitleColors[i]];
    if (a.titleBackground) color = imColor(*a.titleBackground);
    if (a.transparentTitle) color.w = 0;
    titleColors_[i] = color;
    if (inset) color.w = 0;
  }
  // An inset title bar draws its own collapse arrow inside the inset.
  if (inset) style.WindowMenuButtonPosition = ImGuiDir_None;
}

void ScopedPreviewWindowStyle::afterBegin() const {
  auto& style = ImGui::GetStyle();
  style.FramePadding = original_.FramePadding;
  style.WindowMinSize = original_.WindowMinSize;
  for (size_t i = 0; i < std::size(kWindowTitleColors); ++i)
    style.Colors[kWindowTitleColors[i]] = titleColors_[i];
}

ScopedPreviewWindowStyle::~ScopedPreviewWindowStyle() {
  ImGui::GetStyle() = original_;
}

void NativePreview::drawWindowShadow(const ImGuiNode& root, ImVec2 canvasMin, ImVec2 canvasMax) const {
  const auto& a = root.props.windowAppearance;
  const size_t shadow = titleChoice(a.shadow, kWindowShadows);
  if (shadow == 0) return;
  auto* window = ImGui::GetCurrentWindow();
  if (window->Hidden) return;
  auto& draw = *window->DrawList;
  const ImRect bounds(window->Pos, ImVec2(window->Pos.x + window->Size.x, window->Pos.y + window->Size.y));
  const int rings = shadow == 1 ? 6 : 10;
  const float thickness = (shadow == 1 ? 1.5f : 2.2f) * scale_;
  ImVec4 tint = shadow == 3 ? ImGui::GetStyle().Colors[ImGuiCol_Border] : ImVec4(0, 0, 0, 1);
  const float opacity = std::min(tint.w, shadow == 1 ? 0.26f : shadow == 2 ? 0.45f : 0.4f);
  draw.PushClipRect(canvasMin, canvasMax, false);
  for (int i = rings; i > 0; --i) {
    const float extent = thickness * static_cast<float>(i);
    const float coverage = static_cast<float>(rings - i + 1) / static_cast<float>(rings);
    tint.w = opacity * coverage * coverage;
    // The rings stay outside the window, even when its body is transparent.
    draw.AddRect(ImVec2(bounds.Min.x - extent, bounds.Min.y - extent),
                 ImVec2(bounds.Max.x + extent, bounds.Max.y + extent * 1.2f),
                 ImGui::GetColorU32(tint), window->WindowRounding + extent, 0, thickness);
  }
  draw.PopClipRect();
}

void NativePreview::reset() {
  values_.clear(); radioGroups_.clear(); flowWidths_.clear(); animationStarts_.clear(); windows_.clear();
  beginFrame();
}
void NativePreview::beginFrame(bool designMode, float scale, std::optional<ChartOrbit> orbit) {
  if (scale != scale_) flowWidths_.clear();
  scale_ = scale;
  orbit_ = std::move(orbit);
  report_ = {}; designMode_ = designMode;
  if (designMode_) radioGroups_.clear();
}
const WidgetProps* NativePreview::value(const std::string& nodeId) const {
  const auto found = values_.find(nodeId);
  return found == values_.end() ? nullptr : &found->second;
}

bool NativePreview::supports(WidgetType type) {
  switch (type) {
    case WidgetType::Text: case WidgetType::TextColored: case WidgetType::TextWrapped:
    case WidgetType::BulletText: case WidgetType::HelpMarker: case WidgetType::SeparatorText:
    case WidgetType::Button: case WidgetType::SmallButton: case WidgetType::ArrowButton: case WidgetType::TitleBarButton:
    case WidgetType::Image: case WidgetType::ImageButton:
    case WidgetType::ColorButton: case WidgetType::Selectable: case WidgetType::Checkbox:
    case WidgetType::Toggle: case WidgetType::Spinner: case WidgetType::CircularProgress:
    case WidgetType::RadioButton: case WidgetType::InputText: case WidgetType::InputMultiline:
    case WidgetType::InputFloat: case WidgetType::InputInt: case WidgetType::SliderFloat:
    case WidgetType::SliderInt: case WidgetType::SliderFloat2: case WidgetType::SliderFloat3:
    case WidgetType::SliderAngle: case WidgetType::DragFloat: case WidgetType::DragFloat3: case WidgetType::Knob:
    case WidgetType::DragInt: case WidgetType::ColorEdit3: case WidgetType::ColorEdit4:
    case WidgetType::Combo: case WidgetType::Listbox: case WidgetType::ProgressBar:
    case WidgetType::ListView: case WidgetType::GridView: case WidgetType::PathView:
    case WidgetType::Separator: case WidgetType::Spacing: case WidgetType::Dummy:
    case WidgetType::SameLine: case WidgetType::PlotLines: case WidgetType::PlotHistogram:
    case WidgetType::Chart2d: case WidgetType::Chart3d:
    case WidgetType::Loader: case WidgetType::Component: case WidgetType::Repeater: case WidgetType::Animation:
    case WidgetType::ChildWindow: case WidgetType::CollapsingHeader: case WidgetType::TreeNode:
    case WidgetType::TabBar: case WidgetType::TabItem: case WidgetType::Group:
    case WidgetType::ColumnLayout: case WidgetType::RowLayout: case WidgetType::PositionerFlow: case WidgetType::GridLayout:
    case WidgetType::Table: case WidgetType::MenuBar: return true;
    default: return false;
  }
}

PreviewItem observeNativeItem(const ImGuiNode& node, const std::string& parentId,
                               const std::string& rootId, int depth, int option) {
  PreviewItem item{node.id, node.type, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), option};
  item.parentId = parentId; item.rootId = rootId; item.depth = depth;
  const auto screen = ImGui::GetCursorScreenPos(), local = ImGui::GetCursorPos();
  item.coordinateOrigin = ImVec2(screen.x - local.x, screen.y - local.y);
  item.drawList = ImGui::GetWindowDrawList();
  item.clipMinimum = item.drawList->GetClipRectMin();
  item.clipMaximum = item.drawList->GetClipRectMax();
  item.windowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
  return item;
}

void NativePreview::observe(const ImGuiNode& node, ImVec2 minimum, ImVec2 maximum, int option,
                            std::optional<PreviewResizeWidth> resizeWidth, ImDrawList* containerDrawList) {
  auto item = observeNativeItem(node, parents_.size() > 1 ? parents_[parents_.size() - 2] : rootId_,
                                rootId_, static_cast<int>(parents_.size()), option);
  item.minimum = minimum; item.maximum = maximum;
  item.resizeWidth = resizeWidth;
  if (containerDrawList) item.drawList = containerDrawList;
  report_.items.push_back(std::move(item));
}

void NativePreview::recordWindow(const ImGuiNode& node) {
  auto item = observeNativeItem(node, {}, node.id, 0);
  item.minimum = ImGui::GetWindowPos();
  const auto size = ImGui::GetWindowSize();
  item.maximum = ImVec2(item.minimum.x + size.x, item.minimum.y + size.y);
  report_.items.push_back(std::move(item));
}

void NativePreview::recordTitleBar(const ImGuiNode& node) {
  if (node.props.noTitleBar) return;
  const auto* window = ImGui::GetCurrentWindow();
  const auto full = window->TitleBarRect();
  const auto bar = insetTitleBar(full, node.props.windowAppearance, scale_);
  auto item = observeNativeItem(node, {}, node.id, 0, -2);
  item.minimum = bar.Min;
  item.maximum = bar.Max;
  item.clipMinimum = window->OuterRectClipped.Min;
  item.clipMaximum = window->OuterRectClipped.Max;
  item.titleBarBounds = PreviewTitleBarBounds{full.Min.x, full.Max.x};
  report_.items.push_back(std::move(item));
}

void NativePreview::render(const std::vector<ImGuiNode>& nodes, const std::string& rootId) {
  if (parents_.empty()) rootId_ = rootId;
  for (const auto& node : nodes) if (node.visible) renderNode(node);
}

void NativePreview::renderWindowBody(const ImGuiNode& root) {
  rootId_ = root.id;
  for (const auto& node : root.children) {
    if (!node.visible || node.type == WidgetType::TitleBarButton || isTitleMenu(node)) continue;
    if (node.props.inTitleBar && canRenderInTitleBar(node.type)) continue;
    if (node.props.inTitleBar) report_.limitations.push_back(node.id + ": " + surfaceText(StudioText::UnhandledTitle));
    renderNode(node);
  }
}

bool NativePreview::canRenderInTitleBar(WidgetType type) {
  return WindowChrome::supportsTitleWidget(type);
}

bool NativePreview::usesCustomTitle(const ImGuiNode& root) {
  return !root.props.noTitleBar && WindowChrome::usesCustomTitle(root);
}

TitleBarHit NativePreview::renderTitleBar(const ImGuiNode& root, PreviewWindowState* state) {
  rootId_ = root.id;
  auto* window = ImGui::GetCurrentWindow();
  auto bar = window->TitleBarRect();
  const auto& style = ImGui::GetStyle();
  const float padding = style.FramePadding.x;
  const float gap = style.ItemInnerSpacing.x;
  const float font = ImGui::GetFontSize();
  const auto& properties = root.props;
  const auto& appearance = properties.windowAppearance;
  const bool inset = WindowChrome::hasTitleInsets(appearance);
  const bool customCollapse = inset && !(window->Flags & ImGuiWindowFlags_NoCollapse);
  auto& draw = *window->DrawList;
  const auto fullBar = bar;
  if (inset) bar = insetTitleBar(bar, appearance, scale_);
  ImRect decorationClip = bar;
  decorationClip.ClipWithFull(window->OuterRectClipped);
  if (decorationClip.GetWidth() <= 0 || decorationClip.GetHeight() <= 0) return TitleBarHit::None;
  if (inset || appearance.titleSeparator) {
    draw.PushClipRect(decorationClip.Min, decorationClip.Max, false);
    if (inset && !appearance.transparentTitle) {
      const auto fill = ImGui::GetColorU32(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ?
        ImGuiCol_TitleBgActive : window->Collapsed ? ImGuiCol_TitleBgCollapsed : ImGuiCol_TitleBg);
      ImDrawFlags corners = ImDrawFlags_RoundCornersNone;
      if (bar.Min.x == fullBar.Min.x) corners |= ImDrawFlags_RoundCornersTopLeft;
      if (bar.Max.x == fullBar.Max.x) corners |= ImDrawFlags_RoundCornersTopRight;
      if (corners != ImDrawFlags_RoundCornersNone) corners &= ~ImDrawFlags_RoundCornersNone;
      draw.AddRectFilled(bar.Min, bar.Max, fill, window->WindowRounding, corners);
    }
    if (appearance.titleSeparator) {
      const auto color = appearance.separatorColor ? ImGui::GetColorU32(imColor(*appearance.separatorColor)) :
        ImGui::GetColorU32(ImGuiCol_Border);
      const float y = bar.Max.y - std::max(0.5f, 0.5f * scale_);
      draw.AddLine(ImVec2(bar.Min.x, y), ImVec2(bar.Max.x, y), color, std::max(1.0f, scale_));
    }
    draw.PopClipRect();
  }
  ImRect inner = bar;
  inner.Min.x += padding + window->WindowBorderSize;
  inner.Max.x -= padding + window->WindowBorderSize;
  const float collapseStart = inner.Min.x;
  if (customCollapse) inner.Min.x += font + gap;
  else if (!(window->Flags & ImGuiWindowFlags_NoCollapse)) {
    if (style.WindowMenuButtonPosition == ImGuiDir_Left) inner.Min.x += font + gap;
    if (style.WindowMenuButtonPosition == ImGuiDir_Right) inner.Max.x -= font + gap;
  }
  if (inner.GetWidth() < 1 || inner.GetHeight() <= 0) return TitleBarHit::None;

  enum class WindowControl { Close, Minimize, Maximize };
  const size_t controlSide = titleChoice(properties.windowControlsSide, kWindowControlSides, 2);
  const size_t controlVariant = titleChoice(properties.windowControlsVariant, kWindowControlVariants);
  std::vector<WindowControl> windowControls;
  if (controlVariant == 1 && properties.closable) windowControls.push_back(WindowControl::Close);
  if (properties.showMinimizeButton) windowControls.push_back(WindowControl::Minimize);
  if (properties.showMaximizeButton) windowControls.push_back(WindowControl::Maximize);
  if (controlVariant != 1 && properties.closable) windowControls.push_back(WindowControl::Close);
  const float controlWidth = std::max(1.0f, std::min(bar.GetHeight() - 2.0f * scale_,
    font * (controlVariant == 1 ? 0.9f : controlVariant == 3 ? 1.0f : 1.25f)));
  const float controlSpan = static_cast<float>(windowControls.size()) * (controlWidth + gap);
  const auto controlRegion = inner;
  if (controlSide == 0) inner.Min.x = std::min(inner.Max.x, inner.Min.x + controlSpan);
  if (controlSide == 2) inner.Max.x = std::max(inner.Min.x, inner.Max.x - controlSpan);

  struct Control {
    const ImGuiNode& node;
    const char* labelEnd;
    float labelWidth, width, height;
    size_t icon;
  };
  std::array<std::vector<Control>, 4> groups;
  const float iconSize = font * 0.82f;
  const float height = std::max(1.0f, inner.GetHeight() - 2.0f * scale_);
  for (const auto& node : root.children) {
    const bool dedicated = node.type == WidgetType::TitleBarButton;
    const bool menu = isTitleMenu(node);
    if (!node.visible || (!dedicated && !menu && !(node.props.inTitleBar && canRenderInTitleBar(node.type))) ||
        inner.GetWidth() < 1.0f) continue;
    const char* end = ImGui::FindRenderedTextEnd(node.label.c_str());
    const float labelWidth = ImGui::CalcTextSize(node.label.c_str(), end).x;
    const size_t icon = dedicated ? titleChoice(node.props.titleBarIcon, kTitleIcons) : 0;
    float natural = labelWidth + (icon ? iconSize + (labelWidth > 0 ? gap : 0.0f) : 0.0f) + 2 * padding;
    float width = node.width > 0 ? node.width * scale_ : std::max(font, natural);
    if (node.type == WidgetType::InputText) width = (node.width > 0 ? width : 120.0f * scale_) +
      (labelWidth > 0 ? labelWidth + gap : 0.0f);
    if (node.type == WidgetType::Checkbox) width = height + (labelWidth > 0 ? labelWidth + gap : 0.0f);
    if (node.type == WidgetType::Text || node.type == WidgetType::TextColored) width = ImGui::CalcTextSize(node.props.text.c_str()).x;
    if (node.type == WidgetType::ArrowButton) width = height;
    if (node.type == WidgetType::SmallButton) width = std::max(font, natural);
    if (menu && node.width <= 0) {
      width = 2 * style.ItemSpacing.x;
      for (const auto& text : node.props.items) width += ImGui::CalcTextSize(text.c_str(), nullptr, true).x + style.ItemSpacing.x;
      if (node.props.items.empty()) width += ImGui::CalcTextSize(surfaceText(StudioText::EmptyMenu)).x;
    }
    groups[titleChoice(node.props.titleBarSide, kTitleSides)].push_back({node, end, labelWidth,
      std::clamp(width, 1.0f, inner.GetWidth()), node.height > 0 ? std::clamp(node.height * scale_, 1.0f, height) : height, icon});
  }

  ImRect clip = controlRegion;
  clip.ClipWithFull(window->OuterRectClipped);
  if (clip.GetWidth() <= 0 || clip.GetHeight() <= 0) return TitleBarHit::None;
  ImGui::PushClipRect(clip.Min, clip.Max, false);
  TitleBarHit hoveredControl = TitleBarHit::None;
  if (customCollapse) {
    ImRect hit(ImVec2(collapseStart, bar.GetCenter().y - font * 0.5f),
               ImVec2(collapseStart + font, bar.GetCenter().y + font * 0.5f));
    hit.ClipWithFull(decorationClip);
    if (hit.GetWidth() > 0 && hit.GetHeight() > 0) {
      ImGui::PushClipRect(decorationClip.Min, decorationClip.Max, false);
      const auto id = window->GetID("##window_inset_collapse");
      if (ImGui::ItemAdd(hit, id)) {
        bool hovered = false, held = false;
        if (ImGui::ButtonBehavior(hit, id, &hovered, &held)) window->WantCollapseToggle = true;
        if (hovered) {
          hoveredControl = TitleBarHit::WindowControl;
          draw.AddRectFilled(hit.Min, hit.Max, ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered), 3 * scale_);
        }
        ImGui::RenderArrow(&draw, hit.Min, ImGui::GetColorU32(ImGuiCol_Text),
                           window->Collapsed ? ImGuiDir_Right : ImGuiDir_Down);
        ImGui::RenderNavCursor(hit, id);
      }
      ImGui::PopClipRect();
    }
  }
  const auto submitWindowControls = [&](float x, float limit) {
    for (const auto kind : windowControls) {
      if (x >= limit) break;
      const float radius = controlWidth * 0.5f;
      const float y = bar.GetCenter().y;
      const ImRect rect(ImVec2(x, y - radius), ImVec2(std::min(x + controlWidth, limit), y + radius));
      x += controlWidth + gap;
      if (!rect.Overlaps(clip)) continue;
      ImRect hit = rect;
      hit.ClipWithFull(clip);
      const char* key = kind == WindowControl::Close ? "##window_close" :
                        kind == WindowControl::Minimize ? "##window_minimize" : "##window_maximize";
      const auto id = window->GetID(key);
      if (!ImGui::ItemAdd(hit, id, nullptr, designMode_ ? ImGuiItemFlags_NoNav : ImGuiItemFlags_None)) continue;
      bool hovered = false, held = false;
      const bool pressed = ImGui::ButtonBehavior(hit, id, &hovered, &held);
      if (hovered) hoveredControl = TitleBarHit::WindowControl;
      if (pressed && state && !designMode_) {
        if (kind == WindowControl::Close) state->closed = true;
        else if (kind == WindowControl::Minimize) state->minimized = !state->minimized;
        else { state->maximized = !state->maximized; state->minimized = false; }
        state->updateGeometry = true;
      }
      const bool restore = state && (kind == WindowControl::Minimize ? state->minimized : state->maximized);
      if (hovered && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("%s", kind == WindowControl::Close ? "Cerrar ventana" :
          kind == WindowControl::Minimize ? (restore ? "Restaurar ventana" : "Minimizar ventana") :
          (restore ? "Restaurar tamaño" : "Maximizar ventana"));
      const auto center = rect.GetCenter();
      const float size = std::min(radius * 0.58f, rect.GetWidth() * 0.35f);
      const float stroke = std::max(1.0f, scale_);
      draw.PushClipRect(hit.Min, hit.Max, true);
      const auto fill = ImGui::GetColorU32(held && hovered ? ImGuiCol_ButtonActive :
        hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
      if (controlVariant == 1) {
        const ImVec4 tint = kind == WindowControl::Close ? ImVec4(1, 0.37f, 0.34f, 1) :
          kind == WindowControl::Minimize ? ImVec4(1, 0.74f, 0.18f, 1) : ImVec4(0.16f, 0.78f, 0.25f, 1);
        draw.AddCircleFilled(center, std::min(radius, rect.GetWidth() * 0.5f), ImGui::GetColorU32(tint));
      } else if (hovered || controlVariant == 2) {
        draw.AddRectFilled(rect.Min, rect.Max, fill, (controlVariant == 2 ? 4.0f : 2.0f) * scale_);
      }
      if (controlVariant != 1 || hovered || held) {
        const auto text = ImGui::GetColorU32(controlVariant == 1 ? ImVec4(0.12f, 0.12f, 0.12f, 1) :
          style.Colors[ImGuiCol_Text]);
        const auto a = ImVec2(center.x - size, center.y - size), b = ImVec2(center.x + size, center.y + size);
        if (kind == WindowControl::Close) {
          draw.AddLine(a, b, text, stroke);
          draw.AddLine(ImVec2(a.x, b.y), ImVec2(b.x, a.y), text, stroke);
        } else if (kind == WindowControl::Minimize && !restore) {
          draw.AddLine(ImVec2(a.x, center.y), ImVec2(b.x, center.y), text, stroke);
        } else {
          draw.AddRect(a, b, text, 0, 0, stroke);
          if (restore) draw.AddLine(ImVec2(a.x, a.y + size * 0.65f), ImVec2(b.x, a.y + size * 0.65f), text, stroke);
        }
      }
      ImGui::RenderNavCursor(hit, id);
      draw.PopClipRect();
    }
  };
  if (controlSide == 0) submitWindowControls(controlRegion.Min.x, inner.Min.x);
  if (controlSide == 2) submitWindowControls(inner.Max.x + gap, controlRegion.Max.x);
  const auto submitWidget = [&](const Control& control, const ImRect& rect, const ImRect& hit) {
    const auto& node = control.node;
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && hit.Contains(ImGui::GetIO().MousePos))
      hoveredControl = TitleBarHit::Widget;
    ImGui::PushClipRect(hit.Min, hit.Max, true);
    if (designMode_) {
      ImGui::PushID(node.id.c_str());
      const auto id = window->GetID("##title_canvas_capture");
      if (ImGui::ItemAdd(hit, id, nullptr, ImGuiItemFlags_NoNav)) {
        bool hovered = false, held = false;
        ImGui::ButtonBehavior(hit, id, &hovered, &held);
      }
      ImGui::PopID();
    }

    // These leaf controls advance a line through ItemSize. Keep that line
    // isolated from the window body, including when Begin skips a folded body.
    auto& layout = window->DC;
    const auto cursor = layout.CursorPos, previous = layout.CursorPosPrevLine, maximum = layout.CursorMaxPos;
    const auto line = layout.CurrLineSize, previousLine = layout.PrevLineSize;
    const auto layoutType = layout.LayoutType;
    const float baseline = layout.CurrLineTextBaseOffset, previousBaseline = layout.PrevLineTextBaseOffset;
    const bool sameLine = layout.IsSameLine, setPosition = layout.IsSetPos, skip = window->SkipItems;
    const auto lastItem = ImGui::GetCurrentContext()->LastItemData;
    auto widget = node;
    widget.useCursorPos = false; widget.sameLine = false; widget.indent = 0;
    const bool text = node.type == WidgetType::Text || node.type == WidgetType::TextColored;
    const bool menu = isTitleMenu(node);
    const bool centerLabel = text || menu || node.type == WidgetType::SmallButton;
    const float itemY = centerLabel ? std::max(rect.Min.y, rect.GetCenter().y - font * 0.5f) : rect.Min.y;
    const float labelWidth = node.type == WidgetType::InputText && control.labelWidth > 0 ? control.labelWidth + gap : 0.0f;
    widget.width = std::max(1.0f, rect.GetWidth() - labelWidth) / scale_;
    widget.height = rect.GetHeight() / scale_;
    layout.CursorPos = ImVec2(rect.Min.x, itemY);
    layout.CurrLineSize = {};
    layout.CurrLineTextBaseOffset = 0;
    layout.IsSameLine = layout.IsSetPos = false;
    window->SkipItems = false;
    const size_t firstItem = report_.items.size();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding, std::max(0.0f, (rect.GetHeight() - font) * 0.5f)));
    if (menu) {
      NodeColors colors(node.props);
      ImGui::PushID(node.id.c_str());
      layout.LayoutType = ImGuiLayoutType_Horizontal;
      ImGui::BeginDisabled(designMode_);
      if (node.props.items.empty()) ImGui::TextDisabled("%s", surfaceText(StudioText::EmptyMenu));
      else renderMenuLabels(node.props);
      ImGui::EndDisabled();
      ImGui::PopID();
      report_.items.push_back(observeNativeItem(node, root.id, root.id, 1));
    } else renderNode(widget);
    ImGui::PopStyleVar();
    for (size_t i = firstItem; i < report_.items.size(); ++i) {
      auto& item = report_.items[i];
      item.minimum = rect.Min; item.maximum = rect.Max; item.coordinateOrigin = bar.Min; item.titleBar = true;
    }
    layout.CursorPos = cursor; layout.CursorPosPrevLine = previous; layout.CursorMaxPos = maximum;
    layout.CurrLineSize = line; layout.PrevLineSize = previousLine;
    layout.LayoutType = layoutType;
    layout.CurrLineTextBaseOffset = baseline; layout.PrevLineTextBaseOffset = previousBaseline;
    layout.IsSameLine = sameLine; layout.IsSetPos = setPosition; window->SkipItems = skip;
    ImGui::GetCurrentContext()->LastItemData = lastItem;
    ImGui::PopClipRect();
  };
  auto submit = [&](const Control& control, float x, float limit) {
    const auto& node = control.node;
    const auto& p = node.props;
    const float y = p.titleBarSide == "free" && p.titleBarY
      ? bar.Min.y + std::clamp(*p.titleBarY * scale_, 0.0f, bar.GetHeight() - control.height)
      : bar.GetCenter().y - control.height * 0.5f;
    const ImRect rect(ImVec2(x, y), ImVec2(std::min(x + control.width, limit), y + control.height));
    if (rect.GetWidth() <= 0 || !rect.Overlaps(clip)) return;
    ImRect hit = rect;
    hit.ClipWithFull(clip);
    if (node.type != WidgetType::TitleBarButton) {
      submitWidget(control, rect, hit);
      return;
    }
    ImGui::PushID(node.id.c_str());
    const auto id = window->GetID("##title_button");
    if (ImGui::ItemAdd(hit, id, nullptr, designMode_ ? ImGuiItemFlags_NoNav : ImGuiItemFlags_None)) {
      bool hovered = false, held = false;
      const bool pressed = ImGui::ButtonBehavior(hit, id, &hovered, &held);
      if (hovered) hoveredControl = TitleBarHit::Widget;
      if (pressed && !designMode_) report_.activatedIds.push_back(node.id);
      auto item = observeNativeItem(node, root.id, root.id, 1);
      item.minimum = rect.Min; item.maximum = rect.Max; item.coordinateOrigin = bar.Min; item.titleBar = true;
      report_.items.push_back(std::move(item));

      NodeColors colors(p);
      const size_t variant = titleChoice(p.titleBarBtnVariant, kTitleVariants, 1);
      const float rounding = std::clamp(p.titleBarBtnRounding.value_or(variant == 1 ? 999.0f : 6.0f) * scale_,
                                       0.0f, rect.GetHeight() * 0.5f);
      const auto color = ImGui::GetColorU32(held && hovered ? ImGuiCol_ButtonActive
        : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
      if (variant == 1 || variant == 3 || hovered) draw.AddRectFilled(rect.Min, rect.Max, color, rounding);
      if (variant == 2) draw.AddRect(rect.Min, rect.Max, color, rounding);
      ImGui::RenderNavCursor(hit, id);
      const auto textColor = ImGui::GetColorU32(ImGuiCol_Text);
      const float iconWidth = control.icon ? iconSize + (control.labelWidth > 0 ? gap : 0.0f) : 0.0f;
      const float contentX = rect.Min.x + std::max(padding, (rect.GetWidth() - control.labelWidth - iconWidth) * 0.5f);
      draw.PushClipRect(hit.Min, hit.Max, true);
      if (control.icon) drawTitleIcon(draw, control.icon, ImVec2(contentX + iconSize * 0.5f, rect.GetCenter().y),
                                     std::min(iconSize, rect.GetHeight()) * 0.45f, textColor);
      draw.AddText(ImVec2(contentX + iconWidth, rect.GetCenter().y - font * 0.5f), textColor,
                   node.label.c_str(), control.labelEnd);
      draw.PopClipRect();
    }
    ImGui::PopID();
  };
  const auto widthOf = [gap](const std::vector<Control>& group) {
    float width = 0;
    for (const auto& control : group) width += control.width + gap;
    return width;
  };
  const auto submitGroup = [&](const std::vector<Control>& group, float x, float limit) {
    for (const auto& control : group) {
      if (x >= limit) break;
      submit(control, x, limit);
      x += control.width + gap;
    }
    return std::min(x, limit);
  };
  const float right = std::max(inner.Min.x, inner.Max.x - widthOf(groups[2]));
  const float left = submitGroup(groups[0], inner.Min.x, right);
  submitGroup(groups[2], right + (groups[2].empty() ? 0.0f : gap), inner.Max.x);
  const float centerControls = controlSide == 1 ? controlSpan : 0.0f;
  const float centerWidth = centerControls + widthOf(groups[1]);
  const float center = std::max(left, std::min(bar.GetCenter().x - centerWidth * 0.5f, right - centerWidth));
  if (controlSide == 1) submitWindowControls(center, right);
  submitGroup(groups[1], center + centerControls, right);
  float titleEnd = centerWidth > 0 ? center : right;
  for (const auto& control : groups[3]) {
    float start = inner.Min.x, limit = inner.Max.x;
    const float preferred = bar.Min.x + control.node.props.titleBarX * scale_;
    if (centerControls > 0) {
      if (preferred < center) limit = center;
      else start = std::min(limit, center + centerControls);
    }
    if (limit - start < control.width) continue;
    const float x = std::clamp(preferred, start, limit - control.width);
    submit(control, x, limit);
    if (x + control.width > left) titleEnd = std::min(titleEnd, x - gap);
  }
  const size_t titleIcon = titleChoice(appearance.titleIcon, kWindowTitleIcons);
  if ((properties.showTitleText || titleIcon != 0) && titleEnd > left) {
    const char* end = ImGui::FindRenderedTextEnd(root.label.c_str());
    const float labelWidth = properties.showTitleText ? ImGui::CalcTextSize(root.label.c_str(), end).x : 0.0f;
    const float iconWidth = titleIcon != 0 ? iconSize + (labelWidth > 0 ? gap : 0.0f) : 0.0f;
    const float width = labelWidth + iconWidth;
    const float alignment = 0.5f * static_cast<float>(titleChoice(properties.titleAlign, kTitleAlignments));
    const ImVec2 lo(left + std::max(0.0f, titleEnd - left - width) * alignment, bar.GetCenter().y - font * 0.5f);
    draw.PushClipRect(ImVec2(left, bar.Min.y), ImVec2(titleEnd, bar.Max.y), true);
    const auto color = ImGui::GetColorU32(ImGuiCol_Text);
    if (titleIcon != 0) drawWindowTitleIcon(draw, titleIcon,
      ImVec2(lo.x + iconSize * 0.5f, bar.GetCenter().y), iconSize * 0.45f, color);
    if (properties.showTitleText)
      draw.AddText(ImVec2(lo.x + iconWidth, lo.y), color, root.label.c_str(), end);
    draw.PopClipRect();
  }
  ImGui::PopClipRect();
  return hoveredControl;
}

void NativePreview::renderNode(const ImGuiNode& n, bool selectTab, bool flowChild) {
  const auto inserted = values_.try_emplace(n.id, n.props);
  auto& p = inserted.first->second;
  const bool openChanged = designMode_ && p.defaultOpen != n.props.defaultOpen;
  if (designMode_) p = n.props;
  parents_.push_back(n.id);
  const bool disabled = designMode_ && !isContainerType(n.type);
  if (disabled) { ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); ImGui::BeginDisabled(); }
  ImGui::PushID(n.id.c_str());
  NodeColors colors(n.props);
  const auto size = sizeOf(n, scale_);
  if (n.useCursorPos) ImGui::SetCursorPos(ImVec2(n.x * scale_, n.y * scale_));
  else if (n.sameLine && !flowChild) {
    if (n.sameLineSpacing > 0) ImGui::SameLine(0, n.sameLineSpacing * scale_);
    else ImGui::SameLine();
  }
  if (n.indent > 0) ImGui::Indent(n.indent * scale_);
  if (n.width != 0 && n.type != WidgetType::PositionerFlow && n.type != WidgetType::Chart2d && n.type != WidgetType::Chart3d &&
      n.type != WidgetType::Animation && n.type != WidgetType::Knob && !isStatusWidget(n.type) &&
      !isImageWidget(n.type) && !isModelView(n.type) && !isTemplateContainer(n.type))
    ImGui::SetNextItemWidth(size.x);
  std::optional<PreviewResizeWidth> resizeWidth;
  const float statusWidth = isStatusWidget(n.type)
    ? StudioStatusWidgets::extent(size.x, (n.type == WidgetType::Toggle ? 44.0f : n.type == WidgetType::Spinner ? 36.0f : 80.0f) * scale_,
      n.type == WidgetType::Toggle ? 12.0f : 16.0f)
    : 0.0f;
  if (designMode_ && isStatusWidget(n.type)) {
    resizeWidth = PreviewResizeWidth{statusWidth, n.type == WidgetType::Toggle ? 12.0f : 16.0f};
  } else if (designMode_ && canvasResizeAxes(n.type) == CanvasResizeAxes::Width) {
    // Capture the frame before the widget consumes SetNextItemWidth. Its item
    // rectangle may also include a label or a group of controls.
    const auto spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    float minimum = 1.0f;
    if (n.type == WidgetType::InputInt || (n.type == WidgetType::InputFloat && p.speed > 0))
      minimum = 1.0f + 2.0f * (ImGui::GetFrameHeight() + spacing);
    else if (vectorControl(n.type)) {
      const float components = n.type == WidgetType::SliderFloat2 ? 2.0f : 3.0f;
      minimum = components + (components - 1) * spacing;
    } else if (n.type == WidgetType::ColorEdit3 || n.type == WidgetType::ColorEdit4) {
      const float components = n.type == WidgetType::ColorEdit3 ? 3.0f : 4.0f;
      minimum = ImGui::GetFrameHeight() + components * (1.0f + spacing);
    }
    if (n.type == WidgetType::Knob) {
      const float width = n.width > 0 ? n.width : 80.0f;
      resizeWidth = PreviewResizeWidth{std::clamp(width, 8.0f, 4096.0f) * scale_, 8.0f * scale_};
    } else if (minimum <= 16000.0f * scale_)
      resizeWidth = PreviewResizeWidth{std::max(ImGui::CalcItemWidth(), minimum), std::max(8.0f * scale_, minimum)};
  } else if (designMode_ && (n.type == WidgetType::InputMultiline || n.type == WidgetType::Listbox ||
                             n.type == WidgetType::PlotLines || n.type == WidgetType::PlotHistogram)) {
    // These widgets accept an explicit size and draw the label outside the frame.
    float frameWidth = ImGui::CalcItemWidth();
    if (size.x > 0) frameWidth = size.x;
    if (size.x < 0) frameWidth = std::max(4.0f, ImGui::GetContentRegionAvail().x + size.x);
    if (n.type == WidgetType::InputMultiline || n.type == WidgetType::Listbox)
      frameWidth = std::floor(frameWidth);
    resizeWidth = PreviewResizeWidth{frameWidth, 8.0f * scale_};
  }
  const std::string label = n.label + "###control";
  const bool integer = ProjectJson::integerWidget(n.type);
  const char* format = safeNumericFormat(p.format, integer) ? p.format.c_str() : (integer ? "%d" : "%.3f");
  if (!p.format.empty() && (integer || floatControl(n.type)) && !safeNumericFormat(p.format, integer))
    report_.limitations.push_back(n.id + ": " + surfaceText(StudioText::NumericFormatHint));
  const float minimum = std::min(p.minFloat, p.maxFloat), maximum = std::max(p.minFloat, p.maxFloat);
  const int minInt = std::min(p.minInt, p.maxInt), maxInt = std::max(p.minInt, p.maxInt);
  const bool groupObservation = n.type == WidgetType::TabBar || n.type == WidgetType::TabItem || !supports(n.type);
  if (groupObservation) ImGui::BeginGroup();
  bool activated = false, record = true;
  ImDrawList* containerDrawList = nullptr;
  const auto beginChild = [&](const char* id, ImVec2 dimensions, ImGuiChildFlags flags) {
    const bool open = ImGui::BeginChild(id, dimensions, flags);
    if (designMode_) containerDrawList = ImGui::GetWindowDrawList();
    return open;
  };
  const auto emptyContainer = [&] {
    ImGui::Dummy(ImVec2(std::max(60.0f * scale_, ImGui::GetContentRegionAvail().x), 36.0f * scale_));
  };
  auto children = [&] {
    render(n.children);
    if (designMode_ && n.children.empty()) emptyContainer();
  };
  const bool sizedUtility = (n.useCursorPos || p.utilityCustomSize) && isFreeLayoutUtility(n.type);
  const auto separator = [&] {
    const bool withText = n.type == WidgetType::SeparatorText;
    if (!sizedUtility) {
      if (withText) ImGui::SeparatorText(n.label.c_str());
      else ImGui::Separator();
      if (designMode_) {
        const auto lo = ImGui::GetItemRectMin(), hi = ImGui::GetItemRectMax();
        observe(n, lo, ImVec2(hi.x, std::max(hi.y, lo.y + 8 * scale_)));
        record = false;
      }
      return;
    }
    const auto& style = ImGui::GetStyle();
    const float width = size.x < 0 ? ImGui::GetContentRegionAvail().x : size.x;
    ImGui::Dummy(ImVec2(std::max(1.0f, width), std::max(1.0f, size.y)));
    const auto lo = ImGui::GetItemRectMin(), hi = ImGui::GetItemRectMax();
    auto& draw = *ImGui::GetWindowDrawList();
    const float y = (lo.y + hi.y) * 0.5f;
    float lineStart = lo.x;
    if (withText) {
      const char* start = n.label.c_str();
      const auto* end = ImGui::FindRenderedTextEnd(start);
      const auto extent = ImGui::CalcTextSize(start, end);
      draw.PushClipRect(lo, hi, true);
      draw.AddText(ImVec2(lo.x, y - extent.y * 0.5f), ImGui::GetColorU32(ImGuiCol_Text), start, end);
      draw.PopClipRect();
      lineStart += extent.x + style.ItemInnerSpacing.x;
    }
    if (lineStart < hi.x) draw.AddLine(ImVec2(lineStart, y), ImVec2(hi.x, y), ImGui::GetColorU32(ImGuiCol_Separator));
  };
  switch (n.type) {
    case WidgetType::Image: case WidgetType::ImageButton: {
      const auto texture = images_ ? images_->texture(p.imageAsset) : NativeImageCache::Texture{};
      if (!texture.error.empty()) report_.limitations.push_back(n.label + ": " + texture.error);
      activated = drawImageWidget(n, texture, scale_);
      break;
    }
    case WidgetType::Text: ImGui::TextUnformatted(p.text.c_str()); break;
    case WidgetType::TextColored:
      ImGui::TextColored(ImVec4(p.color.r, p.color.g, p.color.b, p.color.a), "%s", p.text.c_str()); break;
    case WidgetType::TextWrapped:
      ImGui::PushTextWrapPos(0); ImGui::TextUnformatted(p.text.c_str()); ImGui::PopTextWrapPos(); break;
    case WidgetType::BulletText: ImGui::BulletText("%s", p.text.c_str()); break;
    case WidgetType::HelpMarker:
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered()) { ImGui::BeginTooltip(); ImGui::TextUnformatted(p.text.c_str()); ImGui::EndTooltip(); }
      break;
    case WidgetType::SeparatorText: case WidgetType::Separator: separator(); break;
    case WidgetType::Button: case WidgetType::TitleBarButton: activated = ImGui::Button(label.c_str(), size); break;
    case WidgetType::SmallButton: activated = ImGui::SmallButton(label.c_str()); break;
    case WidgetType::ArrowButton: {
      ImGuiDir direction = ImGuiDir_Right;
      if (p.arrowDir == "left") direction = ImGuiDir_Left;
      if (p.arrowDir == "up") direction = ImGuiDir_Up;
      if (p.arrowDir == "down") direction = ImGuiDir_Down;
      activated = ImGui::ArrowButton(label.c_str(), direction); break;
    }
    case WidgetType::ColorButton:
      activated = ImGui::ColorButton(label.c_str(), ImVec4(p.color.r, p.color.g, p.color.b, p.color.a), 0, size); break;
    case WidgetType::Selectable:
      // The spacing Dear ImGui adds to the click area isn't part of the layout size.
      ImGui::Selectable(label.c_str(), &p.selected, designMode_ ? ImGuiSelectableFlags_NoPadWithHalfSpacing : 0,
        ImVec2(size.x < 0 ? std::max(1.0f, ImGui::GetContentRegionAvail().x) : size.x, size.y));
      break;
    case WidgetType::Checkbox: ImGui::Checkbox(label.c_str(), &p.checked); break;
    case WidgetType::Toggle:
      activated = StudioStatusWidgets::toggle("##toggle", n.label.c_str(), p.checked,
        statusWidth, size.y > 0.0f ? size.y : 24.0f * scale_);
      break;
    case WidgetType::Spinner:
      StudioStatusWidgets::spinner(n.label.c_str(), statusWidth, p.speed, !designMode_);
      break;
    case WidgetType::CircularProgress:
      StudioStatusWidgets::circularProgress(n.label.c_str(), statusWidth, p.progress, p.overlay.c_str());
      break;
    case WidgetType::RadioButton: {
      auto& group = radioGroups_.try_emplace(n.varName, p.intValue).first->second;
      ImGui::RadioButton(label.c_str(), &group, p.radioValue);
      p.intValue = group; break;
    }
    case WidgetType::InputText: ImGui::InputTextWithHint(label.c_str(), p.hint.c_str(), &p.text); break;
    case WidgetType::InputMultiline: ImGui::InputTextMultiline(label.c_str(), &p.text, size); break;
    case WidgetType::InputFloat: ImGui::InputFloat(label.c_str(), &p.floatValue, p.speed, p.speed * 10, format); break;
    case WidgetType::InputInt: {
      const int step = integerStep(p.speed), fast = step * 10;
      ImGui::InputScalar(label.c_str(), ImGuiDataType_S32, &p.intValue, &step, &fast, format); break;
    }
    case WidgetType::SliderFloat: ImGui::SliderFloat(label.c_str(), &p.floatValue, minimum, maximum, format); break;
    case WidgetType::SliderInt: ImGui::SliderInt(label.c_str(), &p.intValue, minInt, maxInt, format); break;
    case WidgetType::SliderAngle: ImGui::SliderAngle(label.c_str(), &p.floatValue, minimum, maximum,
      safeNumericFormat(p.format, false) ? p.format.c_str() : "%.0f deg"); break;
    case WidgetType::DragFloat: ImGui::DragFloat(label.c_str(), &p.floatValue, std::max(0.0f, p.speed), minimum, maximum, format); break;
    case WidgetType::Knob: {
      const float width = std::clamp(n.width > 0 ? n.width : 80.0f, 8.0f, 4096.0f);
      ImGui::BeginGroup();
      if (!n.label.empty()) ImGui::TextUnformatted(n.label.c_str());
      if (knobsInstalled_) {
        const auto settings = knobSettings(p);
        p.floatValue = settings.value;
        activated = ImGuiKnobs::Knob("##dial", &p.floatValue, settings.minimum, settings.maximum,
          settings.speed, format, ImGuiKnobVariant_Wiper, width,
          ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_AlwaysClamp);
      } else {
        const auto* notice = surfaceText(StudioText::CatalogKnobMissing);
        ImGui::TextDisabled("%s", surfaceText(StudioText::CatalogKnob));
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", notice);
        report_.limitations.push_back(n.id + ": " + notice);
      }
      ImGui::EndGroup();
      break;
    }
    case WidgetType::DragInt: ImGui::DragInt(label.c_str(), &p.intValue, std::max(0.0f, p.speed), minInt, maxInt, format); break;
    case WidgetType::SliderFloat2: case WidgetType::SliderFloat3: case WidgetType::DragFloat3: {
      const size_t count = n.type == WidgetType::SliderFloat2 ? 2 : 3;
      if (p.defaultVec.size() < count) p.defaultVec.resize(count, 0);
      if (count == 2) ImGui::SliderFloat2(label.c_str(), p.defaultVec.data(), minimum, maximum, format);
      else if (n.type == WidgetType::SliderFloat3) ImGui::SliderFloat3(label.c_str(), p.defaultVec.data(), minimum, maximum, format);
      else ImGui::DragFloat3(label.c_str(), p.defaultVec.data(), std::max(0.0f, p.speed), minimum, maximum, format);
      break;
    }
    case WidgetType::ColorEdit3: ImGui::ColorEdit3(label.c_str(), &p.color.r); break;
    case WidgetType::ColorEdit4: ImGui::ColorEdit4(label.c_str(), &p.color.r); break;
    case WidgetType::Combo: case WidgetType::Listbox: {
      const bool combo = n.type == WidgetType::Combo;
      const bool valid = p.selectedIndex >= 0 && static_cast<size_t>(p.selectedIndex) < p.items.size();
      const char* current = valid ? p.items[static_cast<size_t>(p.selectedIndex)].c_str() : "(sin seleccion)";
      const bool open = combo ? ImGui::BeginCombo(label.c_str(), current) : ImGui::BeginListBox(label.c_str(), size);
      if (combo || !open) {
        observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), -1, resizeWidth); record = false;
      }
      if (open) {
        for (size_t i = 0; i < p.items.size(); ++i) {
          ImGui::PushID(static_cast<int>(i));
          const bool selected = p.selectedIndex == static_cast<int>(i);
          if (ImGui::Selectable(p.items[i].c_str(), selected)) p.selectedIndex = static_cast<int>(i);
          observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), static_cast<int>(i));
          if (selected) ImGui::SetItemDefaultFocus();
          ImGui::PopID();
        }
        if (combo) ImGui::EndCombo(); else ImGui::EndListBox();
      }
      break;
    }
    case WidgetType::ListView: case WidgetType::GridView: case WidgetType::PathView: {
      const bool grid = n.type == WidgetType::GridView, path = n.type == WidgetType::PathView;
      const float height = size.y > 0 ? size.y : modelViewDefaultHeight(n.type) * scale_;
      const auto flags = ImGuiChildFlags_AlwaysUseWindowPadding | (p.border ? ImGuiChildFlags_Borders : 0);
      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
      if (beginChild("model", ImVec2(size.x, height), flags)) {
        if (!n.label.empty()) ImGui::TextUnformatted(n.label.c_str());
        const int count = static_cast<int>(p.items.size());
        if (count == 0) ImGui::TextDisabled("%s", surfaceText(StudioText::NoItems));
        else if (path) {
          const int remainder = p.selectedIndex % count;
          const int selected = remainder < 0 ? remainder + count : remainder;
          const int previous = selected == 0 ? count - 1 : selected - 1;
          const int next = selected + 1 == count ? 0 : selected + 1;
          ImGui::TextDisabled("%d / %d", selected + 1, count);
          const float arrowWidth = ImGui::GetFrameHeight();
          const float labelWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x -
            2.0f * (arrowWidth + ImGui::GetStyle().ItemSpacing.x));
          ImGui::BeginDisabled(count == 1);
          if (ImGui::ArrowButton("previous", ImGuiDir_Left)) p.selectedIndex = previous;
          observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), previous);
          ImGui::EndDisabled();
          ImGui::SameLine();
          ImGui::Dummy(ImVec2(labelWidth, arrowWidth));
          ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
            ImGui::GetColorU32(ImGuiCol_Header), ImGui::GetStyle().FrameRounding);
          drawModelLabel(p.items[static_cast<size_t>(selected)], true);
          ImGui::SameLine();
          ImGui::BeginDisabled(count == 1);
          if (ImGui::ArrowButton("next", ImGuiDir_Right)) p.selectedIndex = next;
          observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), next);
          ImGui::EndDisabled();
        } else {
          const int selected = std::clamp(p.selectedIndex, 0, count - 1);
          const auto drawOption = [&](int index) {
            ImGui::PushID(index);
            const float rowHeight = ImGui::GetFrameHeight() * (grid ? 1.5f : 1.0f);
            if (ImGui::Selectable("##model_option", index == selected, 0, ImVec2(0, rowHeight)))
              p.selectedIndex = index;
            drawModelLabel(p.items[static_cast<size_t>(index)], grid);
            observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), index);
            ImGui::PopID();
          };
          if (grid) {
            const int columns = std::clamp(p.columns, 1, kMaxModelColumns);
            const float padding = std::clamp(p.spacing, 0.0f, 32.0f) * 0.5f * scale_;
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(padding, padding));
            if (ImGui::BeginTable("model_cells", columns, ImGuiTableFlags_SizingStretchSame)) {
              ImGuiListClipper clipper;
              clipper.Begin((count + columns - 1) / columns);
              while (clipper.Step()) for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                ImGui::TableNextRow();
                for (int index = row * columns; index < std::min(count, (row + 1) * columns); ++index) {
                  ImGui::TableNextColumn();
                  drawOption(index);
                }
              }
              ImGui::EndTable();
            }
            ImGui::PopStyleVar();
          } else {
            ImGuiListClipper clipper;
            clipper.Begin(count);
            while (clipper.Step()) for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) drawOption(i);
          }
        }
      }
      ImGui::EndChild();
      ImGui::PopStyleColor();
      break;
    }
    case WidgetType::ProgressBar: ImGui::ProgressBar(std::clamp(p.progress, 0.0f, 1.0f), size,
      p.overlay.empty() ? nullptr : p.overlay.c_str()); break;
    case WidgetType::Spacing: {
      const auto start = ImGui::GetCursorScreenPos();
      if (sizedUtility) {
        const float width = size.x < 0 ? ImGui::GetContentRegionAvail().x : size.x;
        ImGui::Dummy(ImVec2(std::max(1.0f, width), std::max(1.0f, size.y)));
      }
      else ImGui::Spacing();
      if (designMode_) {
        const auto hi = sizedUtility ? ImGui::GetItemRectMax() :
          ImVec2(start.x + std::max(24.0f * scale_, size.x), start.y + std::max(4.0f * scale_, ImGui::GetStyle().ItemSpacing.y));
        ImGui::GetWindowDrawList()->AddRect(start, hi, ImGui::GetColorU32(ImGuiCol_Border));
        observe(n, start, hi);
        record = false;
      }
      break;
    }
    case WidgetType::Dummy: ImGui::Dummy(ImVec2(n.width < 0 ? ImGui::GetContentRegionAvail().x : size.x, size.y)); break;
    case WidgetType::SameLine: ImGui::SameLine(); record = false; break;
    case WidgetType::PlotLines: case WidgetType::PlotHistogram:
      if (n.type == WidgetType::PlotLines)
        ImGui::PlotLines(label.c_str(), p.plotValues.data(), static_cast<int>(p.plotValues.size()), 0,
          p.overlay.empty() ? nullptr : p.overlay.c_str(), minimum, maximum, size);
      else ImGui::PlotHistogram(label.c_str(), p.plotValues.data(), static_cast<int>(p.plotValues.size()), 0,
          p.overlay.empty() ? nullptr : p.overlay.c_str(), minimum, maximum, size);
      break;
    case WidgetType::Chart2d: Chart2d::render(p, size, scale_, designMode_); break;
    case WidgetType::Chart3d:
      Chart3d::render(p, size, scale_, designMode_, orbit_ && orbit_->nodeId == n.id ? &orbit_->rotation : nullptr);
      break;
    case WidgetType::Animation: {
      ImGuiChildFlags flags = ImGuiChildFlags_Borders;
      if (size.y == 0) flags |= ImGuiChildFlags_AutoResizeY;
      if (beginChild("animation", size, flags)) {
        ImGui::BeginDisabled(designMode_);
        if (ImGui::Button(p.animRunning ? "Pausa###animation_play" : "Reproducir###animation_play")) {
          p.animRunning = !p.animRunning;
          animationStarts_.erase(n.id);
        }
        if (!designMode_) observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0);
        ImGui::EndDisabled();
        if (!n.label.empty()) {
          ImGui::SameLine();
          ImGui::TextUnformatted(n.label.c_str());
        }

        const bool playing = !designMode_ && p.animRunning;
        float amount = 0.5f;
        if (playing) {
          const double now = ImGui::GetTime();
          const double start = animationStarts_.try_emplace(n.id, now).first->second;
          amount = animationAmount(p, now - start);
        }
        const auto effect = animationOption(animationEffects, p.animType, PulseAlpha);
        const float horizontalTravel = effect == SlideX ? 14 * scale_ : 0;
        const float verticalTravel = effect == BounceY ? 8 * scale_ : 0;
        const float width = std::max(1.0f, ImGui::GetContentRegionAvail().x - horizontalTravel);
        const float offsetY = verticalTravel * (1 - amount);
        const auto origin = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(origin.x + horizontalTravel * amount, origin.y + offsetY));

        // A child window gives explicit and flowing children the same moving origin.
        const bool open = ImGui::BeginChild("content", ImVec2(width, 0), ImGuiChildFlags_AutoResizeY,
          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (open) {
          auto& style = ImGui::GetStyle();
          const ImGuiStyle baseStyle = style;
          const float baseScale = scale_, fontScale = ImGui::GetIO().FontGlobalScale;
          const float pulseScale = playing && effect == ScalePulse ? 0.92f + amount * 0.1f : 1.0f;
          if (playing && effect == PulseAlpha) style.Alpha *= 0.35f + amount * 0.65f;
          if (playing && effect == ColorShift) for (auto& color : style.Colors) {
            float h, s, v;
            ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
            ImGui::ColorConvertHSVtoRGB(std::fmod(h + amount * 0.5f, 1.0f), s, v, color.x, color.y, color.z);
          }
          if (pulseScale != 1) {
            style.ScaleAllSizes(pulseScale);
            scale_ *= pulseScale;
            ImGui::GetIO().FontGlobalScale *= pulseScale;
            ImGui::PushFont(ImGui::GetFont());
          }
          if (std::any_of(n.children.begin(), n.children.end(), [](const auto& child) { return child.visible; }))
            render(n.children);
          else if (designMode_)
            ImGui::TextDisabled("%s", surfaceText(StudioText::DropIntoContainer));
          else
            ImGui::TextUnformatted(surfaceText(StudioText::AnimationPreview));
          if (pulseScale != 1) {
            ImGui::GetIO().FontGlobalScale = fontScale;
            ImGui::PopFont();
            scale_ = baseScale;
          }
          style = baseStyle;
        }
        ImGui::EndChild();
        const float remainingTravel = verticalTravel - offsetY;
        if (remainingTravel > 0) {
          ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);
          ImGui::Dummy(ImVec2(0, remainingTravel));
        }
      }
      ImGui::EndChild();
      break;
    }
    case WidgetType::ChildWindow:
      if (beginChild("child", size, p.border ? ImGuiChildFlags_Borders : 0)) children();
      ImGui::EndChild(); break;
    case WidgetType::Loader: case WidgetType::Component: case WidgetType::Repeater: {
      const bool loader = n.type == WidgetType::Loader, repeater = n.type == WidgetType::Repeater;
      const int count = std::clamp(p.repeatCount, 1, kMaxRepeaterInstances);
      ImGuiChildFlags flags = ImGuiChildFlags_AlwaysUseWindowPadding;
      if (p.border) flags |= ImGuiChildFlags_Borders;
      if (size.y == 0) flags |= ImGuiChildFlags_AutoResizeY;
      if (beginChild("template", size, flags)) {
        const auto& title = !repeater && !p.sourceComponent.empty() ? p.sourceComponent : n.label;
        if (loader) {
          ImGui::BeginDisabled(designMode_);
          if (ImGui::Button(p.loaderActive ? "Descargar###toggle_loader" : "Cargar###toggle_loader"))
            p.loaderActive = !p.loaderActive;
          observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0);
          ImGui::EndDisabled();
          if (!title.empty()) ImGui::SameLine();
        }
        if (!title.empty()) ImGui::TextUnformatted(title.c_str());
        if (repeater) ImGui::TextDisabled("%d instancias", count);
        const bool active = !loader || p.loaderActive;
        const bool hasChildren = std::any_of(n.children.begin(), n.children.end(),
          [](const ImGuiNode& child) { return child.visible; });
        if (!active) ImGui::TextDisabled("%s", surfaceText(StudioText::ContentUnloaded));
        else if (!hasChildren) {
          if (designMode_) ImGui::TextDisabled("%s", surfaceText(StudioText::DropIntoContainer));
        } else if (repeater) {
          const float gap = std::clamp(p.spacing, 0.0f, 48.0f) * scale_;
          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, gap));
          render(n.children);
          if (count > 1) {
            std::string labels;
            for (const auto& child : n.children) if (child.visible) {
              if (!labels.empty()) labels += " / ";
              labels += child.label.empty() ? widgetTypeId(child.type) : child.label;
            }
            for (int instance = 2; instance <= std::min(count, 4); ++instance) {
              ImGui::Separator();
              ImGui::TextDisabled("Instancia %d", instance);
              ImGui::TextWrapped("%s", labels.c_str());
            }
          }
          ImGui::PopStyleVar();
        } else render(n.children);
      }
      ImGui::EndChild();
      break;
    }
    case WidgetType::CollapsingHeader: case WidgetType::TreeNode: {
      if (inserted.second || openChanged) ImGui::SetNextItemOpen(p.defaultOpen, ImGuiCond_Always);
      const bool open = n.type == WidgetType::TreeNode ? ImGui::TreeNode(label.c_str()) : ImGui::CollapsingHeader(label.c_str());
      observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); record = false;
      if (open) { children(); if (n.type == WidgetType::TreeNode) ImGui::TreePop(); }
      break;
    }
    case WidgetType::TabBar:
      if (ImGui::BeginTabBar(label.c_str())) {
        bool first = true;
        for (const auto& child : n.children) if (child.visible) {
          renderNode(child, inserted.second && first); first = false;
        }
        ImGui::EndTabBar();
      }
      record = false; break;
    case WidgetType::TabItem: {
      const bool open = ImGui::BeginTabItem(label.c_str(), nullptr, selectTab ? ImGuiTabItemFlags_SetSelected : 0);
      observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
      if (open) {
        children(); ImGui::EndTabItem();
      }
      record = false; break;
    }
    case WidgetType::Group: case WidgetType::ColumnLayout: case WidgetType::RowLayout: {
      ImGui::BeginGroup();
      const float gap = std::max(0.0f, p.spacing) * scale_;
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, gap));
      bool first = true;
      for (const auto& child : n.children) if (child.visible) {
        if (!first && n.type == WidgetType::RowLayout) ImGui::SameLine();
        renderNode(child); first = false;
      }
      if (designMode_ && first) emptyContainer();
      ImGui::PopStyleVar(); ImGui::EndGroup(); break;
    }
    case WidgetType::PositionerFlow: {
      ImGuiChildFlags flags = p.border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None;
      if (size.y == 0) flags |= ImGuiChildFlags_AutoResizeY;
      if (beginChild("flow", size, flags)) {
        const float gap = std::max(0.0f, p.spacing) * scale_;
        const float right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, gap));
        std::optional<float> previousRight;
        bool rendered = false;
        for (const auto& child : n.children) {
          if (!child.visible || child.type == WidgetType::SameLine) continue;
          if (child.useCursorPos) {
            renderNode(child);
            previousRight.reset();
          } else {
            // Auto-sized controls are measured after submission. On the first
            // frame give each unknown width a line so it can measure at full width.
            const auto width = flowWidths_.find(child.id);
            if (previousRight && child.width >= 0 && width != flowWidths_.end() &&
                *previousRight + gap + width->second <= right)
              ImGui::SameLine(0, gap);
            const float start = ImGui::GetCursorScreenPos().x;
            ImGui::BeginGroup();
            renderNode(child, false, true);
            ImGui::EndGroup();
            previousRight = ImGui::GetItemRectMax().x;
            flowWidths_[child.id] = std::max(0.0f, *previousRight - start);
          }
          rendered = true;
        }
        if (designMode_ && !rendered) ImGui::Dummy(ImVec2(0, 36.0f * scale_));
        ImGui::PopStyleVar();
      }
      ImGui::EndChild();
      break;
    }
    case WidgetType::Table: case WidgetType::GridLayout: {
      const int count = std::clamp(p.columns, 1, kPreviewMaxColumns);
      if (count != p.columns) report_.limitations.push_back(n.id + ": " + surfaceText(StudioText::PreviewTableLimit));
      const bool table = n.type == WidgetType::Table;
      ImGuiTableFlags flags = table ? ImGuiTableFlags_RowBg | (p.border ? ImGuiTableFlags_Borders : 0) : 0;
      if (n.height > 0) flags |= ImGuiTableFlags_ScrollY;
      ImVec2 outerSize = size;
      if (designMode_ && !table && outerSize.y == 0 &&
          std::none_of(n.children.begin(), n.children.end(), [](const ImGuiNode& child) { return child.visible; }))
        outerSize.y = ImGui::GetFrameHeight();
      const float padding = std::max(0.0f, p.spacing) * 0.5f * scale_;
      if (!table) ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(padding, padding));
      const bool submitted = ImGui::BeginTable(label.c_str(), count, flags, outerSize);
      if (submitted) {
        if (designMode_) containerDrawList = ImGui::GetWindowDrawList();
        if (table) {
          for (int i = 0; i < count; ++i) {
            const std::string title = static_cast<size_t>(i) < p.columnLabels.size() ? p.columnLabels[static_cast<size_t>(i)] : "Columna " + std::to_string(i + 1);
            ImGui::TableSetupColumn(title.c_str());
          }
          ImGui::TableHeadersRow();
        }
        for (const auto& child : n.children) if (child.visible) { ImGui::TableNextColumn(); renderNode(child); }
        ImGui::EndTable();
      }
      if (!table) ImGui::PopStyleVar();
      record = submitted; break;
    }
    case WidgetType::MenuBar:
      if (p.menuBarEdge == MenuBarEdge::Top && ImGui::BeginMenuBar()) {
        renderMenuLabels(p);
        ImGui::EndMenuBar();
      } else {
        ImGui::TextDisabled("%s", surfaceText(StudioText::MenuPreviewHint));
        report_.limitations.push_back(n.id + ": " + surfaceText(StudioText::MenuPreviewHint));
      }
      record = false; break;
    default:
      ImGui::TextDisabled("Sin preview nativa: %s", widgetTypeId(n.type));
      report_.limitations.push_back(n.id + ": " + widgetTypeId(n.type));
      children();
      record = false; break;
  }
  if (groupObservation) { ImGui::EndGroup(); record = true; }
  if (record) observe(n, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), -1, resizeWidth, containerDrawList);
  if (activated) report_.activatedIds.push_back(n.id);
  if (n.indent > 0) ImGui::Unindent(n.indent * scale_);
  ImGui::PopID();
  if (disabled) { ImGui::EndDisabled(); ImGui::PopStyleVar(); }
  parents_.pop_back();
}

bool PlotSeriesEditor::render(ImGuiNode& node) {
  auto& values = node.props.plotValues;
  if (draft_ && (draft_->nodeId != node.id || draft_->baseline != values)) reset();
  ImGui::TextDisabled("%zu valores", values.size());
  if (!draft_) {
    if (!ImGui::Button(surfaceLabel(StudioText::EditValues).c_str())) return false;
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << std::setprecision(std::numeric_limits<float>::max_digits10);
    for (size_t i = 0; i < values.size(); ++i) {
      if (i != 0) text << (i % 8 == 0 ? ",\n" : ", ");
      text << values[i];
    }
    draft_ = Draft{node.id, values, text.str(), {}};
    return false;
  }

  ImGui::TextWrapped("%s", surfaceText(StudioText::ValueInputHint));
  if (ImGui::InputTextMultiline("##PlotSeries", &draft_->text, ImVec2(-FLT_MIN, 110))) draft_->error.clear();
  if (ImGui::Button(surfaceLabel(StudioText::CancelValues).c_str())) {
    reset();
    return false;
  }
  ImGui::SameLine();
  if (ImGui::Button(surfaceLabel(StudioText::ApplyValues).c_str())) {
    auto input = draft_->text;
    std::replace(input.begin(), input.end(), ',', ' ');
    std::istringstream tokens(std::move(input)), number;
    tokens.imbue(std::locale::classic());
    number.imbue(std::locale::classic());
    std::vector<float> parsed;
    for (std::string token; tokens >> token;) {
      number.clear();
      number.str(token);
      float value = 0;
      if (!(number >> value) || number.peek() != std::char_traits<char>::eof() ||
          !std::isfinite(value)) {
        draft_->error = "El valor " + std::to_string(parsed.size() + 1) + " no es un numero valido.";
        break;
      }
      parsed.push_back(value);
    }
    if (draft_->error.empty()) {
      const bool changed = parsed != values;
      values = std::move(parsed);
      reset();
      return changed;
    }
  }
  if (!draft_->error.empty()) ImGui::TextWrapped("%s", draft_->error.c_str());
  return false;
}

void renderTitlePosition(WidgetProps& p) {
  titleCombo(surfaceLabel(StudioText::Position).c_str(), p.titleBarSide, kTitleSides, {surfaceText(StudioText::Left), surfaceText(StudioText::Center), surfaceText(StudioText::Right), surfaceText(StudioText::Free)});
  if (p.titleBarSide == "free") {
    finiteInput("X en la barra", p.titleBarX);
    bool centered = !p.titleBarY;
    if (ImGui::Checkbox(surfaceLabel(StudioText::CenterVertically).c_str(), &centered)) p.titleBarY = centered ? std::nullopt : std::optional<float>(0);
    if (p.titleBarY) finiteInput("Y en la barra", *p.titleBarY);
  }
}

void applyWindowChromePreset(WidgetProps& p, const WindowChrome::Preset& preset) {
  auto& a = p.windowAppearance;
  const auto readColor = [](const char* value) -> std::optional<Color4> {
    if (!value) return std::nullopt;
    return ProjectJson::colorFromHex(value);
  };
  a.preset = preset.id;
  p.windowControlsVariant = preset.variant;
  p.windowControlsSide = preset.side;
  p.showCollapseArrow = false;
  p.showMinimizeButton = preset.minimize;
  p.showMaximizeButton = preset.maximize;
  p.closable = true;
  p.titleAlign = preset.alignment;
  a.titleHeight = preset.titleHeight;
  a.titleSeparator = preset.separator;
  a.shadow = preset.shadow;
  a.titleIcon = preset.icon;
  a.background = readColor(preset.background);
  a.titleBackground = readColor(preset.titleBackground);
  a.borderColor = readColor(preset.border);
  a.textColor = readColor(preset.text);
  a.separatorColor = readColor(preset.separatorColor);
  a.rounding = preset.background ? std::optional<float>(preset.rounding) : std::nullopt;
  a.borderSize = preset.background ? std::optional<float>(preset.borderSize) : std::nullopt;
}

namespace {
void editOptionalNumber(const char* label, std::optional<float>& value, float fallback, float lo, float hi,
                        const char* format = "%.0f") {
  ImGui::PushID(label);
  bool enabled = value.has_value();
  if (ImGui::Checkbox(label, &enabled)) value = enabled ? std::optional<float>(fallback) : std::nullopt;
  if (value) ImGui::SliderFloat("##valor", &*value, lo, hi, format, ImGuiSliderFlags_AlwaysClamp);
  ImGui::PopID();
}

void editOptionalColor(const char* label, std::optional<Color4>& value, ImGuiCol themeColor) {
  ImGui::PushID(label);
  bool enabled = value.has_value();
  if (ImGui::Checkbox(label, &enabled)) {
    const auto& c = ImGui::GetStyle().Colors[themeColor];
    value = enabled ? std::optional<Color4>(Color4{c.x, c.y, c.z, c.w}) : std::nullopt;
  }
  if (value) ImGui::ColorEdit4("##color", &value->r, ImGuiColorEditFlags_AlphaBar);
  ImGui::PopID();
}

void editWidgetColor(ImGuiNode& node, const char* label, const char* key, ImGuiCol themeColor) {
  auto& source = node.props.source;
  auto color = widgetColor(source, key);
  const auto original = color ? ProjectJson::colorHex(*color) : std::string{};
  editOptionalColor(label, color, themeColor);
  const auto edited = color ? ProjectJson::colorHex(*color) : std::string{};
  if (edited == original) return;
  if (source.is_null()) source = ProjectJson::propsJson(node.props, node.type);
  if (color) source[key] = edited;
  else source.erase(key);
}

bool renderWindowAppearance(WidgetProps& p) {
  auto& a = p.windowAppearance;
  bool presetChanged = false;
  const auto found = std::find_if(std::begin(WindowChrome::presets), std::end(WindowChrome::presets),
    [&](const WindowChrome::Preset& preset) { return a.preset == preset.id; });
  const char* current = found == std::end(WindowChrome::presets) ? "Personalizado" : found->name;
  if (ImGui::BeginCombo(surfaceLabel(StudioText::VisualPreset).c_str(), current)) {
    for (const auto& preset : WindowChrome::presets) {
      if (ImGui::Selectable(preset.name, a.preset == preset.id)) {
        applyWindowChromePreset(p, preset);
        presetChanged = true;
      }
    }
    ImGui::EndCombo();
  }
  if (ImGui::CollapsingHeader(surfaceLabel(StudioText::WindowAppearance).c_str())) {
    titleCombo(surfaceLabel(StudioText::WindowIcon).c_str(), a.titleIcon, kWindowTitleIcons,
      {"Ninguno", "Bot", "Aplicación", "Terminal", "Destellos", "Carpeta", "Capas"});
    titleCombo(surfaceLabel(StudioText::Shadow).c_str(), a.shadow, kWindowShadows, {"Ninguna", "Suave", "Profunda", "Resplandor"});
    ImGui::Checkbox(surfaceLabel(StudioText::TransparentTitle).c_str(), &a.transparentTitle);
    ImGui::Checkbox(surfaceLabel(StudioText::TitleSeparator).c_str(), &a.titleSeparator);
    ImGui::Checkbox(surfaceLabel(StudioText::NoWindowBackground).c_str(), &p.noBackground);
    ImGui::Checkbox(surfaceLabel(StudioText::ResizeGrip).c_str(), &a.showResizeGrip);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", surfaceText(StudioText::ResizeGripHint));
    editOptionalNumber(surfaceLabel(StudioText::OwnTitleHeight).c_str(), a.titleHeight, 28, 20, 72);
    editOptionalNumber(surfaceLabel(StudioText::TitleLeftMargin).c_str(), a.titleOffsetLeft, 16, 0, 4096);
    editOptionalNumber(surfaceLabel(StudioText::TitleRightMargin).c_str(), a.titleOffsetRight, 16, 0, 4096);
    editOptionalNumber(surfaceLabel(StudioText::CustomRounding).c_str(), a.rounding, 6, 0, 72);
    editOptionalNumber(surfaceLabel(StudioText::CustomBorderWidth).c_str(), a.borderSize, 1, 0, 12, "%.1f");
    editOptionalNumber(surfaceLabel(StudioText::CustomPadX).c_str(), a.paddingX, 8, 0, 256);
    editOptionalNumber(surfaceLabel(StudioText::CustomPadY).c_str(), a.paddingY, 8, 0, 256);
    editOptionalNumber(surfaceLabel(StudioText::CustomOpacity).c_str(), a.alpha, 1, 0, 1, "%.2f");
    editOptionalColor(surfaceLabel(StudioText::WindowBgColor).c_str(), a.background, ImGuiCol_WindowBg);
    editOptionalColor(surfaceLabel(StudioText::TitleBgColor).c_str(), a.titleBackground, ImGuiCol_TitleBgActive);
    editOptionalColor(surfaceLabel(StudioText::FrameBorderColor).c_str(), a.borderColor, ImGuiCol_Border);
    editOptionalColor(surfaceLabel(StudioText::OwnTextColor).c_str(), a.textColor, ImGuiCol_Text);
    if (a.titleSeparator) editOptionalColor(surfaceLabel(StudioText::OwnSeparator).c_str(), a.separatorColor, ImGuiCol_Border);
  }
  return presetChanged;
}
} // namespace

bool renderWidgetProperties(ImGuiNode& node, PlotSeriesEditor& plotEditor) {
  auto& p = node.props;
  const bool chart2d = node.type == WidgetType::Chart2d;
  const bool chart3d = node.type == WidgetType::Chart3d;
  bool presetChanged = false;
  ImGui::PushID(node.id.c_str());
  if (node.type != WidgetType::Window && !NativePreview::supports(node.type))
    ImGui::TextWrapped("%s", surfaceText(StudioText::WidgetUnsupported));
  const bool text = node.type == WidgetType::Text || node.type == WidgetType::TextColored ||
    node.type == WidgetType::TextWrapped || node.type == WidgetType::BulletText || node.type == WidgetType::HelpMarker;
  if (text || node.type == WidgetType::InputText || node.type == WidgetType::InputMultiline) {
    ImGui::InputTextMultiline(surfaceLabel(StudioText::WidgetContent).c_str(), &p.text, ImVec2(-FLT_MIN, 90));
    if (!text) ImGui::InputText(surfaceLabel(StudioText::Placeholder).c_str(), &p.hint);
  }
  if (node.type == WidgetType::Checkbox || node.type == WidgetType::Toggle)
    ImGui::Checkbox(surfaceLabel(StudioText::InitialValue).c_str(), &p.checked);
  if (isStatusWidget(node.type)) ImGui::TextWrapped("%s", surfaceText(StudioText::BuiltInStatusUnsupported));
  if (node.type == WidgetType::Spinner && finiteInput(surfaceLabel(StudioText::SpinnerSpeed).c_str(), p.speed))
    p.speed = std::clamp(p.speed, 0.0f, 10.0f);
  if (node.type == WidgetType::Window) {
    ImGui::SeparatorText(surfaceText(StudioText::TitleBar));
    presetChanged = renderWindowAppearance(p);
    ImGui::Checkbox(surfaceLabel(StudioText::ShowTitle).c_str(), &p.showTitleText);
    ImGui::Checkbox(surfaceLabel(StudioText::CollapseArrow).c_str(), &p.showCollapseArrow);
    ImGui::Checkbox(surfaceLabel(StudioText::PreventCollapse).c_str(), &p.noCollapse);
    ImGui::Checkbox(surfaceLabel(StudioText::MinimizeButton).c_str(), &p.showMinimizeButton);
    ImGui::Checkbox(surfaceLabel(StudioText::MaximizeButton).c_str(), &p.showMaximizeButton);
    ImGui::Checkbox(surfaceLabel(StudioText::CloseButton).c_str(), &p.closable);
    titleCombo(surfaceLabel(StudioText::ControlsSide).c_str(), p.windowControlsSide, kWindowControlSides, {surfaceText(StudioText::Left), surfaceText(StudioText::Center), surfaceText(StudioText::Right)}, 2);
    titleCombo(surfaceLabel(StudioText::ControlsStyle).c_str(), p.windowControlsVariant, kWindowControlVariants, {"Iconos", "Semáforo", "Píldoras", "Clásico"});
    titleCombo(surfaceLabel(StudioText::TitleAlign).c_str(), p.titleAlign, kTitleAlignments, {surfaceText(StudioText::Left), surfaceText(StudioText::Center), surfaceText(StudioText::Right)});
  }
  if (node.type == WidgetType::TitleBarButton) {
    ImGui::SeparatorText(surfaceText(StudioText::TitleBar));
    renderTitlePosition(p);
    titleCombo(surfaceLabel(StudioText::Style).c_str(), p.titleBarBtnVariant, kTitleVariants, {"Icono", "Píldora", "Contorno", "Sólido"}, 1);
    titleCombo(surfaceLabel(StudioText::Icon).c_str(), p.titleBarIcon, kTitleIcons, {"Ninguno", "Panel izquierdo", "Panel derecho", "Destellos",
      "Editar", "Añadir", "Ajustes", "Buscar", "Terminal", "Bot", "Fijar", "Campana"});
    float rounding = p.titleBarBtnRounding.value_or(p.titleBarBtnVariant == "pill" ? 999.0f : 6.0f);
    if (finiteInput("Redondeo", rounding)) p.titleBarBtnRounding = std::max(0.0f, rounding);
  }
  if (node.type != WidgetType::Window && ImGui::CollapsingHeader(surfaceLabel(StudioText::WidgetColors).c_str())) {
    for (const auto& slot : widgetColors)
      if (showsWidgetColor(node.type, slot.use)) editWidgetColor(node, surfaceLabel(slot.label).c_str(), slot.key, slot.color);
  }
  if (node.type == WidgetType::Selectable) ImGui::Checkbox(surfaceLabel(StudioText::Selected).c_str(), &p.selected);
  if (ProjectJson::integerWidget(node.type)) {
    ImGui::InputInt(surfaceLabel(StudioText::InitialValue).c_str(), &p.intValue);
    if (node.type != WidgetType::InputInt) {
      ImGui::InputInt(surfaceLabel(StudioText::Minimum).c_str(), &p.minInt); ImGui::InputInt(surfaceLabel(StudioText::Maximum).c_str(), &p.maxInt);
    }
  }
  if (node.type == WidgetType::RadioButton) {
    if (ImGui::InputInt(surfaceLabel(StudioText::RadioInitial).c_str(), &p.intValue)) p.floatValue = static_cast<float>(p.intValue);
    ImGui::InputInt(surfaceLabel(StudioText::RadioValue).c_str(), &p.radioValue);
    ImGui::TextWrapped("%s", surfaceText(StudioText::RadioHint));
  }
  if (floatControl(node.type)) {
    if (vectorControl(node.type)) {
      const size_t count = node.type == WidgetType::SliderFloat2 ? 2 : 3;
      std::array<float, 3> values{};
      std::copy_n(p.defaultVec.begin(), std::min(p.defaultVec.size(), count), values.begin());
      const bool changed = count == 2 ? ImGui::InputFloat2(surfaceLabel(StudioText::InitialVector).c_str(), values.data()) : ImGui::InputFloat3(surfaceLabel(StudioText::InitialVector).c_str(), values.data());
      if (changed && std::all_of(values.begin(), values.end(), [](float v) { return std::isfinite(v); })) {
        if (p.defaultVec.size() < count) p.defaultVec.resize(count, 0);
        std::copy_n(values.begin(), count, p.defaultVec.begin());
      }
    } else finiteInput(node.type == WidgetType::SliderAngle ? "Valor inicial (radianes)" : "Valor inicial", p.floatValue);
    if (node.type != WidgetType::InputFloat) {
      finiteInput(node.type == WidgetType::SliderAngle ? "Minimo (grados)" : "Minimo", p.minFloat);
      finiteInput(node.type == WidgetType::SliderAngle ? "Maximo (grados)" : "Maximo", p.maxFloat);
    }
  }
  if (floatControl(node.type) || ProjectJson::integerWidget(node.type)) {
    if (node.type == WidgetType::InputInt || node.type == WidgetType::InputFloat ||
        node.type == WidgetType::DragInt || node.type == WidgetType::DragFloat || node.type == WidgetType::DragFloat3 ||
        node.type == WidgetType::Knob)
      finiteInput("Paso / velocidad", p.speed);
    ImGui::InputText(surfaceLabel(StudioText::NumericFormat).c_str(), &p.format);
    if (!p.format.empty() && !safeNumericFormat(p.format, ProjectJson::integerWidget(node.type)))
      ImGui::TextWrapped("%s", surfaceText(StudioText::NumericFormatHint));
  }
  if (node.type == WidgetType::ColorEdit3 || node.type == WidgetType::ColorEdit4 ||
      node.type == WidgetType::ColorButton || node.type == WidgetType::TextColored || chart2d || chart3d)
    ImGui::ColorEdit4(surfaceLabel(StudioText::Color).c_str(), &p.color.r);
  if (chart2d) {
    const auto selected = Chart2d::modeIndex(p.chart2dType);
    if (ImGui::BeginCombo(surfaceLabel(StudioText::ChartType).c_str(), Chart2d::modes[selected].label)) {
      for (const auto& mode : Chart2d::modes)
        if (ImGui::Selectable(mode.label, p.chart2dType == mode.id)) p.chart2dType = mode.id;
      ImGui::EndCombo();
    }
  }
  if (chart3d) {
    const auto selected = Chart3d::modeIndex(p.chart3dType);
    if (ImGui::BeginCombo(surfaceLabel(StudioText::ChartType).c_str(), Chart3d::modes[selected].label)) {
      for (const auto& mode : Chart3d::modes)
        if (ImGui::Selectable(mode.label, p.chart3dType == mode.id)) p.chart3dType = mode.id;
      ImGui::EndCombo();
    }
    ImGui::SliderFloat(surfaceLabel(StudioText::Elevation).c_str(), &p.chart3dRotX, 5, 80, "%.0f°", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(surfaceLabel(StudioText::Azimuth).c_str(), &p.chart3dRotY, -180, 180, "%.0f°", ImGuiSliderFlags_AlwaysClamp);
    if (p.chart3dType == "surface") ImGui::TextDisabled("%s", surfaceText(StudioText::SurfaceHint));
  }
  if (node.type == WidgetType::ArrowButton) {
    const char* directions[] = {"left", "right", "up", "down"};
    int index = 1;
    for (int i = 0; i < 4; ++i) if (p.arrowDir == directions[i]) index = i;
    if (ImGui::Combo(surfaceLabel(StudioText::Direction).c_str(), &index, directions, 4)) p.arrowDir = directions[index];
  }
  const bool model = isModelView(node.type);
  if (itemList(node.type) || model || node.type == WidgetType::MenuBar || chart2d) {
    auto& labels = chart2d ? p.chartLabels : p.items;
    ImGui::TextUnformatted(chart2d ? "Etiquetas de los datos" : "Elementos");
    std::optional<size_t> remove;
    for (size_t i = 0; i < labels.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));
      ImGui::SetNextItemWidth(160);
      ImGui::InputText("##item", &labels[i]); ImGui::SameLine();
      if (ImGui::SmallButton("X")) remove = i;
      ImGui::PopID();
    }
    if (remove) {
      labels.erase(labels.begin() + static_cast<std::ptrdiff_t>(*remove));
      if (!chart2d) p.selectedIndex = std::clamp(p.selectedIndex, 0, std::max(0, static_cast<int>(labels.size()) - 1));
    }
    if (ImGui::Button(chart2d ? "Agregar etiqueta" : "Agregar elemento"))
      labels.emplace_back(chart2d ? "Etiqueta" : "Nuevo elemento");
    if (itemList(node.type) || model) ImGui::InputInt(surfaceLabel(StudioText::InitialIndex).c_str(), &p.selectedIndex);
  }
  if (node.type == WidgetType::ProgressBar || node.type == WidgetType::CircularProgress)
    ImGui::SliderFloat(surfaceLabel(StudioText::Fraction).c_str(), &p.progress, 0, 1);
  if (node.type == WidgetType::ProgressBar || node.type == WidgetType::CircularProgress || plotWidget(node.type))
    ImGui::InputText(surfaceLabel(StudioText::OverlayText).c_str(), &p.overlay);
  const bool plot = plotWidget(node.type);
  if (plot && !chart3d) {
    if (chart2d) ImGui::Checkbox(surfaceLabel(StudioText::AutoMinimum).c_str(), &p.chartAutoMin);
    if (!chart2d || !p.chartAutoMin) finiteInput("Escala minima", p.minFloat);
    if (chart2d) ImGui::Checkbox(surfaceLabel(StudioText::AutoMaximum).c_str(), &p.chartAutoMax);
    if (!chart2d || !p.chartAutoMax) finiteInput("Escala maxima", p.maxFloat);
  }
  if (node.type == WidgetType::Table || node.type == WidgetType::GridLayout || node.type == WidgetType::GridView) {
    int count = p.columns;
    if (ImGui::SliderInt(surfaceLabel(StudioText::Columns).c_str(), &count, 1, model ? kMaxModelColumns : kPreviewMaxColumns)) {
      p.columns = count;
      if (node.type == WidgetType::Table) p.columnLabels.resize(static_cast<size_t>(count), "Columna");
    }
    if (node.type == WidgetType::Table) for (int i = 0; i < std::clamp(p.columns, 1, kPreviewMaxColumns); ++i) {
      std::string title = static_cast<size_t>(i) < p.columnLabels.size() ? p.columnLabels[static_cast<size_t>(i)] : "Columna " + std::to_string(i + 1);
      ImGui::PushID(i);
      if (ImGui::InputText(surfaceLabel(StudioText::Title).c_str(), &title)) {
        if (p.columnLabels.size() < static_cast<size_t>(p.columns)) p.columnLabels.resize(static_cast<size_t>(p.columns), "Columna");
        p.columnLabels[static_cast<size_t>(i)] = title;
      }
      ImGui::PopID();
    }
  }
  if (node.type == WidgetType::ChildWindow || node.type == WidgetType::Table || node.type == WidgetType::PositionerFlow ||
      model || isTemplateContainer(node.type))
    ImGui::Checkbox(surfaceLabel(StudioText::Border).c_str(), &p.border);
  if (node.type == WidgetType::Loader || node.type == WidgetType::Component)
    ImGui::InputText(node.type == WidgetType::Loader ? "Componente origen" : "Nombre del componente", &p.sourceComponent);
  if (node.type == WidgetType::Loader) ImGui::Checkbox(surfaceLabel(StudioText::InitiallyLoaded).c_str(), &p.loaderActive);
  if (node.type == WidgetType::Animation) {
    animationCombo("Tipo de animación", animationEffects, p.animType, PulseAlpha);
    double duration = p.animDuration;
    const double step = 100;
    if (ImGui::InputDouble(surfaceLabel(StudioText::Duration).c_str(), &duration, step, 0, "%.2f") && std::isfinite(duration))
      p.animDuration = std::max(minAnimationDuration, duration);
    animationCombo("Curva", animationCurves, p.animEasing, 1);
    ImGui::Checkbox(surfaceLabel(StudioText::Loop).c_str(), &p.animLoop);
    ImGui::Checkbox(surfaceLabel(StudioText::InitiallyActive).c_str(), &p.animRunning);
  }
  if (node.type == WidgetType::Repeater) {
    ImGui::SliderInt(surfaceLabel(StudioText::Instances).c_str(), &p.repeatCount, 1, kMaxRepeaterInstances);
    ImGui::SliderFloat(surfaceLabel(StudioText::InstanceSpacing).c_str(), &p.spacing, 0, 48);
  }
  if (node.type == WidgetType::CollapsingHeader || node.type == WidgetType::TreeNode) ImGui::Checkbox(surfaceLabel(StudioText::InitiallyOpen).c_str(), &p.defaultOpen);
  if (node.type == WidgetType::RowLayout || node.type == WidgetType::ColumnLayout || node.type == WidgetType::GridLayout ||
      node.type == WidgetType::PositionerFlow)
    finiteInput("Espaciado", p.spacing);
  if (node.type == WidgetType::GridView) ImGui::SliderFloat(surfaceLabel(StudioText::CellSpacing).c_str(), &p.spacing, 0.0f, 32.0f);
  const bool seriesChanged = plot && plotEditor.render(node);
  if (!plot) plotEditor.reset();
  ImGui::PopID();
  return seriesChanged || presetChanged;
}

} // namespace ImGuiStudio
