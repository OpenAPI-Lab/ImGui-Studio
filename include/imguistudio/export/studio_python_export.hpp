#pragma once
#include "studio_cpp_export.hpp"
#include "imguistudio/export/detail/studio_chart2d_python.hpp"
#include "imguistudio/export/detail/studio_chart3d_python.hpp"
#include "imguistudio/export/detail/studio_managed_animation.hpp"
#include "imguistudio/export/detail/studio_managed_images.hpp"
#include "imguistudio/widgets/studio_image_data.hpp"
#include "imguistudio/export/detail/studio_titlebar_python.hpp"

namespace ImGuiStudio {
namespace PythonCodegen {
inline std::string literal(const std::string& value) { return Json(value).dump(); }
inline std::string number(float value) {
  auto text = CppCodegen::number(value);
  text.pop_back();
  return text;
}
inline std::string boolean(bool value) { return value ? "True" : "False"; }
inline std::string pair(float x, float y) { return "(" + number(x) + ", " + number(y) + ")"; }
inline std::string color(const Color4& c, bool alpha = true) {
  return "[" + number(c.r) + ", " + number(c.g) + ", " + number(c.b) + (alpha ? ", " + number(c.a) : "") + "]";
}
struct Emitter {
  GeneratedCpp result;
  std::vector<std::string> lines;
  bool hasModelViews = false;
  bool hasRepeater = false;
  bool hasChart2d = false;
  bool hasChart3d = false;
  bool hasAnimation = false;
  bool hasImageWidgets = false;
  ImageData::ExportAssets imageAssets;
  std::vector<std::string> frameButtons;
  std::set<std::string> titleItems;
  bool hasWindows = false;
  std::set<std::string> scaledNodes;
  static std::string function(const ImGuiNode& n) { return "_draw_" + CppCodegen::hexId(n.id); }
  static std::string titleLabel(const std::string& label) {
    const auto text = label.substr(0, label.find('\0'));
    return literal(text.substr(0, text.find("##")));
  }
  static std::string titleOption(const std::string& value, std::initializer_list<std::string_view> known,
                                 const char* fallback) {
    return std::find(known.begin(), known.end(), value) == known.end() ? std::string(fallback) : value;
  }
  void emitTitleItem(const ImGuiNode& child, bool show) {
    const auto first = lines.size() + 1;
    const auto key = literal("node:" + child.id);
    const int kind = CppCodegen::titleKind(child);
    if (!kind) frameButtons.push_back("state[" + key + "]");
    if (!show) lines.push_back("        # Title item hidden by the window flags.");
    else {
      const auto& p = child.props;
      const auto side = titleOption(p.titleBarSide, {"left", "center", "right", "free"}, "left");
      const auto variant = titleOption(p.titleBarBtnVariant, {"icon", "pill", "outline", "solid"}, "pill");
      const auto icon = kind ? "none" : titleOption(p.titleBarIcon, {"none", "panel_left", "panel_right",
        "sparkles", "edit", "plus", "settings", "search", "terminal", "bot", "pin", "bell"}, "none");
      const auto overrideColor = [&](const char* name) -> std::string {
        if (kind || !p.source.is_object() || !p.source.contains(name) || !p.source.at(name).is_string()) return "None";
        try { return "imgui.ImVec4(*" + color(ProjectJson::colorFromHex(p.source.at(name).get<std::string>())) + ")"; }
        catch (const std::exception&) {
          result.diagnostics.push_back({child.id, std::string("Color incompatible: ") + name});
          return "None";
        }
      };
      auto width = number(child.width);
      if (kind == 11 && child.width <= 0) {
        width = "2 * imgui.get_style().item_spacing.x";
        for (const auto& item : p.items)
          width += " + imgui.calc_text_size(" + titleLabel(item) + ").x + imgui.get_style().item_spacing.x";
        if (p.items.empty()) width += " + imgui.calc_text_size('Menú vacío').x";
      }
      lines.push_back("            (" + key + ", " + titleLabel(kind == 8 ? p.text : child.label) + ", " +
        literal(side) + ", " + literal(variant) + ", " + literal(icon) + ", " + width + ", " +
        number(child.height) + ", " + number(p.titleBarX) + ", " + (p.titleBarY ? number(*p.titleBarY) : "None") +
        ", " + number(p.titleBarBtnRounding.value_or(variant == "pill" ? 999.0f : 6.0f)) +
        ", " + overrideColor("buttonColorOverride") + ", " + overrideColor("textColorOverride") + ", " +
        std::to_string(kind) + ", " + (kind ? function(child) : "None") + ", " +
        overrideColor("buttonHoveredColorOverride") + ", " + overrideColor("buttonActiveColorOverride") + "),");
      if (kind) titleItems.insert(child.id);
      else if (!p.buttonAction.empty())
        result.diagnostics.push_back({child.id,"La accion de boton requiere integracion de la aplicacion"});
    }
    if (!show || !kind) result.nodeLines[child.id] = {first, lines.size()};
  }
  void emit(const ImGuiNode& n, bool flowChild = false) {
    if (result.nodeLines.count(n.id)) return; // Already emitted in the parent window's title bar.
    const auto first = lines.size() + 1;
    const bool scaled = scaledNodes.count(n.id) != 0;
    const bool titleItem = titleItems.count(n.id) != 0;
    lines.push_back("def " + function(n) + (titleItem ? "(state, title_size):" : scaled ? "(state, scale):" : "(state):"));
    const std::string indent = "    ";
    auto put = [&](const std::string& s, int extra = 0) { lines.push_back(indent + std::string(static_cast<size_t>(extra)*4,' ') + s); };
    auto warn = [&](const std::string& s) { result.diagnostics.push_back({n.id,s}); };
    auto childCall = [&](const ImGuiNode& child, const std::string& scale = "scale") {
      return function(child) + (scaledNodes.count(child.id) ? "(state, " + scale + ")" : "(state)");
    };
    auto children = [&](int extra, const std::string& scale = "scale") {
      const auto before = lines.size();
      for (const auto& child : n.children)
        if (child.visible && (n.type != WidgetType::Window || !WindowChrome::isTitleItem(child)))
          put(childCall(child, scale),extra);
      if (before == lines.size()) put("pass",extra);
    };
    const auto& p = n.props;
    const auto id = CppCodegen::hexId(n.id);
    const auto label = literal(n.label.substr(0,n.label.find('\0')) + "###studio_" + id);
    const auto layout = [&](float value) { return number(value) + (scaled && value != 0 ? " * scale" : ""); };
    const auto dimensions = [&](float x, float y) { return "(" + layout(x) + ", " + layout(y) + ")"; };
    const auto width = n.width < 0 ? number(-std::numeric_limits<float>::min()) : layout(n.width);
    const auto size = "(" + width + ", " + layout(std::max(0.0f,n.height)) + ")";
    const auto props = ProjectJson::propsJson(p,n.type);
    const auto key = literal(n.type == WidgetType::RadioButton ? "radio:" + n.varName : "node:" + n.id);
    const auto state = "state[" + key + "]";
    auto call = [&](const std::string& name,const std::string& args = {}) { put("imgui."+name+"("+args+")"); };
    auto change = [&](const std::string& name,const std::string& args) {
      put(state + " = imgui."+name+"("+args+")[1]");
    };
    auto initial = [&](const std::string& value) { return "state.setdefault("+key+", "+value+")"; };
    auto format = [&](bool integral,const std::string& fallback) {
      if (!p.format.empty() && !safeNumericFormat(p.format,integral)) warn("Formato numerico incompatible; se usa el predeterminado");
      return literal(safeNumericFormat(p.format,integral) ? p.format : fallback);
    };
    if (n.type == WidgetType::Window) {
      hasWindows = true;
      static const std::map<std::string,std::string> supported = {
        {"NoTitleBar","no_title_bar"},{"NoResize","no_resize"},{"NoMove","no_move"},{"NoCollapse","no_collapse"},
        {"NoScrollbar","no_scrollbar"},{"NoScrollWithMouse","no_scroll_with_mouse"},{"AlwaysAutoResize","always_auto_resize"},
        {"NoSavedSettings","no_saved_settings"},{"NoMouseInputs","no_mouse_inputs"},{"MenuBar","menu_bar"},
        {"NoBackground","no_background"},{"HorizontalScrollbar","horizontal_scrollbar"},
        {"NoFocusOnAppearing","no_focus_on_appearing"},{"NoBringToFrontOnFocus","no_bring_to_front_on_focus"},
        {"AlwaysVerticalScrollbar","always_vertical_scrollbar"},{"AlwaysHorizontalScrollbar","always_horizontal_scrollbar"},
        {"NoNavInputs","no_nav_inputs"},{"NoNavFocus","no_nav_focus"},{"UnsavedDocument","unsaved_document"},
        {"NoNav","no_nav"},{"NoDecoration","no_decoration"},{"NoInputs","no_inputs"}
      };
      const auto& a = p.windowAppearance;
      std::string flags = "0";
      bool collapse = p.showCollapseArrow && !p.noCollapse;
      for (const auto& flag : ProjectJson::strings(props,"windowFlags",{})) {
        const auto name = flag.rfind("ImGuiWindowFlags_",0)==0 ? flag.substr(17) : flag;
        const auto it = supported.find(name);
        if (it == supported.end()) warn("Flag de ventana pendiente: "+flag);
        else flags += " | imgui.WindowFlags_."+it->second;
        collapse &= name != "NoCollapse";
      }
      if (!collapse) flags += " | imgui.WindowFlags_.no_collapse";
      const bool titleHidden = CppCodegen::hidesTitle(n);
      const bool customTitle = !titleHidden && WindowChrome::usesCustomTitle(n);
      const bool inset = !titleHidden && WindowChrome::hasTitleInsets(a);
      const bool modes = !titleHidden && CppCodegen::hasWindowModes(p);
      const auto viewSize = pair(std::max(0.0f,n.width),std::max(0.0f,n.height));
      result.hasTitleButtons |= customTitle;
      put("if "+initial("True")+":");
      int styleVars = 0, colors = 0;
      const auto variable = [&](const char* name, std::optional<float> value, float min, float max, const char* axis = "") {
        if (!value) return;
        put("imgui.push_style_var" + std::string(axis) + "(imgui.StyleVar_." + name +
          ", " + number(std::clamp(*value, min, max)) + ")", 1);
        ++styleVars;
      };
      const auto tint = [&](const char* name, const std::optional<Color4>& value) {
        if (!value) return;
        put("imgui.push_style_color(imgui.Col_." + std::string(name) + ", imgui.ImVec4(*" + color(*value) + "))", 1);
        ++colors;
      };
      variable("alpha", a.alpha, 0, 1);
      variable("window_rounding", a.rounding, 0, 72);
      variable("window_border_size", p.noBackground ? std::optional<float>{0} : a.borderSize, 0, 12);
      variable("window_padding", a.paddingX, 0, 256, "_x");
      variable("window_padding", a.paddingY, 0, 256, "_y");
      tint("window_bg", a.background); tint("border", a.borderColor); tint("text", a.textColor);
      if (!a.showResizeGrip)
        for (const auto slot : {"resize_grip","resize_grip_hovered","resize_grip_active"}) tint(slot, Color4{0,0,0,0});
      const auto titleTint = a.transparentTitle ? std::optional<Color4>{Color4{0,0,0,0}} : a.titleBackground;
      for (const auto slot : {"title_bg","title_bg_active","title_bg_collapsed"}) tint(slot, titleTint);
      for (const auto& field : WindowChrome::colors)
        if (props.contains(field.key) && !(a.*field.member)) warn(std::string("Color de ventana incompatible: ") + field.key);

      if (!titleHidden && a.titleHeight)
        put("imgui.push_style_var_y(imgui.StyleVar_.frame_padding, max(0, (" +
          number(std::clamp(*a.titleHeight,20.0f,72.0f)) + " - imgui.get_font_size()) * 0.5))", 1);
      if (modes) {
        put("mode = state.setdefault(" + literal("window-mode:" + n.id) + ", {})", 1);
        put("active_mode = mode.get('active')", 1);
        put("minimized = active_mode == 'minimize'", 1);
        put("if minimized: imgui.push_style_var_y(imgui.StyleVar_.window_min_size, 1.0)", 1);
        put("_studio_window_geometry(mode, "+pair(n.x,n.y)+", "+viewSize+")",1);
        flags += " | ((imgui.WindowFlags_.no_move | imgui.WindowFlags_.no_resize | imgui.WindowFlags_.no_collapse) if active_mode else 0)";
      } else {
        put("imgui.set_next_window_pos("+pair(n.x,n.y)+", imgui.Cond_.first_use_ever)",1);
        put("imgui.set_next_window_size("+viewSize+", imgui.Cond_.first_use_ever)",1);
      }
      for (const auto& child : n.children)
        if (child.visible && child.type == WidgetType::MenuBar && child.props.menuBarEdge == MenuBarEdge::Top) {
          flags += modes ? " | (0 if minimized else imgui.WindowFlags_.menu_bar)" : " | imgui.WindowFlags_.menu_bar";
          break;
        }
      put("expanded = _begin_studio_window("+(customTitle ? literal("###studio_" + id) : label)+
          ", "+flags+", "+boolean(inset)+")",1);
      if (modes) put("if minimized: imgui.pop_style_var()",1);
      if (!titleHidden && a.titleHeight) put("imgui.pop_style_var()",1);
      const auto shadow = titleOption(a.shadow, {"none", "soft", "deep", "glow"}, "none");
      if (shadow != "none") put("_studio_window_shadow("+literal(shadow)+")", 1);

      const auto side = titleOption(p.windowControlsSide, {"left", "center", "right"}, "right");
      const auto variant = titleOption(p.windowControlsVariant, {"icons", "traffic_lights", "pills", "classic"}, "icons");
      const auto control = [&](const char* name, int kind, const std::string& group) {
        put("("+literal(std::string("##studio_window_")+name)+", '', "+literal(group)+
            ", '', 'none', 0, 0, 0, None, 0, None, None, "+std::to_string(kind)+", None, None, None),", 2);
      };
      const auto controls = [&] {
        if (variant == "traffic_lights" && p.closable) control("close", 1, side);
        if (p.showMinimizeButton) control("minimize", 2, side);
        if (p.showMaximizeButton) control("maximize", 3, side);
        if (variant != "traffic_lights" && p.closable) control("close", 1, side);
      };
      if (customTitle) {
        put("_render_title_bar(state, "+titleLabel(p.showTitleText ? n.label : "")+", (",1);
        if (inset && collapse) control("collapse", 4, "left");
        if (side != "right") controls();
      }
      for (const auto& child : n.children) {
        if (!child.visible) continue;
        if (WindowChrome::isTitleItem(child)) emitTitleItem(child, customTitle);
        else if (child.props.inTitleBar)
          result.diagnostics.push_back({child.id, "Control no compatible con la barra de título. Se conserva en el cuerpo."});
      }
      if (customTitle) {
        if (side == "right") controls();
        const auto alignment = titleOption(p.titleAlign, {"left", "center", "right"}, "left");
        const auto icon = titleOption(a.titleIcon, {"none", "bot", "app", "terminal", "sparkles", "folder", "layers"}, "none");
        const std::string separator = !a.titleSeparator ? "None" : a.separatorColor ?
          "imgui.ImVec4(*"+color(*a.separatorColor)+")" : "imgui.Col_.border";
        put("), (" + number(std::clamp(a.titleOffsetLeft.value_or(0),0.0f,4096.0f)) + ", " +
          number(std::clamp(a.titleOffsetRight.value_or(0),0.0f,4096.0f)) + ", " +
          (alignment == "left" ? "0.0" : alignment == "center" ? "0.5" : "1.0") + ", " + literal(icon) + ", " +
          separator + ", " + boolean(a.transparentTitle) + ", " + literal(variant) + "), " +
          key + ", " + (modes ? "mode" : "{}") + ")", 1);
      }
      put(modes ? "if expanded and not minimized:" : "if expanded:",1);
      children(2);
      put("imgui.end()",1);
      if (colors) put("imgui.pop_style_color("+std::to_string(colors)+")",1);
      if (styleVars) put("imgui.pop_style_var("+std::to_string(styleVars)+")",1);
    } else {
      call("push_id",literal(id));
      if (!titleItem) {
        if (n.useCursorPos) call("set_cursor_pos",dimensions(n.x,n.y));
        else if (n.sameLine && !flowChild) call("same_line",n.sameLineSpacing <= 0 ? "" : "0.0, "+layout(n.sameLineSpacing));
        if (n.indent > 0) call("indent",layout(n.indent));
      }
      int colors = 0;
      const auto widgetColor = [&](const char* key, const char* slot) {
        if (props.contains(key) && props.at(key).is_string()) {
          try {
            call("push_style_color","imgui.Col_."+std::string(slot)+", imgui.ImVec4(*"+color(ProjectJson::colorFromHex(props.at(key).get<std::string>()))+")");
            ++colors;
          } catch (const std::exception&) { warn(std::string("Color incompatible: ")+key); }
        }
      };
      widgetColor("textColorOverride", "text");
      widgetColor("buttonColorOverride", "button");
      if (titleItem)
        for (const auto& entry : CppCodegen::titleWidgetColors) widgetColor(entry.key, entry.python);
      auto control = [&](const std::string& fn,const std::string& value,const std::string& tail = {}) {
        if (n.width != 0) call("set_next_item_width",width);
        change(fn,label+", "+initial(value)+tail);
      };
      const auto floatRange = ", "+number(std::min(p.minFloat,p.maxFloat))+", "+number(std::max(p.minFloat,p.maxFloat));
      const auto intRange = ", "+std::to_string(std::min(p.minInt,p.maxInt))+", "+std::to_string(std::max(p.minInt,p.maxInt));
      const auto speed = number(std::max(0.0f,p.speed));
      switch (n.type) {
        case WidgetType::Text: call("text_unformatted",literal(p.text)); break;
        case WidgetType::TextColored:
          call("push_style_color","imgui.Col_.text, imgui.ImVec4(*"+color(p.color)+")");
          call("text_unformatted",literal(p.text)); call("pop_style_color"); break;
        case WidgetType::TextWrapped:
          call("push_text_wrap_pos","0.0"); call("text_unformatted",literal(p.text)); call("pop_text_wrap_pos"); break;
        case WidgetType::BulletText:
          call("bullet"); call("text_unformatted",literal(p.text)); break;
        case WidgetType::HelpMarker:
          call("text_disabled",literal("(?)")); put("if imgui.begin_item_tooltip():");
          put("imgui.text_unformatted("+literal(p.text)+")",1); put("imgui.end_tooltip()",1); break;
        case WidgetType::Button: call("button",label+", "+(titleItem ? "title_size" : size)); break;
        case WidgetType::SmallButton: call("small_button",label); break;
        case WidgetType::ColorButton: call("color_button",label+", "+color(p.color)+", 0, "+size); break;
        case WidgetType::ArrowButton: {
          const auto direction = p.arrowDir == "left" || p.arrowDir == "up" || p.arrowDir == "down" ? p.arrowDir : "right";
          call("arrow_button",label+", imgui.Dir_."+direction); break;
        }
        case WidgetType::Checkbox: control("checkbox",boolean(p.checked)); break;
        case WidgetType::Selectable: control("selectable",boolean(p.selected),", 0, "+size); break;
        case WidgetType::RadioButton: control("radio_button",std::to_string(p.intValue),", "+std::to_string(p.radioValue)); break;
        case WidgetType::InputText:
          if (p.text.find('\0') != std::string::npos) warn("El control de texto solo muestra el contenido hasta el primer NUL");
          if (titleItem) call("set_next_item_width","title_size[0]");
          else if (n.width != 0) call("set_next_item_width",layout(n.width));
          change("input_text_with_hint",label+", "+literal(p.hint)+", "+initial(literal(p.text))); break;
        case WidgetType::InputMultiline:
          if (p.text.find('\0') != std::string::npos) warn("El control de texto solo muestra el contenido hasta el primer NUL");
          control("input_text_multiline",literal(p.text),", "+size); break;
        case WidgetType::InputFloat: control("input_float",number(p.floatValue),", "+speed+", 0.0, "+format(false,"%.3f")); break;
        case WidgetType::InputInt: {
          const int step = static_cast<int>(std::clamp(static_cast<double>(p.speed),1.0,100000000.0));
          control("input_int",std::to_string(p.intValue),", "+std::to_string(step)+", "+std::to_string(step*10));
          if (!p.format.empty() && p.format != "%d") warn("InputInt de Python usa formato decimal");
          break;
        }
        case WidgetType::SliderFloat: control("slider_float",number(p.floatValue),floatRange+", "+format(false,"%.3f")); break;
        case WidgetType::SliderAngle: control("slider_angle",number(p.floatValue),floatRange+", "+format(false,"%.0f deg")); break;
        case WidgetType::SliderInt: control("slider_int",std::to_string(p.intValue),intRange+", "+format(true,"%d")); break;
        case WidgetType::DragFloat: control("drag_float",number(p.floatValue),", "+speed+floatRange+", "+format(false,"%.3f")); break;
        case WidgetType::DragInt: control("drag_int",std::to_string(p.intValue),", "+speed+intRange+", "+format(true,"%d")); break;
        case WidgetType::SliderFloat2: case WidgetType::SliderFloat3: case WidgetType::DragFloat3: {
          const size_t count = n.type == WidgetType::SliderFloat2 ? 2 : 3;
          std::string values = "[";
          for (size_t i=0;i<count;++i) values += (i ? ", " : "")+number(i<p.defaultVec.size() ? p.defaultVec[i] : 0);
          if (p.defaultVec.size()>count) warn("Componentes adicionales conservados solo en el proyecto");
          control(n.type == WidgetType::SliderFloat2 ? "slider_float2" : n.type == WidgetType::SliderFloat3 ? "slider_float3" : "drag_float3",
            values+"]",(n.type == WidgetType::DragFloat3 ? ", "+speed : "")+floatRange+", "+format(false,"%.3f"));
          break;
        }
        case WidgetType::ColorEdit3: case WidgetType::ColorEdit4:
          control(n.type == WidgetType::ColorEdit3 ? "color_edit3" : "color_edit4",color(p.color,n.type == WidgetType::ColorEdit4)); break;
        case WidgetType::Combo: case WidgetType::Listbox:
          if (p.items.empty()) call("text_disabled",literal(n.label+" (sin opciones)"));
          else control(n.type == WidgetType::Combo ? "combo" : "list_box",std::to_string(p.selectedIndex),", "+Json(p.items).dump());
          break;
        case WidgetType::ListView: case WidgetType::GridView: case WidgetType::PathView: {
          hasModelViews = true;
          const bool grid = n.type == WidgetType::GridView;
          const auto modelSize = "(" + width + ", " +
            layout(n.height > 0 ? n.height : modelViewDefaultHeight(n.type)) + ")";
          std::string items = "(";
          for (const auto& label : p.items) items += literal(label) + ", ";
          items += ")";
          put("_draw_model_view(state, "+key+", "+literal(n.label)+", "+items+", "+
              std::to_string(p.selectedIndex)+", "+modelSize+", "+boolean(p.border)+", "+
              std::to_string(grid ? std::clamp(p.columns,1,kMaxModelColumns) : 0)+", "+
              layout(std::clamp(p.spacing,0.0f,32.0f))+", "+boolean(n.type == WidgetType::PathView)+")");
          break;
        }
        case WidgetType::PlotLines: case WidgetType::PlotHistogram:
          if (p.plotValues.empty()) call("text_disabled",literal(n.label+" (sin datos)"));
          else {
            put("if "+key+" not in state:");
            put(state+" = array('f', "+Json(p.plotValues).dump()+")",1);
            call(n.type == WidgetType::PlotLines ? "plot_lines" : "plot_histogram",label+", "+state+", 0, "+(p.overlay.empty()?"None":literal(p.overlay))+floatRange+", "+size);
          }
          break;
        case WidgetType::Chart2d: {
          hasChart2d = true;
          const auto& mode = Chart2d::modes[Chart2d::modeIndex(p.chart2dType)];
          if (p.chart2dType != mode.id) warn("Tipo de gráfico desconocido. Se genera como línea.");
          std::string labels = "(";
          for (const auto& text : p.chartLabels) labels += literal(text) + ", ";
          labels += ")";
          put("if "+key+" not in state:");
          put(state+" = array('f', "+Json(p.plotValues).dump()+")",1);
          put("_draw_chart_2d("+literal(mode.id)+", "+literal(p.overlay.empty() ? mode.label : p.overlay)+
            ", "+size+", "+color(p.color)+", "+state+", "+labels+", "+
            (p.chartAutoMin ? "None" : number(p.minFloat))+", "+(p.chartAutoMax ? "None" : number(p.maxFloat))+")");
          break;
        }
        case WidgetType::Chart3d: {
          hasChart3d = true;
          const auto& mode = Chart3d::modes[Chart3d::modeIndex(p.chart3dType)];
          if (p.chart3dType != mode.id) warn("Tipo de gráfico 3D desconocido. Se generan barras.");
          put("if "+key+" not in state:");
          put(state+" = {'values': array('f', "+Json(p.plotValues).dump()+"), 'elevation': "+
            number(p.chart3dRotX)+", 'azimuth': "+number(p.chart3dRotY)+"}",1);
          put("_draw_chart_3d("+literal(mode.id)+", "+literal(p.overlay.empty() ? mode.label : p.overlay)+
            ", "+size+", "+color(p.color)+", "+state+")");
          break;
        }
        case WidgetType::ProgressBar: call("progress_bar",number(std::clamp(p.progress,0.0f,1.0f))+", "+size+", "+(p.overlay.empty()?"None":literal(p.overlay))); break;
        case WidgetType::Separator: call("separator"); break;
        case WidgetType::SeparatorText: call("separator_text",literal(n.label)); break;
        case WidgetType::Spacing: call("spacing"); break;
        case WidgetType::SameLine: call("same_line"); break;
        case WidgetType::Dummy: call("dummy","("+(n.width<0 ? std::string("imgui.get_content_region_avail().x") : width)+", "+layout(std::max(0.0f,n.height))+")"); break;
        case WidgetType::ChildWindow:
          put("if imgui.begin_child("+label+", "+size+", "+(p.border ? "imgui.ChildFlags_.borders" : "0")+"):");
          children(1); call("end_child"); break;
        case WidgetType::Image: case WidgetType::ImageButton: {
          hasImageWidgets = true;
          std::string texture = "None";
          if (p.imageAsset) {
            if (const auto index = imageAssets.add(*p.imageAsset))
              texture = "state.get('studio:image_textures', {}).get(" + std::to_string(*index) + ")";
            else warn("Datos de imagen no válidos. Se genera un marcador.");
          }
          const bool button = n.type == WidgetType::ImageButton;
          const auto draw = "_draw_image(" + texture + ", " + literal(n.label) + ", " + size + ", " +
            (scaled ? "scale" : "1.0") + ", " + boolean(button) + ")";
          if (button) {
            frameButtons.push_back(state);
            put("if " + draw + ":");
            put(state + " = True",1);
            if (!p.buttonAction.empty()) warn("La acción de botón requiere integración de la aplicación.");
          } else put(draw);
          break;
        }
        case WidgetType::Animation:
          hasAnimation = true;
          put("animation = " + initial("{'running': " + boolean(p.animRunning) + ", 'start': None}"));
          put("with _animation(animation, " + literal(n.label) + ", " + size + ", " + literal(p.animType) +
              ", " + literal(p.animEasing) + ", " + animationSeconds(p.animDuration) + ", " +
              boolean(p.animLoop) + ", " + (scaled ? "scale" : "1.0") + ") as (visible, animated_scale):");
          put("if visible:",1);
          if (CppCodegen::hasVisibleChildren(n)) children(2,"animated_scale");
          else put("imgui.text_unformatted('Vista previa de animación')",2);
          break;
        case WidgetType::Loader: case WidgetType::Component: case WidgetType::Repeater: {
          const bool loader = n.type == WidgetType::Loader, repeater = n.type == WidgetType::Repeater;
          const auto& title = !repeater && !p.sourceComponent.empty() ? p.sourceComponent : n.label;
          std::string flags = "imgui.ChildFlags_.always_use_window_padding";
          if (p.border) flags += " | imgui.ChildFlags_.borders";
          if (n.height <= 0) flags += " | imgui.ChildFlags_.auto_resize_y";
          put("if imgui.begin_child('template', "+size+", "+flags+"):");
          if (loader) {
            put("loaded = "+initial(boolean(p.loaderActive)),1);
            put("if imgui.button('Descargar###toggle_loader' if loaded else 'Cargar###toggle_loader'):",1);
            put(state+" = loaded = not loaded",2);
            if (!title.empty()) put("imgui.same_line()",1);
          }
          if (!title.empty()) put("imgui.text_unformatted("+literal(title)+")",1);
          if (repeater) {
            const auto count = std::to_string(std::clamp(p.repeatCount,1,kMaxRepeaterInstances));
            put("imgui.text_disabled('"+count+" instancias')",1);
            if (CppCodegen::hasVisibleChildren(n)) {
              hasRepeater = true;
              const auto gap = std::clamp(p.spacing,0.0f,48.0f);
              put("imgui.push_style_var(imgui.StyleVar_.item_spacing, "+dimensions(gap,gap)+")",1);
              put("for instance in range("+count+"):",1);
              put("if state['studio:repeat_budget'] <= 0:",2);
              put("break",3);
              put("state['studio:repeat_budget'] -= 1",2);
              put("imgui.push_id(instance)",2);
              put("if imgui.begin_child('instance', (0, 0), imgui.ChildFlags_.auto_resize_y):",2);
              children(3);
              put("imgui.end_child()",2);
              put("imgui.pop_id()",2);
              put("imgui.pop_style_var()",1);
            }
          } else if (loader) {
            put("if loaded:",1); children(2);
            put("else:",1); put("imgui.text_disabled('Contenido descargado')",2);
          } else {
            children(1);
          }
          call("end_child");
          break;
        }
        case WidgetType::Group: case WidgetType::ColumnLayout: case WidgetType::RowLayout: {
          call("begin_group");
          call("push_style_var","imgui.StyleVar_.item_spacing, "+dimensions(std::max(0.0f,p.spacing),std::max(0.0f,p.spacing)));
          bool firstChild = true;
          for (const auto& child : n.children) if (child.visible) {
            if (!firstChild && n.type == WidgetType::RowLayout) call("same_line");
            put(childCall(child)); firstChild = false;
          }
          call("pop_style_var"); call("end_group"); break;
        }
        case WidgetType::PositionerFlow: {
          const auto gap = layout(std::max(0.0f, p.spacing));
          std::string flags = p.border ? "imgui.ChildFlags_.borders" : "0";
          if (n.height <= 0) flags += " | imgui.ChildFlags_.auto_resize_y";
          put("if imgui.begin_child('flow', "+size+", "+flags+"):");
          put("imgui.push_style_var(imgui.StyleVar_.item_spacing, ("+gap+", "+gap+"))",1);
          put("right = imgui.get_cursor_screen_pos().x + imgui.get_content_region_avail().x",1);
          put("previous_right = None",1);
          put("widths = imgui.get_state_storage()",1);
          for (const auto& child : n.children) {
            if (!child.visible || child.type == WidgetType::SameLine) continue;
            if (child.useCursorPos) {
              put(childCall(child),1);
              put("previous_right = None",1);
              continue;
            }
            put("width_id = imgui.get_id("+literal("flow-width-"+CppCodegen::hexId(child.id))+")",1);
            if (child.width >= 0) {
              put("width = widths.get_float(width_id, -1.0)",1);
              put("if previous_right is not None and width >= 0.0 and previous_right + "+gap+" + width <= right:",1);
              put("imgui.same_line(0.0, "+gap+")",2);
            }
            put("start = imgui.get_cursor_screen_pos().x",1);
            put("imgui.begin_group()",1);
            put(childCall(child),1);
            put("imgui.end_group()",1);
            put("previous_right = imgui.get_item_rect_max().x",1);
            put("widths.set_float(width_id, max(0.0, previous_right - start))",1);
          }
          put("imgui.pop_style_var()",1);
          call("end_child");
          break;
        }
        case WidgetType::TabBar: case WidgetType::TabItem: {
          const std::string kind = n.type == WidgetType::TabBar ? "tab_bar" : "tab_item";
          put("if imgui.begin_"+kind+"("+label+")"+(n.type == WidgetType::TabItem ? "[0]:" : ":"));
          children(1); put("imgui.end_"+kind+"()",1); break;
        }
        case WidgetType::TreeNode: case WidgetType::CollapsingHeader:
          call("set_next_item_open",boolean(p.defaultOpen)+", imgui.Cond_.once");
          put(std::string("if imgui.")+(n.type == WidgetType::TreeNode ? "tree_node" : "collapsing_header")+"("+label+"):");
          children(1); if (n.type == WidgetType::TreeNode) put("imgui.tree_pop()",1); break;
        case WidgetType::Table: case WidgetType::GridLayout: {
          const bool table = n.type == WidgetType::Table;
          const int count = std::clamp(p.columns,1,511);
          if (count != p.columns) warn("Columnas limitadas a 1..511");
          std::string flags = table ? "imgui.TableFlags_.row_bg" : "0";
          if (table && p.border) flags += " | imgui.TableFlags_.borders";
          if (n.height>0) flags += " | imgui.TableFlags_.scroll_y";
          if (!table) call("push_style_var","imgui.StyleVar_.cell_padding, "+dimensions(std::max(0.0f,p.spacing)*0.5f,std::max(0.0f,p.spacing)*0.5f));
          put("if imgui.begin_table("+label+", "+std::to_string(count)+", "+flags+", "+size+"):");
          if (table) {
            for (int i=0;i<count;++i) put("imgui.table_setup_column("+literal(static_cast<size_t>(i)<p.columnLabels.size()?p.columnLabels[static_cast<size_t>(i)]:"Columna "+std::to_string(i+1))+")",1);
            put("imgui.table_headers_row()",1);
          }
          for (const auto& child:n.children) if (child.visible) { put("imgui.table_next_column()",1); put(childCall(child),1); }
          put("imgui.end_table()",1);
          if (!table) call("pop_style_var");
          break;
        }
        case WidgetType::MenuBar:
          if (titleItem) {
            if (p.items.empty()) call("text_disabled",literal("Menú vacío"));
            else {
              put("for index, item in enumerate("+Json(p.items).dump()+"):");
              put("imgui.push_id(index)",1); put("if imgui.begin_menu(item):",1);
              put("imgui.menu_item('Sin acciones configuradas', '', False, False)",2);
              put("imgui.end_menu()",2); put("imgui.pop_id()",1);
            }
          } else if (p.menuBarEdge != MenuBarEdge::Top) {
            warn("Menu fuera del borde superior pendiente"); call("text_disabled",literal("Menu: borde pendiente"));
          } else {
            put("if imgui.begin_menu_bar():");
            put("for index, item in enumerate("+Json(p.items).dump()+"):",1);
            put("imgui.push_id(index)",2); put("if imgui.begin_menu(item):",2);
            put("imgui.menu_item('Sin acciones configuradas', '', False, False)",3);
            put("imgui.end_menu()",3); put("imgui.pop_id()",2); put("imgui.end_menu_bar()",1);
          }
          break;
        default:
          warn("Widget pendiente: "+std::string(widgetTypeId(n.type)));
          call("text_disabled",literal("Widget pendiente: "+std::string(widgetTypeId(n.type)))); children(0); break;
      }
      if (!p.buttonAction.empty()) warn("La accion requiere codigo de la aplicacion");
      if (colors) call("pop_style_color",std::to_string(colors));
      if (!titleItem && n.indent > 0) call("unindent",layout(n.indent));
      call("pop_id");
    }
    result.nodeLines[n.id] = {first,lines.size()};
    lines.insert(lines.end(),{"",""});
  }
};
} // namespace PythonCodegen

inline GeneratedCpp generatePythonCode(const StudioProject& project) {
  using namespace PythonCodegen;
  Emitter emitter;
  try {
    const auto checked = projectFromJson(projectToJson(project));
    if (!checked) throw std::runtime_error(checked.error);
    const auto& s = project.style;
    emitter.lines = {"from array import array", "import sys", "from imgui_bundle import hello_imgui, imgui", "", "",
      "def apply_style():",
      s.preset == StylePresetId::ImGuiLight ? "    imgui.style_colors_light()" :
        s.preset == StylePresetId::ImGuiClassic ? "    imgui.style_colors_classic()" : "    imgui.style_colors_dark()",
      "    style = imgui.get_style()",
      "    style.window_rounding = "+number(s.windowRounding), "    style.frame_rounding = "+number(s.frameRounding),
      "    style.window_padding = "+pair(s.windowPaddingX,s.windowPaddingY), "    style.item_spacing = "+pair(s.itemSpacingX,s.itemSpacingY)};
    for (const auto& c : {std::pair<const char*,Color4>{"window_bg",s.windowBg},{"title_bg_active",s.titleBg},{"button",s.buttonColor},{"frame_bg",s.frameBg},{"text",s.textColor}})
      emitter.lines.push_back("    style.set_color_(imgui.Col_."+std::string(c.first)+", imgui.ImVec4(*"+color(c.second)+"))");
    const auto style = ProjectJson::styleJson(s);
    const std::pair<const char*,const char*> scalarStyles[] = {
      {"alpha","alpha"},{"childRounding","child_rounding"},{"grabRounding","grab_rounding"},
      {"tabRounding","tab_rounding"},{"scrollbarRounding","scrollbar_rounding"},
      {"windowBorderSize","window_border_size"},{"frameBorderSize","frame_border_size"}
    };
    for (const auto& field : scalarStyles)
      if (style.contains(field.first) && style.at(field.first).is_number())
        emitter.lines.push_back("    style."+std::string(field.second)+" = "+number(style.at(field.first).get<float>()));
    if (style.contains("framePaddingX") && style.at("framePaddingX").is_number() &&
        style.contains("framePaddingY") && style.at("framePaddingY").is_number())
      emitter.lines.push_back("    style.frame_padding = "+pair(style.at("framePaddingX").get<float>(),style.at("framePaddingY").get<float>()));
    emitter.lines.insert(emitter.lines.end(),{"",""});
    collectScaledAnimationChildren(project.nodes, emitter.scaledNodes);
    CppCodegen::emitNodeFunctions(project.nodes, [&](const ImGuiNode& node, bool flowChild) { emitter.emit(node, flowChild); });
    if (emitter.hasModelViews) emitter.lines.push_back(R"PY(def _model_label(text, centered, featured=False):
    if not imgui.is_item_visible():
        return
    lo, hi = imgui.get_item_rect_min(), imgui.get_item_rect_max()
    padding = imgui.get_style().frame_padding.x
    extent = imgui.calc_text_size(text)
    x = max(padding, (hi.x - lo.x - extent.x) * 0.5) if centered else padding
    y = max(0.0, (hi.y - lo.y - extent.y) * 0.5)
    draw = imgui.get_window_draw_list()
    if featured:
        draw.add_rect_filled(lo, hi, imgui.get_color_u32(imgui.Col_.header), imgui.get_style().frame_rounding)
    draw.push_clip_rect((lo.x + padding, lo.y), (hi.x - padding, hi.y), True)
    draw.add_text((lo.x + x, lo.y + y), imgui.get_color_u32(imgui.Col_.text), text)
    draw.pop_clip_rect()


def _model_rows(state, key, items, selected, columns, grid):
    clip = imgui.ListClipper()
    clip.begin((len(items) + columns - 1) // columns)
    height = imgui.get_frame_height() * (1.5 if grid else 1.0)
    while clip.step():
        for row in range(clip.display_start, clip.display_end):
            if grid:
                imgui.table_next_row()
            for index in range(row * columns, min(len(items), (row + 1) * columns)):
                if grid:
                    imgui.table_next_column()
                imgui.push_id(index)
                if imgui.selectable('##model_option', index == selected, 0, (0, height))[0]:
                    state[key] = index
                _model_label(items[index], grid)
                imgui.pop_id()


def _draw_model_view(state, key, label, items, initial, size, border, columns, spacing, path):
    selected = state.setdefault(key, initial)
    flags = imgui.ChildFlags_.always_use_window_padding
    if border:
        flags |= imgui.ChildFlags_.borders
    imgui.push_style_color(imgui.Col_.child_bg, imgui.get_style_color_vec4(imgui.Col_.frame_bg))
    if imgui.begin_child('model', size, flags):
        if label:
            imgui.text_unformatted(label)
        if not items:
            imgui.text_disabled('Sin elementos')
        elif path:
            selected %= len(items)
            imgui.text_disabled(f'{selected + 1} / {len(items)}')
            arrow = imgui.get_frame_height()
            width = max(1.0, imgui.get_content_region_avail().x - 2 * (arrow + imgui.get_style().item_spacing.x))
            imgui.begin_disabled(len(items) == 1)
            if imgui.arrow_button('previous', imgui.Dir_.left):
                state[key] = (selected - 1) % len(items)
            imgui.end_disabled()
            imgui.same_line()
            imgui.dummy((width, arrow))
            _model_label(items[selected], True, True)
            imgui.same_line()
            imgui.begin_disabled(len(items) == 1)
            if imgui.arrow_button('next', imgui.Dir_.right):
                state[key] = (selected + 1) % len(items)
            imgui.end_disabled()
        else:
            selected = max(0, min(len(items) - 1, selected))
            if columns:
                imgui.push_style_var(imgui.StyleVar_.cell_padding, (spacing * 0.5, spacing * 0.5))
                if imgui.begin_table('model_cells', columns, imgui.TableFlags_.sizing_stretch_same):
                    _model_rows(state, key, items, selected, columns, True)
                    imgui.end_table()
                imgui.pop_style_var()
            else:
                _model_rows(state, key, items, selected, 1, False)
    imgui.end_child()
    imgui.pop_style_color()

)PY");
    if (emitter.hasChart2d || emitter.hasChart3d) emitter.lines.push_back(chartSampleFunctions);
    if (emitter.hasChart2d) emitter.lines.push_back(chart2dFunctions);
    if (emitter.hasChart3d) emitter.lines.push_back(chart3dFunctions);
    if (emitter.hasAnimation) emitter.lines.push_back(animationFunctions);
    if (emitter.hasImageWidgets) emitter.lines.push_back(imageDrawing);
    if (emitter.hasWindows) emitter.lines.push_back(titleBarFunctions);
    const auto& images = emitter.imageAssets.items();
    emitter.result.hasImageTextures = !images.empty();
    if (!images.empty()) {
      emitter.lines.push_back("_IMAGE_ASSETS = (");
      for (const auto& asset : images) {
        emitter.lines.push_back("    (");
        for (size_t offset = 0; offset < asset.encoded.size(); offset += CppCodegen::imageBlockSize)
          emitter.lines.push_back("        " + literal(std::string(asset.encoded.substr(offset, CppCodegen::imageBlockSize))));
        emitter.lines.push_back("        , " + std::to_string(asset.size.width) + ", " + std::to_string(asset.size.height) + "),");
      }
      emitter.lines.push_back(")");
      emitter.lines.push_back(imageLoading);
    }
    emitter.lines.push_back("def render_ui(state):");
    const auto start = emitter.lines.size();
    if (emitter.hasRepeater)
      emitter.lines.push_back("    state['studio:repeat_budget'] = "+std::to_string(ProjectJson::kMaxNodes));
    for (const auto& button : emitter.frameButtons) emitter.lines.push_back("    " + button + " = False");
    for (const auto& node : project.nodes) if (node.visible) emitter.lines.push_back("    "+Emitter::function(node)+"(state)");
    if (emitter.lines.size() == start) emitter.lines.push_back("    pass");
    emitter.lines.insert(emitter.lines.end(),{"","","def main():",
      "    params = hello_imgui.RunnerParams()",
      "    params.app_window_params.window_title = "+literal(project.name.substr(0,project.name.find('\0'))),
      "    params.app_window_params.window_geometry.size = ("+std::to_string(project.canvas.width)+", "+std::to_string(project.canvas.height)+")",
      "    params.imgui_window_params.default_imgui_window_type = hello_imgui.DefaultImGuiWindowType.no_default_window",
      "    smoke = '--smoke-test' in sys.argv[1:]",
      "    params.app_window_params.hidden = smoke", "    params.ini_disable = smoke",
      "    params.callbacks.setup_imgui_style = apply_style", "    state = {}", "    frames = 0"});
    if (!images.empty()) emitter.lines.insert(emitter.lines.end(), {
      "    images_loaded = False",
      "    def init_images():",
      "        nonlocal images_loaded",
      "        images_loaded = _load_hello_images(state)",
      "    params.callbacks.post_init = init_images"});
    emitter.lines.insert(emitter.lines.end(),{
      "    def draw():", "        nonlocal frames", "        render_ui(state)", "        frames += 1",
      "        if smoke and frames >= 3:", "            params.app_shall_exit = True",
      "    params.callbacks.show_gui = draw", "    hello_imgui.run(params)",
      "    if smoke:", "        if frames < 3:", "            raise RuntimeError('Smoke test ended before three frames')"});
    if (!images.empty()) emitter.lines.insert(emitter.lines.end(),{
      "        if not images_loaded:", "            raise RuntimeError('Unable to upload embedded images')"});
    emitter.lines.insert(emitter.lines.end(),{"        print('Python rendered three frames.')",
      "", "", "if __name__ == '__main__':", "    main()"});
    std::ostringstream code;
    for (const auto& line : emitter.lines) code << line << '\n';
    for (const auto& d : emitter.result.diagnostics) code << "# [" << CppCodegen::comment(d.nodeId) << "] " << CppCodegen::comment(d.message) << '\n';
    emitter.result.code = code.str();
  } catch (const std::exception& error) { emitter.result.error = error.what(); }
  return emitter.result;
}

inline CppBundle generatePythonBundle(const StudioProject& project) {
  CppBundle bundle;
  bundle.generated = generatePythonCode(project);
  const auto& generated = bundle.generated;
  if (!generated.error.empty()) return bundle;
  std::string report = "Python / imgui-bundle 1.93.0\n";
  for (const auto& diagnostic : generated.diagnostics)
    report += "["+CppCodegen::comment(diagnostic.nodeId)+"] "+CppCodegen::comment(diagnostic.message)+"\n";
  report += "\nRangos de lineas en main.py (inclusivos):\n";
  for (const auto& location : generated.nodeLines)
    report += CppCodegen::comment(location.first)+": "+std::to_string(location.second.first)+"-"+std::to_string(location.second.second)+"\n";
  bundle.files = {
    {"main.py",generated.code},
    {"requirements.txt","imgui-bundle==1.93.0\n"},
    {"codegen_report.txt",report},
    {"README.md",R"README(# Exportación Python

Usa CPython y el binding indicado en requirements.txt. Se verificó con CPython
3.14 de Windows. Si el intérprete elegido no tiene el binding, prepara un entorno
virtual e instala sus dependencias allí.

```sh
python -m venv .venv
python -m pip --python .venv install -r requirements.txt
```

Con el entorno activado, ejecuta python main.py. La opción --smoke-test dibuja
tres frames ocultos con Hello ImGui. Revisa codegen_report.txt para ver las
omisiones y las líneas de código por nodo. Las acciones de botones y menús se
conectan en el código de la aplicación.
)README"}
  };
  if (generated.hasImageTextures) bundle.files["README.md"] +=
    "\nLos PNG y JPEG están incrustados en main.py. Hello ImGui carga y libera las texturas.\n"
    "Con otro backend, usa load_images(state, upload) y release_images(state, release).\n"
    "Consulta state[\"node:<id>\"] tras render_ui para detectar un clic en un botón de imagen.\n";
  if (generated.hasTitleButtons) bundle.files["README.md"] +=
    "\nConsulta state[\"node:<id>\"] después de render_ui para detectar el clic de un botón de título.\n"
    "El indicador se reinicia al iniciar el siguiente render_ui. Conecta ese indicador con la acción de tu aplicación.\n";
  return bundle;
}

inline ProjectSaveResult exportPythonBundle(const StudioProject& project,const std::filesystem::path& destination) {
  const auto bundle = generatePythonBundle(project);
  if (!bundle.generated.error.empty()) return {false,bundle.generated.error};
  return writeGeneratedBundle(bundle,destination);
}
} // namespace ImGuiStudio
