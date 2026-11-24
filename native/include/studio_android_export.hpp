#pragma once
#include "studio_cpp_export.hpp"

namespace ImGuiStudio {

inline CppBundle generateAndroidCppBundle(const StudioProject& project, const std::filesystem::path& pack) {
  auto bundle = generateCppTemplateBundle(project, pack, "imguistudio-android-cpp/1",
    "C++17 / Android / Dear ImGui 1.91.5 / OpenGL ES 3",
    {"settings.gradle", "build.gradle", "AndroidManifest.xml", "CMakeLists.txt",
     "main.cpp", "README.md", "LICENSE.dear-imgui.txt"});
  if (!bundle.generated.error.empty()) return bundle;
  const bool images = bundle.generated.hasImageTextures;
  auto& main = bundle.files.at("main.cpp");
  const auto replace = [&main](std::string_view marker, std::string_view value) {
    return replaceCppTemplateMarker(main, marker, value);
  };
  if (!replace("StudioExportState state;", bundle.generated.stateType + " state;") ||
      !replace("// STUDIO_IMAGE_ADAPTER", images ? CppCodegen::imageOpenGL : "") ||
      !replace("// STUDIO_RELEASE_IMAGES", images ? "ReleaseStudioImages(state, StudioReleaseOpenGLImage, &imageBytes);" : "") ||
      !replace("// STUDIO_LOAD_IMAGES", images ?
        "if (!LoadStudioImages(state, StudioUploadOpenGLImage, &imageBytes))\n"
        "            __android_log_print(ANDROID_LOG_WARN, \"StudioExport\", \"Some images were unavailable.\");" : "") ||
      !replace("size_t imageBytes = 0;", images ? "size_t imageBytes = 0;" : "")) {
    bundle.generated.error = "main.cpp (imguistudio-android-cpp/1)";
    return bundle;
  }
  auto& cmake = bundle.files.at("CMakeLists.txt");
  if (images) cmake += std::string(CppCodegen::imageCMake) +
    "\ntarget_include_directories(studio_export PRIVATE \"${stb_SOURCE_DIR}\")\n";
  if (bundle.generated.hasKnobs) cmake += "\ntarget_sources(studio_export PRIVATE imgui-knobs.cpp)\n";
  return bundle;
}

inline ProjectSaveResult exportAndroidCppBundle(const StudioProject& project, const std::filesystem::path& pack,
                                               const std::filesystem::path& destination) {
  const auto bundle = generateAndroidCppBundle(project, pack);
  if (!bundle.generated.error.empty()) return {false, bundle.generated.error};
  return writeGeneratedBundle(bundle, destination);
}

} // namespace ImGuiStudio
