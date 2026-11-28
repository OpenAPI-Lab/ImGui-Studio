# Arranque, interfaz y lienzo

[English](EDITOR.en.md)

El host posee GLFW y el contexto ImGui. `StudioApp` coordina el documento y usa al renderer para observar la geometría. El lienzo produce propuestas de edición validadas antes de guardarlas en el historial.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [studio_editor_appearance.hpp](../../src/editor/studio_editor_appearance.hpp) | Define paletas, aplica el redondeo del editor y lee y escribe sus preferencias. | El editor guarda estas opciones en `appearance.json` y el diseño mantiene su propio estilo. |
| [studio_project_file_picker.hpp](../../src/editor/studio_project_file_picker.hpp) | Enumera carpetas y archivos JSON. Para guardar, forma un destino con la carpeta y el nombre. | Ambos flujos usan la misma navegación en el selector integrado. El flujo de guardado añade `.json` si el nombre carece de extensión. |
| [main.cpp](../../src/app/main.cpp) | Inicializa ventana, OpenGL, ImGui, fuentes y bucle. Implementa las pruebas de arranque y glifos. | Crea y destruye recursos en orden. La ruta de smoke dibuja fotogramas ocultos con persistencia deshabilitada. |
| [studio_fonts.hpp](../../src/app/studio_fonts.hpp) | `loadStudioFonts` busca fuentes y construye el atlas de la interfaz. | La cobertura depende de los archivos disponibles. Cambia rangos y búsqueda con una prueba de glifos en los seis catálogos. |
| [studio_ui.hpp](../../src/editor/studio_ui.hpp) | Declara `StudioApp`, selección, estado de proyecto, diálogos e historial. | Concentra el estado que sobrevive un fotograma. Mantén los servicios y el documento bajo el dueño adecuado. |
| [studio_ui.cpp](../../src/editor/studio_ui.cpp) | `commitChange`, `undo` y `redo` coordinan diseño y efectos de sesión. Contiene los paneles y acciones. | La confirmación central mantiene versiones, exportación y compilaciones sincronizadas con el documento. |
| [studio_canvas.hpp](../../src/editor/studio_canvas.hpp) | Declara `NativeCanvas` y `CanvasUpdate` para los gestos. | La propuesta comunica selección o edición al editor sin que el gesto posea el archivo del proyecto. |
| [studio_canvas.cpp](../../src/editor/studio_canvas.cpp) | `hitTest` y `process` eligen geometría visible y resuelven arrastre, tamaño y destinos. | Un gesto pendiente anticipa el destino. El documento cambia al confirmar, con un solo paso de deshacer. |
| [studio_preview.hpp](../../src/editor/studio_preview.hpp) | Define `PreviewItem`, `PreviewReport`, estado de ventana y `NativePreview`. | Separa geometría observada, limitaciones y valores transitorios de las propiedades del diseño. |
| [studio_preview.cpp](../../src/editor/studio_preview.cpp) | Dibuja controles nativos y el inspector, registra geometría y restaura estilos personalizados. | La sesión de preview puede editar valores temporales. Los estilos y límites del diseño se aplican localmente para proteger el resto del editor. |

## Recorrido de una edición

Abre [el ejemplo de lienzo](../../examples/barra-y-lienzo-libre.json) y arrastra un hijo. El renderer publica `PreviewItem`, el lienzo resuelve el objetivo y `StudioApp` confirma el proyecto devuelto. **Esc** cancela antes de confirmar. Ese recorrido permite localizar un fallo de hit testing, validación o persistencia por separado.

## Selector de proyectos

`requestProjectPicker` prepara el modo **Abrir** o **Guardar como**. Empieza en la carpeta del proyecto actual cuando existe. Al abrir desde el inicio usa la carpeta del último reciente si la conoce. Para el primer guardado de una copia recuperada puede usar la carpeta de origen. La carpeta personal es el valor de respaldo.

`renderProjectFilePicker` usa `GetOpenFileNameW` o `GetSaveFileNameW` si hay una ventana activa de Windows. El selector integrado usa `ProjectFileBrowser` para enumerar una carpeta y añade `.json` al guardar un nombre sin extensión. Abrir pasa la ruta a `openProject`, que carga y valida antes de reemplazar el diseño. Guardar pasa la ruta a `saveProjectTo`, que conserva la comprobación de cambios externos cuando se guarda sobre el archivo actual. Un resultado inválido deja la selección abierta con su error. Cancelar deja intacto el proyecto actual.

## Preferencias visuales

`EditorAppearance` guarda el identificador de la paleta y el redondeo en `appearance.json`, dentro del [directorio de estado](../PROJECT_FORMAT.md#directorio-de-estado). Si una preferencia es inválida, se usa su valor predeterminado. `applyEditorAppearance` configura las superficies de la aplicación y el viewport toma el color de fondo de la paleta. Durante el dibujo del proyecto, el canvas conserva y restaura ese estilo para aplicar las propiedades del diseño en su propio ámbito.

## Verificación

Usa `NativeCanvasEditingTest`, `NativeWidgetPreviewTest`, `NativeImagePreviewTest` y `SessionRecoveryUiTest`. La prueba de interfaz comprueba el selector integrado, archivos con acentos, rechazos de JSON inválido, cancelación y persistencia de apariencia. Para verificar el selector del sistema, abre y guarda desde una ventana normal en Windows. Si cambia la geometría, repite una prueba manual con clipping, zoom y una ventana superpuesta. [Lienzo](../CANVAS.md)

