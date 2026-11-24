#include "studio_project_io.hpp"
#include "studio_widget_library.hpp"
#include "studio_snapshots.hpp"

#include <cassert>
#include <iostream>
#include <set>

#ifndef STUDIO_FIXTURES_DIR
#define STUDIO_FIXTURES_DIR "tests/fixtures"
#endif

namespace {
using namespace ImGuiStudio;

Json fixture(const char* name) {
  std::ifstream file(std::filesystem::path(STUDIO_FIXTURES_DIR) / name);
  assert(file);
  Json value;
  file >> value;
  return value;
}

void collectTypes(const std::vector<ImGuiNode>& nodes, std::set<WidgetType>& types) {
  for (const auto& node : nodes) {
    assert(node.type != WidgetType::Unknown);
    types.insert(node.type);
    collectTypes(node.children, types);
  }
}

void expectRejected(const Json& json) {
  const auto result = projectFromJson(json);
  assert(!result);
  assert(!result.error.empty());
}
}

int main() {
  using namespace ImGuiStudio;
  for (const char* file : {"blank.json", "scene_inspector.json", "render_settings.json",
                          "debug_console.json", "all_widgets.json"}) {
    const auto original = fixture(file);
    const auto loaded = projectFromJson(original);
    if (!loaded) { std::cerr << file << ": " << loaded.error << '\n'; return 1; }
    assert(loaded.warnings.empty());
    assert(projectToJson(*loaded.project) == original);
    assert(parseProject(original.dump()));
  }

  auto json = fixture("all_widgets.json");
  auto loaded = projectFromJson(json);
  assert(loaded);
  std::set<WidgetType> types;
  collectTypes(loaded.project->nodes, types);
  assert(types.size() == 60);

  // Editing newly interpreted fields must touch only that property and retain
  // all other prototype data, including extension fields and vector tails.
  auto editedJson = json;
  auto editable = *loaded.project;
  bool editedVector = false, editedColumns = false, editedSelection = false;
  bool editedLoader = false, editedComponent = false, editedRepeater = false;
  for (size_t i = 0; i < editable.nodes[0].children.size(); ++i) {
    auto& node = editable.nodes[0].children[i];
    auto& expected = editedJson["nodes"][0]["children"][i]["props"];
    if (node.type == WidgetType::SliderFloat2) {
      node.props.defaultVec[0] = 0.25f;
      expected["defaultVec"][0] = 0.25f;
      editedVector = true;
    }
    if (node.type == WidgetType::Table) {
      node.props.columnLabels[0] = "Renombrada";
      expected["columns"][0] = "Renombrada";
      editedColumns = true;
    }
    if (node.type == WidgetType::Selectable) {
      node.props.selected = true;
      expected["selected"] = true;
      editedSelection = true;
    }
    if (node.type == WidgetType::Loader) {
      node.props.loaderActive = false;
      expected["loaderActive"] = false;
      editedLoader = true;
    }
    if (node.type == WidgetType::Component) {
      node.props.sourceComponent = "Tarjeta actualizada";
      expected["sourceComponent"] = node.props.sourceComponent;
      editedComponent = true;
    }
    if (node.type == WidgetType::Repeater) {
      node.props.repeatCount = 8;
      expected["repeatCount"] = node.props.repeatCount;
      editedRepeater = true;
    }
  }
  assert(editedVector && editedColumns && editedSelection);
  assert(editedLoader && editedComponent && editedRepeater);
  assert(projectToJson(editable) == editedJson);
  auto longVector = fixture("blank.json");
  auto vec = createDefaultNode(WidgetType::SliderFloat3);
  auto vecJson = ProjectJson::nodeJson(vec);
  vecJson["props"]["defaultVec"] = {1, 2, 3, 4, 5};
  vecJson["props"]["future"] = {{"keep", true}};
  longVector["nodes"][0]["children"] = Json::array({vecJson});
  auto vectorProject = projectFromJson(longVector);
  assert(vectorProject && projectToJson(*vectorProject.project) == longVector);
  vectorProject.project->nodes[0].children[0].props.defaultVec[0] = 0.5f;
  longVector["nodes"][0]["children"][0]["props"]["defaultVec"][0] = 0.5f;
  assert(projectToJson(*vectorProject.project) == longVector);

  // Preserve future widgets, fields, enum values and numeric precision, including
  // copies and reverts. Edits must only change the requested serialized fields.
  auto future = fixture("blank.json");
  future["futureRoot"] = {{"secretlessExtension", {1, 2, 3}}};
  future["style"]["futureStyle"] = 0.123456789012345;
  future["canvas"]["orientation"] = "future_orientation";
  future["nodes"][0]["width"] = 420.123456789;
  future["nodes"][0]["props"]["windowFlags"] = {"FutureWindowFlag", "ImGuiWindowFlags_NoResize"};
  future["nodes"][0]["children"] = Json::array({Json{
    {"id", "future_node"}, {"type", "vendor_future_widget"}, {"label", "Future"},
    {"varName", "future"}, {"props", {{"futureProp", {1, "two", true}}}},
    {"children", Json::array()}, {"futureNodeField", "retained"}}});
  auto result = projectFromJson(future);
  assert(result && result.warnings.size() == 1);
  auto project = *result.project;
  assert(projectToJson(project) == future);
  project.nodes[0].label = "Renamed";
  project.nodes[0].props.noMove = true;
  auto expected = future;
  expected["nodes"][0]["label"] = "Renamed";
  expected["nodes"][0]["props"]["windowFlags"].push_back("ImGuiWindowFlags_NoMove");
  assert(projectToJson(project) == expected);
  auto duplicate = cloneWithFreshIds(project.nodes[0].children[0]);
  project.nodes[0].children.push_back(duplicate);
  assert(projectToJson(project)["nodes"][0]["children"][1]["props"] ==
         future["nodes"][0]["children"][0]["props"]);
  SnapshotManager snapshots;
  assert(snapshots.saveSnapshot(project, "future", SnapshotKind::Manual));
  const auto savedId = snapshots.snapshots()[0].id;
  const auto firstFingerprint = computeProjectFingerprint(project);
  project.style.buttonColor.r = 0.9f;
  assert(firstFingerprint != computeProjectFingerprint(project));
  auto restored = snapshots.revertToSnapshot(savedId, project);
  assert(restored && projectToJson(*restored) != projectToJson(project));
  project.nodes[0].children[0].props.source["futureProp"] = "edited extension";
  assert(firstFingerprint != computeProjectFingerprint(project));

  // Legacy files without canvas and without optional fields remain unchanged.
  auto legacy = fixture("blank.json");
  legacy.erase("canvas");
  legacy["nodes"][0].erase("children");
  result = projectFromJson(legacy);
  assert(result && projectToJson(*result.project) == legacy);

  // Structural validation rejects bad input without touching the live project.
  auto bad = fixture("blank.json");
  bad["nodes"].push_back(bad["nodes"][0]);
  expectRejected(bad);
  bad = fixture("blank.json"); bad["nodes"][0]["width"] = "wide"; expectRejected(bad);
  bad = fixture("blank.json"); bad["nodes"][0]["props"]["border"] = 1; expectRejected(bad);
  bad = fixture("blank.json"); bad["canvas"]["canvasWidth"] = 0; expectRejected(bad);
  bad = fixture("blank.json"); bad["canvas"]["canvasHeight"] = 1.5; expectRejected(bad);
  bad = fixture("blank.json"); bad["nodes"][0]["props"]["defaultColor"] = {1, 0}; expectRejected(bad);
  bad = fixture("blank.json"); bad["style"]["colors"]["windowBg"] = "#gg0000"; expectRejected(bad);
  bad = fixture("blank.json"); bad["nodes"][0]["type"] = "button"; expectRejected(bad);
  bad = fixture("blank.json");
  Json* parent = &bad["nodes"][0];
  for (size_t i = 0; i < ProjectJson::kMaxDepth + 2; ++i) {
    (*parent)["children"] = Json::array({Json{{"id", "deep_" + std::to_string(i)},
      {"type", "group"}, {"label", "deep"}, {"varName", "deep"}, {"props", Json::object()}}});
    parent = &(*parent)["children"][0];
  }
  expectRejected(bad);
  assert(!parseProject("{broken json"));
  assert(!parseProject(std::string(ProjectJson::kMaxProjectBytes + 1, ' ')));

  // Disk round trip, replacement, Unicode paths and failed-save preservation.
  const auto directory = std::filesystem::temp_directory_path() / nextId("studio_io_test");
  std::filesystem::create_directory(directory);
  const auto file = directory / std::filesystem::u8path(u8"interfaz_á_日本.json");
  const auto original = fixture("scene_inspector.json");
  project = *projectFromJson(original).project;
  auto saved = saveProjectFile(project, file);
  if (!saved) { std::cerr << saved.error << '\n'; return 1; }
  auto disk = loadProjectFile(file);
  assert(disk && projectToJson(*disk.project) == original);
  project.name = "Replaced";
  assert(saveProjectFile(project, file));
  disk = loadProjectFile(file);
  assert(disk && disk.project->name == "Replaced");
  auto invalid = project;
  invalid.nodes[0].width = std::numeric_limits<float>::infinity();
  assert(!saveProjectFile(invalid, file));
  assert(loadProjectFile(file).project->name == "Replaced");
  assert(!saveProjectFile(project, directory / "missing" / "project.json"));
  assert(!saveProjectFile(project, directory));
  assert(!loadProjectFile(directory / "missing.json"));
  size_t files = 0;
  for (const auto& entry : std::filesystem::directory_iterator(directory)) { (void)entry; ++files; }
  assert(files == 1); // A failed replacement cleans only its own temporary sibling.
  std::filesystem::remove(file);

  StudioWidgetLibrary library;
  assert(library.load(directory));
  auto group = createDefaultNode(WidgetType::Group);
  auto radio = createDefaultNode(WidgetType::RadioButton);
  radio.imguiId = "choice";
  auto otherRadio = createDefaultNode(WidgetType::RadioButton);
  otherRadio.varName = radio.varName;
  group.children = {radio, otherRadio};
  const auto widget = library.save("Control reutilizable", group);
  assert(widget && library.find(widget.id));
  const auto originalWidget = *library.find(widget.id);
  auto firstCopy = instantiateLibraryWidget(*originalWidget.node);
  auto secondCopy = instantiateLibraryWidget(*originalWidget.node);
  assert(firstCopy.id != group.id && firstCopy.id != secondCopy.id);
  assert(firstCopy.children[0].id != secondCopy.children[0].id);
  assert(firstCopy.children[0].varName == firstCopy.children[1].varName);
  assert(firstCopy.children[0].varName != secondCopy.children[0].varName);
  assert(firstCopy.children[0].imguiId != secondCopy.children[0].imguiId);
  const auto widgetPath = directory / std::filesystem::u8path(u8"componente_é_日本.json");
  assert(library.exportFile(originalWidget, widgetPath));
  assert(!library.exportFile(originalWidget, widgetPath));
  const auto separate = directory / "imported";
  StudioWidgetLibrary importedLibrary;
  assert(importedLibrary.load(separate));
  const auto imported = importedLibrary.importFile(widgetPath);
  assert(imported && imported.id != widget.id && importedLibrary.entries().size() == 1);
  assert(ProjectJson::nodeJson(*importedLibrary.find(imported.id)->node) == ProjectJson::nodeJson(group));
  const auto invalidWidget = directory / "invalid-widget.json";
  assert(writeTextFileAtomically(R"({"format":"imgui-studio-widget","version":1,"name":"Invalid","node":{"type":"unknown"}})", invalidWidget));
  assert(!importedLibrary.importFile(invalidWidget) && importedLibrary.entries().size() == 1);
  const auto examples = std::filesystem::path(STUDIO_FIXTURES_DIR).parent_path().parent_path() / "examples";
  const auto demo = importedLibrary.importFile(examples / "tarjeta-de-proceso.widget.json");
  assert(demo && importedLibrary.find(demo.id)->node->children[1].props.progress > 0.6f);
  StudioWidgetLibrary concurrent;
  assert(concurrent.load(directory));
  auto changedWidget = *originalWidget.node;
  changedWidget.label = "Editado";
  const auto revision = *concurrent.find(widget.id);
  assert(concurrent.save("Control editado", changedWidget, &revision));
  assert(!library.save("Edición obsoleta", group, &originalWidget));
  assert(library.save("Segundo widget", createDefaultNode(WidgetType::Button)));
  StudioWidgetLibrary reopenedLibrary;
  assert(reopenedLibrary.load(directory) && reopenedLibrary.entries().size() == 2);
  assert(reopenedLibrary.find(widget.id)->name == "Control editado");
  assert(reopenedLibrary.find(widget.id)->node->label == "Editado");
  assert(std::filesystem::weakly_canonical(directory).parent_path() ==
         std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()));
  std::filesystem::remove_all(directory);
  std::cout << "[OK] 60 widget types, legacy JSON fidelity, validation, snapshots and atomic file IO.\n";
}
