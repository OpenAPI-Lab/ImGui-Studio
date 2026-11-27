#include "studio_canvas.hpp"
#include "imguistudio/localization/studio_locale.hpp"
#include "imguistudio/widgets/studio_widget_library.hpp"
#include "imgui_internal.h"
#include <cstdio>

namespace ImGuiStudio {
namespace {
constexpr float kDropCenterInset = 0.22f;

bool inside(ImVec2 point, ImVec2 minimum, ImVec2 maximum) {
  return point.x >= minimum.x && point.x < maximum.x && point.y >= minimum.y && point.y < maximum.y;
}
void outline(const PreviewItem& item, ImU32 color, float thickness = 2) {
  if (!item.drawList) return;
  item.drawList->PushClipRect(item.clipMinimum, item.clipMaximum, true);
  item.drawList->AddRect(item.minimum, item.maximum, color, 2, 0, thickness);
  item.drawList->PopClipRect();
}
struct ResizeHandle { ImVec2 minimum, maximum; };

std::optional<ResizeHandle> resizeHandle(const PreviewItem& item) {
  const auto axes = canvasResizeAxes(item.type);
  if (item.hierarchy || item.paletteType || item.canvasBackground || item.navigation || item.optionIndex != -1 ||
      item.titleBar || axes == CanvasResizeAxes::None ||
      (axes == CanvasResizeAxes::Width && !item.resizeWidth)) return std::nullopt;
  const float width = item.resizeWidth ? item.resizeWidth->current : item.maximum.x - item.minimum.x;
  const float height = item.maximum.y - item.minimum.y;
  if (width <= 0 || height <= 0) return std::nullopt;
  const float size = std::min({9.0f, width * 0.5f, height * 0.5f});
  const ImVec2 maximum(item.minimum.x + width,
    axes == CanvasResizeAxes::Width ? item.minimum.y + (height + size) * 0.5f : item.maximum.y);
  const ResizeHandle handle{{maximum.x - size, maximum.y - size}, maximum};
  if (handle.minimum.x < item.clipMinimum.x || handle.minimum.y < item.clipMinimum.y ||
      handle.maximum.x > item.clipMaximum.x || handle.maximum.y > item.clipMaximum.y) return std::nullopt;
  return handle;
}
void drawResizeHandle(const PreviewItem& item, const ResizeHandle& handle, bool hovered) {
  if (!item.drawList) return;
  item.drawList->PushClipRect(item.clipMinimum,item.clipMaximum,true);
  item.drawList->AddRectFilled(handle.minimum, handle.maximum,
    hovered ? IM_COL32(255, 218, 118, 255) : IM_COL32(56, 189, 248, 255),2);
  item.drawList->PopClipRect();
}
void insertionLine(const PreviewItem& item, bool after, bool horizontal, ImU32 color) {
  if (!item.drawList) return;
  item.drawList->PushClipRect(item.clipMinimum, item.clipMaximum, true);
  if (horizontal) {
    const float x = after ? item.maximum.x : item.minimum.x;
    item.drawList->AddLine(ImVec2(x, item.minimum.y), ImVec2(x, item.maximum.y), color, 3);
  } else {
    const float y = after ? item.maximum.y : item.minimum.y;
    item.drawList->AddLine(ImVec2(item.minimum.x, y), ImVec2(item.maximum.x, y), color, 3);
  }
  item.drawList->PopClipRect();
}
struct Destination { std::string parent; size_t index = 0; bool inside = false; };
std::optional<Destination> destination(const StudioProject& project, const PreviewItem& item, ImVec2 point) {
  const auto target = findNodeById(project.nodes, item.nodeId);
  if (!target) return std::nullopt;
  const float height = std::max(1.0f, item.maximum.y - item.minimum.y);
  const float fraction = (point.y - item.minimum.y) / height;
  if ((target->type == WidgetType::Window && !item.hierarchy) ||
      (isContainerType(target->type) && fraction > kDropCenterInset && fraction < 1.0f - kDropCenterInset))
    return Destination{target->id, target->children.size(), true};
  const auto location = locateNode(project.nodes, target->id);
  if (!location) return std::nullopt;
  bool after = fraction >= 0.5f;
  if (const auto parent = findNodeById(project.nodes, location->parentId); parent && !item.hierarchy)
    if (parent->type == WidgetType::RowLayout || parent->type == WidgetType::PositionerFlow)
      after = point.x >= (item.minimum.x + item.maximum.x) * 0.5f;
  return Destination{location->parentId, location->index + (after ? 1 : 0), false};
}
}

const PreviewItem* NativeCanvas::hitTest(const std::vector<PreviewItem>& items, ImVec2 point) {
  const PreviewItem* best = nullptr;
  float bestArea = std::numeric_limits<float>::max();
  for (const auto& item : items) {
    if (!item.windowHovered || item.navigation || item.optionIndex != -1 ||
        !inside(point, item.minimum, item.maximum) || !inside(point, item.clipMinimum, item.clipMaximum)) continue;
    const float area = (item.maximum.x - item.minimum.x) * (item.maximum.y - item.minimum.y);
    if (!best || item.depth > best->depth || (item.depth == best->depth && area < bestArea)) {
      best = &item; bestArea = area;
    }
  }
  return best;
}

bool NativeCanvas::dragHeld() const {
  return dragging_ && press_ && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
    ImGui::IsMousePosValid() && !ImGui::IsKeyPressed(ImGuiKey_Escape);
}

bool NativeCanvas::navigatingHierarchy() const {
  return dragHeld() && press_->gesture == Gesture::Move && press_->item.paletteType != WidgetType::Window;
}

ImVec2 NativeCanvas::edgePan(ImVec2 minimum, ImVec2 maximum) const {
  const auto& io = ImGui::GetIO();
  if (!dragHeld() || press_->gesture == Gesture::Resize || press_->gesture == Gesture::Orbit ||
      press_->gesture == Gesture::TitlePosition || press_->gesture == Gesture::TitleWidth ||
      !inside(io.MousePos, minimum, maximum)) return {};
  const float speed = 640.0f * std::min(io.DeltaTime, 0.05f);
  const auto step = [speed](float cursor, float start, float end) {
    const float margin = std::min(36.0f, (end - start) / 3.0f);
    const float fromStart = cursor - start, fromEnd = end - cursor;
    const float nearest = std::min(fromStart, fromEnd);
    if (nearest >= margin) return 0.0f;
    return (fromStart < fromEnd ? speed : -speed) * (1.0f - nearest / margin);
  };
  return {step(io.MousePos.x, minimum.x, maximum.x), step(io.MousePos.y, minimum.y, maximum.y)};
}

void NativeCanvas::scrollHierarchy() {
  if (!navigatingHierarchy() || !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) return;
  const auto* drawList = ImGui::GetWindowDrawList();
  const auto minimum = drawList->GetClipRectMin(), maximum = drawList->GetClipRectMax();
  const auto& io = ImGui::GetIO();
  if (!inside(io.MousePos, minimum, maximum)) return;
  const float margin = std::min(40.0f, (maximum.y - minimum.y) / 3.0f);
  const float fromTop = io.MousePos.y - minimum.y, fromBottom = maximum.y - io.MousePos.y;
  const float distance = std::min(fromTop, fromBottom);
  if (distance >= margin) return;
  const float step = std::max(1.0f, std::round(600.0f * io.DeltaTime * (1.0f - distance / margin)));
  const float current = ImGui::GetScrollY();
  const float next = std::clamp(current + (fromTop < fromBottom ? -step : step), 0.0f, ImGui::GetScrollMaxY());
  if (next == current) return;
  hierarchyHover_.reset();
  ImGui::SetScrollY(next);
}

bool NativeCanvas::shouldExpandHierarchy(const std::string& id) const {
  if (!hierarchyHover_ || hierarchyHover_->id != id || !navigatingHierarchy() ||
      !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) return false;
  return ImGui::GetTime() - hierarchyHover_->since >= 0.55 &&
    inside(ImGui::GetMousePos(), hierarchyHover_->minimum, hierarchyHover_->maximum);
}

CanvasUpdate NativeCanvas::process(const StudioProject& project, const std::vector<PreviewItem>& items,
    const std::vector<std::string>& selected, ImVec2 canvasOrigin, bool snap, float scale) {
  CanvasUpdate update;
  auto& io = ImGui::GetIO();
  const auto hovered = hitTest(items, io.MousePos);
  const PreviewItem* resizeItem = nullptr;
  const PreviewItem* titleResize = nullptr;
  bool leftTitleEdge = false;
  for (const auto& item : items) {
    if (item.paletteType || item.canvasBackground ||
        std::find(selected.begin(), selected.end(), item.nodeId) == selected.end()) continue;
    if (item.titleBarBounds) {
      if (selected.size() != 1) continue;
      const float size = std::min(10.0f, (item.maximum.x - item.minimum.x) * 0.5f);
      if (size < 1) continue;
      const float y = (item.minimum.y + item.maximum.y) * 0.5f;
      const ResizeHandle edges[] = {
        {{item.minimum.x, y - 4}, {item.minimum.x + size, y + 4}},
        {{item.maximum.x - size, y - 4}, {item.maximum.x, y + 4}},
      };
      for (size_t i = 0; i < std::size(edges); ++i) {
        const auto& edge = edges[i];
        const bool onEdge = item.windowHovered && inside(io.MousePos, item.clipMinimum, item.clipMaximum) &&
          inside(io.MousePos, edge.minimum, edge.maximum);
        drawResizeHandle(item, edge, onEdge);
        if (onEdge && !io.KeyCtrl) { titleResize = &item; leftTitleEdge = i == 0; }
      }
      continue;
    }
    outline(item, IM_COL32(56, 189, 248, 255));
    if (selected.size() != 1) continue;
    const auto handle = resizeHandle(item);
    if (!handle) continue;
    const bool onHandle = item.windowHovered && inside(io.MousePos, handle->minimum, handle->maximum);
    drawResizeHandle(item, *handle, onHandle);
    if (onHandle && !io.KeyCtrl) resizeItem = &item;
  }
  if (!ImGui::IsMousePosValid(&io.MousePos)) { cancel(); return update; }
  if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { cancel(); return update; }
  const auto* activeResize = press_ && press_->gesture == Gesture::Resize ? &press_->item : resizeItem;
  if (titleResize || (press_ && press_->gesture == Gesture::TitleWidth))
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
  else if (activeResize)
    ImGui::SetMouseCursor(canvasResizeAxes(activeResize->type) == CanvasResizeAxes::Width
      ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNWSE);
  if (!press_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && titleResize) {
    press_ = Press{*titleResize,io.MousePos,canvasOrigin,Gesture::TitleWidth,projectToJson(project).dump(),{},{}};
    press_->leftTitleEdge = leftTitleEdge;
    // Claim the title-bar press before Dear ImGui processes native window moving.
    ImGui::SetHoveredID(ImHashStr("##studio_title_width"));
  } else if (!press_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && resizeItem) {
    press_ = Press{*resizeItem,io.MousePos,canvasOrigin,Gesture::Resize,projectToJson(project).dump(),{},{}};
  } else if (!press_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered) {
    if (hovered->canvasBackground) update.selection.emplace();
    const auto node = findNodeById(project.nodes, hovered->nodeId);
    const auto member = std::find(selected.begin(), selected.end(), hovered->nodeId);
    if (node && io.KeyCtrl) {
      auto& toggled = update.selection.emplace(selected);
      if (member == selected.end()) toggled.push_back(node->id);
      else toggled.erase(toggled.begin() + (member - selected.begin()));
      return update;
    }
    const bool draggable = node && (node->type != WidgetType::Window || hovered->hierarchy);
    const bool orbit = node && node->type == WidgetType::Chart3d && !hovered->hierarchy && io.KeyShift;
    const bool useGroup = draggable && !orbit && member != selected.end() && selected.size() > 1;
    if (node && !useGroup) update.selection = std::vector<std::string>{node->id};
    if (node && hovered->titleBar && !hovered->hierarchy) {
      update.selection = std::vector<std::string>{node->id};
      press_ = Press{*hovered,io.MousePos,canvasOrigin,Gesture::TitlePosition,projectToJson(project).dump(),
                     {node->id},{}};
    } else if (orbit) {
      press_ = Press{*hovered,io.MousePos,canvasOrigin,Gesture::Orbit,projectToJson(project).dump(),
                     {node->id},{},{node->props.chart3dRotX,node->props.chart3dRotY}};
    } else if (hovered->paletteType || draggable) {
      const bool absolute = !hovered->hierarchy && !hovered->paletteType && node &&
        (node->useCursorPos || isFreeLayoutUtility(node->type) || io.KeyAlt);
      press_ = Press{*hovered,io.MousePos,canvasOrigin,absolute ? Gesture::Position : Gesture::Move,projectToJson(project).dump(),
                     useGroup ? selected : std::vector<std::string>{hovered->nodeId},{}};
      if (absolute) {
        for (const auto& id : press_->nodes) {
          const auto visible = std::find_if(items.begin(), items.end(), [&](const PreviewItem& item) {
            return item.nodeId == id && item.parentId == hovered->parentId && !item.hierarchy &&
              !item.navigation && item.optionIndex == -1 && item.type != WidgetType::Window &&
              item.minimum.x < item.clipMaximum.x && item.minimum.y < item.clipMaximum.y &&
              item.maximum.x > item.clipMinimum.x && item.maximum.y > item.clipMinimum.y;
          });
          if (visible != items.end()) press_->positionedItems.push_back(*visible);
        }
      }
    }
  }
  if (!press_) return update;
  const ImVec2 screenDelta(io.MousePos.x - press_->point.x, io.MousePos.y - press_->point.y);
  if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
      (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
       screenDelta.x * screenDelta.x + screenDelta.y * screenDelta.y >= io.MouseDragThreshold * io.MouseDragThreshold)) dragging_ = true;
  if (!dragging_) {
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      if (press_->nodes.size() > 1)
        update.selection = std::vector<std::string>{press_->item.nodeId};
      cancel();
    }
    return update;
  }
  DocumentEditResult planned;
  std::string message;
  std::optional<Destination> target;
  const ImVec2 delta(screenDelta.x / scale, screenDelta.y / scale);
  if (press_->gesture == Gesture::TitleWidth) {
    const auto& item = press_->item;
    const auto& full = *item.titleBarBounds;
    const bool left = press_->leftTitleEdge;
    const float original = left ? item.minimum.x - full.windowLeft : full.windowRight - item.maximum.x;
    const float canvasWidth = (item.maximum.x - item.minimum.x) / scale;
    const float maximum = std::min(4096.0f, original / scale + std::max(0.0f, canvasWidth - 48.0f));
    const float start = original / scale;
    const float requested = start + (left ? delta.x : -delta.x);
    const float margin = std::clamp(snap ? std::round(requested / 8.0f) * 8.0f : requested, 0.0f, maximum);
    ImVec2 lo = item.minimum, hi = item.maximum;
    if (left) lo.x = full.windowLeft + margin * scale;
    else hi.x = full.windowRight - margin * scale;
    auto* overlay = ImGui::GetForegroundDrawList();
    overlay->PushClipRect(item.clipMinimum, item.clipMaximum, true);
    overlay->AddRect(lo, hi, IM_COL32(56, 189, 248, 220), 2, 0, 2);
    overlay->PopClipRect();
    message = std::string(surfaceText(StudioText::TitleWidth)) + " (" + surfaceText(StudioText::DragEsc) + ")";
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && delta.x != 0 && margin != start) {
      auto next = project;
      if (auto* window = findNodeById(next.nodes, item.nodeId)) {
        auto& appearance = window->props.windowAppearance;
        (left ? appearance.titleOffsetLeft : appearance.titleOffsetRight) = margin;
        planned = checkedDocumentEdit(std::move(next), project);
      }
    }
  } else if (press_->gesture == Gesture::TitlePosition) {
    const auto& item = press_->item;
    const float start = (item.minimum.x - item.coordinateOrigin.x) / scale;
    const float nextX = std::max(0.0f, snap ? std::round((start + delta.x) / 8.0f) * 8.0f : start + delta.x);
    const float shift = (nextX - start) * scale;
    message = std::string(surfaceText(StudioText::TitlePosition)) + " (" + surfaceText(StudioText::DragEsc) + ")";
    auto* overlay = ImGui::GetForegroundDrawList();
    overlay->PushClipRect(item.clipMinimum, item.clipMaximum, true);
    overlay->AddRect(ImVec2(item.minimum.x + shift, item.minimum.y),
      ImVec2(item.maximum.x + shift, item.maximum.y), IM_COL32(56, 189, 248, 220), 2, 0, 2);
    overlay->PopClipRect();
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && screenDelta.x != 0) {
      auto next = project;
      if (auto* button = findNodeById(next.nodes, item.nodeId)) {
        button->props.titleBarSide = "free";
        button->props.titleBarX = nextX;
        planned = checkedDocumentEdit(std::move(next), project);
      }
    }
  } else if (press_->gesture == Gesture::Orbit) {
    const auto rotation = Chart3d::orbit(press_->rotation, delta.x, delta.y);
    orbit_ = ChartOrbit{press_->item.nodeId, rotation};
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    message = std::string(surfaceText(StudioText::ChartOrbit)) + " (" + surfaceText(StudioText::DragEsc) + ")";
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      auto next = project;
      if (auto* chart = findNodeById(next.nodes, orbit_->nodeId)) {
        chart->props.chart3dRotX = rotation.elevation;
        chart->props.chart3dRotY = rotation.azimuth;
        planned = checkedDocumentEdit(std::move(next), project);
      }
    }
  } else if (press_->gesture == Gesture::Resize) {
    const auto& initial = press_->item;
    const bool widthOnly = canvasResizeAxes(initial.type) == CanvasResizeAxes::Width;
    const float width = (initial.resizeWidth ? initial.resizeWidth->current : initial.maximum.x - initial.minimum.x) / scale;
    const float height = (initial.maximum.y - initial.minimum.y) / scale;
    const bool changeHeight = !widthOnly && delta.y != 0;
    const float minimumWidth = initial.resizeWidth ? initial.resizeWidth->minimum / scale : 8.0f;
    const auto size = ImVec2(delta.x == 0 ? width : canvasWidgetExtent(width + delta.x, snap, minimumWidth),
                            changeHeight ? canvasWidgetExtent(height + delta.y, snap) : height);
    auto* overlay = ImGui::GetForegroundDrawList();
    overlay->PushClipRect(initial.clipMinimum,initial.clipMaximum,true);
    overlay->AddRect(initial.minimum,ImVec2(initial.minimum.x + size.x * scale, initial.minimum.y + size.y * scale),
                     IM_COL32(56, 189, 248, 220),2,0,2);
    overlay->PopClipRect();
    char dimensions[128];
    if (widthOnly) std::snprintf(dimensions, sizeof(dimensions), "%.0f", size.x);
    else std::snprintf(dimensions, sizeof(dimensions), "%.0f × %.0f", size.x, size.y);
    if (widthOnly) message = std::string(surfaceText(StudioText::Width)) + ": ";
    message += std::string(dimensions) + " (" + surfaceText(StudioText::DragEsc) + ")";
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      const auto* node = findNodeById(project.nodes, initial.nodeId);
      const bool preserveExtent = node && isFreeLayoutUtility(node->type) && node->type != WidgetType::Dummy &&
        !node->useCursorPos && !node->props.utilityCustomSize && (delta.x != 0 || changeHeight);
      planned = resizeDocumentWidget(project,initial.nodeId,
        delta.x != 0 || preserveExtent ? std::optional<float>(size.x) : std::nullopt,
        changeHeight || preserveExtent ? std::optional<float>(size.y) : std::nullopt, false);
    }
  } else if (press_->gesture == Gesture::Position) {
    const auto& initial = press_->item;
    message = std::string(surfaceText(StudioText::FreePosition)) + " (" + surfaceText(StudioText::DragEsc) + ")";
    if (press_->positionedItems.size() != press_->nodes.size()) {
      planned.error = surfaceText(StudioText::FreeDropRequirement);
    } else {
      const float anchorX = (initial.minimum.x - initial.coordinateOrigin.x) / scale;
      const float anchorY = (initial.minimum.y - initial.coordinateOrigin.y) / scale;
      const auto displacement = [snap](float start, float offset) {
        return snap ? std::round((start + offset) / 8) * 8 - start : offset;
      };
      const ImVec2 cameraShift(canvasOrigin.x - press_->canvasOrigin.x, canvasOrigin.y - press_->canvasOrigin.y);
      const ImVec2 shift(displacement(anchorX, (screenDelta.x - cameraShift.x) / scale),
                         displacement(anchorY, (screenDelta.y - cameraShift.y) / scale));
      const ImVec2 screenShift(shift.x * scale + cameraShift.x, shift.y * scale + cameraShift.y);
      std::vector<NodePosition> positions;
      for (const auto& item : press_->positionedItems) {
        const auto current = std::find_if(items.begin(), items.end(), [&](const PreviewItem& observed) {
          return !observed.hierarchy && observed.optionIndex == -1 && observed.nodeId == item.nodeId;
        });
        if (current != items.end()) {
          auto* overlay = ImGui::GetForegroundDrawList();
          overlay->PushClipRect(current->clipMinimum, current->clipMaximum, true);
          overlay->AddRect(ImVec2(item.minimum.x + screenShift.x, item.minimum.y + screenShift.y),
            ImVec2(item.maximum.x + screenShift.x, item.maximum.y + screenShift.y),
            IM_COL32(56, 189, 248, 220), 2, 0, 2);
          overlay->PopClipRect();
        }
        positions.push_back({item.nodeId, (item.minimum.x - item.coordinateOrigin.x) / scale + shift.x,
                                         (item.minimum.y - item.coordinateOrigin.y) / scale + shift.y});
      }
      if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) planned = positionDocumentNodes(project, positions);
    }
  } else if (press_->item.paletteType && *press_->item.paletteType == WidgetType::Window) {
    if (!hovered || hovered->hierarchy || hovered->paletteType) {
      planned.error = surfaceText(StudioText::DropWindowHint);
    } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      auto window = press_->item.paletteTemplate ? instantiateLibraryWidget(*press_->item.paletteTemplate)
        : createDefaultNode(WidgetType::Window);
      window.x = (io.MousePos.x - canvasOrigin.x) / scale;
      window.y = (io.MousePos.y - canvasOrigin.y) / scale;
      if (snap) { window.x = std::round(window.x / 8) * 8; window.y = std::round(window.y / 8) * 8; }
      update.selection = std::vector<std::string>{window.id};
      planned = insertDocumentNode(project, std::move(window), {}, project.nodes.size());
    }
    message = surfaceText(StudioText::InDesign);
  } else if (hovered && !hovered->paletteType && !hovered->canvasBackground &&
             (target = destination(project, *hovered, io.MousePos))) {
    if (press_->item.paletteType && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      auto node = press_->item.paletteTemplate ? instantiateLibraryWidget(*press_->item.paletteTemplate)
        : createPaletteNode(*press_->item.paletteType, press_->item.palettePreset);
      if (isFreeLayoutUtility(node.type) && !hovered->hierarchy && !hovered->titleBar) {
        const auto root = std::find_if(items.begin(), items.end(), [&](const PreviewItem& item) {
          return item.nodeId == target->parent && item.type == WidgetType::Window &&
            item.optionIndex == -1 && !item.hierarchy;
        });
        if (root != items.end()) {
          const auto coordinate = [snap, scale](float pointer, float origin) {
            const float offset = (pointer - origin) / scale;
            return snap ? std::round(offset / 8.0f) * 8.0f : offset;
          };
          node.x = coordinate(io.MousePos.x, root->coordinateOrigin.x);
          node.y = coordinate(io.MousePos.y, root->coordinateOrigin.y);
          node.useCursorPos = true;
        }
      }
      update.selection = std::vector<std::string>{node.id};
      planned = insertDocumentNode(project, std::move(node), target->parent, target->index);
    } else if (!press_->item.paletteType && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      planned = moveDocumentNodes(project, press_->nodes, target->parent, target->index);
    } else if (!press_->item.paletteType) {
      DocumentMovePlan preview;
      planned.error = planDocumentMove(project, press_->nodes, target->parent, target->index, preview);
    }
    const auto parent = findNodeById(project.nodes, target->parent);
    const ImU32 color = planned.error.empty() ? IM_COL32(52, 211, 153, 255) : IM_COL32(248, 113, 113, 255);
    if (target->inside) {
      message = std::string(surfaceText(StudioText::Inside)) + ": " +
        (parent ? parent->label : surfaceText(StudioText::RootWindows));
      outline(*hovered, color);
    } else {
      const auto location = locateNode(project.nodes, hovered->nodeId);
      const bool after = location && target->index > location->index;
      const auto hoveredNode = findNodeById(project.nodes, hovered->nodeId);
      message = std::string(surfaceText(after ? StudioText::After : StudioText::Before)) + ": " +
        (hoveredNode ? hoveredNode->label : surfaceText(StudioText::Widgets));
      insertionLine(*hovered, after, parent && !hovered->hierarchy &&
        (parent->type == WidgetType::RowLayout || parent->type == WidgetType::PositionerFlow), color);
    }
  } else planned.error = surfaceText(StudioText::DropWidgetHint);
  if (hovered && hovered->hierarchyCollapsed && target && target->inside && planned.error.empty() &&
      navigatingHierarchy()) {
    const float rowHeight = hovered->maximum.y - hovered->minimum.y;
    const ImVec2 minimum(std::max(hovered->minimum.x, hovered->clipMinimum.x),
      std::max(hovered->minimum.y + rowHeight * kDropCenterInset, hovered->clipMinimum.y));
    const ImVec2 maximum(std::min(hovered->maximum.x, hovered->clipMaximum.x),
      std::min(hovered->maximum.y - rowHeight * kDropCenterInset, hovered->clipMaximum.y));
    const double since = hierarchyHover_ && hierarchyHover_->id == hovered->nodeId
      ? hierarchyHover_->since : ImGui::GetTime();
    hierarchyHover_ = HierarchyHover{hovered->nodeId, minimum, maximum, since};
  } else hierarchyHover_.reset();
  if (!planned.error.empty()) message = planned.error;
  ImGui::SetTooltip("%s", message.c_str());
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    if (projectToJson(project).dump() != press_->baseline)
      update.edit.error = "El proyecto cambio durante el arrastre; se cancelo el gesto";
    else update.edit = std::move(planned);
    if (!update.edit.project) update.selection.reset();
    cancel();
  } else if (!press_->item.paletteType) update.selection.reset();
  // A ghost candidate ID is never selected before insertion is committed.
  if (!update.edit.project && press_ && press_->item.paletteType) update.selection.reset();
  return update;
}

} // namespace ImGuiStudio
