#pragma once
#include "studio_cpp_export.hpp"
#include "imguistudio/export/detail/studio_chart2d_java.hpp"
#include "imguistudio/export/detail/studio_chart3d_java.hpp"
#include "imguistudio/export/detail/studio_animation_java.hpp"
#include "imguistudio/export/detail/studio_managed_animation.hpp"
#include "imguistudio/export/detail/studio_images_java_rust.hpp"
#include "imguistudio/widgets/studio_image_data.hpp"
#include "imguistudio/export/detail/studio_managed_titlebar.hpp"
#include "imguistudio/export/detail/studio_titlebar_java.hpp"

namespace ImGuiStudio {
namespace JavaCodegen {
inline std::string literal(const std::string& value) {
  // A class-file constant has a 65535-byte modified UTF-8 limit.
  if (value.size() > 16000) {
    std::string joined = "String.join(\"\"";
    for (size_t start=0; start<value.size();) {
      size_t end = std::min(value.size(),start+16000);
      while (end<value.size() && (static_cast<unsigned char>(value[end]) & 0xC0)==0x80) --end;
      joined += ", "+literal(value.substr(start,end-start));
      start = end;
    }
    return joined+")";
  }
  // Fixed-width octal escapes avoid Java's early processing of Unicode escapes.
  std::string out = "\"";
  for (const unsigned char c : value) {
    if (c == '\\' || c == '"') { out += '\\'; out += static_cast<char>(c); }
    else if (c < 32 || c == 127) {
      char escaped[5];
      std::snprintf(escaped,sizeof(escaped),"\\%03o",static_cast<unsigned>(c));
      out += escaped;
    } else out += static_cast<char>(c);
  }
  return out + '"';
}
inline std::string vecArgs(float x,float y) { return CppCodegen::number(x)+", "+CppCodegen::number(y); }
inline std::string vec(float x,float y) { return "new ImVec2("+CppCodegen::number(x)+", "+CppCodegen::number(y)+")"; }
inline std::string color(const Color4& c) {
  return "new ImVec4("+CppCodegen::number(c.r)+", "+CppCodegen::number(c.g)+", "+CppCodegen::number(c.b)+", "+CppCodegen::number(c.a)+")";
}
inline std::string strings(const std::vector<std::string>& items) {
  std::string result = "new String[] {";
  for (const auto& item : items) result += literal(item)+", ";
  return result+"}";
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
  std::set<std::string> scaledNodes;
  static std::string function(const ImGuiNode& n) { return "Draw_"+CppCodegen::hexId(n.id); }
  std::set<std::string> titleItems;
  void emitTitleItem(const ImGuiNode& child, bool show) {
    using CppCodegen::number;
    const auto first = lines.size() + 1;
    const auto key = literal("node:" + child.id);
    const int kind = CppCodegen::titleKind(child);
    if (!kind) frameButtons.push_back(key);
    if (!show) lines.push_back("        // Title item hidden by the window flags.");
    else {
      const auto b = ManagedTitleBar::describe(child, [&](const std::string& message) {
        result.diagnostics.push_back({child.id, message});
      });
      const auto& p = child.props;
      const auto tint = [&](const std::optional<Color4>& c) { return !kind && c ? color(*c) : "null"; };
      const auto width = kind == 11 && child.width <= 0 ?
        "titleMenuWidth(" + strings(p.items) + ")" : number(child.width);
      lines.push_back("            new TitleButton(" + key + ", " +
        literal(ManagedTitleBar::caption(kind == 8 ? p.text : child.label)) +
        ", " + std::to_string(b.side) + ", " + std::to_string(b.variant) + ", " + std::to_string(kind ? 0 : b.icon) +
        ", " + width + ", " + number(child.height) + ", " + number(p.titleBarX) +
        ", " + (p.titleBarY ? number(*p.titleBarY) : "null") + ", " + number(b.rounding) +
        ", " + tint(b.background) + ", " + tint(b.foreground) + ", " + tint(b.hovered) + ", " + tint(b.active) +
        ", " + std::to_string(kind) + ", " + (kind ? "StudioUI::" + function(child) : "null") + "),");
      if (kind) titleItems.insert(child.id);
    }
    if (!show || !kind) result.nodeLines[child.id] = {first, lines.size()};
  }
  void emit(const ImGuiNode& n, bool flowChild = false) {
    if (result.nodeLines.count(n.id)) return; // Submitted by its parent window.
    using CppCodegen::number;
    using CppCodegen::boolean;
    const auto first = lines.size()+1;
    const auto& p = n.props;
    const auto props = ProjectJson::propsJson(p,n.type);
    const auto id = CppCodegen::hexId(n.id);
    const auto label = literal(n.label.substr(0,n.label.find('\0'))+"###studio_"+id);
    const auto key = literal(n.type == WidgetType::RadioButton ? "radio:"+n.varName : "node:"+n.id);
    const bool scaled = scaledNodes.count(n.id) != 0;
    const bool titleItem = titleItems.count(n.id) != 0;
    const auto layout = [&](float value) { return number(value) + (scaled && value != 0 ? " * scale" : ""); };
    const auto dimensions = [&](float x, float y) { return "new ImVec2(" + layout(x) + ", " + layout(y) + ")"; };
    const auto width = n.width < 0 ? number(-std::numeric_limits<float>::min()) : layout(n.width);
    const auto size = "new ImVec2(" + width + ", " + layout(std::max(0.0f,n.height)) + ")";
    auto put = [&](const std::string& line,int depth = 2) { lines.push_back(std::string(static_cast<size_t>(depth)*4,' ')+line); };
    auto javaName = [](std::string name) {
      name.front() = static_cast<char>(std::tolower(static_cast<unsigned char>(name.front())));
      return name;
    };
    auto call = [&](const std::string& name,const std::string& args = {}) { put("ImGui."+javaName(name)+"("+args+");"); };
    auto childCall = [&](const ImGuiNode& child, const std::string& state = "state", const std::string& scale = "scale") {
      return function(child) + "(" + state + (scaledNodes.count(child.id) ? ", " + scale : "") + ");";
    };
    auto children = [&](int depth) {
      for (const auto& child : n.children)
        if (child.visible && (n.type != WidgetType::Window || !WindowChrome::isTitleItem(child)))
          put(childCall(child),depth);
    };
    auto warn = [&](const std::string& message) { result.diagnostics.push_back({n.id,message}); };
    auto format = [&](bool integer,const std::string& fallback) {
      if (!p.format.empty() && !safeNumericFormat(p.format,integer)) warn("Formato numerico incompatible");
      return literal(safeNumericFormat(p.format,integer) ? p.format : fallback);
    };
    auto control = [&](const std::string& name,const std::string& initial,const std::string& suffix = {},const std::string& prefix = {}) {
      if (titleItem && n.type == WidgetType::InputText) call("SetNextItemWidth","titleWidth");
      else if (n.width != 0 && !titleItem) call("SetNextItemWidth",width);
      put("var value = state.get("+key+", () -> "+initial+");");
      put("ImGui."+javaName(name)+"("+label+", "+prefix+"value"+suffix+");");
    };
    put("private static void "+function(n)+(titleItem ? "(UIState state, float titleWidth, float titleHeight)" :
      scaled ? "(UIState state, float scale)" : "(UIState state)"),1);
    put("{",1);
    if (n.type == WidgetType::Window) {
      const auto title = ManagedTitleBar::describeWindow(n);
      const auto& a = p.windowAppearance;
      bool collapse = title.collapse;
      std::string flags = "ImGuiWindowFlags.None";
      static const std::set<std::string> allowed = {"NoTitleBar","NoResize","NoMove","NoCollapse","NoScrollbar",
        "NoScrollWithMouse","AlwaysAutoResize","NoSavedSettings","NoMouseInputs","MenuBar","NoBackground",
        "HorizontalScrollbar","NoFocusOnAppearing","NoBringToFrontOnFocus","AlwaysVerticalScrollbar",
        "AlwaysHorizontalScrollbar","NoNavInputs","NoNavFocus","UnsavedDocument","NoNav","NoDecoration","NoInputs"};
      for (const auto& raw : ProjectJson::strings(props,"windowFlags",{})) {
        const auto flag = raw.rfind("ImGuiWindowFlags_",0) == 0 ? raw.substr(17) : raw;
        if (allowed.count(flag)) flags += " | ImGuiWindowFlags."+flag;
        else warn("Flag de ventana pendiente: "+raw);
        collapse &= flag != "NoCollapse";
      }
      if (!collapse) flags += " | ImGuiWindowFlags.NoCollapse";
      const bool trackGeometry = title.modes || (title.inset && collapse);
      result.hasTitleButtons |= title.custom || title.shadow != 0;
      put("if (!state.get("+key+", () -> new ImBoolean(true)).get()) return;");
      const auto viewSize = vec(std::max(0.0f,n.width),std::max(0.0f,n.height));
      int styleVars = 0, colors = 0;
      const auto styleVar = [&](const char* name, std::optional<float> value, float min, float max) {
        if (!value) return;
        call("PushStyleVar", "ImGuiStyleVar." + std::string(name) + ", " + number(std::clamp(*value, min, max)));
        ++styleVars;
      };
      const auto tint = [&](const char* name, const std::optional<Color4>& value) {
        if (!value) return;
        call("PushStyleColor", "ImGuiCol." + std::string(name) + ", " + color(*value));
        ++colors;
      };
      styleVar("Alpha", a.alpha, 0, 1);
      styleVar("WindowRounding", a.rounding, 0, 72);
      styleVar("WindowBorderSize", p.noBackground ? std::optional<float>{0} : a.borderSize, 0, 12);
      const auto pad = [&](std::optional<float> value, char axis) {
        return value ? number(std::clamp(*value,0.0f,256.0f)) : std::string("ImGui.getStyle().getWindowPadding") + axis + "()";
      };
      if (a.paddingX || a.paddingY) {
        call("PushStyleVar", "ImGuiStyleVar.WindowPadding, new ImVec2("+pad(a.paddingX,'X')+", "+pad(a.paddingY,'Y')+")");
        ++styleVars;
      }
      tint("WindowBg", a.background); tint("Border", a.borderColor); tint("Text", a.textColor);
      if (!a.showResizeGrip)
        for (const auto slot : {"ResizeGrip","ResizeGripHovered","ResizeGripActive"}) tint(slot, Color4{0,0,0,0});
      const auto titleBg = a.transparentTitle ? std::optional<Color4>{Color4{0,0,0,0}} : a.titleBackground;
      for (const auto slot : {"TitleBg","TitleBgActive","TitleBgCollapsed"}) tint(slot, titleBg);
      for (const auto& field : WindowChrome::colors)
        if (props.contains(field.key) && !(a.*field.member)) warn(std::string("Color de ventana incompatible: ")+field.key);

      if (title.visible && a.titleHeight)
        call("PushStyleVar","ImGuiStyleVar.FramePadding, new ImVec2(ImGui.getStyle().getFramePaddingX(), Math.max(0f, ("+
          number(std::clamp(*a.titleHeight,20.0f,72.0f))+" - ImGui.getFontSize()) * 0.5f))");
      if (trackGeometry) {
        const auto modeKey = literal("window-mode:" + n.id);
        put("var mode = state.get("+modeKey+", () -> new WindowMode("+vec(n.x,n.y)+", "+viewSize+"));");
        put("boolean minimized = mode.active == 1;");
        put("if (minimized) ImGui.pushStyleVar(ImGuiStyleVar.WindowMinSize, new ImVec2(ImGui.getStyle().getWindowMinSizeX(), 1));");
        put("prepareWindowMode(mode);");
        flags += " | (mode.active != 0 ? ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoResize | ImGuiWindowFlags.NoCollapse : 0)";
      } else {
        call("SetNextWindowPos",vec(n.x,n.y)+", ImGuiCond.FirstUseEver");
        call("SetNextWindowSize",viewSize+", ImGuiCond.FirstUseEver");
      }
      for (const auto& child : n.children)
        if (child.visible && child.type == WidgetType::MenuBar && child.props.menuBarEdge == MenuBarEdge::Top) {
          flags += trackGeometry ? " | (minimized ? 0 : ImGuiWindowFlags.MenuBar)" : " | ImGuiWindowFlags.MenuBar";
          break;
        }
      if (title.inset) {
        put("var menuPosition = ImGui.getStyle().getWindowMenuButtonPosition();");
        put("ImGui.getStyle().setWindowMenuButtonPosition(imgui.flag.ImGuiDir.None);");
        for (const auto slot : {"TitleBg","TitleBgActive","TitleBgCollapsed"})
          call("PushStyleColor", "ImGuiCol." + std::string(slot) + ", new ImVec4(0, 0, 0, 0)");
      }
      put("var flags = " + flags + ";");
      put("boolean visible = ImGui.begin("+(title.custom ? literal("###studio_" + id) : label)+", flags);");
      if (title.inset) {
        call("PopStyleColor","3");
        put("ImGui.getStyle().setWindowMenuButtonPosition(menuPosition);");
      }
      if (trackGeometry) put("if (minimized) ImGui.popStyleVar();");
      if (title.shadow) put("drawStudioShadow(" + std::to_string(title.shadow) + ");");
      const auto control = [&](const char* name, int kind, int side) {
        put("new TitleButton("+literal(std::string("##studio_window_")+name)+", \"\", " +
          std::to_string(side) + ", 0, 0, 0, 0, 0, null, 0, null, null, null, null, " + std::to_string(kind) + ", null),", 3);
      };
      if (title.custom) {
        put("renderTitleBar(state, "+literal(ManagedTitleBar::caption(p.showTitleText ? n.label : ""))+", new TitleButton[] {");
        if (title.inset && collapse) control("collapse", 4, 0);
        if (title.side != 2) ManagedTitleBar::eachWindowControl(p, title, control);
      }
      for (const auto& child : n.children) {
        if (ManagedTitleBar::visibleItem(child)) emitTitleItem(child, title.custom);
        else if (child.visible && child.props.inTitleBar)
          result.diagnostics.push_back({child.id,"Control no compatible con la barra de título. Se conserva en el cuerpo."});
      }
      if (title.custom) {
        if (title.side == 2) ManagedTitleBar::eachWindowControl(p, title, control);
        put("}, new TitleAppearance(" + number(title.left) + ", " + number(title.right) + ", " +
          number(title.alignment) + ", " + std::to_string(title.icon) + ", " + std::to_string(title.variant) + ", " +
          boolean(a.titleSeparator) + ", " + (a.separatorColor ? color(*a.separatorColor) : "null") + ", " +
          boolean(a.transparentTitle) + "), flags, " + key + ", " + (trackGeometry ? "mode" : "null") + ", visible);");
      }
      if (title.visible && a.titleHeight) call("PopStyleVar");
      put(trackGeometry ? "if (visible && !minimized) {" : "if (visible) {");
      children(3); put("}"); call("End");
      if (colors) call("PopStyleColor",std::to_string(colors));
      if (styleVars) call("PopStyleVar",std::to_string(styleVars));
    } else {
      call("PushID",literal(id));
      if (!titleItem) {
        if (n.useCursorPos) call("SetCursorPos",dimensions(n.x,n.y));
        else if (n.sameLine && !flowChild) call("SameLine",n.sameLineSpacing>0 ? "0f, "+layout(n.sameLineSpacing) : "");
        if (n.indent>0) call("Indent",layout(n.indent));
      }
      int colors = 0;
      const auto widgetColor = [&](const char* keyName, const char* slot) {
        if (!props.contains(keyName) || !props.at(keyName).is_string()) return;
        try { call("PushStyleColor","ImGuiCol."+std::string(slot)+", "+color(ProjectJson::colorFromHex(props.at(keyName).get<std::string>()))); ++colors; }
        catch (const std::exception&) { warn(std::string("Color incompatible: ")+keyName); }
      };
      widgetColor("textColorOverride", "Text");
      widgetColor("buttonColorOverride", "Button");
      if (titleItem)
        for (const auto& entry : CppCodegen::titleWidgetColors) widgetColor(entry.key, entry.native);
      const auto fr = ", "+number(std::min(p.minFloat,p.maxFloat))+", "+number(std::max(p.minFloat,p.maxFloat));
      const auto ir = ", "+std::to_string(std::min(p.minInt,p.maxInt))+", "+std::to_string(std::max(p.minInt,p.maxInt));
      const auto speed = ", "+number(std::max(0.0f,p.speed));
      auto container = [&](const std::string& begin,const std::string& end,bool alwaysEnd = false) {
        put("if ("+begin+") {"); children(3);
        if (!alwaysEnd && !end.empty()) put(end+";",3);
        put("}"); if (alwaysEnd) put(end+";");
      };
      switch (n.type) {
        case WidgetType::Text: call("TextUnformatted",literal(p.text)); break;
        case WidgetType::TextColored:
          call("PushStyleColor","ImGuiCol.Text, "+color(p.color)); call("TextUnformatted",literal(p.text)); call("PopStyleColor"); break;
        case WidgetType::TextWrapped:
          call("PushTextWrapPos","0f"); call("TextUnformatted",literal(p.text)); call("PopTextWrapPos"); break;
        case WidgetType::BulletText: call("Bullet"); call("TextUnformatted",literal(p.text)); break;
        case WidgetType::HelpMarker:
          call("TextUnformatted",literal("(?)"));
          put("if (ImGui.beginItemTooltip()) {"); put("ImGui.textUnformatted("+literal(p.text)+");",3); put("ImGui.endTooltip();",3); put("}"); break;
        case WidgetType::Button: call("Button",label+", "+(titleItem ? "new ImVec2(titleWidth, titleHeight)" : size)); break;
        case WidgetType::SmallButton: call("SmallButton",label); break;
        case WidgetType::ColorButton: call("ColorButton",label+", "+color(p.color)+", ImGuiColorEditFlags.None, "+size); break;
        case WidgetType::ArrowButton:
          call("ArrowButton",label+", ImGuiDir."+(p.arrowDir=="left" ? "Left" : p.arrowDir=="up" ? "Up" : p.arrowDir=="down" ? "Down" : "Right")); break;
        case WidgetType::Checkbox: control("Checkbox","new ImBoolean("+boolean(p.checked)+")"); break;
        case WidgetType::Selectable: control("Selectable","new ImBoolean("+boolean(p.selected)+")",", ImGuiSelectableFlags.None, "+size); break;
        case WidgetType::RadioButton: control("RadioButton","new ImInt("+std::to_string(p.intValue)+")",", "+std::to_string(p.radioValue)); break;
        case WidgetType::InputText: case WidgetType::InputMultiline: {
          if (p.text.find('\0') != std::string::npos) warn("El control de texto muestra hasta el primer NUL");
          const auto bytes = std::max<size_t>(4096,p.text.size()+1024);
          const auto initialText = "new ImString("+literal(p.text)+", "+std::to_string(bytes)+")";
          if (n.type == WidgetType::InputText)
            control("InputTextWithHint",initialText,{},literal(p.hint)+", ");
          else control("InputTextMultiline",initialText,", "+vecArgs(n.width < 0 ? -std::numeric_limits<float>::min() : n.width,std::max(0.0f,n.height)));
          break;
        }
        case WidgetType::InputFloat: control("InputFloat","new ImFloat("+number(p.floatValue)+")",speed+", 0f, "+format(false,"%.3f")); break;
        case WidgetType::InputInt: {
          const int step = static_cast<int>(std::clamp(static_cast<double>(p.speed),1.0,100000000.0));
          control("InputInt","new ImInt("+std::to_string(p.intValue)+")",", "+std::to_string(step)+", "+std::to_string(step*10));
          if (!p.format.empty() && p.format != "%d") warn("InputInt usa formato decimal");
          break;
        }
        case WidgetType::SliderFloat: control("SliderFloat","new float[]{"+number(p.floatValue)+"}",fr+", "+format(false,"%.3f")); break;
        case WidgetType::SliderAngle: control("SliderAngle","new float[]{"+number(p.floatValue)+"}",fr+", "+format(false,"%.0f deg")); break;
        case WidgetType::SliderInt: control("SliderInt","new int[]{"+std::to_string(p.intValue)+"}",ir+", "+format(true,"%d")); break;
        case WidgetType::DragFloat: control("DragFloat","new float[]{"+number(p.floatValue)+"}",speed+fr+", "+format(false,"%.3f")); break;
        case WidgetType::DragInt: control("DragInt","new int[]{"+std::to_string(p.intValue)+"}",speed+ir+", "+format(true,"%d")); break;
        case WidgetType::SliderFloat2: case WidgetType::SliderFloat3: case WidgetType::DragFloat3: {
          const auto value = [&](size_t i) { return i<p.defaultVec.size()?p.defaultVec[i]:0.0f; };
          const bool two = n.type == WidgetType::SliderFloat2;
          const auto initial = "new float[]{"+vecArgs(value(0),value(1))+(two?"":", "+number(value(2)))+"}";
          if (p.defaultVec.size()>(two?2u:3u)) warn("Componentes adicionales conservados solo en el proyecto");
          control(two?"SliderFloat2":n.type==WidgetType::SliderFloat3?"SliderFloat3":"DragFloat3",initial,
            (n.type==WidgetType::DragFloat3?speed:"")+fr+", "+format(false,"%.3f"));
          break;
        }
        case WidgetType::ColorEdit3:
          control("ColorEdit3","new float[]{"+vecArgs(p.color.r,p.color.g)+", "+number(p.color.b)+"}"); break;
        case WidgetType::ColorEdit4: control("ColorEdit4","new float[]{"+vecArgs(p.color.r,p.color.g)+", "+vecArgs(p.color.b,p.color.a)+"}"); break;
        case WidgetType::Combo: case WidgetType::Listbox:
          if (p.items.empty()) call("TextUnformatted",literal(n.label+" (sin opciones)"));
          else {
            put("var items = state.get("+literal("items:"+n.id)+", () -> "+strings(p.items)+");");
            control(n.type==WidgetType::Combo?"Combo":"ListBox","new ImInt("+std::to_string(p.selectedIndex)+")",", items");
          }
          break;
        case WidgetType::PlotLines: case WidgetType::PlotHistogram:
          if (p.plotValues.empty()) call("TextUnformatted",literal(n.label+" (sin datos)"));
          else {
            std::string values = "new float[] {";
            for (float sample : p.plotValues) values += number(sample)+", ";
            put("var values = state.get("+key+", () -> "+values+"});");
            call(n.type==WidgetType::PlotLines?"PlotLines":"PlotHistogram",label+", values, values.length, 0, "+literal(p.overlay)+fr+", "+size);
          }
          break;
        case WidgetType::Chart2d: {
          hasChart2d = true;
          const auto& mode = Chart2d::modes[Chart2d::modeIndex(p.chart2dType)];
          if (p.chart2dType != mode.id) warn("Tipo de gráfico desconocido. Se genera como línea.");
          const auto json = Json(p.plotValues).dump();
          put("var values = state.get("+key+", () -> chartValues("+literal(json.substr(1,json.size()-2))+"));");
          put("var labels = state.get("+literal("chart-labels:"+n.id)+", () -> "+strings(p.chartLabels)+");");
          put("drawChart2d("+literal(mode.id)+", "+literal(p.overlay.empty() ? mode.label : p.overlay)+
            ", "+size+", "+color(p.color)+", values, labels, "+
            (p.chartAutoMin ? "null" : number(p.minFloat))+", "+(p.chartAutoMax ? "null" : number(p.maxFloat))+");");
          break;
        }
        case WidgetType::Chart3d: {
          hasChart3d = true;
          const auto& mode = Chart3d::modes[Chart3d::modeIndex(p.chart3dType)];
          if (p.chart3dType != mode.id) warn("Tipo de gráfico 3D desconocido. Se generan barras.");
          const auto json = Json(p.plotValues).dump();
          put("var chart = state.get("+key+", () -> new Chart3dState(chartValues("+
            literal(json.substr(1,json.size()-2))+"), "+number(p.chart3dRotX)+", "+number(p.chart3dRotY)+"));");
          put("drawChart3d("+literal(mode.id)+", "+literal(p.overlay.empty() ? mode.label : p.overlay)+
            ", "+size+", "+color(p.color)+", chart);");
          break;
        }
        case WidgetType::ProgressBar: call("ProgressBar",number(std::clamp(p.progress,0.0f,1.0f))+", "+size+", "+(p.overlay.empty()?"null":literal(p.overlay))); break;
        case WidgetType::Separator: call("Separator"); break;
        case WidgetType::SeparatorText: call("SeparatorText",literal(n.label)); break;
        case WidgetType::Spacing: call("Spacing"); break;
        case WidgetType::SameLine: call("SameLine"); break;
        case WidgetType::Dummy:
          if (n.width < 0) call("Dummy","new ImVec2(ImGui.getContentRegionAvailX(), "+layout(std::max(0.0f,n.height))+")");
          else call("Dummy",size);
          break;
        case WidgetType::ChildWindow:
          container("ImGui.beginChild("+label+", "+size+", "+(p.border?"ImGuiChildFlags.Border":"ImGuiChildFlags.None")+")","ImGui.endChild()",true); break;
        case WidgetType::Image: case WidgetType::ImageButton: {
          hasImageWidgets = true;
          std::string texture = "null";
          if (p.imageAsset) {
            if (const auto index = imageAssets.add(*p.imageAsset))
              texture = "state.imageTextures.get(" + std::to_string(*index) + ")";
            else warn("Datos de imagen no válidos. Se genera un marcador.");
          }
          const bool button = n.type == WidgetType::ImageButton;
          const auto draw = "drawImage(" + texture + ", " + literal(n.label) + ", " + size + ", " +
            (scaled ? "scale" : "1f") + ", " + boolean(button) + ")";
          if (button) {
            frameButtons.push_back(key);
            put("if (" + draw + ") state.get(" + key + ", ImBoolean::new).set(true);");
            if (!p.buttonAction.empty()) warn("La acción de botón requiere integración de la aplicación.");
          } else put(draw + ";");
          break;
        }
        case WidgetType::Animation:
          hasAnimation = true;
          put("var animation = state.get(" + key + ", () -> new AnimationState(" + boolean(p.animRunning) + "));");
          put("drawAnimation(state, animation, " + literal(n.label) + ", " + size + ", " + literal(p.animType) +
              ", " + literal(p.animEasing) + ", " + animationSeconds(p.animDuration) + ", " + boolean(p.animLoop) +
              ", " + (scaled ? "scale" : "1f") + ", (childState, animatedScale) -> {");
          if (CppCodegen::hasVisibleChildren(n)) {
            for (const auto& child : n.children) if (child.visible) put(childCall(child, "childState", "animatedScale"),3);
          } else put("ImGui.textUnformatted(\"Vista previa de animación\");",3);
          put("});");
          break;
        case WidgetType::Loader: case WidgetType::Component: case WidgetType::Repeater: {
          const bool loader = n.type == WidgetType::Loader, repeater = n.type == WidgetType::Repeater;
          const auto& title = !repeater && !p.sourceComponent.empty() ? p.sourceComponent : n.label;
          std::string flags = "ImGuiChildFlags.AlwaysUseWindowPadding";
          if (p.border) flags += " | ImGuiChildFlags.Border";
          if (n.height <= 0) flags += " | ImGuiChildFlags.AutoResizeY";
          put("if (ImGui.beginChild(\"template\", "+size+", "+flags+")) {");
          if (loader) {
            put("var loaded = state.get("+key+", () -> new ImBoolean("+boolean(p.loaderActive)+"));",3);
            put("if (ImGui.button(loaded.get() ? \"Descargar###toggle_loader\" : \"Cargar###toggle_loader\")) loaded.set(!loaded.get());",3);
            if (!title.empty()) put("ImGui.sameLine();",3);
          }
          if (!title.empty()) put("ImGui.textUnformatted("+literal(title)+");",3);
          if (repeater) {
            const auto count = std::to_string(std::clamp(p.repeatCount,1,kMaxRepeaterInstances));
            put("ImGui.textUnformatted(\""+count+" instancias\");",3);
            if (CppCodegen::hasVisibleChildren(n)) {
              hasRepeater = true;
              const auto gap = std::clamp(p.spacing,0.0f,48.0f);
              put("ImGui.pushStyleVar(ImGuiStyleVar.ItemSpacing, "+dimensions(gap,gap)+");",3);
              put("for (int instance = 0; instance < "+count+" && state.repeatBudget > 0; ++instance) {",3);
              put("--state.repeatBudget;",4);
              put("ImGui.pushID(instance);",4);
              put("if (ImGui.beginChild(\"instance\", new ImVec2(0, 0), ImGuiChildFlags.AutoResizeY)) {",4);
              children(5);
              put("}",4);
              put("ImGui.endChild();",4);
              put("ImGui.popID();",4);
              put("}",3);
              put("ImGui.popStyleVar();",3);
            }
          } else if (loader) {
            put("if (loaded.get()) {",3); children(4);
            put("} else ImGui.textUnformatted(\"Contenido descargado\");",3);
          } else {
            children(3);
          }
          put("}");
          call("EndChild");
          break;
        }
        case WidgetType::ListView: case WidgetType::GridView: case WidgetType::PathView: {
          hasModelViews = true;
          const auto modelSize = "new ImVec2(" + width + ", " +
            layout(n.height > 0 ? n.height : modelViewDefaultHeight(n.type)) + ")";
          const auto columns = std::to_string(n.type == WidgetType::GridView ? std::clamp(p.columns,1,kMaxModelColumns) : 0);
          put("var selection = state.get("+key+", () -> new ImInt("+std::to_string(p.selectedIndex)+"));");
          put("var items = state.get("+literal("items:"+n.id)+", () -> "+strings(p.items)+");");
          put("selection.set(DrawModelView("+literal(n.label)+", items, selection.get(), "+
              modelSize+", "+boolean(p.border)+", "+columns+", "+layout(std::clamp(p.spacing,0.0f,32.0f))+", "+
              boolean(n.type == WidgetType::PathView)+"));");
          break;
        }
        case WidgetType::PositionerFlow: {
          const float gap = std::max(0.0f, p.spacing);
          std::string flags = p.border ? "ImGuiChildFlags.Border" : "ImGuiChildFlags.None";
          if (n.height <= 0) flags += " | ImGuiChildFlags.AutoResizeY";
          put("if (ImGui.beginChild(\"flow\", "+size+", "+flags+")) {");
          put("ImGui.pushStyleVar(ImGuiStyleVar.ItemSpacing, "+dimensions(gap,gap)+");",3);
          const bool wraps = std::any_of(n.children.begin(), n.children.end(), [](const ImGuiNode& child) {
            return child.visible && !child.useCursorPos && child.type != WidgetType::SameLine && child.width >= 0;
          });
          if (wraps) {
            put("float right = ImGui.getCursorScreenPosX() + ImGui.getContentRegionAvailX();",3);
            put("Float previousRight = null;",3);
            put("var widths = ImGui.getStateStorage();",3);
          }
          for (const auto& child : n.children) {
            if (!child.visible || child.type == WidgetType::SameLine) continue;
            if (!wraps || child.useCursorPos) {
              put(childCall(child),3);
              if (wraps) put("previousRight = null;",3);
              continue;
            }
            put("{",3);
            put("int widthId = ImGui.getID("+literal("flow-width-"+CppCodegen::hexId(child.id))+");",4);
            if (child.width >= 0) {
              put("float width = widths.getFloat(widthId, -1f);",4);
              put("if (previousRight != null && width >= 0f && previousRight + "+layout(gap)+" + width <= right)",4);
              put("ImGui.sameLine(0f, "+layout(gap)+");",5);
            }
            put("float start = ImGui.getCursorScreenPosX();",4);
            put("ImGui.beginGroup();",4);
            put(childCall(child),4);
            put("ImGui.endGroup();",4);
            put("float end = ImGui.getItemRectMaxX();",4);
            put("previousRight = end;",4);
            put("widths.setFloat(widthId, Math.max(0f, end - start));",4);
            put("}",3);
          }
          put("ImGui.popStyleVar();",3);
          put("}");
          call("EndChild");
          break;
        }
        case WidgetType::Group: case WidgetType::ColumnLayout: case WidgetType::RowLayout: {
          call("BeginGroup");
          call("PushStyleVar","ImGuiStyleVar.ItemSpacing, "+dimensions(std::max(0.0f,p.spacing),std::max(0.0f,p.spacing)));
          bool firstChild = true;
          for (const auto& child : n.children) if (child.visible) {
            if (!firstChild && n.type==WidgetType::RowLayout) call("SameLine");
            put(childCall(child)); firstChild = false;
          }
          call("PopStyleVar"); call("EndGroup"); break;
        }
        case WidgetType::TabBar: container("ImGui.beginTabBar("+label+")","ImGui.endTabBar()"); break;
        case WidgetType::TabItem: container("ImGui.beginTabItem("+label+")","ImGui.endTabItem()"); break;
        case WidgetType::TreeNode: case WidgetType::CollapsingHeader:
          call("SetNextItemOpen",boolean(p.defaultOpen)+", ImGuiCond.Once");
          if (n.type == WidgetType::TreeNode) container("ImGui.treeNode("+label+")","ImGui.treePop()");
          else container("ImGui.collapsingHeader("+label+")","");
          break;
        case WidgetType::Table: case WidgetType::GridLayout: {
          const bool table = n.type == WidgetType::Table;
          const int count = std::clamp(p.columns,1,511);
          if (count!=p.columns) warn("Columnas limitadas a 1..511");
          std::string flags = table ? "ImGuiTableFlags.RowBg" : "ImGuiTableFlags.None";
          if (table && p.border) flags += " | ImGuiTableFlags.Borders";
          if (n.height>0) flags += " | ImGuiTableFlags.ScrollY";
          if (!table) call("PushStyleVar","ImGuiStyleVar.CellPadding, "+dimensions(std::max(0.0f,p.spacing)*0.5f,std::max(0.0f,p.spacing)*0.5f));
          put("if (ImGui.beginTable("+label+", "+std::to_string(count)+", "+flags+", "+size+")) {");
          if (table) {
            for (int i=0;i<count;++i) put("ImGui.tableSetupColumn("+literal(static_cast<size_t>(i)<p.columnLabels.size()?p.columnLabels[static_cast<size_t>(i)]:"Columna "+std::to_string(i+1))+");",3);
            put("ImGui.tableHeadersRow();",3);
          }
          for (const auto& child:n.children) if (child.visible) { put("ImGui.tableNextColumn();",3); put(childCall(child),3); }
          put("ImGui.endTable();",3); put("}");
          if (!table) call("PopStyleVar");
          break;
        }
        case WidgetType::MenuBar:
          if (titleItem) put("drawTitleMenu("+strings(p.items)+");");
          else if (p.menuBarEdge!=MenuBarEdge::Top) { warn("Menu fuera del borde superior pendiente"); call("TextUnformatted",literal("Menu: borde pendiente")); }
          else {
            put("if (ImGui.beginMenuBar()) {");
            for (size_t i=0;i<p.items.size();++i) {
              put("ImGui.pushID("+std::to_string(i)+");",3);
              put("if (ImGui.beginMenu("+literal(p.items[i])+")) {",3);
              put("ImGui.menuItem(\"Sin acciones configuradas\", \"\", false, false);",4);
              put("ImGui.endMenu();",4); put("}",3); put("ImGui.popID();",3);
            }
            put("ImGui.endMenuBar();",3); put("}");
          }
          break;
        default:
          warn("Widget pendiente: "+std::string(widgetTypeId(n.type)));
          call("TextUnformatted",literal("Widget pendiente: "+std::string(widgetTypeId(n.type)))); children(2); break;
      }
      if (!p.buttonAction.empty()) warn("La accion requiere codigo de la aplicacion");
      if (colors) call("PopStyleColor",std::to_string(colors));
      if (!titleItem && n.indent>0) call("Unindent",layout(n.indent));
      call("PopID");
    }
    put("}",1);
    result.nodeLines[n.id] = {first,lines.size()};
    lines.emplace_back();
  }
};
}

inline GeneratedCpp generateJavaCode(const StudioProject& project, std::vector<std::string>* imageResources = nullptr) {
  using namespace JavaCodegen;
  Emitter emitter;
  try {
    const auto valid = projectFromJson(projectToJson(project));
    if (!valid) throw std::runtime_error(valid.error);
    const auto& s = project.style;
    auto& lines = emitter.lines;
    lines = {"import imgui.*;", "import imgui.flag.*;", "import imgui.type.*;",
      "import java.util.HashMap;", "import java.util.Map;", "import java.util.function.Supplier;", "",
      "public final class StudioUI {",
      "    private StudioUI() {}",
      "",
      "    public static void applyStyle() {",
      s.preset==StylePresetId::ImGuiLight?"        ImGui.styleColorsLight();":s.preset==StylePresetId::ImGuiClassic?"        ImGui.styleColorsClassic();":"        ImGui.styleColorsDark();",
      "        var style = ImGui.getStyle();",
      "        style.setWindowRounding("+CppCodegen::number(s.windowRounding)+");",
      "        style.setFrameRounding("+CppCodegen::number(s.frameRounding)+");",
      "        style.setWindowPadding("+vecArgs(s.windowPaddingX,s.windowPaddingY)+");",
      "        style.setItemSpacing("+vecArgs(s.itemSpacingX,s.itemSpacingY)+");"};
    for (const auto& value : {std::pair<const char*,Color4>{"WindowBg",s.windowBg},{"TitleBgActive",s.titleBg},{"Button",s.buttonColor},{"FrameBg",s.frameBg},{"Text",s.textColor}}) {
      const auto& c = value.second;
      lines.push_back("        style.setColor(ImGuiCol."+std::string(value.first)+", "+vecArgs(c.r,c.g)+", "+vecArgs(c.b,c.a)+");");
    }
    const auto style = ProjectJson::styleJson(s);
    const std::pair<const char*,const char*> scalarStyles[] = {
      {"alpha","Alpha"},{"childRounding","ChildRounding"},{"grabRounding","GrabRounding"},
      {"tabRounding","TabRounding"},{"scrollbarRounding","ScrollbarRounding"},
      {"windowBorderSize","WindowBorderSize"},{"frameBorderSize","FrameBorderSize"}
    };
    for (const auto& field : scalarStyles)
      if (style.contains(field.first) && style.at(field.first).is_number())
        lines.push_back("        style.set"+std::string(field.second)+"("+CppCodegen::number(style.at(field.first).get<float>())+");");
    if (style.contains("framePaddingX") && style.at("framePaddingX").is_number() &&
        style.contains("framePaddingY") && style.at("framePaddingY").is_number())
      lines.push_back("        style.setFramePadding("+vecArgs(style.at("framePaddingX").get<float>(),style.at("framePaddingY").get<float>())+");");
    lines.insert(lines.end(),{"    }",""});
    collectScaledAnimationChildren(project.nodes, emitter.scaledNodes);
    CppCodegen::emitNodeFunctions(project.nodes, [&](const ImGuiNode& node, bool flowChild) { emitter.emit(node, flowChild); });
    lines.push_back("    public static void render(UIState state) {");
    if (emitter.hasRepeater)
      lines.push_back("        state.repeatBudget = "+std::to_string(ProjectJson::kMaxNodes)+";");
    for (const auto& key : emitter.frameButtons)
      lines.push_back("        state.get(" + key + ", ImBoolean::new).set(false);");
    for (const auto& node : project.nodes) if (node.visible) lines.push_back("        "+Emitter::function(node)+"(state);");
    lines.insert(lines.end(),{"    }",""});
    if (emitter.hasModelViews) lines.push_back(R"MODEL(
    private static void DrawModelLabel(String text, boolean centered, boolean featured) {
        if (!ImGui.isItemVisible()) return;
        var lo = ImGui.getItemRectMin();
        var hi = ImGui.getItemRectMax();
        var padding = ImGui.getStyle().getFramePadding();
        var extent = ImGui.calcTextSize(text);
        float x = centered ? Math.max(padding.x, (hi.x - lo.x - extent.x) * 0.5f) : padding.x;
        float y = Math.max(0f, (hi.y - lo.y - extent.y) * 0.5f);
        var draw = ImGui.getWindowDrawList();
        if (featured) draw.addRectFilled(lo, hi, ImGui.getColorU32(ImGuiCol.Header), ImGui.getStyle().getFrameRounding());
        draw.pushClipRect(new ImVec2(lo.x + padding.x, lo.y), new ImVec2(hi.x - padding.x, hi.y), true);
        draw.addText(new ImVec2(lo.x + x, lo.y + y), ImGui.getColorU32(ImGuiCol.Text), text);
        draw.popClipRect();
    }

    private static int DrawModelRows(String[] items, int value, int columns, boolean grid) {
        int selected = Math.max(0, Math.min(items.length - 1, value));
        float height = ImGui.getFrameHeight() * (grid ? 1.5f : 1f);
        var clip = new ImGuiListClipper();
        try {
            clip.begin((items.length + columns - 1) / columns);
            while (clip.step()) for (int row = clip.getDisplayStart(); row < clip.getDisplayEnd(); ++row) {
                if (grid) ImGui.tableNextRow();
                int start = row * columns;
                for (int i = start; i < Math.min(items.length, start + columns); ++i) {
                    if (grid) ImGui.tableNextColumn();
                    ImGui.pushID(i);
                    if (ImGui.selectable("##model_option", i == selected, ImGuiSelectableFlags.None, new ImVec2(0, height))) value = i;
                    DrawModelLabel(items[i], grid, false);
                    ImGui.popID();
                }
            }
        } finally { clip.destroy(); }
        return value;
    }

    private static int DrawModelView(String label, String[] items, int value, ImVec2 size, boolean border, int columns, float spacing, boolean path) {
        int flags = ImGuiChildFlags.AlwaysUseWindowPadding;
        if (border) flags |= ImGuiChildFlags.Border;
        ImGui.pushStyleColor(ImGuiCol.ChildBg, ImGui.getStyleColorVec4(ImGuiCol.FrameBg));
        if (ImGui.beginChild("model", size, flags)) {
            if (label.length() > 0) ImGui.textUnformatted(label);
            if (items.length == 0) ImGui.textDisabled("Sin elementos");
            else if (path) {
                int selected = ((value % items.length) + items.length) % items.length;
                ImGui.textUnformatted((selected + 1) + " / " + items.length);
                float arrow = ImGui.getFrameHeight();
                float width = Math.max(1f, ImGui.getContentRegionAvail().x - 2 * (arrow + ImGui.getStyle().getItemSpacingX()));
                ImGui.beginDisabled(items.length == 1);
                if (ImGui.arrowButton("previous", ImGuiDir.Left)) value = (selected + items.length - 1) % items.length;
                ImGui.endDisabled();
                ImGui.sameLine();
                ImGui.dummy(new ImVec2(width, arrow));
                DrawModelLabel(items[selected], true, true);
                ImGui.sameLine();
                ImGui.beginDisabled(items.length == 1);
                if (ImGui.arrowButton("next", ImGuiDir.Right)) value = (selected + 1) % items.length;
                ImGui.endDisabled();
            } else if (columns > 0) {
                ImGui.pushStyleVar(ImGuiStyleVar.CellPadding, new ImVec2(spacing * 0.5f, spacing * 0.5f));
                if (ImGui.beginTable("model_cells", columns, ImGuiTableFlags.SizingStretchSame)) {
                    value = DrawModelRows(items, value, columns, true);
                    ImGui.endTable();
                }
                ImGui.popStyleVar();
            } else value = DrawModelRows(items, value, 1, false);
        }
        ImGui.endChild();
        ImGui.popStyleColor();
        return value;
    }
)MODEL");
    if (emitter.hasChart2d || emitter.hasChart3d) lines.push_back(chartSampleFunctions);
    if (emitter.hasChart2d) lines.push_back(chart2dFunctions);
    if (emitter.hasChart3d) lines.push_back(chart3dFunctions);
    if (emitter.hasAnimation) lines.push_back(animationFunctions);
    if (emitter.hasImageWidgets) lines.push_back(imageDrawing);
    if (emitter.result.hasTitleButtons) lines.push_back(titleBarFunctions);
    const auto& images = emitter.imageAssets.items();
    emitter.result.hasImageTextures = !images.empty();
    if (!images.empty()) {
      lines.push_back("    private static final int[][] IMAGE_SIZES = {");
      for (const auto& asset : images)
        lines.push_back("        {" + std::to_string(asset.size.width) + ", " + std::to_string(asset.size.height) + "},");
      lines.push_back("    };");
      lines.push_back(imageLoading);
    }
    lines.insert(lines.end(),{"",
      "    public static final class UIState {",
      "        private final Map<String, Object> values = new HashMap<>();",
      "        @SuppressWarnings(\"unchecked\")",
      "        public <T> T get(String key, Supplier<T> create) {",
      "            return (T) values.computeIfAbsent(key, ignored -> create.get());",
      "        }"});
    if (emitter.hasRepeater) lines.push_back("        private int repeatBudget;");
    if (!images.empty()) lines.push_back("        private final Map<Integer, ImageTexture> imageTextures = new HashMap<>();");
    lines.insert(lines.end(),{"    }","}"});
    std::ostringstream output;
    for (const auto& line : lines) output << line << '\n';
    for (const auto& d : emitter.result.diagnostics) output << "// [" << CppCodegen::comment(d.nodeId) << "] " << CppCodegen::comment(d.message) << '\n';
    emitter.result.code = output.str();
    if (imageResources)
      for (const auto& asset : images) imageResources->push_back(asset.bytes);
  } catch (const std::exception& error) { emitter.result.error = error.what(); }
  return emitter.result;
}

inline CppBundle generateJavaBundle(const StudioProject& project) {
  CppBundle bundle;
  std::vector<std::string> imageResources;
  bundle.generated = generateJavaCode(project, &imageResources);
  const auto& generated = bundle.generated;
  if (!generated.error.empty()) return bundle;
  std::string report = "Java 17+ / imgui-java 1.90.0\n";
  for (const auto& d : generated.diagnostics)
    report += "["+CppCodegen::comment(d.nodeId)+"] "+CppCodegen::comment(d.message)+"\n";
  report += "\nRangos en StudioUI.java (inclusivos):\n";
  for (const auto& range : generated.nodeLines)
    report += CppCodegen::comment(range.first)+": "+std::to_string(range.second.first)+"-"+std::to_string(range.second.second)+"\n";
  std::string main = R"JAVA(import imgui.ImGui;
import imgui.app.Application;
import imgui.app.Configuration;
import java.util.Arrays;
import static org.lwjgl.glfw.GLFW.*;
)JAVA";
  if (generated.hasImageTextures) main += "import static org.lwjgl.opengl.GL11C.*;\n";
  main += R"JAVA(
public final class Main extends Application {
    private final StudioUI.UIState state = new StudioUI.UIState();
    private final boolean smoke;
    private int frames;
)JAVA";
  if (generated.hasImageTextures) main += JavaCodegen::imageBackend;
  main += R"JAVA(
    private Main(boolean smoke) { this.smoke = smoke; }

    @Override protected void configure(Configuration config) {
)JAVA";
  main += "        config.setTitle("+JavaCodegen::literal(project.name.substr(0,project.name.find('\0')))+");\n";
  main += "        config.setWidth("+std::to_string(project.canvas.width)+");\n";
  main += "        config.setHeight("+std::to_string(project.canvas.height)+");\n";
  main += R"JAVA(    }

    @Override protected void preRun() {
        if (smoke) {
            glfwHideWindow(getHandle());
            ImGui.getIO().setIniFilename(null);
        }
        StudioUI.applyStyle();
)JAVA";
  if (generated.hasImageTextures) main += "        imagesLoaded = StudioUI.loadImages(state, this::uploadImage);\n";
  main += R"JAVA(    }

    @Override public void process() {
        StudioUI.render(state);
        if (smoke && ++frames == 3) glfwSetWindowShouldClose(getHandle(), true);
    }

    public static void main(String[] args) {
        var app = new Main(Arrays.asList(args).contains("--smoke-test"));
        Application.launch(app);
        if (app.smoke) {
            if (app.frames < 3) throw new IllegalStateException("Smoke test ended before three frames");
)JAVA";
  if (generated.hasImageTextures)
    main += "            if (!app.imagesLoaded) throw new IllegalStateException(\"Unable to upload embedded images\");\n";
  main += R"JAVA(            System.out.println("imgui-java rendered three frames");
        }
    }
}
)JAVA";
  bundle.files = {
    {"StudioUI.java",generated.code},
    {"Main.java",main},
    {"pom.xml",R"XML(<project xmlns="http://maven.apache.org/POM/4.0.0">
  <modelVersion>4.0.0</modelVersion>
  <groupId>local.studio</groupId>
  <artifactId>studio-export</artifactId>
  <version>1.0.0</version>
  <properties>
    <maven.compiler.release>17</maven.compiler.release>
    <project.build.sourceEncoding>UTF-8</project.build.sourceEncoding>
  </properties>
  <dependencies>
    <dependency>
      <groupId>io.github.spair</groupId>
      <artifactId>imgui-java-app</artifactId>
      <version>1.90.0</version>
    </dependency>
  </dependencies>
  <build>
    <sourceDirectory>.</sourceDirectory>
    <plugins>
      <plugin>
        <groupId>org.apache.maven.plugins</groupId>
        <artifactId>maven-compiler-plugin</artifactId>
        <version>3.13.0</version>
        <configuration><includes><include>Main.java</include><include>StudioUI.java</include></includes></configuration>
      </plugin>
      <plugin>
        <groupId>org.apache.maven.plugins</groupId>
        <artifactId>maven-dependency-plugin</artifactId>
        <version>3.8.1</version>
      </plugin>
    </plugins>
  </build>
</project>
)XML"},
    {"codegen_report.txt",report},
    {"README.md",R"TXT(# Exportación Java

Requiere JDK 17 o superior, imgui-java 1.90.0 y LWJGL 3.3.6, incluidos los
JAR nativos para tu plataforma. Main.java inicia la ventana GLFW/OpenGL.
StudioUI.java también se puede integrar en una aplicación propia.

Si usas Maven, ejecuta mvn compile dependency:copy-dependencies. Luego:

    java --enable-native-access=ALL-UNNAMED -cp "target/classes;target/dependency/*" Main

En Linux/macOS cambia el separador del classpath a dos puntos. En macOS
añade -XstartOnFirstThread. La verificación de esta entrega se hizo en Windows.

Con los JAR ya disponibles en lib/ puedes usar directamente el JDK:

    javac --release 17 -encoding UTF-8 -cp "lib/*" -d out Main.java StudioUI.java
    java --enable-native-access=ALL-UNNAMED -cp "out;.;lib/*" Main

Para esa opción, coloca en lib/ imgui-java-app, imgui-java-binding e
imgui-java-lwjgl3 (versión 1.90.0), más lwjgl, lwjgl-glfw y lwjgl-opengl
(3.3.6) con sus JAR nativos del sistema. No se requiere Maven ni Gradle.

La opción --smoke-test oculta la ventana al arrancar y cierra tras tres
frames. Consulta codegen_report.txt para las omisiones y las líneas por widget.
)TXT"}
  };
  if (generated.hasImageTextures) {
    auto& pom = bundle.files["pom.xml"];
    pom.insert(pom.find("    <plugins>"), R"XML(    <resources>
      <resource><directory>.</directory><includes><include>studio_image_*.bin</include></includes></resource>
    </resources>
)XML");
    for (size_t index = 0; index < imageResources.size(); ++index)
      bundle.files["studio_image_" + std::to_string(index) + ".bin"] = std::move(imageResources[index]);
    bundle.files["README.md"] += "\nLos PNG y JPEG se exportan como recursos .bin del classpath. Maven los copia a target/classes.\n"
      "Si compilas con javac, conserva estos archivos en el classpath junto con las clases.\n"
      "Usa StudioUI.loadImages y StudioUI.releaseImages para otro backend. Consulta\n"
      "state.get(\"node:<id>\", ImBoolean::new) después de render para detectar el clic.\n";
  }
  if (generated.hasTitleButtons) bundle.files["README.md"] +=
    "\nConsulta state.get(\"node:<id>\", ImBoolean::new).get() después de render para detectar el clic de un botón de título.\n"
    "El indicador se reinicia en el siguiente render. Los botones de título de esta salida responden al ratón.\n";
  return bundle;
}

inline ProjectSaveResult exportJavaBundle(const StudioProject& project,const std::filesystem::path& destination) {
  const auto bundle = generateJavaBundle(project);
  if (!bundle.generated.error.empty()) return {false,bundle.generated.error};
  return writeGeneratedBundle(bundle,destination);
}
} // namespace ImGuiStudio
