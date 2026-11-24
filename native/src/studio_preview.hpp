#pragma once

#include "studio_types.hpp"
#include "studio_window_appearance.hpp"
#include "studio_images.hpp"
#include "studio_numeric_format.hpp"
#include "imgui.h"

namespace ImGuiStudio {

// Dear ImGui 1.91.5 requires a positive column count below 512.
inline constexpr int kPreviewMaxColumns = 511;

struct PreviewResizeWidth {
  float current;
  float minimum;
};

struct PreviewTitleBarBounds {
  float windowLeft, windowRight;
};

struct PreviewItem {
  std::string nodeId;
  WidgetType type;
  ImVec2 minimum, maximum;
  int optionIndex = -1;
  std::string parentId{}, rootId{};
  int depth = 0;
  ImVec2 coordinateOrigin{}, clipMinimum{}, clipMaximum{};
  ImDrawList* drawList = nullptr;
  bool windowHovered = false;
  bool hierarchy = false;
  bool hierarchyCollapsed = false;
  bool canvasBackground = false;
  bool navigation = false;
  std::optional<WidgetType> paletteType{};
  std::optional<PreviewResizeWidth> resizeWidth{};
  bool titleBar = false;
  const char* palettePreset = nullptr;
  std::optional<PreviewTitleBarBounds> titleBarBounds;
  std::shared_ptr<const ImGuiNode> paletteTemplate;
};

PreviewItem observeNativeItem(const ImGuiNode& node, const std::string& parentId,
                              const std::string& rootId, int depth, int option = -1);

struct PreviewReport {
  std::vector<PreviewItem> items;
  std::vector<std::string> activatedIds;
  std::vector<std::string> limitations;
};

struct ChartOrbit {
  std::string nodeId;
  Chart3d::Rotation rotation;
};

struct PreviewWindowState {
  bool minimized = false;
  bool maximized = false;
  bool closed = false;
  bool updateGeometry = false;
};

enum class TitleBarHit { None, Widget, WindowControl };

class ScopedPreviewWindowStyle {
 public:
  ScopedPreviewWindowStyle(const ImGuiNode& node, float scale, bool minimized);
  ~ScopedPreviewWindowStyle();
  ScopedPreviewWindowStyle(const ScopedPreviewWindowStyle&) = delete;
  ScopedPreviewWindowStyle& operator=(const ScopedPreviewWindowStyle&) = delete;
  void afterBegin() const;
 private:
  ImGuiStyle original_;
  ImVec4 titleColors_[3];
};

// Transient values belong to the renderer, never to the document being previewed.
// Geometry observations also support selection/hit testing in the native canvas.
class NativePreview {
 public:
  explicit NativePreview(NativeImageCache* images = nullptr) : images_(images) {}
  void reset();
  void setKnobsInstalled(bool installed) { knobsInstalled_ = installed; }
  void beginFrame(bool designMode = false, float scale = 1.0f, std::optional<ChartOrbit> orbit = std::nullopt);
  void render(const std::vector<ImGuiNode>& nodes, const std::string& rootId = {});
  static bool usesCustomTitle(const ImGuiNode& root);
  static bool canRenderInTitleBar(WidgetType type);
  TitleBarHit renderTitleBar(const ImGuiNode& root, PreviewWindowState* state = nullptr);
  void drawWindowShadow(const ImGuiNode& root, ImVec2 canvasMin, ImVec2 canvasMax) const;
  PreviewWindowState& windowState(const std::string& id) { return windows_[id]; }
  void renderWindowBody(const ImGuiNode& root);
  void recordWindow(const ImGuiNode& node);
  void recordTitleBar(const ImGuiNode& node);
  const PreviewReport& report() const { return report_; }
  const WidgetProps* value(const std::string& nodeId) const;
  static bool supports(WidgetType type);
 private:
  NativeImageCache* images_;
  std::map<std::string, WidgetProps> values_;
  std::map<std::string, int> radioGroups_;
  std::map<std::string, float> flowWidths_;
  std::map<std::string, double> animationStarts_;
  std::map<std::string, PreviewWindowState> windows_;
  PreviewReport report_;
  bool designMode_ = false;
  bool knobsInstalled_ = false;
  float scale_ = 1.0f;
  std::optional<ChartOrbit> orbit_;
  std::vector<std::string> parents_;
  std::string rootId_;
  void observe(const ImGuiNode& node, ImVec2 minimum, ImVec2 maximum, int option = -1,
               std::optional<PreviewResizeWidth> resizeWidth = std::nullopt, ImDrawList* containerDrawList = nullptr);
  void renderNode(const ImGuiNode& node, bool selectTab = false, bool flowChild = false);
};

class PlotSeriesEditor {
 public:
  bool render(ImGuiNode& node);
  void reset() { draft_.reset(); }

 private:
  struct Draft {
    std::string nodeId;
    std::vector<float> baseline;
    std::string text, error;
  };
  std::optional<Draft> draft_;
};

// The inspector edits a document copy. Applying a series is a discrete change.
bool renderWidgetProperties(ImGuiNode& node, PlotSeriesEditor& plotEditor);
void renderTitlePosition(WidgetProps& props);
void applyWindowChromePreset(WidgetProps& props, const WindowChrome::Preset& preset);
void applyPreviewStyle(const ImGuiStyleConfig& style);

} // namespace ImGuiStudio
