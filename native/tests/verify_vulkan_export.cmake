set(source "${WORK_ROOT}/source")
execute_process(COMMAND "${GENERATOR}" --vulkan "${source}" TIMEOUT 30 RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "Vulkan export failed: ${result}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" -S "${source}" -B "${WORK_ROOT}/missing-sdk"
  -G "${BUILD_GENERATOR}" -DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=TRUE
  TIMEOUT 45 RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(result STREQUAL "0" OR NOT errors MATCHES "Vulkan headers and loader library were not found")
  message(FATAL_ERROR "Expected a Vulkan SDK diagnostic:\n${output}\n${errors}")
endif()

set(overrides)
if(VULKAN_ROOT)
  list(APPEND overrides "-DVulkan_ROOT=${VULKAN_ROOT}")
endif()
foreach(dep GLFW IMGUI STB)
  if(NOT "${${dep}_SOURCE}" STREQUAL "")
    list(APPEND overrides "-DFETCHCONTENT_SOURCE_DIR_${dep}=${${dep}_SOURCE}")
  endif()
endforeach()
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${source}" -B "${WORK_ROOT}/build" -G "${BUILD_GENERATOR}"
  ${overrides} TIMEOUT 100 RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "Vulkan configure failed: ${result}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${WORK_ROOT}/build" --config Debug --parallel 2
  TIMEOUT 120 RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "Vulkan build failed: ${result}")
endif()
execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${WORK_ROOT}/build" -C Debug -V
  TIMEOUT 45 RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result STREQUAL "0" OR NOT output MATCHES "Vulkan presented 5 frames" OR
    NOT output MATCHES "Vulkan resize, checkbox input and PNG/JPEG draw commands verified" OR
    "${output}\n${errors}" MATCHES "Validation Error|VUID-")
  message(FATAL_ERROR "Vulkan runtime or validation failed:\n${output}\n${errors}")
endif()
message(STATUS "${output}")
