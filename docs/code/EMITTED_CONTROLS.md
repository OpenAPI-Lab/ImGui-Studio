# Helpers de controles en código generado

[English](EMITTED_CONTROLS.en.md)

Estas cabeceras contienen implementaciones emitidas literalmente y helpers que traducen nodos a los tipos del destino. Compilar ImGui Studio solo valida el contenedor C++. Cualquier cambio en un bloque emitido requiere compilar o interpretar el lenguaje generado.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [studio_codegen_titlebar.hpp](../../include/imguistudio/export/detail/studio_codegen_titlebar.hpp) | Incluye métodos emitidos de título para salidas nativas. | Conserva posiciones, balance de estilos y pulsos de botón por fotograma. |
| [studio_managed_titlebar.hpp](../../include/imguistudio/export/detail/studio_managed_titlebar.hpp) | Convierte el árbol en elementos como `Button` y `WindowTitle`. | Centraliza selección de hijos visibles y datos del título para que todos los bindings reciban la misma entrada. |
| [studio_titlebar_csharp.hpp](../../include/imguistudio/export/detail/studio_titlebar_csharp.hpp) | Implementa el chrome emitido con ImGui.NET. | Las variantes de campo embebido y botón dedicado tienen distinta visibilidad al plegar. |
| [studio_titlebar_java.hpp](../../include/imguistudio/export/detail/studio_titlebar_java.hpp) | Implementa el encabezado emitido con imgui-java. | Comprueba clics, etiquetas e IDs dentro del alcance de cada ventana. |
| [studio_titlebar_python.hpp](../../include/imguistudio/export/detail/studio_titlebar_python.hpp) | Emite los helpers de iconos y encabezado para imgui-bundle. | Conserva los retornos de cada fotograma y el orden de las llamadas del binding. |
| [studio_titlebar_rust.hpp](../../include/imguistudio/export/detail/studio_titlebar_rust.hpp) | Emite `TitleButton`, `TitleAppearance`, `WindowMode` y `TitleContext`. | Separa configuración, estado y contexto temporal. Compila el host si cambian préstamos o callbacks. |
| [studio_codegen_image.hpp](../../include/imguistudio/export/detail/studio_codegen_image.hpp) | Describe recursos y emite adaptadores C/C++ para dibujar imágenes. | Conserva la independencia del recurso respecto al backend, además de tamaño y estado de clic. |
| [studio_managed_images.hpp](../../include/imguistudio/export/detail/studio_managed_images.hpp) | Incluye cargadores y dibujo emitidos para Python y C#. | El backend proporciona texturas vivas durante el dibujo y las libera al cerrar. |
| [studio_images_java_rust.hpp](../../include/imguistudio/export/detail/studio_images_java_rust.hpp) | Compone helpers de carga, textura y dibujo para Java y Rust. | Java usa recursos del classpath y Rust incorpora recursos durante la compilación. Verifica ambos empaquetados. |
| [studio_codegen_animation.hpp](../../include/imguistudio/export/detail/studio_codegen_animation.hpp) | Emite bindings C/C++ y funciones de la animación nativa. | Mantén la forma de calcular el ciclo y de restaurar el estilo al terminar de dibujar los hijos. |
| [studio_managed_animation.hpp](../../include/imguistudio/export/detail/studio_managed_animation.hpp) | Recopila hijos escalados y emite lógica de animación para Python y C#. | Conservar unidades y tamaños evita que el layout y la fase de animación diverjan. |
| [studio_animation_java.hpp](../../include/imguistudio/export/detail/studio_animation_java.hpp) | Emite los helpers de animación con imgui-java. | La capa emitida debe equilibrar los cambios de estilo al salir del contenedor. |
| [studio_animation_rust.hpp](../../include/imguistudio/export/detail/studio_animation_rust.hpp) | Emite estructuras `AnimationStyle` y `Animation` para controlar los efectos. | Comprueba el alcance del estilo y las medidas usadas con hijos de altura automática. |
| [studio_codegen_chart2d.hpp](../../include/imguistudio/export/detail/studio_codegen_chart2d.hpp) | Emite bindings y funciones compartidas más los modos 2D de C/C++. | Las rutas C y C++ usan firmas de binding distintas. Prueba ambas y los modos con datos vacíos. |
| [studio_codegen_chart3d.hpp](../../include/imguistudio/export/detail/studio_codegen_chart3d.hpp) | Emite la proyección y dibujo 3D para C/C++. | Conserva orientación inicial, controles de órbita y orden de dibujo. |
| [studio_chart2d_csharp.hpp](../../include/imguistudio/export/detail/studio_chart2d_csharp.hpp) | Emite muestreo y modos 2D para ImGui.NET. | Comprueba el tamaño de la serie y el dibujo por coordenadas del binding. |
| [studio_chart2d_java.hpp](../../include/imguistudio/export/detail/studio_chart2d_java.hpp) | Emite muestreo y modos 2D para imgui-java. | Comprueba los límites de arrays y los overloads del draw list. |
| [studio_chart2d_python.hpp](../../include/imguistudio/export/detail/studio_chart2d_python.hpp) | Emite el muestreo y helpers 2D de Python, incluido el gráfico circular. | Comprueba la escala automática y la forma de los argumentos de imgui-bundle. |
| [studio_chart2d_rust.hpp](../../include/imguistudio/export/detail/studio_chart2d_rust.hpp) | Emite `chart_samples` con su buffer de índices y el renderer `Chart2d`. | El tamaño del buffer fija el límite de muestreo. Preserva slices válidos para series cortas y vacías. |
| [studio_chart3d_csharp.hpp](../../include/imguistudio/export/detail/studio_chart3d_csharp.hpp) | Emite el estado, proyección y render 3D para C#. | Comprueba que la entrada modifique solo el estado de la vista generada. |
| [studio_chart3d_java.hpp](../../include/imguistudio/export/detail/studio_chart3d_java.hpp) | Emite proyección, caras y render 3D con imgui-java. | Verifica color, orden de caras y generación del estado de órbita. |
| [studio_chart3d_python.hpp](../../include/imguistudio/export/detail/studio_chart3d_python.hpp) | Emite las funciones matemáticas y de dibujo 3D de Python. | Comprueba proyección y datos inválidos en el destino, además de la sintaxis. |
| [studio_chart3d_rust.hpp](../../include/imguistudio/export/detail/studio_chart3d_rust.hpp) | Emite `Chart3dState` y los tipos de puntos, caras y cámara. | El tamaño de la serie está en el tipo. Conserva consistencia entre el estado emitido y el helper. |

## Verificación

Para una propiedad nueva, prueba el inspector y compila el destino que consume el helper. Repite el ejemplo [Barra de título](../TITLE_BAR.md), [Estado y progreso](../../examples/estado-y-progreso.json) o [Imágenes y entrada](../../examples/vulkan-imagenes.json), según la ruta. Incluye el archivo generado y el rango del nodo si falla un compilador.

