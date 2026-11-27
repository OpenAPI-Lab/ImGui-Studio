# Compilar, probar y empaquetar

[English](BUILD_AND_TEST.en.md)


## Editor de escritorio

Usa CMake 3.16+, Ninja y un compilador C++17 con soporte para compilar las dependencias C. Ejecuta desde la raíz:

```sh
cmake -S . -B build/editor -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_CORE_TEST=OFF
cmake --build build/editor --target imgui_studio
```

| Host | Herramientas y salida |
| --- | --- |
| Windows | MSYS2 UCRT64 con GCC o terminal de desarrollo de Visual Studio con MSVC. Salida `build/editor/bin/imgui_studio.exe`. |
| Ubuntu/Debian | `build-essential`, `cmake`, `ninja-build`, `pkg-config`, `libgl-dev`, `xorg-dev`, `libwayland-dev`, `wayland-protocols`, `libxkbcommon-dev`. Salida `build/editor/bin/imgui_studio`. |
| macOS | Clang y SDK de las herramientas de línea de comandos de Xcode, CMake y Ninja. Salida configurada `build/editor/bin/ImGui Studio.app`. Prueba en Mac pendiente. |

CMake descarga GLFW, Dear ImGui y stb mediante archivos con hash fijado. Para reutilizar fuentes locales, indica `FETCHCONTENT_SOURCE_DIR_GLFW`, `FETCHCONTENT_SOURCE_DIR_IMGUI` o `FETCHCONTENT_SOURCE_DIR_STB`. Las rutas locales son entradas del usuario. En Linux instala las bibliotecas gráficas de los backends que habilites. WSL requiere WSLg para la ventana.

## Opciones

| Opción | Uso |
| --- | --- |
| `BUILD_STUDIO_GUI` | Activa el ejecutable y sus dependencias gráficas. |
| `BUILD_CORE_TEST` | Activa las pruebas del núcleo y las pruebas que admite la configuración actual. |
| `STUDIO_ASSISTANT_NETWORK`, `STUDIO_CATALOG_NETWORK` | Habilitan sus conexiones cuando se encuentra libcurl 7.85+. |
| `STUDIO_ASSISTANT_SECRET_SERVICE` | Habilita el llavero de Linux si encuentra libsecret y el entorno ofrece un servicio de secretos. |
| `STUDIO_PACKAGING` | Activa reglas de instalación, CPack y verificadores. Usa Python 3. |

## Elegir pruebas

```sh
cmake -S . -B build/core -G Ninja -DBUILD_STUDIO_GUI=OFF
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

El núcleo funciona sin pantalla. Con GUI, usa `ctest --test-dir build/editor -N` para enumerar las pruebas que se habilitaron en ese build. Configura otro directorio con `BUILD_CORE_TEST=ON` si partiste del comando de Release. `--smoke-test` dibuja tres fotogramas en una ventana oculta y `--font-smoke-test` comprueba glifos de interfaz.

| Área | Pruebas de referencia |
| --- | --- |
| Guardado, historial y árbol | `ProjectPersistenceTest`, `SessionRecoveryTest`, `DocumentEditingTest`. |
| Lienzo, imágenes y recuperación visual | `NativeCanvasEditingTest`, `NativeImagePreviewTest`, `SessionRecoveryUiTest`. |
| Exportación | `CppExportGenerationTest`, `AndroidCppExportGenerationTest`, `AppleCppExportGenerationTest`. Los runtimes opcionales requieren sus propios toolchains. |
| Servicios | `NativeExtensionCatalogTest`, `AssistantApiTest` y `AssistantLoopbackTest` cuando está habilitada. |
| Paquetes | `WindowsZipPackageTest` o `LinuxDebPackageTest` según host y herramientas. |

## Distribución

Configura otro build con `-DSTUDIO_PACKAGING=ON`, compila `imgui_studio` y ejecuta `cpack --config build/package/CPackConfig.cmake -C Release` si configuraste en `build/package`. La [referencia de empaquetado](REFERENCE.md#paquetes-de-escritorio) detalla los generadores, las dependencias y las comprobaciones de los artefactos. Para instalar una versión, consulta [Instalar ImGui Studio](INSTALLATION.md).
