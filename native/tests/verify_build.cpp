#include "studio_build_profiles.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
using namespace ImGuiStudio;
namespace fs = std::filesystem;
static void finish(NativeBuild& build) {
  const auto deadline = std::chrono::steady_clock::now()+std::chrono::minutes(3);
  while (build.busy() && std::chrono::steady_clock::now()<deadline) { build.poll(); std::this_thread::sleep_for(std::chrono::milliseconds(2)); }
  if (build.busy() || build.phase() == BuildPhase::Failed) std::cerr << build.log() << '\n';
  assert(!build.busy());
}
static int verifyPython(const fs::path& root,const std::string& python,const std::string& binding) {
  auto project = createDefaultProject("Python Build ñ");
  auto button = createDefaultNode(WidgetType::Button);
  project.nodes[0].children.push_back(button);
  const auto original = projectToJson(project);
  NativeBuild build;
  BuildSettings settings;
  settings.target = BuildTarget::Python;
  settings.cmake = "unused-missing-cmake";
  settings.python = "missing-python-studio-test";
  assert(!build.build(project,root,settings));
  assert(build.error().find("Python no encontrado") != std::string::npos);
  settings.python = python;
  settings.pythonBinding = (root / "binding-no-existe").u8string();
  assert(!build.build(project,root,settings));
  assert(build.error().find("inexistente") != std::string::npos);
  settings.pythonBinding = binding;
  fs::create_directories(root);
  // A separate package metadata directory lets this test exercise version
  // mismatch even when the selected interpreter has the right binding installed.
  const auto wrong = root / nextId("wrong_binding");
  const auto metadata = wrong / "imgui_bundle-0.0.dist-info";
  fs::create_directories(metadata);
  assert(writeTextFileAtomically("Metadata-Version: 2.1\nName: imgui-bundle\nVersion: 0.0\n",metadata / "METADATA"));
  auto incompatible = settings;
  incompatible.pythonBinding = wrong.u8string();
  assert(build.build(project,root,incompatible));
  finish(build);
  assert(build.phase() == BuildPhase::Failed && !build.canRun(project));
  assert(build.log().find("encontrado 0.0") != std::string::npos);
  assert(std::any_of(build.diagnostics().begin(),build.diagnostics().end(),[](const BuildDiagnostic& diagnostic) {
    return diagnostic.severity == DiagnosticSeverity::Error && diagnostic.message.find("encontrado 0.0") != std::string::npos;
  }));

  assert(build.build(project,root,settings));
  assert(build.phase() == BuildPhase::Checking);
  finish(build);
  assert(build.phase() == BuildPhase::Ready && build.canRun(project,settings,root));
  assert(build.executable() == build.sourceDirectory() / "main.py");
  assert(build.log().find("sintaxis y binding") != std::string::npos);
  auto changedProfile = settings;
  changedProfile.pythonBinding += "/another";
  assert(!build.canRun(project,changedProfile,root));
  auto changed = project;
  changed.name = "changed";
  assert(!build.canRun(changed,settings,root));
  assert(build.run(project,settings,root,true));
  finish(build);
  assert(build.phase() == BuildPhase::Ready && build.log().find("Python rendered three frames.") != std::string::npos);
  assert(build.run(project,settings,root));
  build.cancel();
  finish(build);
  assert(build.phase() == BuildPhase::Cancelled);

  assert(build.build(project,root,settings));
  finish(build);
  assert(build.phase() == BuildPhase::Ready);
  assert(writeTextFileAtomically("raise RuntimeError('unverified change')\n",build.executable()));
  assert(!build.run(project,settings,root));
  assert(build.error().find("La copia Python cambio") != std::string::npos);
  assert(!build.canRun(project) && projectToJson(project) == original);
  std::cout << "Python binding validation, graphical smoke, cancellation and modified artifact rejection verified.\n";
  return 0;
}
static int verifyJava(const fs::path& root,const std::string& jdk,const std::string& jars) {
  auto project = createDefaultProject("Java Build ñ");
  auto button = createDefaultNode(WidgetType::Button);
  project.nodes[0].children.push_back(button);
  const auto original = projectToJson(project);
  NativeBuild build;
  BuildSettings settings;
  settings.target = BuildTarget::Java;
  settings.cmake = "unused-missing-cmake";
  settings.jdkHome = (root / "jdk-no-existe").u8string();
  assert(!build.build(project,root,settings));
  assert(build.error().find("Se necesitan java y javac") != std::string::npos);
  settings.jdkHome = jdk;
  assert(!build.build(project,root,settings));
  assert(build.error().find("Indique la carpeta de JAR") != std::string::npos);
  settings.javaBinding = (root / nextId("empty-jars")).u8string();
  fs::create_directories(fs::u8path(settings.javaBinding));
  assert(!build.build(project,root,settings));
  assert(build.error().find("no contiene JAR") != std::string::npos);
  settings.javaBinding = jars;

  assert(build.build(project,root,settings));
  assert(build.phase() == BuildPhase::CheckingJava);
  finish(build);
  assert(build.phase() == BuildPhase::Ready && build.canRun(project,settings,root));
  assert(build.executable().filename() == "Main.class");
  auto changed = settings;
  changed.javaBinding += "/another";
  assert(!build.canRun(project,changed,root));
  changed = settings;
  changed.jdkHome += "/another";
  assert(!build.canRun(project,changed,root));
  assert(build.run(project,settings,root,true));
  finish(build);
  assert(build.phase() == BuildPhase::Ready && build.log().find("rendered three frames") != std::string::npos);
  assert(build.run(project,settings,root));
  build.cancel();
  finish(build);
  assert(build.phase() == BuildPhase::Cancelled);

  assert(build.build(project,root,settings));
  // javac -version is the only active process until the controller polls again.
  const auto path = build.sourceDirectory() / "StudioUI.java";
  const auto source = readTextFile(path,ProjectJson::kMaxProjectBytes); assert(source);
  const auto code = generateJavaCode(project);
  const auto line = code.nodeLines.at(button.id).first + 2;
  size_t offset = 0;
  for (size_t i=1;i<line;++i) offset = source.text.find('\n',offset)+1;
  auto broken = source.text;
  broken.insert(offset,"        studioInjectedMissingMethod();\n");
  assert(writeTextFileAtomically(broken,path));
  finish(build);
  assert(build.phase() == BuildPhase::Failed && !build.canRun(project));
  bool mapped = false;
  for (const auto& d : build.diagnostics()) {
    if (d.nodeId != button.id || d.line != line || d.severity != DiagnosticSeverity::Error) continue;
    const auto snapshot = build.diagnosticSource(d);
    assert(snapshot && *snapshot == source.text);
    mapped = true;
  }
  assert(mapped && projectToJson(project) == original);
  std::cout << "Java compile, graphical smoke, cancellation and javac widget mapping verified.\n";
  return 0;
}
static int verifyRust(const fs::path& root, const std::string& cargo, bool release) {
  auto project = createDefaultProject("Native Rust Build ñ");
  const auto button = createDefaultNode(WidgetType::Button);
  project.nodes.front().children.push_back(button);
  const auto original = projectToJson(project);
  BuildSettings settings;
  settings.target = BuildTarget::Rust;
  settings.cargo = (root / "missing-cargo").u8string();
  settings.rustOffline = true;
  NativeBuild build;
  assert(!build.build(project,root,settings));
  assert(build.error().find("Cargo no encontrado") != std::string::npos);
  settings.cargo = cargo;
  settings.configuration = "MinSizeRel";
  assert(!build.build(project,root,settings));
  assert(build.error().find("Debug y Release") != std::string::npos);
  settings.configuration = release ? "Release" : "Debug";
  assert(build.build(project,root,settings));
  assert(build.phase() == BuildPhase::CheckingRust && !build.canRun(project));
  finish(build);
  assert(build.phase() == BuildPhase::Ready && build.executable().stem() == "studio-export");
  assert(build.executable().parent_path().filename() == (release ? "release" : "debug"));
  assert(build.canRun(project,settings,root));
  auto changedProfile = settings;
  changedProfile.configuration = release ? "Debug" : "Release";
  assert(!build.canRun(project,changedProfile,root));
  changedProfile = settings;
  changedProfile.rustOffline = false;
  assert(!build.canRun(project,changedProfile,root));
  auto changed = project;
  changed.nodes.front().children.front().label = "Edited";
  assert(!build.canRun(changed,settings,root));
  assert(build.run(project,settings,root,true));
  finish(build);
  assert(build.phase() == BuildPhase::Ready && build.log().find("Rust imgui-rs rendered three frames") != std::string::npos);
  if (release) {
    assert(projectToJson(project) == original);
    std::cout << "Rust Cargo Release build, direct graphical run and profile tracking verified.\n";
    return 0;
  }
  assert(build.run(project,settings,root));
  build.cancel();
  finish(build);
  assert(build.phase() == BuildPhase::Cancelled && !build.canRun(project));

  assert(build.build(project,root,settings));
  const auto path = build.sourceDirectory() / "ui.rs";
  const auto snapshot = readTextFile(path,ProjectJson::kMaxProjectBytes);
  assert(snapshot);
  const auto line = generateRustCode(project).nodeLines.at(button.id).first + 2;
  auto broken = snapshot.text;
  size_t offset = 0;
  for (size_t i = 1; i < line; ++i) offset = broken.find('\n',offset) + 1;
  broken.insert(offset,"        compile_error!(\"injected widget error\");\n");
  assert(writeTextFileAtomically(broken,path));
  finish(build);
  assert(build.phase() == BuildPhase::Failed && !build.canRun(project));
  const auto diagnostic = std::find_if(build.diagnostics().begin(),build.diagnostics().end(),[&](const BuildDiagnostic& d) {
    return d.severity == DiagnosticSeverity::Error && d.message == "injected widget error" && d.nodeId == button.id;
  });
  assert(diagnostic != build.diagnostics().end() && diagnostic->line == line);
  const auto* source = build.diagnosticSource(*diagnostic);
  assert(source && *source == snapshot.text && projectToJson(project) == original);
  std::cout << "Rust Cargo build/run, cancellation, profile tracking and rustc widget mapping verified.\n";
  return 0;
}

static int verifyVulkan(const fs::path& root, const std::string& sdk) {
  fs::create_directories(root);
  const auto project = createDefaultProject("Vulkan Build");
  NativeBuild build;
  BuildSettings settings;
  settings.cmake = STUDIO_CMAKE_PATH;
  settings.generator = "Ninja";
  settings.glfwSource = STUDIO_GLFW_SOURCE;
  settings.imguiSource = STUDIO_IMGUI_SOURCE;
  settings.cppRenderer = CppRenderer::Vulkan;
  settings.vulkanRoot = (root / nextId("missing_sdk")).u8string();
  assert(!build.build(project,root,settings) && build.error().find("SDK de Vulkan") != std::string::npos);
  settings.vulkanRoot = sdk;
  assert(build.build(project,root,settings));
  finish(build);
  assert(build.phase() == BuildPhase::Ready && build.canRun(project,settings,root));
  const auto source = build.sourceDirectory();
  assert(fs::is_regular_file(source / "studio_vulkan.hpp"));
  assert(!fs::exists(source / "studio_vulkan_images.hpp"));
  auto gl = settings;
  gl.cppRenderer = CppRenderer::OpenGL;
  assert(!build.canRun(project,gl,root));
  assert(build.run(project,settings,root,true));
  finish(build);
  assert(build.phase() == BuildPhase::Ready && build.log().find("Vulkan presented 3 frames") != std::string::npos);
  std::cout << build.log();
  return 0;
}

int main(int argc,char** argv) {
  if ((argc == 3 || argc == 4) && std::string_view(argv[2]) == "--vulkan")
    return verifyVulkan(fs::absolute(fs::u8path(argv[1])),argc == 4 ? argv[3] : "");
  if (argc == 5 && std::string(argv[2]) == "--python")
    return verifyPython(fs::absolute(fs::u8path(argv[1])),argv[3],argv[4]);
  if (argc == 5 && std::string(argv[2]) == "--java")
    return verifyJava(fs::absolute(fs::u8path(argv[1])),argv[3],argv[4]);
  if (argc == 4 && std::string(argv[2]) == "--rust")
    return verifyRust(fs::absolute(fs::u8path(argv[1])),argv[3],false);
  if (argc == 4 && std::string(argv[2]) == "--rust-release")
    return verifyRust(fs::absolute(fs::u8path(argv[1])),argv[3],true);
  assert(argc == 2 || argc == 3 || argc == 4);
  const bool c = argc == 4;
  const bool csharp = argc == 3;
  auto project = createDefaultProject("Native Build ñ");
  auto button = createDefaultNode(WidgetType::Button); button.label = "Generated app"; project.nodes[0].children.push_back(button);
  const auto original = projectToJson(project);
  const auto root = fs::absolute(fs::u8path(argv[1]));
  NativeBuild build;
  assert(!build.run(project));
  BuildSettings settings; settings.cmake = "missing-cmake-studio-test";
  assert(!build.build(project,root,settings)); assert(build.phase() == BuildPhase::Failed);
  settings.cmake = STUDIO_CMAKE_PATH; settings.generator = "Ninja";
  settings.glfwSource = STUDIO_GLFW_SOURCE; settings.imguiSource = STUDIO_IMGUI_SOURCE;
  if (c) {
    settings.target = BuildTarget::C;
    settings.cimguiSource = argv[2]; settings.cImguiSource = argv[3];
    settings.imguiSource = (root / "unused-cpp-imgui").u8string();
    settings.cCompiler = "missing-c-compiler-studio-test";
    assert(!build.build(project,root,settings));
    assert(build.error().find("Compilador C no encontrado") != std::string::npos);
    settings.cCompiler = detectToolchain().cCompiler.u8string();
    assert(!settings.cCompiler.empty());
  }
  if (csharp) {
    settings.target = BuildTarget::CSharp;
    settings.cmake = "unused-missing-cmake";
    settings.dotnet = "missing-dotnet-studio-test";
    assert(!build.build(project,root,settings));
    assert(build.error().find("dotnet no encontrado") != std::string::npos);
    settings.dotnet = argv[2];
  }
  fs::create_directories(root);
  BuildProfileStore profiles;
  const auto profilePath = root / (nextId("build_profile") + ".json");
  assert(profiles.load(profilePath,root.u8string())); profiles.document().active().settings = settings;
  assert(profiles.save());
  BuildProfileStore reopened; assert(reopened.load(profilePath,"ignored"));
  settings = reopened.document().active().settings;
  assert(!detectToolchain().cmake.empty());
  assert(build.build(project,root,settings)); assert(build.busy());
  assert(build.phase() == (csharp ? BuildPhase::Restoring : BuildPhase::Configuring));
  assert(!build.build(project,root,settings)); assert(!build.run(project));
  finish(build);
  assert(build.phase() == BuildPhase::Ready && fs::is_regular_file(build.executable()));
  assert(build.executable().stem() == (csharp ? "StudioExport" : c ? "studio_export_c" : "studio_export"));
  assert(fs::exists(build.sourceDirectory() / (csharp ? "Program.cs" : c ? "main.c" : "main.cpp")));
  if (csharp) {
    assert(build.executable().extension() == ".dll");
    assert(fs::exists(root / "nuget-packages" / "imgui.net"));
    assert(build.log().find("StudioExport.csproj") != std::string::npos);
    assert(build.log().find("--no-restore") != std::string::npos);
    auto changedLauncher = settings;
    changedLauncher.dotnet = "another-dotnet";
    assert(!build.canRun(project,changedLauncher,root));
  }
  assert(build.canRun(project)); assert(projectToJson(project) == original);
  assert(build.canRun(project,settings,root));
  assert(build.canRun(project,settings,root / ""));
  assert(build.canRun(project,settings,root / "."));
  auto changedSettings = settings; changedSettings.configuration = "Release";
  assert(!build.canRun(project,changedSettings,root)); assert(!build.run(project,changedSettings,root,true));
  changedSettings = settings; changedSettings.target = c ? BuildTarget::Cpp : BuildTarget::C;
  assert(!build.canRun(project,changedSettings,root)); assert(!build.run(project,changedSettings,root,true));
  assert(!build.canRun(project,settings,root / "different"));
  auto changed = project; changed.nodes[0].children[0].label = "Edited";
  assert(!build.canRun(changed)); assert(!build.run(changed)); assert(!build.busy());
  assert(build.run(project,settings,root,true)); finish(build); assert(build.phase() == BuildPhase::Ready); assert(build.exitCode() == 0);
  if (csharp) assert(build.log().find("rendered three OpenGL frames") != std::string::npos);
  assert(build.run(project)); std::this_thread::sleep_for(std::chrono::milliseconds(300));
  build.cancel(); finish(build); assert(build.phase() == BuildPhase::Cancelled); assert(!build.canRun(project));
  // Inject a standard preprocessor error while configure is still running.
  // #line makes its reported location a real generated widget range.
  assert(build.build(project,root,settings));
  const auto headerPath = build.sourceDirectory() / (csharp ? "StudioUI.cs" : c ? "studio_ui.h" : "ui.hpp");
  const auto header = readTextFile(headerPath,ProjectJson::kMaxProjectBytes); assert(header);
  const auto generated = csharp ? generateCSharpCode(project) : c ? generateCCode(project) : generateCppDetailed(project);
  const auto line = generated.nodeLines.at(button.id).first;
  const auto mark = "\n#line "+std::to_string(line)+" "+CppCodegen::quote(headerPath.generic_u8string())+"\n#error studio_widget_error\n";
  assert(writeTextFileAtomically(header.text+mark,headerPath));
  finish(build); assert(build.phase() == BuildPhase::Failed && !build.canRun(project));
  bool mappedError = false;
  for (const auto& diagnostic : build.diagnostics()) if (diagnostic.message.find("studio_widget_error") != std::string::npos) {
    assert(diagnostic.origin == DiagnosticOrigin::Compiler && diagnostic.severity == DiagnosticSeverity::Error);
    assert(diagnostic.nodeId == button.id && diagnostic.line == line);
    if (csharp) assert(diagnostic.code == "CS1029");
    const auto snapshot = build.diagnosticSource(diagnostic); assert(snapshot && snapshot->find("studio_widget_error") == std::string::npos);
    mappedError = true;
  }
  assert(mappedError);
  if (csharp) {
    std::cout << "Real NuGet restore/build, graphical smoke, cancellation and Roslyn widget mapping verified.\n";
    return 0;
  }
  // A real configure failure is surfaced with its output and no runnable artifact.
  settings.generator = "Invalid generator for ImGui Studio test";
  assert(build.build(project,root,settings)); finish(build);
  assert(build.phase() == BuildPhase::Failed && !build.canRun(project));
  assert(build.log().find("Invalid generator") != std::string::npos);
  bool cmakeError = false;
  for (const auto& diagnostic : build.diagnostics()) if (diagnostic.origin == DiagnosticOrigin::CMake) cmakeError = true;
  assert(cmakeError);
  std::cout << "Real CMake configure/build, generated Run/smoke, stale design, cancel and failure verified.\n";
}
