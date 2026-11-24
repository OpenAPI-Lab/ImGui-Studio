cmake_minimum_required(VERSION 3.22)
project(StudioExportAndroid LANGUAGES C CXX)
if(NOT ANDROID)
  message(FATAL_ERROR "Use Android Studio, Gradle, or the Android NDK CMake toolchain to build this project.")
endif()

include(FetchContent)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
FetchContent_Declare(imgui
  URL https://codeload.github.com/ocornut/imgui/tar.gz/refs/tags/v1.91.5
  URL_HASH SHA256=2aa2d169c569368439e5d5667e0796d09ca5cc6432965ce082e516937d7db254)
FetchContent_MakeAvailable(imgui)

set(native_glue "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
add_library(studio_export SHARED main.cpp
  "${native_glue}/android_native_app_glue.c"
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_android.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
target_compile_features(studio_export PRIVATE cxx_std_17)
target_compile_definitions(studio_export PRIVATE IMGUI_IMPL_OPENGL_ES3)
target_include_directories(studio_export PRIVATE "${native_glue}"
  "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
target_link_libraries(studio_export PRIVATE android log EGL GLESv3)
# CMake 3.22 on Windows can invoke clang++ by its DOS short name.
target_link_options(studio_export PRIVATE "--driver-mode=g++"
  "-Wl,-u,ANativeActivity_onCreate" "-Wl,-z,max-page-size=16384")
