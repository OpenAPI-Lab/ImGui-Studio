include(GNUInstallDirs)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(studio_packaging_dir "${CMAKE_CURRENT_LIST_DIR}")
set(studio_icons "${CMAKE_CURRENT_BINARY_DIR}/studio-icons")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${studio_packaging_dir}/make_icons.py")
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "${studio_packaging_dir}/make_icons.py" "${studio_icons}"
  RESULT_VARIABLE icons_result
)
if(NOT icons_result STREQUAL "0")
  message(FATAL_ERROR "Could not prepare ImGui Studio icons: ${icons_result}")
endif()

set(CPACK_PACKAGE_NAME "ImGui Studio")
set(CPACK_PACKAGE_VENDOR "ImGui Studio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Native visual designer for Dear ImGui")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ImGui Studio")
set(CPACK_PACKAGE_FILE_NAME "ImGuiStudio-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_VERBATIM_VARIABLES YES)
get_filename_component(CPACK_RESOURCE_FILE_LICENSE "${studio_packaging_dir}/../../LICENSE" ABSOLUTE)

if(WIN32)
  if(CMAKE_VERSION VERSION_LESS 3.21)
    message(FATAL_ERROR "Windows packaging requires CMake 3.21 or later to stage runtime DLLs.")
  endif()
  enable_language(RC)
  string(CONFIGURE [=[
1 ICON "@studio_icons@/imgui-studio.ico"
1 VERSIONINFO
FILEVERSION @PROJECT_VERSION_MAJOR@,@PROJECT_VERSION_MINOR@,@PROJECT_VERSION_PATCH@,0
PRODUCTVERSION @PROJECT_VERSION_MAJOR@,@PROJECT_VERSION_MINOR@,@PROJECT_VERSION_PATCH@,0
FILEOS 0x40004
FILETYPE 0x1
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "040904b0"
    BEGIN
      VALUE "FileDescription", "ImGui Studio"
      VALUE "FileVersion", "@PROJECT_VERSION@"
      VALUE "ProductName", "ImGui Studio"
      VALUE "ProductVersion", "@PROJECT_VERSION@"
    END
  END
  BLOCK "VarFileInfo"
  BEGIN
    VALUE "Translation", 0x0409, 1200
  END
END
]=] studio_resource @ONLY)
  file(GENERATE OUTPUT "${studio_icons}/imgui-studio.rc" CONTENT "${studio_resource}")
  target_sources(imgui_studio PRIVATE "${studio_icons}/imgui-studio.rc")
  install(TARGETS imgui_studio RUNTIME_DEPENDENCY_SET studio_runtime
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  get_filename_component(studio_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
  set(studio_runtime_dirs "${studio_compiler_bin}")
  if(STUDIO_ASSISTANT_NETWORK AND CURL_FOUND)
    list(APPEND studio_runtime_dirs "$<TARGET_FILE_DIR:CURL::libcurl>")
  endif()
  install(CODE [[
    if(POLICY CMP0207)
      cmake_policy(SET CMP0207 NEW)
    endif()
  ]])
  install(RUNTIME_DEPENDENCY_SET studio_runtime
    DIRECTORIES ${studio_runtime_dirs}
    PRE_EXCLUDE_REGEXES "^([Aa][Pp][Ii]|[Ee][Xx][Tt])-[Mm][Ss]-"
    POST_EXCLUDE_REGEXES "[/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\]" "[/\\\\][Ss][Yy][Ss][Ww][Oo][Ww]64[/\\\\]"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
    include(InstallRequiredSystemLibraries)
  endif()
  if(MINGW)
    set(studio_mingw_notices "${studio_compiler_bin}/../share/licenses")
    set(studio_notice_packages gcc-libs libwinpthread winpthreads)
    if(STUDIO_ASSISTANT_NETWORK AND CURL_FOUND)
      list(APPEND studio_notice_packages brotli curl gettext-runtime libiconv libidn2
        libpsl libssh2 libunistring nghttp2 nghttp3 ngtcp2 openssl zlib zstd)
    endif()
    foreach(pkg IN LISTS studio_notice_packages)
      if(IS_DIRECTORY "${studio_mingw_notices}/${pkg}")
        install(DIRECTORY "${studio_mingw_notices}/${pkg}" DESTINATION "${CMAKE_INSTALL_DOCDIR}/third-party")
      endif()
    endforeach()
    if(STUDIO_ASSISTANT_NETWORK AND CURL_FOUND AND EXISTS "${studio_compiler_bin}/libidn2-0.dll"
       AND NOT IS_DIRECTORY "${studio_mingw_notices}/libidn2")
      install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libidn2-notices/"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/third-party/libidn2")
    endif()
  endif()
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_PACKAGE_EXECUTABLES "imgui_studio" "ImGui Studio")
  set(CPACK_CREATE_DESKTOP_LINKS "imgui_studio")
  set(CPACK_NSIS_EXECUTABLES_DIRECTORY "${CMAKE_INSTALL_BINDIR}")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_INSTALL_BINDIR}\\imgui_studio.exe")
  set(CPACK_NSIS_MUI_ICON "${studio_icons}/imgui-studio.ico")
  set(CPACK_NSIS_MUI_UNIICON "${studio_icons}/imgui-studio.ico")
  string(CONCAT CPACK_NSIS_DEFINES
    "  VIProductVersion \"${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0\"\n"
    "  VIAddVersionKey /LANG=1033 \"ProductName\" \"ImGui Studio\"\n"
    "  VIAddVersionKey /LANG=1033 \"FileDescription\" \"ImGui Studio Installer\"\n"
    "  VIAddVersionKey /LANG=1033 \"ProductVersion\" \"${PROJECT_VERSION}\"\n"
    "  VIAddVersionKey /LANG=1033 \"FileVersion\" \"${PROJECT_VERSION}\"\n")
elseif(APPLE)
  set(studio_icon "${studio_icons}/imgui-studio.icns")
  set_source_files_properties("${studio_icon}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_sources(imgui_studio PRIVATE "${studio_icon}")
  set_target_properties(imgui_studio PROPERTIES MACOSX_BUNDLE_ICON_FILE "imgui-studio.icns")
  install(TARGETS imgui_studio BUNDLE DESTINATION ".")
  set(CPACK_GENERATOR "DragNDrop")
  set(studio_docdir "ImGui Studio.app/Contents/Resources")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  install(TARGETS imgui_studio RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  configure_file("${studio_packaging_dir}/imgui-studio.desktop"
    "${CMAKE_CURRENT_BINARY_DIR}/imgui-studio.desktop" COPYONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/imgui-studio.desktop" DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
  install(FILES "${studio_icons}/imgui-studio.png" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps")
  set(STUDIO_PACKAGE_MAINTAINER "ImGui Studio (local build)" CACHE STRING "DEB maintainer name and contact")
  set(CPACK_GENERATOR "DEB")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
  set(CPACK_DEBIAN_PACKAGE_NAME "imgui-studio")
  set(studio_docdir "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CPACK_DEBIAN_PACKAGE_NAME}")
  set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${STUDIO_PACKAGE_MAINTAINER}")
  set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  if(TARGET PkgConfig::STUDIO_LIBSECRET)
    # dpkg-shlibdeps cannot infer the package name when libsecret comes from an extracted SDK.
    list(APPEND CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS ${STUDIO_LIBSECRET_LIBRARY_DIRS})
    if(CPACK_DEBIAN_PACKAGE_DEPENDS)
      string(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS ", ")
    endif()
    string(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS "libsecret-1-0 (>= 0.19.1)")
  endif()
else()
  install(TARGETS imgui_studio RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  set(CPACK_GENERATOR "TGZ")
endif()

if(NOT studio_docdir)
  set(studio_docdir "${CMAKE_INSTALL_DOCDIR}")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  install(FILES "${CPACK_RESOURCE_FILE_LICENSE}" DESTINATION "${studio_docdir}" RENAME copyright)
else()
  install(FILES "${CPACK_RESOURCE_FILE_LICENSE}" DESTINATION "${studio_docdir}" RENAME LICENSE.txt)
endif()
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION "${studio_docdir}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/COBERTURA_IDIOMAS.md" DESTINATION "${studio_docdir}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/studio_locale_strings.inc" DESTINATION "${studio_docdir}/include")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/examples/barra-y-lienzo-libre.json" DESTINATION "${studio_docdir}/examples")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/examples/tarjeta-de-proceso.widget.json" DESTINATION "${studio_docdir}/examples")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/examples/estado-y-progreso.json" DESTINATION "${studio_docdir}/examples")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/examples/barra-de-titulo.json" DESTINATION "${studio_docdir}/examples")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/examples/vulkan-imagenes.json" DESTINATION "${studio_docdir}/examples")
install(FILES "${imgui_SOURCE_DIR}/LICENSE.txt" DESTINATION "${studio_docdir}/third-party" RENAME Dear-ImGui.txt)
install(FILES "${studio_knobs}/LICENSE" DESTINATION "${studio_docdir}/third-party" RENAME imgui-knobs.txt)
install(FILES "${glfw_SOURCE_DIR}/LICENSE.md" DESTINATION "${studio_docdir}/third-party" RENAME GLFW.md)
install(FILES "${stb_SOURCE_DIR}/LICENSE" DESTINATION "${studio_docdir}/third-party" RENAME stb.txt)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/LICENSE.MIT"
  DESTINATION "${studio_docdir}/third-party" RENAME nlohmann-json.txt)

include(CPack)

if(NOT CMAKE_CROSSCOMPILING)
  set(studio_package_test "${Python3_EXECUTABLE}" "${studio_packaging_dir}/verify_package.py"
    --build "${CMAKE_BINARY_DIR}" --cpack "${CMAKE_CPACK_COMMAND}" "--config=$<CONFIG>")
  if(WIN32)
    add_test(NAME WindowsZipPackageTest COMMAND ${studio_package_test} --format ZIP)
    set_tests_properties(WindowsZipPackageTest PROPERTIES TIMEOUT 150)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_program(STUDIO_DPKG dpkg)
    find_program(STUDIO_DPKG_DEB dpkg-deb)
    find_program(STUDIO_FAKEROOT fakeroot)
    if(STUDIO_DPKG AND STUDIO_DPKG_DEB AND STUDIO_FAKEROOT)
      if(TARGET PkgConfig::STUDIO_LIBSECRET)
        list(APPEND studio_package_test --require-libsecret)
      endif()
      add_test(NAME LinuxDebPackageTest COMMAND ${studio_package_test} --format DEB
        --dpkg "${STUDIO_DPKG}" --dpkg-deb "${STUDIO_DPKG_DEB}" --fakeroot "${STUDIO_FAKEROOT}")
      set_tests_properties(LinuxDebPackageTest PROPERTIES TIMEOUT 180)
    else()
      message(STATUS "DEB package lifecycle test requires dpkg, dpkg-deb and fakeroot")
    endif()
  endif()
endif()
