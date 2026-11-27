string(RANDOM LENGTH 12 suffix)
set(directory "${OUTPUT}/run-${suffix}")
file(MAKE_DIRECTORY "${directory}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf "${ARCHIVE}"
  WORKING_DIRECTORY "${directory}" RESULT_VARIABLE result TIMEOUT 30)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "Cannot extract Android pack: ${result}")
endif()

set(project "${directory}/project")
execute_process(COMMAND "${GENERATOR}" --android "${directory}/Android-Cpp" "${project}"
  RESULT_VARIABLE result TIMEOUT 30)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "Android export generation failed: ${result}")
endif()
foreach(file IN ITEMS AndroidManifest.xml build.gradle settings.gradle CMakeLists.txt main.cpp ui.hpp
    codegen_report.txt LICENSE.dear-imgui.txt LICENSE.imgui-knobs.txt)
  if(NOT EXISTS "${project}/${file}")
    message(FATAL_ERROR "Android export omitted ${file}")
  endif()
endforeach()
message(STATUS "Generated Android project: ${project}")
