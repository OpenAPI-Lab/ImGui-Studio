# Motor de compilación, perfiles y procesos

[English](BUILD.en.md)

Build exporta una copia del diseño, la prepara con el toolchain y guarda el resultado del intento. La UI usa los estados del controlador para habilitar Run y mostrar errores.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [studio_build.hpp](../../include/imguistudio/build/studio_build.hpp) | `BuildSettings` y el controlador resuelven herramientas, preparan el intento y ejecutan fases por destino. | El artefacto solo es válido para el diseño y los ajustes de su intento. Los diagnósticos deben conservar la captura original. |
| [studio_build_profiles.hpp](../../include/imguistudio/build/studio_build_profiles.hpp) | `BuildProfileStore` valida y persiste perfiles. `attachBuildProfile` incorpora uno explícitamente al JSON. | Las rutas locales se quedan en el perfil salvo al compartirlo expresamente. Un archivo inválido o una revisión concurrente no se sustituye. |
| [studio_build_diagnostics.hpp](../../include/imguistudio/build/studio_build_diagnostics.hpp) | `DiagnosticStream` consume fragmentos y `diagnosticNode` relaciona ubicaciones con rangos de nodos. | La ruta debe corresponder al archivo generado. Gana el rango más estrecho que contiene la línea. |
| [studio_process.hpp](../../include/imguistudio/build/studio_process.hpp) | `ProcessCommand` describe ejecutable, argumentos y directorio. `NativeProcess` captura salida y administra el hijo. | Los argumentos se pasan por separado, stdin se cierra y la cancelación opera sobre el job o grupo que controla el editor. |

### Ejemplo en C++

Este ejemplo simula dos fragmentos del compilador. La entrada aparece cuando se completa la línea y se asocia al nodo con el rango más específico.

```cpp
#include "imguistudio/build/studio_build_diagnostics.hpp"
#include <cassert>

int main() {
    using namespace ImGuiStudio;
    const auto source = std::filesystem::absolute("generated");
    DiagnosticStream stream;
    std::vector<BuildDiagnostic> entries;
    stream.reset(source, source, {{"panel", {10, 100}}, {"button", {20, 35}}},
                 "Build");

    stream.feed("ui.hpp:25:7: error: invalid ", entries);
    stream.feed("expression\n", entries);
    stream.finish(entries);
    assert(entries.size() == 1);
    assert(entries.front().nodeId == "button");
}
```

## Verificación

Ejecuta `NativeProcessTest`, `BuildProfilesPersistenceTest`, `BuildDiagnosticsTest` y `NativeProjectBuildTest`. Para un destino nuevo, cubre fallo de toolchain, cancelación y cambio de diseño después de compilar. [Run / Build](../RUN_BUILD.md) · [Diagnósticos](../BUILD_DIAGNOSTICS.md)

