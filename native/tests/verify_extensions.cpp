#include "studio_extensions.hpp"
#include "studio_cpp_export.hpp"
#include "studio_c_export.hpp"
#include "studio_csharp_export.hpp"
#include "studio_java_export.hpp"
#include "studio_python_export.hpp"
#include "studio_rust_export.hpp"

#include <cassert>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
  using namespace ImGuiStudio;
  const auto state = std::filesystem::temp_directory_path() / nextId("studio_extensions_test");
  const bool online = argc >= 2 && std::string_view(argv[1]) == "--online";
  auto files = KnobsPackage{};
  for (size_t i = 0; i < files.size(); ++i) files[i] = KnobsSource::files[i].contents;
  auto corrupt = files;
  corrupt[0][0] = 'X';
  assert(!hasInstalledKnobs(state) && !installKnobsPackage(state, corrupt));
  assert(!hasInstalledKnobs(state));

  StudioExtensionCatalog catalog;
  catalog.load(state);
  if (online) {
    assert(catalog.canInstall());
    catalog.install();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(50);
    while (catalog.busy() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      catalog.poll();
    }
    if (!catalog.installed() || !catalog.error().empty()) {
      std::cerr << "GitHub installation failed: " << catalog.error() << '\n';
      return 1;
    }
  } else {
    assert(installKnobsPackage(state, files));
  }
  assert(!installKnobsPackage(state, corrupt) && hasInstalledKnobs(state));
  StudioExtensionCatalog reopened;
  reopened.load(state);
  assert(reopened.installed());
  reopened.remove();
  assert(reopened.error().empty() && !reopened.installed() && !hasInstalledKnobs(state));
  assert(!std::filesystem::exists(state / "extensions" / "imgui-knobs.json"));
  assert(installKnobsPackage(state, files) && hasInstalledKnobs(state));

  auto project = createDefaultProject("Muestra de perilla");
  auto knob = createDefaultNode(WidgetType::Knob);
  knob.label = "Volumen";
  knob.props.minFloat = knob.props.maxFloat = 100;
  knob.props.floatValue = 500;
  project.nodes.front().children = {knob};
  const auto original = projectToJson(project);
  const auto reloaded = projectFromJson(original);
  assert(reloaded && projectToJson(*reloaded.project) == original);
  const auto cpp = generateCppBundle(project);
  assert(cpp.generated.error.empty() && cpp.generated.diagnostics.empty() && cpp.generated.hasKnobs);
  assert(cpp.generated.code.find("ImGuiKnobs::Knob(") != std::string::npos);
  assert(cpp.files.at("LICENSE.imgui-knobs.txt") == KnobsSource::files.front().contents);
  assert(cpp.files.at("imgui-knobs.cpp") == KnobsSource::files.back().contents);
  for (const auto& result : {generateCCode(project), generatePythonCode(project), generateCSharpCode(project),
                              generateJavaCode(project), generateRustCode(project)}) {
    assert(result.error.empty() && !result.diagnostics.empty());
    assert(std::any_of(result.diagnostics.begin(), result.diagnostics.end(),
             [&](const auto& diagnostic) { return diagnostic.nodeId == knob.id; }));
    assert(result.code.find("ImGuiKnobs::") == std::string::npos);
  }
  if (argc == 3 && (online || std::string_view(argv[1]) == "--export")) {
    const auto exported = writeGeneratedBundle(cpp, std::filesystem::u8path(argv[2]));
    if (!exported) { std::cerr << exported.error << '\n'; return 1; }
  }
  assert(std::filesystem::weakly_canonical(state).parent_path() ==
    std::filesystem::weakly_canonical(std::filesystem::temp_directory_path()));
  std::filesystem::remove_all(state);
  std::cout << "[OK] Catalog " << (online ? "GitHub download, " : "")
    << "integrity, persistence, removal and C++ export.\n";
}
