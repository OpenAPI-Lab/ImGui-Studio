#include "imguistudio/export/studio_cpp_export.hpp"
#include "imguistudio/export/studio_android_export.hpp"
#include "imguistudio/export/studio_apple_export.hpp"
#include "imguistudio/export/studio_csharp_export.hpp"
#include "imguistudio/export/studio_java_export.hpp"
#include "imguistudio/export/studio_c_export.hpp"
#include "imguistudio/export/studio_python_export.hpp"
#include "imguistudio/export/studio_rust_export.hpp"
#include "studio_chrome_interactions.hpp"
#include <cassert>
#include <iostream>
#include <set>

using namespace ImGuiStudio;
namespace fs = std::filesystem;
static void write(const fs::path& path, const std::string& text) {
  const auto result = writeTextFileAtomically(text, path);
  if (!result) throw std::runtime_error(result.error);
}
static StudioProject edgeProject() {
  auto project = createDefaultProject("class");
  auto& root = project.nodes.front(); root.id = "root"; root.width = 1000; root.height = 760;
  auto add = [&](WidgetType type, const std::string& id, const std::string& name) -> ImGuiNode& {
    auto node = createDefaultNode(type); node.id = id; node.varName = name;
    node.label = "Same label %n \\\"\nUTF-8: español"; root.children.push_back(node);
    return root.children.back();
  };
  add(WidgetType::Checkbox,"bool","class").props.checked = true;
  add(WidgetType::SliderFloat,"float","class").props.floatValue = 48;
  add(WidgetType::InputInt,"int","a_").props.intValue = -2147483647;
  add(WidgetType::SliderFloat3,"vec","a").props.defaultVec = {0.25f,0.5f,0.75f};
  add(WidgetType::InputText,"text","9 text").props.text = std::string(1200,'x') + "\nñ\\\"" + std::string("\0end",4);
  auto& combo = add(WidgetType::Combo,"combo","combo"); combo.props.items = {"First","Second %n"}; combo.props.selectedIndex = 1;
  add(WidgetType::Combo,"emptyCombo","combo").props.items.clear();
  auto& plot = add(WidgetType::PlotLines,"plot","plot");
  plot.props.plotValues = {2,4,8}; plot.props.minFloat = -2; plot.props.maxFloat = 9;
  add(WidgetType::PlotHistogram,"emptyPlot","plot").props.plotValues.clear();
  add(WidgetType::InputFloat,"format","invalid").props.format = "%n";
  add(WidgetType::RadioButton,"radio1","a-b").props.radioValue = 0;
  add(WidgetType::RadioButton,"radio2","a-b").props.radioValue = 1;
  add(WidgetType::RadioButton,"radio3","a b").props.radioValue = 2;
  add(WidgetType::Checkbox,"hidden","unused").visible = false;
  add(WidgetType::Checkbox,"reservedNamespace","std");
  add(WidgetType::InputText,"className","value_classState");
  add(WidgetType::SameLine,"legacySameLine","unused");
  auto addFlow = [&](const std::string& id, float x, float width) {
    auto flow = createDefaultNode(WidgetType::PositionerFlow, x, 80);
    flow.id = id; flow.width = width; flow.useCursorPos = true;
    flow.props.spacing = 8; flow.props.border = false;
    size_t index = 0;
    for (const float childWidth : {80.0f, 110.0f, 64.0f}) {
      auto child = createDefaultNode(WidgetType::Button);
      child.id = id + "_" + std::to_string(index++);
      child.width = childWidth; child.height = 30; child.sameLine = true;
      flow.children.push_back(std::move(child));
    }
    root.children.push_back(std::move(flow));
  };
  addFlow("wrappedFlow", 355, 220);
  addFlow("wideFlow", 590, 340);
  auto addModel = [&](WidgetType type, const char* id, float x, float width) {
    auto view = createDefaultNode(type, x, 185);
    view.id = view.varName = id; view.width = width; view.height = 120; view.useCursorPos = true;
    view.props.items = {"Repeated ## label", "Repeated ## label", "Third %n", "Cuarto"};
    view.props.columns = 2; view.props.selectedIndex = 0;
    root.children.push_back(std::move(view));
  };
  addModel(WidgetType::ListView, "modelList", 350, 190);
  addModel(WidgetType::GridView, "modelGrid", 550, 206);
  addModel(WidgetType::PathView, "modelPath", 764, 210);
  auto emptyPath = createDefaultNode(WidgetType::PathView, 350, 320);
  emptyPath.id = "emptyModel"; emptyPath.useCursorPos = true;
  emptyPath.width = 190; emptyPath.props.items.clear();
  root.children.push_back(std::move(emptyPath));
  auto addTemplate = [&](WidgetType type, const char* id, const char* childId, float x, float width) {
    auto node = createDefaultNode(type, x, 470);
    node.id = node.varName = id; node.useCursorPos = true; node.width = width; node.height = 172;
    node.props.sourceComponent = "Template %n"; node.props.loaderActive = false; node.props.repeatCount = 4;
    auto check = createDefaultNode(WidgetType::Checkbox);
    check.id = check.varName = childId; check.props.checked = false;
    node.children.push_back(std::move(check));
    root.children.push_back(std::move(node));
  };
  addTemplate(WidgetType::Repeater, "templateRepeater", "repeatChoice", 350, 192);
  addTemplate(WidgetType::Loader, "templateLoader", "loadedChoice", 555, 218);
  addTemplate(WidgetType::Component, "templateComponent", "componentChoice", 780, 198);
  for (size_t i = 0; i < std::size(Chart2d::modes); ++i) {
    const auto& mode = Chart2d::modes[i];
    auto chart = createDefaultNode(WidgetType::Chart2d, 16 + 160 * static_cast<float>(i), 666);
    chart.id = chart.varName = std::string("chart_") + mode.id;
    chart.useCursorPos = true; chart.width = 144; chart.height = 78;
    chart.props.chart2dType = mode.id;
    chart.props.overlay = "Evolución %n";
    chart.props.chartLabels = {"Uno", "Dos %n", "Tres", "Cuatro"};
    chart.props.color = {1.0f, 0.0f, static_cast<float>(i + 1) / 16, 1.0f};
    chart.props.chartAutoMin = chart.props.chartAutoMax = false;
    chart.props.minFloat = -10; chart.props.maxFloat = 100;
    chart.props.plotValues = {-2, 20, 46, 13};
    root.children.push_back(std::move(chart));
  }
  auto emptyChart = createDefaultNode(WidgetType::Chart2d, 20, 510);
  emptyChart.id = "emptyChart";
  emptyChart.useCursorPos = true;
  emptyChart.props.plotValues.clear();
  root.children.push_back(std::move(emptyChart));
  auto other = createDefaultNode(WidgetType::Window); other.id = "other"; other.label = root.label; other.x = 1100;
  project.nodes.push_back(other);
  auto animationWindow = createDefaultNode(WidgetType::Window, 1090, 4);
  animationWindow.id = "animations"; animationWindow.label = "Animations";
  animationWindow.width = 280; animationWindow.height = 405;
  const std::string effects[] = {"pulse_alpha", "bounce_y", "slide_x", "scale_pulse", "color_shift"};
  for (size_t i = 0; i < std::size(effects); ++i) {
    auto animation = createDefaultNode(WidgetType::Animation, 12, 28 + 72 * static_cast<float>(i));
    animation.id = animation.varName = effects[i];
    animation.useCursorPos = true; animation.width = 248; animation.height = 66;
    animation.props.animType = effects[i]; animation.props.animDuration = 600;
    animation.props.animLoop = i != 2;
    animation.props.animEasing = i == 2 ? "linear" : "sine";
    animation.props.animRunning = i != 2;
    auto child = createDefaultNode(i == 2 ? WidgetType::Checkbox : WidgetType::Button, 16, 2);
    child.id = child.varName = effects[i] + "_child";
    child.label = "Child"; child.useCursorPos = true; child.width = 120; child.height = 22;
    if (child.type == WidgetType::Checkbox) child.props.checked = false;
    animation.children.push_back(std::move(child));
    animationWindow.children.push_back(std::move(animation));
  }
  project.nodes.push_back(std::move(animationWindow));
  auto imageWindow = createDefaultNode(WidgetType::Window, 24, 805);
  project.canvas.height = 1000;
  imageWindow.id = "embedded_images"; imageWindow.label = "Embedded images";
  imageWindow.props.showCollapseArrow = true;
  imageWindow.props.closable = true;
  imageWindow.width = 385; imageWindow.height = 128;
  const char* textures[] = {
    "iVBORw0KGgoAAAANSUhEUgAAAAIAAAABCAYAAAD0In+KAAAAEUlEQVR4nGP4z8Dwn+E/QwMAEHkDfiHA/Y0AAAAASUVORK5CYII=",
    "/9j/4AAQSkZJRgABAgAAAQABAAD/2wBDAAgEBAQEBAUFBQUFBQYGBgYGBgYGBgYGBgYHBwcICAgHBwcGBgcHCAgICAkJCQgICAg"
    "JCQoKCgwMCwsODg4RERT/xABKAAEAAAAAAAAAAAAAAAAAAAAAAQEAAAAAAAAAAAAAAAAAAAAAEAEAAAAAAAAAAAAAAAAAAAAAEQE"
    "AAAAAAAAAAAAAAAAAAAAA/8AAEQgAAgACAwEiAAIRAAMRAP/aAAwDAQACEQMRAD8AAA//2Q=="
  };
  for (int i = 0; i < 3; ++i) {
    auto node = createDefaultNode(i == 1 ? WidgetType::ImageButton : WidgetType::Image, 16 + 112.0f * i, 30);
    node.id = node.varName = i == 1 ? "image_action" : "still_image_" + std::to_string(i);
    node.useCursorPos = true; node.width = 100; node.height = 70;
    node.props.imageAsset = std::make_shared<const Json>(Json{{"data", textures[i == 1]}});
    imageWindow.children.push_back(std::move(node));
  }
  for (const auto* side : {"left", "center", "right", "free"}) {
    auto node = createDefaultNode(WidgetType::TitleBarButton);
    node.id = node.varName = std::string("title_") + side;
    node.label.clear();
    node.width = 30; node.height = 14;
    node.props.titleBarSide = side;
    node.props.titleBarBtnVariant = "outline";
    node.props.titleBarIcon = "search";
    node.props.titleBarX = 255;
    node.props.titleBarY = 2;
    node.props.source = Json{{"buttonColorOverride", "#FF6644"}, {"textColorOverride", "#FFFFFF"}};
    imageWindow.children.push_back(std::move(node));
  }
  project.nodes.push_back(std::move(imageWindow));
  auto orbitWindow = createDefaultNode(WidgetType::Window, 1100, 420);
  orbitWindow.id = "orbitCharts";
  orbitWindow.label = "Gráficos 3D";
  orbitWindow.width = 274; orbitWindow.height = 490;
  const Color4 accents[] = {{0, 1, 1, 1}, {1, 0, 1, 1}, {1, 0, 0, 1}};
  for (size_t i = 0; i < std::size(Chart3d::modes); ++i) {
    const auto& mode = Chart3d::modes[i];
    auto chart = createDefaultNode(WidgetType::Chart3d, 16, 34 + 144 * static_cast<float>(i));
    chart.id = chart.varName = std::string("chart3d_") + mode.id;
    chart.useCursorPos = true; chart.width = 186; chart.height = 132;
    chart.props.chart3dType = mode.id; chart.props.color = accents[i];
    chart.props.chart3dRotX = 33; chart.props.chart3dRotY = -40;
    chart.props.overlay = "Orientación %n";
    orbitWindow.children.push_back(std::move(chart));
  }
  auto empty3d = createDefaultNode(WidgetType::Chart3d, 208, 34);
  empty3d.id = empty3d.varName = "chart3d_empty";
  empty3d.useCursorPos = true; empty3d.width = 55; empty3d.height = 80;
  empty3d.props.plotValues.clear();
  orbitWindow.children.push_back(std::move(empty3d));
  project.nodes.push_back(std::move(orbitWindow));
  return project;
}

static std::string chartInteractions(const GeneratedCpp& generated, bool cApi) {
  const std::string tint = cApi ? "igGetColorU32_Vec4((ImVec4){1, 0, blue, 1})"
                                : "ImGui::GetColorU32(ImVec4(1, 0, blue, 1))";
  const std::string lineTint = cApi ? "igGetColorU32_Vec4((ImVec4){1, 0, 0.0625f, 1})"
                                    : "ImGui::GetColorU32(ImVec4(1, 0, 0.0625f, 1))";
  std::string code = R"TEST(    for (int mode = 0; mode < 6; ++mode) {
        const float blue = (float)(mode + 1) / 16;
)TEST";
  code += "        const ImU32 accent = " + tint + ";\n";
  code += R"TEST(        bool painted = false;
        for (int i = 0; i < window->DrawList->VtxBuffer.Size; ++i)
            if (window->DrawList->VtxBuffer.Data[i].col == accent) painted = true;
        assert(painted);
    }
    float peaks[800] = {0};
    peaks[274] = 100;
    peaks[611] = -100;
    size_t samples[512];
)TEST";
  code += std::string("    const size_t sample_count = ") + (cApi ? "" : "StudioChartDetails::") +
    "StudioChartSamples(peaks, 800, samples);\n";
  code += R"TEST(
    bool has_low = false, has_high = false;
    assert(sample_count <= 512 && samples[0] == 0 && samples[sample_count - 1] == 799);
    for (size_t i = 0; i < sample_count; ++i) {
        has_low |= samples[i] == 611;
        has_high |= samples[i] == 274;
    }
    assert(has_low && has_high);
)TEST";
  code += "    const ImU32 chart_line_color = " + lineTint + ";\n";
  code += R"TEST(    float original_top = FLT_MAX;
    for (int i = 0; i < window->DrawList->VtxBuffer.Size; ++i) {
        const ImDrawVert vertex = window->DrawList->VtxBuffer.Data[i];
        if (vertex.col == chart_line_color && vertex.pos.y < original_top) original_top = vertex.pos.y;
    }
)TEST";
  code += "    state." + generated.variables.at("chart_line") + "[0] = 100;\n";
  code += cApi ? "    igNewFrame(); RenderStudioUI(&state); igRender();\n"
              : "    ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();\n";
  code += R"TEST(    float changed_top = FLT_MAX;
    for (int i = 0; i < window->DrawList->VtxBuffer.Size; ++i) {
        const ImDrawVert vertex = window->DrawList->VtxBuffer.Data[i];
        if (vertex.col == chart_line_color && vertex.pos.y < changed_top) changed_top = vertex.pos.y;
    }
    assert(changed_top + 12 < original_top);
)TEST";
  return code;
}

static std::string chart3dInteractions(const StudioProject& project, const GeneratedCpp& generated, bool cApi) {
  const auto& chartWindow = project.nodes.back();
  const auto title = CppCodegen::quote(chartWindow.label + "###studio_" +
    CppCodegen::hexId(chartWindow.id) + "_" + CppCodegen::hexId(chartWindow.imguiId));
  const auto field = "state." + generated.variables.at("chart3d_bars");
  const std::string find = cApi ? "igFindWindowByName(" : "ImGui::FindWindowByName(";
  const std::string frame = cApi ? "igNewFrame(); RenderStudioUI(&state); igRender();"
                                : "ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();";
  const std::string pos = cApi ? "ImGuiIO_AddMousePosEvent(io, " : "io.AddMousePosEvent(";
  const std::string button = cApi ? "ImGuiIO_AddMouseButtonEvent(io, 0, " : "io.AddMouseButtonEvent(0, ";
  const std::string color = cApi ? "igGetColorU32_Vec4((ImVec4)" : "ImGui::GetColorU32(ImVec4";
  const std::string cyan = color + (cApi ? "{0, 1, 1, 1})" : "(0, 1, 1, 1))");
  const std::string magenta = color + (cApi ? "{1, 0, 1, 1})" : "(1, 0, 1, 1))");
  std::string code = "    ImGuiWindow* chart_window = " + find + title + "); assert(chart_window);\n";
  code += "    const ImU32 bars_color = " + cyan + ", scatter_color = " + magenta + ";\n";
  code += R"TEST(    bool bars_drawn = false, scatter_drawn = false, surface_drawn = false;
    float first_bar_x = FLT_MAX;
    for (int i = 0; i < chart_window->DrawList->VtxBuffer.Size; ++i) {
        const ImDrawVert vertex = chart_window->DrawList->VtxBuffer.Data[i];
        if (vertex.col == bars_color) {
            bars_drawn = true;
            if (vertex.pos.x < first_bar_x) first_bar_x = vertex.pos.x;
        }
        scatter_drawn |= vertex.col == scatter_color;
        surface_drawn |= vertex.pos.y > chart_window->Pos.y + 322 &&
                         (vertex.col & 0x00ffff00u) == 0 && (vertex.col & 0xffu) > 60;
    }
    assert(bars_drawn && scatter_drawn && surface_drawn);
    const float orbit_x = chart_window->Pos.x + 90, orbit_y = chart_window->Pos.y + 90;
)TEST";
  code += "    assert(" + field + ".elevation == 33 && " + field + ".azimuth == -40);\n";
  code += "    assert(" + field + ".values[0] == 0.35f);\n";
  code += "    " + pos + "orbit_x, orbit_y); " + frame + "\n";
  code += "    " + button + "true); " + frame + "\n";
  code += "    " + pos + "orbit_x + 50, orbit_y + 40); " + frame + "\n";
  code += "    " + button + "false); " + frame + "\n";
  code += "    assert(fabsf(" + field + ".elevation - 53) < 0.01f && fabsf(" + field + ".azimuth + 10) < 0.01f);\n";
  code += R"TEST(    float rotated_bar_x = FLT_MAX;
    for (int i = 0; i < chart_window->DrawList->VtxBuffer.Size; ++i) {
        const ImDrawVert vertex = chart_window->DrawList->VtxBuffer.Data[i];
        if (vertex.col == bars_color && vertex.pos.x < rotated_bar_x) rotated_bar_x = vertex.pos.x;
    }
    assert(rotated_bar_x < FLT_MAX && fabsf(rotated_bar_x - first_bar_x) > 1);
)TEST";
  return code;
}

static std::string animationInteractions(const StudioProject& project, const GeneratedCpp& generated, bool cApi) {
  const auto it = std::find_if(project.nodes.begin(), project.nodes.end(),
                              [](const ImGuiNode& node) { return node.id == "animations"; });
  assert(it != project.nodes.end());
  const auto title = CppCodegen::quote(it->label + "###studio_" + CppCodegen::hexId(it->id) + "_");
  const std::string find = cApi ? "igFindWindowByName(" : "ImGui::FindWindowByName(";
  const std::string frame = cApi ? "igNewFrame(); RenderStudioUI(&state); igRender();"
                                : "ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();";
  const std::string tap = cApi ? "click(&state, " : "click(";
  const std::string slide = "state." + generated.variables.at("slide_x");
  const std::string checkbox = "state." + generated.variables.at("slide_x_child");
  std::string code = "    ImGuiWindow* animation_window = " + find + title + "); assert(animation_window);\n";
  code += R"TEST(    assert(animation_window->DC.ChildWindows.Size == 5);
    ImGuiWindow** panels = animation_window->DC.ChildWindows.Data;
    ImGuiWindow* content[5];
    for (int i = 0; i < 5; ++i) {
        assert(panels[i]->DC.ChildWindows.Size == 1);
        content[i] = panels[i]->DC.ChildWindows.Data[0];
        assert(content[i]->DrawList->VtxBuffer.Size > 0);
    }
)TEST";
  code += "    assert(!" + slide + ".running);\n";
  code += "    const float neutral_x = content[2]->Pos.x;\n";
  code += "    const float play_x = panels[2]->DC.CursorStartPos.x + 8;\n";
  code += "    const float play_y = panels[2]->DC.CursorStartPos.y + 8;\n";
  code += "    " + tap + "play_x, play_y); assert(" + slide + ".running);\n";
  for (const char* effect : {"pulse_alpha", "bounce_y", "scale_pulse", "color_shift"})
    code += "    state." + generated.variables.at(effect) + ".start = -1;\n";
  code += "    " + frame + "\n";
  code += R"TEST(    const float slide_start_x = content[2]->Pos.x, bounce_start_y = content[1]->Pos.y;
    const float small_width = content[3]->ContentSize.x;
    const ImU32 dim = content[0]->DrawList->VtxBuffer.Data[0].col;
    const ImU32 original_hue = content[4]->DrawList->VtxBuffer.Data[0].col;
    io_DELTA = 0.28f;
)TEST";
  code += "    " + frame + "\n";
  code += R"TEST(    assert(content[1]->Pos.y < bounce_start_y - 5);
    assert((content[0]->DrawList->VtxBuffer.Data[0].col >> 24) > (dim >> 24) + 40);
    assert(content[4]->DrawList->VtxBuffer.Data[0].col != original_hue);
)TEST";
  code += "    io_DELTA = 1.0f / 60; " + frame + "\n";
  code += "    assert(content[3]->ContentSize.x > small_width + 5);\n";
  code += "    io_DELTA = 0.7f; " + frame + "\n";
  code += "    const float slide_end_x = content[2]->Pos.x;\n";
  code += "    assert(slide_end_x > slide_start_x + 10 && slide_end_x > neutral_x);\n";
  code += "    io_DELTA = 1.0f / 60;\n";
  code += "    assert(!" + checkbox + ");\n";
  code += "    " + tap + "slide_end_x + 22, content[2]->Pos.y + 9); assert(" + checkbox + ");\n";
  code += "    assert(content[2]->Pos.x == slide_end_x);\n";
  code += "    " + tap + "play_x, play_y); assert(!" + slide + ".running && " + slide + ".start < 0);\n";
  code += "    assert(content[2]->Pos.x == neutral_x);\n";
  const std::string delta = cApi ? "io->DeltaTime" : "io.DeltaTime";
  for (auto pos = code.find("io_DELTA"); pos != std::string::npos; pos = code.find("io_DELTA", pos + delta.size()))
    code.replace(pos, 8, delta);
  return code;
}

constexpr char imageTestCallbacks[] = R"TEST(
static StudioImageTexture testImageUpload(const unsigned char* bytes, size_t count, void* user) {
    ++*(int*)user;
    if (bytes[0] == 0x89) {
        assert(count == 74 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G');
        const StudioImageTexture texture = {900, 2, 1};
        return texture;
    }
    assert(count == 202 && bytes[0] == 0xff && bytes[1] == 0xd8 && bytes[count - 1] == 0xd9);
    const StudioImageTexture texture = {901, 2, 2};
    return texture;
}
static void testImageRelease(StudioImageTexture texture, void* user) {
    assert(texture.id == 900 || texture.id == 901);
    --*(int*)user;
}
)TEST";

static std::string loadTestImages(bool cApi, const std::string& variable = "state") {
  const auto state = (cApi ? "&" : "") + variable;
  return "    int image_count = 0;\n    assert(LoadStudioImages(" + state + ", testImageUpload, &image_count));\n"
    "    assert(image_count == 2 && LoadStudioImages(" + state + ", testImageUpload, &image_count));\n"
    "    assert(image_count == 2);\n";
}

static std::string imageInteractions(const GeneratedCpp& generated, bool cApi) {
  const auto title = CppCodegen::quote("Embedded images###studio_" + CppCodegen::hexId("embedded_images") + "_");
  const std::string frame = cApi ? "igNewFrame(); RenderStudioUI(&state); igRender();" :
    "ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();";
  const std::string find = cApi ? "igFindWindowByName(" : "ImGui::FindWindowByName(";
  const std::string move = cApi ? "ImGuiIO_AddMousePosEvent(io, " : "io.AddMousePosEvent(";
  const std::string click = cApi ? "ImGuiIO_AddMouseButtonEvent(io, 0, " : "io.AddMouseButtonEvent(0, ";
  const auto pressed = "state." + generated.variables.at("image_action");
  std::string code = "    ImGuiWindow* image_window = " + find + title + "); assert(image_window);\n";
  code += R"TEST(    int png_quads = 0, jpeg_quads = 0;
    for (int i = 0; i < image_window->DrawList->CmdBuffer.Size; ++i) {
        const ImDrawCmd cmd = image_window->DrawList->CmdBuffer.Data[i];
        if (cmd.TextureId == 900) {
            assert(cmd.ElemCount == 6);
            const ImDrawVert vertex = image_window->DrawList->VtxBuffer.Data[cmd.VtxOffset +
                image_window->DrawList->IdxBuffer.Data[cmd.IdxOffset]];
            assert(vertex.pos.y == image_window->Pos.y + 40);
            ++png_quads;
        } else if (cmd.TextureId == 901) ++jpeg_quads;
    }
    assert(png_quads == 2 && jpeg_quads == 1);
    const float button_x = image_window->Pos.x + 160, button_y = image_window->Pos.y + 52;
)TEST";
  code += "    assert(!" + pressed + ");\n";
  code += "    " + move + "button_x, button_y); " + frame + "\n";
  code += "    " + click + "true); " + frame + "\n";
  code += "    " + click + "false); " + frame + "\n";
  code += "    assert(" + pressed + ");\n    " + frame + "\n    assert(!" + pressed + ");\n";
  code += std::string("    ReleaseStudioImages(") + (cApi ? "&state" : "state") + ", testImageRelease, &image_count);\n";
  code += "    assert(image_count == 0 && !state.studio_image_textures[0].id && !state.studio_image_textures[1].id);\n";
  return code;
}

static std::string titleInteractions(const GeneratedCpp& generated, bool cApi) {
  const std::string click = cApi ? "    click(&state, " : "    click(";
  const std::string frame = cApi ? "igNewFrame(); RenderStudioUI(&state); igRender();" :
    "ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();";
  const auto state = [&](const std::string& id) { return "state." + generated.variables.at(id); };
  std::string code;
  const std::pair<const char*, const char*> cases[] = {
    {"left", "32"}, {"center", "190"}, {"right", "347"}, {"free", "266"}
  };
  for (const auto& [side, x] : cases) {
    const auto pressed = state(std::string("title_") + side);
    code += click + "image_window->Pos.x + " + x + ", image_window->Pos.y + 8);\n";
    code += "    assert(" + pressed + ");\n    " + frame + "\n    assert(!" + pressed + ");\n";
  }
  code += cApi ? "    igSetWindowCollapsed_WindowPtr(image_window, true, ImGuiCond_Always);\n"
               : "    ImGui::SetWindowCollapsed(image_window, true, ImGuiCond_Always);\n";
  code += "    " + frame + "\n    assert(image_window->Collapsed);\n";
  code += click + "image_window->Pos.x + 266, image_window->Pos.y + 8);\n";
  code += "    assert(" + state("title_free") + ");\n";
  code += "    " + state("embedded_images") + " = false;\n    " + frame + "\n";
  code += "    assert(!" + state("title_free") + ");\n";
  return code;
}

static std::string modelInteractions(const GeneratedCpp& generated, bool cApi) {
  const auto& vars = generated.variables;
  const std::string prefix = cApi ? "    click(&state, " : "    click(";
  const std::string gui = cApi ? "ig" : "ImGui::";
  std::string code = R"TEST(    ImGuiWindow *list = NULL, *grid = NULL, *path = NULL;
    for (int i = 0; i < window->DC.ChildWindows.Size; ++i) {
        ImGuiWindow* child = window->DC.ChildWindows.Data[i];
        if (child->Size.x == 190.0f && list == NULL) list = child;
        if (child->Size.x == 206.0f) grid = child;
        if (child->Size.x == 210.0f) path = child;
    }
    assert(list && grid && path);
)TEST";
  code += "    const float text_line = " + gui + "GetTextLineHeightWithSpacing();\n";
  code += "    const float frame = " + gui + "GetFrameHeight(), row = " + gui + "GetFrameHeightWithSpacing();\n";
  code += prefix + "list->DC.CursorStartPos.x + 9, list->DC.CursorStartPos.y + text_line + row + frame / 2);\n";
  code += "    assert(state." + vars.at("modelList") + " == 1);\n";
  code += prefix + "grid->Pos.x + grid->Size.x * 0.75f, grid->DC.CursorStartPos.y + text_line + frame / 2);\n";
  code += "    assert(state." + vars.at("modelGrid") + " == 1);\n";
  code += "    const float path_y = path->DC.CursorStartPos.y + 2 * text_line + frame / 2;\n";
  code += prefix + "path->DC.CursorStartPos.x + 5, path_y);\n";
  code += "    assert(state." + vars.at("modelPath") + " == 3);\n";
  code += prefix + "path->Pos.x + path->Size.x - path->WindowPadding.x - 5, path_y);\n";
  code += "    assert(state." + vars.at("modelPath") + " == 0);\n";
  code += R"TEST(    ImGuiWindow *repeater = NULL, *loader = NULL, *component = NULL;
    for (int i = 0; i < window->DC.ChildWindows.Size; ++i) {
        ImGuiWindow* child = window->DC.ChildWindows.Data[i];
        if (child->Size.x == 192.0f) repeater = child;
        if (child->Size.x == 218.0f) loader = child;
        if (child->Size.x == 198.0f) component = child;
    }
    assert(repeater && loader && component && repeater->DC.ChildWindows.Size == 4);
    ImGuiWindow* first_instance = repeater->DC.ChildWindows.Data[0];
    ImGuiWindow* second_instance = repeater->DC.ChildWindows.Data[1];
    assert(first_instance->ID != second_instance->ID && first_instance->Pos.y < second_instance->Pos.y);
)TEST";
  code += "    assert(!state." + vars.at("templateLoader") + " && !state." + vars.at("loadedChoice") + ");\n";
  code += prefix + "first_instance->DC.CursorStartPos.x + 5, first_instance->DC.CursorStartPos.y + frame / 2);\n";
  code += "    assert(state." + vars.at("repeatChoice") + ");\n";
  code += prefix + "second_instance->DC.CursorStartPos.x + 5, second_instance->DC.CursorStartPos.y + frame / 2);\n";
  code += "    assert(!state." + vars.at("repeatChoice") + ");\n";
  code += prefix + "component->DC.CursorStartPos.x + 5, component->DC.CursorStartPos.y + text_line + frame / 2);\n";
  code += "    assert(state." + vars.at("componentChoice") + ");\n";
  code += "    const float loader_x = loader->DC.CursorStartPos.x + 5, loader_y = loader->DC.CursorStartPos.y + frame / 2;\n";
  code += prefix + "loader_x, loader_y);\n";
  code += "    assert(state." + vars.at("templateLoader") + ");\n";
  code += prefix + "loader_x, loader_y + row);\n";
  code += "    assert(state." + vars.at("loadedChoice") + ");\n";
  code += prefix + "loader_x, loader_y);\n";
  code += "    assert(!state." + vars.at("templateLoader") + " && state." + vars.at("loadedChoice") + ");\n";
  return code;
}

struct ModelClick { const char* id; int x, y, index; };
// The edge project's model children start at y = 185 inside the root at (48, 48).
constexpr ModelClick modelClicks[] = {
  {"modelList", 415, 290, 1}, {"modelGrid", 750, 267, 1},
  {"modelPath", 825, 283, 3}, {"modelPath", 1008, 283, 0},
};

static void writeManagedInteractionTests(const fs::path& directory, const CppBundle& csharp, const CppBundle& java) {
  std::string cs = R"CS(        void Frame()
        {
            StudioUI.TemplateItems.Clear();
            ImGui.NewFrame(); StudioUI.Render(state); ImGui.Render();
        }
        void Click(float x, float y)
        {
            io.AddMousePosEvent(x, y); Frame();
            io.AddMouseButtonEvent(0, true); Frame();
            io.AddMouseButtonEvent(0, false); Frame();
        }
)CS";
  std::string j = R"JAVA(import imgui.ImGui;
import imgui.ImGuiIO;
import imgui.type.ImBoolean;
import imgui.type.ImInt;

public final class ModelInteraction {
    private static void frame(StudioUI.UIState state) {
        StudioUI.templateItems.clear();
        ImGui.newFrame(); StudioUI.render(state); ImGui.render();
    }

    private static void click(StudioUI.UIState state, ImGuiIO io, float x, float y) {
        io.addMousePosEvent(x, y); frame(state);
        io.addMouseButtonEvent(0, true); frame(state);
        io.addMouseButtonEvent(0, false); frame(state);
    }

    private static void clickItem(StudioUI.UIState state, ImGuiIO io, String node) {
        var item = StudioUI.templateItems.stream().filter(entry -> entry.node().equals(node)).findFirst().orElseThrow();
        click(state, io, item.x() + 5, item.y() + 5);
    }

    private static float chartTop(int accent) {
        var data = ImGui.getDrawData();
        int stride = imgui.ImDrawData.sizeOfImDrawVert();
        float top = Float.POSITIVE_INFINITY;
        for (int list = 0; list < data.getCmdListsCount(); ++list) {
            var vertices = data.getCmdListVtxBufferData(list).order(java.nio.ByteOrder.nativeOrder());
            for (int i = 0; i < data.getCmdListVtxBufferSize(list); ++i) {
                int offset = i * stride;
                if (vertices.getInt(offset + 16) == accent) top = Math.min(top, vertices.getFloat(offset + 4));
            }
        }
        return top;
    }

    private static float chart3dMesh() {
        int cyan = ImGui.getColorU32(0f, 1f, 1f, 1f), magenta = ImGui.getColorU32(1f, 0f, 1f, 1f);
        var data = ImGui.getDrawData();
        int stride = imgui.ImDrawData.sizeOfImDrawVert();
        float firstBar = Float.POSITIVE_INFINITY;
        boolean scatter = false, surface = false;
        for (int list = 0; list < data.getCmdListsCount(); ++list) {
            var vertices = data.getCmdListVtxBufferData(list).order(java.nio.ByteOrder.nativeOrder());
            for (int i = 0; i < data.getCmdListVtxBufferSize(list); ++i) {
                int offset = i * stride;
                float x = vertices.getFloat(offset), y = vertices.getFloat(offset + 4);
                if (x < 1100 || y < 420) continue;
                int color = vertices.getInt(offset + 16);
                if (color == cyan) firstBar = Math.min(firstBar, x);
                scatter |= color == magenta;
                surface |= y > 742 && (color & 0x00ffff00) == 0 && (color & 0xff) > 60;
            }
        }
        if (!Float.isFinite(firstBar) || !scatter || !surface) throw new AssertionError("Missing 3D geometry.");
        return firstBar;
    }

    public static void main(String[] args) {
        var context = ImGui.createContext();
        try {
            var io = ImGui.getIO();
            io.setIniFilename(null);
            io.setDisplaySize(1280, 900);
            io.setDeltaTime(1f / 60f);
            io.getFonts().getTexDataAsRGBA32(new ImInt(), new ImInt());
            var state = new StudioUI.UIState();
            StudioUI.applyStyle();
            for (int i = 0; i < 3; ++i) frame(state);
)JAVA";
  for (const auto& point : modelClicks) {
    const auto x = std::to_string(point.x), y = std::to_string(point.y), index = std::to_string(point.index);
    const auto key = CppCodegen::quote(std::string("node:") + point.id);
    cs += "        Click(" + x + ", " + y + ");\n        if (state.Get(" + key + ", -1) != " + index +
      ") throw new InvalidOperationException(\"Model selection failed: " + point.id + "\");\n";
    j += "            click(state, io, " + x + ", " + y + ");\n            if (state.get(" + key +
      ", () -> new ImInt(-1)).get() != " + index + ") throw new AssertionError(\"Model selection failed: " + point.id + "\");\n";
  }
  cs += R"CS(        var repeats = StudioUI.TemplateItems.FindAll(item => item.Node == "repeatChoice");
        var ids = new System.Collections.Generic.HashSet<uint>();
        foreach (var item in repeats) ids.Add(item.Id);
        if (repeats.Count != 4 || ids.Count != 4 ||
            repeats[0].Position.Y >= repeats[1].Position.Y)
            throw new InvalidOperationException("Expected four distinct repeater instances.");
        void ClickItem(string node)
        {
            int index = StudioUI.TemplateItems.FindIndex(item => item.Node == node);
            if (index < 0) throw new InvalidOperationException("Missing template item: " + node);
            var position = StudioUI.TemplateItems[index].Position;
            Click(position.X + 5, position.Y + 5);
        }
        ClickItem("repeatChoice");
        if (!state.Get("node:repeatChoice", false)) throw new InvalidOperationException("First instance did not respond.");
        Click(repeats[1].Position.X + 5, repeats[1].Position.Y + 5);
        if (state.Get("node:repeatChoice", true)) throw new InvalidOperationException("Second instance did not respond.");
        ClickItem("componentChoice");
        if (!state.Get("node:componentChoice", false)) throw new InvalidOperationException("Component child did not respond.");
        if (state.Get("node:templateLoader", false) || StudioUI.TemplateItems.Exists(item => item.Node == "loadedChoice"))
            throw new InvalidOperationException("Loader started loaded.");
        ClickItem("templateLoader");
        if (!state.Get("node:templateLoader", false)) throw new InvalidOperationException("Loader did not load.");
        ClickItem("loadedChoice");
        if (!state.Get("node:loadedChoice", false)) throw new InvalidOperationException("Loaded child did not respond.");
        ClickItem("templateLoader");
        if (state.Get("node:templateLoader", true) || !state.Get("node:loadedChoice", false))
            throw new InvalidOperationException("Loader did not unload while retaining state.");
        float ChartTop(uint accent)
        {
            var data = ImGui.GetDrawData();
            float top = float.PositiveInfinity;
            for (int list = 0; list < data.CmdListsCount; ++list)
            {
                var vertices = data.CmdLists[list].VtxBuffer;
                for (int index = 0; index < vertices.Size; ++index)
                {
                    var vertex = vertices[index];
                    if (vertex.col == accent) top = Math.Min(top, vertex.pos.Y);
                }
            }
            return top;
        }
        for (int mode = 0; mode < 6; ++mode)
        {
            uint color = ImGui.GetColorU32(new Vector4(1, 0, (mode + 1) / 16f, 1));
            if (!float.IsFinite(ChartTop(color))) throw new InvalidOperationException("Chart mode did not render.");
        }
        var line = state.Get("node:chart_line", Array.Empty<float>());
        if (line.Length != 4 || line[0] != -2 || state.Get<float[]?>("node:emptyChart", null)?.Length != 0)
            throw new InvalidOperationException("Chart data was not initialized from the project.");
        uint lineColor = ImGui.GetColorU32(new Vector4(1, 0, 0.0625f, 1));
        float originalTop = ChartTop(lineColor);
        line[0] = 100;
        Frame();
        if (!(ChartTop(lineColor) + 12 < originalTop))
            throw new InvalidOperationException("Chart geometry did not respond to a changed value.");
        var peaks = new float[800];
        peaks[274] = 100;
        peaks[611] = -100;
        state.Set("node:chart_line", peaks);
        Frame();
        if (!(ChartTop(lineColor) + 12 < originalTop))
            throw new InvalidOperationException("Chart sampling lost the peak.");

        uint barsColor = ImGui.GetColorU32(new Vector4(0, 1, 1, 1));
        uint scatterColor = ImGui.GetColorU32(new Vector4(1, 0, 1, 1));
        float Chart3dMesh()
        {
            var data = ImGui.GetDrawData();
            float firstBar = float.PositiveInfinity;
            bool scatter = false, surface = false;
            for (int list = 0; list < data.CmdListsCount; ++list)
            {
                var vertices = data.CmdLists[list].VtxBuffer;
                for (int index = 0; index < vertices.Size; ++index)
                {
                    var vertex = vertices[index];
                    if (vertex.pos.X < 1100 || vertex.pos.Y < 420) continue;
                    if (vertex.col == barsColor) firstBar = Math.Min(firstBar, vertex.pos.X);
                    scatter |= vertex.col == scatterColor;
                    surface |= vertex.pos.Y > 742 && (vertex.col & 0x00ffff00u) == 0 && (vertex.col & 0xffu) > 60;
                }
            }
            if (!float.IsFinite(firstBar) || !scatter || !surface)
                throw new InvalidOperationException("A 3D chart mode did not render.");
            return firstBar;
        }
        var bars = state.Get<StudioUI.Chart3dState?>("node:chart3d_bars", null);
        if (bars == null || bars.Elevation != 33 || bars.Azimuth != -40 || Math.Abs(bars.Values[0] - 0.35f) > 1e-6f ||
            state.Get<StudioUI.Chart3dState?>("node:chart3d_empty", null)?.Values.Length != 0)
            throw new InvalidOperationException("3D chart state did not load.");
        float originalX = Chart3dMesh();
        io.AddMousePosEvent(1190, 510); Frame();
        io.AddMouseButtonEvent(0, true); Frame();
        io.AddMousePosEvent(1240, 550); Frame();
        io.AddMouseButtonEvent(0, false); Frame();
        if (Math.Abs(bars.Elevation - 53) >= 0.01f || Math.Abs(bars.Azimuth + 10) >= 0.01f ||
            Math.Abs(Chart3dMesh() - originalX) <= 1)
            throw new InvalidOperationException("3D chart did not respond to dragging.");
        string[] effects = ["pulse_alpha", "bounce_y", "slide_x", "scale_pulse", "color_shift"];
        var slide = state.Get<StudioUI.AnimationState?>("node:slide_x", null)!;
        float neutralX = StudioUI.AnimationItems["slide_x"].Position.X;
        if (slide.Running || slide.Start.HasValue) throw new InvalidOperationException("Animation initial state.");
        Click(1119, 192);
        if (!slide.Running) throw new InvalidOperationException("Animation play button.");
        foreach (string effect in effects)
            if (effect != "slide_x") state.Get<StudioUI.AnimationState?>("node:" + effect, null)!.Start = null;
        Frame();
        var dim = StudioUI.AnimationItems["pulse_alpha"];
        var bounce = StudioUI.AnimationItems["bounce_y"];
        var scale = StudioUI.AnimationItems["scale_pulse"];
        var hue = StudioUI.AnimationItems["color_shift"];
        float startX = StudioUI.AnimationItems["slide_x"].Position.X;
        io.DeltaTime = 0.28f;
        Frame();
        if (StudioUI.AnimationItems["pulse_alpha"].Alpha <= dim.Alpha + 0.4f ||
            StudioUI.AnimationItems["bounce_y"].Position.Y >= bounce.Position.Y - 5 ||
            StudioUI.AnimationItems["scale_pulse"].Width <= scale.Width + 6 ||
            StudioUI.AnimationItems["scale_pulse"].Font <= scale.Font ||
            StudioUI.AnimationItems["color_shift"].Color == hue.Color)
            throw new InvalidOperationException("Animation effects did not update the rendered controls.");
        io.DeltaTime = 0.7f;
        Frame();
        io.DeltaTime = 1f / 60f;
        var moved = StudioUI.AnimationItems["slide_x"].Position;
        if (moved.X <= startX + 10 || moved.X <= neutralX || state.Get("node:slide_x_child", false))
            throw new InvalidOperationException("Animation slide positions.");
        Click(moved.X + 6, moved.Y + 7);
        if (!state.Get("node:slide_x_child", false) || StudioUI.AnimationItems["slide_x"].Position.X != moved.X)
            throw new InvalidOperationException("Animated control did not receive the click.");
        Click(1119, 192);
        if (slide.Running || slide.Start.HasValue || StudioUI.AnimationItems["slide_x"].Position.X != neutralX)
            throw new InvalidOperationException("Animation did not pause.");
        int uploads = 0;
        nint Upload(byte[] bytes, int width, int height)
        {
            ++uploads;
            if (bytes[0] == 0x89 && bytes.Length == 74 && width == 2 && height == 1) return 900;
            if (bytes[0] == 0xff && bytes[1] == 0xd8 && bytes.Length == 202 && width == 2 && height == 2) return 901;
            throw new InvalidOperationException("Unexpected image resource.");
        }
        if (!StudioUI.LoadImages(state, Upload) || !StudioUI.LoadImages(state, Upload) || uploads != 2)
            throw new InvalidOperationException("Image loading and reuse.");
        Frame();
        int pngs = 0, jpegs = 0;
        var imageDraw = ImGui.GetDrawData();
        for (int list = 0; list < imageDraw.CmdListsCount; ++list)
        {
            var commands = imageDraw.CmdLists[list].CmdBuffer;
            for (int command = 0; command < commands.Size; ++command)
            {
                var cmd = commands[command];
                nint id = cmd.TextureId;
                if (id == 900)
                {
                    var draw = imageDraw.CmdLists[list];
                    int vertex = checked((int)cmd.VtxOffset + draw.IdxBuffer[(int)cmd.IdxOffset]);
                    if (cmd.ElemCount != 6 || draw.VtxBuffer[vertex].pos.Y != 845)
                        throw new InvalidOperationException("PNG aspect ratio.");
                    ++pngs;
                }
                if (id == 901) ++jpegs;
            }
        }
        if (pngs != 2 || jpegs != 1) throw new InvalidOperationException("Embedded image draw calls.");
        Click(170, 850);
        if (!state.Get("node:image_action", false)) throw new InvalidOperationException("Image button click.");
        Frame();
        if (state.Get("node:image_action", false)) throw new InvalidOperationException("Image button state reset.");
        var released = new System.Collections.Generic.List<nint>();
        StudioUI.ReleaseImages(state, (texture, width, height) => released.Add(texture));
        StudioUI.ReleaseImages(state, (texture, width, height) => released.Add(texture));
        if (released.Count != 2 || !released.Contains(900) || !released.Contains(901))
            throw new InvalidOperationException("Image cleanup.");
        foreach (var (side, x) in new (string, float)[] { ("left", 56), ("center", 214), ("right", 371), ("free", 290) })
        {
            string key = "node:title_" + side;
            Click(x, 813);
            if (!state.Get(key, false)) throw new InvalidOperationException("Title click " + side);
            Frame();
            if (state.Get(key, false)) throw new InvalidOperationException("Title click reset " + side);
        }
        ImGui.SetWindowCollapsed("###studio_656d6265646465645f696d61676573", true, ImGuiCond.Always);
        Frame();
        Click(290, 813);
        if (!state.Get("node:title_free", false)) throw new InvalidOperationException("Collapsed title click.");
        state.Set("node:embedded_images", false);
        Frame();
        if (state.Get("node:title_free", false)) throw new InvalidOperationException("Closed title state.");
        Console.WriteLine("Model selection, templates, charts, animations and images verified.");
)CS";
  j += R"JAVA(            var repeats = StudioUI.templateItems.stream().filter(item -> item.node().equals("repeatChoice")).toList();
            if (repeats.size() != 4 || repeats.stream().map(StudioUI.TemplateItem::id).distinct().count() != 4 ||
                repeats.get(0).y() >= repeats.get(1).y()) throw new AssertionError("Expected four distinct instances.");
            clickItem(state, io, "repeatChoice");
            if (!state.get("node:repeatChoice", ImBoolean::new).get()) throw new AssertionError("First instance.");
            var second = repeats.get(1);
            click(state, io, second.x() + 5, second.y() + 5);
            if (state.get("node:repeatChoice", ImBoolean::new).get()) throw new AssertionError("Second instance.");
            clickItem(state, io, "componentChoice");
            if (!state.get("node:componentChoice", ImBoolean::new).get()) throw new AssertionError("Component.");
            var loaded = state.get("node:templateLoader", ImBoolean::new);
            if (loaded.get() || StudioUI.templateItems.stream().anyMatch(item -> item.node().equals("loadedChoice")))
                throw new AssertionError("Loader started loaded.");
            clickItem(state, io, "templateLoader");
            if (!loaded.get()) throw new AssertionError("Loader did not load.");
            clickItem(state, io, "loadedChoice");
            var child = state.get("node:loadedChoice", ImBoolean::new);
            if (!child.get()) throw new AssertionError("Loaded child.");
            clickItem(state, io, "templateLoader");
            if (loaded.get() || !child.get()) throw new AssertionError("Loader did not retain the child value.");
            for (int mode = 0; mode < 6; ++mode) {
                int accent = ImGui.getColorU32(1f, 0f, (mode + 1) / 16f, 1f);
                if (!Float.isFinite(chartTop(accent))) throw new AssertionError("Chart mode did not render.");
            }
            var line = state.get("node:chart_line", () -> new float[0]);
            if (line.length != 4 || line[0] != -2 ||
                state.get("node:emptyChart", () -> new float[] {1}).length != 0)
                throw new AssertionError("Chart values did not load.");
            int lineAccent = ImGui.getColorU32(1f, 0f, 0.0625f, 1f);
            float originalTop = chartTop(lineAccent);
            line[0] = 100;
            frame(state);
            if (!(chartTop(lineAccent) + 12 < originalTop)) throw new AssertionError("Chart redraw.");
            var bars = state.<StudioUI.Chart3dState>get("node:chart3d_bars", () -> null);
            var empty = state.<StudioUI.Chart3dState>get("node:chart3d_empty", () -> null);
            if (bars == null || bars.elevation != 33 || bars.azimuth != -40 || Math.abs(bars.values[0] - 0.35f) > 1e-6f ||
                empty == null || empty.values.length != 0) throw new AssertionError("3D chart state.");
            float originalX = chart3dMesh();
            io.addMousePosEvent(1190, 510); frame(state);
            io.addMouseButtonEvent(0, true); frame(state);
            io.addMousePosEvent(1240, 550); frame(state);
            io.addMouseButtonEvent(0, false); frame(state);
            if (Math.abs(bars.elevation - 53) >= 0.01f || Math.abs(bars.azimuth + 10) >= 0.01f ||
                Math.abs(chart3dMesh() - originalX) <= 1) throw new AssertionError("3D chart drag.");
            String[] effects = {"pulse_alpha", "bounce_y", "slide_x", "scale_pulse", "color_shift"};
            var slide = state.<StudioUI.AnimationState>get("node:slide_x", () -> null);
            float neutralX = StudioUI.animationItems.get("slide_x").x();
            if (slide.running || slide.start >= 0) throw new AssertionError("Animation initial state.");
            click(state, io, 1119, 192);
            if (!slide.running) throw new AssertionError("Animation play button.");
            for (String effect : effects)
                if (!effect.equals("slide_x")) state.<StudioUI.AnimationState>get("node:" + effect, () -> null).start = -1;
            frame(state);
            var dim = StudioUI.animationItems.get("pulse_alpha");
            var bounce = StudioUI.animationItems.get("bounce_y");
            var size = StudioUI.animationItems.get("scale_pulse");
            var hue = StudioUI.animationItems.get("color_shift");
            float startX = StudioUI.animationItems.get("slide_x").x();
            io.setDeltaTime(0.28f);
            frame(state);
            if (StudioUI.animationItems.get("pulse_alpha").alpha() <= dim.alpha() + 0.4f ||
                StudioUI.animationItems.get("bounce_y").y() >= bounce.y() - 5 ||
                StudioUI.animationItems.get("scale_pulse").width() <= size.width() + 6 ||
                StudioUI.animationItems.get("scale_pulse").font() <= size.font() ||
                StudioUI.animationItems.get("color_shift").color() == hue.color())
                throw new AssertionError("Animation effects did not update the rendered controls.");
            io.setDeltaTime(0.7f);
            frame(state);
            io.setDeltaTime(1f / 60);
            var moved = StudioUI.animationItems.get("slide_x");
            var check = state.get("node:slide_x_child", () -> new ImBoolean(false));
            if (moved.x() <= startX + 10 || moved.x() <= neutralX || check.get())
                throw new AssertionError("Animation slide positions.");
            click(state, io, moved.x() + 6, moved.y() + 7);
            if (!check.get() || StudioUI.animationItems.get("slide_x").x() != moved.x())
                throw new AssertionError("Animated control click.");
            click(state, io, 1119, 192);
            if (slide.running || slide.start >= 0 || StudioUI.animationItems.get("slide_x").x() != neutralX)
                throw new AssertionError("Animation pause.");
            int[] uploads = {0};
            StudioUI.ImageUpload upload = (bytes, width, height) -> {
                ++uploads[0];
                if (bytes.length == 74 && (bytes[0] & 255) == 0x89 && width == 2 && height == 1) return 900;
                if (bytes.length == 202 && (bytes[0] & 255) == 0xff && width == 2 && height == 2) return 901;
                throw new AssertionError("Unexpected image resource.");
            };
            if (!StudioUI.loadImages(state, upload) || !StudioUI.loadImages(state, upload) || uploads[0] != 2)
                throw new AssertionError("Image loading and reuse.");
            frame(state);
            var images = ImGui.getDrawData();
            int pngs = 0, jpegs = 0;
            for (int list = 0; list < images.getCmdListsCount(); ++list) {
                for (int command = 0; command < images.getCmdListCmdBufferSize(list); ++command) {
                    long id = images.getCmdListCmdBufferTextureId(list, command);
                    if (id == 900) {
                        int indexSize = imgui.ImDrawData.sizeOfImDrawIdx();
                        int offset = images.getCmdListCmdBufferIdxOffset(list, command) * indexSize;
                        var indices = images.getCmdListIdxBufferData(list).order(java.nio.ByteOrder.nativeOrder());
                        int vertex = (indexSize == 2 ? Short.toUnsignedInt(indices.getShort(offset)) : indices.getInt(offset))
                            + images.getCmdListCmdBufferVtxOffset(list, command);
                        var vertices = images.getCmdListVtxBufferData(list).order(java.nio.ByteOrder.nativeOrder());
                        if (images.getCmdListCmdBufferElemCount(list, command) != 6 ||
                            vertices.getFloat(vertex * imgui.ImDrawData.sizeOfImDrawVert() + 4) != 845)
                            throw new AssertionError("PNG aspect ratio.");
                        ++pngs;
                    }
                    if (id == 901) ++jpegs;
                }
            }
            if (pngs != 2 || jpegs != 1) throw new AssertionError("Embedded image draw calls.");
            var imageButton = state.get("node:image_action", ImBoolean::new);
            click(state, io, 170, 850);
            if (!imageButton.get()) throw new AssertionError("Image button click.");
            frame(state);
            if (imageButton.get()) throw new AssertionError("Image button state reset.");
            var released = new java.util.HashSet<Long>();
            StudioUI.ImageRelease release = (id, width, height) -> {
                if (!released.add(id)) throw new AssertionError("Texture released twice.");
            };
            StudioUI.releaseImages(state, release);
            StudioUI.releaseImages(state, release);
            if (!released.equals(java.util.Set.of(900L, 901L))) throw new AssertionError("Image cleanup.");
            var titleClicks = java.util.Map.of("left", 56, "center", 214, "right", 371, "free", 290);
            for (var title : titleClicks.entrySet()) {
                var pressed = state.get("node:title_" + title.getKey(), ImBoolean::new);
                click(state, io, title.getValue(), 813);
                if (!pressed.get()) throw new AssertionError("Title click " + title.getKey());
                frame(state);
                if (pressed.get()) throw new AssertionError("Title click reset " + title.getKey());
            }
            var titleWindow = imgui.internal.ImGui.findWindowByName("###studio_656d6265646465645f696d61676573");
            imgui.internal.ImGui.setWindowCollapsed(titleWindow, true, imgui.flag.ImGuiCond.Always);
            frame(state);
            click(state, io, 290, 813);
            var freeTitle = state.get("node:title_free", ImBoolean::new);
            if (!freeTitle.get()) throw new AssertionError("Collapsed title click.");
            state.get("node:embedded_images", ImBoolean::new).set(false);
            frame(state);
            if (freeTitle.get()) throw new AssertionError("Closed title state.");
            System.out.println("Java model selection, templates, charts, animations and images verified.");
        } finally { ImGui.destroyContext(context); }
    }
}
)JAVA";
  auto program = csharp.files.at("Program.cs");
  const auto csInsertion = program.find("        Console.WriteLine(\"ImGui.NET rendered three frames in memory.");
  assert(csInsertion != std::string::npos);
  program.insert(csInsertion, cs);
  write(directory / "csharp" / "Program.cs", program);

  auto ui = csharp.files.at("StudioUI.cs");
  const std::string classOpen = "public static class StudioUI\n{\n";
  const auto classStart = ui.find(classOpen);
  assert(classStart != std::string::npos);
  ui.insert(classStart + classOpen.size(),
            "    internal static List<(string Node, Vector2 Position, uint Id)> TemplateItems { get; } = new();\n");
  ui.insert(classStart + classOpen.size(),
            "    internal static Dictionary<string, (Vector2 Position, float Width, float Alpha, uint Color, float Font)> AnimationItems { get; } = new();\n");
  for (const std::string effect : {"pulse_alpha", "bounce_y", "slide_x", "scale_pulse", "color_shift"}) {
    const auto function = ui.find("private static void Draw_" + CppCodegen::hexId(effect + "_child") + "(");
    assert(function != std::string::npos);
    const auto item = ui.find("        ImGui.PopID();", function);
    assert(item != std::string::npos);
    ui.insert(item, "        AnimationItems[" + CppCodegen::quote(effect) +
                    "] = (ImGui.GetItemRectMin(), ImGui.GetItemRectSize().X, ImGui.GetStyle().Alpha, "
                    "ImGui.GetColorU32(ImGuiCol.Button), ImGui.GetFontSize());\n");
  }
  for (const auto* node : {"repeatChoice", "componentChoice", "loadedChoice", "templateLoader"}) {
    const auto function = ui.find("private static void Draw_" + CppCodegen::hexId(node) + "(UIState state)");
    assert(function != std::string::npos);
    const bool loader = std::string_view(node) == "templateLoader";
    const auto item = ui.find(loader ? "            ImGui.SameLine();" : "        ImGui.PopID();", function);
    assert(item != std::string::npos);
    const auto id = loader ? std::string("0") : "ImGui.GetID(\"###studio_" + CppCodegen::hexId(node) + "\")";
    ui.insert(item, std::string(loader ? "            " : "        ") + "TemplateItems.Add((" +
                   CppCodegen::quote(node) + ", ImGui.GetItemRectMin(), " + id + "));\n");
  }
  write(directory / "csharp" / "StudioUI.cs", ui);
  write(directory / "java" / "ModelInteraction.java", j);
  auto javaUi = java.files.at("StudioUI.java");
  const std::string javaClass = "public final class StudioUI {\n";
  const auto javaClassStart = javaUi.find(javaClass);
  assert(javaClassStart != std::string::npos);
  javaUi.insert(javaClassStart + javaClass.size(), R"JAVA(    record TemplateItem(String node, float x, float y, int id) {}
    static final java.util.ArrayList<TemplateItem> templateItems = new java.util.ArrayList<>();
    private static void recordTemplateItem(String node, int id) {
        var position = ImGui.getItemRectMin();
        templateItems.add(new TemplateItem(node, position.x, position.y, id));
    }
    record AnimationItem(float x, float y, float width, float alpha, int color, int font) {}
    static final java.util.Map<String, AnimationItem> animationItems = new java.util.HashMap<>();
    private static void recordAnimationItem(String effect) {
        var position = ImGui.getItemRectMin();
        animationItems.put(effect, new AnimationItem(position.x, position.y, ImGui.getItemRectSizeX(),
            ImGui.getStyle().getAlpha(), ImGui.getColorU32(ImGuiCol.Button), ImGui.getFontSize()));
    }
)JAVA");
  for (const std::string effect : {"pulse_alpha", "bounce_y", "slide_x", "scale_pulse", "color_shift"}) {
    const auto function = javaUi.find("private static void Draw_" + CppCodegen::hexId(effect + "_child") + "(");
    assert(function != std::string::npos);
    const auto item = javaUi.find("        ImGui.popID();", function);
    assert(item != std::string::npos);
    javaUi.insert(item, "        recordAnimationItem(" + CppCodegen::quote(effect) + ");\n");
  }
  for (const auto* node : {"repeatChoice", "componentChoice", "loadedChoice", "templateLoader"}) {
    const auto function = javaUi.find("private static void Draw_" + CppCodegen::hexId(node) + "(UIState state)");
    assert(function != std::string::npos);
    const bool loader = std::string_view(node) == "templateLoader";
    const auto item = javaUi.find(loader ? "            ImGui.sameLine();" : "        ImGui.popID();", function);
    assert(item != std::string::npos);
    const auto id = loader ? std::string("0") : "ImGui.getID(\"###studio_" + CppCodegen::hexId(node) + "\")";
    javaUi.insert(item, std::string(loader ? "            " : "        ") + "recordTemplateItem(" +
                   CppCodegen::quote(node) + ", " + id + ");\n");
  }
  write(directory / "java" / "StudioUI.java", javaUi);
}
static std::string chromeInteractions(const StudioProject& project, const GeneratedCpp& generated, bool cApi) {
  const auto suffix = [&](const ImGuiNode& node) {
    return "###studio_" + CppCodegen::hexId(node.id) + "_" + CppCodegen::hexId(node.imguiId);
  };
  const std::string gui = cApi ? "ig" : "ImGui::";
  const std::string frame = gui + "NewFrame(); RenderStudioUI(" + (cApi ? "&state" : "state") + "); " + gui + "Render();";
  const std::string clicks = cApi ? "click(&state, " : "click(";
  const auto state = [&](const char* key) { return "state." + generated.variables.at(key); };
  std::string code;
  if (!cApi) code += "    auto click = [&](float x, float y) {\n"
    "        io.AddMousePosEvent(x,y); " + frame + "\n"
    "        io.AddMouseButtonEvent(0,true); " + frame + "\n"
    "        io.AddMouseButtonEvent(0,false); " + frame + "\n"
    "    };\n";
  code += "    ImGuiWindow* chrome = " + gui + "FindWindowByName(" + CppCodegen::quote(suffix(project.nodes[0])) + ");\n";
  code += "    ImGuiWindow* plain = " + gui + "FindWindowByName(" +
    CppCodegen::quote(project.nodes[1].label + suffix(project.nodes[1])) + ");\n";
  code += R"TEST(    assert(chrome && plain && chrome->TitleBarHeight == 44 && plain->TitleBarHeight == 0);
    assert(chrome->Flags & ImGuiWindowFlags_NoCollapse);
    assert(!(chrome->Flags & ImGuiWindowFlags_MenuBar));
    assert(chrome->WindowPadding.x == 17 && chrome->WindowPadding.y == 19 && chrome->WindowRounding == 9);
    assert(plain->WindowPadding.x == 10 && plain->WindowPadding.y == 10 && plain->WindowRounding == 6);
    bool separator = false, title_fill = false;
    for (int i = 0; i < chrome->DrawList->VtxBuffer.Size; ++i) {
        const ImDrawVert v = chrome->DrawList->VtxBuffer.Data[i];
        if (v.pos.y >= 40 && v.pos.y <= 85 && v.pos.x >= 61 && v.pos.x <= 629) {
            separator |= v.col == 0xffb04cf0;
            title_fill |= v.col == 0xff734c28;
        }
    }
    assert(separator && title_fill);
)TEST";
  code += "    " + clicks + "85, 62);\n";
  code += "    ImGuiWindow* menu = " + gui + "FindWindowByName(\"##Menu_00\");\n"
    "    assert(menu && menu->Active && (menu->Flags & ImGuiWindowFlags_Popup));\n";
  code += "    " + clicks + "600, 160);\n";
  code += "    " + clicks + "312, 62);\n    assert(" + state("chrome_checkbox") + ");\n";
  code += "    " + clicks + "498, 62);\n    assert(" + state("chrome_action") + ");\n";
  code += R"TEST(    bool colored_check = false, colored_hover = false;
    for (int i = 0; i < chrome->DrawList->VtxBuffer.Size; ++i) {
        const ImDrawVert v = chrome->DrawList->VtxBuffer.Data[i];
        if (v.pos.y >= 40 && v.pos.y <= 85) {
            colored_check |= v.col == 0xffa0ff66;
            colored_hover |= v.col == 0xffdd5588;
        }
    }
    assert(colored_check && colored_hover);
)TEST";
  code += "    " + clicks + "218, 62);\n";
  code += cApi ? "    ImGuiIO_AddInputCharactersUTF8(io, \"busca\");\n" : "    io.AddInputCharactersUTF8(\"busca\");\n";
  code += "    " + frame + "\n";
  code += cApi ? "    assert(strcmp(" + state("chrome_query") + ", \"busca\") == 0);\n"
              : "    assert(" + state("chrome_query") + " == \"busca\");\n";
  code += "    " + clicks + "600, 62);\n    " + frame + "\n    assert(chrome->Size.y >= 44 && chrome->Size.y <= 54);\n";
  code += "    " + clicks + "600, 62);\n    " + frame + "\n    assert(chrome->Size.y == 280);\n";
  code += "    " + clicks + "617, 62);\n    " + frame + "\n    assert(chrome->Size.x == 1400 && chrome->Pos.x == 0);\n";
  code += "    " + clicks + "1357, 22);\n    " + frame + "\n    assert(chrome->Size.x == 620 && chrome->Pos.x == 40);\n";
  code += "    " + clicks + "586, 62);\n    assert(!" + state("chrome") + ");\n";
  return code;
}

static void generateCase(const fs::path& directory, const StudioProject& project, bool edge, bool chrome = false) {
  fs::create_directories(directory);
  const auto original = projectToJson(project);
  const auto header = generateCppDetailed(project,CppEmitMode::Header);
  const auto function = generateCppDetailed(project,CppEmitMode::Function);
  const auto full = generateCppDetailed(project,CppEmitMode::Full);
  assert(header.error.empty() && function.error.empty() && full.error.empty());
  assert(projectToJson(project) == original);
  assert(header.nodeLines.size() > 0 || project.nodes.empty());
  if (chrome) assert(header.complete());
  write(directory / "ui.hpp",header.code); write(directory / "function.hpp",function.code);
  write(directory / "full.cpp",full.code);
  write(directory / "peer.cpp","#include \"ui.hpp\"\nvoid peer() { " + header.stateType + " state; (void)state; }\n");
  std::string assertions, interaction;
  if (edge) {
    const auto& v = header.variables;
    assert(v.at("radio1") == v.at("radio2") && v.at("radio1") != v.at("radio3"));
    assert(!v.count("hidden"));
    std::set<std::string> names;
    for (const auto& item : v) if (item.first != "radio2") {
      assert(item.second.find("__") == std::string::npos); assert(names.insert(item.second).second);
    }
    assert(!header.diagnostics.empty());
    assertions = "assert(state." + v.at("bool") + "); assert(state." + v.at("float") + " == 48.0f);\n"
      "assert(state." + v.at("text") + ".size() == " + std::to_string(project.nodes[0].children[4].props.text.size()) + ");\n"
      "assert(state." + v.at("combo") + " == 1); assert(state." + v.at("plot") + "[2] == 8.0f);\n";
    const auto& root = project.nodes.front();
    const auto title = root.label + "###studio_" + CppCodegen::hexId(root.id) + "_" + CppCodegen::hexId(root.imguiId);
    interaction = "auto* window = ImGui::FindWindowByName(" + CppCodegen::quote(title) + "); assert(window);\n" +
      chartInteractions(header, false) +
      "ImGuiWindow *wrappedFlow = nullptr, *wideFlow = nullptr;\n"
      "for (auto* child : window->DC.ChildWindows) {\n"
      "  if (child->Size.x == 220.0f) wrappedFlow = child;\n"
      "  if (child->Size.x == 340.0f) wideFlow = child;\n"
      "}\n"
      "assert(wrappedFlow && wideFlow);\n"
      "assert(wrappedFlow->ContentSize.y == 68.0f && wideFlow->ContentSize.y == 30.0f);\n"
      "io.AddMousePosEvent(window->DC.CursorStartPos.x+5, window->DC.CursorStartPos.y+5);\n"
      "ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();\n"
      "io.AddMouseButtonEvent(0,true); ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();\n"
      "io.AddMouseButtonEvent(0,false); ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();\n"
      "assert(!state." + v.at("bool") + ");\n"
      "auto click = [&](float x, float y) {\n"
      "    io.AddMousePosEvent(x, y); ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();\n"
      "    io.AddMouseButtonEvent(0, true); ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();\n"
      "    io.AddMouseButtonEvent(0, false); ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render();\n"
      "};\n" + modelInteractions(header, false) + chart3dInteractions(project, header, false) +
      animationInteractions(project, header, false) + imageInteractions(header, false) + titleInteractions(header, false);
  }
  if (chrome) interaction = chromeInteractions(project, header, false);
  // Preinclude dependencies before a namespace to compile Function mode alongside Header mode.
  const std::string runner = "#include \"ui.hpp\"\n#include \"imgui_internal.h\"\n#include <cassert>\nnamespace FunctionCase {\n#include \"function.hpp\"\n}\nvoid peer();\n" +
    std::string(edge ? imageTestCallbacks : "") + "int main() {\n"
    "peer(); IMGUI_CHECKVERSION(); ImGui::CreateContext(); auto& io=ImGui::GetIO();\n"
    "io.IniFilename=nullptr; io.DisplaySize=ImVec2(1400,1000); io.DeltaTime=1.0f/60.0f;\n"
    "unsigned char* pixels; int w,h; io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);\n"
    + header.stateType + " state; FunctionCase::" + header.stateType + " second;\n" + assertions +
    (edge ? loadTestImages(false) : "") +
    "ApplyStudioStyle(); for(int i=0;i<3;++i) { ImGui::NewFrame(); RenderStudioUI(state); ImGui::Render(); }\n"
    + interaction + "FunctionCase::ApplyStudioStyle(); for(int i=0;i<3;++i) { ImGui::NewFrame(); FunctionCase::RenderStudioUI(second); ImGui::Render(); }\n"
    "ImGui::DestroyContext(); }\n";
  write(directory / "runner.cpp",runner);
  const auto python = generatePythonCode(project);
  assert(python.error.empty() && !python.code.empty());
  if (chrome) assert(python.complete());
  assert(python.nodeLines.size() == header.nodeLines.size());
  assert(projectToJson(project) == original);
  write(directory / "main.py",python.code);
  const auto csharp = generateCSharpBundle(project);
  assert(csharp.generated.error.empty() && csharp.generated.nodeLines.size() == header.nodeLines.size());
  if (chrome) assert(csharp.generated.complete());
  if (fs::exists(directory / "csharp")) {
    // Build generation is repeatable while app exports still require a new folder.
    for (const auto& file : csharp.files) write(directory / "csharp" / file.first,file.second);
  } else {
    assert(exportCSharpBundle(project,directory / "csharp"));
  }
  assert(!exportCSharpBundle(project,directory / "csharp"));
  auto javaProject = project;
  if (edge) {
    auto longText = createDefaultNode(WidgetType::Text);
    longText.props.text = std::string(15999,'x')+"\xf0\x9f\x98\x80"+std::string(50000,'y')+"\\u000a";
    javaProject.nodes.front().children.push_back(longText);
  }
  const auto java = generateJavaBundle(javaProject);
  assert(java.generated.error.empty() && java.generated.nodeLines.size() == header.nodeLines.size()+(edge?1:0));
  if (chrome) assert(java.generated.complete());
  if (fs::exists(directory / "java")) {
    for (const auto& file : java.files) write(directory / "java" / file.first,file.second);
  } else {
    assert(exportJavaBundle(javaProject,directory / "java"));
  }
  assert(!exportJavaBundle(project,directory / "java"));
  if (edge) writeManagedInteractionTests(directory, csharp, java);
  if (chrome) {
    const auto insertAfter = [](std::string& source, std::string_view anchor, const std::string& code) {
      const auto start = source.find(anchor);
      assert(start != std::string::npos);
      const auto end = source.find('\n', start);
      assert(end != std::string::npos);
      source.insert(end + 1, code);
    };
    auto csUi = csharp.files.at("StudioUI.cs");
    insertAfter(csUi, "bool visible = ImGui.Begin(\"###studio_6368726f6d65\", flags);",
      "        WindowChromeCheck.Position = ImGui.GetWindowPos();\n"
      "        WindowChromeCheck.Size = ImGui.GetWindowSize();\n"
      "        WindowChromeCheck.Padding = ImGui.GetStyle().WindowPadding;\n"
      "        WindowChromeCheck.Height = ImGui.GetFrameHeight();\n");
    insertAfter(csUi, "if (ImGui.BeginPopup(\"##flyout\")) {", "                    WindowChromeCheck.MenuOpened = true;\n");
    auto csProgram = csharp.files.at("Program.cs");
    insertAfter(csProgram, "Console.WriteLine(\"ImGui.NET rendered three frames in memory.\");",
      "        WindowChromeCheck.Run(state);\n");
    write(directory / "csharp" / "StudioUI.cs", csUi);
    write(directory / "csharp" / "Program.cs", csProgram);
    write(directory / "csharp" / "WindowChromeCheck.cs", StudioChromeTests::csharp);
    auto javaUi = java.files.at("StudioUI.java");
    insertAfter(javaUi, "boolean visible = ImGui.begin(\"###studio_6368726f6d65\", flags);",
      "        WindowChromeCheck.position = ImGui.getWindowPos();\n"
      "        WindowChromeCheck.size = ImGui.getWindowSize();\n"
      "        WindowChromeCheck.padding = ImGui.getStyle().getWindowPadding();\n"
      "        WindowChromeCheck.height = ImGui.getFrameHeight();\n");
    insertAfter(javaUi, "if (ImGui.beginPopup(\"##flyout\")) {", "                    WindowChromeCheck.menuOpened = true;\n");
    write(directory / "java" / "StudioUI.java", javaUi);
    write(directory / "java" / "WindowChromeCheck.java", StudioChromeTests::java);
  }
  auto cProject = project;
  if (edge) {
    auto literal = createDefaultNode(WidgetType::Text);
    literal.props.text = "ImGui::Text ImVec2(1, 2) \xC3\xB1";
    cProject.nodes.front().children.push_back(literal);
  }
  const auto c = generateCCode(cProject);
  assert(c.error.empty() && c.nodeLines.size() == header.nodeLines.size()+(edge?1:0));
  if (chrome) assert(c.complete());
  if (edge) assert(c.code.find("\"ImGui::Text ImVec2(1, 2) \xC3\xB1\"") != std::string::npos);
  if (fs::exists(directory / "c_app")) {
    for (const auto& file : generateCBundle(cProject).files) write(directory / "c_app" / file.first,file.second);
  }
  else assert(exportCBundle(cProject,directory / "c_app"));
  if (edge || chrome) {
    const auto& root = project.nodes.front();
    const auto title = root.label + "###studio_" + CppCodegen::hexId(root.id) + "_" + CppCodegen::hexId(root.imguiId);
    write(directory / "c_app" / "model_interaction.c", "#include \"studio_ui.h\"\n#include <assert.h>\n#include <string.h>\n\n" +
      std::string(edge ? imageTestCallbacks : "") + "static void click(" + c.stateType + R"TEST(* state, float x, float y) {
    ImGuiIO* io = igGetIO();
    ImGuiIO_AddMousePosEvent(io, x, y); igNewFrame(); RenderStudioUI(state); igRender();
    ImGuiIO_AddMouseButtonEvent(io, 0, true); igNewFrame(); RenderStudioUI(state); igRender();
    ImGuiIO_AddMouseButtonEvent(io, 0, false); igNewFrame(); RenderStudioUI(state); igRender();
}

int main(void) {
    ImGuiContext* context = igCreateContext(NULL);
    ImGuiIO* io = igGetIO();
    io->IniFilename = NULL; io->DisplaySize = (ImVec2){1400, 1000}; io->DeltaTime = 1.0f / 60;
    unsigned char* pixels; int width, height;
    ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, NULL);
)TEST" + "    " + c.stateType + R"TEST( state;
    InitStudioState(&state);
)TEST" + (edge ? loadTestImages(true) : "") + R"TEST(    ApplyStudioStyle();
    for (int i = 0; i < 3; ++i) { igNewFrame(); RenderStudioUI(&state); igRender(); }
)TEST" + (edge ? "    ImGuiWindow* window = igFindWindowByName(" + CppCodegen::quote(title) + "); assert(window);\n" +
      chartInteractions(c, true) + modelInteractions(c, true) + chart3dInteractions(project, c, true) +
      animationInteractions(project, c, true) + imageInteractions(c, true) + titleInteractions(c, true) :
      chromeInteractions(project, c, true)) +
      "    igDestroyContext(context);\n    return 0;\n}\n");
  }
  assert(!exportCBundle(project,directory / "c_app"));
  const auto rust = generateRustBundle(project);
  assert(rust.generated.error.empty() && rust.generated.nodeLines.size() == header.nodeLines.size());
  if (chrome) assert(rust.generated.complete());
  if (edge) {
    const auto& variables = rust.generated.variables;
    assert(variables.at("radio1") == variables.at("radio2") && variables.at("radio1") != variables.at("radio3"));
    assert(variables.at("float") != variables.at("bool") && !variables.count("hidden"));
    assert(rust.generated.code.find("\\x00end") != std::string::npos);
    assert(rust.generated.code.find(".scale_min(-2.0f32).scale_max(9.0f32)") != std::string::npos);
    assert(!rust.generated.diagnostics.empty());
  }
  if (fs::exists(directory / "rust")) {
    for (const auto& file : rust.files) write(directory / "rust" / file.first, file.second);
  } else {
    assert(exportRustBundle(project, directory / "rust"));
  }
  if (edge) {
    std::string clicks;
    for (const auto& point : modelClicks) {
      clicks += "        click(&mut context, &mut state, [" + std::to_string(point.x) + ".0, " +
        std::to_string(point.y) + ".0]);\n" + "        assert_eq!(state." + rust.generated.variables.at(point.id) +
        ", " + std::to_string(point.index) + ");\n";
    }
    auto rustUi = rust.files.at("ui.rs");
    const std::string playAnchor = "        if !settings.title.is_empty()";
    const auto playItem = rustUi.find(playAnchor);
    assert(playItem != std::string::npos);
    rustUi.insert(playItem, "        #[cfg(test)]\n        if settings.effect == \"slide_x\" { layout_tests::record_animation(ui, \"play\"); }\n");
    for (const std::string effect : {"pulse_alpha", "bounce_y", "slide_x", "scale_pulse", "color_shift"}) {
      const auto marker = rustUi.find("let _id = ui.push_id(" + CppCodegen::quote(CppCodegen::hexId(effect + "_child")) + ");");
      assert(marker != std::string::npos);
      const auto control = rustUi.find(effect == "slide_x" ? "ui.checkbox(" : "ui.button_with_size(", marker);
      assert(control != std::string::npos);
      const auto item = rustUi.find('\n', control);
      assert(item != std::string::npos);
      rustUi.insert(item + 1, "#[cfg(test)]\nlayout_tests::record_animation(ui, " + CppCodegen::quote(effect) + ");\n");
    }
    for (const auto* node : {"repeatChoice", "componentChoice", "loadedChoice", "templateLoader"}) {
      const auto field = "state." + rust.generated.variables.at(node);
      const bool loader = std::string_view(node) == "templateLoader";
      const auto marker = rustUi.find(loader ? " = !" + field + ";" : "&mut " + field + ");");
      assert(marker != std::string::npos);
      const auto item = loader ? rustUi.find("ui.same_line();", marker) : rustUi.find('\n', marker) + 1;
      assert(item != std::string::npos && item > marker);
      rustUi.insert(item, "#[cfg(test)]\nlayout_tests::record_template_item(ui, " + CppCodegen::quote(node) + ");\n");
    }
    const auto check = [&](const std::string& node, bool expected) {
      return "        assert_eq!(state." + rust.generated.variables.at(node) + ", " + CppCodegen::boolean(expected) + ");\n";
    };
    clicks += R"RS(        let repeats = items("repeatChoice");
        assert_eq!(repeats.len(), 4);
        assert_eq!(repeats.iter().map(|item| item.1).collect::<std::collections::HashSet<_>>().len(), 4);
        assert!(repeats[0].0[1] < repeats[1].0[1]);
        assert_eq!(state.template_heights.len(), 4);
        click(&mut context, &mut state, target(repeats[0].0));
)RS";
    clicks += check("repeatChoice", true);
    clicks += "        click(&mut context, &mut state, target(repeats[1].0));\n" + check("repeatChoice", false);
    clicks += "        click(&mut context, &mut state, target(items(\"componentChoice\")[0].0));\n" + check("componentChoice", true);
    clicks += check("templateLoader", false) + "        assert!(items(\"loadedChoice\").is_empty());\n";
    clicks += "        click(&mut context, &mut state, target(items(\"templateLoader\")[0].0));\n" + check("templateLoader", true);
    clicks += "        click(&mut context, &mut state, target(items(\"loadedChoice\")[0].0));\n" + check("loadedChoice", true);
    clicks += "        click(&mut context, &mut state, target(items(\"templateLoader\")[0].0));\n" +
      check("templateLoader", false) + check("loadedChoice", true);
    clicks += R"RS(        for mode in 1..=6 {
            let blue = (255.0 * mode as f32 / 16.0).round() as u8;
            assert!(chart_top(context.render(), [255, 0, blue, 255]).is_finite(),
                    "Chart mode {} did not render", mode);
        }
        let line_color = [255, 0, 16, 255];
        let original_top = chart_top(context.render(), line_color);
)RS";
    const auto& chartVariables = rust.generated.variables;
    clicks += "        assert_eq!(state." + chartVariables.at("chart_line") + ", [-2.0, 20.0, 46.0, 13.0]);\n";
    clicks += "        assert!(state." + chartVariables.at("emptyChart") + ".is_empty());\n";
    clicks += "        state." + chartVariables.at("chart_line") + "[0] = 100.0;\n";
    clicks += R"RS(        frame(&mut context, &mut state);
        assert!(chart_top(context.render(), line_color) + 12.0 < original_top);
        let mut peaks = [0.0; 800];
        peaks[274] = 100.0;
        peaks[611] = -100.0;
        let mut indices = [0; 512];
        let len = chart_samples(&peaks, &mut indices);
        assert!(indices[..len].contains(&274) && indices[..len].contains(&611));
)RS";
    const auto chart3dBars = "state." + chartVariables.at("chart3d_bars");
    const auto chart3dEmpty = "state." + chartVariables.at("chart3d_empty");
    clicks += "        assert_eq!((" + chart3dBars + ".elevation, " + chart3dBars + ".azimuth), (33.0, -40.0));\n";
    clicks += "        assert_eq!(" + chart3dBars + ".values[0], 0.35); assert!(" + chart3dEmpty + ".values.is_empty());\n";
    clicks += R"RS(        let original_x = chart3d_mesh(context.render());
        context.io_mut().add_mouse_pos_event([1190.0, 510.0]); frame(&mut context, &mut state);
        context.io_mut().add_mouse_button_event(imgui::MouseButton::Left, true); frame(&mut context, &mut state);
        context.io_mut().add_mouse_pos_event([1240.0, 550.0]); frame(&mut context, &mut state);
        context.io_mut().add_mouse_button_event(imgui::MouseButton::Left, false); frame(&mut context, &mut state);
        assert!((chart3d_mesh(context.render()) - original_x).abs() > 1.0);
)RS";
    clicks += "        assert!((" + chart3dBars + ".elevation - 53.0).abs() < 0.01);\n";
    clicks += "        assert!((" + chart3dBars + ".azimuth + 10.0).abs() < 0.01);\n";
    const auto slide = "state." + chartVariables.at("slide_x");
    clicks += "        assert!(!" + slide + ".running && " + slide + ".start.is_none());\n";
    clicks += R"RS(        let mut uploads = 0;
        let mut upload = |bytes: &[u8], width, height| {
            uploads += 1;
            match (bytes, width, height) {
                ([0x89, b'P', b'N', b'G', ..], 2, 1) if bytes.len() == 74 => Some(imgui::TextureId::new(900)),
                ([0xff, 0xd8, ..], 2, 2) if bytes.len() == 202 => Some(imgui::TextureId::new(901)),
                _ => panic!("Unexpected image resource"),
            }
        };
        assert!(load_images(&mut state, &mut upload));
        assert!(load_images(&mut state, &mut upload));
        assert_eq!(uploads, 2);
        frame(&mut context, &mut state);
        let (mut pngs, mut jpegs) = (0, 0);
        for draw in context.render().draw_lists() {
            for command in draw.commands() {
                if let imgui::DrawCmd::Elements { count, cmd_params } = command {
                    match cmd_params.texture_id.id() {
                        900 => {
                            assert_eq!(count, 6);
                            let vertex = draw.idx_buffer()[cmd_params.idx_offset] as usize + cmd_params.vtx_offset;
                            assert_eq!(draw.vtx_buffer()[vertex].pos[1], 845.0);
                            pngs += 1;
                        }
                        901 => jpegs += 1,
                        _ => {}
                    }
                }
            }
        }
        assert_eq!((pngs, jpegs), (2, 1));
        click(&mut context, &mut state, [170.0, 850.0]);
)RS";
    clicks += check("image_action", true);
    clicks += "        frame(&mut context, &mut state);\n" + check("image_action", false);
    clicks += R"RS(        let mut released = Vec::new();
        release_images(&mut state, |id, _, _| released.push(id.id()));
        release_images(&mut state, |id, _, _| released.push(id.id()));
        assert_eq!(released, [900, 901]);
)RS";
    for (const auto& [side, x] : std::array<std::pair<const char*, const char*>, 4>{
           {{"left", "56.0"}, {"center", "214.0"}, {"right", "371.0"}, {"free", "290.0"}}}) {
      const auto key = std::string("title_") + side;
      clicks += std::string("        click(&mut context, &mut state, [") + x + ", 813.0]);\n" + check(key, true);
      clicks += "        frame(&mut context, &mut state);\n" + check(key, false);
    }
    clicks += R"RS(        unsafe {
            imgui::sys::igSetWindowCollapsed_Str(c"###studio_656d6265646465645f696d61676573".as_ptr(),
                                                  true, imgui::sys::ImGuiCond_Always as _);
        }
        frame(&mut context, &mut state);
        click(&mut context, &mut state, [290.0, 813.0]);
)RS" + check("title_free", true) +
      "        state." + chartVariables.at("embedded_images") + " = false;\n"
      "        frame(&mut context, &mut state);\n" + check("title_free", false);
    clicks += R"RS(        let neutral_x = animation_item("slide_x").position[0];
        let play = target(animation_item("play").position);
        click(&mut context, &mut state, play);
)RS";
    clicks += "        assert!(" + slide + ".running);\n";
    for (const auto* effect : {"pulse_alpha", "bounce_y", "scale_pulse", "color_shift"})
      clicks += "        state." + chartVariables.at(effect) + ".start = None;\n";
    clicks += R"RS(        frame(&mut context, &mut state);
        let dim = animation_item("pulse_alpha");
        let bounce = animation_item("bounce_y");
        let size = animation_item("scale_pulse");
        let hue = animation_item("color_shift");
        let start_x = animation_item("slide_x").position[0];
        context.io_mut().delta_time = 0.28;
        frame(&mut context, &mut state);
        assert!(animation_item("pulse_alpha").alpha > dim.alpha + 0.4);
        assert!(animation_item("bounce_y").position[1] < bounce.position[1] - 5.0);
        assert!(animation_item("scale_pulse").width > size.width + 6.0);
        assert!(animation_item("scale_pulse").font > size.font);
        assert_ne!(animation_item("color_shift").color, hue.color);
        context.io_mut().delta_time = 0.7;
        frame(&mut context, &mut state);
        context.io_mut().delta_time = 1.0 / 60.0;
        let moved = animation_item("slide_x").position;
        assert!(moved[0] > start_x + 10.0 && moved[0] > neutral_x);
)RS";
    clicks += check("slide_x_child", false);
    clicks += "        click(&mut context, &mut state, target(moved));\n" + check("slide_x_child", true);
    clicks += R"RS(        assert_eq!(animation_item("slide_x").position[0], moved[0]);
        click(&mut context, &mut state, play);
        assert_eq!(animation_item("slide_x").position[0], neutral_x);
)RS";
    clicks += "        assert!(!" + slide + ".running && " + slide + ".start.is_none());\n";
    write(directory / "rust" / "ui.rs", rustUi + R"RS(
#[cfg(test)]
mod layout_tests {
    use super::*;
    type Item = (&'static str, [f32; 2], imgui::Id);
    #[derive(Clone, Copy)]
    struct AnimationItem {
        position: [f32; 2],
        width: f32,
        font: f32,
        alpha: f32,
        color: [f32; 4],
    }
    thread_local! {
        static ITEMS: std::cell::RefCell<Vec<Item>> = const { std::cell::RefCell::new(Vec::new()) };
        static ANIMATIONS: std::cell::RefCell<Vec<(&'static str, AnimationItem)>> = const { std::cell::RefCell::new(Vec::new()) };
    }
    pub(super) fn record_animation(ui: &imgui::Ui, effect: &'static str) {
        ANIMATIONS.with_borrow_mut(|items| items.push((effect, AnimationItem {
            position: ui.item_rect_min(), width: ui.item_rect_size()[0], font: ui.current_font_size(),
            alpha: ui.clone_style().alpha, color: ui.style_color(imgui::StyleColor::Button),
        })));
    }
    fn animation_item(effect: &str) -> AnimationItem {
        ANIMATIONS.with_borrow(|items| items.iter().find(|entry| entry.0 == effect).expect(effect).1)
    }
    pub(super) fn record_template_item(ui: &imgui::Ui, node: &'static str) {
        ITEMS.with_borrow_mut(|items| items.push((node, ui.item_rect_min(), ui.new_id_str("template-probe"))));
    }
    fn items(node: &str) -> Vec<([f32; 2], imgui::Id)> {
        ITEMS.with_borrow(|items| items.iter().filter(|item| item.0 == node).map(|item| (item.1, item.2)).collect())
    }
    fn frame(ctx: &mut imgui::Context, state: &mut UiState) {
        ITEMS.with_borrow_mut(Vec::clear);
        ANIMATIONS.with_borrow_mut(Vec::clear);
        render_ui(ctx.frame(), state);
        ctx.render();
    }
    fn target(pos: [f32; 2]) -> [f32; 2] { [pos[0] + 5.0, pos[1] + 5.0] }
    fn chart_top(data: &imgui::DrawData, color: [u8; 4]) -> f32 {
        data.draw_lists().flat_map(|list| list.vtx_buffer())
            .filter(|vertex| vertex.col == color)
            .map(|vertex| vertex.pos[1]).fold(f32::INFINITY, f32::min)
    }
    fn chart3d_mesh(data: &imgui::DrawData) -> f32 {
        let mut bars = f32::INFINITY;
        let (mut scatter, mut surface) = (false, false);
        for vertex in data.draw_lists().flat_map(|list| list.vtx_buffer()) {
            if vertex.pos[0] < 1100.0 || vertex.pos[1] < 420.0 { continue; }
            if vertex.col == [0, 255, 255, 255] { bars = bars.min(vertex.pos[0]); }
            scatter |= vertex.col == [255, 0, 255, 255];
            surface |= vertex.pos[1] > 742.0 && vertex.col[0] > 60 && vertex.col[1] == 0 && vertex.col[2] == 0;
        }
        assert!(bars.is_finite() && scatter && surface, "Missing 3D geometry");
        bars
    }
)RS" +
      "    #[test]\n    fn model_and_flow_layout() {\n"
      "        let mut context = imgui::Context::create();\n"
      "        context.set_ini_filename(None);\n"
      "        context.io_mut().display_size = [1400.0, 1000.0];\n"
      "        context.io_mut().delta_time = 1.0 / 60.0;\n"
      "        context.fonts().build_rgba32_texture();\n"
      "        apply_style(context.style_mut());\n"
      "        let mut state = UiState::default();\n"
      "        for _ in 0..4 { frame(&mut context, &mut state); }\n"
      "        assert_eq!(state.flow_metrics[\"flow-height-" + CppCodegen::hexId("wrappedFlow") + "\"], 68.0);\n"
      "        assert_eq!(state.flow_metrics[\"flow-height-" + CppCodegen::hexId("wideFlow") + "\"], 30.0);\n"
      "        fn click(ctx: &mut imgui::Context, state: &mut UiState, pos: [f32; 2]) {\n"
      "            ctx.io_mut().add_mouse_pos_event(pos); frame(ctx, state);\n"
      "            ctx.io_mut().add_mouse_button_event(imgui::MouseButton::Left, true); frame(ctx, state);\n"
      "            ctx.io_mut().add_mouse_button_event(imgui::MouseButton::Left, false); frame(ctx, state);\n"
      "        }\n" + clicks + "    }\n}\n");
  }
  assert(!exportRustBundle(project, directory / "rust"));
  if (chrome) {
    auto rustUi = rust.files.at("ui.rs");
    const auto observe = [&](std::string_view anchor, const char* probe) {
      const auto line = rustUi.find(anchor);
      assert(line != std::string::npos);
      const auto end = rustUi.find('\n', line);
      rustUi.insert(end + 1, probe);
    };
    observe("begin_title_window(ui, c\"###studio_6368726f6d65\"",
      "        #[cfg(test)] title_interaction::observe(ui);\n");
    observe("if let Some(_menu) = ui.begin_menu(\"Archivo\") {",
      "                            #[cfg(test)] title_interaction::open_menu();\n");
    write(directory / "rust" / "ui.rs", rustUi + StudioChromeTests::rust);
  }
  assert(projectToJson(project) == original);
}
static int generateVulkanCase(const fs::path& output) {
  const auto project = loadProjectFile(fs::path(STUDIO_FIXTURES_DIR) / "../../examples/vulkan-imagenes.json");
  assert(project && project.warnings.empty());
  const auto gl = generateCppBundle(*project.project);
  auto vk = generateCppBundle(*project.project,CppRenderer::Vulkan);
  assert(vk.generated.complete() && vk.generated.hasImageTextures);
  assert(gl.files.at("ui.hpp") == vk.files.at("ui.hpp"));
  assert(!gl.files.count("studio_vulkan.hpp") && !gl.files.count("studio_vulkan_images.hpp"));
  assert(gl.files.at("CMakeLists.txt").find("Vulkan::Vulkan") == std::string::npos);
  assert(vk.files.at("CMakeLists.txt").find("OpenGL::GL") == std::string::npos);
  auto& main = vk.files.at("main.cpp");
  const std::string smokeFrames = "smokeFrames = 3";
  const auto framesLimit = main.find(smokeFrames);
  assert(framesLimit != std::string::npos);
  main.replace(framesLimit,smokeFrames.size(),"smokeFrames = 5");
  const auto hook = [&](std::string_view anchor, const std::string& code) {
    const auto at = main.find(anchor);
    assert(at != std::string::npos);
    main.insert(at,code);
  };
  hook("        unsigned frames", "        bool drawn[2]{}, resizeRequested = false, resized = false;\n"
    "        const int initialWidth = width, initialHeight = height;\n");
  hook("            if (!renderer.resize", R"CPP(            if (smoke && frames == 1) {
                if (!resizeRequested) {
                    glfwSetWindowSize(window.get(),704,450);
                    resizeRequested = true;
                }
                if (width == initialWidth && height == initialHeight) {
                    glfwWaitEventsTimeout(0.016);
                    continue;
                }
                resized = true;
            }
)CPP");
  hook("            ImGui::NewFrame();", R"CPP(            if (smoke) {
                auto& io = ImGui::GetIO();
                io.AddFocusEvent(true);
                io.AddMousePosEvent(48,60);
                // Let ImGui settle focus after the window first appears.
                if (frames >= 3) ImGui_ImplGlfw_MouseButtonCallback(window.get(),0,frames == 3 ? GLFW_PRESS : GLFW_RELEASE,0);
            }
)CPP");
  hook("            if (renderer.draw", R"CPP(            if (smoke) {
                for (const auto* drawList : ImGui::GetDrawData()->CmdLists)
                    for (const auto& draw : drawList->CmdBuffer)
                        for (unsigned i = 0; i < 2; ++i)
                            drawn[i] |= draw.ElemCount != 0 && draw.GetTexID() == state.studio_image_textures[i].id;
            }
)CPP");
  hook("        renderer.idle();", "        if (smoke && (!resized || !drawn[0] || !drawn[1] || !state." +
    vk.generated.variables.at("vulkan_check") + ")) throw std::runtime_error(\"Vulkan swapchain, texture or input test failed\");\n"
    "        if (smoke) std::puts(\"Vulkan resize, checkbox input and PNG/JPEG draw commands verified\");\n");
  fs::create_directories(output);
  for (const auto& file : vk.files) write(output / file.first,file.second);
  return 0;
}

static StudioProject imageAndKnobProject() {
  const auto fixture = loadProjectFile(fs::path(STUDIO_FIXTURES_DIR) / "../../examples/vulkan-imagenes.json");
  assert(fixture && fixture.warnings.empty());
  auto project = *fixture.project;
  auto knob = createDefaultNode(WidgetType::Knob);
  knob.id = "android_knob";
  project.nodes.front().children.push_back(std::move(knob));
  return project;
}

static int generateAndroidCase(const fs::path& pack, const fs::path& output) {
  const auto project = imageAndKnobProject();
  assert(!exportAndroidCppBundle(project, pack / "missing", output));
  assert(!fs::exists(output));
  const auto bundle = generateAndroidCppBundle(project, pack);
  assert(bundle.generated.complete() && bundle.generated.hasImageTextures && bundle.generated.hasKnobs);
  assert(bundle.files.at("ui.hpp") == generateCppDetailed(project).code);
  assert(bundle.files.at("main.cpp").find(bundle.generated.stateType + " state;") != std::string::npos);
  assert(bundle.files.at("main.cpp").find("LoadStudioImages(state") != std::string::npos);
  assert(bundle.files.at("main.cpp").find("ReleaseStudioImages(state") != std::string::npos);
  assert(bundle.files.at("main.cpp").find("STUDIO_IMAGE_ADAPTER") == std::string::npos);
  assert(bundle.files.at("CMakeLists.txt").find("imgui-knobs.cpp") != std::string::npos);
  assert(bundle.files.count("LICENSE.imgui-knobs.txt"));
  const auto exported = exportAndroidCppBundle(project, pack, output);
  if (!exported) throw std::runtime_error(exported.error);
  assert(!exportAndroidCppBundle(project, pack, output));

  const auto simple = generateAndroidCppBundle(createDefaultProject(), pack);
  assert(simple.generated.complete());
  assert(simple.files.at("main.cpp").find("stb_image.h") == std::string::npos);
  assert(simple.files.at("CMakeLists.txt").find("FetchContent_Declare(stb") == std::string::npos);
  return 0;
}

static int generateAppleCases(const fs::path& packs, const fs::path& output) {
  const auto project = imageAndKnobProject();
  const auto macPack = packs / "macOS-Cpp";
  const auto iosPack = packs / "iOS-Cpp";
  assert(!exportAppleCppBundle(project, macPack, output / "wrong-pack", AppleCppTarget::IOS));
  assert(!exportAppleCppBundle(project, iosPack, output / "wrong-pack", AppleCppTarget::MacOS));
  assert(!fs::exists(output / "wrong-pack"));
  const auto expected = generateCppDetailed(project);
  for (const auto target : {AppleCppTarget::MacOS, AppleCppTarget::IOS}) {
    const bool ios = target == AppleCppTarget::IOS;
    const auto& pack = ios ? iosPack : macPack;
    const auto destination = output / (ios ? "ios" : "macos");
    const auto bundle = generateAppleCppBundle(project, pack, target);
    assert(bundle.generated.complete() && bundle.generated.hasImageTextures && bundle.generated.hasKnobs);
    assert(bundle.files.at("ui.hpp") == expected.code);
    const auto& cmake = bundle.files.at("CMakeLists.txt");
    assert(cmake.find(ios ? "set(studio_target \"iOS\")" : "set(studio_target \"Darwin\")") != std::string::npos);
    assert(cmake.find("@STUDIO_APPLE_") == std::string::npos);
    assert(cmake.find("FetchContent_Declare(stb") != std::string::npos);
    assert(cmake.find("imgui-knobs.cpp") != std::string::npos);
    assert(bundle.files.at("studio_apple_config.hpp").find("#define STUDIO_APPLE_IMAGES 1") != std::string::npos);
    assert(bundle.files.count("studio_metal_images.hpp"));
    assert(bundle.files.count("LICENSE.dear-imgui.txt") && bundle.files.count("LICENSE.imgui-knobs.txt"));
    assert((bundle.files.at("Info.plist").find("StudioSceneDelegate") != std::string::npos) == ios);
    const auto saved = exportAppleCppBundle(project, pack, destination, target);
    if (!saved) throw std::runtime_error(saved.error);
    assert(!exportAppleCppBundle(project, pack, destination, target));

    const auto simple = generateAppleCppBundle(createDefaultProject(), pack, target);
    assert(simple.generated.complete());
    assert(!simple.files.count("studio_metal_images.hpp") && !simple.files.count("imgui-knobs.cpp"));
    assert(simple.files.at("CMakeLists.txt").find("FetchContent_Declare(stb") == std::string::npos);
    assert(simple.files.at("studio_apple_config.hpp").find("#define STUDIO_APPLE_IMAGES 0") != std::string::npos);
  }
  return 0;
}

int main(int argc, char** argv) {
  if (argc == 4 && std::string_view(argv[1]) == "--apple") return generateAppleCases(fs::u8path(argv[2]),fs::u8path(argv[3]));
  if (argc == 4 && std::string_view(argv[1]) == "--android") return generateAndroidCase(fs::u8path(argv[2]),fs::u8path(argv[3]));
  if (argc == 3 && std::string_view(argv[1]) == "--vulkan") return generateVulkanCase(fs::u8path(argv[2]));
  assert(sanitizeIdent("__class__") == "value_class");
  assert(CppCodegen::number(48) == "48.0f");
  assert(escapeString(std::string("\0" "7",2)) == "\\0007");
  if (argc != 2) { std::cerr << "Expected output directory\n"; return 1; }
  const auto output = fs::absolute(fs::u8path(argv[1])); fs::create_directories(output);
  for (const auto* name : {"blank","scene_inspector","render_settings","debug_console","all_widgets"}) {
    const auto loaded = loadProjectFile(fs::path(STUDIO_FIXTURES_DIR) / (std::string(name) + ".json"));
    assert(loaded); generateCase(output / name,*loaded.project,false);
  }
  auto empty = createDefaultProject("Empty"); empty.nodes.clear(); generateCase(output / "empty",empty,false);
  const auto edge = edgeProject(); generateCase(output / "edge",edge,true);
  const auto chrome = loadProjectFile(fs::path(STUDIO_FIXTURES_DIR) / "../../examples/barra-de-titulo.json");
  assert(chrome && chrome.warnings.empty());
  generateCase(output / "window_chrome", *chrome.project, false, true);
  auto deep = createDefaultProject("Deep Python");
  auto* parent = &deep.nodes.front();
  for (int i=0;i<105;++i) {
    parent->children.push_back(createDefaultNode(WidgetType::TreeNode));
    parent = &parent->children.back();
  }
  const auto deepPython = generatePythonCode(deep);
  assert(deepPython.error.empty() && deepPython.nodeLines.size() == 106);
  const auto deepCSharp = generateCSharpCode(deep);
  assert(deepCSharp.error.empty() && deepCSharp.nodeLines.size() == 106);
  write(output / "deep.py",deepPython.code);
  const auto pythonDestination = output / nextId("python_export");
  assert(exportPythonBundle(edge,pythonDestination));
  assert(!exportPythonBundle(empty,pythonDestination));
  assert(readTextFile(pythonDestination / "main.py",ProjectJson::kMaxProjectBytes).text == generatePythonCode(edge).code);
  for (const auto* file : {"main.py","requirements.txt","codegen_report.txt","README.md"})
    assert(fs::remove(pythonDestination / file));
  assert(fs::remove(pythonDestination));
  // New export only; existing folders and their contents must survive untouched.
  const auto destination = output / ("export_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
  const auto result = exportCppBundle(edge,destination); assert(result);
  const auto saved = readTextFile(destination / "ui.hpp",ProjectJson::kMaxProjectBytes); assert(saved);
  assert(!exportCppBundle(empty,destination));
  assert(readTextFile(destination / "ui.hpp",ProjectJson::kMaxProjectBytes).text == saved.text);
  assert(!exportCppBundle(empty,output / "absent_parent" / "child"));
  for (const auto& file : generateCppBundle(edge).files) assert(fs::remove(destination / file.first));
  assert(fs::remove(destination));
  // Stable scaffold consumed by the independent configure/build verification.
  const auto bundle = generateCppBundle(edge);
  fs::create_directories(output / "scaffold");
  for (const auto& file : bundle.files) write(output / "scaffold" / file.first,file.second);
  std::cout << "Generated eight cases, three modes; export preservation verified.\n";
}
