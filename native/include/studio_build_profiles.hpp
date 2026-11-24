#pragma once
#include "studio_build.hpp"
#include "studio_session_store.hpp"
#include <set>

namespace ImGuiStudio {
inline constexpr size_t kMaxBuildProfilesBytes = 1048576;
inline constexpr size_t kMaxBuildProfiles = 32;
struct BuildProfile {
  std::string id = nextId("profile"), name = "C++ local", workspace;
  BuildSettings settings;
  Json source = Json::object();
};
struct BuildProfiles {
  std::string activeId;
  std::vector<BuildProfile> profiles;
  Json source = Json::object();
  BuildProfile& active() {
    for (auto& p : profiles) if (p.id == activeId) return p;
    throw std::runtime_error("Perfil activo inexistente");
  }
};
inline BuildProfiles defaultBuildProfiles(const std::string& workspace) {
  BuildProfiles doc; BuildProfile profile; profile.workspace = workspace;
  doc.activeId = profile.id; doc.profiles.push_back(profile); return doc;
}
namespace BuildProfileJson {
inline std::string text(const Json& j, const char* key, const std::string& fallback = {}) {
  if (!j.contains(key)) return fallback;
  ProjectJson::require(j.at(key).is_string(),std::string(key)+": se esperaba texto");
  auto value = j.at(key).get<std::string>();
  ProjectJson::require(value.size() <= 8192 && value.find('\0') == std::string::npos,std::string(key)+": texto excesivo o NUL");
  return value;
}
inline void version(const Json& j) {
  ProjectJson::require(j.is_object(),"Perfiles: se esperaba objeto");
  ProjectJson::require(j.contains("schemaVersion") && j.at("schemaVersion").is_number_integer() && j.at("schemaVersion") == 1,
    "Version de perfiles no compatible; archivo conservado sin reemplazar");
}
inline BuildProfile readProfile(const Json& j) {
  ProjectJson::require(j.is_object(),"Perfil: se esperaba objeto");
  BuildProfile p; p.source = j; p.id = text(j,"id"); p.name = text(j,"name"); p.workspace = text(j,"workspace");
  ProjectJson::require(!p.id.empty() && !p.name.empty(),"Perfil sin ID o nombre");
  ProjectJson::require(j.contains("settings") && j.at("settings").is_object(),"Perfil sin settings");
  const auto& s = j.at("settings");
  p.settings.cmake = text(s,"cmake","cmake"); p.settings.generator = text(s,"generator"); p.settings.compiler = text(s,"compiler");
  p.settings.configuration = text(s,"configuration","Debug");
  p.settings.glfwSource = text(s,"glfwSource"); p.settings.imguiSource = text(s,"imguiSource");
  const auto target = text(s,"target","cpp");
  ProjectJson::require(target == "cpp" || target == "c" || target == "csharp" || target == "python" ||
    target == "java" || target == "rust","Destino de compilacion no compatible");
  p.settings.target = target == "rust" ? BuildTarget::Rust : target == "java" ? BuildTarget::Java : target == "python" ? BuildTarget::Python :
    target == "csharp" ? BuildTarget::CSharp : target == "c" ? BuildTarget::C : BuildTarget::Cpp;
  const auto renderer = text(s,"cppRenderer","opengl");
  ProjectJson::require(renderer == "opengl" || renderer == "vulkan","Renderizador C++ no compatible");
  p.settings.cppRenderer = renderer == "vulkan" ? CppRenderer::Vulkan : CppRenderer::OpenGL;
  p.settings.vulkanRoot = text(s,"vulkanRoot");
  p.settings.dotnet = text(s,"dotnet","dotnet");
  p.settings.python = text(s,"python","python");
  p.settings.pythonBinding = text(s,"pythonBinding");
  p.settings.jdkHome = text(s,"jdkHome");
  p.settings.javaBinding = text(s,"javaBinding");
  p.settings.cargo = text(s,"cargo");
  if (s.contains("rustOffline")) {
    ProjectJson::require(s.at("rustOffline").is_boolean(),"rustOffline: se esperaba booleano");
    p.settings.rustOffline = s.at("rustOffline").get<bool>();
  }
  p.settings.cCompiler = text(s,"cCompiler");
  p.settings.cimguiSource = text(s,"cimguiSource");
  p.settings.cImguiSource = text(s,"cImguiSource");
  ProjectJson::require(!p.settings.cmake.empty(),"CMake no puede estar vacio");
  const auto& config = p.settings.configuration;
  ProjectJson::require(config == "Debug" || config == "Release" || config == "RelWithDebInfo" || config == "MinSizeRel","Configuracion de perfil no valida");
  return p;
}
inline Json profileJson(const BuildProfile& p) {
  Json j = p.source.is_object() ? p.source : Json::object();
  j["id"] = p.id; j["name"] = p.name; j["workspace"] = p.workspace;
  auto s = j.contains("settings") && j.at("settings").is_object() ? j.at("settings") : Json::object();
  s.update(Json{{"cmake",p.settings.cmake},{"generator",p.settings.generator},{"compiler",p.settings.compiler},
    {"configuration",p.settings.configuration},{"glfwSource",p.settings.glfwSource},{"imguiSource",p.settings.imguiSource},
    {"cppRenderer",p.settings.cppRenderer == CppRenderer::Vulkan ? "vulkan" : "opengl"},{"vulkanRoot",p.settings.vulkanRoot},
    {"target",buildTargetId(p.settings.target)},{"dotnet",p.settings.dotnet},{"python",p.settings.python},
    {"pythonBinding",p.settings.pythonBinding},{"jdkHome",p.settings.jdkHome},{"javaBinding",p.settings.javaBinding},{"cCompiler",p.settings.cCompiler},
    {"cimguiSource",p.settings.cimguiSource},{"cImguiSource",p.settings.cImguiSource},
    {"cargo",p.settings.cargo},{"rustOffline",p.settings.rustOffline}});
  j["settings"] = std::move(s); return j;
}
inline Json documentJson(const BuildProfiles& doc) {
  auto j = doc.source.is_object() ? doc.source : Json::object();
  j["schemaVersion"] = 1; j["activeProfile"] = doc.activeId; j["profiles"] = Json::array();
  for (const auto& p : doc.profiles) j["profiles"].push_back(profileJson(p));
  return j;
}
inline BuildProfiles readDocument(const Json& j) {
  version(j); BuildProfiles doc; doc.source = j; doc.activeId = text(j,"activeProfile");
  ProjectJson::require(j.contains("profiles") && j.at("profiles").is_array() && !j.at("profiles").empty() && j.at("profiles").size() <= kMaxBuildProfiles,"Cantidad de perfiles fuera de 1..32");
  std::set<std::string> ids;
  for (const auto& entry : j.at("profiles")) {
    auto p = readProfile(entry); ProjectJson::require(ids.insert(p.id).second,"ID de perfil duplicado"); doc.profiles.push_back(std::move(p));
  }
  (void)doc.active(); return doc;
}
inline Json parse(const std::string& bytes) {
  ProjectJson::require(bytes.size() <= kMaxBuildProfilesBytes,"Perfiles superan 1 MiB");
  return Json::parse(bytes,[](int depth,Json::parse_event_t,Json&) { ProjectJson::require(depth <= 64,"Perfiles demasiado anidados"); return true; });
}
}

// Load never writes. Save uses a lease plus exact-content conflict detection.
// Malformed/future schemas remain untouched and disable saving until reloaded.
class BuildProfileStore {
 public:
  bool load(const std::filesystem::path& path, const std::string& defaultWorkspace) {
    path_ = path; expected_.reset(); writable_ = false;
    error_.clear();
    try {
      auto candidate = defaultBuildProfiles(defaultWorkspace);
      path_ = std::filesystem::absolute(path_);
      if (std::filesystem::exists(path_)) {
        const auto input = readTextFile(path_,kMaxBuildProfilesBytes);
        if (!input) throw std::runtime_error(input.error);
        auto parsed = BuildProfileJson::readDocument(BuildProfileJson::parse(input.text));
        expected_ = input.text; candidate = std::move(parsed);
      }
      document_ = std::move(candidate);
      writable_ = true; return true;
    } catch (const std::exception& e) { error_ = e.what(); return false; }
  }
  ProjectSaveResult save() {
    try {
      if (!writable_ || path_.empty()) return {false,"Perfiles no guardables: " + error_};
      const auto value = BuildProfileJson::documentJson(document_);
      (void)BuildProfileJson::readDocument(value);
      const auto bytes = value.dump(2) + "\n";
      // Enforce size/depth before creating or replacing anything.
      (void)BuildProfileJson::parse(bytes);
      const auto parent = std::filesystem::absolute(path_).parent_path();
      std::filesystem::create_directories(parent);
      SessionFileLease lease(sessionLockPath(path_));
      if (!lease) return {false,"Otra sesion esta guardando perfiles"};
      const bool exists = std::filesystem::exists(path_);
      if (exists != expected_.has_value()) return {false,"Perfiles cambiaron fuera de esta sesion. Recargue antes de guardar."};
      if (exists) {
        const auto current = readTextFile(path_,kMaxBuildProfilesBytes);
        if (!current || current.text != *expected_) return {false,"Perfiles cambiaron fuera de esta sesion. Recargue antes de guardar."};
      }
      const auto result = writeTextFileAtomically(bytes,path_,kMaxBuildProfilesBytes);
      if (result) { expected_ = bytes; error_.clear(); }
      return result;
    } catch (const std::exception& e) { return {false,e.what()}; }
  }
  BuildProfiles& document() { return document_; }
  const BuildProfiles& document() const { return document_; }
  bool writable() const { return writable_; }
  const std::filesystem::path& path() const { return path_; }
  const std::string& error() const { return error_; }
 private:
  BuildProfiles document_ = defaultBuildProfiles((std::filesystem::current_path() / "build" / "studio-projects").u8string());
  std::filesystem::path path_;
  std::optional<std::string> expected_;
  bool writable_ = false;
  std::string error_;
};

struct ProjectBuildProfileResult {
  std::optional<BuildProfile> profile;
  std::string error;
  explicit operator bool() const { return profile.has_value(); }
};
inline ProjectBuildProfileResult projectBuildProfile(const StudioProject& project) {
  try {
    const auto j = projectToJson(project);
    ProjectJson::require(j.contains("buildConfig") && j.at("buildConfig").is_object() && j.at("buildConfig").contains("nativeCpp"),
      "No hay perfil C++ nativo en buildConfig.nativeCpp. La configuracion desconocida se conserva.");
    const auto& native = j.at("buildConfig").at("nativeCpp"); BuildProfileJson::version(native);
    ProjectJson::require(native.contains("profile"),"nativeCpp sin perfil");
    return {BuildProfileJson::readProfile(native.at("profile")),{}};
  } catch (const std::exception& e) { return {std::nullopt,e.what()}; }
}
struct ProjectBuildAttachResult { std::optional<StudioProject> project; std::string error; };
inline ProjectBuildAttachResult attachBuildProfile(const StudioProject& project,const BuildProfile& profile) {
  try {
    const auto value = BuildProfileJson::profileJson(profile); (void)BuildProfileJson::readProfile(value);
    auto j = projectToJson(project);
    if (!j.contains("buildConfig")) j["buildConfig"] = Json::object();
    auto& config = j["buildConfig"];
    ProjectJson::require(config.is_object(),"buildConfig no es un objeto; se conserva sin reemplazar");
    Json native = Json::object();
    if (config.contains("nativeCpp")) {
      const auto existing = projectBuildProfile(project);
      ProjectJson::require(static_cast<bool>(existing),existing.error);
      native = config.at("nativeCpp");
    }
    native["schemaVersion"] = 1;
    // Preserve unknown project-profile settings even when attaching a local one.
    auto merged = native.contains("profile") ? native.at("profile") : Json::object();
    auto settings = merged.contains("settings") ? merged.at("settings") : Json::object();
    settings.update(value.at("settings")); merged.update(value); merged["settings"] = std::move(settings);
    native["profile"] = std::move(merged); config["nativeCpp"] = std::move(native);
    auto loaded = projectFromJson(j);
    if (!loaded) return {std::nullopt,loaded.error};
    return {std::move(loaded.project),{}};
  } catch (const std::exception& e) { return {std::nullopt,e.what()}; }
}
} // namespace ImGuiStudio
