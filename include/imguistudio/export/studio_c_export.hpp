#pragma once
#include "studio_cpp_export.hpp"

namespace ImGuiStudio {
inline GeneratedCpp generateCCode(const StudioProject& project) {
  using namespace CppCodegen;
  Emitter e(true);
  try {
    const auto valid = projectFromJson(projectToJson(project));
    if (!valid) throw std::runtime_error(valid.error);
    e.result.stateType = sanitizeIdent(project.name,"StudioUI")+"State";
    e.used.insert(e.result.stateType);
    e.collect(project.nodes);
    e.line(R"C(#ifndef STUDIO_EXPORT_UI_H
#define STUDIO_EXPORT_UI_H
#include <stdbool.h>
#include <stddef.h>
#include <float.h>
#include <math.h>
#ifndef CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#endif
#include "cimgui.h"
#define STUDIO_VEC2(x,y) ((ImVec2){(x),(y)})
#define STUDIO_VEC4(x,y,z,w) ((ImVec4){(x),(y),(z),(w)})
)C");
    if (e.hasImageWidgets) e.line(imageTypes);
    e.emitImageState();
    if (e.result.hasTitleButtons) e.line(std::string("#define STUDIO_EXPORT_CHROME\n") + titleBarFunctions + "\n#undef STUDIO_EXPORT_CHROME");
    e.line("typedef struct "+e.result.stateType+" {");
    if (e.cFields.empty()) e.line("    int unused;");
    for (const auto& field : e.cFields) e.line("    "+field);
    e.line("} "+e.result.stateType+";");
    e.line("static inline void InitStudioState("+e.result.stateType+"* state) {");
    e.line("    static const "+e.result.stateType+" defaults = {");
    if (e.cInitializers.empty()) e.line("        0");
    for (const auto& field : e.cInitializers) e.line("        "+field);
    e.line("    };");
    e.line("    *state = defaults;");
    e.line("}");
    e.style(project);
    e.modelFunctions();
    e.chartFunctions();
    e.emitAnimationFunctions();
    e.emitImageFunctions();
    e.emitTitleCallbacks();
    e.line("static inline void RenderStudioUI("+e.result.stateType+"* state) {");
    e.line("    (void)state;");
    e.repeatBudget();
    e.resetFrameButtons();
    for (const auto& node : project.nodes) e.emit(node,1);
    e.line("}");
    e.line("#undef STUDIO_VEC2\n#undef STUDIO_VEC4\n#endif");
    std::ostringstream code;
    for (const auto& line : e.lines) code << line << '\n';
    for (const auto& d : e.result.diagnostics) code << "// [" << comment(d.nodeId) << "] " << comment(d.message) << '\n';
    e.result.code = code.str();
  } catch (const std::exception& error) { e.result.error = error.what(); }
  return e.result;
}

inline CppBundle generateCBundle(const StudioProject& project) {
  CppBundle bundle;
  bundle.generated = generateCCode(project);
  const auto& generated = bundle.generated;
  if (!generated.error.empty()) return bundle;
  std::string report = "C11 / cimgui 1.91.6\n";
  for (const auto& d : generated.diagnostics)
    report += "["+CppCodegen::comment(d.nodeId)+"] "+CppCodegen::comment(d.message)+"\n";
  report += "\nRangos en studio_ui.h (inclusivos):\n";
  for (const auto& range : generated.nodeLines)
    report += CppCodegen::comment(range.first)+": "+std::to_string(range.second.first)+"-"+std::to_string(range.second.second)+"\n";
  std::string main = R"C(#include "studio_ui.h"
#include <GLFW/glfw3.h>
#include <string.h>

bool StudioBackendInit(GLFWwindow* window, const char* glsl);
void StudioBackendNewFrame(void);
void StudioBackendRender(void);
void StudioBackendShutdown(void);
)C";
  if (generated.hasImageTextures) main += CppCodegen::imageOpenGL;
  main += R"C(
int main(int argc, char** argv) {
    const bool smoke = argc == 2 && strcmp(argv[1], "--smoke-test") == 0;
    if (!glfwInit()) return 1;
#if defined(__APPLE__)
    const char* glsl = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    const char* glsl = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    if (smoke) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
)C";
  main += "    GLFWwindow* window = glfwCreateWindow("+std::to_string(project.canvas.width)+", "+
    std::to_string(project.canvas.height)+", "+CppCodegen::quote(project.name.substr(0,project.name.find('\0')))+", NULL, NULL);\n";
  main += R"C(    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    ImGuiContext* context = igCreateContext(NULL);
    if (smoke) igGetIO()->IniFilename = NULL;
    if (!StudioBackendInit(window, glsl)) {
        igDestroyContext(context);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    ApplyStudioStyle();
)C";
  main += "    static "+generated.stateType+" state;\n";
  main += "    InitStudioState(&state);\n";
  if (generated.hasImageTextures)
    main += "    size_t image_bytes = 0;\n"
            "    const bool images_loaded = LoadStudioImages(&state, StudioUploadOpenGLImage, &image_bytes);\n";
  main += R"C(    int frames = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        StudioBackendNewFrame();
        igNewFrame();
        RenderStudioUI(&state);
        igRender();
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        StudioBackendRender();
        glfwSwapBuffers(window);
        if (smoke && ++frames == 3) break;
    }
)C";
  if (generated.hasImageTextures) main += "    ReleaseStudioImages(&state, StudioReleaseOpenGLImage, &image_bytes);\n";
  main += R"C(    StudioBackendShutdown();
    igDestroyContext(context);
    glfwDestroyWindow(window);
    glfwTerminate();
)C";
  main += generated.hasImageTextures ? "    return smoke && (frames != 3 || !images_loaded) ? 1 : 0;\n}\n"
                                     : "    return smoke && frames != 3 ? 1 : 0;\n}\n";
  bundle.files = {
    {"studio_ui.h",generated.code},
    {"main.c",main},
    {"backend.cpp",R"CPP(#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

extern "C" bool StudioBackendInit(GLFWwindow* window, const char* glsl) {
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) return false;
    if (ImGui_ImplOpenGL3_Init(glsl)) return true;
    ImGui_ImplGlfw_Shutdown();
    return false;
}
extern "C" void StudioBackendNewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}
extern "C" void StudioBackendRender() {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
extern "C" void StudioBackendShutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}
)CPP"},
    {"CMakeLists.txt",R"CMAKE(cmake_minimum_required(VERSION 3.16)
project(StudioExportC LANGUAGES C CXX)
include(FetchContent)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0" AND NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM "3.5")
endif()
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
  URL https://codeload.github.com/glfw/glfw/tar.gz/refs/tags/3.4
  URL_HASH SHA256=c038d34200234d071fae9345bc455e4a8f2f544ab60150765d7704e08f3dac01)
FetchContent_Declare(cimgui
  URL https://codeload.github.com/cimgui/cimgui/tar.gz/e3b48a15f098d8f569f266abd96807094f827624
  URL_HASH SHA256=e2c119ee3ee571bd8267277eb048c0c596d6b3c2c7377a53bb16d51ba6fa9c3d
  SOURCE_SUBDIR studio_unused)
FetchContent_Declare(imgui
  URL https://codeload.github.com/ocornut/imgui/tar.gz/993fa347495860ed44b83574254ef2a317d0c14f
  URL_HASH SHA256=a543367a7b45959d7ea21eeac808e26d3b2264ebc6d59b5046135791e97f4ca9)
FetchContent_MakeAvailable(glfw cimgui imgui)
find_package(OpenGL REQUIRED)

# cimgui's source expects Dear ImGui in its imgui/ submodule.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/binding/imgui")
configure_file("${cimgui_SOURCE_DIR}/cimgui.cpp" binding/cimgui.cpp COPYONLY)
configure_file("${cimgui_SOURCE_DIR}/cimgui.h" binding/cimgui.h COPYONLY)
foreach(header imgui.h imgui_internal.h imconfig.h imstb_textedit.h imstb_truetype.h imstb_rectpack.h)
  configure_file("${imgui_SOURCE_DIR}/${header}" "binding/imgui/${header}" COPYONLY)
endforeach()
add_library(studio_c_backend STATIC backend.cpp
  "${CMAKE_CURRENT_BINARY_DIR}/binding/cimgui.cpp"
  "${imgui_SOURCE_DIR}/imgui.cpp" "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp" "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/imgui_demo.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
target_compile_features(studio_c_backend PRIVATE cxx_std_17)
target_include_directories(studio_c_backend PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/binding"
  PRIVATE "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
target_link_libraries(studio_c_backend PUBLIC glfw OpenGL::GL)
if(MSVC)
  target_compile_options(studio_c_backend PRIVATE /utf-8)
endif()
add_executable(studio_export_c main.c)
set_target_properties(studio_export_c PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
if(MSVC)
  target_compile_options(studio_export_c PRIVATE /utf-8)
endif()
target_link_libraries(studio_export_c PRIVATE studio_c_backend)
enable_testing()
add_test(NAME ExportCSmoke COMMAND studio_export_c --smoke-test)
)CMAKE"},
    {"codegen_report.txt",report},
    {"README.md","C11 / cimgui 1.91.6 / GLFW 3.4\n\n"
      "Configura con cmake -S . -B build y compila con cmake --build build --config Debug.\n"
      "Ejecuta studio_export_c o ctest --test-dir build -C Debug --output-on-failure.\n"
      "main.c y studio_ui.h son C11. CMake compila cimgui y los backends con C++.\n"
      "Se requieren compiladores C/C++, CMake y OpenGL. La primera configuracion descarga fuentes fijadas por SHA-256.\n"
      "Para integrar la interfaz llama a InitStudioState y ApplyStudioStyle y usa RenderStudioUI por frame.\n"
      "Consulta codegen_report.txt para las omisiones y los rangos por widget.\n"}
  };
  if (generated.hasImageTextures) {
    bundle.files["CMakeLists.txt"] += std::string(CppCodegen::imageCMake) +
      "target_include_directories(studio_export_c PRIVATE \"${stb_SOURCE_DIR}\")\n";
    bundle.files["README.md"] += imageExportReadme;
  }
  if (generated.hasTitleButtons) bundle.files["README.md"] += titleBarExportReadme;
  return bundle;
}

inline ProjectSaveResult exportCBundle(const StudioProject& project,const std::filesystem::path& destination) {
  const auto bundle = generateCBundle(project);
  if (!bundle.generated.error.empty()) return {false,bundle.generated.error};
  return writeGeneratedBundle(bundle,destination);
}
} // namespace ImGuiStudio
