# Validación reproducible

[English](VALIDATION.en.md)


Incluye en el informe el commit probado, sistema, arquitectura, compilador, backend gráfico o destino, comandos y resultado. Usa una copia de un [proyecto de ejemplo](WORKFLOW.md#qué-ejemplo-abrir) y una carpeta de salida aparte para facilitar la reproducción.

## Cambios de modelo y exportación

```sh
cmake -S . -B build/check-core -G Ninja -DBUILD_STUDIO_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build/check-core --target verify_core verify_persistence verify_sessions verify_document_edit verify_codegen
ctest --test-dir build/check-core -R 'CoreAndCodegenTest|ProjectPersistenceTest|SessionRecoveryTest|DocumentEditingTest|CppExportGenerationTest' --output-on-failure
```

Esta selección incluye generación C++, Android y Apple. Para un cambio de formato, abre un proyecto guardado previamente, guarda una copia y comprueba los campos afectados. Los tests usan fixtures preexistentes y carpetas de salida del build.

## Cambios de servicios y herramientas

```sh
cmake -S . -B build/check-editor -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/check-editor --target verify_assistant verify_extensions verify_build_profiles verify_build_diagnostics
ctest --test-dir build/check-editor -R 'AssistantApiTest|AssistantLoopbackTest|NativeExtensionCatalogTest|BuildProfilesPersistenceTest|BuildDiagnosticsTest' --output-on-failure
```

Usa `ctest --test-dir build/check-editor -N` para ver si se habilitó Loopback. Requiere libcurl y Python. La prueba simula al proveedor en la máquina local.

## Cambios gráficos

Compila `imgui_studio` y ejecuta `--smoke-test`. Para un cambio visual, abre además el proyecto de ejemplo y reproduce el gesto o el estado modificado. En Ubuntu con WSLg se han usado estos comandos para probar ambos caminos:

```sh
env -u DISPLAY XDG_SESSION_TYPE=wayland ./build/check-editor/bin/imgui_studio --smoke-test
env -u WAYLAND_DISPLAY XDG_SESSION_TYPE=x11 ./build/check-editor/bin/imgui_studio --smoke-test
```

La segunda ejecución usa XWayland en WSLg. Para diagnósticos Android indica si el APK solo se compiló o también arrancó. Si se ejecutó, registra API, arquitectura y dispositivo o emulador. Reporta arranque, entrada, imágenes, giro y reanudación cuando correspondan. Marca por separado la generación y la ejecución en macOS e iOS.

En Windows con Visual Studio 2026 y un CMake que reconozca ese generador, esta es la comprobación gráfica local:

```powershell
cmake -S . -B build/check-msvc -G "Visual Studio 18 2026" -A x64 -DBUILD_CORE_TEST=OFF -DSTUDIO_ASSISTANT_NETWORK=OFF -DSTUDIO_CATALOG_NETWORK=OFF
cmake --build build/check-msvc --config Release --target imgui_studio --parallel 1
$editor = Start-Process .\build\check-msvc\bin\Release\imgui_studio.exe -ArgumentList '--smoke-test' -WindowStyle Hidden -Wait -PassThru
$editor.ExitCode
```

El código de salida habitual de una prueba aprobada es `0`. El ejecutable con MSVC queda en el subdirectorio de la configuración seleccionada. Las conexiones opcionales se desactivan para esta comprobación gráfica.

Consulta la [matriz de alcance](CAPABILITIES.md) para distinguir lo que se verificó localmente y lo que queda pendiente.

### Selector de proyectos y apariencia

Ejecuta `SessionRecoveryUiTest` en una compilación con GUI y pruebas habilitadas. Cubre selección en el navegador integrado, subcarpetas, nombres con acentos, carga inválida, cancelación desde un aviso de cambios y persistencia de paleta. Para probar el diálogo nativo, abre una instancia normal de Windows y repite **Abrir proyecto**, **Guardar como**, el aviso de reemplazo y **Cancelar**. Anota qué selector apareció.

Para cambios visuales, elige una paleta oscura y **Porcelain** en [Apariencia de Studio](WORKFLOW.md#personalizar-la-apariencia-de-studio). Cambia el radio y reabre el editor para comprobar el ajuste. Usa el mismo proyecto antes y después para comprobar que sus colores y radios se conservan en la vista previa.

## Ejecutar una exportación C++ completa

En un build con GUI y pruebas activadas, ejecuta:

```sh
cmake --build build/check-editor --target verify_build generated_window_chrome generated_full_window_chrome
ctest --test-dir build/check-editor -R '^(NativeProjectBuildTest|GeneratedCpp_window_chrome|GeneratedCppWindowChromeSmoke)$' --output-on-failure
```

La primera prueba usa el controlador de Run / Build con CMake y Ninja. Comprueba
compilación, arranque, cancelación, cambios del diseño y fallos del toolchain.
Las otras ejecutan el título generado y su host. Para conectar una acción
propia al título, usa [el ejemplo del botón Ayuda](../examples/title-bar-action/README.md).

Para un cambio de renderizador Vulkan, ejecuta las dos comprobaciones de la
[guía de Vulkan](VULKAN.md#pruebas-del-repositorio).
