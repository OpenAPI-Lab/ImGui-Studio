# Biblioteca de widgets y catálogo

[English](WIDGET_LIBRARY.en.md)


## Definiciones propias

1. Abre **Widgets → Mi biblioteca → Crear widget**. Escribe el nombre y elige **Lienzo en blanco** o un control como **Punto de partida**. También puedes partir del control seleccionado cuando el editor lo permite.
2. Pulsa **Abrir el diseñador**. Añade controles y prueba el resultado en vista previa. Guarda en **Mi biblioteca** para volver al proyecto.
3. Selecciona un contenedor del proyecto y, en **Widgets propios**, selecciona tu definición y pulsa **Insertar**. Para compartirla, usa **Exportar widget** y elige una ruta nueva.

En **Widgets → Mi biblioteca** crea una definición o abre **Añadir desde archivo**. Importa [Tarjeta de proceso](../examples/tarjeta-de-proceso.widget.json), insértala dos veces y cambia una instancia desde el inspector. Los identificadores y propiedades de cada copia se guardan en el proyecto. Una edición de la definición afecta las próximas inserciones.

El importador valida la definición antes de incorporarla a la biblioteca. Los límites del código son 256 entradas, 120 bytes para el nombre, 1024 nodos por widget y 16 MiB por archivo de definición. Los recursos de imagen embebidos acompañan al árbol. Consulta [el contrato de la biblioteca](../include/imguistudio/widgets/studio_widget_library.hpp) al modificar importación o persistencia.

## Perilla del catálogo

Abre **Widgets → Mi biblioteca → Catálogo** y usa **Instalar** en la ficha de imgui-knobs. Cuando concluya, inserta **Perilla** desde **Extensiones instaladas**. Configura su rango y valor inicial en el inspector y pruébala en **Vista previa**. [Catálogo de widgets](WIDGETS.md)

**Catálogo** instala la revisión fijada de imgui-knobs. El cliente obtiene cabecera, implementación y licencia y compara cada archivo con las fuentes revisadas. Solo escribe `extensions/imgui-knobs.json` después de validar la descarga y adquirir el lock de la sesión. Esta confirmación deja disponible la perilla integrada en el editor. C++ incorpora las fuentes y el aviso MIT en la exportación. Los destinos sin render de perilla emiten un marcador y un aviso.

Al retirar la extensión deja de aparecer en la paleta. Un nodo que ya está en un proyecto conserva sus datos. Instala la extensión otra vez para recuperar su vista previa.

## Aportar una extensión

Documenta el origen, revisión, licencia, widget que habilita y destinos afectados. El adaptador nativo, la validación de archivos y el soporte de exportación se revisan en conjunto. Incluye un proyecto pequeño y los avisos que se esperan en destinos parciales. Usa `NativeExtensionCatalogTest` para comprobar instalación, rechazo de contenido alterado y exportación C++.

[Implementación del catálogo](../src/services/studio_extensions.cpp) · [Revisión de imgui-knobs](../third_party/imgui-knobs/SOURCE.md)
