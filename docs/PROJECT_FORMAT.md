# Proyecto, historial y recuperación

[English](PROJECT_FORMAT.en.md)


## Contrato de archivo

El JSON del proyecto almacena diseño. Incluye el nombre, el árbol de nodos, propiedades, estilo y configuración del lienzo. Los nodos usan identificadores para edición y diagnóstico. La jerarquía se valida al cargar y editar. Si un archivo supera 32 MiB, 100 000 nodos o 128 niveles, la carga falla y el documento abierto se conserva. El lector y escritor preservan datos desconocidos donde el modelo conserva su fuente. La comparación de JSON es semántica y el formato de espacios puede cambiar.

[Ejemplo de proyecto](../examples/barra-de-titulo.json) · [Validador y serializador](../include/imguistudio/project/studio_serialization.hpp)

## Tres tipos de persistencia

| Archivo | Qué contiene | Por qué existe |
| --- | --- | --- |
| `proyecto.json` | Diseño y propiedades persistentes | Permite compartir el diseño y volver a exportarlo. |
| `proyecto.json.imguistudio/history.json` | Versiones del proyecto | Separa snapshots del diseño portable. Mantiene hasta 35 versiones. |
| `recovery/` en el directorio de estado | Checkpoints de sesión, historial y undo/redo | Permite recuperar cambios pendientes tras un cierre interrumpido. |

Al guardar se comprueba si el archivo cambió en disco y se usa reemplazo mediante un archivo temporal. Los locks coordinan sesiones concurrentes. La mera existencia de un `.lock` no indica que un proceso lo mantenga.

**Recuperar copia** abre un documento sin ruta. Usa **Guardar como** para elegir destino. **Descartar** retira la copia confirmada y **Más tarde** la conserva. El último intervalo antes de un checkpoint puede perderse. Las diferencias y los errores del historial se presentan por separado del guardado del proyecto.

## Directorio de estado

Windows usa `%LOCALAPPDATA%/ImGuiStudio`. Linux usa `$XDG_STATE_HOME/ImGuiStudio` o `~/.local/state/ImGuiStudio`. macOS usa `~/Library/Application Support/ImGuiStudio`. Define `IMGUI_STUDIO_STATE_DIR` para aislar una prueba. El directorio contiene también recientes, ajustes de idioma, biblioteca, extensiones y perfiles del editor.

## Verificación

`ProjectPersistenceTest` recorre fixtures y escrituras. `DocumentEditingTest` comprueba operaciones válidas e inválidas del árbol. `SessionRecoveryTest` cubre checkpoints, concurrencia y archivos dañados. Consulta el [mapa de pruebas](BUILD_AND_TEST.md).
