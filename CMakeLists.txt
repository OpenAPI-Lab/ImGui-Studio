cmake_minimum_required(VERSION 3.16)
project(ImGuiStudio VERSION 1.0.0 LANGUAGES CXX)

add_subdirectory(native)

if(TARGET imgui_studio)
  set_target_properties(imgui_studio PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

if(BUILD_CORE_TEST OR (BUILD_STUDIO_GUI AND STUDIO_PACKAGING))
  enable_testing()
endif()
