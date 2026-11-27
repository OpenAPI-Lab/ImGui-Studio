# Barras de título y acciones

[English](TITLE_BAR.en.md)

## Diseñar la barra

Abre [Barra de título](../examples/barra-de-titulo.json) y selecciona **Panel de actividad**. En el inspector, **Barra de título** controla la visibilidad del encabezado y los botones de plegar, minimizar, maximizar y cerrar. Esos botones afectan la ventana del diseño en vista previa. Usa **Reiniciar vista previa** para recuperar la geometría y reabrir ventanas cerradas. En **Apariencia de la ventana** ajusta altura, márgenes, icono, colores y separador. Arrastra uno de los extremos de la barra para editar su ancho.

| Qué quieres colocar | Cómo configurarlo |
| --- | --- |
| Menú dentro del encabezado | Selecciona el nodo de menú hijo y elige **Borde de Menú → Barra de título**. |
| Botón dedicado | Inserta **Botón de título** como hijo directo. Elige lado o posición libre e icono. Arrastra horizontalmente para colocar el botón. |
| Texto, casilla o entrada de una línea | Selecciona el hijo compatible y activa **En la barra de título**. |
| Barra sin controles visibles | Desactiva los que sobran en el inspector. **Sin título** oculta el encabezado y los controles integrados. |

## Conectar el clic

El botón **Ayuda** del ejemplo exporta un pulso de un fotograma en `state.chrome_help`. La ventana de ayuda necesita estado propio del host porque ese pulso se restablece en el siguiente fotograma. En el C++ exportado conserva un booleano fuera del bucle y procesa el pulso inmediatamente después de `RenderStudioUI(state)`. El [ejemplo de integración](../examples/title-bar-action/README.md) incluye el encabezado listo para copiar, el punto de llamada y comandos.

Consulta `codegen_report.txt` al cambiar de lenguaje. C# y Java dibujan controles embebidos como casillas y texto de entrada con el cuerpo abierto. Los botones dedicados y los controles de ventana siguen disponibles cuando se pliega. Los menús requieren que el host conecte sus propias acciones.

## Verificación y aporte

`GeneratedCpp_window_chrome` comprueba el estado exportado y `GeneratedCppWindowChromeSmoke` abre el host OpenGL. Si cambias colocación o estilo, prueba ambos y comprueba el ejemplo en **Vista previa** con el cuerpo abierto y plegado. Adjunta el tamaño de barra y el destino de exportación al reportar una diferencia.

[Render nativo](../src/editor/studio_preview.cpp) · [Generación de controles del título](../include/imguistudio/export/detail/studio_codegen_titlebar.hpp)
