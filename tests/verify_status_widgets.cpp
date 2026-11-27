#include "imguistudio/export/studio_cpp_export.hpp"
#include "imguistudio/export/studio_c_export.hpp"
#include "imguistudio/export/studio_csharp_export.hpp"
#include "imguistudio/export/studio_java_export.hpp"
#include "imguistudio/export/studio_python_export.hpp"
#include "imguistudio/export/studio_rust_export.hpp"

#include <cassert>
#include <iostream>

int main(int argc, char** argv) {
  using namespace ImGuiStudio;
  const auto example = loadProjectFile(std::filesystem::path(STUDIO_EXAMPLES_DIR) / "estado-y-progreso.json");
  assert(example && example.warnings.empty());
  const auto serialized = projectToJson(*example.project);
  const auto restored = parseProject(serialized.dump());
  assert(restored && projectToJson(*restored.project) == serialized);
  const auto& controls = restored.project->nodes.front().children;
  assert(controls[0].type == WidgetType::Toggle && controls[0].props.checked);
  assert(controls[1].type == WidgetType::Spinner && controls[1].props.speed == 0.9f);
  assert(controls[2].type == WidgetType::CircularProgress && controls[2].props.progress == 0.65f);
  assert(controls[2].props.overlay == "65 %");

  const auto bundle = generateCppBundle(*restored.project);
  assert(bundle.generated.error.empty() && bundle.generated.diagnostics.empty() && bundle.generated.hasStatusWidgets);
  assert(bundle.files.at("studio_status_widgets.hpp") == statusWidgetSource);
  const auto& header = bundle.generated.code;
  assert(header.find("StudioStatusWidgets::toggle(\"##toggle\", \"Modo automático\", state.automatic") != std::string::npos);
  assert(header.find("StudioStatusWidgets::spinner(\"Sincronizando\", 48.0f, state.synchronizing)") != std::string::npos);
  assert(header.find("StudioStatusWidgets::circularProgress(\"Procesado\", 96.0f, state.processing") != std::string::npos);
  assert(header.find("PushStyleColor(ImGuiCol_PlotHistogram") != std::string::npos);
  assert(!generateCppBundle(createDefaultProject()).files.count("studio_status_widgets.hpp"));
  for (const auto& result : {generateCCode(*restored.project), generateCSharpCode(*restored.project),
                            generateJavaCode(*restored.project), generatePythonCode(*restored.project),
                            generateRustCode(*restored.project)}) {
    assert(result.error.empty() && result.code.find("StudioStatusWidgets::") == std::string::npos);
    for (const auto& node : controls)
      assert(std::any_of(result.diagnostics.begin(), result.diagnostics.end(),
        [&](const auto& diagnostic) { return diagnostic.nodeId == node.id; }));
  }
  if (argc == 3 && std::string_view(argv[1]) == "--export") {
    const auto exported = writeGeneratedBundle(bundle, std::filesystem::u8path(argv[2]));
    if (!exported) { std::cerr << exported.error << '\n'; return 1; }
  }
  std::cout << "[OK] Status widget JSON, C++ export, and compatibility diagnostics.\n";
}
