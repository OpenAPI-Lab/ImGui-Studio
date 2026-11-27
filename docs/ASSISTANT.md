# Asistente y proveedores

[English](ASSISTANT.en.md)


Abre **Vista → Asistente IA** y elige proveedor y modelo. **Detectar modelos** consulta el servidor configurado. Una detección exitosa comprueba que el servidor devolvió su lista de modelos. Enviar una consulta y aplicar una propuesta son acciones separadas.

## Conexión y credenciales

| Entrada | Comportamiento |
| --- | --- |
| Ollama o LM Studio local | El editor propone la URL de loopback. Las claves son opcionales. |
| OpenAI, Anthropic o Gemini | La solicitud requiere una clave. Las variables `OPENAI_API_KEY`, `ANTHROPIC_API_KEY` y `GEMINI_API_KEY` se consultan para el endpoint oficial correspondiente si el campo temporal está vacío. |
| URL personalizada | El editor valida la URL base, incorpora la ruta del proveedor y exige HTTPS cuando envía una clave a un servidor remoto. |
| Clave temporal | Permanece en el campo de la sesión y se limpia al cambiar de proveedor o URL. |
| Clave guardada | La acción explícita de guardar usa el almacén de credenciales del sistema. La entrada está asociada al proveedor y la URL. |

`assistant_settings.json` guarda proveedor, endpoints y modelos en el directorio de estado. El dato guardado en el llavero se carga mediante **Claves guardadas en el sistema**. Linux requiere libsecret y un Secret Service activo en la sesión para usar el llavero. Windows utiliza WinCred. La implementación de Keychain queda pendiente de validar en macOS.

## De la consulta al diseño

Al pulsar **Enviar**, el servidor recibe la consulta, el contexto reciente y un resumen del diseño. Revisa esos datos si trabajas con un proyecto privado. El editor presenta la propuesta antes de modificar el documento. **Agregar al diseño** comprueba que siga correspondiendo al diseño actual y la aplica como edición deshacible. Para una propuesta obsoleta, solicita otra sobre el estado vigente.

## Pruebas y diagnóstico

CMake busca libcurl 7.85 o superior. Con la conexión activada y Python disponible, `AssistantLoopbackTest` usa un servidor local simulado y credenciales ficticias para comprobar las cinco variantes. `AssistantApiTest` verifica formatos, reglas de URL y validación de propuestas. Si preparas un cambio de proveedor, conserva el caso simulado y documenta por separado cualquier prueba en un servicio externo.

[Formatos de solicitud](../include/imguistudio/services/studio_assistant_api.hpp) · [Validación de propuestas](../include/imguistudio/services/studio_assistant_proposal.hpp) · [Política de reportes](../SECURITY.md)
