execute_process(
  COMMAND "${VERIFY_CODEGEN}" "${WORK_ROOT}"
  RESULT_VARIABLE generated
)
if(NOT generated STREQUAL "0")
  message(FATAL_ERROR "Could not generate Rust fixtures: ${generated}")
endif()

if(NOT DEFINED ENV{CARGO_TARGET_DIR})
  set(ENV{CARGO_TARGET_DIR} "${WORK_ROOT}/target")
endif()
if(NOT DEFINED ENV{RUSTC})
  get_filename_component(cargo_directory "${CARGO}" DIRECTORY)
  get_filename_component(cargo_extension "${CARGO}" EXT)
  if(EXISTS "${cargo_directory}/rustc${cargo_extension}")
    set(ENV{RUSTC} "${cargo_directory}/rustc${cargo_extension}")
  endif()
endif()
foreach(case blank scene_inspector render_settings debug_console all_widgets edge empty window_chrome)
  execute_process(
    COMMAND "${CARGO}" run --offline --manifest-path "${WORK_ROOT}/${case}/rust/Cargo.toml" -- --smoke-test
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE errors
    TIMEOUT 240
  )
  if(NOT status STREQUAL "0" OR NOT output MATCHES "Rust imgui-rs rendered three frames")
    message(FATAL_ERROR "Rust fixture ${case} failed (${status}):\n${output}\n${errors}")
  endif()
  message(STATUS "Rust fixture ${case}: ${output}")
endforeach()

execute_process(
  COMMAND "${CARGO}" test --offline --manifest-path "${WORK_ROOT}/edge/rust/Cargo.toml"
  RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 60
)
if(NOT status STREQUAL "0" OR NOT output MATCHES "model_and_flow_layout .*ok")
  message(FATAL_ERROR "Rust layout and interaction verification failed (${status}):\n${output}\n${errors}")
endif()
message(STATUS "Rust model selection, flow layout, animation and images verified in memory")

execute_process(
  COMMAND "${CARGO}" test --offline --manifest-path "${WORK_ROOT}/window_chrome/rust/Cargo.toml"
  RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 60
)
if(NOT status STREQUAL "0" OR NOT output MATCHES "window_chrome_interaction .*ok")
  message(FATAL_ERROR "Rust window chrome interactions failed (${status}):\n${output}\n${errors}")
endif()
message(STATUS "Rust window chrome interactions verified")
