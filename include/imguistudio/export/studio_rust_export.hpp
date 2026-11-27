#pragma once
#include "studio_cpp_export.hpp"
#include "imguistudio/export/detail/studio_chart2d_rust.hpp"
#include "imguistudio/export/detail/studio_chart3d_rust.hpp"
#include "imguistudio/export/detail/studio_animation_rust.hpp"
#include "imguistudio/export/detail/studio_managed_animation.hpp"
#include "imguistudio/export/detail/studio_images_java_rust.hpp"
#include "imguistudio/widgets/studio_image_data.hpp"
#include "imguistudio/export/detail/studio_managed_titlebar.hpp"
#include "imguistudio/export/detail/studio_titlebar_rust.hpp"

namespace ImGuiStudio {
namespace RustCodegen {

inline std::string literal(const std::string& value) {
  std::string out = "\"";
  for (const unsigned char c : value) {
    if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
    else if (c < 32 || c == 127) {
      char escape[8];
      std::snprintf(escape, sizeof(escape), "\\x%02x", static_cast<unsigned>(c));
      out += escape;
    } else out += static_cast<char>(c);
  }
  return out + '"';
}

inline std::string number(float v) {
  return CppCodegen::number(v) + "32";
}

inline std::string vec(float x, float y) { return "[" + number(x) + ", " + number(y) + "]"; }

inline std::string color(const Color4& c, bool alpha = true) {
  return "[" + number(c.r) + ", " + number(c.g) + ", " + number(c.b) + (alpha ? ", " + number(c.a) : "") + "]";
}

inline std::string strings(const std::vector<std::string>& values) {
  std::string out = "[";
  for (const auto& s : values) out += literal(s) + ", ";
  return out + "]";
}

struct Emitter {
  GeneratedCpp result;
  std::vector<std::string> lines, declarations, initializers;
  std::set<std::string> used;
  std::map<std::string, std::string> radios;
  bool hasFlowMetrics = false;
  bool hasModelViews = false;
  bool hasChart2d = false;
  bool hasChart3d = false;
  bool hasRepeater = false;
  bool hasTemplateHeights = false;
  bool hasAnimation = false;
  bool hasImageWidgets = false;
  ImageData::ExportAssets imageAssets;
  std::vector<std::string> frameButtons;
  bool scaled = false;

  std::string field(const ImGuiNode& node, const std::string& type, const std::string& initial) {
    if (node.type == WidgetType::RadioButton) {
      const auto existing = radios.find(node.varName);
      if (existing != radios.end()) {
        result.variables[node.id] = existing->second;
        return "state." + existing->second;
      }
    }
    auto base = "value_" + sanitizeIdent(node.varName);
    for (auto& ch : base) if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
    auto name = base;
    for (int i = 2; !used.insert(name).second; ++i) name = base + "_" + std::to_string(i);
    if (node.type == WidgetType::RadioButton) radios.emplace(node.varName, name);
    result.variables[node.id] = name;
    declarations.push_back("    pub " + name + ": " + type + ",\n");
    initializers.push_back("            " + name + ": " + initial + ",\n");
    return "state." + name;
  }

  using TitleEvent = std::pair<std::string, std::string>;
  static std::string titleCallback(const ImGuiNode& node) { return "title_widget_" + CppCodegen::hexId(node.id); }

  void emitTitleItem(const ImGuiNode& child, size_t depth, bool show, std::vector<TitleEvent>& events) {
    using CppCodegen::boolean;
    const auto first = lines.size() + 1;
    const std::string pad(depth * 4, ' ');
    const auto kind = CppCodegen::titleKind(child);
    if (!kind) frameButtons.push_back(field(child, "bool", "false"));
    if (!show) lines.push_back(pad + "// Title item hidden by the window flags.");
    else {
      const auto b = ManagedTitleBar::describe(child, [&](const std::string& message) {
        result.diagnostics.push_back({child.id, message});
      });
      const auto& p = child.props;
      const auto event = "pressed_" + CppCodegen::hexId(child.id);
      if (!kind) events.push_back({event, frameButtons.back()});
      const auto tint = [&](const std::optional<Color4>& c) { return !kind && c ? color(*c) : "[0.0; 4]"; };
      auto width = number(child.width);
      if (kind == 11 && child.width <= 0) {
        width = "ui.clone_style().item_spacing[0] * 2.0";
        const auto measure = [&](const std::string& item) {
          width += " + unsafe { studio_title_menu_entry_width(c" + literal(ManagedTitleBar::caption(item)) + ".as_ptr()) }";
        };
        for (const auto& item : p.items) measure(item);
        if (p.items.empty()) measure("Menú vacío");
      }
      lines.push_back(pad + "TitleButton {");
      lines.push_back(pad + "    id: c"+literal("##studio_title_" + CppCodegen::hexId(child.id))+".as_ptr(), label: c"+
        literal(ManagedTitleBar::caption(kind == 8 ? p.text : child.label))+".as_ptr(), pressed: "+
        (kind ? "std::ptr::null_mut()" : "&mut " + event)+",");
      lines.push_back(pad + "    side: "+std::to_string(b.side)+", variant: "+std::to_string(b.variant)+
        ", icon: "+std::to_string(kind ? 0 : b.icon)+", width: "+width+", height: "+number(child.height)+",");
      lines.push_back(pad + "    x: "+number(p.titleBarX)+", y: "+number(p.titleBarY.value_or(0))+
        ", rounding: "+number(b.rounding)+", has_y: "+boolean(p.titleBarY.has_value())+",");
      lines.push_back(pad + "    custom_bg: "+boolean(!kind && b.background.has_value())+
        ", custom_text: "+boolean(!kind && b.foreground.has_value())+", bg: "+tint(b.background)+", text: "+tint(b.foreground)+",");
      lines.push_back(pad + "    kind: "+std::to_string(kind)+", draw_widget: "+(kind ? "Some("+titleCallback(child)+")" : "None")+",");
      lines.push_back(pad + "    custom_hover: "+boolean(!kind && b.hovered.has_value())+", custom_active: "+
        boolean(!kind && b.active.has_value())+", hover: "+tint(b.hovered)+", active: "+tint(b.active)+",");
      lines.push_back(pad + "},");
    }
    if (!show || !kind) result.nodeLines[child.id] = {first, lines.size()};
  }
  void emitChart(const ImGuiNode& n, const std::string& size, size_t depth) {
    hasChart2d = true;
    const auto& p = n.props;
    const auto& mode = Chart2d::modes[Chart2d::modeIndex(p.chart2dType)];
    if (p.chart2dType != mode.id)
      result.diagnostics.push_back({n.id, "Tipo de gráfico desconocido. Se genera como línea."});
    std::string values = "[";
    for (const auto sample : p.plotValues) values += number(sample) + ", ";
    const auto fieldName = field(n, "[f32; " + std::to_string(p.plotValues.size()) + "]", values + "]");
    const auto bound = [](bool automatic, float value) { return automatic ? "None" : "Some(" + number(value) + ")"; };
    const std::string pad(depth * 4, ' ');
    lines.push_back(pad + "draw_chart_2d(ui, Chart2d {");
    lines.push_back(pad + "    mode: " + literal(mode.id) + ", title: " +
                    literal(p.overlay.empty() ? mode.label : p.overlay) + ",");
    lines.push_back(pad + "    size: " + size + ", color: " + color(p.color) + ",");
    lines.push_back(pad + "    values: &" + fieldName + ", labels: &" + strings(p.chartLabels) + ",");
    lines.push_back(pad + "    min: " + bound(p.chartAutoMin, p.minFloat) + ", max: " + bound(p.chartAutoMax, p.maxFloat) + ",");
    lines.push_back(pad + "});");
  }

  void emitChart3d(const ImGuiNode& n, const std::string& size, size_t depth) {
    hasChart3d = true;
    const auto& p = n.props;
    const auto& mode = Chart3d::modes[Chart3d::modeIndex(p.chart3dType)];
    if (p.chart3dType != mode.id)
      result.diagnostics.push_back({n.id, "Tipo de gráfico 3D desconocido. Se generan barras."});
    std::string values = "[";
    for (const float sample : p.plotValues) values += number(sample) + ", ";
    const auto fieldName = field(n, "Chart3dState<" + std::to_string(p.plotValues.size()) + ">",
      "Chart3dState { values: " + values + "], elevation: " + number(p.chart3dRotX) +
      ", azimuth: " + number(p.chart3dRotY) + " }");
    lines.push_back(std::string(depth * 4, ' ') + "draw_chart_3d(ui, " + literal(mode.id) + ", " +
      literal(p.overlay.empty() ? mode.label : p.overlay) + ", " + size + ", " + color(p.color) +
      ", &mut " + fieldName + ");");
  }

  void emit(const ImGuiNode& n, size_t depth, bool flowChild = false, bool titleItem = false) {
    using CppCodegen::boolean;
    if (!n.visible) return;
    const auto start = lines.size() + 1;
    if (flowChild && n.type == WidgetType::SameLine) {
      lines.push_back(std::string(depth * 4, ' ') + "// Flow determines line breaks for its children.");
      result.nodeLines[n.id] = {start, lines.size()};
      return;
    }
    const auto& p = n.props;
    const auto id = CppCodegen::hexId(n.id);
    const auto label = literal(n.label.substr(0, n.label.find('\0')) + "###studio_" + id);
    const auto layout = [&](float value) { return number(value) + (scaled && value != 0 ? " * animated_scale" : ""); };
    const auto dimensions = [&](float x, float y) { return "[" + layout(x) + ", " + layout(y) + "]"; };
    const auto itemWidth = n.width < 0 ? std::string("-f32::MIN_POSITIVE") : layout(n.width);
    const auto size = "[" + itemWidth + ", " + layout(std::max(0.0f,n.height)) + "]";
    auto put = [&](const std::string& text) { lines.push_back(std::string(depth * 4, ' ') + text); };
    auto call = [&](const std::string& fn, const std::string& args = {}) { put("ui." + fn + "(" + args + ");"); };
    auto warn = [&](const std::string& message) { result.diagnostics.push_back({n.id, message}); };
    auto children = [&] {
      for (const auto& child : n.children)
        if (n.type != WidgetType::Window || !WindowChrome::isTitleItem(child)) emit(child, depth);
    };
    auto block = [&](const std::string& opening, const std::string& closing = "}") {
      put(opening); ++depth; children(); --depth; put(closing);
    };
    const auto props = ProjectJson::propsJson(p, n.type);
    if (n.type == WidgetType::Window) {
      static const std::map<std::string, std::string> flags = {
        {"NoTitleBar","NO_TITLE_BAR"},{"NoResize","NO_RESIZE"},{"NoMove","NO_MOVE"},
        {"NoCollapse","NO_COLLAPSE"},{"NoScrollbar","NO_SCROLLBAR"},{"MenuBar","MENU_BAR"},
        {"AlwaysAutoResize","ALWAYS_AUTO_RESIZE"},{"NoSavedSettings","NO_SAVED_SETTINGS"},
        {"NoBackground","NO_BACKGROUND"},{"HorizontalScrollbar","HORIZONTAL_SCROLLBAR"},{"NoDecoration","NO_DECORATION"},
        {"NoScrollWithMouse","NO_SCROLL_WITH_MOUSE"},{"NoMouseInputs","NO_MOUSE_INPUTS"},
        {"NoFocusOnAppearing","NO_FOCUS_ON_APPEARING"},{"NoBringToFrontOnFocus","NO_BRING_TO_FRONT_ON_FOCUS"},
        {"AlwaysVerticalScrollbar","ALWAYS_VERTICAL_SCROLLBAR"},{"AlwaysHorizontalScrollbar","ALWAYS_HORIZONTAL_SCROLLBAR"},
        {"NoNavInputs","NO_NAV_INPUTS"},{"NoNavFocus","NO_NAV_FOCUS"},{"NoNav","NO_NAV"},
        {"UnsavedDocument","UNSAVED_DOCUMENT"},{"NoInputs","NO_INPUTS"}
      };
      const auto title = ManagedTitleBar::describeWindow(n);
      const auto& a = p.windowAppearance;
      bool collapse = title.collapse;
      std::string bits = "imgui::WindowFlags::empty()";
      for (const auto& raw : ProjectJson::strings(props, "windowFlags", {})) {
        const auto name = raw.rfind("ImGuiWindowFlags_", 0) == 0 ? raw.substr(17) : raw;
        const auto it = flags.find(name);
        if (it == flags.end()) warn("Flag de ventana pendiente: " + raw);
        else bits += " | imgui::WindowFlags::" + it->second;
        collapse &= name != "NoCollapse";
      }
      if (!collapse) bits += " | imgui::WindowFlags::NO_COLLAPSE";
      result.hasTitleButtons |= title.custom || title.shadow != 0;
      const bool closable = title.custom && p.closable;
      const auto opened = closable ? field(n, "bool", "true") : std::string{};
      const auto mode = title.modes ? "state.window_mode_" + id : std::string{};
      if (title.modes) {
        declarations.push_back("    window_mode_" + id + ": WindowMode,\n");
        initializers.push_back("            window_mode_" + id + ": WindowMode::default(),\n");
      }
      put(closable ? "if " + opened + " {" : "{"); ++depth;
      const auto styleVar = [&](const char* name, std::optional<float> value, float min, float max) {
        if (value) put("let _style_override = ui.push_style_var(imgui::StyleVar::" +
          std::string(name) + "(" + number(std::clamp(*value, min, max)) + "));");
      };
      const auto tint = [&](const char* name, const std::optional<Color4>& value) {
        if (value) put("let _window_tint = ui.push_style_color(imgui::StyleColor::" +
          std::string(name) + ", " + color(*value) + ");");
      };
      styleVar("Alpha", a.alpha, 0, 1);
      styleVar("WindowRounding", a.rounding, 0, 72);
      styleVar("WindowBorderSize", p.noBackground ? std::optional<float>{0} : a.borderSize, 0, 12);
      const auto pad = [&](std::optional<float> value, int axis) {
        return value ? number(std::clamp(*value,0.0f,256.0f)) : "ui.clone_style().window_padding[" + std::to_string(axis) + "]";
      };
      if (a.paddingX || a.paddingY)
        put("let _window_padding = ui.push_style_var(imgui::StyleVar::WindowPadding(["+pad(a.paddingX,0)+", "+pad(a.paddingY,1)+"]));");
      tint("WindowBg", a.background); tint("Border", a.borderColor); tint("Text", a.textColor);
      if (!a.showResizeGrip)
        for (const auto slot : {"ResizeGrip","ResizeGripHovered","ResizeGripActive"}) tint(slot, Color4{0,0,0,0});
      const auto titleBg = a.transparentTitle ? std::optional<Color4>{Color4{0,0,0,0}} : a.titleBackground;
      for (const auto slot : {"TitleBg","TitleBgActive","TitleBgCollapsed"}) tint(slot, titleBg);
      for (const auto& property : WindowChrome::colors)
        if (props.contains(property.key) && !(a.*property.member)) warn(std::string("Color de ventana incompatible: ")+property.key);
      const bool customHeight = title.visible && a.titleHeight.has_value();
      if (customHeight)
        put("let title_padding = ui.push_style_var(imgui::StyleVar::FramePadding([ui.clone_style().frame_padding[0], "
          "(("+number(std::clamp(*a.titleHeight,20.0f,72.0f))+" - ui.current_font_size()) * 0.5).max(0.0)]));");
      if (title.modes) {
        put("let minimized = " + mode + ".mode == 1;");
        put("let minimum_size = minimized.then(|| ui.push_style_var(imgui::StyleVar::WindowMinSize([ui.clone_style().window_min_size[0], 1.0])));");
        bits += " | if " + mode + ".mode != 0 { imgui::WindowFlags::NO_MOVE | imgui::WindowFlags::NO_RESIZE | imgui::WindowFlags::NO_COLLAPSE } else { imgui::WindowFlags::empty() }";
      }
      for (const auto& child : n.children)
        if (child.visible && child.type == WidgetType::MenuBar && child.props.menuBarEdge == MenuBarEdge::Top) {
          bits += title.modes ? " | if minimized { imgui::WindowFlags::empty() } else { imgui::WindowFlags::MENU_BAR }" :
            " | imgui::WindowFlags::MENU_BAR"; break;
        }
      const auto position = vec(n.x,n.y), windowSize = vec(std::max(0.0f,n.width),std::max(0.0f,n.height));
      if (title.custom) {
        put("let (_window, studio_expanded) = begin_title_window(ui, c"+literal("###studio_" + id)+
          ", "+position+", "+windowSize+", "+bits+", "+boolean(title.inset)+", "+
          (title.modes ? "Some(&mut "+mode+")" : "None")+");");
        if (title.modes) put("drop(minimum_size);");
        if (title.shadow) put("unsafe { studio_window_shadow("+std::to_string(title.shadow)+"); }");

        for (const auto& child : n.children) {
          if (!ManagedTitleBar::visibleItem(child)) continue;
          if (!CppCodegen::titleKind(child)) {
            put("let mut pressed_"+CppCodegen::hexId(child.id)+" = false;");
            continue;
          }
          put("unsafe extern \"C\" fn "+titleCallback(child)+"(data: *mut std::ffi::c_void, title_width: f32, title_height: f32) {");
          ++depth;
          put("let context = unsafe { &mut *data.cast::<TitleContext<'_>>() };");
          put("let ui = context.ui; let state = &mut *context.state;");
          put("let _ = (&state, title_width, title_height);");
          emit(child, depth, false, true);
          --depth; put("}");
        }
        if (p.closable) put("let mut close = false;");
        if (p.showMinimizeButton) put("let mut minimize = false;");
        if (p.showMaximizeButton) put("let mut maximize = false;");
        if (title.inset && collapse) put("let mut collapse = false;");
        put("let title_buttons = ["); ++depth;
        const auto control = [&](const char* name, int kind, int side) {
          put("TitleButton { id: c"+literal(std::string("##studio_window_")+name)+".as_ptr(), label: c\"\".as_ptr(), pressed: &mut "+name+",");
          put("    side: "+std::to_string(side)+", variant: 0, icon: 0, width: 0.0, height: 0.0, x: 0.0, y: 0.0, rounding: 0.0,");
          put("    has_y: false, custom_bg: false, custom_text: false, bg: [0.0;4], text: [0.0;4],");
          put("    kind: "+std::to_string(kind)+", draw_widget: None, custom_hover: false, custom_active: false, hover: [0.0;4], active: [0.0;4] },");
        };
        if (title.inset && collapse) control("collapse", 4, 0);
        if (title.side != 2) ManagedTitleBar::eachWindowControl(p, title, control);
        std::vector<TitleEvent> events;
        for (const auto& child : n.children) if (ManagedTitleBar::visibleItem(child)) emitTitleItem(child, depth, true, events);
        if (title.side == 2) ManagedTitleBar::eachWindowControl(p, title, control);
        --depth; put("];");
        put("let title_mode = "+(title.modes ? mode+".mode" : "0")+";");
        put("let mut title_context = TitleContext { ui, state };");
        put("unsafe { studio_render_title_bar(c"+literal(ManagedTitleBar::caption(p.showTitleText ? n.label : ""))+
          ".as_ptr(), title_buttons.as_ptr(), title_buttons.len(), TitleAppearance {");
        put("    left: "+number(title.left)+", right: "+number(title.right)+", alignment: "+number(title.alignment)+
          ", icon: "+std::to_string(title.icon)+", controls_variant: "+std::to_string(title.variant)+",");
        put("    separator: "+boolean(a.titleSeparator)+", custom_separator: "+boolean(a.separatorColor.has_value())+
          ", transparent: "+boolean(a.transparentTitle)+", separator_color: "+(a.separatorColor ? color(*a.separatorColor) : "[0.0;4]")+", fill: 0,");
        put("}, (&mut title_context as *mut TitleContext<'_>).cast(), title_mode); }");
        for (const auto& [event, fieldName] : events) put(fieldName+" = "+event+";");
        if (p.closable) put("if close { "+opened+" = false; }");
        if (p.showMinimizeButton) put("if minimize { unsafe { studio_set_window_mode(&mut "+mode+", 1); } }");
        if (p.showMaximizeButton) put("if maximize { unsafe { studio_set_window_mode(&mut "+mode+", 2); } }");
        if (customHeight) put("drop(title_padding);");
        block(title.modes ? "if studio_expanded && !minimized {" : "if studio_expanded {");
      } else {
        std::vector<TitleEvent> unused;
        for (const auto& child : n.children) if (ManagedTitleBar::visibleItem(child)) emitTitleItem(child, depth, false, unused);
        const auto builder = "ui.window(" + label + ").position(" + position + ", imgui::Condition::FirstUseEver).size(" +
          windowSize + ", imgui::Condition::FirstUseEver).flags(" + bits + ")";
        put("let studio_window = " + builder + ".begin();");
        if (customHeight) put("drop(title_padding);");
        put("if let Some(_window) = studio_window {"); ++depth;
        if (title.shadow) put("unsafe { studio_window_shadow("+std::to_string(title.shadow)+"); }");
        children(); --depth; put("}");
      }
      for (const auto& child : n.children)
        if (child.visible && child.props.inTitleBar && !ManagedTitleBar::visibleItem(child))
          result.diagnostics.push_back({child.id,"Control no compatible con la barra de título. Se conserva en el cuerpo."});
      --depth; put("}");
    } else {
      put("{"); ++depth;
      put("let _id = ui.push_id(" + literal(id) + ");");
      if (!titleItem) {
        if (n.useCursorPos) call("set_cursor_pos", dimensions(n.x, n.y));
        else if (n.sameLine && !flowChild) {
          if (n.sameLineSpacing > 0) call("same_line_with_spacing", "0.0, " + layout(n.sameLineSpacing));
          else call("same_line");
        }
        if (n.indent > 0) call("indent_by", layout(n.indent));
      }
      const auto widgetColor = [&](const char* keyName, const char* slot) {
        if (!props.contains(keyName) || !props.at(keyName).is_string()) return;
        try {
          put("let _widget_color = ui.push_style_color(imgui::StyleColor::" + std::string(slot) + ", " +
              color(ProjectJson::colorFromHex(props.at(keyName).get<std::string>())) + ");");
        } catch (const std::exception&) { warn(std::string("Color incompatible: ") + keyName); }
      };
      widgetColor("textColorOverride", "Text");
      widgetColor("buttonColorOverride", "Button");
      if (titleItem)
        for (const auto& entry : CppCodegen::titleWidgetColors) widgetColor(entry.key, entry.native);
      auto width = [&] {
        if (titleItem) call("set_next_item_width", "title_width");
        else if (n.width != 0) call("set_next_item_width", itemWidth);
      };
      auto format = [&](bool integer, const std::string& fallback) {
        if (!p.format.empty() && !safeNumericFormat(p.format, integer)) warn("Formato numerico incompatible");
        return literal(safeNumericFormat(p.format, integer) ? p.format : fallback);
      };
      const auto floatRange = number(std::min(p.minFloat, p.maxFloat)) + ", " + number(std::max(p.minFloat, p.maxFloat));
      const auto intRange = std::to_string(std::min(p.minInt, p.maxInt)) + ", " + std::to_string(std::max(p.minInt, p.maxInt));
      switch (n.type) {
        case WidgetType::Text: call("text", literal(p.text)); break;
        case WidgetType::TextColored: call("text_colored", color(p.color) + ", " + literal(p.text)); break;
        case WidgetType::TextWrapped: call("text_wrapped", literal(p.text)); break;
        case WidgetType::BulletText: call("bullet_text", literal(p.text)); break;
        case WidgetType::SeparatorText:
          call("separator"); call("text", literal(n.label)); warn("Separador con texto adaptado a imgui-rs 0.12"); break;
        case WidgetType::HelpMarker:
          call("text_disabled", literal("(?)"));
          put("if ui.is_item_hovered() { ui.tooltip_text(" + literal(p.text) + "); }"); break;
        case WidgetType::Button: call("button_with_size", label + ", " + (titleItem ? "[title_width, title_height]" : size)); break;
        case WidgetType::SmallButton: call("small_button", label); break;
        case WidgetType::ArrowButton:
          call("arrow_button", label + ", imgui::Direction::" +
            (p.arrowDir == "left" ? "Left" : p.arrowDir == "up" ? "Up" : p.arrowDir == "down" ? "Down" : "Right")); break;
        case WidgetType::ColorButton: put("ui.color_button_config(" + label + ", " + color(p.color) + ").size(" + size + ").build();"); break;
        case WidgetType::Selectable: {
          const auto selected = field(n, "bool", boolean(p.selected));
          put("if ui.selectable_config(" + label + ").selected(" + selected + ").size(" + size + ").build() { " +
              selected + " = !" + selected + "; }");
          break;
        }
        case WidgetType::Checkbox: call("checkbox", label + ", &mut " + field(n, "bool", boolean(p.checked))); break;
        case WidgetType::RadioButton: call("radio_button", label + ", &mut " + field(n, "i32", std::to_string(p.intValue)) + ", " + std::to_string(p.radioValue)); break;
        case WidgetType::InputText: case WidgetType::InputMultiline: {
          width();
          const auto args = label + ", &mut " + field(n, "String", literal(p.text) + ".to_owned()");
          if (n.type == WidgetType::InputText) put("ui.input_text(" + args + ").hint(" + literal(p.hint) + ").build();");
          else put("ui.input_text_multiline(" + args + ", " + size + ").build();");
          break;
        }
        case WidgetType::InputFloat: case WidgetType::InputInt: {
          width();
          const bool integer = n.type == WidgetType::InputInt;
          const auto value = field(n, integer ? "i32" : "f32", integer ? std::to_string(p.intValue) : number(p.floatValue));
          const auto step = integer ? std::to_string(static_cast<int>(std::clamp(p.speed, 1.0f, 100000.0f))) : number(p.speed);
          put("ui." + std::string(integer ? "input_int" : "input_float") + "(" + label + ", &mut " + value +
              ").step(" + step + ").display_format(" + format(integer, integer ? "%d" : "%.3f") + ").build();");
          break;
        }
        case WidgetType::SliderFloat: case WidgetType::SliderInt: case WidgetType::SliderAngle:
        case WidgetType::DragFloat: case WidgetType::DragInt: case WidgetType::SliderFloat2:
        case WidgetType::SliderFloat3: case WidgetType::DragFloat3: {
          width();
          const bool integer = n.type == WidgetType::SliderInt || n.type == WidgetType::DragInt;
          const bool drag = n.type == WidgetType::DragFloat || n.type == WidgetType::DragInt || n.type == WidgetType::DragFloat3;
          const bool vector = n.type == WidgetType::SliderFloat2 || n.type == WidgetType::SliderFloat3 || n.type == WidgetType::DragFloat3;
          const int count = n.type == WidgetType::SliderFloat2 ? 2 : 3;
          std::string type = integer ? "i32" : "f32";
          std::string initial = integer ? std::to_string(p.intValue) : number(p.floatValue);
          if (vector) {
            type = "[f32; " + std::to_string(count) + "]";
            initial = "[";
            for (int i = 0; i < count; ++i) initial += number(static_cast<size_t>(i) < p.defaultVec.size() ? p.defaultVec[i] : 0) + ", ";
            initial += "]";
          }
          const auto value = "&mut " + field(n, type, initial);
          const auto range = integer ? intRange : floatRange;
          const auto fmt = format(integer, integer ? "%d" : n.type == WidgetType::SliderAngle ? "%.0f deg" : "%.3f");
          auto builder = drag ? "imgui::Drag::new(" + label + ").range(" + range + ").speed(" + number(std::max(0.0f, p.speed)) + ")" :
            n.type == WidgetType::SliderAngle ? "imgui::AngleSlider::new(" + label + ").range_degrees(" + range + ")" :
            "ui.slider_config(" + label + ", " + range + ")";
          put(builder + ".display_format(" + fmt + ")." + (vector ? "build_array(" : "build(") +
              (drag || n.type == WidgetType::SliderAngle ? "ui, " : "") + value + ");");
          break;
        }
        case WidgetType::ColorEdit3: case WidgetType::ColorEdit4: {
          width();
          const bool alpha = n.type == WidgetType::ColorEdit4;
          call(alpha ? "color_edit4" : "color_edit3", label + ", &mut " + field(n, alpha ? "[f32; 4]" : "[f32; 3]", color(p.color, alpha)));
          break;
        }
        case WidgetType::Combo: case WidgetType::Listbox: {
          width();
          const bool combo = n.type == WidgetType::Combo;
          const auto value = field(n, "usize", std::to_string(p.items.empty() ? 0 :
            std::min(static_cast<size_t>(std::max(0, p.selectedIndex)), p.items.size() - 1)));
          const auto items = p.items.empty() ? "[\"(sin opciones)\"]" : strings(p.items);
          if (combo) call("combo_simple_string", label + ", &mut " + value + ", &" + items);
          else {
            put("if let Some(_listbox) = imgui::ListBox::new(" + label + ").size(" + size + ").begin(ui) {");
            ++depth;
            put("for (index, label) in " + std::string(items) + ".iter().enumerate() {");
            put("    let _option = ui.push_id_usize(index);");
            put("    if ui.selectable_config(label).selected(" + value + " == index).build() { " + value + " = index; }");
            put("}");
            --depth; put("}");
          }
          break;
        }
        case WidgetType::ProgressBar:
          put("imgui::ProgressBar::new(" + number(std::clamp(p.progress, 0.0f, 1.0f)) + ").size(" + size + ")" +
              (p.overlay.empty() ? "" : ".overlay_text(" + literal(p.overlay) + ")") + ".build(ui);"); break;
        case WidgetType::PlotLines: case WidgetType::PlotHistogram: {
          std::string values = "[";
          for (auto value : p.plotValues) values += number(value) + ", ";
          const std::string fn = n.type == WidgetType::PlotLines ? "plot_lines" : "plot_histogram";
          put("ui." + fn + "(" + label + ", &" + values + "]).graph_size(" + size + ")" +
              ".scale_min(" + number(std::min(p.minFloat, p.maxFloat)) + ").scale_max(" +
              number(std::max(p.minFloat, p.maxFloat)) + ")" +
              (p.overlay.empty() ? "" : ".overlay_text(" + literal(p.overlay) + ")") + ".build();");
          break;
        }
        case WidgetType::Chart2d: emitChart(n, size, depth); break;
        case WidgetType::Chart3d: emitChart3d(n, size, depth); break;
        case WidgetType::Separator: call("separator"); break;
        case WidgetType::Spacing: call("spacing"); break;
        case WidgetType::SameLine: call("same_line"); break;
        case WidgetType::Dummy:
          call("dummy", n.width < 0 ? "[ui.content_region_avail()[0], " + layout(std::max(0.0f, n.height)) + "]" : size); break;
        case WidgetType::ChildWindow:
          block("if let Some(_child) = ui.child_window(" + label + ").size(" + size + ").border(" + boolean(p.border) + ").begin() {"); break;
        case WidgetType::Image: case WidgetType::ImageButton: {
          hasImageWidgets = true;
          std::string texture = "None";
          if (p.imageAsset) {
            if (const auto index = imageAssets.add(*p.imageAsset))
              texture = "state.image_textures[" + std::to_string(*index) + "]";
            else warn("Datos de imagen no válidos. Se genera un marcador.");
          }
          const bool button = n.type == WidgetType::ImageButton;
          const auto draw = "draw_image(ui, " + texture + ", " + literal(n.label) + ", " + size + ", " +
            (scaled ? "animated_scale" : "1.0") + ", " + boolean(button) + ")";
          if (button) {
            frameButtons.push_back(field(n, "bool", "false"));
            put("if " + draw + " { " + frameButtons.back() + " = true; }");
            if (!p.buttonAction.empty()) warn("La acción de botón requiere integración de la aplicación.");
          } else put(draw + ";");
          break;
        }
        case WidgetType::Animation: {
          hasAnimation = true;
          const auto value = field(n, "AnimationState",
            "AnimationState { running: " + boolean(p.animRunning) + ", start: None }");
          const bool visible = CppCodegen::hasVisibleChildren(n);
          const bool scaleChildren = scaled || p.animType == "scale_pulse";
          const bool previousScale = scaled;
          put("let mut animation = " + value + ";");
          put("let settings = Animation {");
          ++depth;
          put("title: " + literal(n.label) + ", size: " + size + ", effect: " + literal(p.animType) +
              ", curve: " + literal(p.animEasing) + ",");
          put("seconds: " + animationSeconds(p.animDuration) + "_f64, looping: " + boolean(p.animLoop) +
              ", layout_scale: " + (scaled ? "animated_scale" : "1.0") + ",");
          --depth;
          put("};");
          put("draw_animation(ui, &mut animation, settings, " +
              std::string(visible && scaleChildren ? "|animated_scale|" : "|_animated_scale|") + " {");
          ++depth;
          scaled = scaleChildren;
          if (visible) children();
          else call("text", "\"Vista previa de animación\"");
          scaled = previousScale;
          --depth;
          put("});");
          put(value + " = animation;");
          break;
        }
        case WidgetType::Loader: case WidgetType::Component: case WidgetType::Repeater: {
          const bool loader = n.type == WidgetType::Loader, repeater = n.type == WidgetType::Repeater;
          const auto& title = !repeater && !p.sourceComponent.empty() ? p.sourceComponent : n.label;
          const auto childWindow = [&](const std::string& name, const std::string& width, float height,
                                       bool border, bool padding, const std::string& initialHeight, const auto& draw) {
            auto builder = "ui.child_window(" + name + ").border(" + boolean(border) + ")";
            if (padding) builder += ".always_use_window_padding(true)";
            const bool autoHeight = height <= 0;
            if (autoHeight) {
              // imgui-rs 0.12 has fixed-size child windows. Keep the measured height for each ImGui ID.
              hasTemplateHeights = true;
              put("let height_id = ui.new_id_str(" + name + ");");
              put("let height = state.template_heights.get(&height_id).copied().unwrap_or_else(|| " + initialHeight + ");");
              builder += ".size([" + width + ", height]).scroll_bar(false).scrollable(false)";
            } else {
              builder += ".size([" + width + ", " + layout(height) + "])";
            }
            put("if let Some(_template) = " + builder + ".begin() {"); ++depth;
            if (autoHeight) put("let content = ui.begin_group();");
            draw();
            if (autoHeight) {
              put("content.end();");
              put("let height = ui.item_rect_max()[1] - ui.window_pos()[1] + ui.window_content_region_min()[1];");
              put("state.template_heights.insert(height_id, height.ceil().max(4.0));");
            }
            --depth; put("}");
          };
          childWindow("\"template\"", itemWidth, n.height, p.border, true, "ui.content_region_avail()[1].max(4.0)", [&] {
            const auto loaded = loader ? field(n, "bool", boolean(p.loaderActive)) : std::string();
            if (loader) {
              put("if ui.button(if " + loaded + " { \"Descargar###toggle_loader\" } else { \"Cargar###toggle_loader\" }) {");
              ++depth; put(loaded + " = !" + loaded + ";"); --depth; put("}");
              if (!title.empty()) call("same_line");
            }
            if (!title.empty()) call("text", literal(title));
            if (repeater) {
              const auto count = std::to_string(std::clamp(p.repeatCount, 1, kMaxRepeaterInstances));
              call("text_disabled", "\"" + count + " instancias\"");
              if (CppCodegen::hasVisibleChildren(n)) {
                hasRepeater = true;
                const auto gap = std::clamp(p.spacing, 0.0f, 48.0f);
                put("let _spacing = ui.push_style_var(imgui::StyleVar::ItemSpacing(" + dimensions(gap, gap) + "));");
                put("for instance in 0.." + count + " {"); ++depth;
                put("if repeat_budget == 0 { break; }");
                put("repeat_budget -= 1;");
                put("let _instance = ui.push_id_usize(instance);");
                childWindow("\"instance\"", "0.0", 0.0f, false, false, "ui.frame_height().max(4.0)", children);
                --depth; put("}");
              }
            } else if (loader) {
              block("if " + loaded + " {", "} else {"); ++depth;
              call("text_disabled", "\"Contenido descargado\"");
              --depth; put("}");
            } else {
              children();
            }
          });
          break;
        }
        case WidgetType::TreeNode:
          block("if let Some(_tree) = ui.tree_node_config(" + label + ").default_open(" + boolean(p.defaultOpen) + ").push() {"); break;
        case WidgetType::CollapsingHeader:
          block("if imgui::CollapsingHeader::new(" + label + ").default_open(" + boolean(p.defaultOpen) + ").build(ui) {"); break;
        case WidgetType::TabBar: block("if let Some(_tabs) = ui.tab_bar(" + label + ") {"); break;
        case WidgetType::TabItem: block("if let Some(_tab) = ui.tab_item(" + label + ") {"); break;
        case WidgetType::Group: case WidgetType::ColumnLayout: block("ui.group(|| {", "});"); break;
        case WidgetType::RowLayout: {
          put("ui.group(|| {"); ++depth;
          bool first = true;
          for (const auto& child : n.children) {
            if (!child.visible) continue;
            if (!first && !child.useCursorPos && !child.sameLine)
              call("same_line_with_spacing", "0.0, " + layout(std::max(0.0f, p.spacing)));
            emit(child, depth);
            first = false;
          }
          --depth; put("});");
          break;
        }
        case WidgetType::ListView: case WidgetType::GridView: case WidgetType::PathView: {
          hasModelViews = true;
          const auto selection = field(n, "i32", std::to_string(p.selectedIndex));
          const auto modelSize = "[" + itemWidth + ", " +
            layout(n.height > 0 ? n.height : modelViewDefaultHeight(n.type)) + "]";
          const auto columns = std::to_string(n.type == WidgetType::GridView ? std::clamp(p.columns,1,kMaxModelColumns) : 0);
          put("draw_model_view(ui, " + literal(n.label) + ", &" + strings(p.items) + ", &mut " + selection + ", " +
              modelSize + ", " + boolean(p.border) + ", " + columns + ", " +
              layout(std::clamp(p.spacing,0.0f,32.0f)) + ", " + boolean(n.type == WidgetType::PathView) + ");");
          break;
        }
        case WidgetType::PositionerFlow: {
          const bool autoHeight = n.height <= 0;
          const auto heightKey = literal("flow-height-" + id);
          const auto gap = layout(std::max(0.0f, p.spacing));
          const auto managed = [](const ImGuiNode& child) {
            return child.visible && !child.useCursorPos && child.type != WidgetType::SameLine;
          };
          const bool wraps = std::any_of(n.children.begin(), n.children.end(), [&](const ImGuiNode& child) {
            return managed(child) && child.width >= 0;
          });
          hasFlowMetrics |= autoHeight || wraps;
          auto builder = "ui.child_window(\"flow\").border(" + boolean(p.border) + ")";
          if (autoHeight) {
            // imgui-rs 0.12 predates child auto-height. Reuse the last measured content height.
            put("let height = state.flow_metrics.get(" + heightKey + ").copied().unwrap_or_else(|| ui.content_region_avail()[1].max(4.0));");
            builder += ".size([" + itemWidth +
                       ", height]).scroll_bar(false).scrollable(false)";
          } else builder += ".size(" + size + ")";
          put("if let Some(_flow) = " + builder + ".begin() {"); ++depth;
          put("let _spacing = ui.push_style_var(imgui::StyleVar::ItemSpacing([" + gap + ", " + gap + "]));");
          if (autoHeight) put("let content = ui.begin_group();");
          if (wraps) {
            put("let right = ui.cursor_screen_pos()[0] + ui.content_region_avail()[0];");
            put("let previous_right: Option<f32> = None;");
          }
          for (const auto& child : n.children) {
            if (!child.visible) continue;
            if (!wraps || !managed(child)) {
              emit(child, depth, true);
              if (wraps && child.useCursorPos && child.type != WidgetType::SameLine)
                put("let previous_right: Option<f32> = None;");
              continue;
            }
            const auto key = literal("flow-width-" + CppCodegen::hexId(child.id));
            if (child.width >= 0) {
              put("if let (Some(previous), Some(width)) = (previous_right, state.flow_metrics.get(" + key + ")) {");
              ++depth; put("if previous + " + gap + " + width <= right { ui.same_line_with_spacing(0.0, " + gap + "); }");
              --depth; put("}");
            }
            put("let start = ui.cursor_screen_pos()[0];");
            put("ui.group(|| {"); ++depth;
            emit(child, depth, true);
            --depth; put("});");
            put("let end = ui.item_rect_max()[0];");
            put("let previous_right = Some(end);");
            put("state.flow_metrics.insert(" + key + ", (end - start).max(0.0));");
          }
          if (autoHeight) {
            put("content.end();");
            put("let height = ui.item_rect_max()[1] - ui.window_pos()[1] + ui.window_content_region_min()[1];");
            put("state.flow_metrics.insert(" + heightKey + ", height.ceil().max(4.0));");
          }
          --depth; put("}");
          break;
        }
        case WidgetType::Table: case WidgetType::GridLayout: {
          const bool table = n.type == WidgetType::Table;
          const int columns = std::clamp(p.columns, 1, 64);
          if (columns != p.columns) warn("Numero de columnas limitado a 64 por imgui-rs");
          const std::string flags = table ? "imgui::TableFlags::BORDERS | imgui::TableFlags::ROW_BG" :
                                            "imgui::TableFlags::SIZING_STRETCH_SAME";
          put("if let Some(_table) = ui.begin_table_with_sizing(" + label + ", " + std::to_string(columns) +
              ", " + flags + ", " + size + ", 0.0) {");
          ++depth;
          if (table) {
            for (int i = 0; i < columns; ++i)
              call("table_setup_column", literal(static_cast<size_t>(i) < p.columnLabels.size() ?
                p.columnLabels[i] : "Columna " + std::to_string(i + 1)));
            call("table_headers_row");
          }
          for (const auto& child : n.children) {
            if (!child.visible) continue;
            call("table_next_column"); emit(child, depth);
          }
          --depth; put("}");
          break;
        }
        case WidgetType::MenuBar: {
          if (!titleItem && p.menuBarEdge != MenuBarEdge::Top) { warn("Borde de menú pendiente de exportar"); break; }
          if (!titleItem) { put("if let Some(_bar) = ui.begin_menu_bar() {"); ++depth; }
          if (titleItem && p.items.empty()) put("ui.text_disabled(\"Menú vacío\");");
          for (size_t i = 0; i < p.items.size(); ++i) {
            put("{");
            put("    let _item = ui.push_id_int("+std::to_string(i)+");");
            put("    if let Some(_menu) = ui.begin_menu(" + literal(p.items[i]) + ") {");
            put("        ui.text_disabled(\"Sin acciones configuradas\");");
            put("    }");
            put("}");
          }
          if (!titleItem) { --depth; put("}"); }
          break;
        }
        default:
          warn("Widget pendiente en Rust: " + (n.source.is_object() ?
            n.source.value("type", std::string(widgetTypeId(n.type))) : std::string(widgetTypeId(n.type))));
          call("text_disabled", literal(n.label)); children(); break;
      }
      if (!p.buttonAction.empty()) warn("La accion del widget requiere codigo de aplicacion");
      if (!titleItem && n.indent > 0) call("unindent_by", layout(n.indent));
      --depth; put("}");
    }
    result.nodeLines[n.id] = {start, lines.size()};
  }
};
} // namespace RustCodegen

inline GeneratedCpp generateRustCode(const StudioProject& project, std::vector<std::string>* imageResources = nullptr) {
  RustCodegen::Emitter emitter;
  try {
    for (const auto& node : project.nodes) emitter.emit(node, 1);
    const auto& images = emitter.imageAssets.items();
    emitter.result.hasImageTextures = !images.empty();
    std::ostringstream code;
    code << "// Generated by ImGui Studio for imgui-rs 0.12.\n"
         << "pub struct UiState {\n";
    if (emitter.hasFlowMetrics) code << "    flow_metrics: std::collections::HashMap<&'static str, f32>,\n";
    if (emitter.hasTemplateHeights) code << "    template_heights: std::collections::HashMap<imgui::Id, f32>,\n";
    if (!images.empty()) code << "    image_textures: [Option<ImageTexture>; " << images.size() << "],\n";
    for (const auto& line : emitter.declarations) code << line;
    code << "}\n\nimpl Default for UiState {\n    fn default() -> Self {\n        Self {\n";
    if (emitter.hasFlowMetrics) code << "            flow_metrics: std::collections::HashMap::new(),\n";
    if (emitter.hasTemplateHeights) code << "            template_heights: std::collections::HashMap::new(),\n";
    if (!images.empty()) code << "            image_textures: [None; " << images.size() << "],\n";
    for (const auto& line : emitter.initializers) code << line;
    code << "        }\n    }\n}\n\npub fn apply_style(style: &mut imgui::Style) {\n";
    const auto& s = project.style;
    code << "    style.use_" << (s.preset == StylePresetId::ImGuiLight ? "light" :
      s.preset == StylePresetId::ImGuiClassic ? "classic" : "dark") << "_colors();\n";
    using RustCodegen::number;
    code << "    style.window_rounding = " << number(s.windowRounding) << ";\n"
         << "    style.frame_rounding = " << number(s.frameRounding) << ";\n"
         << "    style.window_padding = " << RustCodegen::vec(s.windowPaddingX, s.windowPaddingY) << ";\n"
         << "    style.item_spacing = " << RustCodegen::vec(s.itemSpacingX, s.itemSpacingY) << ";\n";
    for (const auto& entry : {std::pair<const char*, Color4>{"WindowBg", s.windowBg}, {"TitleBgActive", s.titleBg},
         {"Button", s.buttonColor}, {"FrameBg", s.frameBg}, {"Text", s.textColor}})
      code << "    style[imgui::StyleColor::" << entry.first << "] = " << RustCodegen::color(entry.second) << ";\n";
    const auto style = ProjectJson::styleJson(s);
    const std::pair<const char*, const char*> scalars[] = {
      {"alpha", "alpha"}, {"childRounding", "child_rounding"}, {"grabRounding", "grab_rounding"},
      {"tabRounding", "tab_rounding"}, {"scrollbarRounding", "scrollbar_rounding"},
      {"windowBorderSize", "window_border_size"}, {"frameBorderSize", "frame_border_size"}
    };
    for (const auto& field : scalars)
      if (style.contains(field.first) && style.at(field.first).is_number())
        code << "    style." << field.second << " = " << number(style.at(field.first).get<float>()) << ";\n";
    if (style.contains("framePaddingX") && style.at("framePaddingX").is_number() &&
        style.contains("framePaddingY") && style.at("framePaddingY").is_number())
      code << "    style.frame_padding = " << RustCodegen::vec(style.at("framePaddingX").get<float>(),
        style.at("framePaddingY").get<float>()) << ";\n";
    if (style.contains("colors") && style.at("colors").is_object()) {
      const auto& values = style.at("colors");
      const std::pair<const char*, const char*> colors[] = {
        {"childBg", "ChildBg"}, {"titleBg", "TitleBg"}, {"frameBgHovered", "FrameBgHovered"},
        {"buttonHovered", "ButtonHovered"}, {"buttonActive", "ButtonActive"}, {"checkMark", "CheckMark"},
        {"sliderGrab", "SliderGrab"}, {"header", "Header"}, {"headerHovered", "HeaderHovered"},
        {"textDisabled", "TextDisabled"}, {"border", "Border"}, {"separator", "Separator"}
      };
      for (const auto& entry : colors) {
        if (!values.contains(entry.first) || !values.at(entry.first).is_string()) continue;
        try {
          const auto value = RustCodegen::color(ProjectJson::colorFromHex(values.at(entry.first).get<std::string>()));
          code << "    style[imgui::StyleColor::" << entry.second << "] = " << value << ";\n";
        } catch (const std::exception&) {
          emitter.result.diagnostics.push_back({"", std::string("Color de estilo incompatible: ") + entry.first});
        }
      }
    }
    code << "}\n\n";
    if (emitter.hasModelViews) code << R"RS(fn model_label(ui: &imgui::Ui, text: &str, centered: bool, featured: bool) {
    if !ui.is_item_visible() { return; }
    let (lo, hi) = (ui.item_rect_min(), ui.item_rect_max());
    let style = ui.clone_style();
    let extent = ui.calc_text_size(text);
    let pad = style.frame_padding[0];
    let x = if centered { pad.max((hi[0] - lo[0] - extent[0]) * 0.5) } else { pad };
    let y = ((hi[1] - lo[1] - extent[1]) * 0.5).max(0.0);
    let draw = ui.get_window_draw_list();
    if featured {
        draw.add_rect(lo, hi, ui.style_color(imgui::StyleColor::Header))
            .rounding(style.frame_rounding).filled(true).build();
    }
    draw.with_clip_rect_intersect([lo[0] + pad, lo[1]], [hi[0] - pad, hi[1]], || {
        draw.add_text([lo[0] + x, lo[1] + y], ui.style_color(imgui::StyleColor::Text), text);
    });
}

fn model_rows(ui: &imgui::Ui, items: &[&str], value: &mut i32, columns: usize, grid: bool) {
    let selected = (*value).clamp(0, items.len() as i32 - 1) as usize;
    let height = ui.frame_height() * if grid { 1.5 } else { 1.0 };
    let rows = (items.len() + columns - 1) / columns;
    let mut clip = imgui::ListClipper::new(rows as i32).begin(ui);
    while clip.step() {
        for row in clip.display_start() as usize..clip.display_end() as usize {
            if grid { ui.table_next_row(); }
            let start = row * columns;
            let end = (start + columns).min(items.len());
            for (index, item) in items.iter().enumerate().take(end).skip(start) {
                if grid { ui.table_next_column(); }
                let _id = ui.push_id_usize(index);
                if ui.selectable_config("##model_option").selected(index == selected).size([0.0, height]).build() {
                    *value = index as i32;
                }
                model_label(ui, item, grid, false);
            }
        }
    }
}

fn draw_model_view(ui: &imgui::Ui, label: &str, items: &[&str], value: &mut i32, size: [f32; 2],
                   border: bool, columns: usize, spacing: f32, path: bool) {
    let _background = ui.push_style_color(imgui::StyleColor::ChildBg, ui.style_color(imgui::StyleColor::FrameBg));
    if let Some(_child) = ui.child_window("model").size(size).border(border).always_use_window_padding(true).begin() {
        if !label.is_empty() { ui.text(label); }
        if items.is_empty() {
            ui.text_disabled("Sin elementos");
        } else if path {
            let count = items.len() as i32;
            let selected = value.rem_euclid(count);
            ui.text_disabled(format!("{} / {}", selected + 1, count));
            let arrow = ui.frame_height();
            let spacing = ui.clone_style().item_spacing[0];
            let width = (ui.content_region_avail()[0] - 2.0 * (arrow + spacing)).max(1.0);
            ui.disabled(count == 1, || {
                if ui.arrow_button("previous", imgui::Direction::Left) { *value = (selected - 1).rem_euclid(count); }
            });
            ui.same_line();
            ui.dummy([width, arrow]);
            model_label(ui, items[selected as usize], true, true);
            ui.same_line();
            ui.disabled(count == 1, || {
                if ui.arrow_button("next", imgui::Direction::Right) { *value = (selected + 1) % count; }
            });
        } else if columns > 0 {
            let _padding = ui.push_style_var(imgui::StyleVar::CellPadding([spacing * 0.5, spacing * 0.5]));
            if let Some(_table) = ui.begin_table_with_flags("model_cells", columns, imgui::TableFlags::SIZING_STRETCH_SAME) {
                model_rows(ui, items, value, columns, true);
            }
        } else {
            model_rows(ui, items, value, 1, false);
        }
    }
}

)RS";
    if (emitter.hasChart2d || emitter.hasChart3d) code << RustCodegen::chartSampleFunctions;
    if (emitter.hasChart2d) code << RustCodegen::chart2dFunctions;
    if (emitter.hasChart3d) code << RustCodegen::chart3dFunctions;
    if (emitter.hasAnimation) code << RustCodegen::animationFunctions;
    if (emitter.hasImageWidgets) code << RustCodegen::imageDrawing;
    if (emitter.result.hasTitleButtons) code << RustCodegen::titleBarFunctions;
    if (!images.empty()) {
      code << "const IMAGE_ASSETS: &[(&[u8], u32, u32)] = &[\n";
      for (size_t i = 0; i < images.size(); ++i)
        code << "    (include_bytes!(\"studio_image_" << i << ".bin\"), "
             << images[i].size.width << ", " << images[i].size.height << "),\n";
      code << "];\n" << RustCodegen::imageLoading;
    }
    code << "#[allow(unused_variables)]\npub fn render_ui(ui: &imgui::Ui, state: &mut UiState) {\n";
    if (emitter.hasRepeater) code << "    let mut repeat_budget = " << ProjectJson::kMaxNodes << ";\n";
    for (const auto& button : emitter.frameButtons) code << "    " << button << " = false;\n";
    const auto prefix = code.str();
    const auto offset = static_cast<size_t>(std::count(prefix.begin(), prefix.end(), '\n'));
    for (auto& entry : emitter.result.nodeLines) { entry.second.first += offset; entry.second.second += offset; }
    for (const auto& line : emitter.lines) code << line << '\n';
    code << "}\n";
    for (const auto& d : emitter.result.diagnostics)
      code << "// [" << CppCodegen::comment(d.nodeId) << "] " << CppCodegen::comment(d.message) << '\n';
    emitter.result.stateType = "UiState";
    emitter.result.code = code.str();
    if (imageResources)
      for (const auto& asset : images) imageResources->push_back(asset.bytes);
  } catch (const std::exception& error) { emitter.result.error = error.what(); }
  return std::move(emitter.result);
}

inline CppBundle generateRustBundle(const StudioProject& project) {
  CppBundle bundle;
  std::vector<std::string> imageResources;
  bundle.generated = generateRustCode(project, &imageResources);
  const auto& generated = bundle.generated;
  if (!generated.error.empty()) return bundle;
  std::string report = "Rust / imgui-rs 0.12.0\n";
  for (const auto& d : generated.diagnostics)
    report += "[" + CppCodegen::comment(d.nodeId) + "] " + CppCodegen::comment(d.message) + "\n";
  report += "\nRangos de lineas en ui.rs (inclusivos):\n";
  for (const auto& entry : generated.nodeLines)
    report += CppCodegen::comment(entry.first) + ": " + std::to_string(entry.second.first) +
      "-" + std::to_string(entry.second.second) + "\n";
  std::string main = R"RUST(use imgui_glfw_rs::{glfw, ImguiGLFW};
use glfw::Context as _;
use studio_export::{apply_style, render_ui, UiState};
)RUST";
  if (generated.hasImageTextures) main += RustCodegen::imageBackend;
  main += R"RUST(
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let smoke = std::env::args().skip(1).any(|arg| arg == "--smoke-test");
    let mut glfw = glfw::init(glfw::log_errors)?;
    glfw.window_hint(glfw::WindowHint::ContextVersion(3, 3));
    glfw.window_hint(glfw::WindowHint::OpenGlProfile(glfw::OpenGlProfileHint::Core));
    if cfg!(target_os = "macos") {
        glfw.window_hint(glfw::WindowHint::OpenGlForwardCompat(true));
    }
    glfw.window_hint(glfw::WindowHint::Visible(!smoke));
    let (mut window, events) = glfw
)RUST";
  main += "        .create_window(" + std::to_string(project.canvas.width) + ", " +
    std::to_string(project.canvas.height) + ", " +
    RustCodegen::literal(project.name.substr(0, project.name.find('\0'))) + ", glfw::WindowMode::Windowed)\n";
  main += R"RUST(
        .ok_or("GLFW could not create an OpenGL 3.3 window")?;
    window.make_current();
    window.set_all_polling(true);
    glfw.set_swap_interval(glfw::SwapInterval::Sync(1));
    gl::load_with(|symbol| window.get_proc_address(symbol).map_or(std::ptr::null(), |f| f as *const _));
    let mut imgui = imgui::Context::create();
    if smoke { imgui.set_ini_filename(None); }
    apply_style(imgui.style_mut());
    let mut backend = ImguiGLFW::new(&mut imgui, &mut window)?;
    let mut state = UiState::default();
)RUST";
  if (generated.hasImageTextures)
    main += "    let mut images = ImageBackend::default();\n"
            "    let images_loaded = studio_export::load_images(&mut state, |data, width, height| images.upload(data, width, height));\n";
  main += R"RUST(    let mut frames = 0;
    while !window.should_close() {
        glfw.poll_events();
        for (_, event) in glfw::flush_messages(&events) {
            backend.handle_event(&mut imgui, &event);
        }
        render_ui(backend.frame(&mut window, &mut imgui), &mut state);
        let (width, height) = window.get_framebuffer_size();
        // The OpenGL context is current on this thread for all GL calls.
        unsafe {
            gl::Viewport(0, 0, width, height);
            gl::ClearColor(0.08, 0.09, 0.12, 1.0);
            gl::Clear(gl::COLOR_BUFFER_BIT);
        }
        backend.draw(&mut imgui, &mut window);
        window.swap_buffers();
        if smoke {
            frames += 1;
            if frames == 3 { break; }
        }
    }
)RUST";
  if (generated.hasImageTextures)
    main += "    studio_export::release_images(&mut state, |id, width, height| images.release(id, width, height));\n"
            "    if smoke && !images_loaded { return Err(\"Unable to upload embedded images\".into()); }\n";
  main += R"RUST(
    if smoke {
        if frames != 3 { return Err("Smoke test ended before three frames".into()); }
        println!("Rust imgui-rs rendered three frames.");
    }
    Ok(())
}
)RUST";
  bundle.files = {
    {"ui.rs", generated.code},
    {"Cargo.toml", R"TOML([package]
name = "studio-export"
version = "0.1.0"
edition = "2021"
rust-version = "1.88"

[lib]
path = "ui.rs"

[[bin]]
name = "studio-export"
path = "main.rs"
required-features = ["desktop"]

[features]
default = ["desktop"]
desktop = ["dep:gl", "dep:glfw", "dep:imgui-glfw-rs"]

[dependencies]
imgui = { version = "=0.12.0", features = ["tables-api"] }
imgui-glfw-rs = { version = "=0.13.1", features = ["opengl"], optional = true }
glfw = { version = "=0.61.0", default-features = false, features = ["src-build", "static-link"], optional = true }
gl = { version = "=0.14.0", optional = true }
)TOML"},
    {"main.rs", main},
    {"codegen_report.txt", report},
    {"README.md", R"TXT(# Exportación Rust

Con Rust, Cargo y un toolchain C++ compatible, ejecuta cargo build y cargo run.
El proyecto usa imgui-rs 0.12.0, GLFW enlazado estáticamente y OpenGL 3.3.
La primera compilación puede obtener crates de crates.io y las bibliotecas
precompiladas de GLFW en Windows y macOS. En Windows MSVC requiere las herramientas
de C++ y el SDK de Windows. Linux necesita las bibliotecas de desarrollo de
ventanas y gráficos de GLFW. El núcleo en ui.rs se puede integrar en otro
host de imgui-rs y compilar con cargo build --no-default-features.

cargo run -- --smoke-test abre un contexto oculto y dibuja tres fotogramas.
Consulta codegen_report.txt para las omisiones y rangos por widget.
)TXT"}
  };
  if (generated.hasImageTextures) {
    auto& manifest = bundle.files["Cargo.toml"];
    manifest.insert(manifest.find(R"TOML("dep:imgui-glfw-rs"])TOML") + std::string_view("\"dep:imgui-glfw-rs\"").size(),
                    ", \"dep:image\"");
    manifest += "image = { version = \"=0.25.10\", default-features = false, features = [\"png\", \"jpeg\"], optional = true }\n";
    for (size_t index = 0; index < imageResources.size(); ++index)
      bundle.files["studio_image_" + std::to_string(index) + ".bin"] = std::move(imageResources[index]);
    bundle.files["README.md"] += "\nCargo incrusta los PNG y JPEG mediante include_bytes!. Conserva los .bin al compilar.\n"
      "Para integrar otro backend, usa load_images y release_images con tus callbacks.\n"
      "El campo booleano de cada botón de imagen indica el clic después de render_ui.\n";
  }
  if (generated.hasTitleButtons) {
    bundle.files["Cargo.toml"] += "imgui-sys = \"=0.12.0\"\n\n[build-dependencies]\ncc = \"=1.11.3\"\n";
    bundle.files["build.rs"] = RustCodegen::titleBarBuildScript;
    bundle.files["studio_title_bar.cpp"] = RustCodegen::titleBarBridge();
    bundle.files["README.md"] += "\nLos botones de título usan el puente studio_title_bar.cpp y el Dear ImGui de imgui-sys.\n"
      "Conserva el puente y build.rs al integrar ui.rs. Consulta el booleano del botón después de render_ui.\n";
  }
  return bundle;
}

inline ProjectSaveResult exportRustBundle(const StudioProject& project, const std::filesystem::path& destination) {
  const auto bundle = generateRustBundle(project);
  if (!bundle.generated.error.empty()) return {false, bundle.generated.error};
  return writeGeneratedBundle(bundle, destination);
}
} // namespace ImGuiStudio
