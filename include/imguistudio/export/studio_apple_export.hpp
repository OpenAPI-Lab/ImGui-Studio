#pragma once
#include "studio_cpp_export.hpp"

namespace ImGuiStudio {

enum class AppleCppTarget { MacOS, IOS };

inline CppBundle generateAppleCppBundle(const StudioProject& project, const std::filesystem::path& pack,
                                        AppleCppTarget target) {
  const bool ios = target == AppleCppTarget::IOS;
  auto bundle = generateCppTemplateBundle(project, pack, ios ? "imguistudio-ios-cpp/1" : "imguistudio-macos-cpp/1",
    ios ? "C++17 / iOS / Dear ImGui 1.91.5 / Metal" : "C++17 / macOS / Dear ImGui 1.91.5 / Metal",
    {"CMakeLists.txt", "Info.plist", "README.md", "main.mm", "studio_metal_images.hpp", "LICENSE.dear-imgui.txt"});
  if (!bundle.generated.error.empty()) return bundle;

  auto& cmake = bundle.files.at("CMakeLists.txt");
  if (!replaceCppTemplateMarker(cmake, "@STUDIO_APPLE_SYSTEM@", ios ? "iOS" : "Darwin") ||
      !replaceCppTemplateMarker(cmake, "@STUDIO_APPLE_DEPLOYMENT@", ios ? "15.0" : "12.0")) {
    bundle.generated.error = "CMakeLists.txt";
    return bundle;
  }
  const bool images = bundle.generated.hasImageTextures;
  bundle.files["studio_apple_config.hpp"] = "#pragma once\nusing StudioExportState = " +
    bundle.generated.stateType + ";\n#define STUDIO_APPLE_IMAGES " + (images ? "1\n" : "0\n");
  if (images) cmake += std::string(CppCodegen::imageCMake) +
    "\ntarget_include_directories(studio_export PRIVATE \"${stb_SOURCE_DIR}\")\n";
  else bundle.files.erase("studio_metal_images.hpp");
  if (bundle.generated.hasKnobs) cmake += "\ntarget_sources(studio_export PRIVATE imgui-knobs.cpp)\n";
  return bundle;
}

inline ProjectSaveResult exportAppleCppBundle(const StudioProject& project, const std::filesystem::path& pack,
                                              const std::filesystem::path& destination, AppleCppTarget target) {
  const auto bundle = generateAppleCppBundle(project, pack, target);
  if (!bundle.generated.error.empty()) return {false, bundle.generated.error};
  return writeGeneratedBundle(bundle, destination);
}

} // namespace ImGuiStudio
