# Conectar el botón Ayuda a una acción

[English](README.en.md)

Este ejemplo usa [Barra de título](../barra-de-titulo.json) y el exportador **C++ / OpenGL**. El botón **Ayuda** vive dentro del título del panel. El exportador registra el clic en `state.chrome_help` durante el fotograma. El host usa ese pulso para mantener abierta una ventana de ayuda entre fotogramas.

## Integrar y ejecutar

1. Abre `barra-de-titulo.json` en ImGui Studio. En **Código generado**, exporta el proyecto C++ con OpenGL a una carpeta nueva.
2. Copia [title_bar_help.hpp](title_bar_help.hpp) junto a `ui.hpp` y `main.cpp`.
3. En `main.cpp`, agrega `#include "title_bar_help.hpp"` después de `#include "ui.hpp"`. Agrega `bool helpOpen = false;` junto a la variable `state`, antes del bucle de render.
4. Llama a `RenderTitleBarHelp(state, helpOpen);` inmediatamente después de `RenderStudioUI(state);` y antes de `ImGui::Render()`. Si el archivo tiene ambas llamadas en una línea, sepáralas.
5. Desde la carpeta exportada, configura y compila el proyecto.

```sh
cmake -S . -B build -G Ninja
cmake --build build --target studio_export
ctest --test-dir build --output-on-failure
```

Ejecuta `build/studio_export.exe` en Windows o `./build/studio_export` en Linux. Pulsa **Ayuda**, cierra esa ventana y pulsa el botón de nuevo. El diseño genera el evento y el host decide qué hacer al recibirlo. El encabezado usa el tipo generado para este proyecto. Si cambias el nombre del proyecto o del control, revisa `ui.hpp` y actualiza el ejemplo según corresponda.

La prueba rápida del proyecto exportado cubre el arranque gráfico. Las pruebas del repositorio `GeneratedCpp_window_chrome` y `GeneratedCppWindowChromeSmoke` comprueban por separado el control de título y la ejecución de su host.
