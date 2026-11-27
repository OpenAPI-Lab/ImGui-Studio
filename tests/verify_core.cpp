#include "imguistudio/export/studio_codegen.hpp"
#include "imguistudio/project/studio_snapshots.hpp"
#include "imguistudio/project/studio_types.hpp"

#include <cassert>
#include <iostream>

int main() {
  using namespace ImGuiStudio;

  // 1. Create default project with 1 empty window
  StudioProject proj = createDefaultProject("DemoStudioCpp");
  assert(proj.nodes.size() == 1);
  assert(proj.nodes[0].type == WidgetType::Window);

  // 2. Add widgets (MenuBar, Button, SliderFloat, Checkbox)
  ImGuiNode menu = createDefaultNode(WidgetType::MenuBar);
  menu.props.menuBarEdge = MenuBarEdge::Left;
  ImGuiNode btn = createDefaultNode(WidgetType::Button);
  btn.label = "Compilar";
  ImGuiNode slider = createDefaultNode(WidgetType::SliderFloat);
  slider.label = "Escala";

  proj.nodes[0].children.push_back(menu);
  proj.nodes[0].children.push_back(btn);
  proj.nodes[0].children.push_back(slider);

  const ProjectStats stats = countProjectStats(proj);
  assert(stats.windowCount == 1);
  assert(stats.widgetCount == 3);

  // 3. Snapshot Version History & non-destructive revert
  SnapshotManager history;
  assert(history.saveSnapshot(proj, "Base con 3 widgets", SnapshotKind::Milestone));

  // Modify design
  ImGuiNode chk = createDefaultNode(WidgetType::Checkbox);
  chk.label = "Activar sombras";
  proj.nodes[0].children.push_back(chk);
  assert(countProjectStats(proj).widgetCount == 4);

  // Revert back to the milestone snapshot
  const std::string targetId = history.snapshots().back().id;
  auto restored = history.revertToSnapshot(targetId, proj);
  assert(restored.has_value());
  assert(countProjectStats(*restored).widgetCount == 3);
  // Revert created a non-destructive backup of the 4-widget state
  assert(history.snapshots().size() == 2);
  assert(history.snapshots().front().kind == SnapshotKind::RevertBackup);

  // 4. Generate Dear ImGui C++17 code
  const std::string hpp = generateCppCode(*restored, CppEmitMode::Header);
  assert(hpp.find("ImGui::Begin(\"Ventana Principal###studio_") != std::string::npos);
  assert(hpp.find("ImGui::Button(\"Compilar\"") != std::string::npos);
  assert(hpp.find("ImGui::SliderFloat(\"Escala\"") != std::string::npos);
  assert(hpp.find("ImGui::End();") != std::string::npos);

  std::cout << "[OK] C++17 ImGui Studio core, snapshots, and codegen verified.\n";
  return 0;
}
