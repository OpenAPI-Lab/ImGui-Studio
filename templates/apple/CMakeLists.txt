cmake_minimum_required(VERSION 3.24)
if(NOT CMAKE_HOST_APPLE)
  message(FATAL_ERROR "This Apple export requires a Mac with Xcode and the target SDK.")
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "@STUDIO_APPLE_DEPLOYMENT@" CACHE STRING "Minimum OS version")
project(StudioExport LANGUAGES CXX OBJCXX)
set(studio_target "@STUDIO_APPLE_SYSTEM@")
if(NOT CMAKE_SYSTEM_NAME STREQUAL studio_target)
  message(FATAL_ERROR "This template targets ${studio_target}. See README.md for the configure command.")
endif()

include(FetchContent)
FetchContent_Declare(imgui
  URL https://codeload.github.com/ocornut/imgui/tar.gz/refs/tags/v1.91.5
  URL_HASH SHA256=2aa2d169c569368439e5d5667e0796d09ca5cc6432965ce082e516937d7db254)
FetchContent_MakeAvailable(imgui)

set(notices LICENSE.dear-imgui.txt)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.imgui-knobs.txt")
  list(APPEND notices LICENSE.imgui-knobs.txt)
endif()
set_source_files_properties(${notices} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
add_executable(studio_export MACOSX_BUNDLE main.mm ${notices}
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm")
set(STUDIO_BUNDLE_ID "org.imguistudio.exported" CACHE STRING "Bundle identifier")
set_target_properties(studio_export PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES
  OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED YES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
  MACOSX_BUNDLE_GUI_IDENTIFIER "${STUDIO_BUNDLE_ID}"
  MACOSX_BUNDLE_BUNDLE_NAME "Studio Export"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
  MACOSX_BUNDLE_BUNDLE_VERSION "1"
  XCODE_GENERATE_SCHEME YES
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${STUDIO_BUNDLE_ID}")
target_include_directories(studio_export PRIVATE "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
  target_compile_options(studio_export PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
endif()
target_link_libraries(studio_export PRIVATE "-framework Metal" "-framework MetalKit" "-framework QuartzCore")
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set_target_properties(studio_export PROPERTIES XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
  target_link_libraries(studio_export PRIVATE "-framework UIKit")
else()
  target_sources(studio_export PRIVATE "${imgui_SOURCE_DIR}/backends/imgui_impl_osx.mm")
  target_link_libraries(studio_export PRIVATE "-framework Cocoa" "-framework GameController")
endif()
