# Ampliar el editor

[English](EXTENDING.en.md)


## Un widget

1. Define su tipo, representación serializada y valores iniciales en [el modelo](../include/imguistudio/project/studio_types.hpp) y [el registro](../include/imguistudio/project/studio_widget_registry.inc). Conserva los nombres persistentes cuando amplíes un tipo existente.
2. Añade el control al inspector y su render en [la vista previa](../src/editor/studio_preview.cpp). Mantén los valores de interacción separados de los iniciales. Declara el tamaño y comportamiento dentro de contenedores.
3. Implementa el render en los generadores compatibles, o conserva un diagnóstico indicando el marcador emitido. Comprueba si usa recursos, código auxiliar, título de ventana o una dependencia adicional.
4. Comprueba guardado/apertura, render y salida generada. [Los widgets de estado](../include/imguistudio/widgets/studio_status_widgets.hpp) muestran cómo compartir una implementación C++ entre vista previa y exportación.

## Ejemplo para orientarte en el código

Sigue **Progreso circular** antes de cambiar un control. Abre [el proyecto de ejemplo](../examples/estado-y-progreso.json), selecciona `job_progress` y localiza sus propiedades `fraction` y `overlay`.

| Lugar | Qué revisar y por qué |
| --- | --- |
| [Registro de widgets](../include/imguistudio/project/studio_widget_registry.inc) | `circular_progress` es el identificador persistente. Mantenerlo permite abrir diseños anteriores. |
| [Serialización](../include/imguistudio/project/studio_serialization.hpp) | `readProps` y `propsJson` leen y escriben las propiedades del nodo. Si añades una, prueba abrir un diseño que todavía carezca de ella. |
| [Vista previa e inspector](../src/editor/studio_preview.cpp) | Sigue el caso `WidgetType::CircularProgress` y los campos del inspector. Comprueba que editar valores iniciales y cambiar valores temporales tengan el efecto esperado. |
| [Dibujo compartido](../include/imguistudio/widgets/studio_status_widgets.hpp) | El helper concentra el dibujo para que el editor y el C++ exportado muestren el mismo control. |
| [Generador C++](../include/imguistudio/export/studio_codegen_native.hpp) | Emite la llamada y el estado generado. En otros destinos, comprueba que el informe declare cualquier propiedad que quede sin soporte. |

Si cambias el radio o los colores, comprueba el control en vista previa y en un proyecto C++ generado. Si cambias el formato, abre, guarda y vuelve a abrir el ejemplo. Documenta en ambos idiomas qué comportamiento se puede comprobar. [Guía de validación](VALIDATION.md)

## Una plantilla o un exportador

Parte de [CppBundle](../include/imguistudio/export/studio_cpp_export.hpp) y la [plantilla opcional de Android](../include/imguistudio/export/studio_android_export.hpp). Valida el descriptor, conserva `codegen_report.txt`, incluye dependencias necesarias y documenta el comando de construcción. En una extensión revisa la licencia y la revisión de origen. Si integras código externo, conserva sus avisos en el proyecto generado cuando corresponda.

## Una traducción

Agrega o modifica la clave en [el catálogo inglés/español](../include/imguistudio/localization/studio_locale_strings.inc) y su equivalente en [el catálogo adicional](../include/imguistudio/localization/studio_locale_extra.inc). Pasa la etiqueta por el mecanismo de traducción y conserva el identificador ImGui cuando el texto visible pueda variar. Comprueba un idioma de texto largo, otro corto y los glifos requeridos. Consulta [localización](LOCALIZATION.md) para paquetes externos.
