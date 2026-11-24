#pragma once
#include "studio_codegen.hpp"
#include "studio_project_io.hpp"
#include "studio_knobs_sources.hpp"
#include "studio_status_source.hpp"
#include "studio_codegen_vulkan.hpp"
#include "studio_vulkan_source.hpp"
#include <initializer_list>

namespace ImGuiStudio {
enum class CppRenderer { OpenGL, Vulkan };
inline const char* cppRendererLabel(CppRenderer renderer) {
  return renderer == CppRenderer::Vulkan ? "Vulkan" : "OpenGL";
}
struct CppBundle {
  GeneratedCpp generated;
  std::map<std::string, std::string> files;
};
inline constexpr char imageExportReadme[] = R"README(
Las imágenes PNG y JPEG están incrustadas en el código generado. CMake
obtiene stb_image para el ejecutable GLFW, que carga y libera las
texturas automáticamente. Si integras la interfaz con otro backend, llama
a LoadStudioImages con tu callback de subida de bytes PNG o JPEG, y a
ReleaseStudioImages con tu callback de liberación antes de cerrar ese
backend. Consulta el estado del botón de imagen después de RenderStudioUI.
El ejecutable usa hasta 128 MiB de texturas. Los recursos que no se puedan
cargar se muestran con un marcador.
)README";
inline constexpr char titleBarExportReadme[] = R"README(
Los botones de título son hijos directos de la ventana. El código usa la
barra de Dear ImGui de la versión incluida en el proyecto. Consulta el
campo booleano del botón después de RenderStudioUI. Se activa en el frame
del clic y se reinicia al iniciar el siguiente RenderStudioUI. Conecta
ese campo con la acción que corresponda en tu aplicación.
)README";
inline CppBundle generateCppInterfaceBundle(const StudioProject& project, const std::string& target) {
  CppBundle bundle;
  bundle.generated = generateCppDetailed(project);
  if (!bundle.generated.error.empty()) return bundle;
  bundle.files["ui.hpp"] = bundle.generated.code;
  std::string report = target + "\n";
  report += "Cobertura parcial. Revisar avisos y conectar acciones de la aplicacion.\n";
  for (const auto& diagnostic : bundle.generated.diagnostics)
    report += "[" + CppCodegen::comment(diagnostic.nodeId) + "] " + CppCodegen::comment(diagnostic.message) + "\n";
  report += "\nRangos de lineas en ui.hpp (inclusivos):\n";
  for (const auto& range : bundle.generated.nodeLines)
    report += CppCodegen::comment(range.first) + ": " + std::to_string(range.second.first) + "-" + std::to_string(range.second.second) + "\n";
  bundle.files["codegen_report.txt"] = std::move(report);
  if (bundle.generated.hasKnobs) {
    for (const auto& file : KnobsSource::files)
      bundle.files[std::string_view(file.name) == "LICENSE" ? "LICENSE.imgui-knobs.txt" : file.name] =
        std::string(file.contents);
  }
  if (bundle.generated.hasStatusWidgets) bundle.files["studio_status_widgets.hpp"] = statusWidgetSource;
  return bundle;
}

inline CppBundle generateCppTemplateBundle(const StudioProject& project, const std::filesystem::path& pack,
                                           const std::string& version, const std::string& target,
                                           std::initializer_list<const char*> templateFiles) {
  const auto descriptor = readTextFile(pack / "template.version", 128);
  if (!descriptor || (descriptor.text != version + "\n" && descriptor.text != version + "\r\n")) {
    CppBundle invalid;
    invalid.generated.error = "template.version (" + version + ")";
    return invalid;
  }
  auto bundle = generateCppInterfaceBundle(project, target);
  if (!bundle.generated.error.empty()) return bundle;
  constexpr size_t templateFileLimit = 64 * 1024;
  for (const char* name : templateFiles) {
    auto file = readTextFile(pack / name, templateFileLimit);
    if (!file || file.text.empty() || file.text.find('\0') != std::string::npos) {
      bundle.generated.error = name;
      return bundle;
    }
    bundle.files[name] = std::move(file.text);
  }
  return bundle;
}

inline bool replaceCppTemplateMarker(std::string& content, std::string_view marker, std::string_view value) {
  const auto position = content.find(marker);
  if (position == std::string::npos || content.find(marker, position + marker.size()) != std::string::npos) return false;
  content.replace(position, marker.size(), value);
  return true;
}

inline CppBundle generateCppBundle(const StudioProject& project, CppRenderer renderer = CppRenderer::OpenGL) {
  CppBundle bundle = generateCppInterfaceBundle(project,
    std::string("C++17 / Dear ImGui 1.91.5 / GLFW 3.4 / ") + cppRendererLabel(renderer));
  if (!bundle.generated.error.empty()) return bundle;
  const bool vulkan = renderer == CppRenderer::Vulkan;
  bundle.files["main.cpp"] = vulkan ? CppCodegen::vulkanMain(project, bundle.generated.stateType, bundle.generated.hasImageTextures) :
    "#ifndef NOMINMAX\n#define NOMINMAX\n#endif\n#include \"ui.hpp\"\n"
    "#include \"imgui_impl_glfw.h\"\n#include \"imgui_impl_opengl3.h\"\n"
    "#include <GLFW/glfw3.h>\n#include <cstring>\n" +
    CppCodegen::applicationMain(project, bundle.generated.stateType, bundle.generated.hasImageTextures);
  if (vulkan) {
    bundle.files["studio_vulkan.hpp"] = VulkanSource::renderer;
    if (bundle.generated.hasImageTextures) bundle.files["studio_vulkan_images.hpp"] = VulkanSource::images;
  }
  const char* dependency = vulkan ? R"CMAKE(
if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "The ImGui Studio Vulkan template supports Windows and Linux.")
endif()
find_package(Vulkan QUIET)
if(NOT Vulkan_FOUND)
  message(FATAL_ERROR "Vulkan headers and loader library were not found. Use the LunarG Vulkan SDK on Windows or libvulkan-dev on Ubuntu/Debian. Set Vulkan_ROOT to a custom SDK path and configure again.")
endif()
)CMAKE" : "";
  bundle.files["CMakeLists.txt"] = std::string(R"CMAKE(cmake_minimum_required(VERSION 3.16)
project(StudioExport LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0" AND NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM "3.5")
endif()
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
)CMAKE") + dependency + R"CMAKE(
include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
  URL https://codeload.github.com/glfw/glfw/tar.gz/refs/tags/3.4
  URL_HASH SHA256=c038d34200234d071fae9345bc455e4a8f2f544ab60150765d7704e08f3dac01)
FetchContent_Declare(imgui
  URL https://codeload.github.com/ocornut/imgui/tar.gz/refs/tags/v1.91.5
  URL_HASH SHA256=2aa2d169c569368439e5d5667e0796d09ca5cc6432965ce082e516937d7db254)
FetchContent_MakeAvailable(glfw imgui)
)CMAKE" + (vulkan ? "" : "find_package(OpenGL REQUIRED)\n") + R"CMAKE(
add_executable(studio_export main.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_)CMAKE" + (vulkan ? "vulkan" : "opengl3") + R"CMAKE(.cpp)
target_include_directories(studio_export PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(studio_export PRIVATE glfw )CMAKE" + (vulkan ? "Vulkan::Vulkan" : "OpenGL::GL") + R"CMAKE()
if(MSVC)
  target_compile_options(studio_export PRIVATE /utf-8)
endif()
if(APPLE)
  target_link_libraries(studio_export PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif(UNIX)
  target_link_libraries(studio_export PRIVATE ${CMAKE_DL_LIBS} pthread)
endif()
enable_testing()
add_test(NAME ExportSmoke COMMAND studio_export --smoke-test)
)CMAKE";
  bundle.files["README.md"] = std::string("# Proyecto C++ exportado con ") + cppRendererLabel(renderer) + "\n\n" +
    (vulkan ? R"README(Requiere compilador C++17, CMake >= 3.16 y GLFW con su entorno gráfico.
En Windows usa el SDK de Vulkan o los encabezados y la biblioteca de tu
toolchain. En Ubuntu/Debian usa libvulkan-dev. Ambas plataformas requieren
un controlador Vulkan que pueda presentar la ventana. Para un SDK ubicado
en otra carpeta, pasa -DVulkan_ROOT=/ruta/al/sdk al configurar.
)README" : "Requiere compilador C++17, CMake >= 3.16, OpenGL y dependencias de plataforma de GLFW.\n") +
    R"README(La primera configuración descarga Dear ImGui 1.91.5 y GLFW 3.4 con hashes verificados.

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Ejecutable: build/studio_export (Ninja/Make), build/studio_export.exe
(Ninja Windows) o build/Debug/studio_export.exe (Visual Studio).
--smoke-test dibuja tres fotogramas sin escribir imgui.ini.

ui.hpp contiene estado, ApplyStudioStyle y RenderStudioUI. main.cpp inicia
GLFW y el renderizador. Las fuentes se pueden compilar sin el editor ni
nlohmann/json. Incluir una
sola interfaz exportada por unidad de traduccion; los nombres de las funciones
publicas son comunes a las exportaciones. Cabecera compatible con varias TUs.
codegen_report.txt enumera omisiones y rangos de codigo por ID de nodo.
Los widgets no implementados se muestran como placeholders. Las acciones de
botones/menus requieren codigo de aplicacion. No se exportan credenciales AI,
historial ni el JSON original. UTF-8 en cadenas; fuentes/IME se configuran aparte.
)README";
  if (vulkan)
    bundle.files["README.md"] += "\nEn Wayland, la ventana de prueba se muestra brevemente para presentar los tres fotogramas.\n";
  if (bundle.generated.hasImageTextures) {
    bundle.files["CMakeLists.txt"] += std::string(CppCodegen::imageCMake) +
      "target_include_directories(studio_export PRIVATE \"${stb_SOURCE_DIR}\")\n";
    bundle.files["README.md"] += imageExportReadme;
  }
  if (bundle.generated.hasTitleButtons) bundle.files["README.md"] += titleBarExportReadme;
  if (bundle.generated.hasKnobs) {
    bundle.files["CMakeLists.txt"] += "target_sources(studio_export PRIVATE imgui-knobs.cpp)\n";
    bundle.files["README.md"] += std::string("\nPerilla: imgui-knobs, licencia MIT, revisión ") +
      KnobsSource::revision + ". Las fuentes y LICENSE.imgui-knobs.txt están junto a ui.hpp.\n";
  }
  if (bundle.generated.hasStatusWidgets) {
    bundle.files["README.md"] += "\nInterruptor, indicador de carga y progreso circular usan studio_status_widgets.hpp.\n";
  }
  return bundle;
}

// Exclusively creates a new directory. Never replaces an existing export.
// On interruption an incomplete directory may remain; choose a new destination.
inline ProjectSaveResult writeNewExportDirectory(const std::map<std::string,std::string>& files,
                                                 const std::filesystem::path& destination,
                                                 size_t fileLimit = ProjectJson::kMaxProjectBytes) {
  try {
    for (const auto& file : files) {
      const auto name = std::filesystem::u8path(file.first);
      if (name.empty() || name.has_root_path() || name.has_parent_path() || name == "." || name == ".." || file.first.find('\0') != std::string::npos)
        return {false,"Nombre de archivo de exportacion no valido"};
    }
    if (destination.empty() || !std::filesystem::create_directory(destination))
      return {false, "Elija una carpeta nueva dentro de un directorio existente"};
    std::vector<std::filesystem::path> written;
    for (const auto& file : files) {
      const auto target = destination / file.first;
      const auto result = writeTextFileAtomically(file.second, target, fileLimit);
      if (!result) {
        // Remove only the files this operation created; leave unrelated files alone.
        std::error_code ignored;
        for (const auto& path : written) std::filesystem::remove(path, ignored);
        std::filesystem::remove(destination, ignored);
        return result;
      }
      written.push_back(target);
    }
    return {true, {}};
  } catch (const std::exception& error) { return {false, error.what()}; }
}
inline ProjectSaveResult writeGeneratedBundle(const CppBundle& bundle, const std::filesystem::path& destination) {
  const size_t limit = ProjectJson::kMaxProjectBytes * (bundle.generated.hasImageTextures ? 2 : 1);
  return writeNewExportDirectory(bundle.files, destination, limit);
}
inline ProjectSaveResult exportCppBundle(const StudioProject& project, const std::filesystem::path& destination,
                                         CppRenderer renderer = CppRenderer::OpenGL) {
  const auto bundle = generateCppBundle(project, renderer);
  if (!bundle.generated.error.empty()) return {false, bundle.generated.error};
  return writeGeneratedBundle(bundle,destination);
}
} // namespace ImGuiStudio
