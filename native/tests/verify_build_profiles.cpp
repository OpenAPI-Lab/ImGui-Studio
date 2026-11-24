#include "studio_build_profiles.hpp"
#include <cassert>
#include <iostream>
#include <thread>

using namespace ImGuiStudio;
namespace fs = std::filesystem;
static void write(const fs::path& path,const std::string& bytes) { assert(writeTextFileAtomically(bytes,path,2*kMaxBuildProfilesBytes)); }
int main(int argc,char** argv) {
  if (argc == 3 && std::string(argv[1]) == "--read") {
    BuildProfileStore fresh; assert(fresh.load(fs::u8path(argv[2]),"unused"));
    const auto& profile = fresh.document().active();
    assert(profile.name == "Release portable"); assert(profile.settings.configuration == "Release");
    assert(profile.settings.generator == "Ninja"); assert(profile.workspace == "workspace with spaces");
    assert(fresh.document().source.at("futureDoc").at("keep") == 42);
    std::cout << "fresh-process-ok\n"; return 0;
  }
  const auto root = fs::temp_directory_path() / nextId("studio_build_profiles_test");
  fs::create_directory(root); const auto path = root / "profiles.json";
  BuildProfileStore first,second;
  assert(first.load(path,"workspace with spaces")); assert(!fs::exists(path));
  auto& profile = first.document().active(); profile.name = "Release portable";
  profile.settings.configuration = "Release"; profile.settings.generator = "Ninja";
  profile.source["futureProfile"] = Json::array({1,"x"}); profile.source["settings"] = Json{{"futureSetting",123.123456789}};
  first.document().source["futureDoc"] = Json{{"keep",42}};
  const auto id = profile.id; assert(first.save());
  assert(second.load(path,"ignored")); assert(second.document().activeId == id);
  assert(second.document().active().source.at("settings").at("futureSetting") == 123.123456789);
  // A new OS process reads the saved active profile; no globals/session memory.
  NativeProcess process; assert(process.start({fs::absolute(fs::u8path(argv[0])),root,{"--read",path.u8string()}}));
  std::string output; const auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(10);
  while (process.running() && std::chrono::steady_clock::now()<deadline) {
    process.poll(); output += process.takeOutput(); std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  assert(!process.running() && process.exitCode() == 0 && output.find("fresh-process-ok") != std::string::npos);
  auto copy = second.document().active(); copy.id = nextId("copy"); copy.name = "Local debug"; copy.settings.configuration = "Debug";
  copy.settings.target = BuildTarget::C;
  copy.settings.cCompiler = "gcc";
  copy.settings.cimguiSource = "cimgui source";
  copy.settings.cImguiSource = "matched imgui source";
  second.document().profiles.push_back(copy); second.document().activeId = copy.id; assert(second.save());
  const auto changed = readTextFile(path,kMaxBuildProfilesBytes).text;
  first.document().active().name = "stale";
  assert(!first.save()); assert(readTextFile(path,kMaxBuildProfilesBytes).text == changed);
  assert(first.load(path,"ignored")); assert(first.document().active().name == "Local debug");
  assert(first.document().active().settings == copy.settings);
  { SessionFileLease locked(sessionLockPath(path)); assert(locked); assert(!first.save()); }
  assert(first.save());
  // Defaults only create files on save; racing creators cannot replace each other.
  BuildProfileStore creatorA,creatorB; const auto newPath = root / "new.json";
  assert(creatorA.load(newPath,"a") && creatorB.load(newPath,"b"));
  assert(creatorA.save()); assert(!creatorB.save());
  // Strict known schema, bounded inputs, and opaque unknown properties.
  const auto valid = BuildProfileJson::documentJson(first.document());
  auto legacy = valid;
  for (auto& entry : legacy["profiles"]) {
    for (const auto* key : {"target","cCompiler","cimguiSource","cImguiSource","cppRenderer","vulkanRoot"}) entry["settings"].erase(key);
  }
  assert(BuildProfileJson::readDocument(legacy).active().settings.target == BuildTarget::Cpp);
  assert(BuildProfileJson::readDocument(legacy).active().settings.cppRenderer == CppRenderer::OpenGL);
  auto vulkan = first.document().active();
  vulkan.settings.target = BuildTarget::Cpp;
  vulkan.settings.cppRenderer = CppRenderer::Vulkan;
  vulkan.settings.vulkanRoot = "Local Vulkan SDK";
  const auto roundTrip = BuildProfileJson::readProfile(BuildProfileJson::profileJson(vulkan));
  assert(roundTrip.settings == vulkan.settings);
  auto opengl = vulkan.settings;
  opengl.cppRenderer = CppRenderer::OpenGL;
  assert(!(opengl == vulkan.settings));
  auto different = vulkan.settings;
  different.vulkanRoot += "/other";
  assert(!(different == vulkan.settings));
  different.cppRenderer = CppRenderer::OpenGL;
  assert(different == opengl);
  auto managed = copy;
  managed.settings.target = BuildTarget::CSharp;
  managed.settings.dotnet = "custom SDK/dotnet";
  const auto managedRoundTrip = BuildProfileJson::readProfile(BuildProfileJson::profileJson(managed));
  assert(managedRoundTrip.settings == managed.settings);
  assert(managedRoundTrip.settings.cCompiler == managed.settings.cCompiler);
  auto interpreted = managed;
  interpreted.settings.target = BuildTarget::Python;
  interpreted.settings.python = "CPython path/python";
  interpreted.settings.pythonBinding = "binding path";
  const auto pythonRoundTrip = BuildProfileJson::readProfile(BuildProfileJson::profileJson(interpreted));
  assert(pythonRoundTrip.settings == interpreted.settings);
  assert(pythonRoundTrip.settings.dotnet == managed.settings.dotnet);
  auto java = interpreted;
  java.settings.target = BuildTarget::Java;
  java.settings.jdkHome = "Java path/jdk";
  java.settings.javaBinding = "Java jars";
  const auto javaRoundTrip = BuildProfileJson::readProfile(BuildProfileJson::profileJson(java));
  assert(javaRoundTrip.settings == java.settings);
  assert(javaRoundTrip.settings.python == interpreted.settings.python);
  auto rust = java;
  rust.settings.target = BuildTarget::Rust;
  rust.settings.cargo = "Rust toolchain/cargo";
  rust.settings.rustOffline = true;
  const auto rustRoundTrip = BuildProfileJson::readProfile(BuildProfileJson::profileJson(rust));
  assert(rustRoundTrip.settings == rust.settings);
  assert(rustRoundTrip.settings.jdkHome == java.settings.jdkHome);
  auto invalid = valid; invalid["schemaVersion"] = 99; write(path,invalid.dump());
  assert(!first.load(path,"fallback")); assert(!first.writable()); assert(!first.save());
  assert(BuildProfileJson::documentJson(first.document()) == valid);
  assert(readTextFile(path,kMaxBuildProfilesBytes).text == invalid.dump());
  write(path,"{broken"); assert(!first.load(path,"fallback")); assert(!first.save()); assert(readTextFile(path,kMaxBuildProfilesBytes).text == "{broken");
  for (int which=0;which<8;++which) {
    invalid = valid;
    if (which==0) invalid["activeProfile"] = "absent";
    if (which==1) invalid["profiles"].push_back(invalid["profiles"][0]);
    if (which==2) invalid["profiles"][0]["settings"]["cmake"] = 123;
    if (which==3) invalid["profiles"][0]["settings"]["configuration"] = "Invalid";
    if (which==4) invalid["profiles"][0]["workspace"] = std::string("a\0b",3);
    if (which==5) invalid["profiles"][0]["name"] = std::string(8193,'x');
    if (which==6) invalid["profiles"] = Json::array();
    if (which==7) invalid["profiles"][0]["settings"]["target"] = "future-target";
    write(path,invalid.dump()); assert(!first.load(path,"fallback")); assert(!first.save());
    assert(readTextFile(path,kMaxBuildProfilesBytes).text == invalid.dump());
  }
  write(path,std::string(kMaxBuildProfilesBytes+1,'x')); assert(!first.load(path,"fallback")); assert(!first.save());
  write(path,valid.dump()); assert(first.load(path,"fallback"));
  first.document().profiles[0].name.clear(); const auto beforeInvalidSave = readTextFile(path,kMaxBuildProfilesBytes).text;
  assert(!first.save()); assert(readTextFile(path,kMaxBuildProfilesBytes).text == beforeInvalidSave);

  auto project = createDefaultProject("Opaque build config");
  project.source = projectToJson(project);
  project.source["buildConfig"] = Json{{"command","echo must-not-execute"},{"python",Json{{"runtime","custom"}}},{"future",9.123456789}};
  const auto original = projectToJson(project);
  assert(!projectBuildProfile(project)); assert(projectToJson(project) == original);
  const auto attached = attachBuildProfile(project,second.document().active()); assert(attached.project);
  const auto imported = projectBuildProfile(*attached.project); assert(imported);
  assert(imported.profile->settings == second.document().active().settings);
  const auto attachedJson = projectToJson(*attached.project);
  for (const auto& key : {"command","python","future"}) assert(attachedJson["buildConfig"][key] == original["buildConfig"][key]);
  assert(projectToJson(project) == original);
  auto withExtras = attachedJson;
  withExtras["buildConfig"]["nativeCpp"]["keep"] = "namespace";
  withExtras["buildConfig"]["nativeCpp"]["profile"]["keep"] = "profile";
  withExtras["buildConfig"]["nativeCpp"]["profile"]["settings"]["keep"] = "settings";
  const auto loaded = projectFromJson(withExtras); assert(loaded);
  const auto reattached = attachBuildProfile(*loaded.project,second.document().active()); assert(reattached.project);
  const auto kept = projectToJson(*reattached.project)["buildConfig"]["nativeCpp"];
  assert(kept["keep"] == "namespace" && kept["profile"]["keep"] == "profile" && kept["profile"]["settings"]["keep"] == "settings");
  withExtras["buildConfig"]["nativeCpp"]["schemaVersion"] = 2;
  const auto future = projectFromJson(withExtras); assert(future);
  assert(!projectBuildProfile(*future.project)); assert(!attachBuildProfile(*future.project,copy).project);
  assert(projectToJson(*future.project) == withExtras);
  auto opaque = project; opaque.source["buildConfig"] = "legacy opaque";
  assert(!attachBuildProfile(opaque,copy).project); assert(projectToJson(opaque)["buildConfig"] == "legacy opaque");
  // Only remove this test's freshly created tree inside the OS temporary directory.
  assert(fs::weakly_canonical(root).parent_path() == fs::weakly_canonical(fs::temp_directory_path()));
  fs::remove_all(root);
  std::cout << "Profiles persistence, fresh process, conflicts, corruption and project preservation verified.\n";
}
