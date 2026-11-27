# Guardado, versiones y recuperación

[English](RECOVERY.en.md)

## Qué restaura cada opción

| Acción | Resultado |
| --- | --- |
| **Guardar proyecto** | Escribe el diseño actual en el JSON. |
| **Guardar versión** | Captura una versión en el historial adjunto. |
| **Revertir** en el historial | Carga la versión en el editor. El historial puede guardar un respaldo del estado anterior. Guarda el proyecto para actualizar el JSON. |
| **Recuperar copia** al iniciar | Abre el estado pendiente como un proyecto sin ruta. Usa **Guardar proyecto como…** para guardarlo. |
| **Más tarde** | Conserva la copia para consultarla después en **Archivo → Recuperar sesiones anteriores…**. |
| **Descartar… → Sí, descartar copia** | Elimina esa copia de recuperación. |

## Ejemplo con un archivo de prueba

Abre [Estado y progreso](../examples/estado-y-progreso.json) y guárdalo con otro nombre. Abre **Vista → Historial de versiones** y guarda una versión llamada «Antes del cambio». Cambia el texto de **Procesado** en el inspector y guarda el proyecto. Pulsa **Revertir** junto a «Antes del cambio» y observa el texto en el editor. Pulsa **Guardar proyecto** para hacer permanente ese estado.

Si el editor informa que el archivo cambió desde que lo abriste, usa **Guardar proyecto como…** para conservar tu edición en otro archivo. Compara las dos versiones antes de reemplazar la original. Guarda una copia del archivo de historial antes de reparar metadatos dañados de forma manual.

## Qué protege el estado

`saveProjectIfUnchanged` usa un lock y compara el contenido al guardar para detectar conflictos. El reemplazo escribe y vacía un temporal en el mismo directorio. El historial auxiliar conserva hasta 35 snapshots. Recuperación conserva el diseño y hasta 40 estados en cada pila de deshacer y rehacer. El editor separa intentos de checkpoint por dos segundos. Un cierre abrupto puede perder las últimas ediciones. Los locks activos son del sistema operativo. La existencia de un archivo `.lock` por sí sola no indica que una sesión siga abierta.

## Verificación

`SessionRecoveryTest` verifica conflictos, locks activos, copia interrumpida y registros dañados. `SessionRecoveryUiTest` dibuja el diálogo de recuperación en un contexto Dear ImGui. Reporta el resultado de abrir, guardar, recuperar o descartar usando un proyecto de ejemplo y el mensaje exacto. Usa `IMGUI_STUDIO_STATE_DIR` para aislar un ensayo.

[Formato de proyecto](PROJECT_FORMAT.md) · [Bloqueo y recuperación](../include/imguistudio/project/studio_session_store.hpp) · [Escritura de archivos](../include/imguistudio/project/studio_project_io.hpp)
