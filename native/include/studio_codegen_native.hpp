#pragma once
#include "studio_serialization.hpp"
#include "studio_codegen_animation.hpp"
#include "studio_codegen_chart2d.hpp"
#include "studio_codegen_chart3d.hpp"
#include "studio_codegen_image.hpp"
#include "studio_codegen_titlebar.hpp"
#include "studio_image_data.hpp"
#include "studio_numeric_format.hpp"
#include "studio_knob.hpp"
#include <iomanip>
#include <locale>
#include <set>

namespace ImGuiStudio {
enum class TargetLanguage { Cpp, CSharp, Rust, Java, Python, C };
enum class CppEmitMode { Header, Function, Full };
struct CppDiagnostic { std::string nodeId, message; };
struct GeneratedCpp {
  std::string code, stateType, error;
  std::vector<CppDiagnostic> diagnostics;
  std::map<std::string, std::string> variables;
  std::map<std::string, std::pair<size_t, size_t>> nodeLines;
  bool hasImageTextures = false;
  bool hasTitleButtons = false;
  bool hasKnobs = false;
  bool hasStatusWidgets = false;
  bool complete() const { return error.empty() && diagnostics.empty(); }
};
inline std::string sanitizeIdent(const std::string& raw, const std::string& fallback = "item") {
  std::string s;
  for (unsigned char c : raw) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) s += static_cast<char>(c);
    else if ((c == '_' || c == ' ' || c == '-') && !s.empty() && s.back() != '_') s += '_';
  }
  while (!s.empty() && s.back() == '_') s.pop_back();
  if (s.empty()) s = fallback;
  if (s[0] >= '0' && s[0] <= '9') s = "value_" + s;
  static const std::set<std::string> keywords = {"alignas","alignof","and","and_eq","asm","auto","bitand","bitor","bool","break","case","catch","char","char16_t","char32_t","class","compl","const","constexpr","const_cast","continue","decltype","default","delete","do","double","dynamic_cast","else","enum","explicit","export","extern","false","float","for","friend","goto","if","inline","int","long","mutable","namespace","new","noexcept","not","not_eq","nullptr","operator","or","or_eq","private","protected","public","register","reinterpret_cast","return","short","signed","sizeof","static","static_assert","static_cast","struct","switch","template","this","thread_local","throw","true","try","typedef","typeid","typename","union","unsigned","using","virtual","void","volatile","wchar_t","while","xor","xor_eq","min","max"};
  if (keywords.count(s)) s = "value_" + s;
  return s;
}
inline std::string escapeString(const std::string& text) {
  std::string out;
  for (unsigned char c : text) {
    if (c == '\\') out += "\\\\"; else if (c == '"') out += "\\\"";
    else if (c == '?') out += "\\?"; else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r"; else if (c == '\t') out += "\\t";
    else if (c < 32 || c == 127) { char b[5]; std::snprintf(b, sizeof(b), "\\%03o", static_cast<unsigned>(c)); out += b; }
    else out += static_cast<char>(c);
  }
  return out;
}
namespace CppCodegen {
inline std::string quote(const std::string& s) { return "\"" + escapeString(s) + "\""; }
inline std::string number(float v) {
  if (!std::isfinite(v)) throw std::runtime_error("Non-finite C++ value");
  std::ostringstream out; out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<float>::max_digits10) << v;
  auto s = out.str(); if (s.find_first_of(".eE") == std::string::npos) s += ".0";
  return s + "f";
}
inline std::string boolean(bool v) { return v ? "true" : "false"; }
inline std::string color(const Color4& c) { return "ImVec4(" + number(c.r) + ", " + number(c.g) + ", " + number(c.b) + ", " + number(c.a) + ")"; }
inline std::string comment(std::string text) {
  for (auto& c : text) if (static_cast<unsigned char>(c) < 32 || c == '\\') c = ' ';
  return text;
}
inline std::string hexId(const std::string& s) {
  std::string out; const char* digits = "0123456789abcdef";
  for (unsigned char c : s) { out += digits[c >> 4]; out += digits[c & 15]; }
  return out;
}
inline bool hasVisibleChildren(const ImGuiNode& node) {
  return std::any_of(node.children.begin(), node.children.end(), [](const ImGuiNode& child) { return child.visible; });
}

inline int titleOption(const std::string& value, std::initializer_list<std::string_view> choices, int fallback = 0) {
  const auto item = std::find(choices.begin(), choices.end(), value);
  return item == choices.end() ? fallback : static_cast<int>(item - choices.begin());
}

inline int titleKind(const ImGuiNode& node) {
  switch (node.type) {
    case WidgetType::Button: return 5;
    case WidgetType::InputText: return 6;
    case WidgetType::Checkbox: return 7;
    case WidgetType::Text: case WidgetType::TextColored: return 8;
    case WidgetType::ArrowButton: return 9;
    case WidgetType::SmallButton: return 10;
    case WidgetType::MenuBar: return 11;
    default: return 0;
  }
}

struct TitleWidgetColor {
  const char* key;
  const char* native;
  const char* python;
};
inline constexpr TitleWidgetColor titleWidgetColors[] = {
  {"buttonHoveredColorOverride", "ButtonHovered", "button_hovered"},
  {"buttonActiveColorOverride", "ButtonActive", "button_active"},
  {"frameBgOverride", "FrameBg", "frame_bg"},
  {"frameBgHoveredOverride", "FrameBgHovered", "frame_bg_hovered"},
  {"frameBgActiveOverride", "FrameBgActive", "frame_bg_active"},
  {"headerColorOverride", "Header", "header"},
  {"headerHoveredColorOverride", "HeaderHovered", "header_hovered"},
  {"headerActiveColorOverride", "HeaderActive", "header_active"},
  {"textSelectedBgOverride", "TextSelectedBg", "text_selected_bg"},
  {"checkMarkColorOverride", "CheckMark", "check_mark"},
};

inline bool hasWindowModes(const WidgetProps& p) { return p.showMinimizeButton || p.showMaximizeButton; }

inline bool hidesTitle(const ImGuiNode& window) {
  const auto flags = ProjectJson::strings(ProjectJson::propsJson(window.props, window.type), "windowFlags", {});
  return window.props.noTitleBar || std::any_of(flags.begin(), flags.end(), [](const std::string& flag) {
    return flag == "ImGuiWindowFlags_NoDecoration" || flag == "NoDecoration" || flag == "NoTitleBar";
  });
}

template<class Emit>
void emitNodeFunctions(const std::vector<ImGuiNode>& roots, Emit emit) {
  std::vector<std::pair<const ImGuiNode*, bool>> pending;
  const auto pushVisible = [&](const std::vector<ImGuiNode>& nodes, bool flowChild) {
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
      if (it->visible) pending.emplace_back(&*it, flowChild);
  };
  pushVisible(roots, false);
  while (!pending.empty()) {
    const auto [node, flowChild] = pending.back();
    pending.pop_back();
    emit(*node, flowChild);
    pushVisible(node->children, node->type == WidgetType::PositionerFlow);
  }
}

class Emitter {
 public:
  explicit Emitter(bool cApi = false) : cApi(cApi) {}
  const bool cApi;
  GeneratedCpp result;
  std::vector<std::string> lines, declarations, cFields, cInitializers;
  // Adapt only emitter-owned API tokens. Project literals are copied verbatim.
  static std::string cLine(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size();) {
      if (text[i] == '"') {
        out += text[i++];
        while (i < text.size()) {
          const char c = text[i++];
          out += c;
          if (c == '\\' && i < text.size()) out += text[i++];
          else if (c == '"') break;
        }
      } else if (text.compare(i,7,"ImGui::") == 0) { out += "ig"; i += 7; }
      else if (text.compare(i,7,"ImVec2(") == 0) { out += "STUDIO_VEC2("; i += 7; }
      else if (text.compare(i,7,"ImVec4(") == 0) { out += "STUDIO_VEC4("; i += 7; }
      else out += text[i++];
    }
    return out;
  }
  std::set<std::string> used;
  std::map<std::string, std::string> radios;
  bool hasModelRows = false, hasModelPaths = false;
  bool hasRepeater = false;
  bool hasChart2d = false;
  bool hasChart3d = false;
  bool hasAnimation = false;
  bool hasImageWidgets = false;
  std::map<std::string_view, size_t> imageIndex;
  std::map<std::string, size_t> imageNodes;
  std::vector<std::string_view> imageAssets;
  std::vector<std::string> frameButtons;
  std::vector<const ImGuiNode*> titleCallbacks;
  std::map<std::string, std::string> windowModes;
  std::string animationScale;
  std::string layout(float value) const {
    return animationScale.empty() || value == 0 ? number(value) : number(value) + " * " + animationScale;
  }
  std::string width(float value) const { return value < 0 ? "-FLT_MIN" : layout(value); }
  std::string size(const ImGuiNode& n) const {
    return "ImVec2(" + width(n.width) + ", " + layout(std::max(0.0f, n.height)) + ")";
  }
  void repeatBudget() {
    if (hasRepeater) {
      line("    // Bound the expansion of nested repeaters in each frame.");
      line("    int studio_repeat_budget = " + std::to_string(ProjectJson::kMaxNodes) + ";");
      line("    (void)studio_repeat_budget;");
    }
  }
  void line(const std::string& text = {}) {
    if (text.empty()) { lines.emplace_back(); return; }
    std::istringstream input(text); std::string part;
    while (std::getline(input, part)) lines.push_back(cApi ? cLine(part) : part);
  }
  void warn(const ImGuiNode& n, const std::string& message) {
    for (const auto& d : result.diagnostics) if (d.nodeId == n.id && d.message == message) return;
    result.diagnostics.push_back({n.id, message});
  }
  std::string alloc(const ImGuiNode& n) {
    const auto base = (cApi ? "field_" : "") + sanitizeIdent(n.varName, "value"); auto name = base;
    for (int i = 2; used.count(name); ++i) name = base + "_" + std::to_string(i);
    used.insert(name); result.variables[n.id] = name; return name;
  }
  void declare(const std::string& type,const std::string& name,const std::string& value,
               const std::string& suffix = {}) {
    if (cApi) {
      cFields.push_back(type+" "+name+suffix+";");
      cInitializers.push_back("."+name+" = "+value+",");
    } else {
      declarations.push_back(type+" "+name+" = "+value+";");
    }
  }
  void collect(const std::vector<ImGuiNode>& nodes) {
    for (const auto& n : nodes) {
      if (!n.visible) continue;
      const auto& p = n.props;
      if (n.type == WidgetType::Window &&
          ((!hidesTitle(n) && WindowChrome::usesCustomTitle(n)) ||
           titleOption(p.windowAppearance.shadow, {"none", "soft", "deep", "glow"}) != 0)) {
        result.hasTitleButtons = true;
        for (const auto& child : n.children)
          if (!hidesTitle(n) && child.visible && WindowChrome::isTitleItem(child) && titleKind(child))
            titleCallbacks.push_back(&child);
      }
      if (n.type == WidgetType::Repeater && hasVisibleChildren(n)) hasRepeater = true;
      if (n.type == WidgetType::Loader) {
        declare("bool", alloc(n), boolean(p.loaderActive));
      } else if (n.type == WidgetType::Animation) {
        hasAnimation = true;
        declare("struct { bool running; double start; }", alloc(n), "{" + boolean(p.animRunning) + ", -1.0}");
      } else if (n.type == WidgetType::TitleBarButton) {
        result.hasTitleButtons = true;
        frameButtons.push_back(alloc(n));
        declare("bool", frameButtons.back(), "false");
      } else if (n.type == WidgetType::Knob && !cApi) {
        result.hasKnobs = true;
        declare("float", alloc(n), number(knobSettings(p).value));
      } else if (isStatusWidget(n.type) && !cApi) {
        result.hasStatusWidgets = true;
        const auto name = alloc(n);
        if (n.type == WidgetType::Toggle) declare("bool", name, boolean(p.checked));
        else if (n.type == WidgetType::Spinner) declare("float", name, number(std::clamp(p.speed, 0.0f, 10.0f)));
        else declare("float", name, number(std::clamp(p.progress, 0.0f, 1.0f)));
      } else if (isImageWidget(n.type)) {
        hasImageWidgets = true;
        if (n.type == WidgetType::ImageButton) {
          frameButtons.push_back(alloc(n));
          declare("bool", frameButtons.back(), "false");
        }
        if (p.imageAsset) {
          const auto* encoded = ImageData::encoded(*p.imageAsset);
          const auto existing = encoded ? imageIndex.find(*encoded) : imageIndex.end();
          if (existing != imageIndex.end()) imageNodes.emplace(n.id, existing->second);
          else if (encoded && !ImageData::decode(*encoded).empty()) {
            imageNodes.emplace(n.id, imageAssets.size());
            imageIndex.emplace(*encoded, imageAssets.size());
            imageAssets.emplace_back(*encoded);
            result.hasImageTextures = true;
          } else warn(n, "Datos de imagen no válidos. Se genera un marcador.");
        }
      } else if (n.type == WidgetType::RadioButton && radios.count(n.varName)) {
        result.variables[n.id] = radios[n.varName];
      } else if (n.type == WidgetType::Window || n.type == WidgetType::Checkbox || n.type == WidgetType::Selectable) {
        const auto name = alloc(n);
        declare("bool", name, boolean(n.type == WidgetType::Window || (n.type == WidgetType::Checkbox ? p.checked : p.selected)));
        if (n.type == WidgetType::Window && !hidesTitle(n) && hasWindowModes(p)) {
          auto mode = name + "_window_mode";
          while (!used.insert(mode).second) mode += "_";
          windowModes.emplace(n.id, mode);
          declare(std::string(cApi ? "" : "StudioTitleBarExport::") + "StudioWindowMode",
            mode, "{0, false, {0, 0}, {0, 0}}");
        }
      } else if (ProjectJson::integerWidget(n.type) || n.type == WidgetType::RadioButton || n.type == WidgetType::Combo || n.type == WidgetType::Listbox || isModelView(n.type)) {
        const auto name = alloc(n);
        const bool choice = n.type == WidgetType::Combo || n.type == WidgetType::Listbox || isModelView(n.type);
        declare("int",name,std::to_string(choice ? p.selectedIndex : p.intValue));
        if (!p.items.empty()) {
          hasModelRows |= n.type == WidgetType::ListView || n.type == WidgetType::GridView;
          hasModelPaths |= n.type == WidgetType::PathView;
        }
        if (n.type == WidgetType::RadioButton) radios[n.varName] = name;
      } else if (n.type == WidgetType::InputFloat || n.type == WidgetType::SliderFloat || n.type == WidgetType::SliderAngle || n.type == WidgetType::DragFloat) {
        declare("float",alloc(n),number(p.floatValue));
      } else if (n.type == WidgetType::InputText || n.type == WidgetType::InputMultiline) {
        const auto name = alloc(n);
        if (cApi) {
          declare("char",name,quote(p.text),"["+std::to_string(std::max<size_t>(4096,p.text.size()+1024))+"]");
          if (p.text.find('\0') != std::string::npos) warn(n,"El campo de texto muestra hasta el primer NUL");
        } else {
          declare("std::string",name,"std::string("+quote(p.text)+", "+std::to_string(p.text.size())+")");
        }
      } else if (n.type == WidgetType::SliderFloat2 || n.type == WidgetType::SliderFloat3 || n.type == WidgetType::DragFloat3 ||
                 n.type == WidgetType::ColorEdit3 || n.type == WidgetType::ColorEdit4 || n.type == WidgetType::PlotLines ||
                 n.type == WidgetType::PlotHistogram || n.type == WidgetType::Chart2d || n.type == WidgetType::Chart3d) {
        std::vector<float> values;
        if (n.type == WidgetType::Chart2d) hasChart2d = true;
        if (n.type == WidgetType::Chart3d) hasChart3d = true;
        if (n.type == WidgetType::ColorEdit3 || n.type == WidgetType::ColorEdit4) values = {p.color.r,p.color.g,p.color.b,p.color.a};
        else if (n.type == WidgetType::PlotLines || n.type == WidgetType::PlotHistogram ||
                 n.type == WidgetType::Chart2d || n.type == WidgetType::Chart3d)
          values = p.plotValues;
        else {
          const size_t count = n.type == WidgetType::SliderFloat2 ? 2 : 3; values = p.defaultVec;
          if (values.size() > count) warn(n, "El control usa sus primeras " + std::to_string(count) + " componentes; el proyecto conserva las restantes");
          values.resize(count, 0);
        }
        const auto name = alloc(n);
        const auto count = std::to_string(cApi ? std::max<size_t>(1,values.size()) : values.size());
        std::string initializer = "{";
        for (size_t i = 0; i < values.size(); ++i) initializer += (i ? ", " : "") + number(values[i]);
        if (cApi && values.empty()) initializer += "0";
        initializer += "}";
        if (n.type == WidgetType::Chart3d) {
          const auto series = cApi ? "float values["+count+"]" : "std::array<float, "+count+"> values";
          declare("struct { "+series+"; float elevation; float azimuth; }", name,
                  "{"+initializer+", "+number(p.chart3dRotX)+", "+number(p.chart3dRotY)+"}");
        } else if (cApi) declare("float",name,initializer,"["+count+"]");
        else declare("std::array<float, "+count+">",name,initializer);
      }
      collect(n.children);
    }
  }
  std::string format(const ImGuiNode& n, bool integer = false, const std::string& fallback = "%.3f") {
    if (!n.props.format.empty() && safeNumericFormat(n.props.format, integer)) return quote(n.props.format);
    if (!n.props.format.empty()) warn(n, "Formato numerico incompatible; se exporta el predeterminado");
    return quote(integer ? "%d" : fallback);
  }
  void style(const StudioProject& project) {
    const auto& s = project.style;
    line(cApi ? "static inline void ApplyStudioStyle(void) {" : "inline void ApplyStudioStyle() {");
    const std::string preset = s.preset == StylePresetId::ImGuiLight ? "Light" : s.preset == StylePresetId::ImGuiClassic ? "Classic" : "Dark";
    line("    ImGui::StyleColors"+preset+(cApi ? "(NULL);" : "();"));
    line(cApi ? "    ImGuiStyle* style = ImGui::GetStyle();" : "    ImGuiStyle& style = ImGui::GetStyle();");
    const std::string field = cApi ? "    style->" : "    style.";
    const auto j = ProjectJson::styleJson(s);
    line(field + "WindowRounding = " + number(s.windowRounding) + ";");
    line(field + "FrameRounding = " + number(s.frameRounding) + ";");
    line(field + "WindowPadding = ImVec2(" + number(s.windowPaddingX) + ", " + number(s.windowPaddingY) + ");");
    line(field + "ItemSpacing = ImVec2(" + number(s.itemSpacingX) + ", " + number(s.itemSpacingY) + ");");
    const std::pair<const char*,const char*> numeric[] = {{"childRounding","ChildRounding"},{"grabRounding","GrabRounding"},{"tabRounding","TabRounding"},{"scrollbarRounding","ScrollbarRounding"},{"windowBorderSize","WindowBorderSize"},{"frameBorderSize","FrameBorderSize"},{"alpha","Alpha"},{"framePaddingX","FramePadding.x"},{"framePaddingY","FramePadding.y"}};
    for (const auto& item : numeric) if (j.contains(item.first) && j.at(item.first).is_number()) line(field + std::string(item.second) + " = " + number(j.at(item.first).get<float>()) + ";");
    const std::pair<const char*,const char*> colors[] = {{"childBg","ChildBg"},{"titleBg","TitleBg"},{"frameBgHovered","FrameBgHovered"},{"buttonHovered","ButtonHovered"},{"buttonActive","ButtonActive"},{"checkMark","CheckMark"},{"sliderGrab","SliderGrab"},{"header","Header"},{"headerHovered","HeaderHovered"},{"textDisabled","TextDisabled"},{"border","Border"},{"separator","Separator"}};
    for (const auto& item : colors) if (j.contains("colors") && j.at("colors").contains(item.first)) {
      try { line(field + "Colors[ImGuiCol_" + std::string(item.second) + "] = " + color(ProjectJson::colorFromHex(j.at("colors").at(item.first).get<std::string>())) + ";"); }
      catch (const std::exception&) { result.diagnostics.push_back({{}, std::string("Color de estilo incompatible: ") + item.first}); }
    }
    line(field + "Colors[ImGuiCol_WindowBg] = " + color(s.windowBg) + ";");
    line(field + "Colors[ImGuiCol_TitleBgActive] = " + color(s.titleBg) + ";");
    line(field + "Colors[ImGuiCol_Button] = " + color(s.buttonColor) + ";");
    line(field + "Colors[ImGuiCol_FrameBg] = " + color(s.frameBg) + ";");
    line(field + "Colors[ImGuiCol_Text] = " + color(s.textColor) + ";"); line("}");
  }
  void modelFunctions() {
    if (!hasModelRows && !hasModelPaths) return;
    const std::string fn = cApi ? "static inline " : "inline ";
    line(fn + "void StudioDrawModelLabel(const char* text, bool centered, bool featured) {");
    if (cApi) {
      line("    ImVec2 lo, hi, extent; ImGui::GetItemRectMin(&lo); ImGui::GetItemRectMax(&hi);");
      line("    ImGui::CalcTextSize(&extent, text, NULL, false, -1); const ImGuiStyle* style = ImGui::GetStyle();");
      line("    const ImVec2 padding = style->FramePadding;");
    } else {
      line("    const ImVec2 lo = ImGui::GetItemRectMin(), hi = ImGui::GetItemRectMax(), extent = ImGui::CalcTextSize(text);");
      line("    const ImGuiStyle& style = ImGui::GetStyle(); const ImVec2 padding = style.FramePadding;");
    }
    line(R"MODEL(    const float cx = (hi.x - lo.x - extent.x) * 0.5f;
    const float cy = (hi.y - lo.y - extent.y) * 0.5f;
    const ImVec2 pos = ImVec2(lo.x + (centered && cx > padding.x ? cx : padding.x), lo.y + (cy > 0 ? cy : 0));
    const ImVec2 clip_lo = ImVec2(lo.x + padding.x, lo.y), clip_hi = ImVec2(hi.x - padding.x, hi.y);
    ImDrawList* draw = ImGui::GetWindowDrawList();)MODEL");
    if (cApi) {
      line("    if (featured) ImDrawList_AddRectFilled(draw, lo, hi, ImGui::GetColorU32_Col(ImGuiCol_Header, 1), style->FrameRounding, 0);");
      line("    ImDrawList_PushClipRect(draw, clip_lo, clip_hi, true);");
      line("    ImDrawList_AddText_Vec2(draw, pos, ImGui::GetColorU32_Col(ImGuiCol_Text, 1), text, NULL);");
      line("    ImDrawList_PopClipRect(draw);");
    } else {
      line("    if (featured) draw->AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_Header), style.FrameRounding);");
      line("    draw->PushClipRect(clip_lo, clip_hi, true);");
      line("    draw->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), text);");
      line("    draw->PopClipRect();");
    }
    line("}");

    if (hasModelRows) modelRowsFunction(fn);
    if (hasModelPaths) modelPathFunction(fn);
  }

  void chartFunctions() {
    if (!hasChart2d && !hasChart3d) return;
    line("#include <stdio.h>");
    if (hasChart3d) line("#include <math.h>\n#include <stdlib.h>");
    if (!cApi) line("namespace StudioChartDetails {");
    if (hasChart2d) {
      line("enum StudioChartMode {");
      for (const auto& mode : Chart2d::modes) line(std::string("    ") + mode.symbol + ",");
      line("};");
    }
    if (hasChart3d) {
      line("enum StudioChart3dMode {");
      for (const auto& mode : Chart3d::modes) line(std::string("    ") + mode.symbol + ",");
      line("};");
    }
    line(cApi ? cChartBindings : cppChartBindings);
    line(chartSharedFunctions);
    if (hasChart2d) line(chart2dFunctions);
    if (hasChart3d) line(chart3dFunctions);
    line(chartBindingCleanup);
    if (!cApi) line("} // namespace StudioChartDetails");
  }

  void emitAnimationFunctions() {
    if (!hasAnimation) return;
    line("#include <math.h>");
    if (!cApi) line("namespace StudioAnimationDetails {");
    line(cApi ? cAnimationBindings : cppAnimationBindings);
    line(animationFunctions);
    if (!cApi) line("} // namespace StudioAnimationDetails");
  }

  void emitImageState() {
    if (!result.hasImageTextures) return;
    const auto count = std::to_string(imageAssets.size());
    declare(cApi ? "StudioImageTexture" : "std::array<StudioImageTexture, " + count + ">",
            "studio_image_textures", cApi ? "{{0, 0, 0}}" : "{}", cApi ? "[" + count + "]" : "");
  }

  void emitImageFunctions() {
    if (!hasImageWidgets) return;
    line(cApi ? cImageBindings : cppImageBindings);
    line(imageDrawing);
    if (result.hasImageTextures) {
      line("#include <stdlib.h>\n#include <string.h>");
      line(imageLoader);
      const std::string access = cApi ? "state->studio_image_textures" : "state.studio_image_textures";
      const auto stateParameter = result.stateType + (cApi ? "* state" : "& state");
      line("STUDIO_IMAGE_FN bool LoadStudioImages(" + stateParameter + ", StudioImageUpload upload, void* user) {");
      line("    if (!upload) return false;");
      for (size_t index = 0; index < imageAssets.size(); ++index) {
        const auto image = imageAssets[index];
        const auto texture = access + "[" + std::to_string(index) + "]";
        line("    if (!" + texture + ".id) {");
        line("        static const char encoded[][97] = {");
        for (size_t offset = 0; offset < image.size(); offset += imageBlockSize)
          line("            \"" + std::string(image.substr(offset, imageBlockSize)) + "\",");
        line("        };");
        line("        " + texture + " = StudioLoadImageBlocks(encoded, " + std::to_string(image.size()) + ", upload, user);");
        line("    }");
      }
      const auto count = std::to_string(imageAssets.size());
      line("    for (size_t i = 0; i < " + count + "; ++i) if (!" + access + "[i].id) return false;");
      line("    return true;\n}");
      line("STUDIO_IMAGE_FN void ReleaseStudioImages(" + stateParameter + ", StudioImageRelease release, void* user) {");
      line("    if (!release) return;");
      line("    for (size_t i = 0; i < " + count + "; ++i) {");
      line("        StudioImageTexture* texture = &" + access + "[i];");
      line("        if (texture->id) release(*texture, user);");
      line("        texture->id = 0; texture->width = 0; texture->height = 0;");
      line("    }\n}");
    }
    line("#undef STUDIO_IMAGE_FN");
  }

  void resetFrameButtons() {
    for (const auto& fieldName : frameButtons)
      line(std::string(cApi ? "    state->" : "    state.") + fieldName + " = false;");
  }

  std::string titleScope() const { return cApi ? "" : "StudioTitleBarExport::"; }
  static std::string titleCallback(const ImGuiNode& n) { return "StudioTitleWidget_" + hexId(n.id); }

  void emitTitleCallbacks() {
    for (const auto* child : titleCallbacks) {
      line("static inline void " + titleCallback(*child) + "(void* data, float title_width, float title_height) {");
      line(cApi ? "    " + result.stateType + "* state = (" + result.stateType + "*)data;" :
        "    auto& state = *static_cast<" + result.stateType + "*>(data);");
      line("    (void)state; (void)title_width; (void)title_height;");
      emit(*child, 1, false, true);
      line("}");
    }
  }

  void emitTitleButtons(const ImGuiNode& window, const std::string& in, bool show, const std::string& state,
                        bool collapse, const std::string& mode) {
    const auto& p = window.props;
    const auto& a = p.windowAppearance;
    const auto scope = titleScope();
    const std::string emptyColors = "{0, 0, 0, 0}, {0, 0, 0, 0}";
    const int side = titleOption(p.windowControlsSide, {"left", "center", "right"}, 2);
    const int variant = titleOption(p.windowControlsVariant, {"icons", "traffic_lights", "pills", "classic"});
    const bool inset = WindowChrome::hasTitleInsets(a);
    const bool insetCollapse = show && inset && collapse;
    const bool hasItems = show && (p.closable || hasWindowModes(p) || insetCollapse ||
      std::any_of(window.children.begin(), window.children.end(), [](const ImGuiNode& child) {
        return child.visible && WindowChrome::isTitleItem(child);
      }));
    if (show) {
      if (p.closable) line(in + "bool studio_close_pressed = false;");
      if (p.showMinimizeButton) line(in + "bool studio_minimize_pressed = false;");
      if (p.showMaximizeButton) line(in + "bool studio_maximize_pressed = false;");
      if (insetCollapse) line(in + "bool studio_collapse_pressed = false;");
      if (hasItems) line(in + "const " + scope + "StudioTitleButton studio_title_buttons[] = {");
    }
    const auto control = [&](const char* name, int kind, int group) {
      line(in + "    {\"##studio_window_" + name + "\", \"\", &studio_" + name + "_pressed, " +
        std::to_string(group) + ", 0, 0, 0, 0, 0, 0, 0, false, false, false, " +
        emptyColors + ", " + std::to_string(kind) + ", NULL, false, false, " + emptyColors + "},");
    };
    const auto controls = [&] {
      if (variant == 1 && p.closable) control("close", 1, side);
      if (p.showMinimizeButton) control("minimize", 2, side);
      if (p.showMaximizeButton) control("maximize", 3, side);
      if (variant != 1 && p.closable) control("close", 1, side);
    };
    if (insetCollapse) control("collapse", 4, 0);
    if (show && side != 2) controls();
    for (const auto& child : window.children) {
      if (!child.visible || !WindowChrome::isTitleItem(child)) continue;
      const int kind = titleKind(child);
      const auto firstLine = lines.size() + 1;
      if (!show) line(in + "// Title item hidden by the window flags.");
      else {
        const auto& item = child.props;
        const int group = titleOption(item.titleBarSide, {"left", "center", "right", "free"});
        const int appearance = titleOption(item.titleBarBtnVariant, {"icon", "pill", "outline", "solid"}, 1);
        const int icon = kind ? 0 : titleOption(item.titleBarIcon, {"none", "panel_left", "panel_right",
          "sparkles", "edit", "plus", "settings", "search", "terminal", "bot", "pin", "bell"});
        const auto overrideColor = [&](const char* name) -> std::optional<Color4> {
          if (!item.source.is_object() || !item.source.contains(name) || !item.source.at(name).is_string()) return std::nullopt;
          try { return ProjectJson::colorFromHex(item.source.at(name).get<std::string>()); }
          catch (const std::exception&) { warn(child, std::string("Color incompatible: ") + name); return std::nullopt; }
        };
        const auto bg = kind ? std::optional<Color4>{} : overrideColor("buttonColorOverride");
        const auto fg = kind ? std::optional<Color4>{} : overrideColor("textColorOverride");
        const auto hover = kind ? std::optional<Color4>{} : overrideColor("buttonHoveredColorOverride");
        const auto active = kind ? std::optional<Color4>{} : overrideColor("buttonActiveColorOverride");
        const auto tint = [&](const std::optional<Color4>& value) { return value ? color(*value) : "{0, 0, 0, 0}"; };
        const auto clicked = kind ? "NULL" : std::string(cApi ? "&state->" : "&state.") + result.variables.at(child.id);
        std::string width = number(child.width);
        if (kind == 11 && child.width <= 0) {
          width = std::string(cApi ? "ImGui::GetStyle()->" : "ImGui::GetStyle().") + "ItemSpacing.x * 2.0f";
          for (const auto& caption : item.items) width += " + " + scope + "StudioTitleMenuEntryWidth(" + quote(caption) + ")";
          if (item.items.empty()) width += " + " + scope + "StudioTitleMenuEntryWidth(\"Menú vacío\")";
        }
        line(in + "    { " + quote("##studio_title_" + hexId(child.id)) + ", " + quote(kind == 8 ? item.text : child.label) +
          ", " + clicked + ", " + std::to_string(group) + ", " + std::to_string(appearance) + ", " + std::to_string(icon) +
          ", " + width + ", " + number(child.height) + ", " + number(item.titleBarX) +
          ", " + number(item.titleBarY.value_or(0)) + ", " + number(item.titleBarBtnRounding.value_or(appearance == 1 ? 999.0f : 6.0f)) +
          ", " + boolean(item.titleBarY.has_value()) + ", " + boolean(bg.has_value()) + ", " + boolean(fg.has_value()) +
          ", " + tint(bg) + ", " + tint(fg) + ", " + std::to_string(kind) + ", " + (kind ? titleCallback(child) : "NULL") +
          ", " + boolean(hover.has_value()) + ", " + boolean(active.has_value()) + ", " + tint(hover) + ", " + tint(active) + " },");
        if (!kind && !item.buttonAction.empty()) warn(child, "La accion de boton requiere integracion de la aplicacion");
      }
      if (!result.nodeLines.count(child.id)) result.nodeLines[child.id] = {firstLine, lines.size()};
    }
    if (!show) return;
    if (side == 2) controls();
    if (hasItems) line(in + "};");
    const int titleIconIds[] = {0, 9, 12, 8, 3, 13, 14};
    const int icon = titleIconIds[titleOption(a.titleIcon, {"none", "bot", "app", "terminal", "sparkles", "folder", "layers"})];
    const int alignment = titleOption(p.titleAlign, {"left", "center", "right"});
    const std::string colorSlot = "(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ? ImGuiCol_TitleBgActive : "
      "ImGui::IsWindowCollapsed() ? ImGuiCol_TitleBgCollapsed : ImGuiCol_TitleBg)";
    line(in + "const " + scope + "StudioTitleAppearance studio_title_appearance = {" +
      number(std::clamp(a.titleOffsetLeft.value_or(0), 0.0f, 4096.0f)) + ", " +
      number(std::clamp(a.titleOffsetRight.value_or(0), 0.0f, 4096.0f)) + ", " + number(alignment * 0.5f) +
      ", " + std::to_string(icon) + ", " + std::to_string(variant) + ", " + boolean(a.titleSeparator) +
      ", " + boolean(a.separatorColor.has_value()) + ", " + boolean(a.transparentTitle) +
      ", " + (a.separatorColor ? color(*a.separatorColor) : "{0, 0, 0, 0}") +
      ", " + scope + "StudioTitleColor" + colorSlot + "};");
    line(in + scope + "StudioRenderTitleBar(" + quote(p.showTitleText ? window.label : "") + ", " +
      (hasItems ? "studio_title_buttons, sizeof(studio_title_buttons) / sizeof(studio_title_buttons[0])" : "NULL, 0") +
      ", false, studio_title_appearance, " + (cApi ? "(void*)state" : "(void*)&state") + ", " +
      (mode.empty() ? "0" : mode + ".mode") + ");");
    if (p.closable) line(in + "if (studio_close_pressed) " + state + " = false;");
    if (p.showMinimizeButton) line(in + "if (studio_minimize_pressed) " + scope + "StudioSetWindowMode(&" + mode + ", 1);");
    if (p.showMaximizeButton) line(in + "if (studio_maximize_pressed) " + scope + "StudioSetWindowMode(&" + mode + ", 2);");
  }


  void emitAnimation(const ImGuiNode& n, const std::string& state, const std::string& in, int depth) {
    const auto& p = n.props;
    const std::pair<const char*, const char*> effects[] = {
      {"pulse_alpha", "StudioPulseAlpha"}, {"bounce_y", "StudioBounceY"}, {"slide_x", "StudioSlideX"},
      {"scale_pulse", "StudioScalePulse"}, {"color_shift", "StudioColorShift"}
    };
    const std::pair<const char*, const char*> curves[] = {
      {"sine", "StudioSine"}, {"linear", "StudioLinear"},
      {"ease_in_out", "StudioEaseInOut"}, {"bounce", "StudioBounce"}
    };
    const auto select = [](const auto& options, const std::string& value) {
      for (const auto& option : options) if (value == option.first) return option.second;
      return options[0].second;
    };
    const auto effect = select(effects, p.animType);
    const auto curve = select(curves, p.animEasing);
    std::ostringstream duration;
    duration.imbue(std::locale::classic());
    duration << std::setprecision(std::numeric_limits<double>::max_digits10)
             << (std::isfinite(p.animDuration) ? std::max(200.0, p.animDuration) / 1000.0 : 1.4);
    const auto scale = animationScale.empty() ? "1.0f" : animationScale;
    const std::string ns = cApi ? "" : "StudioAnimationDetails::";
    line(in + ns + "StudioAnimationScope studio_animation;");
    line(in + "if (" + ns + "StudioBeginAnimation(&studio_animation, &" + state + ".running, &" + state + ".start, " +
         quote(n.label) + ", " + size(n) + ", " + ns + effect + ", " + ns + curve + ", " + duration.str() + ", " +
         boolean(p.animLoop) + ", " + scale + ")) {");
    line(in + "    if (studio_animation.content_visible) {");
    const auto oldScale = animationScale;
    if (p.animType == "scale_pulse" && hasVisibleChildren(n)) {
      animationScale = "studio_animation_scale_" + std::to_string(depth);
      line(in + "        const float " + animationScale + " = " + scale + " * studio_animation.pulse_scale;");
      line(in + "        (void)" + animationScale + ";");
    }
    if (hasVisibleChildren(n)) {
      for (const auto& child : n.children) emit(child, depth + 3);
    } else {
      line(in + "        ImGui::TextUnformatted(\"Vista previa de animación\"" + (cApi ? ", NULL);" : ");"));
    }
    animationScale = oldScale;
    line(in + "    }");
    line(in + "    " + ns + "StudioEndAnimationContent(&studio_animation);");
    line(in + "}\n" + in + "ImGui::EndChild();");
  }

  void modelRowsFunction(const std::string& fn) {
    line(fn + "void StudioRenderModelRows(const char* const* items, int count, int* value, int columns, bool grid) {");
    line(R"MODEL(    const int selected = *value < 0 ? 0 : (*value >= count ? count - 1 : *value);
    const int rows = (count + columns - 1) / columns;
    const float height = ImGui::GetFrameHeight() * (grid ? 1.5f : 1.0f);)MODEL");
    if (cApi) {
      line("    ImGuiListClipper* clip = ImGuiListClipper_ImGuiListClipper();");
      line("    ImGuiListClipper_Begin(clip, rows, -1);");
      line("    while (ImGuiListClipper_Step(clip)) for (int row = clip->DisplayStart; row < clip->DisplayEnd; ++row) {");
      line("        if (grid) ImGui::TableNextRow(0, 0);");
    } else {
      line("    ImGuiListClipper clip; clip.Begin(rows);");
      line("    while (clip.Step()) for (int row = clip.DisplayStart; row < clip.DisplayEnd; ++row) {");
      line("        if (grid) ImGui::TableNextRow();");
    }
    line("        for (int i = row * columns; i < (row + 1) * columns && i < count; ++i) {");
    line("            if (grid) ImGui::TableNextColumn();");
    line(cApi ? "            ImGui::PushID_Int(i);" : "            ImGui::PushID(i);");
    line(std::string(cApi ? "            if (ImGui::Selectable_Bool(" : "            if (ImGui::Selectable(") +
         "\"##model_option\", i == selected, 0, ImVec2(0, height))) *value = i;");
    line("            StudioDrawModelLabel(items[i], grid, false);");
    line("            ImGui::PopID();\n        }\n    }");
    if (cApi) line("    ImGuiListClipper_destroy(clip);");
    line("}");
  }

  void modelPathFunction(const std::string& fn) {
    line(fn + "void StudioRenderModelPath(const char* const* items, int count, int* value) {");
    line(R"MODEL(    const int remainder = *value % count;
    const int selected = remainder < 0 ? remainder + count : remainder;
    ImGui::TextDisabled("%d / %d", selected + 1, count);
    const float arrow = ImGui::GetFrameHeight();)MODEL");
    if (cApi) {
      line("    ImVec2 available; ImGui::GetContentRegionAvail(&available);");
      line("    const float width = available.x - 2 * (arrow + ImGui::GetStyle()->ItemSpacing.x);");
    } else line("    const float width = ImGui::GetContentRegionAvail().x - 2 * (arrow + ImGui::GetStyle().ItemSpacing.x);");
    line(R"MODEL(    ImGui::BeginDisabled(count == 1);
    if (ImGui::ArrowButton("previous", ImGuiDir_Left)) *value = selected == 0 ? count - 1 : selected - 1;
    ImGui::EndDisabled();)MODEL");
    line(cApi ? "    ImGui::SameLine(0, -1);" : "    ImGui::SameLine();");
    line("    ImGui::Dummy(ImVec2(width > 1 ? width : 1, arrow));");
    line("    StudioDrawModelLabel(items[selected], true, true);");
    line(cApi ? "    ImGui::SameLine(0, -1);" : "    ImGui::SameLine();");
    line(R"MODEL(    ImGui::BeginDisabled(count == 1);
    if (ImGui::ArrowButton("next", ImGuiDir_Right)) *value = selected + 1 == count ? 0 : selected + 1;
    ImGui::EndDisabled();
})MODEL");
  }

  void emitModelView(const ImGuiNode& n, const std::string& state, const std::string& in) {
    const auto& p = n.props;
    const auto body = in + "    ";
    const std::string flags = std::string("ImGuiChildFlags_AlwaysUseWindowPadding") + (p.border ? " | ImGuiChildFlags_Borders" : "");
    line(in + (cApi ? "ImGui::PushStyleColor_Vec4(ImGuiCol_ChildBg, *ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));"
                   : "ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));"));
    line(in + (cApi ? "if (ImGui::BeginChild_Str(" : "if (ImGui::BeginChild(") + "\"model\", ImVec2(" +
         width(n.width) + ", " + layout(n.height > 0 ? n.height : modelViewDefaultHeight(n.type)) + "), " +
         flags + (cApi ? ", 0" : "") + ")) {");
    if (!n.label.empty()) line(body + "ImGui::TextUnformatted(" + quote(n.label) + (cApi ? ", NULL);" : ");"));
    if (p.items.empty()) line(body + "ImGui::TextDisabled(\"Sin elementos\");");
    else {
      std::string items = "static const char* const studio_items[] = {";
      for (const auto& item : p.items) items += quote(item) + ", ";
      line(body + items + "};");
      const auto args = "studio_items, " + std::to_string(p.items.size()) + ", &" + state;
      if (n.type == WidgetType::PathView) line(body + "StudioRenderModelPath(" + args + ");");
      else if (n.type == WidgetType::ListView) line(body + "StudioRenderModelRows(" + args + ", 1, false);");
      else {
        const auto columns = std::to_string(std::clamp(p.columns, 1, kMaxModelColumns));
        const auto padding = layout(std::clamp(p.spacing, 0.0f, 32.0f) * 0.5f);
        line(body + (cApi ? "ImGui::PushStyleVar_Vec2" : "ImGui::PushStyleVar") +
             "(ImGuiStyleVar_CellPadding, ImVec2(" + padding + ", " + padding + "));");
        line(body + "if (ImGui::BeginTable(\"model_cells\", " + columns + ", ImGuiTableFlags_SizingStretchSame" +
             (cApi ? ", ImVec2(0, 0), 0" : "") + ")) {");
        line(body + "    StudioRenderModelRows(" + args + ", " + columns + ", true);");
        line(body + "    ImGui::EndTable();\n" + body + "}");
        line(body + (cApi ? "ImGui::PopStyleVar(1);" : "ImGui::PopStyleVar();"));
      }
    }
    line(in + "}\n" + in + "ImGui::EndChild();\n" + in + "ImGui::PopStyleColor(1);");
  }

  void emitTemplate(const ImGuiNode& n, const std::string& state, const std::string& in, int depth) {
    const auto& p = n.props;
    const bool loader = n.type == WidgetType::Loader, repeater = n.type == WidgetType::Repeater;
    const int count = std::clamp(p.repeatCount, 1, kMaxRepeaterInstances);
    const std::string begin = cApi ? "if (ImGui::BeginChild_Str(" : "if (ImGui::BeginChild(";
    const std::string windowFlags = cApi ? ", 0" : "";
    std::string flags = "ImGuiChildFlags_AlwaysUseWindowPadding";
    if (p.border) flags += " | ImGuiChildFlags_Borders";
    if (n.height <= 0) flags += " | ImGuiChildFlags_AutoResizeY";
    line(in + begin + "\"template\", " + size(n) + ", " + flags + windowFlags + ")) {");
    const auto body = in + "    ";
    const auto& title = !repeater && !p.sourceComponent.empty() ? p.sourceComponent : n.label;
    if (loader) {
      line(body + "if (ImGui::Button(" + state + " ? \"Descargar###toggle_loader\" : \"Cargar###toggle_loader\"" +
           (cApi ? ", ImVec2(0, 0)" : "") + ")) " + state + " = !" + state + ";");
      if (!title.empty()) line(body + (cApi ? "ImGui::SameLine(0, -1);" : "ImGui::SameLine();"));
    }
    if (!title.empty()) line(body + "ImGui::TextUnformatted(" + quote(title) + (cApi ? ", NULL);" : ");"));
    if (repeater) line(body + "ImGui::TextDisabled(\"%d instancias\", " + std::to_string(count) + ");");

    if (loader) {
      line(body + "if (" + state + ") {");
      for (const auto& child : n.children) emit(child, depth + 3);
      line(body + "} else ImGui::TextDisabled(\"Contenido descargado\");");
    } else if (repeater && hasVisibleChildren(n)) {
      const auto gap = layout(std::clamp(p.spacing, 0.0f, 48.0f));
      line(body + (cApi ? "ImGui::PushStyleVar_Vec2" : "ImGui::PushStyleVar") +
           "(ImGuiStyleVar_ItemSpacing, ImVec2(" + gap + ", " + gap + "));");
      line(body + "for (int studio_instance = 0; studio_instance < " + std::to_string(count) +
           " && studio_repeat_budget > 0; ++studio_instance) {");
      const auto loop = body + "    ";
      line(loop + "--studio_repeat_budget;");
      line(loop + (cApi ? "ImGui::PushID_Int(studio_instance);" : "ImGui::PushID(studio_instance);"));
      line(loop + begin + "\"instance\", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY" + windowFlags + ")) {");
      for (const auto& child : n.children) emit(child, depth + 4);
      line(loop + "}");
      line(loop + "ImGui::EndChild();\n" + loop + "ImGui::PopID();");
      line(body + "}");
      line(body + (cApi ? "ImGui::PopStyleVar(1);" : "ImGui::PopStyleVar();"));
    } else {
      for (const auto& child : n.children) emit(child, depth + 2);
    }
    line(in + "}\n" + in + "ImGui::EndChild();");
  }

  void emit(const ImGuiNode& n, int depth, bool flowChild = false, bool titleItem = false) {
    if (!n.visible) return;
    const auto firstLine = lines.size() + 1;
    const std::string pad(static_cast<size_t>(depth) * 4, ' '), in = pad + "    ";
    if (flowChild && n.type == WidgetType::SameLine) {
      line(pad + "// Flow determines line breaks for its children.");
      result.nodeLines[n.id] = {firstLine, lines.size()};
      return;
    }
    const auto& p = n.props;
    const auto found = result.variables.find(n.id);
    const auto state = found == result.variables.end() ? std::string() : (cApi ? "state->" : "state.") + found->second;
    const auto label = quote(n.label + (n.imguiId.empty() ? "" : "##" + n.imguiId));
    line(pad + "{");
    line(in + (cApi ? "static const char" : "static constexpr char") + " studio_node_id[] = " + quote(n.id) + ";");
    line(in + std::string(cApi ? "ImGui::PushID_StrStr(" : "ImGui::PushID(") + "studio_node_id, studio_node_id + sizeof(studio_node_id) - 1);");
    if (n.type != WidgetType::Window && !titleItem) {
      if (n.useCursorPos) line(in + "ImGui::SetCursorPos(ImVec2(" + layout(n.x) + ", " + layout(n.y) + "));");
      else if (n.sameLine && !flowChild) line(in + (n.sameLineSpacing > 0 ? "ImGui::SameLine(0.0f, " + layout(n.sameLineSpacing) + ");" : (cApi ? "ImGui::SameLine(0, -1);" : "ImGui::SameLine();")));
      if (n.indent > 0) line(in + "ImGui::Indent(" + layout(n.indent) + ");");
    }
    const auto props = ProjectJson::propsJson(p, n.type); int colors = 0, vars = 0;
    const auto colorOverride = [&](const char* key, const char* slot) {
      if (!props.contains(key) || !props.at(key).is_string()) return;
      try {
        line(in + std::string(cApi ? "ImGui::PushStyleColor_Vec4(ImGuiCol_" : "ImGui::PushStyleColor(ImGuiCol_") +
          slot + ", " + color(ProjectJson::colorFromHex(props.at(key).get<std::string>())) + ");");
        ++colors;
      } catch (const std::exception&) { warn(n, std::string("Color incompatible: ") + key); }
    };
    const std::pair<const char*,const char*> overrides[] = {{"buttonColorOverride","Button"},{"textColorOverride","Text"},{"windowBgOverride","WindowBg"},{"titleBgOverride","TitleBgActive"},{"windowBorderColorOverride","Border"}};
    if (n.type != WidgetType::Window)
      for (const auto& item : overrides) colorOverride(item.first, item.second);
    if (titleItem)
      for (const auto& item : titleWidgetColors) colorOverride(item.key, item.native);
    if (!cApi && n.type == WidgetType::Toggle) {
      colorOverride("buttonHoveredColorOverride", "ButtonHovered");
      colorOverride("buttonActiveColorOverride", "ButtonActive");
    }
    if (!cApi && isRoundStatusWidget(n.type)) {
      colorOverride("frameBgOverride", "FrameBg");
      colorOverride("progressFillColorOverride", "PlotHistogram");
    }
    if (n.type != WidgetType::Window)
      for (const auto& item : {std::pair<const char*,const char*>{"windowRoundingOverride","WindowRounding"},{"windowBorderSizeOverride","WindowBorderSize"},{"windowAlphaOverride","Alpha"}})
      if (props.contains(item.first) && props.at(item.first).is_number()) { line(in + std::string(cApi ? "ImGui::PushStyleVar_Float(ImGuiStyleVar_" : "ImGui::PushStyleVar(ImGuiStyleVar_") + item.second + ", " + number(props.at(item.first).get<float>()) + ");"); ++vars; }
    if (titleItem && n.type == WidgetType::InputText) line(in + "ImGui::SetNextItemWidth(title_width);");
    if (!titleItem && !state.empty() && n.width != 0 && n.type != WidgetType::Window &&
        n.type != WidgetType::Chart2d && n.type != WidgetType::Chart3d && n.type != WidgetType::Animation &&
        n.type != WidgetType::Knob && !isStatusWidget(n.type) && !isImageWidget(n.type) &&
        !isModelView(n.type) && !isTemplateContainer(n.type))
      line(in + "ImGui::SetNextItemWidth(" + width(n.width) + ");");
    auto children = [&] { for (const auto& child : n.children) emit(child, depth + 2); };
    auto call = [&](const std::string& text) { line(in + text + ";"); };
    const std::string null = cApi ? "NULL" : "nullptr";
    const std::string flagsDefault = cApi ? ", 0" : "";
    const auto array = [&](const std::string& value) { return value + (cApi ? "" : ".data()"); };
    const auto lo = number(std::min(p.minFloat,p.maxFloat)), hi = number(std::max(p.minFloat,p.maxFloat));
    const auto il = std::to_string(std::min(p.minInt,p.maxInt)), ih = std::to_string(std::max(p.minInt,p.maxInt));
    switch (n.type) {
      case WidgetType::Image: case WidgetType::ImageButton: {
        const auto asset = imageNodes.find(n.id);
        std::string texture = "studio_no_texture";
        if (asset == imageNodes.end()) line(in + "const StudioImageTexture studio_no_texture = {0, 0, 0};");
        else texture = std::string(cApi ? "state->" : "state.") + "studio_image_textures[" + std::to_string(asset->second) + "]";
        const bool button = n.type == WidgetType::ImageButton;
        const auto scale = animationScale.empty() ? "1.0f" : animationScale;
        const auto draw = "StudioDrawImage(" + texture + ", " + quote(n.label) + ", " + size(n) + ", " +
          scale + ", " + boolean(button) + ")";
        if (button) {
          line(in + "if (" + draw + ") " + state + " = true;");
          if (!p.buttonAction.empty()) warn(n, "La accion de boton requiere integracion de la aplicacion");
        } else call(draw);
        break;
      }
      case WidgetType::Chart2d: {
        const auto& mode = Chart2d::modes[Chart2d::modeIndex(p.chart2dType)];
        if (p.chart2dType != mode.id) warn(n, "Tipo de gráfico desconocido. Se genera como línea.");
        if (!p.chartLabels.empty()) {
          line(in + "static const char* const studio_labels[] = {");
          for (const auto& text : p.chartLabels) line(in + "    " + quote(text) + ",");
          line(in + "};");
        }
        const auto labels = p.chartLabels.empty() ? null : "studio_labels";
        const std::string chartNamespace = cApi ? "" : "StudioChartDetails::";
        call(chartNamespace + "StudioDrawChart2d(" + chartNamespace + mode.symbol + ", " +
          quote(p.overlay.empty() ? mode.label : p.overlay) +
          ", " + size(n) + ", " + color(p.color) + ", " + array(state) + ", " + std::to_string(p.plotValues.size()) +
          ", " + labels + ", " + std::to_string(p.chartLabels.size()) + ", " +
          boolean(p.chartAutoMin) + ", " + number(p.minFloat) + ", " + boolean(p.chartAutoMax) + ", " +
          number(p.maxFloat) + ")");
        break;
      }
      case WidgetType::Chart3d: {
        const auto& mode = Chart3d::modes[Chart3d::modeIndex(p.chart3dType)];
        if (p.chart3dType != mode.id) warn(n, "Tipo de gráfico 3D desconocido. Se generan barras.");
        const std::string chartNamespace = cApi ? "" : "StudioChartDetails::";
        call(chartNamespace + "StudioDrawChart3d(" + chartNamespace + mode.symbol + ", " +
          quote(p.overlay.empty() ? mode.label : p.overlay) + ", " + size(n) + ", " + color(p.color) + ", " +
          array(state + ".values") + ", " + std::to_string(p.plotValues.size()) + ", &" +
          state + ".elevation, &" + state + ".azimuth)");
        break;
      }
      case WidgetType::Window: {
        const auto& a = p.windowAppearance;
        std::string flags = "ImGuiWindowFlags_None";
        const std::set<std::string> allowed = {"NoTitleBar","NoResize","NoMove","NoScrollbar","NoScrollWithMouse","NoCollapse","AlwaysAutoResize","NoBackground","NoSavedSettings","NoMouseInputs","MenuBar","HorizontalScrollbar","NoFocusOnAppearing","NoBringToFrontOnFocus","AlwaysVerticalScrollbar","AlwaysHorizontalScrollbar","NoNavInputs","NoNavFocus","UnsavedDocument","NoNav","NoDecoration","NoInputs"};
        bool collapse = p.showCollapseArrow && !p.noCollapse;
        for (const auto& flag : ProjectJson::strings(props,"windowFlags",{})) {
          const auto name = flag.rfind("ImGuiWindowFlags_",0) == 0 ? flag.substr(17) : flag;
          if (allowed.count(name)) flags += " | ImGuiWindowFlags_" + name;
          else warn(n, "Window flag no exportado: " + flag);
          collapse &= name != "NoCollapse";
        }
        if (!collapse) flags += " | ImGuiWindowFlags_NoCollapse";
        const bool titleHidden = hidesTitle(n);
        const bool customTitle = !titleHidden && WindowChrome::usesCustomTitle(n);
        const bool inset = !titleHidden && WindowChrome::hasTitleInsets(a);
        const auto modeField = windowModes.find(n.id);
        const auto mode = modeField == windowModes.end() ? std::string{} :
          std::string(cApi ? "state->" : "state.") + modeField->second;
        const auto scope = titleScope();
        const auto title = (customTitle ? std::string{} : n.label.substr(0,n.label.find('\0'))) +
          "###studio_" + hexId(n.id) + "_" + hexId(n.imguiId);
        const auto body = in + "    ";
        const std::string style = cApi ? "ImGui::GetStyle()->" : "ImGui::GetStyle().";
        const std::string pushVar = cApi ? "ImGui::PushStyleVar_Float" : "ImGui::PushStyleVar";
        const std::string pushColor = cApi ? "ImGui::PushStyleColor_Vec4" : "ImGui::PushStyleColor";
        int windowVars = 0, windowColors = 0;
        const auto variable = [&](const char* name, std::optional<float> value, float min, float max) {
          if (!value) return;
          line(body + pushVar + "(ImGuiStyleVar_" + name + ", " + number(std::clamp(*value, min, max)) + ");");
          ++windowVars;
        };
        const auto tint = [&](const char* name, const std::optional<Color4>& value) {
          if (!value) return;
          line(body + pushColor + "(ImGuiCol_" + name + ", " + color(*value) + ");");
          ++windowColors;
        };
        const auto padding = [&](char axis, std::optional<float> value) {
          if (!value) return;
          line(body + "ImGui::PushStyleVar" + axis + "(ImGuiStyleVar_WindowPadding, " +
            number(std::clamp(*value, 0.0f, 256.0f)) + ");");
          ++windowVars;
        };
        line(in + "if (" + state + ") {");
        variable("Alpha", a.alpha, 0, 1);
        variable("WindowRounding", a.rounding, 0, 72);
        variable("WindowBorderSize", p.noBackground ? std::optional<float>{0} : a.borderSize, 0, 12);
        padding('X', a.paddingX); padding('Y', a.paddingY);
        tint("WindowBg", a.background); tint("Border", a.borderColor); tint("Text", a.textColor);
        if (!a.showResizeGrip)
          for (const auto slot : {"ResizeGrip","ResizeGripHovered","ResizeGripActive"}) tint(slot, Color4{0,0,0,0});
        const auto titleTint = a.transparentTitle ? std::optional<Color4>{Color4{0,0,0,0}} : a.titleBackground;
        for (const auto slot : {"TitleBg","TitleBgActive","TitleBgCollapsed"}) tint(slot, titleTint);
        for (const auto& field : WindowChrome::colors)
          if (props.contains(field.key) && !(a.*field.member)) warn(n, std::string("Color de ventana incompatible: ") + field.key);

        if (!titleHidden && a.titleHeight)
          line(body + "ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, " +
            std::string(cApi ? "fmaxf(" : "(std::max)(") + "0.0f, (" +
            number(std::clamp(*a.titleHeight, 20.0f, 72.0f)) + " - ImGui::GetFontSize()) * 0.5f));");
        const auto position = "ImVec2(" + number(n.x) + ", " + number(n.y) + ")";
        if (!mode.empty()) {
          line(body + "const bool studio_minimized = " + mode + ".mode == 1;");
          line(body + "if (studio_minimized) ImGui::PushStyleVarY(ImGuiStyleVar_WindowMinSize, 1.0f);");
          line(body + scope + "StudioWindowGeometry(&" + mode + ", " + position + ", " + size(n) + ");");
          flags += " | (" + mode + ".mode ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0)";
        } else {
          line(body + "ImGui::SetNextWindowPos(" + position + ", ImGuiCond_FirstUseEver" + (cApi ? ", ImVec2(0, 0)" : "") + ");");
          line(body + "ImGui::SetNextWindowSize(" + size(n) + ", ImGuiCond_FirstUseEver);");
        }
        if (std::any_of(n.children.begin(), n.children.end(), [](const ImGuiNode& child) {
          return child.visible && child.type == WidgetType::MenuBar && child.props.menuBarEdge == MenuBarEdge::Top;
        })) flags += mode.empty() ? " | ImGuiWindowFlags_MenuBar" : " | (studio_minimized ? 0 : ImGuiWindowFlags_MenuBar)";
        if (inset) {
          line(body + "const ImGuiDir studio_menu_position = " + style + "WindowMenuButtonPosition;");
          line(body + style + "WindowMenuButtonPosition = ImGuiDir_None;");
          for (const auto slot : {"TitleBg","TitleBgActive","TitleBgCollapsed"})
            line(body + pushColor + "(ImGuiCol_" + slot + ", ImVec4(0, 0, 0, 0));");
        }
        line(body + "const bool studio_expanded = ImGui::Begin(" + quote(title) + ", " + null + ", " + flags + ");");
        if (inset) {
          line(body + "ImGui::PopStyleColor(3);");
          line(body + style + "WindowMenuButtonPosition = studio_menu_position;");
        }
        if (!mode.empty()) line(body + "if (studio_minimized) ImGui::PopStyleVar(1);");
        if (!titleHidden && a.titleHeight) line(body + "ImGui::PopStyleVar(1);");
        const auto shadow = titleOption(a.shadow, {"none", "soft", "deep", "glow"});
        if (shadow) line(body + scope + "StudioWindowShadow(" + std::to_string(shadow) + ");");
        emitTitleButtons(n, body, customTitle, state, collapse, mode);
        line(body + std::string(mode.empty() ? "if (studio_expanded) {" : "if (studio_expanded && !studio_minimized) {"));
        for (const auto& child : n.children) {
          if (WindowChrome::isTitleItem(child)) continue;
          if (child.visible && child.props.inTitleBar)
            warn(child, "Control no compatible con la barra de título. Se conserva en el cuerpo.");
          emit(child, depth + 3);
        }
        line(body + "}");
        line(body + "ImGui::End();");
        if (windowColors) line(body + "ImGui::PopStyleColor(" + std::to_string(windowColors) + ");");
        if (windowVars) line(body + "ImGui::PopStyleVar(" + std::to_string(windowVars) + ");");
        line(in + "}");
        break;
      }
      case WidgetType::Text: call("ImGui::TextUnformatted(" + quote(p.text) + (cApi ? ", NULL" : "") + ")"); break;
      case WidgetType::TextColored: call("ImGui::TextColored(" + color(p.color) + ", \"%s\", " + quote(p.text) + ")"); break;
      case WidgetType::TextWrapped: call("ImGui::PushTextWrapPos(0.0f)"); call("ImGui::TextUnformatted(" + quote(p.text) + (cApi ? ", NULL" : "") + ")"); call("ImGui::PopTextWrapPos()"); break;
      case WidgetType::BulletText: call("ImGui::BulletText(\"%s\", " + quote(p.text) + ")"); break;
      case WidgetType::HelpMarker:
        call("ImGui::TextDisabled(\"(?)\")");
        line(in + std::string(cApi ? "if (ImGui::BeginItemTooltip()) { " : "if (ImGui::IsItemHovered()) { ImGui::BeginTooltip(); ")+
          "ImGui::TextUnformatted(" + quote(p.text) + (cApi ? ", NULL" : "") + "); ImGui::EndTooltip(); }"); break;
      case WidgetType::Button: case WidgetType::SmallButton: case WidgetType::ArrowButton: case WidgetType::ColorButton: {
        std::string expression;
        if (n.type == WidgetType::Button) expression = "ImGui::Button(" + label + ", " +
          (titleItem ? "ImVec2(title_width, title_height)" : size(n)) + ")";
        else if (n.type == WidgetType::SmallButton) expression = "ImGui::SmallButton(" + label + ")";
        else if (n.type == WidgetType::ColorButton) expression = "ImGui::ColorButton(" + label + ", " + color(p.color) + ", 0, " + size(n) + ")";
        else expression = "ImGui::ArrowButton(" + label + ", ImGuiDir_" + (p.arrowDir == "left" ? "Left" : p.arrowDir == "up" ? "Up" : p.arrowDir == "down" ? "Down" : "Right") + ")";
        line(in + "if (" + expression + ") { /* Connect application action here. */ }");
        if (!p.buttonAction.empty()) warn(n,"La accion de boton requiere integracion de la aplicacion");
        break;
      }
      case WidgetType::Checkbox: call("ImGui::Checkbox(" + label + ", &" + state + ")"); break;
      case WidgetType::Selectable: call(std::string(cApi ? "ImGui::Selectable_BoolPtr(" : "ImGui::Selectable(") + label + ", &" + state + ", 0, " + size(n) + ")"); break;
      case WidgetType::RadioButton: call(std::string(cApi ? "ImGui::RadioButton_IntPtr(" : "ImGui::RadioButton(") + label + ", &" + state + ", " + std::to_string(p.radioValue) + ")"); break;
      case WidgetType::InputText: call("ImGui::InputTextWithHint(" + label + ", " + quote(p.hint) + ", " + (cApi ? state + ", sizeof("+state+"), 0, NULL, NULL" : "&"+state) + ")"); break;
      case WidgetType::InputMultiline: call("ImGui::InputTextMultiline(" + label + ", " + (cApi ? state + ", sizeof("+state+")" : "&"+state) + ", " + size(n) + (cApi ? ", 0, NULL, NULL" : "") + ")"); break;
      case WidgetType::InputFloat: call("ImGui::InputFloat(" + label + ", &" + state + ", " + number(p.speed) + ", " + number(p.speed <= std::numeric_limits<float>::max() / 10.0f ? p.speed * 10.0f : p.speed) + ", " + format(n) + flagsDefault + ")"); break;
      case WidgetType::InputInt: {
        const int step = static_cast<int>(std::clamp(static_cast<double>(p.speed),1.0,static_cast<double>(INT32_MAX / 100)));
        line(in + "const int studio_step = " + std::to_string(step) + ", studio_fast = " + std::to_string(step * 10) + ";");
        call("ImGui::InputScalar(" + label + ", ImGuiDataType_S32, &" + state + ", &studio_step, &studio_fast, " + format(n,true) + flagsDefault + ")"); break;
      }
      case WidgetType::SliderFloat: case WidgetType::SliderInt:
        call(std::string(n.type == WidgetType::SliderInt ? "ImGui::SliderInt(" : "ImGui::SliderFloat(") + label + ", &" + state + ", " + (n.type == WidgetType::SliderInt ? il + ", " + ih : lo + ", " + hi) + ", " + format(n,n.type == WidgetType::SliderInt) + flagsDefault + ")"); break;
      case WidgetType::SliderAngle: call("ImGui::SliderAngle(" + label + ", &" + state + ", " + lo + ", " + hi + ", " + format(n,false,"%.0f deg") + flagsDefault + ")"); break;
      case WidgetType::DragFloat: case WidgetType::DragInt:
        call(std::string(n.type == WidgetType::DragInt ? "ImGui::DragInt(" : "ImGui::DragFloat(") + label + ", &" + state + ", " + number(std::max(0.0f,p.speed)) + ", " + (n.type == WidgetType::DragInt ? il + ", " + ih : lo + ", " + hi) + ", " + format(n,n.type == WidgetType::DragInt) + flagsDefault + ")"); break;
      case WidgetType::Knob: {
        if (cApi) {
          warn(n, "imgui-knobs requiere la exportación C++.");
          call("ImGui::TextDisabled(\"%s\", " + quote(n.label + " (imgui-knobs)") + ")");
          break;
        }
        const auto dial = knobSettings(p);
        const float width = std::clamp(n.width > 0 ? n.width : 80.0f, 8.0f, 4096.0f);
        call("ImGui::BeginGroup()");
        if (!n.label.empty()) call("ImGui::TextUnformatted(" + quote(n.label) + ")");
        call("ImGuiKnobs::Knob(\"##dial\", &" + state + ", " + number(dial.minimum) + ", " +
          number(dial.maximum) + ", " + number(dial.speed) + ", " + format(n) +
          ", ImGuiKnobVariant_Wiper, " + number(width) + ", ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_AlwaysClamp)");
        call("ImGui::EndGroup()");
        break;
      }
      case WidgetType::SliderFloat2: case WidgetType::SliderFloat3: case WidgetType::DragFloat3:
        call(std::string(n.type == WidgetType::SliderFloat2 ? "ImGui::SliderFloat2(" : n.type == WidgetType::SliderFloat3 ? "ImGui::SliderFloat3(" : "ImGui::DragFloat3(") + label + ", " + array(state) + ", " + (n.type == WidgetType::DragFloat3 ? number(std::max(0.0f,p.speed)) + ", " : "") + lo + ", " + hi + ", " + format(n) + flagsDefault + ")"); break;
      case WidgetType::ColorEdit3: case WidgetType::ColorEdit4: call(std::string(n.type == WidgetType::ColorEdit3 ? "ImGui::ColorEdit3(" : "ImGui::ColorEdit4(") + label + ", " + array(state) + flagsDefault + ")"); break;
      case WidgetType::Combo: case WidgetType::Listbox: {
        if (cApi) {
          if (p.items.empty()) call("ImGui::TextDisabled(\"%s\", "+quote(n.label+" (sin opciones)")+")");
          else {
            std::string items = "static const char* const studio_items[] = {";
            for (const auto& item : p.items) items += quote(item)+", ";
            line(in + items + "};");
            call(std::string(n.type == WidgetType::Combo ? "ImGui::Combo_Str_arr(" : "ImGui::ListBox_Str_arr(")+label+
              ", &"+state+", studio_items, "+std::to_string(p.items.size())+", -1)");
          }
          break;
        }
        std::string items = "static const std::array<const char*, " + std::to_string(p.items.size()) + "> studio_items = {";
        for (size_t i = 0; i < p.items.size(); ++i) items += (i ? ", " : "") + quote(p.items[i]);
        line(in + items + "};");
        if (n.type == WidgetType::Combo) {
          line(in + "const char* studio_current = (" + state + " >= 0 && static_cast<size_t>(" + state + ") < studio_items.size()) ? studio_items[static_cast<size_t>(" + state + ")] : \"(sin seleccion)\";");
          line(in + "if (ImGui::BeginCombo(" + label + ", studio_current)) {");
        } else line(in + "if (ImGui::BeginListBox(" + label + ", " + size(n) + ")) {");
        line(in + "    for (size_t studio_i = 0; studio_i < studio_items.size(); ++studio_i) {");
        line(in + "        ImGui::PushID(static_cast<int>(studio_i));");
        line(in + "        if (ImGui::Selectable(studio_items[studio_i], " + state + " == static_cast<int>(studio_i))) " + state + " = static_cast<int>(studio_i);");
        line(in + "        ImGui::PopID();"); line(in + "    }");
        line(in + (n.type == WidgetType::Combo ? "    ImGui::EndCombo();" : "    ImGui::EndListBox();")); line(in + "}"); break;
      }
      case WidgetType::ListView: case WidgetType::GridView: case WidgetType::PathView:
        emitModelView(n, state, in); break;
      case WidgetType::ProgressBar: call("ImGui::ProgressBar(" + number(std::clamp(p.progress,0.0f,1.0f)) + ", " + size(n) + ", " + (p.overlay.empty() ? null : quote(p.overlay)) + ")"); break;
      case WidgetType::Toggle: case WidgetType::Spinner: case WidgetType::CircularProgress: {
        if (cApi) {
          warn(n, "Control visual actualmente disponible en la exportación C++.");
          call("ImGui::TextDisabled(\"%s\", " + quote(n.label) + ")");
          break;
        }
        const auto caption = quote(n.label);
        const auto width = layout(n.width);
        if (n.type == WidgetType::Toggle)
          call("StudioStatusWidgets::toggle(\"##toggle\", " + caption + ", " + state + ", " + width + ", " + layout(n.height) + ")");
        else if (n.type == WidgetType::Spinner)
          call("StudioStatusWidgets::spinner(" + caption + ", " + width + ", " + state + ")");
        else
          call("StudioStatusWidgets::circularProgress(" + caption + ", " + width + ", " + state +
            ", " + quote(p.overlay) + ")");
        break;
      }
      case WidgetType::PlotLines: case WidgetType::PlotHistogram:
        if (cApi) {
          if (p.plotValues.empty()) call("ImGui::TextDisabled(\"%s\", "+quote(n.label+" (sin datos)")+")");
          else call(std::string(n.type == WidgetType::PlotLines ? "ImGui::PlotLines_FloatPtr(" : "ImGui::PlotHistogram_FloatPtr(")+
            label+", "+state+", "+std::to_string(p.plotValues.size())+", 0, "+(p.overlay.empty()?null:quote(p.overlay))+
            ", "+lo+", "+hi+", "+size(n)+", sizeof(float))");
          break;
        }
        call(std::string(n.type == WidgetType::PlotLines ? "ImGui::PlotLines(" : "ImGui::PlotHistogram(") + label + ", " + state + ".data(), static_cast<int>(" + state + ".size()), 0, " + (p.overlay.empty() ? null : quote(p.overlay)) + ", " + lo + ", " + hi + ", " + size(n) + ")"); break;
      case WidgetType::Separator: call("ImGui::Separator()"); break;
      case WidgetType::SeparatorText: call("ImGui::SeparatorText(" + quote(n.label) + ")"); break;
      case WidgetType::Spacing: call("ImGui::Spacing()"); break;
      case WidgetType::SameLine: call(cApi ? "ImGui::SameLine(0, -1)" : "ImGui::SameLine()"); break;
      case WidgetType::Dummy:
        if (cApi && n.width < 0) {
          line(in+"ImVec2 available;");
          call("ImGui::GetContentRegionAvail(&available)");
          call("ImGui::Dummy(ImVec2(available.x, "+layout(std::max(0.0f,n.height))+"))");
          break;
        }
        call("ImGui::Dummy(ImVec2(" + (n.width < 0 ? "ImGui::GetContentRegionAvail().x" : layout(n.width)) + ", " + layout(std::max(0.0f,n.height)) + "))"); break;
      case WidgetType::ChildWindow:
        line(in + (cApi ? "if (ImGui::BeginChild_Str(" : "if (ImGui::BeginChild(") + label + ", " + size(n) + ", " + (p.border ? "ImGuiChildFlags_Borders" : "ImGuiChildFlags_None") + flagsDefault + ")) {"); children(); line(in + "}"); call("ImGui::EndChild()"); break;
      case WidgetType::Loader: case WidgetType::Component: case WidgetType::Repeater:
        emitTemplate(n, state, in, depth); break;
      case WidgetType::Animation:
        emitAnimation(n, state, in, depth); break;
      case WidgetType::Group: case WidgetType::ColumnLayout: case WidgetType::RowLayout: {
        call("ImGui::BeginGroup()"); call(std::string(cApi ? "ImGui::PushStyleVar_Vec2" : "ImGui::PushStyleVar") + "(ImGuiStyleVar_ItemSpacing, ImVec2(" + layout(std::max(0.0f,p.spacing)) + ", " + layout(std::max(0.0f,p.spacing)) + "))");
        bool first = true;
        for (const auto& child : n.children) if (child.visible) { if (!first && n.type == WidgetType::RowLayout) call(cApi ? "ImGui::SameLine(0, -1)" : "ImGui::SameLine()"); emit(child,depth + 1); first = false; }
        call(cApi ? "ImGui::PopStyleVar(1)" : "ImGui::PopStyleVar()"); call("ImGui::EndGroup()"); break;
      }
      case WidgetType::PositionerFlow: {
        std::string flags = p.border ? "ImGuiChildFlags_Borders" : "ImGuiChildFlags_None";
        if (n.height <= 0) flags += " | ImGuiChildFlags_AutoResizeY";
        const auto gap = layout(std::max(0.0f, p.spacing));
        const auto body = in + "    ";
        const auto managed = [](const ImGuiNode& child) {
          return child.visible && !child.useCursorPos && child.type != WidgetType::SameLine;
        };
        const bool hasFlowItems = std::any_of(n.children.begin(), n.children.end(), managed);
        line(in + (cApi ? "if (ImGui::BeginChild_Str(" : "if (ImGui::BeginChild(") +
             "\"flow\", " + size(n) + ", " + flags + flagsDefault + ")) {");
        line(body + (cApi ? "ImGui::PushStyleVar_Vec2" : "ImGui::PushStyleVar") +
             "(ImGuiStyleVar_ItemSpacing, ImVec2(" + gap + ", " + gap + "));");
        auto readVector = [&](const std::string& pad, const std::string& name, const std::string& function) {
          line(pad + (cApi ? "ImVec2 " + name + "; ImGui::" + function + "(&" + name + ");"
                           : "const ImVec2 " + name + " = ImGui::" + function + "();"));
        };
        if (hasFlowItems) {
          readVector(body, "studio_flow_origin", "GetCursorScreenPos");
          readVector(body, "studio_flow_available", "GetContentRegionAvail");
          line(body + "const float studio_flow_right = studio_flow_origin.x + studio_flow_available.x;");
          line(body + "float studio_previous_right = 0.0f;");
          line(body + "bool studio_has_previous = false;");
          line(body + "ImGuiStorage* studio_flow_storage = ImGui::GetStateStorage();");
        }
        for (const auto& child : n.children) if (child.visible) {
          if (!managed(child)) {
            emit(child, depth + 2, true);
            if (hasFlowItems && child.useCursorPos && child.type != WidgetType::SameLine)
              line(body + "studio_has_previous = false;");
            continue;
          }
          const auto item = body + "    ";
          line(body + "{");
          line(item + (cApi ? "const ImGuiID studio_width_id = ImGui::GetID_Str(" : "const ImGuiID studio_width_id = ImGui::GetID(") +
               quote("flow-width-" + hexId(child.id)) + ");");
          line(item + (cApi ? "const float studio_width = ImGuiStorage_GetFloat(studio_flow_storage, "
                            : "const float studio_width = studio_flow_storage->GetFloat(") + "studio_width_id, -1.0f);");
          line(item + "if (" + boolean(child.width >= 0) + " && studio_has_previous && studio_width >= 0.0f && " +
               "studio_previous_right + " + gap + " + studio_width <= studio_flow_right) ImGui::SameLine(0, " + gap + ");");
          readVector(item, "studio_item_start", "GetCursorScreenPos");
          line(item + "ImGui::BeginGroup();");
          emit(child, depth + 3, true);
          line(item + "ImGui::EndGroup();");
          readVector(item, "studio_item_end", "GetItemRectMax");
          line(item + "studio_previous_right = studio_item_end.x;");
          line(item + "studio_has_previous = true;");
          line(item + "const float studio_measured = studio_item_end.x - studio_item_start.x;");
          line(item + (cApi ? "ImGuiStorage_SetFloat(studio_flow_storage, " : "studio_flow_storage->SetFloat(") +
               "studio_width_id, studio_measured > 0.0f ? studio_measured : 0.0f);");
          line(body + "}");
        }
        line(body + (cApi ? "ImGui::PopStyleVar(1);" : "ImGui::PopStyleVar();"));
        line(in + "}");
        call("ImGui::EndChild()");
        break;
      }
      case WidgetType::TabBar: case WidgetType::TabItem:
        line(in + std::string(n.type == WidgetType::TabBar ? "if (ImGui::BeginTabBar(" : "if (ImGui::BeginTabItem(") + label + (cApi ? (n.type == WidgetType::TabBar ? ", 0" : ", NULL, 0") : "") + ")) {"); children();
        line(in + (n.type == WidgetType::TabBar ? "    ImGui::EndTabBar();" : "    ImGui::EndTabItem();")); line(in + "}"); break;
      case WidgetType::CollapsingHeader: case WidgetType::TreeNode:
        call("ImGui::SetNextItemOpen(" + boolean(p.defaultOpen) + ", ImGuiCond_Once)");
        line(in + std::string(n.type == WidgetType::TreeNode ?
          (cApi ? "if (ImGui::TreeNode_Str(" : "if (ImGui::TreeNode(") :
          (cApi ? "if (ImGui::CollapsingHeader_TreeNodeFlags(" : "if (ImGui::CollapsingHeader(")) +
          label + (cApi && n.type == WidgetType::CollapsingHeader ? ", 0" : "") + ")) {"); children();
        if (n.type == WidgetType::TreeNode) line(in + "    ImGui::TreePop();");
        line(in + "}"); break;
      case WidgetType::Table: case WidgetType::GridLayout: {
        const int count = std::clamp(p.columns,1,511);
        if (count != p.columns) warn(n,"Columnas fuera de 1..511; se exporta el limite de Dear ImGui");
        std::string flags = n.type == WidgetType::Table ? std::string("ImGuiTableFlags_RowBg") + (p.border ? " | ImGuiTableFlags_Borders" : "") : "ImGuiTableFlags_None";
        if (n.height > 0) flags += " | ImGuiTableFlags_ScrollY";
        if (n.type == WidgetType::GridLayout) call(std::string(cApi ? "ImGui::PushStyleVar_Vec2" : "ImGui::PushStyleVar") + "(ImGuiStyleVar_CellPadding, ImVec2(" + layout(std::max(0.0f,p.spacing) * 0.5f) + ", " + layout(std::max(0.0f,p.spacing) * 0.5f) + "))");
        line(in + "if (ImGui::BeginTable(" + label + ", " + std::to_string(count) + ", " + flags + ", " + size(n) + flagsDefault + ")) {");
        if (n.type == WidgetType::Table) {
          for (int i = 0; i < count; ++i) line(in + "    ImGui::TableSetupColumn(" + quote(static_cast<size_t>(i) < p.columnLabels.size() ? p.columnLabels[static_cast<size_t>(i)] : "Columna " + std::to_string(i + 1)) + (cApi ? ", 0, 0, 0" : "") + ");");
          line(in + "    ImGui::TableHeadersRow();");
        }
        for (const auto& child : n.children) if (child.visible) { line(in + "    ImGui::TableNextColumn();"); emit(child,depth + 2); }
        line(in + "    ImGui::EndTable();"); line(in + "}");
        if (n.type == WidgetType::GridLayout) call(cApi ? "ImGui::PopStyleVar(1)" : "ImGui::PopStyleVar()");
        break;
      }
      case WidgetType::MenuBar:
        if (titleItem || p.menuBarEdge == MenuBarEdge::Top) {
          line(in + (titleItem ? "{" : "if (ImGui::BeginMenuBar()) {"));
          if (titleItem && p.items.empty()) line(in + "    ImGui::TextDisabled(\"Menú vacío\");");
          for (size_t i = 0; i < p.items.size(); ++i) {
            line(in + (cApi ? "    ImGui::PushID_Int(" : "    ImGui::PushID(") + std::to_string(i) + ");");
            line(in + "    if (ImGui::BeginMenu(" + quote(p.items[i]) + (cApi ? ", true)) { ImGui::MenuItem_Bool(\"Sin acciones configuradas\", NULL, false, false); ImGui::EndMenu(); }" : ")) { ImGui::MenuItem(\"Sin acciones configuradas\", nullptr, false, false); ImGui::EndMenu(); }"));
            line(in + "    ImGui::PopID();");
          }
          if (!titleItem) line(in + "    ImGui::EndMenuBar();");
          line(in + "}");
        } else { warn(n,"Menu fuera del borde superior: generacion pendiente"); call("ImGui::TextDisabled(\"Menu: borde pendiente de implementacion\")"); }
        break;
      default: warn(n,"Widget pendiente de generacion: " + std::string(widgetTypeId(n.type))); call("ImGui::TextDisabled(\"Widget pendiente: %s\", " + quote(widgetTypeId(n.type)) + ")"); children(); break;
    }
    if (colors) call("ImGui::PopStyleColor(" + std::to_string(colors) + ")");
    if (vars) call("ImGui::PopStyleVar(" + std::to_string(vars) + ")");
    if (!titleItem && n.type != WidgetType::Window && n.indent > 0) call("ImGui::Unindent(" + layout(n.indent) + ")");
    call("ImGui::PopID()"); line(pad + "}"); result.nodeLines[n.id] = {firstLine,lines.size()};
  }
};

inline std::string applicationMain(const StudioProject& project, const std::string& stateType, bool imageTextures = false) {
  std::ostringstream out;
  if (imageTextures) out << imageOpenGL;
  out << R"CPP(
int main(int argc, char** argv) {
    const bool smoke = argc == 2 && std::strcmp(argv[1], "--smoke-test") == 0;
    if (!glfwInit()) return 1;
#if defined(__APPLE__)
    const char* glsl = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    const char* glsl = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    if (smoke) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
)CPP";
  out << "    GLFWwindow* window = glfwCreateWindow(" << project.canvas.width << ", " << project.canvas.height << ", " << quote(project.name.substr(0,project.name.find('\0'))) << ", nullptr, nullptr);\n";
  out << R"CPP(    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window); glfwSwapInterval(1);
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    if (smoke) ImGui::GetIO().IniFilename = nullptr;
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) { ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate(); return 1; }
    if (!ImGui_ImplOpenGL3_Init(glsl)) { ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate(); return 1; }
    ApplyStudioStyle();
)CPP";
  out << "    " << stateType << " state;\n";
  if (imageTextures)
    out << "    size_t image_bytes = 0;\n"
           "    const bool images_loaded = LoadStudioImages(state, StudioUploadOpenGLImage, &image_bytes);\n";
  out << R"CPP(    int frames = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        RenderStudioUI(state); ImGui::Render();
        int width = 0, height = 0; glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height); glClearColor(0.05f, 0.06f, 0.09f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); glfwSwapBuffers(window);
        if (smoke && ++frames == 3) break;
    }
)CPP";
  if (imageTextures) out << "    ReleaseStudioImages(state, StudioReleaseOpenGLImage, &image_bytes);\n";
  out << "    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();\n"
         "    glfwDestroyWindow(window); glfwTerminate();\n"
      << (imageTextures ? "    return smoke && !images_loaded ? 1 : 0;\n" : "    return 0;\n") << "}\n";
  return out.str();
}
} // namespace CppCodegen

inline GeneratedCpp generateCppDetailed(const StudioProject& project, CppEmitMode mode = CppEmitMode::Header) {
  using namespace CppCodegen;
  Emitter e;
  try {
    const auto valid = projectFromJson(projectToJson(project)); if (!valid) throw std::runtime_error(valid.error);
    e.result.stateType = sanitizeIdent(project.name,"StudioUI") + "State";
    e.used.insert(e.result.stateType);
    e.used.insert("std");
    e.used.insert("studio_image_textures");
    if (mode == CppEmitMode::Header) e.line("#pragma once");
    if (mode == CppEmitMode::Full) e.line("#ifndef NOMINMAX\n#define NOMINMAX\n#endif\n#include \"imgui_impl_glfw.h\"\n#include \"imgui_impl_opengl3.h\"\n#include <GLFW/glfw3.h>\n#include <cstring>");
    e.line("#include \"imgui.h\"\n#include \"misc/cpp/imgui_stdlib.h\"\n#include <algorithm>\n#include <array>\n#include <string>\n#include <cfloat>\n#include <cstddef>");
    e.collect(project.nodes);
    if (e.result.hasTitleButtons) e.line("#include \"imgui_internal.h\"");
    if (e.result.hasKnobs) e.line("#include \"imgui-knobs.h\"");
    if (e.result.hasStatusWidgets) e.line("#include \"studio_status_widgets.hpp\"");
    if (e.hasImageWidgets) e.line(imageTypes);
    e.emitImageState();
    if (e.result.hasTitleButtons) e.line(std::string("#define STUDIO_EXPORT_CHROME\n") + titleBarFunctions + "\n#undef STUDIO_EXPORT_CHROME");
    e.line("struct " + e.result.stateType + " {");
    for (const auto& declaration : e.declarations) e.line("    " + declaration);
    e.line("};"); e.style(project); e.modelFunctions(); e.chartFunctions(); e.emitAnimationFunctions(); e.emitImageFunctions();
    e.emitTitleCallbacks();
    e.line("inline void RenderStudioUI(" + e.result.stateType + "& state) {\n    (void)state;");
    e.repeatBudget();
    e.resetFrameButtons();
    for (const auto& node : project.nodes) e.emit(node,1);
    e.line("}"); std::ostringstream code;
    for (const auto& line : e.lines) code << line << '\n';
    if (mode == CppEmitMode::Full) code << applicationMain(project,e.result.stateType,e.result.hasImageTextures);
    for (const auto& d : e.result.diagnostics) code << "// [" << comment(d.nodeId) << "] " << comment(d.message) << '\n';
    e.result.code = code.str();
  } catch (const std::exception& error) { e.result.error = error.what(); e.result.code = "// Generation failed: " + comment(error.what()) + "\n"; }
  return e.result;
}
inline std::string generateCppCode(const StudioProject& project, CppEmitMode mode = CppEmitMode::Header) { return generateCppDetailed(project,mode).code; }
} // namespace ImGuiStudio
