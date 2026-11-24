#pragma once
#include "studio_cpp_export.hpp"
#include "studio_chart2d_csharp.hpp"
#include "studio_chart3d_csharp.hpp"
#include "studio_managed_animation.hpp"
#include "studio_managed_images.hpp"
#include "studio_image_data.hpp"
#include "studio_managed_titlebar.hpp"
#include "studio_titlebar_csharp.hpp"

namespace ImGuiStudio {
namespace CSharpCodegen {
inline std::string literal(const std::string& value) {
  // JSON escapes non-BMP characters as surrogate pairs, also accepted by C#.
  return Json(value).dump(-1,' ',true);
}
inline std::string vec(float x,float y) { return "new Vector2("+CppCodegen::number(x)+", "+CppCodegen::number(y)+")"; }
inline std::string color(const Color4& c) {
  return "new Vector4("+CppCodegen::number(c.r)+", "+CppCodegen::number(c.g)+", "+CppCodegen::number(c.b)+", "+CppCodegen::number(c.a)+")";
}
inline std::string strings(const std::vector<std::string>& items) {
  std::string result = "new string[] {";
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
        "TitleMenuWidth(" + strings(p.items) + ")" : number(child.width);
      lines.push_back("            new TitleButton(" + key + ", " +
        literal(ManagedTitleBar::caption(kind == 8 ? p.text : child.label)) +
        ", " + std::to_string(b.side) + ", " + std::to_string(b.variant) + ", " + std::to_string(kind ? 0 : b.icon) +
        ", " + width + ", " + number(child.height) + ", " + number(p.titleBarX) +
        ", " + (p.titleBarY ? number(*p.titleBarY) : "null") + ", " + number(b.rounding) +
        ", " + tint(b.background) + ", " + tint(b.foreground) + ", " + tint(b.hovered) + ", " + tint(b.active) +
        ", " + std::to_string(kind) + ", " + (kind ? function(child) : "null") + "),");
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
    const auto dimensions = [&](float x, float y) { return "new Vector2(" + layout(x) + ", " + layout(y) + ")"; };
    const auto width = n.width < 0 ? number(-std::numeric_limits<float>::min()) : layout(n.width);
    const auto size = "new Vector2(" + width + ", " + layout(std::max(0.0f,n.height)) + ")";
    auto put = [&](const std::string& line,int depth = 2) { lines.push_back(std::string(static_cast<size_t>(depth)*4,' ')+line); };
    auto call = [&](const std::string& name,const std::string& args = {}) { put("ImGui."+name+"("+args+");"); };
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
      put("var value = state.Get("+key+", "+initial+");");
      put("if (ImGui."+name+"("+label+", "+prefix+"ref value"+suffix+")) state.Set("+key+", value);");
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
      put("if (!state.Get("+key+", true)) return;");
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
        return value ? number(std::clamp(*value,0.0f,256.0f)) : std::string("ImGui.GetStyle().WindowPadding.") + axis;
      };
      if (a.paddingX || a.paddingY) {
        call("PushStyleVar", "ImGuiStyleVar.WindowPadding, new Vector2("+pad(a.paddingX,'X')+", "+pad(a.paddingY,'Y')+")");
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
        call("PushStyleVar","ImGuiStyleVar.FramePadding, new Vector2(ImGui.GetStyle().FramePadding.X, Math.Max(0f, ("+
          number(std::clamp(*a.titleHeight,20.0f,72.0f))+" - ImGui.GetFontSize()) * 0.5f))");
      if (trackGeometry) {
        const auto modeKey = literal("window-mode:" + n.id);
        put("var mode = state.GetOrCreate("+modeKey+", static () => new WindowMode("+vec(n.x,n.y)+", "+viewSize+"));");
        put("bool minimized = mode.Active == 1;");
        put("if (minimized) ImGui.PushStyleVar(ImGuiStyleVar.WindowMinSize, new Vector2(ImGui.GetStyle().WindowMinSize.X, 1));");
        put("PrepareWindowMode(mode);");
        flags += " | (mode.Active != 0 ? ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoResize | ImGuiWindowFlags.NoCollapse : 0)";
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
        put("var menuPosition = ImGui.GetStyle().WindowMenuButtonPosition;");
        put("ImGui.GetStyle().WindowMenuButtonPosition = ImGuiDir.None;");
        for (const auto slot : {"TitleBg","TitleBgActive","TitleBgCollapsed"})
          call("PushStyleColor", "ImGuiCol." + std::string(slot) + ", Vector4.Zero");
      }
      put("var flags = " + flags + ";");
      put("bool visible = ImGui.Begin("+(title.custom ? literal("###studio_" + id) : label)+", flags);");
      if (title.inset) {
        call("PopStyleColor","3");
        put("ImGui.GetStyle().WindowMenuButtonPosition = menuPosition;");
      }
      if (trackGeometry) put("if (minimized) ImGui.PopStyleVar();");
      if (title.shadow) put("DrawStudioShadow(" + std::to_string(title.shadow) + ");");
      const auto control = [&](const char* name, int kind, int side) {
        put("new TitleButton("+literal(std::string("##studio_window_")+name)+", \"\", " +
          std::to_string(side) + ", 0, 0, 0, 0, 0, null, 0, null, null, null, null, " + std::to_string(kind) + ", null),", 3);
      };
      if (title.custom) {
        put("RenderTitleBar(state, "+literal(ManagedTitleBar::caption(p.showTitleText ? n.label : ""))+", new TitleButton[] {");
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
          put("if (ImGui.BeginItemTooltip()) {"); put("ImGui.TextUnformatted("+literal(p.text)+");",3); put("ImGui.EndTooltip();",3); put("}"); break;
        case WidgetType::Button: call("Button",label+", "+(titleItem ? "new Vector2(titleWidth, titleHeight)" : size)); break;
        case WidgetType::SmallButton: call("SmallButton",label); break;
        case WidgetType::ColorButton: call("ColorButton",label+", "+color(p.color)+", ImGuiColorEditFlags.None, "+size); break;
        case WidgetType::ArrowButton:
          call("ArrowButton",label+", ImGuiDir."+(p.arrowDir=="left" ? "Left" : p.arrowDir=="up" ? "Up" : p.arrowDir=="down" ? "Down" : "Right")); break;
        case WidgetType::Checkbox: control("Checkbox",boolean(p.checked)); break;
        case WidgetType::Selectable: control("Selectable",boolean(p.selected),", ImGuiSelectableFlags.None, "+size); break;
        case WidgetType::RadioButton: control("RadioButton",std::to_string(p.intValue),", "+std::to_string(p.radioValue)); break;
        case WidgetType::InputText: case WidgetType::InputMultiline: {
          if (p.text.find('\0') != std::string::npos) warn("El control de texto muestra hasta el primer NUL");
          const auto bytes = std::max<size_t>(4096,p.text.size()+1024);
          const auto buffer = ", "+std::to_string(bytes)+"u";
          if (n.type == WidgetType::InputText)
            control("InputTextWithHint",literal(p.text),buffer,literal(p.hint)+", ");
          else control("InputTextMultiline",literal(p.text),buffer+", "+size);
          break;
        }
        case WidgetType::InputFloat: control("InputFloat",number(p.floatValue),speed+", 0f, "+format(false,"%.3f")); break;
        case WidgetType::InputInt: {
          const int step = static_cast<int>(std::clamp(static_cast<double>(p.speed),1.0,100000000.0));
          control("InputInt",std::to_string(p.intValue),", "+std::to_string(step)+", "+std::to_string(step*10));
          if (!p.format.empty() && p.format != "%d") warn("InputInt usa formato decimal");
          break;
        }
        case WidgetType::SliderFloat: control("SliderFloat",number(p.floatValue),fr+", "+format(false,"%.3f")); break;
        case WidgetType::SliderAngle: control("SliderAngle",number(p.floatValue),fr+", "+format(false,"%.0f deg")); break;
        case WidgetType::SliderInt: control("SliderInt",std::to_string(p.intValue),ir+", "+format(true,"%d")); break;
        case WidgetType::DragFloat: control("DragFloat",number(p.floatValue),speed+fr+", "+format(false,"%.3f")); break;
        case WidgetType::DragInt: control("DragInt",std::to_string(p.intValue),speed+ir+", "+format(true,"%d")); break;
        case WidgetType::SliderFloat2: case WidgetType::SliderFloat3: case WidgetType::DragFloat3: {
          const auto value = [&](size_t i) { return i<p.defaultVec.size()?p.defaultVec[i]:0.0f; };
          const bool two = n.type == WidgetType::SliderFloat2;
          const auto initial = two ? vec(value(0),value(1)) : "new Vector3("+number(value(0))+", "+number(value(1))+", "+number(value(2))+")";
          if (p.defaultVec.size()>(two?2u:3u)) warn("Componentes adicionales conservados solo en el proyecto");
          control(two?"SliderFloat2":n.type==WidgetType::SliderFloat3?"SliderFloat3":"DragFloat3",initial,
            (n.type==WidgetType::DragFloat3?speed:"")+fr+", "+format(false,"%.3f"));
          break;
        }
        case WidgetType::ColorEdit3:
          control("ColorEdit3","new Vector3("+number(p.color.r)+", "+number(p.color.g)+", "+number(p.color.b)+")"); break;
        case WidgetType::ColorEdit4: control("ColorEdit4",color(p.color)); break;
        case WidgetType::Combo: case WidgetType::Listbox:
          if (p.items.empty()) call("TextUnformatted",literal(n.label+" (sin opciones)"));
          else {
            put("var items = state.GetOrCreate("+literal("items:"+n.id)+", static () => "+strings(p.items)+");");
            control(n.type==WidgetType::Combo?"Combo":"ListBox",std::to_string(p.selectedIndex),", items, items.Length");
          }
          break;
        case WidgetType::PlotLines: case WidgetType::PlotHistogram:
          if (p.plotValues.empty()) call("TextUnformatted",literal(n.label+" (sin datos)"));
          else {
            std::string values = "new float[] {";
            for (float sample : p.plotValues) values += number(sample)+", ";
            put("var values = state.GetOrCreate("+key+", static () => "+values+"});");
            call(n.type==WidgetType::PlotLines?"PlotLines":"PlotHistogram",label+", ref values[0], values.Length, 0, "+literal(p.overlay)+fr+", "+size);
          }
          break;
        case WidgetType::Chart2d: {
          hasChart2d = true;
          const auto& mode = Chart2d::modes[Chart2d::modeIndex(p.chart2dType)];
          if (p.chart2dType != mode.id) warn("Tipo de gráfico desconocido. Se genera como línea.");
          std::string values = "new float[] {";
          for (float sample : p.plotValues) values += number(sample)+", ";
          values += "}";
          put("var values = state.GetOrCreate("+key+", static () => "+values+");");
          put("var labels = state.GetOrCreate("+literal("chart-labels:"+n.id)+", static () => "+strings(p.chartLabels)+");");
          put("DrawChart2d("+literal(mode.id)+", "+literal(p.overlay.empty() ? mode.label : p.overlay)+
            ", "+size+", "+color(p.color)+", values, labels, "+
            (p.chartAutoMin ? "null" : number(p.minFloat))+", "+(p.chartAutoMax ? "null" : number(p.maxFloat))+");");
          break;
        }
        case WidgetType::Chart3d: {
          hasChart3d = true;
          const auto& mode = Chart3d::modes[Chart3d::modeIndex(p.chart3dType)];
          if (p.chart3dType != mode.id) warn("Tipo de gráfico 3D desconocido. Se generan barras.");
          std::string values = "new float[] {";
          for (float sample : p.plotValues) values += number(sample)+", ";
          values += "}";
          put("var chart = state.GetOrCreate("+key+", static () => new Chart3dState("+values+", "+
              number(p.chart3dRotX)+", "+number(p.chart3dRotY)+"));");
          put("DrawChart3d("+literal(mode.id)+", "+literal(p.overlay.empty() ? mode.label : p.overlay)+
            ", "+size+", "+color(p.color)+", chart);");
          break;
        }
        case WidgetType::ProgressBar: call("ProgressBar",number(std::clamp(p.progress,0.0f,1.0f))+", "+size+", "+(p.overlay.empty()?"null":literal(p.overlay))); break;
        case WidgetType::Separator: call("Separator"); break;
        case WidgetType::SeparatorText: call("SeparatorText",literal(n.label)); break;
        case WidgetType::Spacing: call("Spacing"); break;
        case WidgetType::SameLine: call("SameLine"); break;
        case WidgetType::Dummy:
          if (n.width < 0) call("Dummy","new Vector2(ImGui.GetContentRegionAvail().X, "+layout(std::max(0.0f,n.height))+")");
          else call("Dummy",size);
          break;
        case WidgetType::ChildWindow:
          container("ImGui.BeginChild("+label+", "+size+", "+(p.border?"ImGuiChildFlags.Borders":"ImGuiChildFlags.None")+")","ImGui.EndChild()",true); break;
        case WidgetType::Image: case WidgetType::ImageButton: {
          hasImageWidgets = true;
          std::string texture = "default";
          if (p.imageAsset) {
            if (const auto index = imageAssets.add(*p.imageAsset))
              texture = "state.ImageTextures.GetValueOrDefault(" + std::to_string(*index) + ")";
            else warn("Datos de imagen no válidos. Se genera un marcador.");
          }
          const bool button = n.type == WidgetType::ImageButton;
          const auto draw = "DrawImage(" + texture + ", " + literal(n.label) + ", " + size + ", " +
            (scaled ? "scale" : "1f") + ", " + boolean(button) + ")";
          if (button) {
            frameButtons.push_back(key);
            put("if (" + draw + ") state.Set(" + key + ", true);");
            if (!p.buttonAction.empty()) warn("La acción de botón requiere integración de la aplicación.");
          } else put(draw + ";");
          break;
        }
        case WidgetType::Animation:
          hasAnimation = true;
          put("var animation = state.GetOrCreate(" + key + ", static () => new AnimationState(" +
              boolean(p.animRunning) + "));");
          put("DrawAnimation(state, animation, " + literal(n.label) + ", " + size + ", " + literal(p.animType) +
              ", " + literal(p.animEasing) + ", " + animationSeconds(p.animDuration) + ", " +
              boolean(p.animLoop) + ", " + (scaled ? "scale" : "1f") + ", static (childState, animatedScale) => {");
          if (CppCodegen::hasVisibleChildren(n)) {
            for (const auto& child : n.children) if (child.visible) put(childCall(child, "childState", "animatedScale"),3);
          } else put("ImGui.TextUnformatted(\"Vista previa de animación\");",3);
          put("});");
          break;
        case WidgetType::Loader: case WidgetType::Component: case WidgetType::Repeater: {
          const bool loader = n.type == WidgetType::Loader, repeater = n.type == WidgetType::Repeater;
          const auto& title = !repeater && !p.sourceComponent.empty() ? p.sourceComponent : n.label;
          std::string flags = "ImGuiChildFlags.AlwaysUseWindowPadding";
          if (p.border) flags += " | ImGuiChildFlags.Borders";
          if (n.height <= 0) flags += " | ImGuiChildFlags.AutoResizeY";
          put("if (ImGui.BeginChild(\"template\", "+size+", "+flags+")) {");
          if (loader) {
            put("bool loaded = state.Get("+key+", "+boolean(p.loaderActive)+");",3);
            put("if (ImGui.Button(loaded ? \"Descargar###toggle_loader\" : \"Cargar###toggle_loader\")) {",3);
            put("loaded = !loaded;",4);
            put("state.Set("+key+", loaded);",4);
            put("}",3);
            if (!title.empty()) put("ImGui.SameLine();",3);
          }
          if (!title.empty()) put("ImGui.TextUnformatted("+literal(title)+");",3);
          if (repeater) {
            const auto count = std::to_string(std::clamp(p.repeatCount,1,kMaxRepeaterInstances));
            put("ImGui.TextUnformatted(\""+count+" instancias\");",3);
            if (CppCodegen::hasVisibleChildren(n)) {
              hasRepeater = true;
              const auto gap = std::clamp(p.spacing,0.0f,48.0f);
              put("ImGui.PushStyleVar(ImGuiStyleVar.ItemSpacing, "+dimensions(gap,gap)+");",3);
              put("for (int instance = 0; instance < "+count+" && state.RepeatBudget > 0; ++instance) {",3);
              put("--state.RepeatBudget;",4);
              put("ImGui.PushID(instance);",4);
              put("if (ImGui.BeginChild(\"instance\", Vector2.Zero, ImGuiChildFlags.AutoResizeY)) {",4);
              children(5);
              put("}",4);
              put("ImGui.EndChild();",4);
              put("ImGui.PopID();",4);
              put("}",3);
              put("ImGui.PopStyleVar();",3);
            }
          } else if (loader) {
            put("if (loaded) {",3); children(4);
            put("} else ImGui.TextUnformatted(\"Contenido descargado\");",3);
          } else {
            children(3);
          }
          put("}");
          call("EndChild");
          break;
        }
        case WidgetType::ListView: case WidgetType::GridView: case WidgetType::PathView: {
          hasModelViews = true;
          const auto modelSize = "new Vector2(" + width + ", " +
            layout(n.height > 0 ? n.height : modelViewDefaultHeight(n.type)) + ")";
          const auto columns = std::to_string(n.type == WidgetType::GridView ? std::clamp(p.columns,1,kMaxModelColumns) : 0);
          put("var items = state.GetOrCreate("+literal("items:"+n.id)+", static () => "+strings(p.items)+");");
          put("state.Set("+key+", DrawModelView("+literal(n.label)+", items, state.Get("+key+", "+std::to_string(p.selectedIndex)+"), "+
              modelSize+", "+boolean(p.border)+", "+columns+", "+layout(std::clamp(p.spacing,0.0f,32.0f))+", "+
              boolean(n.type == WidgetType::PathView)+"));");
          break;
        }
        case WidgetType::PositionerFlow: {
          const float gap = std::max(0.0f, p.spacing);
          std::string flags = p.border ? "ImGuiChildFlags.Borders" : "ImGuiChildFlags.None";
          if (n.height <= 0) flags += " | ImGuiChildFlags.AutoResizeY";
          put("if (ImGui.BeginChild(\"flow\", "+size+", "+flags+")) {");
          put("ImGui.PushStyleVar(ImGuiStyleVar.ItemSpacing, "+dimensions(gap,gap)+");",3);
          const bool wraps = std::any_of(n.children.begin(), n.children.end(), [](const ImGuiNode& child) {
            return child.visible && !child.useCursorPos && child.type != WidgetType::SameLine && child.width >= 0;
          });
          if (wraps) {
            put("float right = ImGui.GetCursorScreenPos().X + ImGui.GetContentRegionAvail().X;",3);
            put("float? previousRight = null;",3);
            put("var widths = ImGui.GetStateStorage();",3);
          }
          for (const auto& child : n.children) {
            if (!child.visible || child.type == WidgetType::SameLine) continue;
            if (!wraps || child.useCursorPos) {
              put(childCall(child),3);
              if (wraps) put("previousRight = null;",3);
              continue;
            }
            put("{",3);
            put("uint widthId = ImGui.GetID("+literal("flow-width-"+CppCodegen::hexId(child.id))+");",4);
            if (child.width >= 0) {
              put("float width = widths.GetFloat(widthId, -1f);",4);
              put("if (previousRight != null && width >= 0f && previousRight + "+layout(gap)+" + width <= right)",4);
              put("ImGui.SameLine(0f, "+layout(gap)+");",5);
            }
            put("float start = ImGui.GetCursorScreenPos().X;",4);
            put("ImGui.BeginGroup();",4);
            put(childCall(child),4);
            put("ImGui.EndGroup();",4);
            put("float end = ImGui.GetItemRectMax().X;",4);
            put("previousRight = end;",4);
            put("widths.SetFloat(widthId, Math.Max(0f, end - start));",4);
            put("}",3);
          }
          put("ImGui.PopStyleVar();",3);
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
        case WidgetType::TabBar: container("ImGui.BeginTabBar("+label+")","ImGui.EndTabBar()"); break;
        case WidgetType::TabItem: container("ImGui.BeginTabItem("+label+")","ImGui.EndTabItem()"); break;
        case WidgetType::TreeNode: case WidgetType::CollapsingHeader:
          call("SetNextItemOpen",boolean(p.defaultOpen)+", ImGuiCond.Once");
          if (n.type == WidgetType::TreeNode) container("ImGui.TreeNode("+label+")","ImGui.TreePop()");
          else container("ImGui.CollapsingHeader("+label+")","");
          break;
        case WidgetType::Table: case WidgetType::GridLayout: {
          const bool table = n.type == WidgetType::Table;
          const int count = std::clamp(p.columns,1,511);
          if (count!=p.columns) warn("Columnas limitadas a 1..511");
          std::string flags = table ? "ImGuiTableFlags.RowBg" : "ImGuiTableFlags.None";
          if (table && p.border) flags += " | ImGuiTableFlags.Borders";
          if (n.height>0) flags += " | ImGuiTableFlags.ScrollY";
          if (!table) call("PushStyleVar","ImGuiStyleVar.CellPadding, "+dimensions(std::max(0.0f,p.spacing)*0.5f,std::max(0.0f,p.spacing)*0.5f));
          put("if (ImGui.BeginTable("+label+", "+std::to_string(count)+", "+flags+", "+size+")) {");
          if (table) {
            for (int i=0;i<count;++i) put("ImGui.TableSetupColumn("+literal(static_cast<size_t>(i)<p.columnLabels.size()?p.columnLabels[static_cast<size_t>(i)]:"Columna "+std::to_string(i+1))+");",3);
            put("ImGui.TableHeadersRow();",3);
          }
          for (const auto& child:n.children) if (child.visible) { put("ImGui.TableNextColumn();",3); put(childCall(child),3); }
          put("ImGui.EndTable();",3); put("}");
          if (!table) call("PopStyleVar");
          break;
        }
        case WidgetType::MenuBar:
          if (titleItem) put("DrawTitleMenu("+strings(p.items)+");");
          else if (p.menuBarEdge!=MenuBarEdge::Top) { warn("Menu fuera del borde superior pendiente"); call("TextUnformatted",literal("Menu: borde pendiente")); }
          else {
            put("if (ImGui.BeginMenuBar()) {");
            for (size_t i=0;i<p.items.size();++i) {
              put("ImGui.PushID("+std::to_string(i)+");",3);
              put("if (ImGui.BeginMenu("+literal(p.items[i])+")) {",3);
              put("ImGui.MenuItem(\"Sin acciones configuradas\", \"\", false, false);",4);
              put("ImGui.EndMenu();",4); put("}",3); put("ImGui.PopID();",3);
            }
            put("ImGui.EndMenuBar();",3); put("}");
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

inline GeneratedCpp generateCSharpCode(const StudioProject& project, std::vector<std::string>* imageResources = nullptr) {
  using namespace CSharpCodegen;
  Emitter emitter;
  try {
    const auto valid = projectFromJson(projectToJson(project));
    if (!valid) throw std::runtime_error(valid.error);
    auto& lines = emitter.lines;
    const auto& s = project.style;
    lines = {"using System;", "using System.Collections.Generic;", "using System.Numerics;", "using ImGuiNET;", "",
      "namespace StudioExport;", "",
      "public static class StudioUI", "{",
      "    public static void ApplyStyle()", "    {",
      s.preset==StylePresetId::ImGuiLight?"        ImGui.StyleColorsLight();":s.preset==StylePresetId::ImGuiClassic?"        ImGui.StyleColorsClassic();":"        ImGui.StyleColorsDark();",
      "        var style = ImGui.GetStyle();",
      "        style.WindowRounding = "+CppCodegen::number(s.windowRounding)+";",
      "        style.FrameRounding = "+CppCodegen::number(s.frameRounding)+";",
      "        style.WindowPadding = "+vec(s.windowPaddingX,s.windowPaddingY)+";",
      "        style.ItemSpacing = "+vec(s.itemSpacingX,s.itemSpacingY)+";"};
    for (const auto& value : {std::pair<const char*,Color4>{"WindowBg",s.windowBg},{"TitleBgActive",s.titleBg},{"Button",s.buttonColor},{"FrameBg",s.frameBg},{"Text",s.textColor}})
      lines.push_back("        style.Colors[(int)ImGuiCol."+std::string(value.first)+"] = "+color(value.second)+";");
    const auto style = ProjectJson::styleJson(s);
    const std::pair<const char*,const char*> scalarStyles[] = {
      {"alpha","Alpha"},{"childRounding","ChildRounding"},{"grabRounding","GrabRounding"},
      {"tabRounding","TabRounding"},{"scrollbarRounding","ScrollbarRounding"},
      {"windowBorderSize","WindowBorderSize"},{"frameBorderSize","FrameBorderSize"}
    };
    for (const auto& field : scalarStyles)
      if (style.contains(field.first) && style.at(field.first).is_number())
        lines.push_back("        style."+std::string(field.second)+" = "+CppCodegen::number(style.at(field.first).get<float>())+";");
    if (style.contains("framePaddingX") && style.at("framePaddingX").is_number() &&
        style.contains("framePaddingY") && style.at("framePaddingY").is_number())
      lines.push_back("        style.FramePadding = "+vec(style.at("framePaddingX").get<float>(),style.at("framePaddingY").get<float>())+";");
    lines.insert(lines.end(),{"    }",""});
    collectScaledAnimationChildren(project.nodes, emitter.scaledNodes);
    CppCodegen::emitNodeFunctions(project.nodes, [&](const ImGuiNode& node, bool flowChild) { emitter.emit(node, flowChild); });
    lines.insert(lines.end(),{"    public static void Render(UIState state)","    {"});
    if (emitter.hasRepeater)
      lines.push_back("        state.RepeatBudget = "+std::to_string(ProjectJson::kMaxNodes)+";");
    for (const auto& button : emitter.frameButtons) lines.push_back("        state.Set(" + button + ", false);");
    for (const auto& node : project.nodes) if (node.visible) lines.push_back("        "+Emitter::function(node)+"(state);");
    lines.insert(lines.end(),{"    }",""});
    if (emitter.hasModelViews) lines.push_back(R"MODEL(
    private static void DrawModelLabel(string text, bool centered, bool featured) {
        if (!ImGui.IsItemVisible()) return;
        var lo = ImGui.GetItemRectMin();
        var hi = ImGui.GetItemRectMax();
        var padding = ImGui.GetStyle().FramePadding;
        var extent = ImGui.CalcTextSize(text);
        float x = centered ? Math.Max(padding.X, (hi.X - lo.X - extent.X) * 0.5f) : padding.X;
        float y = Math.Max(0f, (hi.Y - lo.Y - extent.Y) * 0.5f);
        var draw = ImGui.GetWindowDrawList();
        if (featured) draw.AddRectFilled(lo, hi, ImGui.GetColorU32(ImGuiCol.Header), ImGui.GetStyle().FrameRounding);
        draw.PushClipRect(new Vector2(lo.X + padding.X, lo.Y), new Vector2(hi.X - padding.X, hi.Y), true);
        draw.AddText(new Vector2(lo.X + x, lo.Y + y), ImGui.GetColorU32(ImGuiCol.Text), text);
        draw.PopClipRect();
    }

    private static unsafe int DrawModelRows(string[] items, int value, int columns, bool grid) {
        int selected = Math.Max(0, Math.Min(items.Length - 1, value));
        float height = ImGui.GetFrameHeight() * (grid ? 1.5f : 1f);
        var clip = new ImGuiListClipperPtr(ImGuiNative.ImGuiListClipper_ImGuiListClipper());
        try {
            clip.Begin((items.Length + columns - 1) / columns);
            while (clip.Step()) for (int row = clip.DisplayStart; row < clip.DisplayEnd; ++row) {
                if (grid) ImGui.TableNextRow();
                int start = row * columns;
                for (int i = start; i < Math.Min(items.Length, start + columns); ++i) {
                    if (grid) ImGui.TableNextColumn();
                    ImGui.PushID(i);
                    if (ImGui.Selectable("##model_option", i == selected, ImGuiSelectableFlags.None, new Vector2(0, height))) value = i;
                    DrawModelLabel(items[i], grid, false);
                    ImGui.PopID();
                }
            }
        } finally { clip.Destroy(); }
        return value;
    }

    private static int DrawModelView(string label, string[] items, int value, Vector2 size, bool border, int columns, float spacing, bool path) {
        var flags = ImGuiChildFlags.AlwaysUseWindowPadding;
        if (border) flags |= ImGuiChildFlags.Borders;
        ImGui.PushStyleColor(ImGuiCol.ChildBg, ImGui.GetStyle().Colors[(int)ImGuiCol.FrameBg]);
        if (ImGui.BeginChild("model", size, flags)) {
            if (label.Length > 0) ImGui.TextUnformatted(label);
            if (items.Length == 0) ImGui.TextDisabled("Sin elementos");
            else if (path) {
                int selected = ((value % items.Length) + items.Length) % items.Length;
                ImGui.TextUnformatted((selected + 1) + " / " + items.Length);
                float arrow = ImGui.GetFrameHeight();
                float width = Math.Max(1f, ImGui.GetContentRegionAvail().X - 2 * (arrow + ImGui.GetStyle().ItemSpacing.X));
                ImGui.BeginDisabled(items.Length == 1);
                if (ImGui.ArrowButton("previous", ImGuiDir.Left)) value = (selected + items.Length - 1) % items.Length;
                ImGui.EndDisabled();
                ImGui.SameLine();
                ImGui.Dummy(new Vector2(width, arrow));
                DrawModelLabel(items[selected], true, true);
                ImGui.SameLine();
                ImGui.BeginDisabled(items.Length == 1);
                if (ImGui.ArrowButton("next", ImGuiDir.Right)) value = (selected + 1) % items.Length;
                ImGui.EndDisabled();
            } else if (columns > 0) {
                ImGui.PushStyleVar(ImGuiStyleVar.CellPadding, new Vector2(spacing * 0.5f, spacing * 0.5f));
                if (ImGui.BeginTable("model_cells", columns, ImGuiTableFlags.SizingStretchSame)) {
                    value = DrawModelRows(items, value, columns, true);
                    ImGui.EndTable();
                }
                ImGui.PopStyleVar();
            } else value = DrawModelRows(items, value, 1, false);
        }
        ImGui.EndChild();
        ImGui.PopStyleColor();
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
      lines.push_back("    private static readonly (int Width, int Height)[] ImageSizes = {");
      for (const auto& asset : images)
        lines.push_back("        (" + std::to_string(asset.size.width) + ", " + std::to_string(asset.size.height) + "),");
      lines.push_back("    };");
      lines.push_back(imageLoading);
    }
    lines.push_back("}");
    lines.insert(lines.end(),{"","public sealed class UIState","{",
      "    private readonly Dictionary<string, object> values = new();",
      "    public T Get<T>(string key, T initial) => values.TryGetValue(key, out var value) ? (T)value : initial;",
      "    public void Set<T>(string key, T value) where T : notnull => values[key] = value;",
      "    public T GetOrCreate<T>(string key, Func<T> create) where T : notnull",
      "    {", "        if (values.TryGetValue(key, out var cached)) return (T)cached;",
      "        T value = create();", "        values.Add(key, value);", "        return value;", "    }"});
    if (emitter.hasRepeater) lines.push_back("    internal int RepeatBudget;");
    if (!images.empty()) lines.push_back("    internal readonly Dictionary<int, StudioUI.ImageTexture> ImageTextures = new();");
    lines.push_back("}");
    std::ostringstream output;
    for (const auto& line : lines) output << line << '\n';
    for (const auto& d : emitter.result.diagnostics) output << "// [" << CppCodegen::comment(d.nodeId) << "] " << CppCodegen::comment(d.message) << '\n';
    emitter.result.code = output.str();
    if (imageResources) {
      imageResources->reserve(images.size());
      for (const auto& asset : images) imageResources->push_back(asset.bytes);
    }
  } catch (const std::exception& error) { emitter.result.error = error.what(); }
  return emitter.result;
}

inline CppBundle generateCSharpBundle(const StudioProject& project) {
  CppBundle bundle;
  std::vector<std::string> imageResources;
  bundle.generated = generateCSharpCode(project, &imageResources);
  const auto& generated = bundle.generated;
  if (!generated.error.empty()) return bundle;
  std::string report = "C# / .NET 8 / ImGui.NET 1.91.6.1 / Silk.NET 2.23.0\n";
  for (const auto& d : generated.diagnostics)
    report += "["+CppCodegen::comment(d.nodeId)+"] "+CppCodegen::comment(d.message)+"\n";
  report += "\nRangos en StudioUI.cs (inclusivos):\n";
  for (const auto& range : generated.nodeLines)
    report += CppCodegen::comment(range.first)+": "+std::to_string(range.second.first)+"-"+std::to_string(range.second.second)+"\n";
  std::string main = R"CS(using System;
using System.IO;
using System.Numerics;
using System.Runtime.InteropServices;
using ImGuiNET;
using Silk.NET.Input;
using Silk.NET.Maths;
using Silk.NET.OpenGL;
using Silk.NET.OpenGL.Extensions.ImGui;
using Silk.NET.Windowing;
using StudioExport;

if (Array.IndexOf(args, "--headless-test") >= 0)
    RunHeadless();
else
    RunWindow(Array.IndexOf(args, "--smoke-test") >= 0);

static void RunWindow(bool smoke)
{
    var options = WindowOptions.Default;
)CS";
  main += "    options.Title = "+CSharpCodegen::literal(project.name.substr(0,project.name.find('\0')))+";\n";
  main += "    options.Size = new Vector2D<int>("+std::to_string(project.canvas.width)+", "+std::to_string(project.canvas.height)+");\n";
  main += R"CS(    options.IsVisible = !smoke;
    if (OperatingSystem.IsLinux())
    {
        // Silk.NET's loader looks for GLFW by its soname. Load the matching NuGet asset first.
        var glfw = Path.Combine(AppContext.BaseDirectory, "runtimes", RuntimeInformation.RuntimeIdentifier,
            "native", "libglfw.so.3");
        if (File.Exists(glfw)) NativeLibrary.TryLoad(glfw, out _);
    }
    using var window = Window.Create(options);
    var state = new UIState();
    GL? gl = null;
    IInputContext? input = null;
    ImGuiController? controller = null;
    int frames = 0;
)CS";
  if (generated.hasImageTextures)
    main += "    ImageBackend? imageBackend = null;\n    bool imagesLoaded = false;\n";
  main += R"CS(
    void ReleaseGraphics()
    {
)CS";
  if (generated.hasImageTextures) main += R"CS(        if (imageBackend is { } images)
        {
            StudioUI.ReleaseImages(state, images.Release);
            imageBackend = null;
        }
)CS";
  main += R"CS(        controller?.Dispose();
        controller = null;
        input?.Dispose();
        input = null;
        gl?.Dispose();
        gl = null;
    }

    window.Load += () =>
    {
        gl = window.CreateOpenGL();
        input = window.CreateInput();
        controller = new ImGuiController(gl, window, input, () =>
        {
            unsafe { if (smoke) ImGui.GetIO().NativePtr->IniFilename = null; }
        });
        StudioUI.ApplyStyle();
)CS";
  if (generated.hasImageTextures) main += R"CS(        imageBackend = new ImageBackend(gl);
        imagesLoaded = StudioUI.LoadImages(state, imageBackend.Upload);
)CS";
  main += R"CS(    };
    window.Render += delta =>
    {
        if (controller is not { } gui || gl is not { } graphics) return;
        var size = window.FramebufferSize;
        if (size.X <= 0 || size.Y <= 0) return;
        graphics.Viewport(0, 0, (uint)size.X, (uint)size.Y);
        graphics.ClearColor(0.08f, 0.10f, 0.14f, 1f);
        graphics.Clear(ClearBufferMask.ColorBufferBit);
        gui.Update((float)Math.Max(delta, 0.001));
        StudioUI.Render(state);
        gui.Render();
        if (smoke)
        {
            var error = graphics.GetError();
            if ((uint)error != 0) throw new InvalidOperationException($"OpenGL error: {error}");
            if (++frames == 3) window.Close();
        }
    };
    window.Closing += ReleaseGraphics;
    try { window.Run(); }
    finally { ReleaseGraphics(); }
    if (smoke)
    {
        if (frames != 3) throw new InvalidOperationException("Window closed before three OpenGL frames");
)CS";
  if (generated.hasImageTextures)
    main += "        if (!imagesLoaded) throw new InvalidOperationException(\"Unable to upload embedded images\");\n";
  main += R"CS(        Console.WriteLine("ImGui.NET rendered three OpenGL frames.");
    }
}

static unsafe void RunHeadless()
{
    var context = ImGui.CreateContext();
    try
    {
        var io = ImGui.GetIO();
        io.NativePtr->IniFilename = null;
        io.DisplaySize = new Vector2(1280, 900);
        io.DeltaTime = 1f / 60f;
        io.Fonts.GetTexDataAsRGBA32(out IntPtr pixels, out _, out _, out _);
        StudioUI.ApplyStyle();
        var state = new UIState();
        for (int i = 0; i < 3; i++)
        {
            ImGui.NewFrame();
            StudioUI.Render(state);
            ImGui.Render();
        }
        Console.WriteLine("ImGui.NET rendered three frames in memory.");
    }
    finally { ImGui.DestroyContext(context); }
}
)CS";
  bundle.files = {
    {"StudioUI.cs",generated.code},
    {"StudioExport.csproj",R"XML(<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net8.0</TargetFramework>
    <OutputType>Exe</OutputType>
    <Nullable>enable</Nullable>
    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>
  </PropertyGroup>
  <ItemGroup>
    <PackageReference Include="ImGui.NET" Version="1.91.6.1" />
    <PackageReference Include="Silk.NET.OpenGL.Extensions.ImGui" Version="2.23.0" />
    <PackageReference Include="Silk.NET.Input.Glfw" Version="2.23.0" />
  </ItemGroup>
</Project>
)XML"},
    {"Program.cs",main},
    {"codegen_report.txt",report},
    {"README.md","C# / .NET 8\n\nEjecuta dotnet run en esta carpeta para abrir la ventana GLFW/OpenGL.\n"
     "Requiere OpenGL 3.3 y las bibliotecas nativas de la plataforma. NuGet restaura las dependencias del proyecto.\n"
     "dotnet run -- --smoke-test prueba tres frames OpenGL ocultos. dotnet run -- --headless-test prueba tres frames en memoria.\n"
     "Llama a StudioUI.ApplyStyle una vez, conserva una UIState y llama a StudioUI.Render cada frame.\n"
     "Revisa codegen_report.txt para las omisiones y ubicaciones por widget.\n"}
  };
  if (generated.hasImageTextures) {
    auto& projectFile = bundle.files["StudioExport.csproj"];
    std::string resources = "  <ItemGroup>\n    <PackageReference Include=\"StbImageSharp\" Version=\"2.30.16\" />\n";
    for (size_t i = 0; i < imageResources.size(); ++i) {
      const auto index = std::to_string(i);
      const auto name = "studio_image_" + index + ".bin";
      resources += "    <EmbeddedResource Include=\"" + name + "\" LogicalName=\"StudioExport.Images." + index + "\" />\n";
      bundle.files[name] = std::move(imageResources[i]);
    }
    resources += "  </ItemGroup>\n";
    projectFile.insert(projectFile.find("</Project>"), resources);
    bundle.files["ImageBackend.cs"] = CSharpCodegen::imageBackend;
    bundle.files["README.md"] += "\nEl ensamblado incluye los PNG y JPEG como recursos. Conserva sus archivos .bin al compilar.\n"
      "El ejecutable carga y libera las texturas con Silk.NET OpenGL. Con otro backend,\n"
      "usa StudioUI.LoadImages y StudioUI.ReleaseImages con tus callbacks. Un clic en un\n"
      "botón de imagen se lee con state.Get<bool>(\"node:<id>\", false) después de Render.\n";
  }
  if (generated.hasTitleButtons) bundle.files["README.md"] +=
    "\nConsulta state.Get<bool>(\"node:<id>\", false) después de Render para detectar el clic de un botón de título.\n"
    "El indicador se reinicia en el siguiente Render. Conserva la versión de cimgui que incluye ImGui.NET.\n";
  return bundle;
}

inline ProjectSaveResult exportCSharpBundle(const StudioProject& project,const std::filesystem::path& destination) {
  const auto bundle = generateCSharpBundle(project);
  if (!bundle.generated.error.empty()) return {false,bundle.generated.error};
  return writeGeneratedBundle(bundle,destination);
}
} // namespace ImGuiStudio
