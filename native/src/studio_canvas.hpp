#pragma once

#include "studio_document_edit.hpp"
#include "studio_preview.hpp"

namespace ImGuiStudio {

struct CanvasUpdate {
  std::optional<std::vector<std::string>> selection;
  DocumentEditResult edit;
};

class NativeCanvas {
 public:
  CanvasUpdate process(const StudioProject& project, const std::vector<PreviewItem>& items,
                       const std::vector<std::string>& selected, ImVec2 canvasOrigin, bool snap, float scale);
  void cancel() { press_.reset(); hierarchyHover_.reset(); orbit_.reset(); dragging_ = false; }
  bool dragging() const { return dragging_; }
  const std::optional<ChartOrbit>& orbitPreview() const { return orbit_; }
  ImVec2 edgePan(ImVec2 minimum, ImVec2 maximum) const;
  void scrollHierarchy();
  bool shouldExpandHierarchy(const std::string& id) const;
  static const PreviewItem* hitTest(const std::vector<PreviewItem>& items, ImVec2 point);
 private:
  enum class Gesture { Move, Position, Resize, Orbit, TitlePosition, TitleWidth };
  struct Press {
    PreviewItem item;
    ImVec2 point;
    ImVec2 canvasOrigin;
    Gesture gesture;
    std::string baseline;
    std::vector<std::string> nodes;
    std::vector<PreviewItem> positionedItems;
    Chart3d::Rotation rotation{};
    bool leftTitleEdge = false;
  };
  struct HierarchyHover {
    std::string id;
    ImVec2 minimum, maximum;
    double since;
  };
  bool navigatingHierarchy() const;
  bool dragHeld() const;
  std::optional<Press> press_;
  std::optional<HierarchyHover> hierarchyHover_;
  std::optional<ChartOrbit> orbit_;
  bool dragging_ = false;
};

} // namespace ImGuiStudio
