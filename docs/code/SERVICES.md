# Asistente, credenciales y extensiones

[English](SERVICES.en.md)

El flujo de red, los secretos y la aplicación de cambios son responsabilidades distintas. El catálogo y el asistente pueden compartir transporte y mantienen contratos distintos de datos.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [studio_assistant_api.hpp](../../include/imguistudio/services/studio_assistant_api.hpp) | Define formatos por proveedor, endpoints, autenticación y conversión de respuestas. | Valida la conexión antes de preparar la petición. Un nuevo proveedor requiere fixtures de petición y respuesta. |
| [studio_assistant_proposal.hpp](../../include/imguistudio/services/studio_assistant_proposal.hpp) | `readAssistantProposal` interpreta la propuesta. `applyAssistantProposal` propone una edición del árbol. | El contenido de una respuesta se valida como datos del proyecto y la propuesta debe corresponder al documento de origen. |
| [studio_assistant_http.hpp](../../src/services/studio_assistant_http.hpp) | Declara el resultado HTTP y el límite del transporte. | Los llamadores pueden tratar cancelación y error sin usar directamente libcurl. |
| [studio_assistant_http.cpp](../../src/services/studio_assistant_http.cpp) | Ejecuta el intercambio, acumula una respuesta acotada y consulta la cancelación. | Un fallo de conexión debe volver como resultado. Conserva las restricciones de protocolo y verificación TLS. |
| [studio_assistant_panel.hpp](../../src/services/studio_assistant_panel.hpp) | Declara preferencias, petición, conversación y estado de la UI. | El panel conserva secretos temporales separados de los ajustes serializados. |
| [studio_assistant_panel.cpp](../../src/services/studio_assistant_panel.cpp) | Carga preferencias del proveedor, inicia trabajo, consulta su estado y revisa propuestas. | El fotograma puede seguir respondiendo durante el trabajo. Aplicar depende de la acción del usuario y del diseño base. |
| [studio_assistant_vault.hpp](../../src/services/studio_assistant_vault.hpp) | Declara resultados, disponibilidad y cancelación del llavero. | La UI no necesita interpretar detalles de WinCred, Secret Service o Keychain. |
| [studio_assistant_vault.cpp](../../src/services/studio_assistant_vault.cpp) | Implementa adaptadores de almacenamiento y carga por plataforma. | Una entrada se identifica por proveedor y URL base. El proyecto JSON y el código emitido quedan fuera del llavero. |
| [studio_extensions.hpp](../../src/services/studio_extensions.hpp) | Declara `StudioExtensionCatalog` y la interfaz de instalación usada por la UI. | La presencia en la paleta depende del catálogo. Los nodos del diseño conservan sus datos. |
| [studio_extensions.cpp](../../src/services/studio_extensions.cpp) | Descarga y compara los archivos fijados de imgui-knobs y conserva el registro de instalación. | Solo activa la entrada tras comprobar los bytes y confirmar el registro. El renderer usa el adaptador compilado. |

### Ejemplo en C++

Este caso usa una respuesta local de ejemplo. Ilustra que la revisión queda ligada al diseño capturado y que un cambio posterior invalida la propuesta.

```cpp
#include "imguistudio/services/studio_assistant_proposal.hpp"
#include <cassert>

int main() {
    using namespace ImGuiStudio;
    StudioProject project = createDefaultProject();
    const auto reviewed = projectToJson(project);
    const auto proposal = readAssistantProposal(
        "```imgui-json\n{\"type\":\"text\",\"label\":\"Status\"}\n```");
    assert(proposal.error.empty());
    assert(proposal.windows.size() == 1);

    const auto accepted = applyAssistantProposal(project, proposal, reviewed);
    assert(accepted);
    project.name = "Changed after review";
    const auto rejected = applyAssistantProposal(project, proposal, reviewed);
    assert(!rejected && !rejected.error.empty());
}
```

## Verificación

Usa `AssistantApiTest`, `AssistantLoopbackTest` y `NativeExtensionCatalogTest`. Un cambio de proveedor debe funcionar contra el servidor local de prueba y cubrir respuestas rechazadas. Una extensión requiere conservar procedencia y licencia en el proyecto que la incorpore. [Asistente](../ASSISTANT.md) · [Biblioteca](../WIDGET_LIBRARY.md)

