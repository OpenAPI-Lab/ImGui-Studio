#pragma once

#include "studio_serialization.hpp"
#include <iterator>

namespace ImGuiStudio {

struct NodeLocation {
  std::string parentId;
  size_t index = 0;
  size_t depth = 0;
};

inline std::optional<NodeLocation> locateNode(const std::vector<ImGuiNode>& nodes,
    const std::string& id, const std::string& parent = {}, size_t depth = 0) {
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].id == id) return NodeLocation{parent, i, depth};
    if (auto found = locateNode(nodes[i].children, id, nodes[i].id, depth + 1)) return found;
  }
  return std::nullopt;
}

inline bool containsNode(const ImGuiNode& node, const std::string& id) {
  return node.id == id || findNodeById(node.children, id) != nullptr;
}

inline size_t subtreeHeight(const ImGuiNode& node) {
  size_t height = 0;
  for (const auto& child : node.children) height = std::max(height, 1 + subtreeHeight(child));
  return height;
}

struct DocumentEditResult {
  std::optional<StudioProject> project;
  std::string error;
  explicit operator bool() const { return project.has_value(); }
};

inline DocumentEditResult checkedDocumentEdit(StudioProject next, const StudioProject& original) {
  try {
    const auto value = projectToJson(next);
    const auto valid = projectFromJson(value);
    if (!valid) return {std::nullopt, valid.error};
    if (value == projectToJson(original)) return {}; // A valid no-op adds no undo step.
    return {std::move(next), {}};
  } catch (const std::exception& e) { return {std::nullopt, e.what()}; }
}

struct DocumentMovePlan {
  std::string origin;
  std::vector<size_t> indices;
  size_t adjustedIndex = 0;
};

// index denotes a gap in the destination's original sibling list (0..size).
// This can also check a drop target without copying or serializing the project.
inline std::string planDocumentMove(const StudioProject& project, const std::vector<std::string>& ids,
    const std::string& parentId, size_t index, DocumentMovePlan& plan) {
  plan = {};
  const auto parent = parentId.empty() ? nullptr : findNodeById(project.nodes, parentId);
  if (!parentId.empty() && !parent) return "El contenedor de destino no existe";
  const size_t depth = parent ? locateNode(project.nodes, parentId)->depth + 1 : 0;
  const auto size = parent ? parent->children.size() : project.nodes.size();
  if (index > size) return "Posicion de insercion fuera del contenedor";
  plan.adjustedIndex = index;
  for (const auto& id : ids) {
    const auto location = locateNode(project.nodes, id);
    const auto source = findNodeById(project.nodes, id);
    if (!source || !location) return "El nodo de origen ya no existe";
    if (plan.indices.empty()) plan.origin = location->parentId;
    if (plan.origin != location->parentId) return "Para mover varios widgets deben compartir contenedor";
    if (std::find(plan.indices.begin(), plan.indices.end(), location->index) != plan.indices.end())
      return "Hay widgets repetidos en la seleccion";
    if (!parentId.empty() && containsNode(*source, parentId))
      return "No se puede mover un nodo dentro de si mismo o de sus hijos";
    if (parent ? !canAcceptChild(parent->type, source->type)
               : !(source->type == WidgetType::Window || (source->type == WidgetType::Unknown && plan.origin.empty())))
      return "Este contenedor no admite ese tipo de widget";
    if (depth > location->depth && depth + subtreeHeight(*source) > ProjectJson::kMaxDepth)
      return "Se superaria la profundidad maxima de la jerarquia";
    if (plan.origin == parentId && location->index < index) --plan.adjustedIndex;
    plan.indices.push_back(location->index);
  }
  return {};
}

inline DocumentEditResult moveDocumentNodes(const StudioProject& project, const std::vector<std::string>& ids,
    const std::string& parentId, size_t index) {
  if (ids.empty()) return {};
  DocumentMovePlan plan;
  if (auto error = planDocumentMove(project, ids, parentId, index, plan); !error.empty())
    return {std::nullopt, std::move(error)};
  auto& [origin, indices, adjusted] = plan;
  if (origin == parentId && indices.size() == 1 && adjusted == indices.front()) return {};
  std::sort(indices.begin(), indices.end());
  StudioProject next = project;
  auto& originalSiblings = origin.empty() ? next.nodes : findNodeById(next.nodes, origin)->children;
  std::vector<ImGuiNode> moved;
  moved.reserve(indices.size());
  for (size_t offset : indices) moved.push_back(std::move(originalSiblings[offset]));
  for (auto it = indices.rbegin(); it != indices.rend(); ++it)
    originalSiblings.erase(originalSiblings.begin() + static_cast<std::ptrdiff_t>(*it));
  auto& siblings = parentId.empty() ? next.nodes : findNodeById(next.nodes, parentId)->children;
  siblings.insert(siblings.begin() + static_cast<std::ptrdiff_t>(adjusted),
    std::make_move_iterator(moved.begin()), std::make_move_iterator(moved.end()));
  return checkedDocumentEdit(std::move(next), project);
}

inline DocumentEditResult moveDocumentNode(const StudioProject& project, const std::string& id,
    const std::string& parentId, size_t index) {
  return moveDocumentNodes(project, {id}, parentId, index);
}

inline DocumentEditResult insertDocumentNode(const StudioProject& project, ImGuiNode node,
    const std::string& parentId, size_t index) {
  const auto parent = parentId.empty() ? nullptr : findNodeById(project.nodes, parentId);
  if (!parentId.empty() && !parent) return {std::nullopt, "El contenedor de destino no existe"};
  if (parent ? !canAcceptChild(parent->type, node.type) : node.type != WidgetType::Window)
    return {std::nullopt, "Este contenedor no admite ese tipo de widget"};
  const size_t size = parent ? parent->children.size() : project.nodes.size();
  if (index > size) return {std::nullopt, "Posicion de insercion fuera del contenedor"};
  StudioProject next = project;
  auto& siblings = parentId.empty() ? next.nodes : findNodeById(next.nodes, parentId)->children;
  siblings.insert(siblings.begin() + static_cast<std::ptrdiff_t>(index), std::move(node));
  return checkedDocumentEdit(std::move(next), project);
}

struct NodePosition {
  std::string id;
  float x;
  float y;
};

inline DocumentEditResult positionDocumentNodes(const StudioProject& project, const std::vector<NodePosition>& positions) {
  StudioProject next = project;
  for (const auto& position : positions) {
    if (!std::isfinite(position.x) || !std::isfinite(position.y))
      return {std::nullopt, "La posicion debe ser finita"};
    auto node = findNodeById(next.nodes, position.id);
    if (!node || node->type == WidgetType::Window) return {std::nullopt, "Seleccione un widget para posicionarlo"};
    node->x = position.x; node->y = position.y;
    node->useCursorPos = true; node->sameLine = false;
  }
  return checkedDocumentEdit(std::move(next), project);
}

inline DocumentEditResult positionDocumentNode(const StudioProject& project, const std::string& id,
    float x, float y, bool snap) {
  if (snap) { x = std::round(x / 8) * 8; y = std::round(y / 8) * 8; }
  return positionDocumentNodes(project, {{id, x, y}});
}

enum class CanvasResizeAxes { None, Width, Both };

inline CanvasResizeAxes canvasResizeAxes(WidgetType type) {
  switch (type) {
    case WidgetType::Button: case WidgetType::ColorButton: case WidgetType::ChildWindow:
    case WidgetType::ProgressBar: case WidgetType::Toggle: case WidgetType::Dummy: case WidgetType::InputMultiline:
    case WidgetType::Listbox: case WidgetType::PlotLines: case WidgetType::PlotHistogram:
    case WidgetType::ListView: case WidgetType::GridView: case WidgetType::PathView:
    case WidgetType::Loader: case WidgetType::Component: case WidgetType::Repeater:
    case WidgetType::Selectable: case WidgetType::PositionerFlow:
    case WidgetType::Animation: case WidgetType::Image: case WidgetType::ImageButton:
    case WidgetType::Table: case WidgetType::GridLayout: case WidgetType::Chart2d: case WidgetType::Chart3d:
    case WidgetType::Separator: case WidgetType::SeparatorText: case WidgetType::Spacing:
      return CanvasResizeAxes::Both;
    case WidgetType::InputText: case WidgetType::InputFloat: case WidgetType::InputInt:
    case WidgetType::SliderFloat: case WidgetType::SliderInt: case WidgetType::SliderAngle:
    case WidgetType::DragFloat: case WidgetType::DragInt: case WidgetType::SliderFloat2:
    case WidgetType::SliderFloat3: case WidgetType::DragFloat3: case WidgetType::Combo:
    case WidgetType::ColorEdit3: case WidgetType::ColorEdit4: case WidgetType::Knob:
    case WidgetType::Spinner: case WidgetType::CircularProgress:
      return CanvasResizeAxes::Width;
    default: return CanvasResizeAxes::None;
  }
}

inline float canvasWidgetExtent(float extent, bool snap, float minimum = 8.0f) {
  const float lowerBound = snap ? std::ceil(minimum / 8.0f) * 8.0f : minimum;
  const float bounded = std::clamp(extent, lowerBound, 16000.0f);
  return snap ? std::round(bounded / 8.0f) * 8.0f : bounded;
}

inline DocumentEditResult resizeDocumentWidget(const StudioProject& project,const std::string& id,
    std::optional<float> width,std::optional<float> height,bool snap) {
  const auto source = findNodeById(project.nodes,id);
  const auto axes = source ? canvasResizeAxes(source->type) : CanvasResizeAxes::None;
  if (axes == CanvasResizeAxes::None)
    return {std::nullopt,"Seleccione un widget redimensionable"};
  if (height && axes == CanvasResizeAxes::Width)
    return {std::nullopt,"Este widget solo admite ajuste de ancho"};
  if ((width && !std::isfinite(*width)) || (height && !std::isfinite(*height)))
    return {std::nullopt,"El tamano debe ser finito"};
  if (!width && !height) return {};
  StudioProject next = project;
  auto* widget = findNodeById(next.nodes,id);
  if (width) widget->width = canvasWidgetExtent(*width,snap);
  if (height) widget->height = canvasWidgetExtent(*height,snap);
  if (isFreeLayoutUtility(widget->type) && widget->type != WidgetType::Dummy) widget->props.utilityCustomSize = true;
  return checkedDocumentEdit(std::move(next),project);
}

} // namespace ImGuiStudio
