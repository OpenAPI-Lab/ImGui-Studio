set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(studio_core INTERFACE)
set(studio_knobs "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui-knobs")
file(READ "${studio_knobs}/LICENSE" STUDIO_KNOBS_LICENSE)
file(READ "${studio_knobs}/imgui-knobs.h" STUDIO_KNOBS_HEADER)
file(READ "${studio_knobs}/imgui-knobs.cpp" STUDIO_KNOBS_SOURCE)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${studio_knobs}/LICENSE" "${studio_knobs}/imgui-knobs.h" "${studio_knobs}/imgui-knobs.cpp")
configure_file(packaging/studio_knobs_sources.hpp.in generated/studio_knobs_sources.hpp @ONLY NEWLINE_STYLE UNIX)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/imguistudio/widgets/studio_status_widgets.hpp" STUDIO_STATUS_WIDGET_SOURCE)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/imguistudio/widgets/studio_status_widgets.hpp")
configure_file(packaging/studio_status_source.hpp.in generated/studio_status_source.hpp @ONLY NEWLINE_STYLE UNIX)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  templates/studio_vulkan.hpp templates/studio_vulkan_images.hpp)
file(READ templates/studio_vulkan.hpp studio_vulkan_renderer)
# MSVC limits each literal before concatenation. Split after a complete line
# ending so checkouts using CRLF also preserve the emitted source.
string(FIND "${studio_vulkan_renderer}" "    void idle() const" studio_vulkan_split)
string(SUBSTRING "${studio_vulkan_renderer}" 0 ${studio_vulkan_split} studio_vulkan_renderer_first)
string(SUBSTRING "${studio_vulkan_renderer}" ${studio_vulkan_split} -1 studio_vulkan_renderer_last)
file(READ templates/studio_vulkan_images.hpp studio_vulkan_images)
configure_file(packaging/studio_vulkan_source.hpp.in generated/studio_vulkan_source.hpp @ONLY NEWLINE_STYLE UNIX)
target_include_directories(studio_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include "${CMAKE_CURRENT_BINARY_DIR}/generated")

option(BUILD_STUDIO_GUI "Build the native GLFW/OpenGL editor" ON)
option(STUDIO_PACKAGING "Enable staged installation and CPack for the native editor" OFF)
set(studio_pack_directory "${CMAKE_CURRENT_BINARY_DIR}/optional-packs")
set(studio_android_pack "${studio_pack_directory}/ImGuiStudio-Android-Cpp-${PROJECT_VERSION}.zip")
set(studio_android_template "${CMAKE_CURRENT_SOURCE_DIR}/templates/android")

function(studio_add_template_pack target folder)
  set(archive "${studio_pack_directory}/ImGuiStudio-${folder}-${PROJECT_VERSION}.zip")
  set(entries)
  foreach(source IN LISTS ARGN)
    get_filename_component(name "${source}" NAME)
    list(APPEND entries "${folder}/${name}")
  endforeach()
  add_custom_command(OUTPUT "${archive}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${studio_pack_directory}/${folder}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${ARGN} "${studio_pack_directory}/${folder}"
    COMMAND "${CMAKE_COMMAND}" -E chdir "${studio_pack_directory}"
      "${CMAKE_COMMAND}" -E tar cf "${archive}" --format=zip ${entries}
    DEPENDS ${ARGN} VERBATIM)
  add_custom_target("${target}" DEPENDS "${archive}")
endfunction()

studio_add_template_pack(studio_android_pack Android-Cpp
  "${studio_android_template}/template.version"
  "${studio_android_template}/settings.gradle"
  "${studio_android_template}/build.gradle"
  "${studio_android_template}/AndroidManifest.xml"
  "${studio_android_template}/CMakeLists.txt"
  "${studio_android_template}/main.cpp"
  "${studio_android_template}/README.md"
  "${studio_android_template}/LICENSE.dear-imgui.txt")

function(studio_add_apple_pack target platform folder)
  set(common "${CMAKE_CURRENT_SOURCE_DIR}/templates/apple")
  studio_add_template_pack("${target}" "${folder}"
    "${common}/CMakeLists.txt" "${common}/main.mm" "${common}/studio_metal_images.hpp"
    "${common}/${platform}/Info.plist" "${common}/${platform}/template.version" "${common}/${platform}/README.md"
    "${studio_android_template}/LICENSE.dear-imgui.txt")
endfunction()
studio_add_apple_pack(studio_macos_pack macos macOS-Cpp)
studio_add_apple_pack(studio_ios_pack ios iOS-Cpp)

if(BUILD_STUDIO_GUI)
  # GLFW 3.4 predates CMake 4's removal of pre-3.5 policy compatibility.
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0" AND NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM "3.5")
  endif()
  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()
  include(FetchContent)

  # Fetch GLFW 3.4
  FetchContent_Declare(
    glfw
    URL https://codeload.github.com/glfw/glfw/tar.gz/refs/tags/3.4
    URL_HASH SHA256=c038d34200234d071fae9345bc455e4a8f2f544ab60150765d7704e08f3dac01
  )
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  if(STUDIO_PACKAGING)
    set(GLFW_INSTALL OFF)
  endif()
  FetchContent_MakeAvailable(glfw)

  # Fetch Dear ImGui v1.91.5
  FetchContent_Declare(
    imgui
    URL https://codeload.github.com/ocornut/imgui/tar.gz/refs/tags/v1.91.5
    URL_HASH SHA256=2aa2d169c569368439e5d5667e0796d09ca5cc6432965ce082e516937d7db254
  )
  FetchContent_MakeAvailable(imgui)

  FetchContent_Declare(
    stb
    URL https://codeload.github.com/nothings/stb/tar.gz/f0569113c93ad095470c54bf34a17b36646bbbb5
    URL_HASH SHA256=4d05c96640ae3a8cbdafdad8d344b50ce610802f78aee80154acdb8e266282e0
  )
  FetchContent_MakeAvailable(stb)

  find_package(OpenGL REQUIRED)

  add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${studio_knobs}/imgui-knobs.cpp
  )

  target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${studio_knobs}
  )

  target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

  add_library(studio_images STATIC src/widgets/studio_images.cpp)
  target_include_directories(studio_images PRIVATE ${stb_SOURCE_DIR})
  target_link_libraries(studio_images PUBLIC studio_core imgui_lib)

  find_package(Threads REQUIRED)
  add_library(studio_assistant STATIC src/services/studio_assistant_panel.cpp src/services/studio_assistant_http.cpp
    src/services/studio_assistant_vault.cpp src/services/studio_extensions.cpp)
  target_link_libraries(studio_assistant PUBLIC studio_core imgui_lib PRIVATE Threads::Threads)
  if(WIN32)
    target_link_libraries(studio_assistant PRIVATE advapi32)
  elseif(APPLE)
    find_library(STUDIO_SECURITY_FRAMEWORK Security REQUIRED)
    find_library(STUDIO_COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    target_link_libraries(studio_assistant PRIVATE ${STUDIO_SECURITY_FRAMEWORK} ${STUDIO_COREFOUNDATION_FRAMEWORK})
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(STUDIO_ASSISTANT_SECRET_SERVICE "Use the Linux Secret Service via libsecret when available" ON)
    if(STUDIO_ASSISTANT_SECRET_SERVICE)
      find_package(PkgConfig QUIET)
      if(PkgConfig_FOUND)
        pkg_check_modules(STUDIO_LIBSECRET QUIET IMPORTED_TARGET libsecret-1>=0.19.0)
      endif()
      if(TARGET PkgConfig::STUDIO_LIBSECRET)
        target_compile_definitions(studio_assistant PRIVATE STUDIO_ASSISTANT_LIBSECRET)
        target_link_libraries(studio_assistant PRIVATE PkgConfig::STUDIO_LIBSECRET)
        message(STATUS "Native assistant: Linux Secret Service enabled")
      else()
        message(STATUS "Native assistant: libsecret development files unavailable; system vault disabled")
      endif()
    endif()
  endif()
  option(STUDIO_ASSISTANT_NETWORK "Connect the native assistant using libcurl when available" ON)
  option(STUDIO_CATALOG_NETWORK "Download catalog extensions using libcurl when available" ON)
  if(STUDIO_ASSISTANT_NETWORK OR STUDIO_CATALOG_NETWORK)
    find_package(CURL 7.85 QUIET)
    if(CURL_FOUND)
      target_compile_definitions(studio_assistant PRIVATE STUDIO_HTTP_CURL
        $<$<BOOL:${STUDIO_ASSISTANT_NETWORK}>:STUDIO_ASSISTANT_CURL>
        $<$<BOOL:${STUDIO_CATALOG_NETWORK}>:STUDIO_CATALOG_CURL>)
      target_link_libraries(studio_assistant PRIVATE CURL::libcurl)
      message(STATUS "Studio HTTP: libcurl enabled")
    else()
      message(STATUS "Studio HTTP: libcurl development files unavailable; network disabled")
    endif()
  endif()

  add_executable(imgui_studio
    src/app/main.cpp
    src/editor/studio_ui.cpp
    src/editor/studio_preview.cpp
    src/editor/studio_canvas.cpp
  )
  set_target_properties(imgui_studio PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

  target_include_directories(imgui_studio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(imgui_studio PRIVATE imgui_lib studio_images)

  if(APPLE)
    target_link_libraries(imgui_lib PUBLIC "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
    set_target_properties(imgui_studio PROPERTIES
      MACOSX_BUNDLE TRUE
      OUTPUT_NAME "ImGui Studio"
      MACOSX_BUNDLE_BUNDLE_NAME "ImGui Studio"
      MACOSX_BUNDLE_GUI_IDENTIFIER "org.imguistudio.desktop"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
      MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
  elseif(WIN32)
    set_target_properties(imgui_studio PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(imgui_lib PUBLIC gdi32 opengl32 comdlg32)
  elseif(UNIX)
    target_link_libraries(imgui_lib PUBLIC ${CMAKE_DL_LIBS} pthread)
  endif()

  if(MSVC)
    target_compile_options(imgui_lib PRIVATE /utf-8)
    target_compile_options(studio_images PRIVATE /utf-8)
    target_compile_options(imgui_studio PRIVATE /utf-8)
  endif()

  target_link_libraries(imgui_studio PRIVATE studio_core studio_assistant)
  if(STUDIO_PACKAGING)
    include(packaging/StudioPackaging.cmake)
  endif()
endif()

# Standalone C++17 core verification test (no GUI dependencies)
option(BUILD_CORE_TEST "Build standalone C++17 core verification test" ON)
if(BUILD_CORE_TEST OR (BUILD_STUDIO_GUI AND STUDIO_PACKAGING))
  enable_testing()
endif()
if(BUILD_CORE_TEST)
  add_executable(verify_core tests/verify_core.cpp)
  target_link_libraries(verify_core PRIVATE studio_core)
  if(MSVC)
    target_compile_options(verify_core PRIVATE /utf-8)
  endif()
  add_test(NAME CoreAndCodegenTest COMMAND verify_core)
endif()

if(BUILD_CORE_TEST)
  add_executable(verify_persistence tests/verify_persistence.cpp)
  target_link_libraries(verify_persistence PRIVATE studio_core)
  target_compile_definitions(verify_persistence PRIVATE STUDIO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  if(MSVC)
    target_compile_options(verify_persistence PRIVATE /utf-8)
    target_compile_options(verify_core PRIVATE /UNDEBUG)
    target_compile_options(verify_persistence PRIVATE /UNDEBUG)
  else()
    target_compile_options(verify_core PRIVATE -UNDEBUG)
    target_compile_options(verify_persistence PRIVATE -UNDEBUG)
  endif()
  add_test(NAME ProjectPersistenceTest COMMAND verify_persistence)
  add_executable(verify_sessions tests/verify_sessions.cpp)
  target_link_libraries(verify_sessions PRIVATE studio_core)
  if(MSVC)
    target_compile_options(verify_sessions PRIVATE /utf-8 /UNDEBUG)
  else()
    target_compile_options(verify_sessions PRIVATE -UNDEBUG)
  endif()
  add_test(NAME SessionRecoveryTest COMMAND verify_sessions)
  add_executable(verify_document_edit tests/verify_document_edit.cpp)
  target_link_libraries(verify_document_edit PRIVATE studio_core)
  if(MSVC)
    target_compile_options(verify_document_edit PRIVATE /utf-8 /UNDEBUG)
  else()
    target_compile_options(verify_document_edit PRIVATE -UNDEBUG)
  endif()
  add_test(NAME DocumentEditingTest COMMAND verify_document_edit)
  add_executable(verify_status_widgets tests/verify_status_widgets.cpp)
  target_link_libraries(verify_status_widgets PRIVATE studio_core)
  target_compile_definitions(verify_status_widgets PRIVATE STUDIO_EXAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/examples")
  if(MSVC)
    target_compile_options(verify_status_widgets PRIVATE /utf-8 /UNDEBUG)
  else()
    target_compile_options(verify_status_widgets PRIVATE -UNDEBUG)
  endif()
  add_test(NAME StatusWidgetExportTest COMMAND verify_status_widgets)
  add_executable(verify_codegen tests/verify_codegen.cpp)
  add_dependencies(verify_codegen studio_android_pack studio_macos_pack studio_ios_pack)
  target_link_libraries(verify_codegen PRIVATE studio_core)
  target_compile_definitions(verify_codegen PRIVATE STUDIO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  if(MSVC)
    target_compile_options(verify_codegen PRIVATE /utf-8 /UNDEBUG)
  else()
    target_compile_options(verify_codegen PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  endif()
  set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated_cpp")
  set(STUDIO_PYTHON_TEST_EXECUTABLE "" CACHE FILEPATH "CPython with imgui-bundle 1.93.0 for optional generated-code tests")
  set(STUDIO_DOTNET_TEST_EXECUTABLE "" CACHE FILEPATH "dotnet with the .NET 8 SDK for optional generated C# tests")
  option(STUDIO_TEST_CSHARP_WINDOW "Run graphical OpenGL smoke tests for generated C# projects" OFF)
  option(STUDIO_TEST_C_EXPORT "Build and run generated C11 projects (requires OpenGL and cimgui sources)" OFF)
  set(STUDIO_C_TEST_GLFW_SOURCE "" CACHE PATH "Optional GLFW 3.4 source directory for generated C tests")
  set(STUDIO_C_TEST_CIMGUI_SOURCE "" CACHE PATH "Optional cimgui 1.91.6 source directory for generated C tests")
  set(STUDIO_C_TEST_IMGUI_SOURCE "" CACHE PATH "Optional matching Dear ImGui source directory for generated C tests")
  if(STUDIO_TEST_C_EXPORT)
    add_test(NAME GeneratedCRuntime COMMAND "${CMAKE_COMMAND}"
      "-DWORK_ROOT=${CMAKE_CURRENT_BINARY_DIR}/c-runtime"
      "-DGENERATOR=$<TARGET_FILE:verify_codegen>"
      "-DBUILD_GENERATOR=${CMAKE_GENERATOR}"
      "-DGLFW_SOURCE=${STUDIO_C_TEST_GLFW_SOURCE}"
      "-DCIMGUI_SOURCE=${STUDIO_C_TEST_CIMGUI_SOURCE}"
      "-DIMGUI_SOURCE=${STUDIO_C_TEST_IMGUI_SOURCE}"
      "-DSTB_SOURCE=${stb_SOURCE_DIR}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_c_export.cmake")
    set_tests_properties(GeneratedCRuntime PROPERTIES TIMEOUT 310)
  endif()
  set(STUDIO_JAVA_TEST_JDK "" CACHE PATH "JDK directory for optional generated Java tests")
  set(STUDIO_JAVA_TEST_BINDING_DIR "" CACHE PATH "Jar directory for imgui-java 1.90.0 and LWJGL 3.3.6")
  if(STUDIO_JAVA_TEST_JDK AND STUDIO_JAVA_TEST_BINDING_DIR)
    add_test(NAME GeneratedJavaRuntime COMMAND "${CMAKE_COMMAND}"
      "-DWORK_ROOT=${CMAKE_CURRENT_BINARY_DIR}/java-runtime"
      "-DGENERATOR=$<TARGET_FILE:verify_codegen>"
      "-DJAVA=${STUDIO_JAVA_TEST_JDK}/bin/java${CMAKE_EXECUTABLE_SUFFIX}"
      "-DJAVAC=${STUDIO_JAVA_TEST_JDK}/bin/javac${CMAKE_EXECUTABLE_SUFFIX}"
      "-DBINDINGS=${STUDIO_JAVA_TEST_BINDING_DIR}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_java_export.cmake")
    set_tests_properties(GeneratedJavaRuntime PROPERTIES TIMEOUT 200)
  endif()
  if(STUDIO_DOTNET_TEST_EXECUTABLE)
    add_test(NAME GeneratedCSharpRuntime COMMAND "${CMAKE_COMMAND}"
      "-DWORK_ROOT=${CMAKE_CURRENT_BINARY_DIR}/csharp-runtime"
      "-DGENERATOR=$<TARGET_FILE:verify_codegen>"
      "-DDOTNET=${STUDIO_DOTNET_TEST_EXECUTABLE}"
      "-DTEST_WINDOW=${STUDIO_TEST_CSHARP_WINDOW}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_csharp_export.cmake")
    set_tests_properties(GeneratedCSharpRuntime PROPERTIES TIMEOUT 420)
  endif()
  set(STUDIO_PYTHON_TEST_BINDING_DIR "" CACHE PATH "Optional isolated path containing the Python binding")
  if(STUDIO_PYTHON_TEST_EXECUTABLE)
    add_test(NAME GeneratedPythonRuntime COMMAND "${STUDIO_PYTHON_TEST_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_python_export.py" $<TARGET_FILE:verify_codegen>)
    set_tests_properties(GeneratedPythonRuntime PROPERTIES TIMEOUT 90)
    if(STUDIO_PYTHON_TEST_BINDING_DIR)
      set_tests_properties(GeneratedPythonRuntime PROPERTIES ENVIRONMENT "PYTHONPATH=${STUDIO_PYTHON_TEST_BINDING_DIR}")
    endif()
  endif()
  add_test(NAME CppExportGenerationTest COMMAND verify_codegen "${CMAKE_CURRENT_BINARY_DIR}/export_checks")
  add_test(NAME AndroidCppExportGenerationTest COMMAND "${CMAKE_COMMAND}"
    "-DGENERATOR=$<TARGET_FILE:verify_codegen>"
    "-DARCHIVE=${studio_android_pack}"
    "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/android-export-checks"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_android_export.cmake")
  add_test(NAME AppleCppExportGenerationTest COMMAND "${CMAKE_COMMAND}"
    "-DGENERATOR=$<TARGET_FILE:verify_codegen>"
    "-DPACKS=${studio_pack_directory}"
    "-DVERSION=${PROJECT_VERSION}"
    "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/apple-export-checks"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_apple_export.cmake")
  option(STUDIO_TEST_APPLE_BUILD "Build generated macOS and iOS Simulator apps with Xcode" OFF)
  if(STUDIO_TEST_APPLE_BUILD)
    if(NOT CMAKE_HOST_APPLE)
      message(FATAL_ERROR "STUDIO_TEST_APPLE_BUILD requires macOS and Xcode.")
    endif()
    add_test(NAME AppleCppExportBuildTest COMMAND "${CMAKE_COMMAND}"
      "-DGENERATOR=$<TARGET_FILE:verify_codegen>"
      "-DPACKS=${studio_pack_directory}"
      "-DVERSION=${PROJECT_VERSION}"
      "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/apple-build-checks"
      -DBUILD_APPLE=ON
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_apple_export.cmake")
    set_tests_properties(AppleCppExportBuildTest PROPERTIES TIMEOUT 720)
  endif()
  option(STUDIO_TEST_VULKAN_EXPORT "Compile and run the optional C++ Vulkan template" OFF)
  set(STUDIO_VULKAN_TEST_ROOT "" CACHE PATH "Optional Vulkan SDK root for generated C++ tests")
  if(STUDIO_TEST_VULKAN_EXPORT)
    add_test(NAME GeneratedVulkanRuntime COMMAND "${CMAKE_COMMAND}"
      "-DWORK_ROOT=${CMAKE_CURRENT_BINARY_DIR}/vulkan-runtime"
      "-DGENERATOR=$<TARGET_FILE:verify_codegen>"
      "-DBUILD_GENERATOR=${CMAKE_GENERATOR}"
      "-DVULKAN_ROOT=${STUDIO_VULKAN_TEST_ROOT}"
      "-DGLFW_SOURCE=${glfw_SOURCE_DIR}"
      "-DIMGUI_SOURCE=${imgui_SOURCE_DIR}"
      "-DSTB_SOURCE=${stb_SOURCE_DIR}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_vulkan_export.cmake")
    set_tests_properties(GeneratedVulkanRuntime PROPERTIES TIMEOUT 320)
  endif()
  set(STUDIO_RUST_TEST_CARGO "" CACHE FILEPATH "Cargo with cached dependencies for optional graphical Rust export tests")
  if(STUDIO_RUST_TEST_CARGO)
    add_test(NAME GeneratedRustRuntime COMMAND "${CMAKE_COMMAND}"
      "-DWORK_ROOT=${CMAKE_CURRENT_BINARY_DIR}/rust-runtime"
      "-DVERIFY_CODEGEN=$<TARGET_FILE:verify_codegen>"
      "-DCARGO=${STUDIO_RUST_TEST_CARGO}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_rust_export.cmake")
    set_tests_properties(GeneratedRustRuntime PROPERTIES TIMEOUT 600)
  endif()
  find_package(Threads REQUIRED)
  add_executable(verify_process tests/verify_process.cpp)
  target_link_libraries(verify_process PRIVATE studio_core Threads::Threads)
  if(WIN32)
    target_link_libraries(verify_process PRIVATE shell32)
  endif()
  if(MSVC)
    target_compile_options(verify_process PRIVATE /utf-8 /UNDEBUG /W4 /WX)
  else()
    target_compile_options(verify_process PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  endif()
  add_test(NAME NativeProcessTest COMMAND verify_process)
  set_tests_properties(NativeProcessTest PROPERTIES TIMEOUT 25)
  add_executable(verify_build_profiles tests/verify_build_profiles.cpp)
  target_link_libraries(verify_build_profiles PRIVATE studio_core Threads::Threads)
  if(MSVC)
    target_compile_options(verify_build_profiles PRIVATE /utf-8 /UNDEBUG /W4 /WX)
  else()
    target_compile_options(verify_build_profiles PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  endif()
  add_test(NAME BuildProfilesPersistenceTest COMMAND verify_build_profiles)
  set_tests_properties(BuildProfilesPersistenceTest PROPERTIES TIMEOUT 20)
  add_executable(verify_build_diagnostics tests/verify_build_diagnostics.cpp)
  target_link_libraries(verify_build_diagnostics PRIVATE studio_core)
  if(MSVC)
    target_compile_options(verify_build_diagnostics PRIVATE /utf-8 /UNDEBUG /W4 /WX)
  else()
    target_compile_options(verify_build_diagnostics PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  endif()
  add_test(NAME BuildDiagnosticsTest COMMAND verify_build_diagnostics)
  if(BUILD_STUDIO_GUI)
    add_executable(verify_assistant tests/verify_assistant.cpp)
    target_include_directories(verify_assistant PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(verify_assistant PRIVATE studio_assistant)
    if(MSVC)
      target_compile_options(verify_assistant PRIVATE /utf-8 /UNDEBUG)
    else()
      target_compile_options(verify_assistant PRIVATE -UNDEBUG)
    endif()
    add_test(NAME AssistantApiTest COMMAND verify_assistant)
    add_executable(verify_extensions tests/verify_extensions.cpp)
    target_include_directories(verify_extensions PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(verify_extensions PRIVATE studio_core studio_assistant Threads::Threads)
    if(MSVC)
      target_compile_options(verify_extensions PRIVATE /utf-8 /UNDEBUG)
    else()
      target_compile_options(verify_extensions PRIVATE -UNDEBUG)
    endif()
    add_test(NAME NativeExtensionCatalogTest COMMAND verify_extensions)
    if(STUDIO_ASSISTANT_NETWORK AND CURL_FOUND)
      set(assistant_test_python "${STUDIO_PYTHON_TEST_EXECUTABLE}")
      if(NOT assistant_test_python)
        find_package(Python3 QUIET COMPONENTS Interpreter)
        set(assistant_test_python "${Python3_EXECUTABLE}")
      endif()
      if(assistant_test_python)
        add_test(NAME AssistantLoopbackTest COMMAND "${assistant_test_python}"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_assistant_http.py" $<TARGET_FILE:verify_assistant>)
        set_tests_properties(AssistantLoopbackTest PROPERTIES TIMEOUT 35)
      endif()
    endif()
    add_executable(verify_build tests/verify_build.cpp)
    target_link_libraries(verify_build PRIVATE studio_core Threads::Threads)
    target_compile_definitions(verify_build PRIVATE
      STUDIO_CMAKE_PATH="${CMAKE_COMMAND}"
      STUDIO_GLFW_SOURCE="${glfw_SOURCE_DIR}"
      STUDIO_IMGUI_SOURCE="${imgui_SOURCE_DIR}")
    if(MSVC)
      target_compile_options(verify_build PRIVATE /utf-8 /UNDEBUG /W4 /WX)
    else()
      target_compile_options(verify_build PRIVATE -UNDEBUG -Wall -Wextra -Werror)
    endif()
    add_test(NAME NativeProjectBuildTest COMMAND verify_build "${CMAKE_CURRENT_BINARY_DIR}/build_projects")
    set_tests_properties(NativeProjectBuildTest PROPERTIES TIMEOUT 240)
    if(STUDIO_TEST_VULKAN_EXPORT)
      add_test(NAME NativeVulkanProjectBuildTest COMMAND verify_build
        "${CMAKE_CURRENT_BINARY_DIR}/build_vulkan_projects" --vulkan "${STUDIO_VULKAN_TEST_ROOT}")
      set_tests_properties(NativeVulkanProjectBuildTest PROPERTIES TIMEOUT 240)
    endif()
    if(STUDIO_TEST_C_EXPORT)
      add_test(NAME NativeCProjectBuildTest COMMAND verify_build "${CMAKE_CURRENT_BINARY_DIR}/build_c_projects"
        "${STUDIO_C_TEST_CIMGUI_SOURCE}" "${STUDIO_C_TEST_IMGUI_SOURCE}")
      set_tests_properties(NativeCProjectBuildTest PROPERTIES TIMEOUT 240)
    endif()
    if(STUDIO_DOTNET_TEST_EXECUTABLE AND STUDIO_TEST_CSHARP_WINDOW)
      set(dotnet_test_root "${CMAKE_CURRENT_BINARY_DIR}/build_csharp_projects")
      file(MAKE_DIRECTORY "${dotnet_test_root}")
      file(WRITE "${dotnet_test_root}/global.json" "{\"sdk\":{\"version\":\"8.0.100\",\"rollForward\":\"latestFeature\"}}\n")
      add_test(NAME NativeCSharpProjectBuildTest COMMAND verify_build "${dotnet_test_root}" "${STUDIO_DOTNET_TEST_EXECUTABLE}")
      set_tests_properties(NativeCSharpProjectBuildTest PROPERTIES TIMEOUT 240 ENVIRONMENT
        "DOTNET_CLI_HOME=${dotnet_test_root}/cli;NUGET_HTTP_CACHE_PATH=${dotnet_test_root}/http-cache;DOTNET_CLI_TELEMETRY_OPTOUT=1;DOTNET_NOLOGO=1")
    endif()
    if(STUDIO_PYTHON_TEST_EXECUTABLE)
      add_test(NAME NativePythonProjectBuildTest COMMAND verify_build "${CMAKE_CURRENT_BINARY_DIR}/build_python_projects"
        --python "${STUDIO_PYTHON_TEST_EXECUTABLE}" "${STUDIO_PYTHON_TEST_BINDING_DIR}")
      set_tests_properties(NativePythonProjectBuildTest PROPERTIES TIMEOUT 90)
    endif()
    if(STUDIO_JAVA_TEST_JDK AND STUDIO_JAVA_TEST_BINDING_DIR)
      add_test(NAME NativeJavaProjectBuildTest COMMAND verify_build "${CMAKE_CURRENT_BINARY_DIR}/build_java_projects"
        --java "${STUDIO_JAVA_TEST_JDK}" "${STUDIO_JAVA_TEST_BINDING_DIR}")
      set_tests_properties(NativeJavaProjectBuildTest PROPERTIES TIMEOUT 90)
    endif()
    if(STUDIO_RUST_TEST_CARGO)
      add_test(NAME NativeRustProjectBuildTest COMMAND verify_build "${CMAKE_CURRENT_BINARY_DIR}/build_rust_projects"
        --rust "${STUDIO_RUST_TEST_CARGO}")
      add_test(NAME NativeRustReleaseBuildTest COMMAND verify_build "${CMAKE_CURRENT_BINARY_DIR}/build_rust_release"
        --rust-release "${STUDIO_RUST_TEST_CARGO}")
      set_tests_properties(NativeRustProjectBuildTest NativeRustReleaseBuildTest PROPERTIES TIMEOUT 480)
    endif()
    set(generated_sources)
    foreach(case blank scene_inspector render_settings debug_console all_widgets edge empty window_chrome)
      list(APPEND generated_sources "${generated_dir}/${case}/runner.cpp" "${generated_dir}/${case}/peer.cpp")
    endforeach()
    list(APPEND generated_sources "${generated_dir}/edge/full.cpp" "${generated_dir}/window_chrome/full.cpp")
    add_custom_command(OUTPUT ${generated_sources}
      COMMAND $<TARGET_FILE:verify_codegen> "${generated_dir}"
      DEPENDS verify_codegen
        tests/fixtures/blank.json tests/fixtures/scene_inspector.json
        tests/fixtures/render_settings.json tests/fixtures/debug_console.json tests/fixtures/all_widgets.json
        examples/barra-de-titulo.json
      COMMENT "Generating C++ compile/runtime verification cases" VERBATIM)
    add_custom_target(generate_cpp_cases DEPENDS ${generated_sources})
    foreach(case blank scene_inspector render_settings debug_console all_widgets edge empty window_chrome)
      add_executable(generated_${case} "${generated_dir}/${case}/runner.cpp" "${generated_dir}/${case}/peer.cpp")
      add_dependencies(generated_${case} generate_cpp_cases)
      target_link_libraries(generated_${case} PRIVATE imgui_lib)
      if(MSVC)
        target_compile_options(generated_${case} PRIVATE /utf-8 /UNDEBUG /W4 /WX)
      else()
        target_compile_options(generated_${case} PRIVATE -UNDEBUG -Wall -Wextra -Werror)
      endif()
      add_test(NAME GeneratedCpp_${case} COMMAND generated_${case})
    endforeach()
    add_executable(generated_full "${generated_dir}/edge/full.cpp")
    add_dependencies(generated_full generate_cpp_cases)
    target_link_libraries(generated_full PRIVATE imgui_lib)
    target_include_directories(generated_full PRIVATE ${stb_SOURCE_DIR})
    if(MSVC)
      target_compile_options(generated_full PRIVATE /utf-8 /W4 /WX)
    else()
      target_compile_options(generated_full PRIVATE -Wall -Wextra -Werror)
    endif()
    add_test(NAME GeneratedCppFullSmoke COMMAND generated_full --smoke-test)
    add_executable(generated_full_window_chrome "${generated_dir}/window_chrome/full.cpp")
    add_dependencies(generated_full_window_chrome generate_cpp_cases)
    target_link_libraries(generated_full_window_chrome PRIVATE imgui_lib)
    add_test(NAME GeneratedCppWindowChromeSmoke COMMAND generated_full_window_chrome --smoke-test)
    add_executable(verify_session_ui tests/verify_session_ui.cpp src/editor/studio_ui.cpp src/editor/studio_preview.cpp src/editor/studio_canvas.cpp)
    target_include_directories(verify_session_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(verify_session_ui PRIVATE studio_core imgui_lib studio_assistant studio_images)
    if(MSVC)
      target_compile_options(verify_session_ui PRIVATE /utf-8 /UNDEBUG)
    else()
      target_compile_options(verify_session_ui PRIVATE -UNDEBUG)
    endif()
    add_test(NAME SessionRecoveryUiTest COMMAND verify_session_ui)
    add_executable(verify_preview tests/verify_preview.cpp src/editor/studio_preview.cpp)
    target_include_directories(verify_preview PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(verify_preview PRIVATE studio_core imgui_lib studio_images)
    if(MSVC)
      target_compile_options(verify_preview PRIVATE /utf-8 /UNDEBUG)
    else()
      target_compile_options(verify_preview PRIVATE -UNDEBUG)
    endif()
    add_test(NAME NativeWidgetPreviewTest COMMAND verify_preview)
    add_test(NAME NativeImagePreviewTest COMMAND verify_preview --images)
    add_executable(verify_canvas tests/verify_canvas.cpp src/editor/studio_ui.cpp src/editor/studio_preview.cpp src/editor/studio_canvas.cpp)
    target_include_directories(verify_canvas PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(verify_canvas PRIVATE STUDIO_EXAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/examples")
    target_link_libraries(verify_canvas PRIVATE studio_core imgui_lib studio_assistant studio_images)
    if(MSVC)
      target_compile_options(verify_canvas PRIVATE /utf-8 /UNDEBUG)
    else()
      target_compile_options(verify_canvas PRIVATE -UNDEBUG)
    endif()
    add_test(NAME NativeCanvasEditingTest COMMAND verify_canvas)
  endif()
endif()
