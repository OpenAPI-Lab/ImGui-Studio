# Modelo, serialización y persistencia

[English](PROJECT.en.md)

`StudioProject` es el modelo compartido por el editor y los exportadores. El código separa interpretación de JSON, cambios del árbol, reemplazo de archivos e historial para que cada operación pueda fallar con un resultado propio.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [studio_types.hpp](../../include/imguistudio/project/studio_types.hpp) | Define `StudioProject`, `ImGuiNode`, `WidgetProps` y `canAcceptChild`. | Es la fuente de las reglas de jerarquía. Un nodo mantiene su ID al moverse y conserva datos de origen para la serialización. |
| [studio_widget_registry.inc](../../include/imguistudio/project/studio_widget_registry.inc) | El macro `STUDIO_WIDGET` define el enum, el ID de JSON y la condición de contenedor. | Un registro común evita mapeos divergentes. Conserva los IDs persistentes y el orden existente del enum. |
| [studio_serialization.hpp](../../include/imguistudio/project/studio_serialization.hpp) | `parseProject` limita la entrada, `projectFromJson` valida y `projectToJson` reconstruye el archivo. | Un tipo desconocido se conserva con aviso. IDs duplicados, jerarquía inválida y exceso de límites producen error. La comparación es semántica. |
| [studio_document_edit.hpp](../../include/imguistudio/project/studio_document_edit.hpp) | Planifica inserciones y movimientos, aplica posición y tamaño, y devuelve `DocumentEditResult`. | El resultado se valida antes de comprometerlo. Un cambio inválido conserva la entrada y un resultado idéntico evita una entrada de deshacer. |
| [studio_project_io.hpp](../../include/imguistudio/project/studio_project_io.hpp) | `readTextFile` acota la lectura y `writeTextFileAtomically` escribe y reemplaza con un temporal vecino. | El temporal se escribe y vacía antes de reemplazar el destino. Los pasos de historial y detección de modificaciones se resuelven en la sesión. |
| [studio_snapshots.hpp](../../include/imguistudio/project/studio_snapshots.hpp) | `SnapshotManager` captura y compara diseños, limita retención y devuelve una copia al revertir. | La versión se separa del archivo principal. Revertir permite resguardar el estado que se abandona. |
| [studio_session_store.hpp](../../include/imguistudio/project/studio_session_store.hpp) | `SessionFileLease`, `ProjectHistoryFile` y `RecoverySession` coordinan revisión, lock y checkpoint. | La revisión se compara bajo lock. Una copia fallida o un formato futuro requieren conservar datos y mostrar el error. |

## Ejemplo del contrato

`moveDocumentNode(project, "job_progress", "status_window", 0)` propone insertar un nodo existente en el primer hueco del contenedor. Usa el proyecto retornado solo si el resultado contiene uno. Un resultado sin proyecto y sin `error` indica que no se aplicó ningún cambio. Mira el [fixture de edición](../../tests/verify_document_edit.cpp) para entradas válidas y rechazadas.

### Ejemplo en C++

Prueba con `examples/estado-y-progreso.json` como argumento. El ejemplo compara el árbol propuesto con el original y deja el archivo intacto.

```cpp
#include "imguistudio/project/studio_document_edit.hpp"
#include "imguistudio/project/studio_project_io.hpp"
#include <cassert>

int main(int argc, char** argv) {
    using namespace ImGuiStudio;
    if (argc != 2) return 2;
    const auto loaded = loadProjectFile(std::filesystem::u8path(argv[1]));
    if (!loaded) return 1;

    const auto result = moveDocumentNode(*loaded.project, "job_progress",
                                         "status_window", 0);
    if (!result || !result.error.empty()) return 1;
    const auto* window = findNodeById(result.project->nodes, "status_window");
    assert(window && window->children.front().id == "job_progress");
    assert(loaded.project->nodes.front().children.front().id == "auto_toggle");
}
```

## Verificación

Ejecuta `CoreAndCodegenTest`, `ProjectPersistenceTest`, `DocumentEditingTest` y `SessionRecoveryTest`. Para cambios del formato, incluye un diseño anterior y comprueba que sus datos desconocidos sobrevivan al ciclo. [Formato de proyecto](../PROJECT_FORMAT.md) · [Recuperación](../RECOVERY.md)

