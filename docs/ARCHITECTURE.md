# Arquitectura

[English](ARCHITECTURE.en.md)


| Capa | Responsabilidad y motivo |
| --- | --- |
| Host gráfico | [src/app/main.cpp](../src/app/main.cpp) inicia GLFW, OpenGL, Dear ImGui y las fuentes. Mantiene el ciclo de eventos y entrega cada fotograma a `StudioApp`. |
| Editor | [studio_ui.cpp](../src/editor/studio_ui.cpp) coordina diálogos, paneles y sesión. [studio_canvas.cpp](../src/editor/studio_canvas.cpp) interpreta selección, geometría y arrastre. |
| Documento | [studio_types.hpp](../include/imguistudio/project/studio_types.hpp) define nodos, propiedades, estilo y lienzo. [studio_document_edit.hpp](../include/imguistudio/project/studio_document_edit.hpp) valida ediciones estructurales para que UI y otros llamadores compartan las mismas reglas. |
| Vista previa | [studio_preview.cpp](../src/editor/studio_preview.cpp) dibuja controles del documento y recoge estado transitorio. La interacción de la vista previa se separa de las propiedades guardadas. |
| Disco | [studio_serialization.hpp](../include/imguistudio/project/studio_serialization.hpp) interpreta el JSON. [studio_project_io.hpp](../include/imguistudio/project/studio_project_io.hpp) se ocupa de leer y reemplazar archivos. [studio_session_store.hpp](../include/imguistudio/project/studio_session_store.hpp) gestiona historial, recuperación y concurrencia. |
| Exportación | Los generadores recorren el documento y producen código, rangos por nodo y diagnósticos. El empaquetador agrega el host y, cuando procede, dependencias o recursos. |
| Build | [studio_build.hpp](../include/imguistudio/build/studio_build.hpp) exporta una copia del documento vivo, configura el toolchain y gestiona el artefacto. [studio_process.hpp](../include/imguistudio/build/studio_process.hpp) captura salida y controla el subproceso. |
| Servicios | `studio_assistant_*` separa proveedor HTTP, almacenamiento de credenciales y validación de propuestas. `studio_extensions.*` gestiona la instalación verificada del catálogo. |

## Recorrido de un cambio

Un arrastre parte de la geometría observada en la vista previa. El lienzo calcula el destino y el editor confirma una edición del árbol. El documento resultante puede guardarse o exportarse. Durante la edición, la recuperación escribe checkpoints por separado. Una compilación toma el diseño vivo y escribe su propia carpeta de salida.

La diferencia entre modelo, render y ejecución explica por qué un nodo puede conservarse en JSON aunque un exportador emita un marcador. Consulta [el formato](PROJECT_FORMAT.md), [la exportación](EXPORTING.md) y [el mapa de fuentes](SOURCE_MAP.md).
