#pragma once
#include "imguistudio/export/studio_c_export.hpp"
#include "imguistudio/export/studio_csharp_export.hpp"
#include "imguistudio/export/studio_python_export.hpp"
#include "imguistudio/export/studio_java_export.hpp"
#include "imguistudio/export/studio_rust_export.hpp"
#include "studio_process.hpp"
#include "studio_build_diagnostics.hpp"

namespace ImGuiStudio {
enum class BuildPhase { Idle, Configuring, Restoring, Building, Checking, CheckingJava, CheckingRust, Ready, Running, Stopping, Cancelled, Failed };
inline const char* buildPhaseLabel(BuildPhase phase) {
  switch (phase) {
    case BuildPhase::Idle: return "Sin compilar";
    case BuildPhase::Configuring: return "Configurando CMake";
    case BuildPhase::Restoring: return "Restaurando NuGet";
    case BuildPhase::Building: return "Compilando";
    case BuildPhase::Checking: return "Verificando Python";
    case BuildPhase::CheckingJava: return "Verificando JDK";
    case BuildPhase::CheckingRust: return "Verificando Cargo";
    case BuildPhase::Ready: return "Compilado";
    case BuildPhase::Running: return "Ejecutando";
    case BuildPhase::Stopping: return "Deteniendo";
    case BuildPhase::Cancelled: return "Cancelado";
    case BuildPhase::Failed: return "Error";
  }
  return "Error";
}
enum class BuildTarget { Cpp, C, CSharp, Python, Java, Rust };
inline const char* buildTargetLabel(BuildTarget target) {
  return target == BuildTarget::Rust ? "Rust" : target == BuildTarget::Java ? "Java" : target == BuildTarget::Python ? "Python" :
    target == BuildTarget::CSharp ? "C#" : target == BuildTarget::C ? "C11" : "C++";
}
inline const char* buildTargetId(BuildTarget target) {
  return target == BuildTarget::Rust ? "rust" : target == BuildTarget::Java ? "java" : target == BuildTarget::Python ? "python" :
    target == BuildTarget::CSharp ? "csharp" : target == BuildTarget::C ? "c" : "cpp";
}
struct BuildSettings {
  std::string cmake = "cmake", generator, compiler, configuration = "Debug";
  std::string glfwSource, imguiSource;
  BuildTarget target = BuildTarget::Cpp;
  CppRenderer cppRenderer = CppRenderer::OpenGL;
  std::string vulkanRoot;
  std::string cCompiler, cimguiSource, cImguiSource;
  std::string dotnet = "dotnet";
  std::string python = "python", pythonBinding;
  std::string jdkHome, javaBinding;
  std::string cargo;
  bool rustOffline = false;
  bool operator==(const BuildSettings& b) const {
    if (target != b.target) return false;
    if (target == BuildTarget::Java) return jdkHome == b.jdkHome && javaBinding == b.javaBinding;
    if (target == BuildTarget::Python) return python == b.python && pythonBinding == b.pythonBinding;
    if (configuration != b.configuration) return false;
    if (target == BuildTarget::Rust) return cargo == b.cargo && rustOffline == b.rustOffline;
    if (target == BuildTarget::CSharp) return dotnet == b.dotnet;
    return cmake == b.cmake && generator == b.generator && compiler == b.compiler && glfwSource == b.glfwSource &&
      (target == BuildTarget::C ? cCompiler == b.cCompiler && cimguiSource == b.cimguiSource && cImguiSource == b.cImguiSource
        : imguiSource == b.imguiSource && cppRenderer == b.cppRenderer &&
          (cppRenderer != CppRenderer::Vulkan || vulkanRoot == b.vulkanRoot));
  }
};
struct ToolchainDetection { std::filesystem::path cmake, compiler, cCompiler, dotnet, python; };
inline ToolchainDetection detectToolchain() {
  ToolchainDetection found; found.cmake = findExecutable("cmake"); found.dotnet = findExecutable("dotnet");
  for (const auto* name : {"c++","g++","clang++","cl"}) {
    found.compiler = findExecutable(name); if (!found.compiler.empty()) break;
  }
  for (const auto* name : {"cc","gcc","clang","cl"}) {
    found.cCompiler = findExecutable(name); if (!found.cCompiler.empty()) break;
  }
  for (const auto* name : {"python3","python"}) {
    found.python = findExecutable(name); if (!found.python.empty()) break;
  }
  return found;
}
inline std::filesystem::path normalizedBuildWorkspace(const std::filesystem::path& workspace) {
  auto path = std::filesystem::absolute(workspace).lexically_normal();
  while (path.filename().empty() && path != path.root_path()) path = path.parent_path();
  return path;
}

namespace RustBuild {
inline std::filesystem::path findSiblingCompiler(const std::filesystem::path& cargo) {
  if (cargo.empty()) return {};
#if defined(_WIN32)
  constexpr auto name = "rustc.exe";
#else
  constexpr auto name = "rustc";
#endif
  return findExecutable(cargo.parent_path() / name);
}

inline std::filesystem::path findCargo(const std::string& configured) {
  if (!configured.empty()) return findExecutable(std::filesystem::u8path(configured));
  if (auto path = findExecutable("cargo"); !path.empty()) return path;
#if defined(_WIN32)
  constexpr auto homeName = "USERPROFILE";
  constexpr auto executable = "cargo.exe";
#else
  constexpr auto homeName = "HOME";
  constexpr auto executable = "cargo";
#endif
  if (const auto* cargoHome = std::getenv("CARGO_HOME"); cargoHome && *cargoHome)
    if (auto path = findExecutable(std::filesystem::u8path(cargoHome)/"bin"/executable); !path.empty()) return path;
  if (const auto* home = std::getenv(homeName); home && *home)
    return findExecutable(std::filesystem::u8path(home)/".cargo"/"bin"/executable);
  return {};
}

class Messages {
 public:
  template<class Handler>
  void feed(const std::string& chunk, Handler handle) {
    for (const auto c : chunk) {
      if (c == '\n') finish(handle);
      else if (!dropping_) {
        if (pending_.size() >= 256 * 1024) { pending_.clear(); dropping_ = true; }
        else pending_ += c;
      }
    }
  }

  template<class Handler>
  void finish(Handler handle) {
    if (dropping_) handle(nullptr, "[Mensaje de Cargo demasiado largo]\n");
    else if (!pending_.empty()) {
      if (pending_.front() == '{') {
        Json message;
        try {
          message = Json::parse(pending_, [](int depth, Json::parse_event_t, Json&) {
            if (depth > 64) throw std::runtime_error("Mensaje de Cargo demasiado anidado");
            return true;
          }, false);
        } catch (const std::exception&) { /* Retain a readable marker for malformed output. */ }
        if (message.is_object()) handle(&message, {});
        else handle(nullptr, "[Mensaje JSON de Cargo invalido]\n");
      } else handle(nullptr, pending_ + "\n");
    }
    pending_.clear();
    dropping_ = false;
  }
 private:
  std::string pending_;
  bool dropping_ = false;
};
} // namespace RustBuild

namespace PythonBuild {
inline constexpr const char* check = R"PY(import sys
if sys.path and sys.path[0] == "":
    del sys.path[0]
from pathlib import Path
if sys.implementation.name != "cpython" or sys.version_info < (3, 10):
    sys.exit("Python: error: se requiere CPython 3.10 o posterior.")
binding, script = sys.argv[1:]
try:
    compile(Path(script).read_bytes(), script, "exec", dont_inherit=True)
except SyntaxError as error:
    sys.exit(f"{script}:{error.lineno or 1}:{error.offset or 1}: error: {error.msg}")
if binding:
    sys.path.insert(0, binding)
from importlib.metadata import version
try:
    installed = version("imgui-bundle")
    if installed != "1.93.0":
        sys.exit(f"Python: error: se requiere imgui-bundle 1.93.0, encontrado {installed}.")
    from imgui_bundle import imgui, hello_imgui
    if imgui.get_version() != "1.93.0" or not callable(hello_imgui.run):
        sys.exit("Python: error: API del binding incompatible con imgui-bundle 1.93.0.")
except Exception as error:
    sys.exit(f"Python: error: imgui-bundle 1.93.0 no se puede cargar en este interprete: {error}")
print(f"CPython {sys.version.split()[0]}: sintaxis y binding imgui-bundle {installed} verificados.")
)PY";
inline constexpr const char* run = R"PY(import sys
if sys.path and sys.path[0] == "":
    del sys.path[0]
import runpy
binding, script, *args = sys.argv[1:]
if binding:
    sys.path.insert(0, binding)
sys.argv = [script, *args]
runpy.run_path(script, run_name="__main__")
)PY";
}

namespace JavaBuild {
#if defined(_WIN32)
inline constexpr char classpathSeparator = ';';
inline constexpr const char* javaName = "java.exe";
inline constexpr const char* javacName = "javac.exe";
#else
inline constexpr char classpathSeparator = ':';
inline constexpr const char* javaName = "java";
inline constexpr const char* javacName = "javac";
#endif
struct Tools { std::filesystem::path java, javac; };
inline Tools findTools(const std::string& home) {
  if (!home.empty()) {
    const auto bin = std::filesystem::u8path(home) / "bin";
    return {findExecutable(bin / javaName),findExecutable(bin / javacName)};
  }
  const auto javac = findExecutable(javacName);
  if (javac.empty()) return {};
  std::error_code error;
  const auto resolved = std::filesystem::canonical(javac,error);
  const auto compiler = error ? javac : resolved;
  return {findExecutable(compiler.parent_path() / javaName),compiler};
}
}

class NativeBuild {
 public:
  BuildPhase phase() const { return phase_; }
  bool busy() const { return process_.running(); }
  const std::string& log() const { return log_; }
  const std::string& error() const { return error_; }
  std::optional<int> exitCode() const { return exit_; }
  const std::filesystem::path& sourceDirectory() const { return source_; }
  const std::filesystem::path& executable() const { return executable_; }
  const std::vector<BuildDiagnostic>& diagnostics() const { return diagnostics_; }
  const std::string* diagnosticSource(const BuildDiagnostic& diagnostic) const {
    for (const auto& entry : sourceTexts_)
      if (diagnosticPathKey(diagnostic.file) == diagnosticPathKey(source_ / entry.first)) return &entry.second;
    return nullptr;
  }
  bool matches(const StudioProject& project) const { return !fingerprint_.empty() && fingerprint_ == projectToJson(project).dump(); }
  bool canRun(const StudioProject& project) const { return !busy() && !executable_.empty() && matches(project); }
  bool canRun(const StudioProject& project,const BuildSettings& settings,const std::filesystem::path& workspace) const {
    try { return canRun(project) && settings_ == settings && !workspace.empty() && source_.parent_path() == normalizedBuildWorkspace(workspace); }
    catch (const std::exception&) { return false; }
  }
  bool run(const StudioProject& project,const BuildSettings& settings,const std::filesystem::path& workspace,bool smoke = false) {
    if (busy()) return false;
    if (!canRun(project,settings,workspace)) { error_ = "Compile el diseno con el perfil y directorio actuales antes de ejecutar"; return false; }
    return run(project,smoke);
  }

  bool build(const StudioProject& project, const std::filesystem::path& workspace, const BuildSettings& settings) {
    if (busy()) return false;
    phase_ = BuildPhase::Idle;
    error_.clear(); log_.clear(); exit_.reset(); executable_.clear(); fingerprint_.clear(); source_.clear();
    diagnostics_.clear(); sourceTexts_.clear(); nodeRanges_.clear();
    cargoMessages_ = {}; cargoArtifact_.clear();
    settings_ = settings;
    try {
      if (workspace.empty()) return fail("Elija un directorio de compilacion");
      if (settings.target != BuildTarget::Python && settings.target != BuildTarget::Java &&
          settings.configuration != "Debug" && settings.configuration != "Release" &&
          settings.configuration != "RelWithDebInfo" && settings.configuration != "MinSizeRel")
        return fail("Configuracion de compilacion no valida");
      const auto root = normalizedBuildWorkspace(workspace);
      if (settings.target == BuildTarget::Rust) {
        if (settings.configuration != "Debug" && settings.configuration != "Release")
          return fail("Rust admite configuraciones Debug y Release");
        cargo_ = RustBuild::findCargo(settings.cargo);
        if (cargo_.empty()) return fail("Cargo no encontrado. Configure el ejecutable de Cargo o instale Rust.");
        if (!prepareExport(project,root,generateRustBundle(project))) return false;
        return launch({cargo_,source_,{"--version"}},BuildPhase::CheckingRust);
      }
      if (settings.target == BuildTarget::Java) {
        javaTools_ = JavaBuild::findTools(settings.jdkHome);
        if (javaTools_.java.empty() || javaTools_.javac.empty())
          return fail("Se necesitan java y javac del mismo JDK. Configure la carpeta del JDK 17 o posterior.");
        if (settings.javaBinding.empty()) return fail("Indique la carpeta de JAR de imgui-java, LWJGL y sus nativos.");
        const auto jars = std::filesystem::absolute(std::filesystem::u8path(settings.javaBinding));
        if (!std::filesystem::is_directory(jars)) return fail("Directorio de JAR inexistente");
        if (root.u8string().find(JavaBuild::classpathSeparator) != std::string::npos ||
            jars.u8string().find(JavaBuild::classpathSeparator) != std::string::npos)
          return fail("La ruta contiene el separador de classpath de Java");
        const std::filesystem::directory_iterator end;
        const bool hasJar = std::any_of(std::filesystem::directory_iterator(jars),end,[](const auto& entry) {
          return entry.is_regular_file() && entry.path().extension() == ".jar";
        });
        if (!hasJar) return fail("La carpeta de dependencias Java no contiene JAR");
        javaJarClasspath_ = (jars / "*").generic_u8string();
        if (!prepareExport(project,root,generateJavaBundle(project))) return false;
        return launch({javaTools_.javac,source_,{"-version"}},BuildPhase::CheckingJava);
      }
      if (settings.target == BuildTarget::Python) {
        python_ = findExecutable(std::filesystem::u8path(settings.python));
        if (python_.empty()) return fail("Python no encontrado. Configure la ruta a CPython.");
        pythonBinding_.clear();
        if (!settings.pythonBinding.empty()) {
          const auto folder = std::filesystem::absolute(std::filesystem::u8path(settings.pythonBinding));
          if (!std::filesystem::is_directory(folder)) return fail("Directorio de imgui-bundle inexistente");
          pythonBinding_ = folder.u8string();
        }
        if (!prepareExport(project,root,generatePythonBundle(project))) return false;
        return launch(pythonCommand(PythonBuild::check,source_ / "main.py"),BuildPhase::Checking);
      }
      if (settings.target == BuildTarget::CSharp) {
        dotnet_ = findExecutable(std::filesystem::u8path(settings.dotnet));
        if (dotnet_.empty()) return fail("dotnet no encontrado. Configure la ruta a un SDK .NET compatible con net8.0.");
        if (!prepareExport(project,root,generateCSharpBundle(project))) return false;
        return launch({dotnet_,source_,{"restore","StudioExport.csproj","--packages",(root / "nuget-packages").u8string(),
          "--tl:off","--disable-build-servers"}},BuildPhase::Restoring);
      }
      cmake_ = findExecutable(std::filesystem::u8path(settings.cmake));
      if (cmake_.empty()) return fail("CMake no encontrado. Configure su ejecutable o PATH.");
      const bool c = settings.target == BuildTarget::C;
      const bool vulkan = !c && settings.cppRenderer == CppRenderer::Vulkan;
      const auto cppCompiler = findExecutable(std::filesystem::u8path(settings.compiler));
      if (!settings.compiler.empty() && cppCompiler.empty()) return fail("Compilador C++ no encontrado");
      const auto cCompiler = c ? findExecutable(std::filesystem::u8path(settings.cCompiler)) : std::filesystem::path{};
      if (c && !settings.cCompiler.empty() && cCompiler.empty()) return fail("Compilador C no encontrado");
      const std::pair<const char*,std::string> dependencies[] = {
        {"GLFW",settings.glfwSource},{"IMGUI",c ? settings.cImguiSource : settings.imguiSource},
        {"CIMGUI",c ? settings.cimguiSource : ""}
      };
      for (const auto& dependency : dependencies)
        if (!dependency.second.empty() && !std::filesystem::is_directory(std::filesystem::u8path(dependency.second)))
          return fail(std::string("Directorio de dependencia inexistente: ")+dependency.first);
      if (vulkan && !settings.vulkanRoot.empty() && !std::filesystem::is_directory(std::filesystem::u8path(settings.vulkanRoot)))
        return fail("No se encuentra el directorio del SDK de Vulkan.");
      if (!prepareExport(project,root,c ? generateCBundle(project) : generateCppBundle(project,settings.cppRenderer))) return false;
      ProcessCommand command{cmake_,source_,{"-S",source_.u8string(),"-B",build_.u8string(),"-DCMAKE_BUILD_TYPE=" + settings.configuration}};
      if (vulkan && !settings.vulkanRoot.empty())
        command.arguments.push_back("-DVulkan_ROOT=" + std::filesystem::absolute(std::filesystem::u8path(settings.vulkanRoot)).u8string());
      if (!settings.generator.empty()) { command.arguments.push_back("-G"); command.arguments.push_back(settings.generator); }
      if (!cppCompiler.empty()) command.arguments.push_back("-DCMAKE_CXX_COMPILER=" + cppCompiler.u8string());
      if (!cCompiler.empty()) command.arguments.push_back("-DCMAKE_C_COMPILER=" + cCompiler.u8string());
      for (const auto& dependency : dependencies)
        if (!dependency.second.empty())
          command.arguments.push_back(std::string("-DFETCHCONTENT_SOURCE_DIR_") + dependency.first + "=" +
            std::filesystem::absolute(std::filesystem::u8path(dependency.second)).u8string());
      return launch(command,BuildPhase::Configuring);
    } catch (const std::exception& error) { return fail(error.what()); }
  }
  bool run(const StudioProject& project, bool smoke = false) {
    if (busy()) return false;
    if (!canRun(project)) { error_ = "Compile el diseno actual antes de ejecutar"; return false; }
    std::error_code fileError;
    if (!std::filesystem::is_regular_file(executable_,fileError)) return fail("El artefacto compilado ya no existe o no es accesible");
    error_.clear(); exit_.reset();
    ProcessCommand command{executable_,source_,{}};
    if (settings_.target == BuildTarget::CSharp) command = {dotnet_,source_,{executable_.u8string()}};
    if (settings_.target == BuildTarget::Java) {
      command = {javaTools_.java,source_,{}};
#if defined(__APPLE__)
      command.arguments.push_back("-XstartOnFirstThread");
#endif
      command.arguments.insert(command.arguments.end(),{"--enable-native-access=ALL-UNNAMED",
        "-Djava.io.tmpdir="+(source_ / "tmp").u8string(),
        "-cp",build_.u8string()+JavaBuild::classpathSeparator+source_.u8string()+
          JavaBuild::classpathSeparator+javaJarClasspath_,"Main"});
    }
    if (settings_.target == BuildTarget::Python) {
      const auto current = readTextFile(executable_,ProjectJson::kMaxProjectBytes);
      if (!current || current.text != sourceTexts_.at("main.py")) {
        executable_.clear();
        return fail("La copia Python cambio. Vuelva a verificar el diseno con Build.");
      }
      command = pythonCommand(PythonBuild::run,executable_);
    }
    if (smoke) command.arguments.push_back("--smoke-test");
    return launch(command,BuildPhase::Running);
  }
  void cancel() {
    if (!busy()) return;
    phase_ = BuildPhase::Stopping; process_.cancel(); append("\nCancelacion solicitada\n");
  }
  void poll() {
    if (!busy()) return;
    process_.poll(); const auto output = process_.takeOutput();
    if (phase_ == BuildPhase::Building && settings_.target == BuildTarget::Rust) {
      auto handle = [this](const Json* message, const std::string& plain) { readCargoMessage(message, plain); };
      cargoMessages_.feed(output, handle);
      if (!busy()) cargoMessages_.finish(handle);
    } else {
      append(output);
      if (phase_ == BuildPhase::Configuring || phase_ == BuildPhase::Restoring || phase_ == BuildPhase::Building ||
          phase_ == BuildPhase::Checking || phase_ == BuildPhase::CheckingJava || phase_ == BuildPhase::CheckingRust)
        diagnosticStream_.feed(output,diagnostics_);
    }
    if (busy()) return;
    diagnosticStream_.finish(diagnostics_);
    exit_ = process_.exitCode();
    if (phase_ == BuildPhase::Stopping) {
      executable_.clear(); phase_ = BuildPhase::Cancelled; append("Proceso cancelado\n"); return;
    }
    append("\nSalida: " + std::to_string(exit_.value_or(-1)) + "\n");
    if (!process_.error().empty()) { fail(process_.error()); return; }
    if (!exit_ || *exit_ != 0) { fail("El proceso fallo. Consulte la salida del compilador o programa."); return; }
    if (phase_ == BuildPhase::Checking) {
      const auto script = source_ / "main.py";
      const auto current = readTextFile(script,ProjectJson::kMaxProjectBytes);
      if (!current || current.text != sourceTexts_.at("main.py")) { fail("La copia Python cambio durante la verificacion"); return; }
      executable_ = script;
      phase_ = BuildPhase::Ready;
      append("Run ejecutara main.py con el mismo interprete y binding.\n");
    } else if (phase_ == BuildPhase::CheckingJava) {
      std::error_code directoryError;
      std::filesystem::create_directory(source_ / "tmp",directoryError);
      if (directoryError) { fail("No se pudo crear la carpeta temporal de Java: "+directoryError.message()); return; }
      launch({javaTools_.javac,source_,{"-J-Duser.language=en","-J-Duser.country=US","-J-Dfile.encoding=UTF-8",
        "--release","17","-encoding","UTF-8","-proc:none","-cp",javaJarClasspath_,
        "-d",build_.u8string(),"Main.java","StudioUI.java"}},BuildPhase::Building);
    } else if (phase_ == BuildPhase::CheckingRust) {
      ProcessCommand command{cargo_,source_,{"build","--bin","studio-export","--target-dir",build_.u8string(),
        "--message-format=json","--color=never","--jobs","2"}};
      if (findExecutable("rustc").empty()) {
        const auto compiler = RustBuild::findSiblingCompiler(cargo_);
        if (!compiler.empty()) {
          command.arguments.push_back("--config");
          command.arguments.push_back("build.rustc=" + Json(compiler.u8string()).dump());
        }
      }
      if (settings_.configuration == "Release") command.arguments.push_back("--release");
      if (settings_.rustOffline) command.arguments.push_back("--offline");
      launch(command,BuildPhase::Building);
    } else if (phase_ == BuildPhase::Configuring) {
      launch({cmake_,source_,{"--build",build_.u8string(),"--config",settings_.configuration,"--parallel","2","--target",cmakeTarget()}},BuildPhase::Building);
    } else if (phase_ == BuildPhase::Restoring) {
      launch({dotnet_,source_,{"build","StudioExport.csproj","--no-restore","--configuration",settings_.configuration,
        "--output",build_.u8string(),"--tl:off","--disable-build-servers","-p:UseAppHost=false"}},BuildPhase::Building);
    } else if (phase_ == BuildPhase::Building) {
      if (settings_.target == BuildTarget::Rust) {
        std::error_code fileError;
        if (cargoArtifact_.empty() || !std::filesystem::is_regular_file(cargoArtifact_,fileError)) {
          fail("Cargo termino sin producir el ejecutable studio-export"); return;
        }
        executable_ = cargoArtifact_;
        phase_ = BuildPhase::Ready;
        append("Compilacion correcta. Run abrira el ejecutable Rust generado por Cargo.\n");
        return;
      }
      if (settings_.target == BuildTarget::Java) {
        std::error_code fileError;
        for (const auto* file : {"Main.class","StudioUI.class"})
          if (!std::filesystem::is_regular_file(build_ / file,fileError)) { fail(std::string("javac no produjo ")+file); return; }
        executable_ = build_ / "Main.class";
        phase_ = BuildPhase::Ready;
        append("Compilacion correcta. Run ejecutara Main con Java y los JAR del perfil.\n");
        return;
      }
      if (settings_.target == BuildTarget::CSharp) {
        std::error_code fileError;
        for (const auto* file : {"StudioExport.dll","StudioExport.runtimeconfig.json","StudioExport.deps.json"})
          if (!std::filesystem::is_regular_file(build_ / file,fileError)) { fail(std::string(".NET no produjo ")+file); return; }
        executable_ = build_ / "StudioExport.dll";
        phase_ = BuildPhase::Ready;
        append("Compilacion correcta. Run ejecutara StudioExport.dll mediante dotnet.\n");
        return;
      }
#if defined(_WIN32)
      const auto filename = std::string(cmakeTarget())+".exe";
#else
      const auto filename = cmakeTarget();
#endif
      for (const auto& candidate : {build_ / settings_.configuration / filename,build_ / filename}) {
        std::error_code fileError;
        if (std::filesystem::is_regular_file(candidate,fileError)) { executable_ = candidate; break; }
      }
      if (executable_.empty()) { fail(std::string("CMake termino sin producir ")+cmakeTarget()+" en una ruta reconocida"); return; }
      phase_ = BuildPhase::Ready; append("Compilacion correcta. Run ejecutara este proyecto exportado.\n");
    } else if (phase_ == BuildPhase::Running) phase_ = BuildPhase::Ready;
  }
 private:
  NativeProcess process_;
  BuildPhase phase_ = BuildPhase::Idle;
  BuildSettings settings_;
  std::filesystem::path source_, build_, executable_, cmake_, dotnet_, python_;
  std::filesystem::path cargo_, cargoArtifact_;
  std::string log_, error_, fingerprint_, pythonBinding_;
  JavaBuild::Tools javaTools_;
  std::string javaJarClasspath_;
  std::optional<int> exit_;
  DiagnosticStream diagnosticStream_;
  RustBuild::Messages cargoMessages_;
  std::vector<BuildDiagnostic> diagnostics_;
  std::map<std::string,std::string> sourceTexts_;
  std::map<std::string,std::pair<size_t,size_t>> nodeRanges_;
  const char* uiHeader() const {
    return settings_.target == BuildTarget::Rust ? "ui.rs" :
      settings_.target == BuildTarget::Java ? "StudioUI.java" : settings_.target == BuildTarget::Python ? "main.py" :
      settings_.target == BuildTarget::CSharp ? "StudioUI.cs" : settings_.target == BuildTarget::C ? "studio_ui.h" : "ui.hpp";
  }
  void readCargoMessage(const Json* event, const std::string& plain) {
    if (!event) {
      append(plain);
      diagnosticStream_.feed(plain, diagnostics_);
      return;
    }
    try {
      const auto kind = event->value("reason", std::string{});
      const auto manifest = std::filesystem::u8path(event->value("manifest_path", std::string{}));
      std::error_code pathError;
      const bool local = diagnosticPathKey(manifest) == diagnosticPathKey(source_ / "Cargo.toml") ||
        (!manifest.empty() && std::filesystem::equivalent(manifest, source_ / "Cargo.toml", pathError));
      if (kind == "compiler-artifact" && local && event->at("executable").is_string() &&
          event->at("target").value("name", std::string{}) == "studio-export") {
        const auto path = std::filesystem::u8path(event->at("executable").get<std::string>()).lexically_normal();
        if (!path.is_absolute()) return;
        const auto actual = std::filesystem::canonical(path);
        const auto prefix = diagnosticPathKey(std::filesystem::canonical(build_)) + '/';
        if (diagnosticPathKey(actual).rfind(prefix, 0) == 0) cargoArtifact_ = actual;
      } else if (kind == "compiler-message") {
        const auto& message = event->at("message");
        BuildDiagnostic d;
        const auto level = message.value("level", std::string{});
        d.severity = level == "warning" ? DiagnosticSeverity::Warning : level == "error" || level == "failure-note" ?
          DiagnosticSeverity::Error : DiagnosticSeverity::Note;
        d.message = message.value("message", std::string{});
        d.stage = buildPhaseLabel(phase_);
        if (message.at("rendered").is_string()) append(message.at("rendered").get<std::string>());
        else append(level + ": " + d.message + "\n");
        if (message.at("code").is_object()) d.code = message.at("code").value("code", std::string{});
        for (const auto& span : message.at("spans")) {
          if (!local || !span.value("is_primary",false)) continue;
          d.file = std::filesystem::u8path(span.value("file_name", std::string{}));
          if (d.file.is_relative()) d.file = source_ / d.file;
          d.file = d.file.lexically_normal();
          d.line = span.value("line_start",size_t{0});
          d.column = span.value("column_start",size_t{0});
          d.nodeId = diagnosticNode(d, source_, nodeRanges_, uiHeader());
          break;
        }
        appendBuildDiagnostic(diagnostics_, std::move(d));
      }
    } catch (const std::exception&) { append("[Mensaje JSON de Cargo invalido]\n"); }
  }
  ProcessCommand pythonCommand(const char* launcher,const std::filesystem::path& script) const {
    return {python_,source_,{"-E","-B","-u","-X","utf8","-c",launcher,pythonBinding_,script.u8string()}};
  }
  const char* cmakeTarget() const { return settings_.target == BuildTarget::C ? "studio_export_c" : "studio_export"; }
  bool prepareExport(const StudioProject& project,const std::filesystem::path& root,CppBundle bundle) {
    auto& generated = bundle.generated;
    if (!generated.error.empty()) return fail(generated.error);
    std::filesystem::create_directories(root);
    source_ = root / nextId("project");
    const auto result = writeGeneratedBundle(bundle,source_);
    if (!result) return fail(result.error);
    build_ = source_ / "build";
    fingerprint_ = projectToJson(project).dump();
    nodeRanges_ = std::move(generated.nodeLines);
    for (auto& file : bundle.files)
      if (file.first != "README.md" && file.first != "codegen_report.txt")
        sourceTexts_.emplace(file.first,std::move(file.second));
    for (const auto& warning : generated.diagnostics) {
      if (diagnostics_.size() >= DiagnosticStream::maxDiagnostics) break;
      BuildDiagnostic d; d.severity = DiagnosticSeverity::Warning; d.origin = DiagnosticOrigin::Generator;
      d.message = warning.message; d.nodeId = warning.nodeId; d.stage = "Generacion";
      const auto range = nodeRanges_.find(d.nodeId);
      if (range != nodeRanges_.end()) { d.file = source_ / uiHeader(); d.line = range->second.first; }
      diagnostics_.push_back(std::move(d));
    }
    append(std::string("Proyecto exportado (") + buildTargetLabel(settings_.target) + "): " + source_.u8string() + "\n");
    append("Avisos de generacion: " + std::to_string(generated.diagnostics.size()) + ". Revisar codegen_report.txt.\n");
    return true;
  }
  void append(const std::string& value) {
    log_ += value;
    constexpr size_t limit = 1048576;
    if (log_.size() > limit) { log_.erase(0,log_.size()-limit); log_.replace(0,32,"[Salida anterior truncada]\n"); }
  }
  bool fail(const std::string& message) {
    error_ = message;
    BuildDiagnostic d; d.origin = DiagnosticOrigin::Process; d.message = message; d.stage = buildPhaseLabel(phase_); appendBuildDiagnostic(diagnostics_,std::move(d));
    phase_ = BuildPhase::Failed; append("ERROR: " + message + "\n"); return false;
  }
  bool launch(const ProcessCommand& command, BuildPhase phase) {
    append("\n> " + command.executable.u8string());
    for (const auto& arg : command.arguments) append(" " + CppCodegen::quote(arg));
    append("\n");
    if (!process_.start(command)) return fail(process_.error());
    const bool cmakeBuild = settings_.target == BuildTarget::Cpp || settings_.target == BuildTarget::C;
    const auto& directory = phase == BuildPhase::Building && cmakeBuild ? build_ : source_;
    diagnosticStream_.reset(directory,source_,nodeRanges_,buildPhaseLabel(phase),uiHeader());
    exit_.reset();
    phase_ = phase; return true;
  }
};
} // namespace ImGuiStudio
