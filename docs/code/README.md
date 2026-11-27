# Cómo funciona el código

[English](README.en.md)

Usa este índice cuando quieras saber qué hace un archivo, por qué se separó y qué comportamiento debe conservar una contribución. Cada capítulo enlaza el código, presenta el contrato y señala pruebas concretas. El [mapa de fuentes](../SOURCE_MAP.md) sirve para localizar rápidamente un nombre de archivo.

| Capítulo | Dónde empezar |
| --- | --- |
| [Modelo, serialización y persistencia](PROJECT.md) | JSON, validación del árbol, movimientos, guardado, historial y recuperación. |
| [Arranque, interfaz y lienzo](EDITOR.md) | Bucle gráfico, estado del editor, selección y renderizado de preview. |
| [Widgets y recursos](WIDGET_RENDER.md) | Caché de texturas, gráficos, campos, biblioteca y dibujo compartido. |
| [Localización](LOCALIZATION.md) | Claves estables, fuentes y catálogos de traducción. |
| [Motor de compilación](BUILD.md) | Perfiles, procesos, intentos de exportación y diagnóstico. |
| [Asistente y extensiones](SERVICES.md) | Integración con proveedores, secretos, catálogo y propuestas. |
| [Generadores y bundles](EXPORTERS.md) | Cómo se convierte el diseño en archivos de código y recursos. |
| [Helpers emitidos](EMITTED_CONTROLS.md) | Controles auxiliares en los lenguajes de destino. |
| [Hosts y plantillas](RUNTIMES.md) | Ventanas, entrada, texturas y ciclo de vida en Vulkan, Android y Apple. |
| [Compilación y empaquetado](DISTRIBUTION.md) | CMake, CI, CPack y comprobación de artefactos. |
| [Pruebas y fixtures](TESTS.md) | Qué comprueba cada prueba y por qué existe. |

Para enviar el primer cambio, sigue la [guía de contribución](../../CONTRIBUTING.md). El [ejemplo de progreso circular](../EXTENDING.md#ejemplo-para-orientarte-en-el-código) sigue una propiedad desde la serialización hasta el código emitido.
