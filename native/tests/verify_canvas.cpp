#include "studio_canvas.hpp"
#include "studio_project_io.hpp"
#include "studio_ui.hpp"
#include "imgui_internal.h"

#include <cassert>
#include <iostream>

using namespace ImGuiStudio;

namespace {
struct EditorHarness {
  StudioApp editor;
  explicit EditorHarness(StudioProject project) : editor(false, std::move(project)) {}
  void frame() { ImGui::NewFrame(); editor.renderFrame(); ImGui::Render(); }
  PreviewItem item(const std::string& id) const {
    for (const auto& observation : editor.designReport().items)
      if (observation.nodeId == id && observation.optionIndex == -1) return observation;
    std::cerr << "Missing design item: " << id << '\n'; std::abort();
  }
  PreviewItem target(const std::string& id) const {
    for (const auto& observation : editor.editTargets()) if (observation.nodeId == id) return observation;
    std::cerr << "Missing editor target: " << id << '\n'; std::abort();
  }
  static ImVec2 center(const PreviewItem& item) {
    return {(item.minimum.x + item.maximum.x) / 2, (item.minimum.y + item.maximum.y) / 2};
  }
  void move(ImVec2 point) { ImGui::GetIO().AddMousePosEvent(point.x, point.y); frame(); }
  void click(const PreviewItem& item, bool ctrl = false) {
    move(center(item));
    if (ctrl) ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, true);
    ImGui::GetIO().AddMouseButtonEvent(0, true); frame();
    if (ctrl) ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, false);
    ImGui::GetIO().AddMouseButtonEvent(0, false); frame();
  }
  void clickAt(ImVec2 point) {
    move(point); ImGui::GetIO().AddMouseButtonEvent(0, true); frame();
    ImGui::GetIO().AddMouseButtonEvent(0, false); frame();
  }
  void beginDrag(const PreviewItem& item, bool alt = false) {
    move(center(item));
    if (alt) ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, true);
    ImGui::GetIO().AddMouseButtonEvent(0, true); frame();
  }
  void release() { ImGui::GetIO().AddMouseButtonEvent(0, false); frame(); frame(); }
  void middle(bool down) { ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Middle, down); frame(); }
  void undo(bool redo = false) {
    auto& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(redo ? ImGuiKey_Y : ImGuiKey_Z, true); frame();
    io.AddKeyEvent(redo ? ImGuiKey_Y : ImGuiKey_Z, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); frame();
  }
};
}

int main() {
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();
  io.IniFilename = nullptr; io.DisplaySize = ImVec2(1600, 1100); io.DeltaTime = 1.0f / 60;
  unsigned char* pixels; int w, h; io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

  auto project = createDefaultProject("Canvas gestures");
  project.nodes[0].width = 650; project.nodes[0].height = 600;
  auto parent = createDefaultNode(WidgetType::Group);
  auto check = createDefaultNode(WidgetType::Checkbox); check.props.checked = false;
  auto button = createDefaultNode(WidgetType::Button); button.width = 100; button.height = 28;
  parent.children = {check, button};
  auto target = createDefaultNode(WidgetType::ChildWindow); target.width = 350; target.height = 160;
  auto input = createDefaultNode(WidgetType::InputText);
  input.label = "Input with a label outside its frame"; input.width = 0; input.height = 37;
  auto number = createDefaultNode(WidgetType::InputInt);
  number.label = "Number"; number.width = 145;
  auto combo = createDefaultNode(WidgetType::Combo);
  combo.label = "Options"; combo.width = 124;
  auto multiline = createDefaultNode(WidgetType::InputMultiline);
  multiline.label = "Multiline input"; multiline.width = 0; multiline.height = 48;
  auto list = createDefaultNode(WidgetType::Listbox);
  list.label = "List with a label"; list.width = 128; list.height = 48;
  auto lines = createDefaultNode(WidgetType::PlotLines);
  lines.label = "Line graph"; lines.width = 128; lines.height = 40;
  auto histogram = createDefaultNode(WidgetType::PlotHistogram);
  histogram.label = "Histogram"; histogram.width = 128; histogram.height = 40;
  project.nodes[0].children = {parent, target, input, number, combo, multiline, list, lines, histogram};
  auto distant = createDefaultNode(WidgetType::Group); distant.visible = false;
  const auto nested = createDefaultNode(WidgetType::Text);
  distant.children.push_back(nested);
  for (int i = 0; i < 64; ++i) {
    if (i == 56) project.nodes[0].children.push_back(distant);
    auto filler = createDefaultNode(WidgetType::Text);
    filler.visible = false;
    project.nodes[0].children.push_back(std::move(filler));
  }
  auto farWindow = createDefaultNode(WidgetType::Window, 1220, 90);
  farWindow.width = 420; farWindow.height = 460;
  auto farTarget = createDefaultNode(WidgetType::ChildWindow);
  farTarget.width = 280; farTarget.height = 144;
  auto farButton = createDefaultNode(WidgetType::Button);
  farButton.width = 120; farButton.height = 28;
  farWindow.children = {farTarget, farButton};
  project.nodes.push_back(farWindow);
  EditorHarness app(project);
  app.frame(); app.frame();
  const auto original = projectToJson(app.editor.project());
  app.click(app.item(check.id));
  assert(app.editor.selectedNodeId() == check.id);
  assert(!findNodeById(app.editor.project().nodes, check.id)->props.checked);
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);

  // Changing the view moves the canvas windows on screen without editing them.
  const auto rootId = project.nodes.front().id;
  const auto unpanned = app.item(rootId).minimum;
  const auto panStart = EditorHarness::center(app.item(check.id));
  app.move(panStart); app.middle(true);
  app.move(ImVec2(panStart.x + 64, panStart.y + 32));
  assert(ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeAll);
  app.middle(false);
  const auto panned = app.item(rootId).minimum;
  if (panned.x != unpanned.x + 64 || panned.y != unpanned.y + 32)
    std::cerr << "Canvas position before pan (" << unpanned.x << ", " << unpanned.y
              << "), after (" << panned.x << ", " << panned.y << ")\n";
  assert(panned.x == unpanned.x + 64 && panned.y == unpanned.y + 32);
  assert(app.editor.selectedNodeId() == check.id);
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  app.click(app.item(button.id));
  assert(app.editor.selectedNodeId() == button.id);

  // Moving a root by its title after panning still stores project coordinates.
  const ImVec2 title(panned.x + 120, panned.y + 7);
  app.move(title);
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true); app.frame();
  app.move(ImVec2(title.x + 32, title.y + 24));
  app.release();
  const auto& movedRoot = app.editor.project().nodes.front();
  const auto& originalRoot = project.nodes.front();
  assert(movedRoot.x == std::round((originalRoot.x + 32) / 8) * 8);
  assert(movedRoot.y == std::round((originalRoot.y + 24) / 8) * 8);
  assert(app.editor.undoCount() == 1);
  app.undo();
  assert(projectToJson(app.editor.project()) == original && app.item(rootId).minimum.x == panned.x);

  app.move(title); app.middle(true);
  app.move(ImVec2(title.x - 250, title.y - 12));
  assert(app.item(rootId).minimum.x < panned.x);
  io.AddKeyEvent(ImGuiKey_Escape, true); app.frame();
  io.AddKeyEvent(ImGuiKey_Escape, false); app.middle(false);
  assert(app.item(rootId).minimum.x == panned.x && app.item(rootId).minimum.y == panned.y);

  // Scroll the design partly out of the view and check actual draw commands
  // and interaction with the sidebar behind the clipped portion of the root.
  app.move(title); app.middle(true);
  app.move(ImVec2(title.x - 300, title.y));
  app.middle(false);
  const auto clipped = app.item(rootId);
  const auto background = app.target("__studio_canvas__");
  assert(clipped.minimum.x < background.minimum.x && clipped.clipMinimum.x >= background.minimum.x);
  for (const auto& command : clipped.drawList->CmdBuffer) {
    if (command.ElemCount == 0) continue;
    assert(command.ClipRect.x >= background.minimum.x && command.ClipRect.y >= background.minimum.y);
    assert(command.ClipRect.z <= background.maximum.x && command.ClipRect.w <= background.maximum.y);
  }
  assert(app.target(rootId).hierarchy);
  const auto* hierarchy = ImGui::FindWindowByName("##Hierarchy");
  const auto* palette = ImGui::FindWindowByName("##WidgetPalette");
  const auto* code = ImGui::FindWindowByName("##CodeDrawer");
  assert(hierarchy && palette && code);
  assert(hierarchy->Pos.y == palette->Pos.y + palette->Size.y && hierarchy->Pos.y == code->Pos.y);
  {
    auto& platform = ImGui::GetPlatformIO();
    const auto originalSetter = platform.Platform_SetClipboardTextFn;
    auto* originalData = platform.Platform_ClipboardUserData;
    std::string copied;
    platform.Platform_ClipboardUserData = &copied;
    platform.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text) {
      *static_cast<std::string*>(ImGui::GetPlatformIO().Platform_ClipboardUserData) = text;
    };
    ImGui::ActivateItemByID(ImHashStr("###CopyCode", 0, code->ID));
    app.frame(); app.frame();
    platform.Platform_SetClipboardTextFn = originalSetter;
    platform.Platform_ClipboardUserData = originalData;
    assert(copied.find("ImGui::Begin(") != std::string::npos);
  }
  app.move(EditorHarness::center(background));
  io.AddKeyEvent(ImGuiKey_Home, true); app.frame();
  io.AddKeyEvent(ImGuiKey_Home, false); app.frame();
  assert(app.item(rootId).minimum.x == unpanned.x && app.item(rootId).minimum.y == unpanned.y);
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);

  const auto initial = app.item(button.id);
  app.beginDrag(initial, true);
  const auto from = EditorHarness::center(initial);
  app.move(ImVec2(from.x + 31, from.y + 15));
  app.move(ImVec2(from.x + 47, from.y + 31));
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  app.release();
  io.AddKeyEvent(ImGuiMod_Alt, false); app.frame();
  const auto moved = findNodeById(app.editor.project().nodes, button.id);
  assert(moved->useCursorPos && !moved->sameLine);
  assert(moved->x == std::round((initial.minimum.x - initial.coordinateOrigin.x + 47) / 8) * 8);
  assert(moved->y == std::round((initial.minimum.y - initial.coordinateOrigin.y + 31) / 8) * 8);
  assert(app.editor.undoCount() == 1);
  const auto positioned = projectToJson(app.editor.project());
  app.undo(); assert(projectToJson(app.editor.project()) == original);
  app.undo(true); assert(projectToJson(app.editor.project()) == positioned);
  app.undo();

  // A flow widget drops into a nested child container, with one undo transaction.
  app.beginDrag(app.item(check.id));
  const auto childTarget = app.item(target.id);
  app.move(EditorHarness::center(childTarget));
  app.release();
  const auto location = locateNode(app.editor.project().nodes, check.id);
  assert(location && location->parentId == target.id);
  assert(app.editor.undoCount() == 1);
  app.undo(); assert(projectToJson(app.editor.project()) == original);

  // Escape cancels an active gesture without changing project/history.
  app.beginDrag(app.item(button.id), true);
  app.move(ImVec2(from.x + 80, from.y + 20));
  io.AddKeyEvent(ImGuiKey_Escape, true); app.frame();
  io.AddKeyEvent(ImGuiKey_Escape, false); app.release();
  io.AddKeyEvent(ImGuiMod_Alt, false); app.frame();
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);

  // Losing mouse tracking cancels rather than sending a widget off screen.
  app.beginDrag(app.item(button.id), true);
  app.move(ImVec2(from.x + 20, from.y + 20));
  io.AddMousePosEvent(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()); app.frame();
  app.release(); io.AddKeyEvent(ImGuiMod_Alt, false); app.move(from);
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);

  // Drag from the real palette into a child; one inserted widget, not a double click-add.
  app.beginDrag(app.target("palette/text"));
  app.move(EditorHarness::center(app.item(target.id))); app.release();
  const auto insertedParent = findNodeById(app.editor.project().nodes, target.id);
  assert(insertedParent->children.size() == 1 && insertedParent->children[0].type == WidgetType::Text);
  assert(app.editor.undoCount() == 1);
  app.undo(); assert(projectToJson(app.editor.project()) == original);

  // Hierarchy uses the same drop rules as canvas, with real row gestures.
  const auto groupRow = app.target(parent.id);
  app.clickAt(ImVec2(groupRow.minimum.x + 5, (groupRow.minimum.y + groupRow.maximum.y) / 2));
  app.frame();
  app.beginDrag(app.target(check.id));
  const auto buttonRow = app.target(button.id);
  app.move(ImVec2((buttonRow.minimum.x + buttonRow.maximum.x) / 2, buttonRow.maximum.y - 1));
  app.release();
  assert(findNodeById(app.editor.project().nodes, parent.id)->children[1].id == check.id);
  assert(app.editor.undoCount() == 1);
  app.undo(); assert(projectToJson(app.editor.project()) == original);

  // Mix hierarchy/canvas selection, toggle the primary and preserve the group
  // when dragging any member. The destination keeps the original sibling order.
  app.click(app.item(button.id));
  app.click(app.target(check.id), true);
  const std::vector<std::string> selected{button.id, check.id};
  assert(app.editor.selectedNodeIds() == selected && app.editor.selectedNodeId() == check.id);
  app.click(app.item(check.id), true);
  assert(app.editor.selectedNodeIds() == std::vector<std::string>{button.id});
  app.click(app.item(check.id), true);
  app.beginDrag(app.target(button.id));
  app.move(EditorHarness::center(app.item(target.id)));
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  app.release();
  const auto* destination = findNodeById(app.editor.project().nodes, target.id);
  assert(destination->children.size() == 2);
  assert(destination->children[0].id == check.id && destination->children[1].id == button.id);
  assert(app.editor.selectedNodeIds() == selected && app.editor.undoCount() == 1);
  const auto grouped = projectToJson(app.editor.project());
  app.undo(); assert(projectToJson(app.editor.project()) == original);
  app.undo(true); assert(projectToJson(app.editor.project()) == grouped);
  app.undo();

  // Snap the drag anchor and translate both boxes by the same amount.
  const auto checkBox = app.item(check.id), buttonBox = app.item(button.id);
  const auto anchor = EditorHarness::center(buttonBox);
  app.beginDrag(buttonBox, true);
  app.move(ImVec2(anchor.x + 43, anchor.y + 37)); app.release();
  io.AddKeyEvent(ImGuiMod_Alt, false); app.frame();
  const auto* groupCheck = findNodeById(app.editor.project().nodes, check.id);
  const auto* groupButton = findNodeById(app.editor.project().nodes, button.id);
  assert(groupCheck->useCursorPos && groupButton->useCursorPos);
  assert(groupButton->x == std::round((buttonBox.minimum.x - buttonBox.coordinateOrigin.x + 43) / 8) * 8);
  assert(groupButton->y == std::round((buttonBox.minimum.y - buttonBox.coordinateOrigin.y + 37) / 8) * 8);
  assert(groupCheck->x - groupButton->x == (checkBox.minimum.x - checkBox.coordinateOrigin.x) -
    (buttonBox.minimum.x - buttonBox.coordinateOrigin.x));
  assert(groupCheck->y - groupButton->y == (checkBox.minimum.y - checkBox.coordinateOrigin.y) -
    (buttonBox.minimum.y - buttonBox.coordinateOrigin.y));
  const auto movedCheckBox = app.item(check.id), movedButtonBox = app.item(button.id);
  assert(movedCheckBox.minimum.x - movedButtonBox.minimum.x == checkBox.minimum.x - buttonBox.minimum.x);
  assert(movedCheckBox.minimum.y - movedButtonBox.minimum.y == checkBox.minimum.y - buttonBox.minimum.y);
  assert(app.editor.undoCount() == 1);
  app.undo(); assert(projectToJson(app.editor.project()) == original);

  app.beginDrag(app.item(button.id));
  app.move(EditorHarness::center(app.item(target.id)));
  io.AddKeyEvent(ImGuiKey_Escape, true); app.frame();
  io.AddKeyEvent(ImGuiKey_Escape, false); app.release();
  assert(app.editor.selectedNodeIds() == selected);
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);

  app.click(app.item(target.id), true);
  app.beginDrag(app.item(button.id), true);
  app.move(ImVec2(anchor.x + 40, anchor.y + 40)); app.release();
  io.AddKeyEvent(ImGuiMod_Alt, false); app.frame();
  assert(app.editor.selectedNodeIds().size() == 3);
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  app.click(app.item(button.id));
  assert(app.editor.selectedNodeIds() == std::vector<std::string>{button.id});

  // Clicking the canvas background clears selection without changing the document.
  app.clickAt(ImVec2(1100, 700));
  assert(app.editor.selectedNodeId().empty() && projectToJson(app.editor.project()) == original);

  // Undo during another gesture must not be overwritten by the gesture's old baseline.
  app.beginDrag(app.item(button.id), true);
  auto buttonPoint = EditorHarness::center(app.item(button.id));
  app.move(ImVec2(buttonPoint.x + 32, buttonPoint.y + 32)); app.release();
  io.AddKeyEvent(ImGuiMod_Alt, false); app.frame();
  app.beginDrag(app.item(button.id), true);
  buttonPoint = EditorHarness::center(app.item(button.id));
  app.move(ImVec2(buttonPoint.x + 40, buttonPoint.y + 40));
  app.undo(); app.release();
  io.AddKeyEvent(ImGuiMod_Alt, false); app.frame();
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);

  // Select a button, then drag the resize handle on its box. Mouse motion only
  // draws the guide. Releasing commits one undoable size edit, not a move.
  app.click(app.item(button.id));
  assert(app.editor.selectedNodeId() == button.id);
  auto observed = app.item(button.id);
  auto handle = ImVec2(observed.maximum.x-3,observed.maximum.y-3);
  app.move(handle);
  assert(ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeNWSE);
  io.AddMouseButtonEvent(0,true); app.frame();
  app.move(ImVec2(handle.x+37,handle.y+19));
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  app.release();
  const auto resized = findNodeById(app.editor.project().nodes,button.id);
  assert(resized->width == 136 && resized->height == 48);
  assert(resized->x == button.x && resized->y == button.y && resized->useCursorPos == button.useCursorPos);
  assert(app.editor.selectedNodeId() == button.id && app.editor.undoCount() == 1);
  assert(app.item(button.id).maximum.x-app.item(button.id).minimum.x == 136);
  const auto resizedJson = projectToJson(app.editor.project());
  app.undo(); assert(projectToJson(app.editor.project()) == original);
  app.undo(true); assert(projectToJson(app.editor.project()) == resizedJson);
  app.undo();

  // The handle also supports one-axis motion and Escape, preserving the other axis.
  observed = app.item(button.id);
  handle = ImVec2(observed.maximum.x-3,observed.maximum.y-3);
  app.move(handle); io.AddMouseButtonEvent(0,true); app.frame();
  app.move(ImVec2(handle.x+19,handle.y));
  app.release();
  const auto wider = findNodeById(app.editor.project().nodes,button.id);
  assert(wider->width == 120 && wider->height == button.height);
  app.undo();
  observed = app.item(button.id);
  handle = ImVec2(observed.maximum.x-3,observed.maximum.y-3);
  app.move(handle); io.AddMouseButtonEvent(0,true); app.frame();
  app.move(ImVec2(handle.x-60,handle.y-20));
  io.AddKeyEvent(ImGuiKey_Escape,true); app.frame();
  io.AddKeyEvent(ImGuiKey_Escape,false); app.release();
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);

  app.click(app.item(target.id));
  assert(app.editor.selectedNodeId() == target.id);
  observed = app.item(target.id);
  handle = ImVec2(observed.maximum.x-3,observed.maximum.y-3);
  app.move(handle); io.AddMouseButtonEvent(0,true); app.frame();
  app.move(ImVec2(handle.x+25,handle.y+24)); app.release();
  const auto* resizedChild = findNodeById(app.editor.project().nodes,target.id);
  assert(resizedChild->width == 376 && resizedChild->height == 184);
  assert(app.editor.undoCount() == 1);
  app.undo();
  assert(projectToJson(app.editor.project()) == original);

  // A horizontal handle belongs to the input frame, before its label. A
  // vertical drag adds no edit, and a diagonal drag touches only the width.
  auto widthHandle = [&](const std::string& id) {
    const auto field = app.item(id);
    assert(field.resizeWidth);
    const ImVec2 point(field.minimum.x + field.resizeWidth->current - 3, EditorHarness::center(field).y);
    assert(point.x < field.maximum.x);
    app.move(point);
    assert(ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeEW);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true); app.frame();
    return point;
  };
  app.click(app.item(input.id));
  const auto field = app.item(input.id);
  assert(field.resizeWidth && field.maximum.x - field.minimum.x > field.resizeWidth->current);
  auto widthPoint = widthHandle(input.id);
  app.move(ImVec2(widthPoint.x, widthPoint.y + 36)); app.release();
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  widthPoint = widthHandle(input.id);
  app.move(ImVec2(widthPoint.x - 39, widthPoint.y + 44));
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  app.release();
  const float expectedWidth = std::round((field.resizeWidth->current - 39) / 8) * 8;
  const auto* narrower = findNodeById(app.editor.project().nodes, input.id);
  assert(narrower->width == expectedWidth && narrower->height == input.height);
  assert(narrower->props.text == input.props.text);
  assert(app.item(input.id).resizeWidth->current == expectedWidth && app.editor.undoCount() == 1);
  app.undo();
  assert(projectToJson(app.editor.project()) == original);

  // Keep numeric stepper buttons inside the measured width at the minimum.
  app.click(app.item(number.id));
  widthPoint = widthHandle(number.id);
  const float minimumWidth = app.item(number.id).resizeWidth->minimum;
  assert(minimumWidth > 8);
  app.move(ImVec2(widthPoint.x - 135, widthPoint.y)); app.release();
  const float snappedMinimum = std::ceil(minimumWidth / 8) * 8;
  assert(findNodeById(app.editor.project().nodes, number.id)->width == snappedMinimum);
  assert(app.item(number.id).resizeWidth->current == snappedMinimum && app.editor.undoCount() == 1);
  app.undo();
  assert(projectToJson(app.editor.project()) == original);

  app.click(app.item(combo.id)); widthPoint = widthHandle(combo.id);
  app.move(ImVec2(widthPoint.x + 42, widthPoint.y)); app.release();
  assert(findNodeById(app.editor.project().nodes, combo.id)->width == 168);
  assert(app.item(combo.id).resizeWidth->current == 168 && app.editor.undoCount() == 1);
  app.undo();
  assert(projectToJson(app.editor.project()) == original);

  // Labeled controls with two editable dimensions use the frame's corner.
  for (const auto& node : {multiline, list, lines, histogram}) {
    const auto initialItem = app.item(node.id);
    assert(initialItem.resizeWidth && initialItem.maximum.x > initialItem.minimum.x + initialItem.resizeWidth->current);
    app.click(app.item(node.id));
    const ImVec2 handle(initialItem.minimum.x + initialItem.resizeWidth->current - 3, initialItem.maximum.y - 3);
    app.move(handle);
    assert(ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeNWSE);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true); app.frame();
    const float widthDelta = node.width == 0 ? 0.0f : 24.0f;
    app.move(ImVec2(handle.x + widthDelta, handle.y + 16));
    assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
    app.release();
    const auto* resized = findNodeById(app.editor.project().nodes, node.id);
    assert(resized->width == node.width + widthDelta && resized->height == node.height + 16);
    assert(app.editor.undoCount() == 1);
    app.undo();
    assert(projectToJson(app.editor.project()) == original);
  }

  // Hold a real drag at the bottom of the hierarchy until an offscreen
  // destination becomes reachable. Its header must stay in place.
  io.DisplaySize.y = 720;
  io.DeltaTime = 1.0f / 30;
  app.frame(); app.frame();
  const auto hierarchyY = hierarchy->Pos.y;
  const auto sourceRow = app.target(check.id);
  const auto offscreen = app.target(distant.id);
  assert(offscreen.minimum.y > offscreen.clipMaximum.y && offscreen.hierarchyCollapsed);
  const float rowX = EditorHarness::center(sourceRow).x;
  const ImVec2 bottom(rowX, sourceRow.clipMaximum.y - 5);
  app.move(bottom);
  for (int i = 0; i < 5; ++i) app.frame();
  assert(app.target(distant.id).minimum.y == offscreen.minimum.y);
  app.beginDrag(sourceRow);
  app.move(bottom);
  for (int i = 0; i < 100 && app.target(distant.id).maximum.y > bottom.y - 65; ++i) app.frame();
  app.move(ImVec2(rowX, (sourceRow.clipMinimum.y + sourceRow.clipMaximum.y) / 2));
  app.frame();
  const auto revealed = app.target(distant.id);
  assert(revealed.minimum.y >= revealed.clipMinimum.y && revealed.maximum.y <= revealed.clipMaximum.y);
  assert(hierarchy->Pos.y == hierarchyY);

  const ImVec2 center = EditorHarness::center(revealed);
  app.move(center);
  for (int i = 0; i < 5; ++i) app.frame();
  assert(app.target(distant.id).hierarchyCollapsed);
  for (int i = 0; i < 24; ++i) app.frame();
  assert(!app.target(distant.id).hierarchyCollapsed);
  assert(app.target(nested.id).parentId == distant.id);
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  app.release();
  assert(locateNode(app.editor.project().nodes, check.id)->parentId == distant.id);
  assert(app.editor.undoCount() == 1);
  app.undo(); assert(projectToJson(app.editor.project()) == original);

  // A canvas drag can also scroll upward. Escape stops navigation and leaves
  // the document and undo stack alone even while the mouse button is held.
  const ImVec2 top(rowX, revealed.clipMinimum.y + 5);
  app.beginDrag(app.item(check.id));
  app.move(top);
  for (int i = 0; i < 10; ++i) app.frame();
  assert(app.target(distant.id).minimum.y > revealed.minimum.y);
  io.AddKeyEvent(ImGuiKey_Escape, true); app.frame();
  const float canceledY = app.target(distant.id).minimum.y;
  io.AddKeyEvent(ImGuiKey_Escape, false);
  for (int i = 0; i < 12; ++i) app.frame();
  assert(app.target(distant.id).minimum.y == canceledY);
  app.release();
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);

  // Pan during an active layout drag until a second design window is reachable.
  const auto viewport = app.target("__studio_canvas__");
  const auto remote = app.item(farTarget.id);
  assert(remote.minimum.x >= viewport.maximum.x);
  const ImVec2 right(viewport.maximum.x - 5, EditorHarness::center(remote).y);
  app.move(right);
  for (int i = 0; i < 4; ++i) app.frame();
  assert(app.item(farTarget.id).minimum.x == remote.minimum.x);
  app.beginDrag(app.item(check.id));
  app.move(right);
  for (int i = 0; i < 50 && EditorHarness::center(app.item(farTarget.id)).x > right.x - 70; ++i) app.frame();
  const auto exposed = app.item(farTarget.id);
  assert(EditorHarness::center(exposed).x < right.x - 60);
  app.move(EditorHarness::center(exposed)); app.frame();
  assert(app.item(farTarget.id).minimum.x == exposed.minimum.x);
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  app.release();
  assert(locateNode(app.editor.project().nodes, check.id)->parentId == farTarget.id);
  assert(app.editor.undoCount() == 1);
  app.undo();
  assert(projectToJson(app.editor.project()) == original && app.item(farTarget.id).minimum.x == exposed.minimum.x);

  // In free positioning the view moves right under the pointer, and the edit
  // still uses the target's local coordinates with the normal grid snap.
  const auto freeStart = app.item(farButton.id);
  const auto heldAt = EditorHarness::center(freeStart);
  const ImVec2 freeRight(right.x, heldAt.y);
  const float remoteWindowX = app.item(farWindow.id).minimum.x;
  app.beginDrag(freeStart, true);
  app.move(freeRight);
  for (int i = 0; i < 5; ++i) app.frame();
  const float cameraShift = app.item(farWindow.id).minimum.x - remoteWindowX;
  assert(cameraShift < 0 && projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
  app.release();
  io.AddKeyEvent(ImGuiMod_Alt, false); app.frame();
  const auto* placed = findNodeById(app.editor.project().nodes, farButton.id);
  const float snapped = std::round((freeStart.minimum.x - freeStart.coordinateOrigin.x +
    freeRight.x - heldAt.x - cameraShift) / 8) * 8;
  assert(placed->useCursorPos && placed->x == snapped);
  assert(std::abs(app.item(farButton.id).minimum.x - (freeStart.minimum.x + freeRight.x - heldAt.x)) <= 4);
  assert(app.editor.undoCount() == 1);
  app.undo();
  assert(projectToJson(app.editor.project()) == original);

  // Escape stops edge panning even while the left button stays held.
  app.beginDrag(app.item(farButton.id));
  app.move(freeRight); app.frame(); app.frame();
  io.AddKeyEvent(ImGuiKey_Escape, true); app.frame();
  const float stoppedX = app.item(farWindow.id).minimum.x;
  io.AddKeyEvent(ImGuiKey_Escape, false);
  for (int i = 0; i < 5; ++i) app.frame();
  assert(app.item(farWindow.id).minimum.x == stoppedX);
  app.release();
  assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);

  // Pure hit testing rejects clipped/occluded widgets and prefers nested leaves.
  PreviewItem outer; outer.nodeId = "outer"; outer.depth = 0;
  outer.minimum = outer.clipMinimum = ImVec2(0, 0); outer.maximum = outer.clipMaximum = ImVec2(200, 200);
  outer.windowHovered = true;
  auto inner = outer; inner.nodeId = "inner"; inner.depth = 1; inner.minimum = ImVec2(20, 20);
  inner.maximum = ImVec2(100, 100); inner.clipMinimum = ImVec2(40, 40);
  std::vector<PreviewItem> hitItems{outer, inner};
  assert(NativeCanvas::hitTest(hitItems, ImVec2(50, 50))->nodeId == "inner");
  assert(NativeCanvas::hitTest(hitItems, ImVec2(30, 30))->nodeId == "outer");
  hitItems[1].windowHovered = false;
  assert(NativeCanvas::hitTest(hitItems, ImVec2(50, 50))->nodeId == "outer");

  // A flow row uses horizontal insertion even at the lower edge of a widget.
  auto flowProject = createDefaultProject("Flow editing");
  auto flow = createDefaultNode(WidgetType::PositionerFlow);
  flow.width = 370; flow.height = 160;
  auto firstFlowButton = createDefaultNode(WidgetType::Button);
  auto middleFlowButton = createDefaultNode(WidgetType::Button);
  auto lastFlowButton = createDefaultNode(WidgetType::Button);
  firstFlowButton.width = middleFlowButton.width = lastFlowButton.width = 96;
  flow.children = {firstFlowButton, middleFlowButton, lastFlowButton};
  auto gridView = createDefaultNode(WidgetType::GridView);
  gridView.width = 224; gridView.height = 104;
  flowProject.nodes.front().children = {flow, gridView};
  EditorHarness flowApp(flowProject);
  flowApp.frame(); flowApp.frame(); flowApp.frame();
  const auto firstFlowItem = flowApp.item(firstFlowButton.id);
  assert(flowApp.item(lastFlowButton.id).minimum.y == firstFlowItem.minimum.y);
  flowApp.beginDrag(flowApp.item(lastFlowButton.id));
  flowApp.move(ImVec2(firstFlowItem.minimum.x + 3, firstFlowItem.maximum.y - 3));
  assert(flowApp.editor.undoCount() == 0);
  flowApp.release();
  const auto& reordered = flowApp.editor.project().nodes.front().children.front().children;
  assert(reordered.front().id == lastFlowButton.id && reordered[1].id == firstFlowButton.id);
  assert(flowApp.editor.undoCount() == 1);
  flowApp.undo();
  assert(projectToJson(flowApp.editor.project()) == projectToJson(flowProject));

  // Model options are selectable in preview, but editing targets the whole view.
  flowApp.click(flowApp.item(gridView.id));
  assert(flowApp.editor.selectedNodeId() == gridView.id && flowApp.editor.undoCount() == 0);
  const auto gridItem = flowApp.item(gridView.id);
  const ImVec2 gridHandle(gridItem.maximum.x - 3, gridItem.maximum.y - 3);
  flowApp.move(gridHandle);
  assert(ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeNWSE);
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true); flowApp.frame();
  flowApp.move(ImVec2(gridHandle.x + 24, gridHandle.y + 16));
  flowApp.release();
  const auto* resizedGrid = findNodeById(flowApp.editor.project().nodes, gridView.id);
  assert(resizedGrid && resizedGrid->width == 248 && resizedGrid->height == 120);
  assert(resizedGrid->props.items == gridView.props.items && resizedGrid->props.selectedIndex == gridView.props.selectedIndex);
  assert(flowApp.editor.undoCount() == 1);
  flowApp.undo();
  assert(projectToJson(flowApp.editor.project()) == projectToJson(flowProject));

  // Ctrl+wheel changes the design scale around the pointer. Gestures still
  // commit grid-aligned document coordinates, and the view has no undo entry.
  auto zoomProject = createDefaultProject("Zoom and gestures");
  auto& zoomWindow = zoomProject.nodes.front();
  zoomWindow.x = 128; zoomWindow.y = 96; zoomWindow.width = 400; zoomWindow.height = 360;
  auto zoomButton = createDefaultNode(WidgetType::Button, 48, 88);
  zoomButton.useCursorPos = true; zoomButton.width = 96; zoomButton.height = 32;
  zoomWindow.children = {zoomButton};
  EditorHarness zoomApp(zoomProject);
  zoomApp.frame(); zoomApp.frame();
  const auto baseItem = zoomApp.item(zoomButton.id);
  const auto baseWindow = zoomApp.item(zoomWindow.id);
  const auto zoomAnchor = EditorHarness::center(baseItem);
  const auto wheel = [&](float amount) {
    io.AddKeyEvent(ImGuiMod_Ctrl, true); zoomApp.frame();
    io.AddMouseWheelEvent(0, amount); zoomApp.frame();
    io.AddKeyEvent(ImGuiMod_Ctrl, false); zoomApp.frame();
  };
  zoomApp.move(zoomAnchor);
  wheel(8);
  const auto zoomedItem = zoomApp.item(zoomButton.id);
  const auto zoomedCenter = EditorHarness::center(zoomedItem);
  assert(zoomApp.item(zoomWindow.id).maximum.x - zoomApp.item(zoomWindow.id).minimum.x == 600);
  assert(zoomedItem.maximum.x - zoomedItem.minimum.x == 144);
  assert(std::abs(zoomedCenter.x - zoomAnchor.x) <= 1 && std::abs(zoomedCenter.y - zoomAnchor.y) <= 1);
  assert(projectToJson(zoomApp.editor.project()) == projectToJson(zoomProject) && zoomApp.editor.undoCount() == 0);
  const auto zoomedWindow = zoomApp.item(zoomWindow.id);
  zoomApp.middle(true);
  zoomApp.move(ImVec2(zoomAnchor.x + 20, zoomAnchor.y - 12));
  zoomApp.middle(false);
  assert(zoomApp.item(zoomWindow.id).minimum.x == zoomedWindow.minimum.x + 20);
  assert(zoomApp.item(zoomWindow.id).minimum.y == zoomedWindow.minimum.y - 12);

  zoomApp.beginDrag(zoomApp.item(zoomButton.id));
  const auto dragStart = io.MousePos;
  zoomApp.move(ImVec2(dragStart.x + 24, dragStart.y + 24));
  zoomApp.release();
  const auto* movedAtZoom = findNodeById(zoomApp.editor.project().nodes, zoomButton.id);
  assert(movedAtZoom->x == 64 && movedAtZoom->y == 104 && zoomApp.editor.undoCount() == 1);
  const auto movedItem = zoomApp.item(zoomButton.id);
  const ImVec2 zoomHandle(movedItem.maximum.x - 3, movedItem.maximum.y - 3);
  zoomApp.move(zoomHandle); io.AddMouseButtonEvent(0, true); zoomApp.frame();
  zoomApp.move(ImVec2(zoomHandle.x + 36, zoomHandle.y + 24)); zoomApp.release();
  const auto* resizedAtZoom = findNodeById(zoomApp.editor.project().nodes, zoomButton.id);
  assert(resizedAtZoom->width == 120 && resizedAtZoom->height == 48 && zoomApp.editor.undoCount() == 2);
  zoomApp.undo(); zoomApp.undo();
  assert(projectToJson(zoomApp.editor.project()) == projectToJson(zoomProject));

  const auto windowBeforeMove = zoomApp.item(zoomWindow.id);
  const ImVec2 zoomTitle(windowBeforeMove.minimum.x + 150, windowBeforeMove.minimum.y + 8);
  zoomApp.move(zoomTitle);
  io.AddMouseButtonEvent(0, true); zoomApp.frame();
  zoomApp.move(ImVec2(zoomTitle.x + 24, zoomTitle.y + 24)); zoomApp.release();
  const auto& zoomMovedRoot = zoomApp.editor.project().nodes.front();
  assert(zoomMovedRoot.x == 144 && zoomMovedRoot.y == 112);
  assert(zoomMovedRoot.width == zoomWindow.width && zoomMovedRoot.height == zoomWindow.height);
  zoomApp.undo();
  assert(projectToJson(zoomApp.editor.project()) == projectToJson(zoomProject));

  const auto zoomViewport = zoomApp.target("__studio_canvas__");
  const ImVec2 newWindowDrop(zoomViewport.maximum.x - 128, zoomViewport.maximum.y - 128);
  zoomApp.beginDrag(zoomApp.target("palette/window"));
  zoomApp.move(newWindowDrop); zoomApp.release();
  assert(zoomApp.editor.project().nodes.size() == 2);
  const auto& insertedWindow = zoomApp.editor.project().nodes.back();
  const float dropX = zoomWindow.x + (newWindowDrop.x - windowBeforeMove.minimum.x) / 1.5f;
  const float dropY = zoomWindow.y + (newWindowDrop.y - windowBeforeMove.minimum.y) / 1.5f;
  assert(insertedWindow.x == std::round(dropX / 8) * 8 && insertedWindow.y == std::round(dropY / 8) * 8);
  zoomApp.undo();
  assert(projectToJson(zoomApp.editor.project()) == projectToJson(zoomProject));

  zoomApp.move(EditorHarness::center(zoomApp.item(zoomButton.id)));
  wheel(-20);
  const auto small = zoomApp.item(zoomButton.id);
  assert(small.maximum.x - small.minimum.x == 48);
  io.AddKeyEvent(ImGuiKey_Home, true); zoomApp.frame();
  io.AddKeyEvent(ImGuiKey_Home, false); zoomApp.frame();
  const auto restored = zoomApp.item(zoomWindow.id);
  assert(restored.minimum.x == baseWindow.minimum.x && restored.minimum.y == baseWindow.minimum.y);
  assert(restored.maximum.x == baseWindow.maximum.x && restored.maximum.y == baseWindow.maximum.y);
  assert(projectToJson(zoomApp.editor.project()) == projectToJson(zoomProject) && zoomApp.editor.undoCount() == 0);

  // Color editors expose their actual frame, and empty auto-height layouts
  // have a usable design target before the first child is added.
  io.DisplaySize.y = 1100;
  auto resizeProject = createDefaultProject("Resize layouts and color controls");
  auto& resizeWindow = resizeProject.nodes.front();
  resizeWindow.width = 640; resizeWindow.height = 520;
  const auto makeResizeNode = [](WidgetType type, float x, float y, float width, float height) {
    auto node = createDefaultNode(type, x, y);
    node.useCursorPos = true; node.width = width; node.height = height;
    return node;
  };
  const auto pick = makeResizeNode(WidgetType::Selectable, 24, 48, 144, 24);
  const auto rgb = makeResizeNode(WidgetType::ColorEdit3, 224, 48, 160, 40);
  const auto rgba = makeResizeNode(WidgetType::ColorEdit4, 224, 92, 160, 40);
  auto editableFlow = makeResizeNode(WidgetType::PositionerFlow, 24, 152, 208, 0);
  auto editableTable = makeResizeNode(WidgetType::Table, 248, 152, 240, 104);
  const auto editableGrid = makeResizeNode(WidgetType::GridLayout, 24, 304, 192, 0);
  const auto editableChart = makeResizeNode(WidgetType::Chart2d, 248, 304, 232, 112);
  const auto orbitChart = makeResizeNode(WidgetType::Chart3d, 24, 372, 192, 100);
  const auto imageButton = makeResizeNode(WidgetType::ImageButton, 448, 48, 64, 48);
  editableFlow.children.push_back(createDefaultNode(WidgetType::Text));
  editableTable.children.push_back(createDefaultNode(WidgetType::Text));
  resizeWindow.children = {pick, rgb, rgba, editableFlow, editableTable, editableGrid, editableChart, orbitChart, imageButton};
  EditorHarness resizeApp(resizeProject);
  resizeApp.frame(); resizeApp.frame(); resizeApp.frame();
  const auto originalResizeJson = projectToJson(resizeProject);
  const auto resizeFromHandle = [&](const ImGuiNode& node, float dx, float dy, float expectedWidth, float expectedHeight) {
    const auto before = resizeApp.item(node.id);
    resizeApp.clickAt(ImVec2(before.maximum.x - 14, EditorHarness::center(before).y));
    const auto observation = resizeApp.item(node.id);
    const bool widthOnly = canvasResizeAxes(node.type) == CanvasResizeAxes::Width;
    const float handleRight = observation.resizeWidth
      ? observation.minimum.x + observation.resizeWidth->current : observation.maximum.x;
    const ImVec2 point(handleRight - 3, widthOnly ? EditorHarness::center(observation).y : observation.maximum.y - 3);
    resizeApp.move(point);
    if (ImGui::GetMouseCursor() != (widthOnly ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNWSE)) {
      std::cerr << "Resize handle unavailable for " << widgetTypeId(node.type)
                << " (selected: " << (resizeApp.editor.selectedNodeId() == node.id) << ")\n";
      std::abort();
    }
    io.AddMouseButtonEvent(0, true); resizeApp.frame();
    resizeApp.move(ImVec2(point.x + dx, point.y + dy));
    assert(projectToJson(resizeApp.editor.project()) == originalResizeJson);
    resizeApp.release();
    auto expected = resizeProject;
    auto* resizedNode = findNodeById(expected.nodes, node.id);
    resizedNode->width = expectedWidth;
    resizedNode->height = expectedHeight;
    assert(projectToJson(resizeApp.editor.project()) == projectToJson(expected));
    assert(resizeApp.editor.undoCount() == 1);
    resizeApp.undo();
    assert(projectToJson(resizeApp.editor.project()) == originalResizeJson);
  };
  assert(resizeApp.item(pick.id).maximum.x - resizeApp.item(pick.id).minimum.x == 144);
  resizeFromHandle(pick, 24, 16, 168, 40);
  for (const auto* color : {&rgb, &rgba}) {
    const auto observedColor = resizeApp.item(color->id);
    assert(observedColor.resizeWidth && observedColor.resizeWidth->current == 160);
    assert(observedColor.maximum.x - observedColor.minimum.x > 160);
    resizeFromHandle(*color, 24, 16, 184, color->height);
  }
  assert(resizeApp.item(editableFlow.id).drawList == resizeApp.item(editableFlow.children.front().id).drawList);
  resizeFromHandle(editableFlow, 24, 0, 232, 0);
  resizeFromHandle(editableTable, 24, 16, 264, 120);
  const auto emptyGridFrame = resizeApp.item(editableGrid.id);
  const auto gridHeight = emptyGridFrame.maximum.y - emptyGridFrame.minimum.y;
  assert(gridHeight > 0);
  resizeFromHandle(editableGrid, 0, 22, editableGrid.width, canvasWidgetExtent(gridHeight + 22, true));
  resizeFromHandle(editableChart, 24, 16, 256, 128);
  resizeFromHandle(orbitChart, 24, 16, 216, 120);
  resizeFromHandle(imageButton, 24, 16, 88, 64);
  const auto imageItem = resizeApp.item(imageButton.id);
  const auto imageCenter = EditorHarness::center(imageItem);
  resizeApp.beginDrag(imageItem, true);
  resizeApp.move(ImVec2(imageCenter.x + 24, imageCenter.y + 24));
  resizeApp.release();
  io.AddKeyEvent(ImGuiMod_Alt, false); resizeApp.frame();
  const auto* movedImage = findNodeById(resizeApp.editor.project().nodes, imageButton.id);
  assert(movedImage && movedImage->x == imageButton.x + 24 && movedImage->y == imageButton.y + 24);
  resizeApp.undo();
  assert(projectToJson(resizeApp.editor.project()) == originalResizeJson);
  const auto orbitStart = EditorHarness::center(resizeApp.item(orbitChart.id));
  const auto beginOrbit = [&] {
    resizeApp.move(orbitStart);
    io.AddKeyEvent(ImGuiMod_Shift, true); resizeApp.frame();
    io.AddMouseButtonEvent(0, true); resizeApp.frame();
    resizeApp.move(ImVec2(orbitStart.x + 40, orbitStart.y + 20));
    assert(projectToJson(resizeApp.editor.project()) == originalResizeJson);
  };
  beginOrbit();
  resizeApp.release();
  io.AddKeyEvent(ImGuiMod_Shift, false); resizeApp.frame();
  auto rotatedProject = resizeProject;
  auto* rotatedChart = findNodeById(rotatedProject.nodes, orbitChart.id);
  rotatedChart->props.chart3dRotX = 38; rotatedChart->props.chart3dRotY = -14;
  assert(projectToJson(resizeApp.editor.project()) == projectToJson(rotatedProject) && resizeApp.editor.undoCount() == 1);
  resizeApp.undo();
  assert(projectToJson(resizeApp.editor.project()) == originalResizeJson);
  beginOrbit();
  io.AddKeyEvent(ImGuiKey_Escape, true); resizeApp.frame();
  io.AddKeyEvent(ImGuiKey_Escape, false);
  io.AddKeyEvent(ImGuiMod_Shift, false); resizeApp.release();
  assert(projectToJson(resizeApp.editor.project()) == originalResizeJson && resizeApp.editor.undoCount() == 0);

  {
    auto chromeProject = createDefaultProject("Title bar canvas");
    auto& root = chromeProject.nodes.front();
    const auto rootId = root.id;
    root.width = 480;
    auto titleButton = createDefaultNode(WidgetType::TitleBarButton);
    titleButton.props.titleBarSide = "center";
    auto titleLabel = createDefaultNode(WidgetType::Text);
    titleLabel.props.inTitleBar = true;
    root.children = {titleButton, titleLabel};
    EditorHarness chrome(std::move(chromeProject));
    chrome.frame(); chrome.frame();
    const auto before = projectToJson(chrome.editor.project());
    const auto item = chrome.item(titleButton.id);
    const auto initialPosition = chrome.item(rootId).minimum;
    const auto labelItem = chrome.item(titleLabel.id);
    assert(labelItem.titleBar && labelItem.parentId == rootId);
    chrome.click(labelItem);
    assert(chrome.editor.selectedNodeId() == titleLabel.id && chrome.editor.undoCount() == 0);
    assert(item.parentId == rootId && item.minimum.y < initialPosition.y + 20);
    chrome.click(item);
    assert(chrome.editor.selectedNodeId() == titleButton.id && chrome.editor.undoCount() == 0);
    chrome.beginDrag(item);
    const auto start = EditorHarness::center(item);
    chrome.move(ImVec2(start.x + 64, start.y));
    chrome.release();
    const auto& updated = chrome.editor.project().nodes.front();
    assert(updated.children[0].props.titleBarSide == "free");
    assert(updated.children[0].props.titleBarX ==
           std::round((item.minimum.x - initialPosition.x + 64) / 8) * 8);
    assert(chrome.item(rootId).minimum.x == initialPosition.x && chrome.editor.undoCount() == 1);
    chrome.undo();
    assert(projectToJson(chrome.editor.project()) == before);
  }
  {
    auto project = createDefaultProject("Preview window controls");
    auto& root = project.nodes.front();
    const auto id = root.id;
    root.width = 420;
    root.height = 270;
    root.props.closable = true;
    root.props.showCollapseArrow = true;
    root.props.showMinimizeButton = true;
    root.props.showMaximizeButton = true;
    root.props.windowControlsSide = "right";
    root.props.windowControlsVariant = "pills";
    root.props.windowAppearance.titleOffsetLeft = 16;
    auto nativeBody = createDefaultNode(WidgetType::Checkbox);
    root.children = {nativeBody};
    const auto initial = projectToJson(project);
    const auto restored = projectFromJson(initial);
    assert(restored && projectToJson(*restored.project) == initial && restored.project->nodes[0].props.closable);
    EditorHarness app(std::move(project));
    app.frame(); app.frame();
    const auto menuButton = [&](const char* label) {
      auto* menu = ImGui::FindWindowByName("##MainMenuBar");
      assert(menu);
      ImGui::ActivateItemByID(ImHashStr(label, 0, menu->GetID("##menubar")));
      app.frame(); app.frame();
    };
    const auto controlCenter = [](const ImGuiWindow& window, int indexFromRight) {
      const auto& style = ImGui::GetStyle();
      const auto font = ImGui::GetFontSize();
      const float width = std::min(window.TitleBarHeight - 2.0f, font * 1.25f);
      const auto bar = window.TitleBarRect();
      return ImVec2(bar.Max.x - style.FramePadding.x - window.WindowBorderSize -
        width * 0.5f - indexFromRight * (width + style.ItemInnerSpacing.x), bar.GetCenter().y);
    };
    auto* design = ImGui::FindWindowByName(("###canvas_" + id).c_str());
    assert(design);
    app.clickAt(controlCenter(*design, 0)); // Close only selects the window in design.
    assert(app.editor.selectedNodeId() == id && app.editor.undoCount() == 0);
    menuButton("###Preview");
    auto* preview = ImGui::FindWindowByName(("###canvas-preview_" + id).c_str());
    assert(preview && preview->Active && preview->Size.y == 270);
    const auto initialPos = preview->Pos;
    app.clickAt(ImVec2(preview->Pos.x + 26, preview->TitleBarRect().GetCenter().y));
    app.frame();
    assert(preview->Collapsed);
    app.clickAt(ImVec2(preview->Pos.x + 26, preview->TitleBarRect().GetCenter().y));
    app.frame();
    assert(!preview->Collapsed && preview->Size.y == 270);
    app.clickAt(controlCenter(*preview, 2)); app.frame();
    assert(preview->Size.y < 40 && preview->Pos.x == initialPos.x);
    app.clickAt(controlCenter(*preview, 2)); app.frame();
    assert(preview->Size.y == 270);
    app.clickAt(controlCenter(*preview, 1)); app.frame();
    assert(preview->Size.x > 420 && preview->Size.y > 270 && preview->Pos.x < initialPos.x);
    app.clickAt(controlCenter(*preview, 1)); app.frame();
    assert(preview->Size.x == 420 && preview->Size.y == 270 && preview->Pos.x == initialPos.x);
    app.clickAt(controlCenter(*preview, 0)); app.frame();
    assert(!preview->Active);
    menuButton("###PreviewReset");
    assert(preview->Active && !preview->Collapsed && preview->Size.x == 420 && preview->Size.y == 270);
    menuButton("###Ide");
    assert(design->Active && design->Size.x == 420 && design->Size.y == 270);
    assert(projectToJson(app.editor.project()) == initial && app.editor.undoCount() == 0);
  }
  {
    auto project = createDefaultProject("Window appearance");
    auto& styled = project.nodes.front();
    const auto styledId = styled.id;
    styled.width = 420;
    styled.height = 270;
    styled.props.source = Json{{"futureAppearance", "preserve"}};
    applyWindowChromePreset(styled.props, WindowChrome::presets[4]); // Cyber
    auto& appearance = styled.props.windowAppearance;
    appearance.titleOffsetLeft = 23;
    appearance.titleOffsetRight = 17;
    appearance.paddingX = 19;
    appearance.paddingY = 12;
    auto bodyItem = createDefaultNode(WidgetType::Checkbox);
    styled.children = {bodyItem};
    auto plain = createDefaultNode(WidgetType::Window, 490, 48);
    const auto plainId = plain.id;
    plain.props.windowAppearance.showResizeGrip = true;
    auto plainItem = createDefaultNode(WidgetType::Checkbox);
    plain.children = {plainItem};
    project.nodes.push_back(std::move(plain));
    const auto original = projectToJson(project);
    const auto imported = projectFromJson(original);
    assert(imported && projectToJson(*imported.project) == original);
    assert(!imported.project->nodes[0].props.windowAppearance.showResizeGrip);
    assert(imported.project->nodes[1].props.windowAppearance.showResizeGrip);
    auto reset = *imported.project;
    applyWindowChromePreset(reset.nodes[0].props, WindowChrome::presets[0]); // ImGui
    const auto resetProps = projectToJson(reset)["nodes"][0]["props"];
    assert(!resetProps.contains("titleBgOverride") && !resetProps.contains("windowBorderColorOverride"));
    assert(resetProps["futureAppearance"] == "preserve" && resetProps["titleBarOffsetLeft"] == 23);

    EditorHarness app(std::move(project));
    app.frame(); app.frame();
    const auto* customWindow = ImGui::FindWindowByName(("###canvas_" + styledId).c_str());
    const auto* plainWindow = ImGui::FindWindowByName(("###canvas_" + plainId).c_str());
    assert(customWindow && plainWindow && customWindow->Active && plainWindow->Active);
    assert(customWindow->TitleBarHeight == 34 && plainWindow->TitleBarHeight != 34);
    assert(customWindow->WindowPadding.x == 19 && customWindow->WindowPadding.y == 12);
    assert(customWindow->WindowRounding == 2 && customWindow->WindowBorderSize == 1);
    const auto gripColor = ImGui::GetColorU32(ImGuiCol_ResizeGrip);
    const auto drewGrip = [&](const ImGuiWindow* window) {
      const auto& vertices = window->DrawList->VtxBuffer;
      return std::any_of(vertices.begin(), vertices.end(), [&](const ImDrawVert& vertex) {
        return vertex.col == gripColor;
      });
    };
    assert(!drewGrip(customWindow) && drewGrip(plainWindow));
    const auto styledBody = app.item(bodyItem.id);
    const auto plainBody = app.item(plainItem.id);
    assert(styledBody.minimum.y == customWindow->Pos.y + 34 + 12);
    assert(styledBody.maximum.y - styledBody.minimum.y == plainBody.maximum.y - plainBody.minimum.y);

    const auto header = customWindow->TitleBarRect();
    bool hasTitleFill = false, hasSeparator = false, hasShadow = false;
    for (const auto& vertex : customWindow->DrawList->VtxBuffer) {
      if (vertex.col == IM_COL32(0x0f, 0x17, 0x2a, 255)) {
        hasTitleFill = true;
        assert(vertex.pos.x >= header.Min.x + 22 && vertex.pos.x <= header.Max.x - 16);
      }
      if (vertex.col == IM_COL32(0x02, 0x84, 0xc7, 255)) {
        hasSeparator = true;
        assert(std::abs(vertex.pos.y - header.Max.y) < 2);
      }
      if (vertex.pos.y > customWindow->Pos.y + customWindow->Size.y + 4) hasShadow = true;
    }
    assert(hasTitleFill && hasSeparator && hasShadow);
    for (const auto& vertex : plainWindow->DrawList->VtxBuffer)
      assert(vertex.col != IM_COL32(0x0f, 0x17, 0x2a, 255) && vertex.col != IM_COL32(0x02, 0x84, 0xc7, 255));
    assert(projectToJson(app.editor.project()) == original && app.editor.undoCount() == 0);
    const ImVec2 corner(customWindow->Pos.x + customWindow->Size.x - 4,
      customWindow->Pos.y + customWindow->Size.y - 4);
    app.move(corner);
    assert(ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeNWSE);
    io.AddMouseButtonEvent(0, true); app.frame();
    app.move(ImVec2(corner.x + 32, corner.y + 24)); app.release();
    assert(app.editor.project().nodes[0].width > 420);
  }
  {
    auto project = createDefaultProject("Layout libre");
    auto& root = project.nodes.front();
    const auto rootId = root.id;
    root.width = 510; root.height = 360;
    auto menu = createDefaultNode(WidgetType::MenuBar);
    menu.props.menuBarEdge = MenuBarEdge::TitleBar;
    menu.props.items = {"Archivo", "Vista"};
    auto spacer = createDefaultNode(WidgetType::Spacing);
    auto separator = createDefaultNode(WidgetType::Separator);
    separator.width = 130; separator.height = 12;
    root.children = {menu, spacer, separator};
    EditorHarness app(std::move(project));
    app.frame(); app.frame();
    const auto header = [&]() {
      for (const auto& item : app.editor.designReport().items)
        if (item.nodeId == rootId && item.titleBarBounds) return item;
      std::abort();
    };
    const auto initial = header();
    assert(app.item(menu.id).titleBar && app.item(menu.id).minimum.y < initial.maximum.y);
    app.clickAt(ImVec2(initial.maximum.x - 24, EditorHarness::center(initial).y));
    const ImVec2 edge(initial.maximum.x - 4, EditorHarness::center(initial).y);
    app.move(edge); io.AddMouseButtonEvent(0, true); app.frame();
    app.move(ImVec2(edge.x - 64, edge.y)); app.release();
    assert(app.editor.project().nodes.front().props.windowAppearance.titleOffsetRight == 64);
    assert(header().maximum.x == initial.maximum.x - 64 && app.item(rootId).minimum.x == initial.minimum.x);
    app.undo();
    assert(header().maximum.x == initial.maximum.x);
    app.undo(true);
    assert(header().maximum.x == initial.maximum.x - 64);
    for (const auto* widget : {&spacer, &separator}) {
      const auto original = app.item(widget->id);
      const auto start = EditorHarness::center(original);
      const auto insideX = original.minimum.x + 20;
      app.move(ImVec2(insideX, start.y));
      io.AddMouseButtonEvent(0, true); app.frame();
      app.move(ImVec2(insideX + 80, start.y + 96 + (widget == &separator ? 40 : 0)));
      app.release();
      const auto* positioned = findNodeById(app.editor.project().nodes, widget->id);
      assert(positioned->useCursorPos && !positioned->sameLine && positioned->x > 50 && positioned->y > 80);
      const auto current = app.item(widget->id);
      app.click(current);
      const ImVec2 resize(current.maximum.x - 2, current.maximum.y - 2);
      app.move(resize); io.AddMouseButtonEvent(0, true); app.frame();
      app.move(ImVec2(resize.x + 32, resize.y + 16)); app.release();
      const auto* resized = findNodeById(app.editor.project().nodes, widget->id);
      assert(resized->props.utilityCustomSize && resized->width >= widget->width + 24);
    }
    const auto json = projectToJson(app.editor.project());
    const auto reopened = projectFromJson(json);
    assert(reopened && projectToJson(*reopened.project) == json);
    EditorHarness reloaded(*reopened.project);
    reloaded.frame(); reloaded.frame();
    assert(reloaded.item(menu.id).titleBar && reloaded.item(separator.id).minimum.y == app.item(separator.id).minimum.y);
  }
  {
    const auto example = loadProjectFile(std::filesystem::path(STUDIO_EXAMPLES_DIR) / "barra-y-lienzo-libre.json");
    assert(example && example.warnings.empty());
    EditorHarness app(*example.project);
    app.frame(); app.frame();
    assert(app.item("demo_menu").titleBar);
    const auto spacer = app.item("demo_spacing");
    const auto separator = app.item("demo_separator");
    assert(spacer.minimum.x < separator.minimum.x && spacer.minimum.y > separator.minimum.y);
    assert(spacer.maximum.y - spacer.minimum.y == 32 && separator.maximum.x - separator.minimum.x == 176);
  }
  {
    const auto example = loadProjectFile(std::filesystem::path(STUDIO_EXAMPLES_DIR) / "estado-y-progreso.json");
    assert(example && example.warnings.empty());
    EditorHarness app(*example.project);
    app.frame(); app.frame();
    const auto progress = app.item("job_progress");
    assert(progress.resizeWidth && progress.resizeWidth->current == 96.0f);
    app.click(progress);
    const ImVec2 handle(progress.minimum.x + progress.resizeWidth->current - 2, EditorHarness::center(progress).y);
    app.move(handle);
    assert(ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeEW);
    io.AddMouseButtonEvent(0, true); app.frame();
    app.move(ImVec2(handle.x + 24, handle.y)); app.release();
    assert(findNodeById(app.editor.project().nodes, "job_progress")->width == 120);
    app.undo();
    assert(projectToJson(app.editor.project()) == projectToJson(*example.project));

    const auto paletteToggle = app.target("palette/toggle");
    auto* palette = ImGui::FindWindowByName("##WidgetPalette");
    assert(palette && palette->DC.ChildWindows.Size == 1);
    auto* list = palette->DC.ChildWindows[0];
    ImGui::SetScrollY(list, list->Scroll.y + paletteToggle.minimum.y - paletteToggle.clipMinimum.y - 24.0f);
    app.frame(); app.frame();
    const auto visible = app.target("palette/toggle");
    assert(EditorHarness::center(visible).y >= visible.clipMinimum.y && EditorHarness::center(visible).y <= visible.clipMaximum.y);
    app.click(visible);
    assert(app.editor.project().nodes.front().children.back().type == WidgetType::Toggle);
  }
  ImGui::DestroyContext();
  std::cout << "[OK] Native selection, move, resize, hierarchy navigation, undo/redo, cancellation, clipping and zoom.\n";
}
