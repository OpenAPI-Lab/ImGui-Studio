#pragma once

#include "imguistudio/project/studio_document_edit.hpp"
#include <string_view>

namespace ImGuiStudio {

struct AssistantProposal {
  std::vector<ImGuiNode> windows;
  std::string error;
};

namespace AssistantProposalDetail {

inline std::string_view trimFenceLine(std::string_view line) {
  const auto first = line.find_first_not_of(" \t\r");
  if (first == std::string_view::npos) return {};
  return line.substr(first, line.find_last_not_of(" \t\r") - first + 1);
}

inline bool opensProposal(std::string_view line) {
  constexpr std::string_view marker = "```imgui-json";
  return line.size() == marker.size() && std::equal(line.begin(), line.end(), marker.begin(),
    [](char source, char expected) {
      return (source >= 'A' && source <= 'Z' ? source + 'a' - 'A' : source) == expected;
    });
}

inline std::optional<std::string_view> proposalBody(std::string_view reply, bool& opened) {
  size_t body = std::string_view::npos;
  for (size_t start = 0; start < reply.size();) {
    const auto end = reply.find('\n', start);
    const auto line = trimFenceLine(reply.substr(start, end == std::string_view::npos ? end : end - start));
    if (opened) {
      if (line == "```") return reply.substr(body, start - body);
    } else if (opensProposal(line)) {
      opened = true;
      body = end == std::string_view::npos ? reply.size() : end + 1;
    }
    if (end == std::string_view::npos) break;
    start = end + 1;
  }
  return std::nullopt;
}

inline Json hydrateNode(const Json& raw, size_t depth, size_t& count) {
  ProjectJson::require(depth <= ProjectJson::kMaxDepth, "La propuesta supera el limite de profundidad");
  ProjectJson::require(++count <= ProjectJson::kMaxNodes, "La propuesta tiene demasiados widgets");
  ProjectJson::require(raw.is_object(), "Cada widget propuesto debe ser un objeto");
  const auto id = raw.at("type").get<std::string>();
  const auto type = widgetTypeFromId(id);
  ProjectJson::require(type != WidgetType::Unknown, "Tipo de widget desconocido: " + id);
  const float offset = 64.0f + 28.0f * static_cast<float>((count - 1) % 8);
  auto node = ProjectJson::nodeJson(createDefaultNode(type, offset, offset));
  for (const auto* field : {"label", "varName", "x", "y", "width", "height", "visible", "sameLine",
       "useCursorPos", "sameLineSpacing", "indent"})
    if (raw.contains(field)) node[field] = raw.at(field);
  if (raw.contains("props")) {
    ProjectJson::require(raw.at("props").is_object(), "props debe ser un objeto");
    node["props"].update(raw.at("props"));
  }
  if (raw.contains("children")) {
    ProjectJson::require(raw.at("children").is_array(), "children debe ser una lista");
    node["children"] = Json::array();
    for (const auto& child : raw.at("children"))
      node["children"].push_back(hydrateNode(child, depth + 1, count));
  }
  return node;
}

} // namespace AssistantProposalDetail

inline AssistantProposal readAssistantProposal(std::string_view reply) {
  bool opened = false;
  const auto body = AssistantProposalDetail::proposalBody(reply, opened);
  if (!body) {
    if (opened) return {{}, "El bloque imgui-json esta incompleto"};
    return {};
  }
  if (body->size() > ProjectJson::kMaxProjectBytes) return {{}, "La propuesta supera el limite de tamano del proyecto"};
  try {
    const auto content = Json::parse(body->begin(), body->end(), [](int depth, Json::parse_event_t, Json&) {
      ProjectJson::require(depth <= static_cast<int>(ProjectJson::kMaxDepth * 3 + 16),
        "La propuesta supera el limite de profundidad");
      return true;
    });
    const auto nodes = content.is_array() ? content : Json::array({content});
    ProjectJson::require(!nodes.empty(), "La propuesta no contiene widgets");
    std::vector<ImGuiNode> windows, loose;
    std::unordered_set<std::string> ids;
    std::vector<std::string> warnings;
    size_t count = 0;
    for (const auto& node : nodes) {
      auto hydrated = ProjectJson::readNode(AssistantProposalDetail::hydrateNode(node, 0, count), ids, warnings, 0);
      if (hydrated.type == WidgetType::Window) windows.push_back(std::move(hydrated));
      else loose.push_back(std::move(hydrated));
    }
    if (!loose.empty()) {
      auto wrapper = createDefaultNode(WidgetType::Window, 72, 72);
      wrapper.label = "Ventana generada";
      for (const auto& child : loose)
        ProjectJson::require(canAcceptChild(wrapper.type, child.type), "Widget suelto incompatible con una ventana");
      wrapper.children = std::move(loose);
      windows.push_back(std::move(wrapper));
    }
    return {std::move(windows), {}};
  } catch (const std::exception& error) {
    return {{}, error.what()};
  }
}

inline DocumentEditResult applyAssistantProposal(const StudioProject& project, const AssistantProposal& proposal,
    const Json& reviewedDocument) {
  if (!proposal.error.empty()) return {std::nullopt, proposal.error};
  if (proposal.windows.empty()) return {};
  if (projectToJson(project) != reviewedDocument)
    return {std::nullopt, "El proyecto cambio desde la revision de la propuesta"};
  auto next = project;
  next.nodes.insert(next.nodes.end(), proposal.windows.begin(), proposal.windows.end());
  return checkedDocumentEdit(std::move(next), project);
}

} // namespace ImGuiStudio
