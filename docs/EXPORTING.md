# Exportar e integrar una interfaz

[English](EXPORTING.en.md)


En **Código generado** selecciona lenguaje y una carpeta nueva. El editor genera el código del diseño, un host de ejemplo y el informe `codegen_report.txt`. Los rangos del informe relacionan nodos con código. Revisa los avisos antes de integrar un control. La [matriz de capacidades](CAPABILITIES.md) distingue la generación de la ejecución verificada.

## C++ en una aplicación existente

El proyecto genera `ui.hpp`, una estructura de estado derivada del nombre del proyecto, `ApplyStudioStyle()` y `RenderStudioUI(state)`. El host crea el contexto y los backends de Dear ImGui. Con un proyecto llamado `Monitor`, el patrón es:

```cpp
#include "ui.hpp"

struct MonitorView {
    MonitorState state{};

    void initializeStyle() { ApplyStudioStyle(); }
    void drawFrame() { RenderStudioUI(state); }
};
```

Llama a `initializeStyle()` después de crear el contexto. Ejecuta `drawFrame()`
entre `ImGui::NewFrame()` e `ImGui::Render()`. El `main.cpp` exportado
contiene el orden completo. Conserva los recursos y archivos auxiliares.
Si reemplazas el backend, adapta la carga y liberación de texturas a su ciclo
de vida. Conecta las acciones leyendo el estado generado después de dibujar.
El [ejemplo del botón Ayuda](../examples/title-bar-action/README.md) usa ese
patrón para añadir una acción al host C++ exportado.

El destino C++ de escritorio ofrece GLFW/OpenGL y Vulkan en Windows y Linux. Vulkan requiere encabezados, cargador y un controlador compatible con presentación. Puedes configurar `Vulkan_ROOT` para un SDK personalizado.
La [guía de Vulkan](VULKAN.md) muestra cómo probar imágenes, redimensionado y entrada.

## Otros destinos de escritorio

| Lenguaje | Punto de entrada y herramienta |
| --- | --- |
| C11 | CMake, cimgui y ejecutable `studio_export_c`. |
| Python | `main.py` y `requirements.txt`. Usa CPython e imgui-bundle de la versión indicada por el resultado. |
| C# | `StudioExport.csproj`, SDK .NET 8 e ImGui.NET. |
| Java | `StudioUI.java` y proyecto Java con dependencias indicadas por el exportador. |
| Rust | `Cargo.toml`, Cargo e imgui-rs. Los puentes nativos requieren compilador C/C++ cuando el resultado los incluye. |

## Destinos opcionales

Los targets `studio_android_pack`, `studio_macos_pack` y `studio_ios_pack` generan ZIP por separado. Descomprime el elegido e indica al exportador la carpeta con `template.version`. Se genera un proyecto que se compila con las herramientas del destino.

- [Android](ANDROID.md) usa C++, Gradle, el NDK y OpenGL ES 3.
- [macOS](../templates/apple/macos/README.md) usa C++, AppKit, Metal y CMake/Xcode.
- [iOS](../templates/apple/ios/README.md) usa C++, UIKit, Metal y CMake/Xcode.

La [guía de macOS e iOS](APPLE_EXPORTS.md) explica el uso de Xcode, el
simulador y la comprobación de ambos proyectos generados.
