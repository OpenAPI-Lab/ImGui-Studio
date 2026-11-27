# Diagnósticos de compilación y navegación

[English](BUILD_DIAGNOSTICS.en.md)

## Encontrar la causa de un fallo

Abre **Proyecto nativo → Ejecutar / Compilar…**, elige el perfil y ejecuta **Build**. Expande **Diagnósticos**. Identifica la primera salida de error en su fase y compara con la salida reciente del log.

| Fase u origen | Qué revisar |
| --- | --- |
| Configurar | Generador, compilador, SDK y bibliotecas que necesita el destino. Un error de CMake puede apuntar al archivo de configuración. |
| Compilar | Abre **Ver código exportado** para ver la captura del intento. Si existe **Seleccionar widget**, revisa la propiedad que produjo la línea. |
| Generador | Lee el diagnóstico y `codegen_report.txt` para ver controles y propiedades parciales. |
| Ejecutar | Revisa el código de salida, las dependencias de ejecución y el log. La salida de Run se conserva como texto. |

## Contrato de navegación

`DiagnosticStream` procesa fragmentos de stdout y stderr. Interpreta ubicaciones comunes de GCC/Clang, MSVC/Roslyn y CMake. También admite líneas partidas, finales CRLF y el último mensaje sin salto. Descarta líneas individuales de más de 16 KiB del panel estructurado. Conserva hasta 1000 entradas y permite que un error nuevo desplace un aviso o nota.

La asociación combina la ruta normalizada y el rango del generador. Por ejemplo, con un nodo padre en las líneas 10–100 de `ui.hpp` y un hijo en 20–35, un error de la línea 25 apunta al hijo. La misma línea en `external.hpp` queda con su ubicación propia. Tras editar el proyecto, vuelve a compilar para que **Seleccionar widget** use una captura que corresponda al diseño.

## Verificación y reporte

Ejecuta `BuildDiagnosticsTest` para comprobar entradas fragmentadas, códigos de compilador, rutas y rangos anidados. `NativeProjectBuildTest` comprueba el ciclo con un proyecto C++ exportado y un error controlado del fixture. Incluye en tu reporte el destino, generador, compilador, fase, primera línea relevante, rango del reporte y proyecto mínimo. Sustituye rutas o datos privados antes de adjuntar el log.

[Perfiles y ejecución](RUN_BUILD.md) · [Código del parser](../include/imguistudio/build/studio_build_diagnostics.hpp) · [Casos de prueba](../tests/verify_build_diagnostics.cpp)
