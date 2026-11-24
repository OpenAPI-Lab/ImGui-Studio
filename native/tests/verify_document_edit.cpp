#include "studio_document_edit.hpp"
#include "studio_assistant_proposal.hpp"

#include <cassert>
#include <iostream>

using namespace ImGuiStudio;

int main() {
  auto project = createDefaultProject("Document edit");
  auto group = createDefaultNode(WidgetType::Group);
  auto a = createDefaultNode(WidgetType::Button), b = createDefaultNode(WidgetType::Checkbox);
  auto other = createDefaultNode(WidgetType::ChildWindow);
  group.children = {a, b};
  project.nodes[0].children = {group, other};
  auto original = projectToJson(project);
  original["futureMetadata"] = {{"keep", {1, 2, 3}}};
  original["nodes"][0]["children"][0]["children"][0]["props"]["opaque"] = {{"value", 0.123456789012345}};
  project = *projectFromJson(original).project;
  const auto widgetBefore = ProjectJson::nodeJson(*findNodeById(project.nodes, a.id));

  auto moved = moveDocumentNode(project, a.id, other.id, 0);
  assert(moved);
  assert(projectToJson(project) == original);
  assert(findNodeById(moved.project->nodes, group.id)->children.size() == 1);
  assert(ProjectJson::nodeJson(findNodeById(moved.project->nodes, other.id)->children[0]) == widgetBefore);
  assert(projectToJson(*moved.project)["futureMetadata"] == original["futureMetadata"]);
  auto back = moveDocumentNode(*moved.project, a.id, group.id, 0);
  assert(back && projectToJson(*back.project) == original);
  auto reordered = moveDocumentNode(project, a.id, group.id, 2);
  assert(reordered && findNodeById(reordered.project->nodes, group.id)->children[1].id == a.id);
  assert(!moveDocumentNode(project, a.id, group.id, 0));
  assert(moveDocumentNode(project, a.id, group.id, 0).error.empty());
  assert(!moveDocumentNode(project, a.id, group.id, 1));
  assert(!moveDocumentNode(project, group.id, group.id, 0).error.empty());
  assert(!moveDocumentNode(project, group.id, a.id, 0).error.empty());
  assert(!moveDocumentNode(project, a.id, b.id, 0).error.empty());
  assert(!moveDocumentNode(project, a.id, "missing", 0).error.empty());
  assert(!moveDocumentNode(project, "missing", other.id, 0).error.empty());
  assert(!moveDocumentNode(project, a.id, {}, 0).error.empty());
  assert(!moveDocumentNode(project, project.nodes[0].id, other.id, 0).error.empty());
  assert(!moveDocumentNode(project, a.id, other.id, 10).error.empty());
  const auto wholeGroup = moveDocumentNodes(project, {b.id, a.id}, other.id, 0);
  assert(wholeGroup);
  const auto& movedChildren = findNodeById(wholeGroup.project->nodes, other.id)->children;
  assert(movedChildren.size() == 2 && movedChildren[0].id == a.id && movedChildren[1].id == b.id);
  assert(ProjectJson::nodeJson(movedChildren[0]) == widgetBefore);
  const auto groupNoOp = moveDocumentNodes(project, {b.id, a.id}, group.id, 2);
  assert(!groupNoOp && groupNoOp.error.empty());
  assert(!moveDocumentNodes(project, {group.id, a.id}, other.id, 0).error.empty());
  assert(!moveDocumentNodes(project, {a.id, a.id}, other.id, 0).error.empty());
  assert(projectToJson(project) == original);

  auto withGap = project;
  auto& siblings = findNodeById(withGap.nodes, group.id)->children;
  const auto divider = createDefaultNode(WidgetType::Separator);
  siblings.insert(siblings.begin() + 1, divider);
  const auto block = moveDocumentNodes(withGap, {b.id, a.id}, group.id, siblings.size());
  assert(block);
  const auto& reorderedChildren = findNodeById(block.project->nodes, group.id)->children;
  assert(reorderedChildren[0].id == divider.id && reorderedChildren[1].id == a.id && reorderedChildren[2].id == b.id);

  auto deep = project;
  auto* deepest = findNodeById(deep.nodes, other.id);
  for (size_t depth = 1; depth < ProjectJson::kMaxDepth; ++depth) {
    deepest->children.push_back(createDefaultNode(WidgetType::Group));
    deepest = &deepest->children.back();
  }
  DocumentMovePlan preview;
  assert(!planDocumentMove(deep, {a.id}, deepest->id, 0, preview).empty());
  assert(!moveDocumentNodes(deep, {a.id}, deepest->id, 0));

  auto positioned = positionDocumentNode(project, a.id, 43, 71, true);
  assert(positioned);
  const auto positionedNode = findNodeById(positioned.project->nodes, a.id);
  assert(positionedNode->x == 40 && positionedNode->y == 72 && positionedNode->useCursorPos && !positionedNode->sameLine);
  auto expectedWidget = widgetBefore;
  expectedWidget["x"] = 40.0f; expectedWidget["y"] = 72.0f;
  expectedWidget["useCursorPos"] = true; expectedWidget["sameLine"] = false;
  assert(ProjectJson::nodeJson(*positionedNode) == expectedWidget);
  assert(!positionDocumentNode(project, a.id, std::numeric_limits<float>::infinity(), 0, true));
  assert(!positionDocumentNodes(project, {{a.id, 8, 16}, {"missing", 0, 0}}));
  assert(projectToJson(project) == original);

  auto tabs = createDefaultNode(WidgetType::TabBar);
  project.nodes[0].children.push_back(tabs);
  assert(!moveDocumentNode(project, a.id, tabs.id, 0).error.empty());
  assert(moveDocumentNode(project, a.id, tabs.children[0].id, 0));
  assert(!moveDocumentNode(project, tabs.children[0].id, group.id, 0).error.empty());
  assert(moveDocumentNode(project, tabs.children[0].id, tabs.id, 2));

  auto fresh = createDefaultNode(WidgetType::Text);
  assert(insertDocumentNode(project, fresh, other.id, 0));
  assert(!insertDocumentNode(project, fresh, {}, 0));
  assert(!insertDocumentNode(project, a, other.id, 0)); // Duplicate IDs are rejected atomically.
  auto window = createDefaultNode(WidgetType::Window);
  assert(insertDocumentNode(project, window, {}, project.nodes.size()));
  auto fixedBox = createDefaultNode(WidgetType::Button);
  fixedBox.width = -1; fixedBox.height = 0;
  project.nodes[0].children.push_back(fixedBox);
  const auto resized = resizeDocumentWidget(project,fixedBox.id,27,std::nullopt,true);
  assert(resized);
  const auto* result = findNodeById(resized.project->nodes,fixedBox.id);
  assert(result->width == 24 && result->height == 0 && !result->useCursorPos);
  assert(!resizeDocumentWidget(*resized.project,fixedBox.id,24,std::nullopt,true));
  assert(!resizeDocumentWidget(project,fixedBox.id,std::numeric_limits<float>::infinity(),8,true));
  assert(!resizeDocumentWidget(project,project.nodes[0].id,100,100,true));
  assert(canvasWidgetExtent(-10,true) == 8 && canvasWidgetExtent(99,false) == 99);
  assert(canvasWidgetExtent(5, true, 47) == 48 && canvasWidgetExtent(5, false, 47) == 47);
  const auto input = createDefaultNode(WidgetType::InputText);
  project.nodes[0].children.push_back(input);
  const auto widerInput = resizeDocumentWidget(project, input.id, 101, std::nullopt, true);
  assert(widerInput && findNodeById(widerInput.project->nodes, input.id)->width == 104);
  assert(findNodeById(widerInput.project->nodes, input.id)->height == input.height);
  assert(!resizeDocumentWidget(project, input.id, 101, 50, true));
  const auto reviewed = projectToJson(project);
  // Supplied IDs cannot address existing document nodes, and backticks
  // inside labels stay literal.
  const auto proposal = readAssistantProposal(
    "Propuesta de interfaz\r\n  ```ImGui-JSON\r\n"
    "[{\"type\":\"window\",\"id\":\"fake\",\"label\":\"Panel ``` local\",\"width\":370,"
    "\"children\":[{\"type\":\"slider_float\",\"id\":\"fake\",\"props\":{\"defaultNumber\":0.8}}]},"
    "{\"type\":\"text\",\"props\":{\"text\":\"Listo\"}}]\r\n ```\r\nTexto adicional");
  assert(proposal.error.empty() && proposal.windows.size() == 2);
  assert(proposal.windows[0].id != "fake" && proposal.windows[0].width == 370);
  assert(proposal.windows[0].label == "Panel ``` local");
  const auto& slider = proposal.windows[0].children.front();
  assert(slider.id != "fake" && slider.id != proposal.windows[0].id && slider.props.floatValue == 0.8f);
  assert(proposal.windows[1].children.front().props.text == "Listo");
  assert(projectToJson(project) == reviewed);
  const auto applied = applyAssistantProposal(project, proposal, reviewed);
  assert(applied && applied.project->nodes.size() == project.nodes.size() + 2);
  assert(ProjectJson::nodeJson(applied.project->nodes.front()) == ProjectJson::nodeJson(project.nodes.front()));
  assert(!applyAssistantProposal(*applied.project, proposal, reviewed));
  assert(!applyAssistantProposal(*applied.project, proposal, projectToJson(*applied.project)));
  const auto conversational = readAssistantProposal("```cpp\nint x = 0;\n```");
  assert(conversational.windows.empty() && conversational.error.empty());
  for (const auto* invalid : {
      "```imgui-json\n{\n```",
      "```imgui-json\n{\"type\":\"future-widget\"}\n```",
      "```imgui-json\n{\"type\":\"tab_bar\",\"children\":[{\"type\":\"text\"}]}\n```",
      "```imgui-json\n[{\"type\":\"window\"},{\"type\":\"button\",\"props\":[]}]\n```",
      "```imgui-json\n{\"type\":\"group\"}"}) {
    const auto rejected = readAssistantProposal(invalid);
    assert(rejected.windows.empty() && !rejected.error.empty());
    assert(!applyAssistantProposal(project, rejected, reviewed));
    assert(projectToJson(project) == reviewed);
  }
  std::cout << "[OK] Atomic move/reparent/reorder, cycles, tab constraints, insertion and opaque data preservation.\n";
}
