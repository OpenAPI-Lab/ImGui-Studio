# Implementación de widgets y recursos

[English](WIDGET_RENDER.en.md)

El modelo guarda valores y recursos. Los helpers interpretan esos datos y el renderer administra los objetos gráficos de la sesión. Este límite permite reutilizar la misma entrada en varios backends.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [studio_images.hpp](../../src/widgets/studio_images.hpp) | Define `ImageImport` y la interfaz de `NativeImageCache`. | Distingue el recurso serializado de la textura dependiente del contexto. |
| [studio_images.cpp](../../src/widgets/studio_images.cpp) | Importa imágenes, decodifica con stb, crea texturas y libera entradas del caché. | El caché evita subir cada imagen por fotograma. La liberación requiere que el contexto aún esté disponible. |
| [studio_chart2d.hpp](../../src/widgets/studio_chart2d.hpp) | `sampleIndices`, `cartesian`, `pie` y `render` dibujan los modos 2D. | El muestreo acota el dibujo. Comprueba escalas, series vacías y correspondencia entre cursor y datos. |
| [studio_chart3d.hpp](../../src/widgets/studio_chart3d.hpp) | `Projection` y las funciones de puntos y caras proyectan los modos 3D en el draw list. | La orientación afecta el orden visual de las caras. Conserva rangos y separación entre orientación del diseño y órbita temporal. |
| [studio_widget_library.hpp](../../include/imguistudio/widgets/studio_widget_library.hpp) | `StudioWidgetLibrary` valida, lee y escribe definiciones. `instantiateLibraryWidget` copia y reasigna IDs. | Las instancias deben ser independientes y conservar recursos propios después de insertar. |
| [studio_image_data.hpp](../../include/imguistudio/widgets/studio_image_data.hpp) | Valida el recurso base64, sus dimensiones y límites. `ExportAssets` organiza recursos de salida. | El exportador necesita bytes validados y el backend decide cómo crear la textura. Comprueba PNG, JPEG y entradas rechazadas. |
| [studio_window_appearance.hpp](../../include/imguistudio/widgets/studio_window_appearance.hpp) | Registra propiedades numéricas, colores y presets de ventana, además de elegibilidad para el título. | El modelo y el generador necesitan las mismas reglas para preservar valores y reproducir la apariencia. |
| [studio_numeric_format.hpp](../../include/imguistudio/widgets/studio_numeric_format.hpp) | `safeNumericFormat` acepta formatos compatibles con el valor numérico esperado. | Protege el contrato variádico de Dear ImGui. Si se rechaza un formato, conserva el original en el diseño y usa un formato válido al dibujar. |
| [studio_knob.hpp](../../include/imguistudio/widgets/studio_knob.hpp) | `knobSettings` interpreta rango, valor y velocidad de la perilla. | Un ajuste compartido mantiene coherencia entre preview y C++. Prueba rangos degenerados y valores fuera de rango. |
| [studio_status_widgets.hpp](../../include/imguistudio/widgets/studio_status_widgets.hpp) | Implementa `toggle`, `spinner`, `ring` y `circularProgress` encima de Dear ImGui. | CMake inserta este código en proyectos generados que lo usan. Conserva su dependencia directa de Dear ImGui. |

## Verificación

Abre [Estado y progreso](../../examples/estado-y-progreso.json) y [el proyecto de imágenes](../../examples/vulkan-imagenes.json). Ejecuta `StatusWidgetExportTest`, `NativeImagePreviewTest` y `NativeWidgetPreviewTest`. Comprueba los recursos tanto en el editor como en el host exportado del backend afectado. [Biblioteca](../WIDGET_LIBRARY.md)

