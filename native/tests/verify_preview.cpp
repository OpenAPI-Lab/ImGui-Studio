#include "studio_preview.hpp"
#include "studio_project_io.hpp"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>

#include <cassert>
#include <iostream>

using namespace ImGuiStudio;

namespace {
struct Harness {
  NativePreview preview;
  PlotSeriesEditor plotEditor;
  PreviewItem lastInspectorItem;
  size_t seriesApplications = 0;
  std::vector<ImGuiNode> nodes;
  explicit Harness(NativeImageCache* images = nullptr) : preview(images) {
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr; io.DisplaySize = ImVec2(1200, 900); io.DeltaTime = 1.0f / 60.0f;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    unsigned char* pixels = nullptr; int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  }
  ~Harness() { ImGui::DestroyContext(); }
  void frame(bool inspector = false, bool design = false) {
    preview.beginFrame(design);
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(1100, 850), ImGuiCond_Always);
    const auto* root = !inspector && nodes.size() == 1 && nodes.front().type == WidgetType::Window ? &nodes.front() : nullptr;
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings;
    if (root && root->props.noTitleBar) flags |= ImGuiWindowFlags_NoTitleBar;
    const bool open = ImGui::Begin(root ? "###fixture_title" : "Preview fixture", nullptr, flags);
    if (inspector) {
      for (auto& node : nodes) if (renderWidgetProperties(node, plotEditor)) ++seriesApplications;
      lastInspectorItem.minimum = ImGui::GetItemRectMin();
      lastInspectorItem.maximum = ImGui::GetItemRectMax();
    } else if (root) {
      if (!root->props.noTitleBar) preview.renderTitleBar(*root);
      if (open) preview.renderWindowBody(*root);
    } else preview.render(nodes);
    ImGui::End();
    ImGui::Render();
  }
  PreviewItem item(const std::string& id, int option = -1) const {
    for (const auto& value : preview.report().items)
      if (value.nodeId == id && value.optionIndex == option) return value;
    std::cerr << "Missing preview observation: " << id << " option " << option << '\n';
    std::abort();
  }
  void click(const PreviewItem& item, float offset = 8, bool inspector = false) {
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(item.minimum.x + offset, (item.minimum.y + item.maximum.y) * 0.5f);
    frame(inspector);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true); frame(inspector);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false); frame(inspector);
  }
  void replaceText(const std::string& text, bool inspector = false) {
    auto& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); frame(inspector);
    io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); frame(inspector);
    io.AddInputCharactersUTF8(text.c_str()); frame(inspector);
    if (!inspector) {
      io.AddKeyEvent(ImGuiKey_Enter, true); frame();
      io.AddKeyEvent(ImGuiKey_Enter, false); frame();
    }
  }
  Json document() const {
    auto p = createDefaultProject("Preview test");
    p.nodes[0].id = "fixed_root";
    p.nodes[0].children = nodes;
    return projectToJson(p);
  }
};

void verifyImages() {
  // Two RGBA pixels: opaque red and green at 50% alpha.
  constexpr unsigned char png[] = {
    0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0,0,0,0x0d,0x49,0x48,0x44,0x52,
    0,0,0,2,0,0,0,1,8,6,0,0,0,0xf4,0x22,0x7f,0x8a,0,0,0,0x11,0x49,0x44,0x41,0x54,
    0x78,0x9c,0x63,0xf8,0xcf,0xc0,0xf0,0x9f,0xe1,0x3f,0x43,3,0,0x10,0x79,3,0x7e,0x21,0xc0,
    0xfd,0x8d,0,0,0,0,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
  };
  // A 2 × 2 gray JPEG.
  constexpr unsigned char jpeg[] = {
    0xff,0xd8,0xff,0xe0,0x00,0x10,0x4a,0x46,0x49,0x46,0x00,0x01,0x02,0x00,0x00,0x01,
    0x00,0x01,0x00,0x00,0xff,0xdb,0x00,0x43,0x00,0x08,0x04,0x04,0x04,0x04,0x04,0x05,
    0x05,0x05,0x05,0x05,0x05,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
    0x06,0x06,0x07,0x07,0x07,0x08,0x08,0x08,0x07,0x07,0x07,0x06,0x06,0x07,0x07,0x08,
    0x08,0x08,0x08,0x09,0x09,0x09,0x08,0x08,0x08,0x08,0x09,0x09,0x0a,0x0a,0x0a,0x0c,
    0x0c,0x0b,0x0b,0x0e,0x0e,0x0e,0x11,0x11,0x14,0xff,0xc4,0x00,0x4a,0x00,0x01,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
    0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x10,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x11,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0xff,0xc0,0x00,0x11,0x08,0x00,0x02,0x00,0x02,0x03,0x01,
    0x22,0x00,0x02,0x11,0x00,0x03,0x11,0x00,0xff,0xda,0x00,0x0c,0x03,0x01,0x00,0x02,
    0x11,0x03,0x11,0x00,0x3f,0x00,0x00,0x0f,0xff,0xd9
  };
  const auto import = [](const unsigned char* bytes, size_t count, const char* extension) {
    const auto path = std::filesystem::temp_directory_path() / (nextId("studio-image") + extension);
    assert(writeTextFileAtomically(std::string(reinterpret_cast<const char*>(bytes), count), path));
    auto loaded = importImage(path);
    assert(std::filesystem::remove(path));
    assert(loaded.asset && loaded.error.empty() && !importImage(path).error.empty());
    return loaded.asset;
  };

  assert(glfwInit());
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  auto* window = glfwCreateWindow(100, 100, "ImGui Studio image test", nullptr, nullptr);
  assert(window);
  glfwMakeContextCurrent(window);
  GLuint textureId = 0;
  {
    NativeImageCache cache;
    Harness h(&cache);
    auto image = createDefaultNode(WidgetType::Image);
    image.width = 100; image.height = 70;
    image.props.imageAsset = import(png, sizeof(png), ".png");
    auto button = createDefaultNode(WidgetType::ImageButton);
    button.props.imageAsset = import(jpeg, sizeof(jpeg), ".jpg");
    h.nodes = {image, button};
    const auto document = h.document();
    const auto loaded = projectFromJson(document);
    assert(loaded && projectToJson(*loaded.project) == document);
    h.nodes = loaded.project->nodes[0].children;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 7);
    h.frame(); h.frame();
    const auto texture = cache.texture(h.nodes.front().props.imageAsset);
    assert(texture.id && texture.width == 2 && texture.height == 1 && texture.error.empty());
    const auto buttonTexture = cache.texture(h.nodes.back().props.imageAsset);
    assert(buttonTexture.id && buttonTexture.width == 2 && buttonTexture.height == 2 && buttonTexture.error.empty());
    textureId = static_cast<GLuint>(texture.id);
    GLint value = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &value); assert(value == 0);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &value); assert(value == 8);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &value); assert(value == 7);
    glBindTexture(GL_TEXTURE_2D, textureId);
    std::array<unsigned char, 8> rgba{};
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    assert((rgba == std::array<unsigned char, 8>{255, 0, 0, 255, 0, 255, 0, 128}));
    glBindTexture(GL_TEXTURE_2D, 0);

    const auto item = h.item(image.id);
    assert(item.maximum.x - item.minimum.x == 100 && item.maximum.y - item.minimum.y == 70);
    bool drawn = false;
    for (const auto& cmd : item.drawList->CmdBuffer) {
      if (cmd.GetTexID() != texture.id) continue;
      const auto& vertex = item.drawList->VtxBuffer[cmd.VtxOffset + item.drawList->IdxBuffer[cmd.IdxOffset]];
      assert(vertex.pos.x == item.minimum.x && vertex.pos.y == item.minimum.y + 10);
      drawn = true;
    }
    assert(drawn);
    h.click(h.item(button.id));
    assert(h.preview.report().activatedIds == std::vector<std::string>{button.id});
    assert(h.document() == document);
    h.frame(false, true);
    assert(h.preview.report().activatedIds.empty());

    h.nodes[0].props.imageAsset = std::make_shared<const Json>(Json{{"data", "invalid"}});
    h.preview.reset(); h.frame();
    assert(h.preview.report().limitations.size() == 1);
  }
  assert(textureId && !glIsTexture(textureId));
  glfwDestroyWindow(window);
  glfwTerminate();
  std::cout << "[OK] Embedded PNG and JPEG, actual OpenGL pixels, aspect ratio, activation and invalid data.\n";
}
}

int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--images") {
    verifyImages();
    return 0;
  }
  assert(safeNumericFormat("%.2f units", false));
  assert(safeNumericFormat("%03d", true));
  assert(safeNumericFormat("%.0f %%", false));
  for (const auto& bad : {"%s", "%n", "%*f", "%lf", "%f %f", "%999999999f", "%.999f", "%"})
    assert(!safeNumericFormat(bad, false));
  assert(!safeNumericFormat("%.2f", true));

  Harness h;
  const auto editorStyle = ImGui::GetStyle();
  ImGuiStyleConfig theme;
  theme.frameRounding = 7;
  theme.source = Json{{"framePaddingX", 9}, {"colors", {{"buttonHovered", "#ff0000"}}}};
  const auto originalTheme = theme.source;
  applyPreviewStyle(theme);
  assert(ImGui::GetStyle().FrameRounding == 7 && ImGui::GetStyle().FramePadding.x == 9);
  assert(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered].x == 1);
  assert(theme.source == originalTheme);
  ImGui::GetStyle() = editorStyle;
  auto check = createDefaultNode(WidgetType::Checkbox); check.props.checked = false;
  h.nodes = {check}; h.frame(); h.frame();
  const auto original = h.document();
  h.click(h.item(check.id)); h.frame();
  assert(h.preview.value(check.id)->checked);
  assert(h.document() == original); // Input changes only transient runtime values.
  h.preview.reset(); h.frame();
  assert(!h.preview.value(check.id)->checked);

  auto a = createDefaultNode(WidgetType::RadioButton), b = createDefaultNode(WidgetType::RadioButton);
  a.varName = b.varName = "shared_mode";
  a.props.intValue = b.props.intValue = 1;
  a.props.floatValue = b.props.floatValue = 1;
  a.props.radioValue = 1; b.props.radioValue = 2;
  h.nodes = {a, b}; h.preview.reset(); h.frame(); h.frame();
  const auto radios = h.document();
  h.click(h.item(b.id)); h.frame();
  assert(h.preview.value(a.id)->intValue == 2 && h.preview.value(b.id)->intValue == 2);
  assert(h.document() == radios);

  auto combo = createDefaultNode(WidgetType::Combo);
  combo.props.items = {"Uno", "Dos", "Tres"}; combo.props.selectedIndex = 0;
  h.nodes = {combo}; h.preview.reset(); h.frame(); h.frame();
  const auto combos = h.document();
  h.click(h.item(combo.id));
  h.frame(); h.frame(); // Let Dear ImGui place and auto-size the newly opened popup.
  h.click(h.item(combo.id, 2)); h.frame();
  assert(h.preview.value(combo.id)->selectedIndex == 2);
  assert(h.document() == combos);

  auto list = createDefaultNode(WidgetType::Listbox);
  list.height = 120; list.props.items = {"Uno", "Dos", "Tres"};
  list.props.selectedIndex = -1; // Invalid/empty selections render safely without changing data.
  h.nodes = {list}; h.preview.reset(); h.frame(); h.frame();
  h.click(h.item(list.id, 1)); h.frame();
  assert(h.preview.value(list.id)->selectedIndex == 1);
  assert(h.nodes[0].props.selectedIndex == -1);

  auto listView = createDefaultNode(WidgetType::ListView);
  listView.width = 260;
  listView.props.items = {"Cámara##principal", "Iluminación", "Terreno"};
  listView.props.selectedIndex = 99;
  h.nodes = {listView}; h.preview.reset(); h.frame(); h.frame();
  const auto listViewDocument = h.document();
  assert(h.item(listView.id, 1).minimum.y > h.item(listView.id, 0).minimum.y);
  h.click(h.item(listView.id, 1)); h.frame();
  assert(h.preview.value(listView.id)->selectedIndex == 1 && h.document() == listViewDocument);

  auto gridView = createDefaultNode(WidgetType::GridView);
  gridView.width = 300; gridView.height = 165; gridView.props.columns = 2;
  gridView.props.items = {"A", "B", "C", "D", "E"};
  h.nodes = {gridView}; h.preview.reset(); h.frame(); h.frame(); h.frame();
  const auto gridViewDocument = h.document();
  const auto cellA = h.item(gridView.id, 0), cellB = h.item(gridView.id, 1), cellC = h.item(gridView.id, 2);
  assert(cellA.minimum.y == cellB.minimum.y && cellB.minimum.x > cellA.minimum.x);
  assert(cellA.minimum.x == cellC.minimum.x && cellC.minimum.y > cellA.minimum.y);
  h.click(cellB); h.frame();
  assert(h.preview.value(gridView.id)->selectedIndex == 1 && h.document() == gridViewDocument);
  const auto importedGrid = projectFromJson(gridViewDocument);
  assert(importedGrid && importedGrid.project->nodes[0].children[0].props.columns == 2);

  auto pathView = createDefaultNode(WidgetType::PathView);
  pathView.width = 280; pathView.props.items = {"Vista 1", "Vista 2", "Vista 3"};
  pathView.props.selectedIndex = 0;
  h.nodes = {pathView}; h.preview.reset(); h.frame(); h.frame();
  const auto pathViewDocument = h.document();
  h.click(h.item(pathView.id, 2)); h.frame();
  assert(h.preview.value(pathView.id)->selectedIndex == 2);
  h.click(h.item(pathView.id, 0)); h.frame();
  assert(h.preview.value(pathView.id)->selectedIndex == 0 && h.document() == pathViewDocument);
  h.nodes.front().props.items.clear(); h.preview.reset(); h.frame();
  assert(h.preview.report().items.size() == 1 && h.preview.report().limitations.empty());

  auto loader = createDefaultNode(WidgetType::Loader);
  loader.props.loaderActive = false;
  loader.props.sourceComponent = "Panel %n";
  auto loadedCheck = createDefaultNode(WidgetType::Checkbox);
  loader.children = {loadedCheck};
  h.nodes = {loader}; h.preview.reset(); h.frame(); h.frame();
  const auto loaderDocument = h.document();
  assert(!h.preview.value(loadedCheck.id));
  h.click(h.item(loader.id, 0)); h.frame();
  assert(h.preview.value(loader.id)->loaderActive && h.preview.value(loadedCheck.id));
  h.click(h.item(loadedCheck.id)); h.frame();
  assert(h.preview.value(loadedCheck.id)->checked != loadedCheck.props.checked);
  h.click(h.item(loader.id, 0)); h.frame();
  assert(!h.preview.value(loader.id)->loaderActive);
  const auto hasLoadedItem = [&] {
    const auto& items = h.preview.report().items;
    return std::any_of(items.begin(), items.end(), [&](const PreviewItem& item) { return item.nodeId == loadedCheck.id; });
  };
  assert(!hasLoadedItem() && h.document() == loaderDocument);
  h.click(h.item(loader.id, 0)); h.frame();
  assert(hasLoadedItem() && h.preview.value(loadedCheck.id)->checked != loadedCheck.props.checked);
  const auto loaderRoundTrip = projectFromJson(loaderDocument);
  assert(loaderRoundTrip && !loaderRoundTrip.project->nodes[0].children[0].props.loaderActive);
  assert(loaderRoundTrip.project->nodes[0].children[0].props.sourceComponent == "Panel %n");

  auto component = createDefaultNode(WidgetType::Component);
  const auto part = createDefaultNode(WidgetType::Button);
  component.children = {part};
  h.nodes = {component}; h.preview.reset(); h.frame(); h.frame();
  assert(h.item(part.id).parentId == component.id);

  auto repeater = createDefaultNode(WidgetType::Repeater);
  repeater.props.repeatCount = 1;
  repeater.children = {part};
  h.nodes = {repeater}; h.preview.reset(); h.frame(); h.frame(); h.frame();
  const auto templateBounds = h.item(repeater.id);
  h.nodes[0].props.repeatCount = 4;
  const auto repeaterDocument = h.document();
  h.preview.reset();
  h.frame(); h.frame(); h.frame();
  const auto repeatedBounds = h.item(repeater.id);
  assert(repeatedBounds.maximum.y - repeatedBounds.minimum.y > templateBounds.maximum.y - templateBounds.minimum.y);
  const auto& repeatedItems = h.preview.report().items;
  assert(std::count_if(repeatedItems.begin(), repeatedItems.end(), [&](const PreviewItem& item) { return item.nodeId == part.id; }) == 1);
  assert(h.document() == repeaterDocument);
  const auto repeaterRoundTrip = projectFromJson(repeaterDocument);
  assert(repeaterRoundTrip && repeaterRoundTrip.project->nodes[0].children[0].props.repeatCount == 4);

  auto input = createDefaultNode(WidgetType::InputFloat);
  input.width = 220; input.props.floatValue = 0.25f;
  h.nodes = {input}; h.preview.reset(); h.frame(); h.frame();
  h.click(h.item(input.id), 35); h.replaceText("2.75");
  assert(h.preview.value(input.id)->floatValue == 2.75f);
  assert(h.nodes[0].props.floatValue == 0.25f);
  auto intInput = createDefaultNode(WidgetType::InputInt);
  intInput.width = 220; intInput.props.intValue = 7; intInput.props.format = "%03d";
  h.nodes = {intInput}; h.preview.reset(); h.frame(); h.frame();
  h.click(h.item(intInput.id), 35); h.replaceText("42");
  assert(h.preview.value(intInput.id)->intValue == 42 && h.nodes[0].props.intValue == 7);
  auto slider = createDefaultNode(WidgetType::SliderFloat);
  slider.width = 220; slider.props.minFloat = -1; slider.props.maxFloat = 1; slider.props.floatValue = 0.25f;
  h.nodes = {slider}; h.preview.reset(); h.frame(); h.frame();
  h.click(h.item(slider.id), 180); h.frame();
  assert(h.preview.value(slider.id)->floatValue > 0.5f && h.nodes[0].props.floatValue == 0.25f);

  auto text = createDefaultNode(WidgetType::InputText);
  text.width = 400; text.props.text = "original";
  h.nodes = {text}; h.preview.reset(); h.frame(); h.frame();
  const std::string longText = std::string(700, 'a') + u8"_á_日本";
  h.click(h.item(text.id), 35); h.replaceText(longText);
  assert(h.preview.value(text.id)->text == longText);
  assert(h.nodes[0].props.text == "original");
  auto bar = createDefaultNode(WidgetType::TabBar);
  auto firstLeaf = createDefaultNode(WidgetType::Text), secondLeaf = createDefaultNode(WidgetType::Text);
  bar.children[0].children = {firstLeaf}; bar.children[1].children = {secondLeaf};
  h.nodes = {bar}; h.preview.reset(); h.frame(); h.frame();
  const auto tabs = h.document();
  h.click(h.item(bar.children[1].id)); h.frame();
  auto shown = [&](const std::string& id) {
    return std::any_of(h.preview.report().items.begin(), h.preview.report().items.end(),
      [&](const auto& observation) { return observation.nodeId == id; });
  };
  assert(shown(secondLeaf.id) && !shown(firstLeaf.id));
  h.preview.reset(); h.frame(); h.frame();
  assert(shown(firstLeaf.id) && !shown(secondLeaf.id));
  assert(h.document() == tabs);

  auto flow = createDefaultNode(WidgetType::PositionerFlow);
  flow.width = 220;
  flow.props.spacing = 8;
  flow.props.border = false;
  auto narrow = createDefaultNode(WidgetType::Button); narrow.width = 80;
  auto wide = createDefaultNode(WidgetType::Button); wide.width = 110;
  auto hidden = createDefaultNode(WidgetType::Button); hidden.visible = false;
  auto wraps = createDefaultNode(WidgetType::Button); wraps.width = 64;
  wraps.sameLine = true; // Flow computes its own line breaks.
  flow.children = {narrow, wide, hidden, wraps};
  h.nodes = {flow}; h.preview.reset(); h.frame(); h.frame(); h.frame();
  const auto flowDocument = h.document();
  const auto firstFlow = h.item(narrow.id), secondFlow = h.item(wide.id), thirdFlow = h.item(wraps.id);
  assert(firstFlow.minimum.y == secondFlow.minimum.y);
  assert(secondFlow.minimum.x == firstFlow.maximum.x + 8);
  assert(thirdFlow.minimum.x == firstFlow.minimum.x && thirdFlow.minimum.y == firstFlow.maximum.y + 8);
  assert(secondFlow.parentId == flow.id && !h.preview.value(hidden.id));
  assert(h.item(flow.id).maximum.y >= thirdFlow.maximum.y);
  h.click(secondFlow);
  assert(std::find(h.preview.report().activatedIds.begin(), h.preview.report().activatedIds.end(), wide.id) !=
         h.preview.report().activatedIds.end());
  assert(h.document() == flowDocument);
  h.nodes.front().width = 340;
  const auto widenedFlow = h.document();
  h.frame(); h.frame();
  assert(h.item(wide.id).minimum.y == h.item(wraps.id).minimum.y);
  assert(h.item(wraps.id).minimum.x == h.item(wide.id).maximum.x + 8);
  assert(h.document() == widenedFlow);

  for (const auto type : {WidgetType::PlotLines, WidgetType::PlotHistogram}) {
    auto plot = createDefaultNode(type);
    plot.props.plotValues = {0.2f, std::numeric_limits<float>::max()};
    plot.props.overlay = "Sample overlay";
    h.nodes = {plot}; h.frame(true); h.frame(true);
    const auto baseline = h.document();
    const auto appliedBefore = h.seriesApplications;
    auto openSeries = [&] {
      h.click(h.lastInspectorItem, 8, true);
      h.frame(true);
      return h.lastInspectorItem;
    };
    auto apply = openSeries();
    h.click(apply, 8, true); h.frame(true);
    assert(h.document() == baseline); // Even the largest finite float round-trips through the editor.
    assert(h.lastInspectorItem.minimum.y < apply.minimum.y && h.seriesApplications == appliedBefore);

    apply = openSeries();
    auto textbox = apply;
    textbox.minimum.y -= 40; textbox.maximum.y -= 40;
    h.click(textbox, 8, true);
    h.replaceText("0.75, invalid, 2", true);
    h.click(apply, 8, true);
    assert(h.document() == baseline && h.seriesApplications == appliedBefore);
    h.click(textbox, 8, true);
    h.replaceText("-2.5, 3e-1 4.25", true);
    assert(h.document() == baseline);
    h.click(apply, 8, true); h.frame(true);
    const std::vector<float> expected = {-2.5f, 0.3f, 4.25f};
    assert(h.nodes[0].props.plotValues == expected);
    assert(h.seriesApplications == appliedBefore + 1);
    const auto imported = projectFromJson(h.document());
    assert(imported && imported.project->nodes[0].children[0].props.plotValues == expected);
    assert(imported.project->nodes[0].children[0].props.overlay == "Sample overlay");
  }

  auto chart = createDefaultNode(WidgetType::Chart2d);
  chart.props.plotValues = {-8, 42, 15, 60};
  chart.props.chartLabels = {"Norte", "Sur", "Centro", "Este"};
  chart.props.overlay = "Serie %n";
  chart.props.color = {1, 0, 1, 1};
  const auto coloredVertices = [&](const char* mode) {
    chart.props.chart2dType = mode;
    h.nodes = {chart}; h.preview.reset(); h.frame();
    const auto preview = h.item(chart.id);
    assert(preview.maximum.x - preview.minimum.x == chart.width);
    assert(preview.maximum.y - preview.minimum.y == chart.height);
    assert(h.preview.report().limitations.empty());
    return std::count_if(preview.drawList->VtxBuffer.begin(), preview.drawList->VtxBuffer.end(),
      [](const ImDrawVert& vertex) { return vertex.col == IM_COL32(255, 0, 255, 255); });
  };
  for (const char* mode : {"line", "area", "bar", "pie", "scatter", "spline"})
    assert(coloredVertices(mode) > 0);
  chart.props.plotValues.clear();
  assert(coloredVertices("line") == 0);
  chart.props.plotValues.assign(2500, 10);
  chart.props.plotValues[1551] = std::numeric_limits<float>::max();
  assert(coloredVertices("spline") > 0);
  chart.props.plotValues[1551] = 50;
  assert(coloredVertices("pie") > 0);

  auto chartDocument = h.document();
  auto& props = chartDocument["nodes"][0]["children"][0]["props"];
  assert(!props.contains("min") && !props.contains("max"));
  props["chart2dType"] = "future-type";
  props["max"] = 99;
  auto chartImport = projectFromJson(chartDocument);
  assert(chartImport && projectToJson(*chartImport.project) == chartDocument);
  auto& importedChart = chartImport.project->nodes.front().children.front();
  assert(importedChart.props.chartAutoMin && !importedChart.props.chartAutoMax);
  assert(importedChart.props.chartLabels == chart.props.chartLabels && importedChart.props.maxFloat == 99);
  importedChart.props.chart2dType = "bar";
  importedChart.props.chartAutoMax = true;
  const auto editedProps = projectToJson(*chartImport.project)["nodes"][0]["children"][0]["props"];
  assert(editedProps["chart2dType"] == "bar" && !editedProps.contains("max"));

  auto orbitChart = createDefaultNode(WidgetType::Chart3d);
  orbitChart.props.color = {1, 0, 1, 1};
  const auto mesh = [&](const char* mode) {
    orbitChart.props.chart3dType = mode;
    h.nodes = {orbitChart}; h.preview.reset(); h.frame(); h.frame();
    const auto item = h.item(orbitChart.id);
    assert(item.maximum.x - item.minimum.x == orbitChart.width);
    assert(item.maximum.y - item.minimum.y == orbitChart.height);
    assert(h.preview.report().limitations.empty());
    return item.drawList->VtxBuffer.Size;
  };
  const int barsMesh = mesh("bars"), scatterMesh = mesh("scatter"), surfaceMesh = mesh("surface");
  assert(barsMesh != scatterMesh && surfaceMesh != scatterMesh && barsMesh != surfaceMesh);
  const auto chartItem = h.item(orbitChart.id);
  const ImVec2 chartCenter((chartItem.minimum.x + chartItem.maximum.x) * 0.5f,
                           (chartItem.minimum.y + chartItem.maximum.y) * 0.5f);
  const auto orbitDocument = h.document();
  auto& io = ImGui::GetIO();
  io.AddMousePosEvent(chartCenter.x, chartCenter.y); h.frame();
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true); h.frame();
  io.AddMousePosEvent(chartCenter.x + 50, chartCenter.y + 30); h.frame();
  assert(h.preview.value(orbitChart.id)->chart3dRotX == 43);
  assert(std::abs(h.preview.value(orbitChart.id)->chart3dRotY + 8) < 0.0001f);
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, false); h.frame();
  assert(h.document() == orbitDocument);

  auto legacy3d = orbitDocument;
  auto& legacyProps = legacy3d["nodes"][0]["children"][0]["props"];
  legacyProps["chart3dType"] = "future-3d";
  legacyProps["chart3dRotX"] = "unknown";
  legacyProps["chart3dRotY"] = 75;
  const auto imported3d = projectFromJson(legacy3d);
  assert(imported3d && projectToJson(*imported3d.project) == legacy3d);
  assert(imported3d.project->nodes[0].children[0].props.chart3dRotX == 28);
  assert(imported3d.project->nodes[0].children[0].props.chart3dRotY == 75);
  h.nodes = {imported3d.project->nodes[0].children[0]};
  h.preview.reset(); h.frame();
  assert(h.preview.report().limitations.empty());

  auto animation = createDefaultNode(WidgetType::Animation);
  animation.width = 280; animation.height = 110;
  animation.props.animType = "slide_x";
  animation.props.animDuration = 600;
  animation.props.animEasing = "linear";
  animation.props.animLoop = false;
  auto animatedButton = createDefaultNode(WidgetType::Button);
  animatedButton.width = 120; animatedButton.height = 22;
  animatedButton.useCursorPos = true;
  animatedButton.x = 16; animatedButton.y = 6;
  animation.children = {animatedButton};
  const auto startAnimation = [&](const char* effect) {
    animation.props.animType = effect;
    h.nodes = {animation}; h.preview.reset(); h.frame(); h.frame();
    return h.item(animatedButton.id);
  };
  const auto advanceAnimation = [&](float seconds) {
    const float previous = io.DeltaTime;
    io.DeltaTime = seconds; h.frame(); io.DeltaTime = previous;
    return h.item(animatedButton.id);
  };
  const auto slideStart = startAnimation("slide_x");
  const auto animationDocument = h.document();
  const auto slideEnd = advanceAnimation(0.7f);
  assert(slideEnd.minimum.x > slideStart.minimum.x + 10);
  assert(slideEnd.minimum.y == slideStart.minimum.y);
  h.click(slideEnd);
  assert(h.preview.report().activatedIds == std::vector<std::string>{animatedButton.id});
  assert(advanceAnimation(0.7f).minimum.x == slideEnd.minimum.x);
  h.click(h.item(animation.id, 0));
  assert(!h.preview.value(animation.id)->animRunning);
  const float pausedX = h.item(animatedButton.id).minimum.x;
  assert(pausedX > slideStart.minimum.x && pausedX < slideEnd.minimum.x);
  assert(advanceAnimation(0.7f).minimum.x == pausedX);
  h.click(h.item(animation.id, 0));
  assert(h.preview.value(animation.id)->animRunning && h.item(animatedButton.id).minimum.x < pausedX);
  assert(h.document() == animationDocument);
  h.frame(false, true);
  assert(h.item(animatedButton.id).minimum.x == pausedX);
  assert(h.item(animatedButton.id).parentId == animation.id);
  assert(h.item(animation.id).maximum.y - h.item(animation.id).minimum.y == animation.height);

  animation.props.animLoop = true;
  animation.props.animEasing = "sine";
  const auto bounceStart = startAnimation("bounce_y");
  assert(advanceAnimation(0.28f).minimum.y < bounceStart.minimum.y - 5);
  assert(advanceAnimation(0.30f).minimum.y >= bounceStart.minimum.y - 1);
  const auto scaleStart = startAnimation("scale_pulse");
  const auto scalePeak = advanceAnimation(0.28f);
  assert(scalePeak.maximum.x - scalePeak.minimum.x > scaleStart.maximum.x - scaleStart.minimum.x + 6);

  const auto maxAlpha = [](const PreviewItem& item) {
    ImU32 alpha = 0;
    for (const auto& vertex : item.drawList->VtxBuffer)
      alpha = std::max(alpha, vertex.col >> IM_COL32_A_SHIFT);
    return alpha;
  };
  const ImU32 dimAlpha = maxAlpha(startAnimation("pulse_alpha"));
  assert(maxAlpha(advanceAnimation(0.28f)) > dimAlpha + 100);
  io.AddMousePosEvent(900, 700); h.frame();
  const ImU32 initialColor = startAnimation("color_shift").drawList->VtxBuffer[0].col;
  const auto tinted = advanceAnimation(0.28f);
  assert(tinted.drawList->VtxBuffer[0].col != initialColor);
  assert(ImGui::GetStyle().Alpha == editorStyle.Alpha);
  assert(io.FontGlobalScale == 1.0f);

  const auto animationRoundTrip = projectFromJson(animationDocument);
  assert(animationRoundTrip && projectToJson(*animationRoundTrip.project) == animationDocument);
  const auto& importedAnimation = animationRoundTrip.project->nodes[0].children[0].props;
  assert(importedAnimation.animType == "slide_x" && importedAnimation.animEasing == "linear");
  assert(importedAnimation.animDuration == 600 && !importedAnimation.animLoop && importedAnimation.animRunning);
  auto legacyAnimation = animationDocument;
  auto& legacyAnimationProps = legacyAnimation["nodes"][0]["children"][0]["props"];
  legacyAnimationProps["animType"] = "future-effect";
  legacyAnimationProps["animDuration"] = "unknown";
  legacyAnimationProps.erase("animRunning");
  const auto animationImport = projectFromJson(legacyAnimation);
  assert(animationImport && projectToJson(*animationImport.project) == legacyAnimation);
  assert(animationImport.project->nodes[0].children[0].props.animDuration == 1400);
  assert(animationImport.project->nodes[0].children[0].props.animRunning);

  // Every supported type is submitted through its actual ImGui path. Tabs need
  // their legal parent; invisible branches must not allocate runtime state.
  size_t supported = 0;
  h.preview.setKnobsInstalled(true);
  for (const auto& definition : kWidgetDefinitions) {
    if (!NativePreview::supports(definition.type)) continue;
    ++supported;
    auto node = createDefaultNode(definition.type);
    if (definition.type == WidgetType::TabItem) {
      auto bar = createDefaultNode(WidgetType::TabBar); bar.children = {node}; h.nodes = {bar};
    } else h.nodes = {node};
    h.preview.reset(); h.frame(); h.frame();
    assert(h.preview.value(node.id));
    if (!h.preview.report().limitations.empty()) {
      std::cerr << "Unexpected limitation for " << definition.id << ": " << h.preview.report().limitations[0] << '\n';
      return 1;
    }
    auto before = h.document();
    h.frame(true); // Merely showing the inspector must not normalize/import data.
    assert(h.document() == before);
  }
  auto unsupported = createDefaultNode(WidgetType::Unknown);
  h.nodes = {unsupported}; h.preview.reset(); h.frame();
  assert(!h.preview.report().limitations.empty());
  unsupported.visible = false;
  h.nodes = {unsupported}; h.preview.reset(); h.frame();
  assert(!h.preview.value(unsupported.id));

  auto titleWindow = createDefaultNode(WidgetType::Window);
  auto left = createDefaultNode(WidgetType::TitleBarButton);
  auto center = createDefaultNode(WidgetType::TitleBarButton);
  center.props.titleBarSide = "center";
  center.props.titleBarBtnVariant = "outline";
  auto right = createDefaultNode(WidgetType::TitleBarButton);
  right.props.titleBarSide = "right";
  right.props.titleBarBtnVariant = "icon";
  right.props.titleBarIcon = "search";
  right.label.clear();
  auto free = createDefaultNode(WidgetType::TitleBarButton);
  free.props.titleBarSide = "free";
  free.props.titleBarX = 220;
  free.props.titleBarY = 1;
  free.props.titleBarBtnRounding = 4;
  free.height = 12;
  auto body = createDefaultNode(WidgetType::Button);
  auto headerCheck = createDefaultNode(WidgetType::Checkbox);
  headerCheck.props.checked = false;
  headerCheck.props.inTitleBar = true;
  headerCheck.props.titleBarSide = "free";
  headerCheck.props.titleBarX = 790;
  auto headerInput = createDefaultNode(WidgetType::InputText);
  headerInput.label.clear();
  headerInput.width = 110;
  headerInput.props.inTitleBar = true;
  headerInput.props.titleBarSide = "free";
  headerInput.props.titleBarX = 925;
  auto headerMenu = createDefaultNode(WidgetType::MenuBar);
  headerMenu.props.menuBarEdge = MenuBarEdge::TitleBar;
  headerMenu.props.titleBarSide = "free";
  headerMenu.props.titleBarX = 620;
  headerMenu.props.items = {"Archivo", "Editar"};
  titleWindow.children = {left, center, right, free, body, headerCheck, headerInput, headerMenu};
  auto titleProject = createDefaultProject("Title bar buttons");
  titleProject.nodes = {titleWindow};
  auto titleJson = projectToJson(titleProject);
  titleJson["nodes"][0]["children"][0]["props"]["titleBarBtnRounding"] = nullptr;
  const auto loadedTitle = projectFromJson(titleJson);
  assert(loadedTitle && projectToJson(*loadedTitle.project) == titleJson);
  h.nodes = loadedTitle.project->nodes;
  h.preview.reset(); h.frame(); h.frame();
  const auto leftItem = h.item(left.id);
  const auto centerItem = h.item(center.id);
  const auto rightItem = h.item(right.id);
  const auto freeItem = h.item(free.id);
  assert(leftItem.parentId == titleWindow.id && leftItem.rootId == titleWindow.id && leftItem.depth == 1);
  assert(leftItem.maximum.x < freeItem.minimum.x && freeItem.maximum.x < centerItem.minimum.x &&
         centerItem.maximum.x < rightItem.minimum.x);
  assert(freeItem.minimum.x == 220 && freeItem.minimum.y == 1 && freeItem.maximum.y == 13);
  assert(rightItem.maximum.y < h.item(body.id).minimum.y);
  assert(h.item(headerCheck.id).titleBar && h.item(headerInput.id).titleBar);
  assert(h.item(headerCheck.id).parentId == titleWindow.id && h.item(headerInput.id).minimum.x == 925);
  const auto bodyLocation = h.item(body.id).minimum;
  const auto menu = h.item(headerMenu.id);
  assert(menu.titleBar && menu.minimum.x == 620 && menu.maximum.y < bodyLocation.y);
  h.click(menu, 13);
  assert(ImGui::GetCurrentContext()->OpenPopupStack.Size > 0);
  ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true); h.frame();
  ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, false); h.frame();
  assert(ImGui::GetCurrentContext()->OpenPopupStack.Size == 0);
  h.click(h.item(headerCheck.id));
  assert(h.preview.value(headerCheck.id)->checked);
  h.click(h.item(headerInput.id));
  const std::string unicodeText = "Français — cœur · 한국어 · 日本語 · 简体中文";
  h.replaceText(unicodeText);
  assert(h.preview.value(headerInput.id)->text == unicodeText);
  assert(h.item(body.id).minimum.x == bodyLocation.x && h.item(body.id).minimum.y == bodyLocation.y);
  h.click(rightItem);
  assert(h.preview.report().activatedIds == std::vector<std::string>{right.id});
  titleProject.nodes = h.nodes;
  assert(projectToJson(titleProject) == titleJson);
  h.nodes[0].props.noTitleBar = true;
  h.frame();
  assert(h.preview.report().items.size() == 1 && h.item(body.id).parentId == titleWindow.id);

  auto colorInput = createDefaultNode(WidgetType::InputText);
  colorInput.props.text = "Seleccionable";
  colorInput.props.source = {{"frameBgOverride", "#29374b"}, {"textSelectedBgOverride", "#379fcb"}};
  auto colorRow = createDefaultNode(WidgetType::Selectable);
  colorRow.props.selected = true;
  colorRow.props.source = {{"headerColorOverride", "#39684c"}};
  auto colorButton = createDefaultNode(WidgetType::Button);
  colorButton.props.source = {{"buttonHoveredColorOverride", "#7055bd"}, {"buttonActiveColorOverride", "#ba8140"}};
  h.nodes = {colorInput, colorRow, colorButton};
  const auto styledJson = h.document();
  const auto loadedColors = projectFromJson(styledJson);
  assert(loadedColors && projectToJson(*loadedColors.project) == styledJson);
  h.nodes = loadedColors.project->nodes.front().children;
  h.preview.reset(); h.frame(); h.frame();
  const auto drewColor = [&](const std::string& id, ImU32 color) {
    const auto& vertices = h.item(id).drawList->VtxBuffer;
    return std::any_of(vertices.begin(), vertices.end(),
      [=](const ImDrawVert& point) { return point.col == color; });
  };
  assert(drewColor(colorInput.id, IM_COL32(0x29, 0x37, 0x4b, 255)));
  assert(drewColor(colorRow.id, IM_COL32(0x39, 0x68, 0x4c, 255)));
  h.click(h.item(colorInput.id));
  io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); h.frame();
  assert(drewColor(colorInput.id, IM_COL32(0x37, 0x9f, 0xcb, 255)));
  io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); h.frame();
  const auto buttonBounds = h.item(colorButton.id);
  io.AddMousePosEvent(buttonBounds.minimum.x + 6, buttonBounds.minimum.y + 6); h.frame();
  assert(drewColor(colorButton.id, IM_COL32(0x70, 0x55, 0xbd, 255)));
  io.AddMouseButtonEvent(0, true); h.frame();
  assert(drewColor(colorButton.id, IM_COL32(0xba, 0x81, 0x40, 255)));
  io.AddMouseButtonEvent(0, false); h.frame();

  auto dial = createDefaultNode(WidgetType::Knob);
  h.nodes = {dial};
  const auto dialDocument = h.document();
  h.preview.setKnobsInstalled(false);
  h.preview.reset(); h.frame();
  assert(!h.preview.report().limitations.empty());
  h.preview.setKnobsInstalled(true);
  h.frame(); h.frame();
  assert(h.preview.report().limitations.empty());
  const auto dialBounds = h.item(dial.id);
  const ImVec2 dialCenter(dialBounds.minimum.x + dial.width * 0.5f,
    dialBounds.minimum.y + ImGui::GetTextLineHeightWithSpacing() + dial.width * 0.5f);
  io.AddMousePosEvent(dialCenter.x, dialCenter.y); h.frame();
  io.AddMouseButtonEvent(0, true); h.frame();
  io.AddMousePosEvent(dialCenter.x + 36.0f, dialCenter.y - 36.0f); h.frame();
  assert(h.preview.value(dial.id)->floatValue != dial.props.floatValue);
  io.AddMouseButtonEvent(0, false); h.frame();
  assert(h.document() == dialDocument);
  h.preview.setKnobsInstalled(false);
  h.frame();
  assert(!h.preview.report().limitations.empty() && h.document() == dialDocument);

  auto toggle = createDefaultNode(WidgetType::Toggle);
  auto spinner = createDefaultNode(WidgetType::Spinner);
  auto circular = createDefaultNode(WidgetType::CircularProgress);
  spinner.props.source = Json{{"progressFillColorOverride", "#2a9ecb"}};
  circular.props.source = Json{{"progressFillColorOverride", "#55ce95"}};
  h.nodes = {toggle, spinner, circular};
  const auto indicatorDocument = h.document();
  h.preview.reset(); h.frame(); h.frame();
  const auto arcPositions = [&](const std::string& id, ImU32 color) {
    std::vector<std::pair<float, float>> result;
    for (const auto& vertex : h.item(id).drawList->VtxBuffer)
      if (vertex.col == color) result.emplace_back(vertex.pos.x, vertex.pos.y);
    return result;
  };
  const auto spinnerColor = IM_COL32(42, 158, 203, 255);
  const auto circularColor = IM_COL32(85, 206, 149, 255);
  const auto rotating = arcPositions(spinner.id, spinnerColor);
  h.frame();
  assert(!rotating.empty() && rotating != arcPositions(spinner.id, spinnerColor));
  const auto progress = arcPositions(circular.id, circularColor);
  assert(!progress.empty());
  h.click(h.item(toggle.id));
  assert(h.preview.value(toggle.id)->checked && h.document() == indicatorDocument);
  h.frame(false, true);
  assert(!h.preview.value(toggle.id)->checked);
  const auto frozen = arcPositions(spinner.id, spinnerColor);
  h.frame(false, true);
  assert(!frozen.empty() && frozen == arcPositions(spinner.id, spinnerColor));
  h.nodes[2].props.progress = 0.0f;
  h.frame(false, true);
  assert(arcPositions(circular.id, circularColor).empty());
  h.nodes[2].props.progress = 1.0f;
  h.frame(false, true);
  assert(arcPositions(circular.id, circularColor).size() > progress.size());

  auto unsafeFormat = createDefaultNode(WidgetType::SliderFloat);
  unsafeFormat.props.format = "%n";
  h.nodes = {unsafeFormat}; h.preview.reset(); h.frame();
  assert(!h.preview.report().limitations.empty());
  assert(h.nodes[0].props.format == "%n");
  auto oversizedTable = createDefaultNode(WidgetType::Table);
  oversizedTable.props.columns = 600;
  h.nodes = {oversizedTable}; h.preview.reset(); h.frame();
  assert(!h.preview.report().limitations.empty() && h.nodes[0].props.columns == 600);
  auto sideMenu = createDefaultNode(WidgetType::MenuBar);
  sideMenu.props.menuBarEdge = MenuBarEdge::Free;
  h.nodes = {sideMenu}; h.preview.reset(); h.frame();
  assert(!h.preview.report().limitations.empty());
  std::cout << "[OK] " << supported << " native widget paths, interactive values, radio groups, selectors, UTF-8 and document isolation.\n";
}
