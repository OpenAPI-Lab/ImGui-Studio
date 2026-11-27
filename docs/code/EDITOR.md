# Arranque, interfaz y lienzo

[English](EDITOR.en.md)

El host posee GLFW y el contexto ImGui. `StudioApp` coordina el documento y usa al renderer para observar la geometría. El lienzo produce propuestas de edición validadas antes de guardarlas en el historial.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
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

## Verificación

Usa `NativeCanvasEditingTest`, `NativeWidgetPreviewTest`, `NativeImagePreviewTest` y `SessionRecoveryUiTest`. Repite una prueba manual con clipping, zoom y una ventana superpuesta cuando cambie la geometría. [Lienzo](../CANVAS.md)

