# Implementación de la localización

[English](LOCALIZATION.en.md)

Las etiquetas se resuelven con claves estables. La traducción puede cambiar el texto visible mientras el identificador conserva el mismo control ImGui.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [studio_locale.hpp](../../include/imguistudio/localization/studio_locale.hpp) | `StudioLocale` resuelve catálogos y `ScopedStudioLocale` instala el contexto para una superficie. | `surfaceText` sirve para texto y `surfaceLabel` conserva identidad. Una traducción ausente o inválida usa inglés. |
| [studio_locale_strings.inc](../../include/imguistudio/localization/studio_locale_strings.inc) | Define `StudioText` y los textos base español e inglés con `STUDIO_TEXT`. | La misma lista alimenta enum y catálogos. Conserva las claves que usan los paquetes comunitarios. |
| [studio_locale_extra.inc](../../include/imguistudio/localization/studio_locale_extra.inc) | Registra las otras cuatro traducciones integradas por clave. | Actualiza sus entradas al introducir una clave y verifica que el atlas contenga sus caracteres. |

## Ejemplo y verificación

Usa `surfaceLabel(StudioText::SaveProject)` al construir un control que necesita identidad. Para texto sin interacción, usa `surfaceText`. Comprueba cambiar de idioma sin perder el estado del control y ejecuta `--font-smoke-test` en una configuración con las fuentes pertinentes. [Guía de idiomas](../LOCALIZATION.md) · [Atlas](../../src/app/studio_fonts.hpp)

