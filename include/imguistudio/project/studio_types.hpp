#pragma once

#include "../third_party/nlohmann/json.hpp"

#include <algorithm>
#include <chrono>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ImGuiStudio {

enum class WidgetType {
#define STUDIO_WIDGET(name, id, container) name,
#include "studio_widget_registry.inc"
#undef STUDIO_WIDGET
  Unknown
};

struct WidgetDefinition {
  WidgetType type;
  const char* id;
  const char* name;
  bool container;
};
inline constexpr WidgetDefinition kWidgetDefinitions[] = {
#define STUDIO_WIDGET(name, id, container) {WidgetType::name, id, #name, container},
#include "studio_widget_registry.inc"
#undef STUDIO_WIDGET
};

inline const char* widgetTypeId(WidgetType type) {
  for (const auto& def : kWidgetDefinitions) if (def.type == type) return def.id;
  return "unknown";
}
inline WidgetType widgetTypeFromId(const std::string& id) {
  for (const auto& def : kWidgetDefinitions) if (id == def.id) return def.type;
  return WidgetType::Unknown;
}

enum class MenuBarEdge {
  Top,
  Bottom,
  Left,
  Right,
  TitleBar,
  Free
};

enum class StylePresetId {
  ImGuiDark,
  ImGuiClassic,
  ImGuiLight,
  CyberCyan,
  NordFrost
};

struct Color4 {
  float r = 0.15f;
  float g = 0.45f;
  float b = 0.85f;
  float a = 1.0f;

  std::string toImVec4Expr() const {
    char buf[96];
    std::snprintf(buf, sizeof(buf), "ImVec4(%.2ff, %.2ff, %.2ff, %.2ff)", r, g, b, a);
    return std::string(buf);
  }
};

struct ImGuiStyleConfig {
  // Original JSON fields survive edits, copies, undo and snapshots. Null means new data.
  nlohmann::json source;
  StylePresetId preset = StylePresetId::ImGuiDark;
  float windowRounding = 6.0f;
  float frameRounding = 4.0f;
  float windowPaddingX = 10.0f;
  float windowPaddingY = 10.0f;
  float itemSpacingX = 8.0f;
  float itemSpacingY = 6.0f;
  Color4 windowBg{0.08f, 0.09f, 0.12f, 0.96f};
  Color4 titleBg{0.11f, 0.14f, 0.20f, 1.00f};
  Color4 buttonColor{0.18f, 0.43f, 0.76f, 0.85f};
  Color4 frameBg{0.14f, 0.17f, 0.23f, 0.90f};
  Color4 textColor{0.92f, 0.94f, 0.98f, 1.00f};
};

struct CanvasConfig {
  // Original JSON fields survive edits, copies, undo and snapshots. Null means new data.
  nlohmann::json source;
  bool landscape = true;
  std::string orientation = "landscape";
  int width = 1280;
  int height = 720;
};

struct WindowAppearance {
  std::string preset = "imgui", shadow = "none", titleIcon = "none";
  bool titleSeparator = false, transparentTitle = false;
  std::optional<float> titleHeight, titleOffsetLeft, titleOffsetRight;
  bool showResizeGrip = false;
  std::optional<float> rounding, borderSize, paddingX, paddingY, alpha;
  std::optional<Color4> background, titleBackground, borderColor, textColor, separatorColor;
};

struct WidgetProps {
  // Share large encoded images across document copies and undo states.
  std::shared_ptr<const nlohmann::json> imageAsset;
  // Original JSON fields survive edits, copies, undo and snapshots. Null means new data.
  nlohmann::json source;
  std::string text = "Texto";
  std::string hint = "Escribe aquí...";
  std::string buttonAction = "";
  bool checked = false;
  float floatValue = 0.5f;
  float minFloat = 0.0f;
  float maxFloat = 1.0f;
  float speed = 0.1f;
  std::string format;
  std::vector<float> defaultVec{0.0f, 0.0f, 0.0f};
  int intValue = 50;
  int minInt = 0;
  int maxInt = 100;
  float progress = 0.65f;
  Color4 color{0.22f, 0.68f, 0.95f, 1.0f};
  std::vector<std::string> items{"Opción A", "Opción B", "Opción C"};
  int selectedIndex = 0;
  int radioValue = 0;
  bool selected = false;
  std::string overlay;
  std::string arrowDir = "right";
  float spacing = 6.0f;
  std::vector<std::string> columnLabels;
  int columns = 2;
  bool noTitleBar = false;
  bool noResize = false;
  bool noMove = false;
  bool noCollapse = false;
  bool noBackground = false;
  bool closable = false, showCollapseArrow = false;
  bool showMinimizeButton = false, showMaximizeButton = false, showTitleText = true;
  std::string windowControlsSide = "right", windowControlsVariant = "icons", titleAlign = "left";
  WindowAppearance windowAppearance;
  bool inTitleBar = false;
  bool utilityCustomSize = false;
  std::string titleBarSide = "left", titleBarBtnVariant = "pill", titleBarIcon = "none";
  float titleBarX = 120.0f;
  std::optional<float> titleBarY, titleBarBtnRounding;
  bool border = true;
  bool defaultOpen = true;
  std::string sourceComponent;
  bool loaderActive = true;
  int repeatCount = 3;
  std::string animType = "pulse_alpha", animEasing = "sine";
  double animDuration = 1400;
  bool animLoop = true, animRunning = true;
  MenuBarEdge menuBarEdge = MenuBarEdge::Top;
  std::vector<float> plotValues{0.2f, 0.45f, 0.3f, 0.75f, 0.6f, 0.9f, 0.5f};
  std::string chart2dType = "line";
  std::vector<std::string> chartLabels;
  bool chartAutoMin = true, chartAutoMax = true;
  std::string chart3dType = "bars";
  float chart3dRotX = 28.0f, chart3dRotY = -38.0f;
};

struct ImGuiNode {
  // Original JSON fields survive edits, copies, undo and snapshots. Null means new data.
  nlohmann::json source;
  std::string id;
  WidgetType type = WidgetType::Button;
  std::string label = "Botón";
  std::string varName = "btn_1";
  std::string imguiId;
  float sameLineSpacing = 0.0f;
  float indent = 0.0f;
  float x = 48.0f;
  float y = 48.0f;
  float width = 140.0f;
  float height = 32.0f;
  bool sameLine = false;
  bool useCursorPos = false;
  bool visible = true;
  WidgetProps props;
  std::vector<ImGuiNode> children;
};

struct StudioProject {
  // Original JSON fields survive edits, copies, undo and snapshots. Null means new data.
  nlohmann::json source;
  std::string name = "MiProyectoImGui";
  ImGuiStyleConfig style;
  CanvasConfig canvas;
  std::vector<ImGuiNode> nodes;
};

inline std::string nextId(const std::string& prefix = "node") {
  static std::atomic<uint64_t> counter{1};
  static const auto session = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  return prefix + "_" + std::to_string(session) + "_" + std::to_string(counter.fetch_add(1));
}

inline std::string widgetTypeName(WidgetType t) {
  for (const auto& def : kWidgetDefinitions) if (def.type == t) return def.name;
  return "Unknown (preserved)";
}

inline std::string stylePresetName(StylePresetId preset) {
  switch (preset) {
    case StylePresetId::ImGuiDark: return "ImGui Dark";
    case StylePresetId::ImGuiClassic: return "ImGui Classic";
    case StylePresetId::ImGuiLight: return "ImGui Light";
    case StylePresetId::CyberCyan: return "Cyber Cyan";
    case StylePresetId::NordFrost: return "Nord Frost";
  }
  return "ImGui Dark";
}

inline std::string menuBarEdgeName(MenuBarEdge edge) {
  switch (edge) {
    case MenuBarEdge::Top: return "top";
    case MenuBarEdge::Bottom: return "bottom";
    case MenuBarEdge::Left: return "left";
    case MenuBarEdge::Right: return "right";
    case MenuBarEdge::TitleBar: return "titlebar";
    case MenuBarEdge::Free: return "free";
  }
  return "top";
}

inline bool isContainerType(WidgetType t) {
  for (const auto& def : kWidgetDefinitions) if (def.type == t) return def.container;
  return false;
}

inline bool isModelView(WidgetType type) {
  return type == WidgetType::ListView || type == WidgetType::GridView || type == WidgetType::PathView;
}

inline bool isImageWidget(WidgetType type) {
  return type == WidgetType::Image || type == WidgetType::ImageButton;
}

inline bool isTemplateContainer(WidgetType type) {
  return type == WidgetType::Loader || type == WidgetType::Component || type == WidgetType::Repeater;
}

inline constexpr int kMaxRepeaterInstances = 20;

namespace Chart2d {
struct Mode { const char* id; const char* label; const char* symbol; };
inline constexpr Mode modes[] = {
  {"line", "Línea", "StudioChartLine"}, {"area", "Área", "StudioChartArea"},
  {"bar", "Barras", "StudioChartBar"}, {"pie", "Dona", "StudioChartPie"},
  {"scatter", "Dispersión", "StudioChartScatter"}, {"spline", "Curva suave", "StudioChartSpline"}
};
inline size_t modeIndex(const std::string& type) {
  for (size_t i = 0; i < std::size(modes); ++i) if (type == modes[i].id) return i;
  return 0;
}
} // namespace Chart2d

namespace Chart3d {
struct Mode { const char* id; const char* label; const char* symbol; };
inline constexpr Mode modes[] = {
  {"bars", "Barras 3D", "StudioChart3dBars"},
  {"scatter", "Dispersión 3D", "StudioChart3dScatter"},
  {"surface", "Superficie 3D", "StudioChart3dSurface"}
};
inline size_t modeIndex(const std::string& type) {
  for (size_t i = 0; i < std::size(modes); ++i) if (type == modes[i].id) return i;
  return 0;
}
struct Rotation { float elevation, azimuth; };
inline Rotation orbit(Rotation start, float dx, float dy) {
  return {std::clamp(start.elevation + dy * 0.5f, 5.0f, 80.0f),
          std::remainder(start.azimuth + dx * 0.6f, 360.0f)};
}
inline std::vector<float> defaultSeries() {
  return {0.35f, 0.72f, 0.54f, 0.88f, 0.62f, 0.45f, 0.91f, 0.68f, 0.52f, 0.78f, 0.41f, 0.84f};
}
} // namespace Chart3d

inline constexpr int kMaxModelColumns = 6;

inline float modelViewDefaultHeight(WidgetType type) {
  if (type == WidgetType::GridView) return 140.0f;
  if (type == WidgetType::PathView) return 124.0f;
  return 120.0f;
}

inline bool canAcceptChild(WidgetType parent, WidgetType child) {
  if (!isContainerType(parent)) return false;
  if (child == WidgetType::Window) return false;
  if (parent == WidgetType::TabBar) return child == WidgetType::TabItem;
  if (child == WidgetType::TabItem) return false;
  if (child == WidgetType::TitleBarButton) return parent == WidgetType::Window;
  return true;
}

inline ImGuiNode createDefaultNode(WidgetType type, float x = 48.0f, float y = 48.0f) {
  ImGuiNode n;
  n.id = nextId("n");
  n.type = type;
  n.x = x;
  n.y = y;

  switch (type) {
    case WidgetType::Window:
      n.label = "Ventana Principal";
      n.varName = "show_main_window";
      n.width = 420.0f;
      n.height = 340.0f;
      break;
    case WidgetType::ChildWindow:
      n.label = "Panel Interior";
      n.varName = "child_panel";
      n.width = 280.0f;
      n.height = 140.0f;
      break;
    case WidgetType::Group:
      n.label = "Grupo";
      n.varName = "group_1";
      n.width = 240.0f;
      n.height = 100.0f;
      break;
    case WidgetType::MenuBar:
      n.label = "Barra de Menú";
      n.varName = "menu_bar";
      n.width = 0.0f;
      n.height = 26.0f;
      n.props.items = {"Archivo", "Editar", "Vista", "Ayuda"};
      break;
    case WidgetType::TabBar:
      n.label = "Pestañas";
      n.varName = "tab_bar";
      n.width = 320.0f;
      n.height = 140.0f;
      {
        ImGuiNode t1 = createDefaultNode(WidgetType::TabItem);
        t1.label = "General";
        ImGuiNode t2 = createDefaultNode(WidgetType::TabItem);
        t2.label = "Avanzado";
        n.children = {t1, t2};
      }
      break;
    case WidgetType::TabItem:
      n.label = "Pestaña";
      n.varName = "tab_item";
      n.width = 0.0f;
      n.height = 100.0f;
      break;
    case WidgetType::Table:
      n.label = "Tabla de Datos";
      n.varName = "data_table";
      n.width = 320.0f;
      n.height = 120.0f;
      n.props.columns = 3;
      break;
    case WidgetType::CollapsingHeader:
      n.label = "Sección Plegable";
      n.varName = "section_header";
      n.width = 260.0f;
      n.height = 90.0f;
      break;
    case WidgetType::TreeNode:
      n.label = "Nodo de Árbol";
      n.varName = "tree_node";
      n.width = 240.0f;
      n.height = 80.0f;
      break;
    case WidgetType::Text:
      n.label = "Etiqueta";
      n.varName = "lbl_text";
      n.width = 200.0f;
      n.height = 22.0f;
      n.props.text = "Hola desde Dear ImGui C++";
      break;
    case WidgetType::TextColored:
      n.label = "Estado";
      n.varName = "lbl_status";
      n.width = 200.0f;
      n.height = 22.0f;
      n.props.text = "Sistema listo";
      n.props.color = Color4{0.32f, 0.85f, 0.56f, 1.0f};
      break;
    case WidgetType::Button:
      n.label = "Ejecutar";
      n.varName = "btn_run";
      n.width = 140.0f;
      n.height = 30.0f;
      break;
    case WidgetType::TitleBarButton:
      n.label = "Acción"; n.varName = "title_button";
      n.width = 0; n.height = 0;
      break;
    case WidgetType::Image:
      n.label = "Imagen"; n.varName = "image";
      n.width = 160; n.height = 96;
      break;
    case WidgetType::ImageButton:
      n.label = "Botón de imagen"; n.varName = "image_button";
      n.width = 64; n.height = 48;
      break;
    case WidgetType::SmallButton:
      n.label = "Aplicar";
      n.varName = "btn_apply";
      n.width = 88.0f;
      n.height = 22.0f;
      break;
    case WidgetType::Checkbox:
      n.label = "Habilitar opción";
      n.varName = "opt_enabled";
      n.width = 180.0f;
      n.height = 24.0f;
      n.props.checked = true;
      break;
    case WidgetType::RadioButton:
      n.label = "Modo Alto";
      n.varName = "radio_mode";
      n.props.intValue = 0; n.props.floatValue = 0;
      n.width = 150.0f;
      n.height = 24.0f;
      break;
    case WidgetType::Toggle:
      n.label = "Interruptor";
      n.varName = "toggle";
      n.width = 44.0f;
      n.height = 24.0f;
      break;
    case WidgetType::Spinner:
      n.label = "Cargando";
      n.varName = "spinner";
      n.width = 36.0f;
      n.props.speed = 0.9f;
      break;
    case WidgetType::CircularProgress:
      n.label = "Progreso";
      n.varName = "circular_progress";
      n.width = 80.0f;
      n.props.progress = 0.65f;
      break;
    case WidgetType::Knob:
      n.label = "Perilla";
      n.varName = "knob";
      n.width = 80.0f;
      n.props.minFloat = 0.0f;
      n.props.maxFloat = 100.0f;
      n.props.floatValue = 35.0f;
      n.props.speed = 0.5f;
      n.props.format = "%.1f";
      break;
    case WidgetType::SliderFloat:
      n.label = "Intensidad";
      n.varName = "intensity";
      n.width = 220.0f;
      n.height = 26.0f;
      n.props.floatValue = 0.65f;
      break;
    case WidgetType::SliderInt:
      n.label = "Iteraciones";
      n.varName = "iterations";
      n.width = 220.0f;
      n.height = 26.0f;
      n.props.intValue = 32;
      break;
    case WidgetType::InputText:
      n.label = "Nombre";
      n.varName = "input_name";
      n.width = 220.0f;
      n.height = 26.0f;
      n.props.text = "ImGuiNative";
      break;
    case WidgetType::InputFloat:
      n.label = "Escala";
      n.varName = "scale_val";
      n.width = 190.0f;
      n.height = 26.0f;
      n.props.floatValue = 1.0f;
      break;
    case WidgetType::InputInt:
      n.label = "Puerto";
      n.varName = "port_num";
      n.width = 190.0f;
      n.height = 26.0f;
      n.props.intValue = 8080;
      break;
    case WidgetType::ColorEdit4:
      n.label = "Color Acento";
      n.varName = "accent_color";
      n.width = 220.0f;
      n.height = 26.0f;
      break;
    case WidgetType::Combo:
      n.label = "Calidad";
      n.varName = "quality_idx";
      n.width = 210.0f;
      n.height = 26.0f;
      n.props.items = {"Baja", "Media", "Alta", "Ultra"};
      break;
    case WidgetType::ProgressBar:
      n.label = "Progreso";
      n.varName = "progress_val";
      n.width = 220.0f;
      n.height = 22.0f;
      n.props.progress = 0.72f;
      break;
    case WidgetType::Separator:
      n.label = "Separador";
      n.varName = "sep";
      n.width = 200.0f;
      n.height = 8.0f;
      break;
    case WidgetType::Spacing:
      n.label = "Espaciado";
      n.varName = "space";
      n.width = 120.0f;
      n.height = 12.0f;
      break;
    case WidgetType::SameLine:
      n.label = "Misma Línea";
      n.varName = "same_line";
      n.width = 60.0f;
      n.height = 14.0f;
      break;
    case WidgetType::PlotLines:
      n.label = "Rendimiento (FPS)";
      n.varName = "fps_plot";
      n.width = 260.0f;
      n.height = 86.0f;
      break;
    case WidgetType::PlotHistogram:
      n.label = "Distribución";
      n.varName = "hist_plot";
      n.width = 260.0f;
      n.height = 86.0f;
      break;
    case WidgetType::Chart2d:
      n.label = "Gráfico 2D";
      n.varName = "chart_2d";
      n.width = 280; n.height = 148;
      n.props.maxFloat = 100;
      n.props.plotValues = {18, 32, 27, 54, 49, 72, 66, 84};
      break;
    case WidgetType::Chart3d:
      n.label = "Gráfico 3D";
      n.varName = "chart_3d";
      n.width = 300; n.height = 184;
      n.props.plotValues = Chart3d::defaultSeries();
      break;
    case WidgetType::ColumnLayout:
      n.label = "Columna";
      n.varName = "col_layout";
      n.width = 240.0f;
      n.height = 120.0f;
      break;
    case WidgetType::RowLayout:
      n.label = "Fila";
      n.varName = "row_layout";
      n.width = 280.0f;
      n.height = 56.0f;
      break;
    case WidgetType::PositionerFlow:
      n.label = "Flujo";
      n.varName = "flow_layout";
      n.width = 0.0f;
      n.height = 0.0f;
      break;
    case WidgetType::GridLayout:
      n.label = "Cuadrícula";
      n.varName = "grid_layout";
      n.width = 280.0f;
      n.height = 140.0f;
      n.props.columns = 2;
      break;
    case WidgetType::ListView:
      n.label = "Vista de lista"; n.varName = "list_view";
      n.width = -1; n.height = 120; n.props.selectedIndex = 0;
      n.props.items = {"Camera_Main", "Directional_Light", "Terrain_Mesh", "Sky_Atmosphere"};
      break;
    case WidgetType::GridView:
      n.label = "Vista de cuadricula"; n.varName = "grid_view";
      n.width = -1; n.height = 140; n.props.columns = 3; n.props.selectedIndex = 0;
      n.props.items = {"Material_01", "Shader_PBR", "Texture_HDR", "Mesh_Cube", "Audio_SFX", "Prefab_HUD"};
      break;
    case WidgetType::PathView:
      n.label = "Vista de recorrido"; n.varName = "path_view";
      n.width = -1; n.height = 124; n.props.selectedIndex = 2;
      n.props.items = {"Vista Frontal", "Perspectiva", "Wireframe", "Sombras", "Post-Proceso"};
      break;
    case WidgetType::Loader:
      n.label = "Loader"; n.varName = "loader";
      n.width = -1; n.height = 0; n.props.sourceComponent = "PanelDetalles";
      break;
    case WidgetType::Component:
      n.label = "Componente"; n.varName = "component";
      n.width = -1; n.height = 0; n.props.sourceComponent = "TarjetaControl";
      break;
    case WidgetType::Repeater:
      n.label = "Repetidor"; n.varName = "repeater";
      n.width = -1; n.height = 0; n.props.spacing = 4;
      break;
    case WidgetType::Animation:
      n.label = "Animación"; n.varName = "animation";
      n.width = -1; n.height = 0;
      break;
    case WidgetType::InputMultiline:
      n.label = "Texto multilinea"; n.varName = "multiline_" + n.id;
      n.width = 280; n.height = 100; n.props.text = "";
      break;
    case WidgetType::SliderAngle:
      n.label = "Rotacion"; n.varName = "angle_" + n.id;
      n.props.minFloat = -360; n.props.maxFloat = 360; n.props.floatValue = 0;
      n.props.format = "%.0f deg";
      break;
    case WidgetType::SliderFloat2:
      n.label = "Vector2"; n.varName = "vec2_" + n.id; n.props.defaultVec = {0, 0};
      break;
    case WidgetType::SliderFloat3: case WidgetType::DragFloat3:
      n.label = "Vector3"; n.varName = "vec3_" + n.id; n.props.defaultVec = {0, 0, 0};
      break;
    default:
      n.label = widgetTypeName(type);
      n.varName = std::string(widgetTypeId(type)) + "_" + n.id;
      break;
  }

  return n;
}

inline ImGuiNode createPaletteNode(WidgetType type, const char* preset) {
  auto node = createDefaultNode(type);
  if (!preset) return node;
  if (type == WidgetType::Chart2d) {
    const auto& mode = Chart2d::modes[Chart2d::modeIndex(preset)];
    node.props.chart2dType = mode.id;
    node.label = mode.label;
    node.props.overlay = mode.label;
  } else if (type == WidgetType::Chart3d) {
    const auto& mode = Chart3d::modes[Chart3d::modeIndex(preset)];
    node.props.chart3dType = mode.id;
    node.label = mode.label;
    node.props.overlay = mode.label;
  }
  return node;
}

inline bool isFreeLayoutUtility(WidgetType type) {
  return type == WidgetType::Separator || type == WidgetType::SeparatorText ||
    type == WidgetType::Spacing || type == WidgetType::Dummy;
}

inline bool isRoundStatusWidget(WidgetType type) {
  return type == WidgetType::Spinner || type == WidgetType::CircularProgress;
}

inline bool isStatusWidget(WidgetType type) {
  return type == WidgetType::Toggle || isRoundStatusWidget(type);
}

inline StudioProject createDefaultProject(const std::string& name = "ProyectoImGuiCpp") {
  StudioProject proj;
  proj.name = name;
  ImGuiNode win = createDefaultNode(WidgetType::Window, 48.0f, 48.0f);
  win.label = "Ventana Principal";
  proj.nodes.push_back(win);
  return proj;
}

inline ImGuiNode* findNodeById(std::vector<ImGuiNode>& nodes, const std::string& id) {
  for (auto& n : nodes) {
    if (n.id == id) return &n;
    if (!n.children.empty()) {
      if (auto* found = findNodeById(n.children, id)) return found;
    }
  }
  return nullptr;
}

inline const ImGuiNode* findNodeById(const std::vector<ImGuiNode>& nodes, const std::string& id) {
  for (const auto& n : nodes) {
    if (n.id == id) return &n;
    if (!n.children.empty()) {
      if (const auto* found = findNodeById(n.children, id)) return found;
    }
  }
  return nullptr;
}

inline bool removeNodeById(std::vector<ImGuiNode>& nodes, const std::string& id) {
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    if (it->id == id) {
      nodes.erase(it);
      return true;
    }
    if (!it->children.empty() && removeNodeById(it->children, id)) {
      return true;
    }
  }
  return false;
}

inline ImGuiNode cloneWithFreshIds(const ImGuiNode& src) {
  ImGuiNode copy = src;
  copy.id = nextId("n");
  copy.varName = src.varName + "_copy";
  copy.children.clear();
  for (const auto& c : src.children) {
    copy.children.push_back(cloneWithFreshIds(c));
  }
  return copy;
}

inline std::optional<std::string> duplicateNodeById(std::vector<ImGuiNode>& nodes, const std::string& id) {
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    if (it->id == id) {
      ImGuiNode dup = cloneWithFreshIds(*it);
      if (dup.type == WidgetType::Window) {
        dup.x += 28.0f;
        dup.y += 28.0f;
      }
      std::string newId = dup.id;
      nodes.insert(it + 1, dup);
      return newId;
    }
    if (!it->children.empty()) {
      auto res = duplicateNodeById(it->children, id);
      if (res.has_value()) return res;
    }
  }
  return std::nullopt;
}

struct ProjectStats {
  int windowCount = 0;
  int widgetCount = 0;
};

inline void countStatsWalk(const std::vector<ImGuiNode>& nodes, bool isRoot, ProjectStats& out) {
  for (const auto& n : nodes) {
    if (isRoot && n.type == WidgetType::Window) {
      out.windowCount += 1;
    } else {
      out.widgetCount += 1;
    }
    if (!n.children.empty()) {
      countStatsWalk(n.children, false, out);
    }
  }
}

inline ProjectStats countProjectStats(const StudioProject& proj) {
  ProjectStats stats;
  countStatsWalk(proj.nodes, true, stats);
  return stats;
}

}  // namespace ImGuiStudio
