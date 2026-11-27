# Catálogo de widgets

[English](WIDGETS.en.md)

Selecciona un contenedor y pulsa o arrastra un widget desde la paleta. El inspector muestra las propiedades del tipo elegido. Usa **Vista previa** para probar cambios de estado. Los identificadores de las tablas corresponden al modelo JSON y ayudan a interpretar los informes. Algunos tipos antiguos se pueden leer aunque tengan soporte parcial en una operación o un exportador.

El archivo generado `codegen_report.txt` indica la cobertura del destino. Consulta [los límites técnicos](REFERENCE.md#controles-nativos-alcance-de-la-tercera-entrega) o sigue [el primer proyecto](WORKFLOW.md) para practicar.

## Ventanas y contenedores

| Widget | Qué hace | Tipo JSON |
| --- | --- | --- |
| Ventana raíz | Define el área de trabajo de un diseño. Ajusta título, tamaño, posición y controles de ventana. | `window` |
| Panel hijo | Reserva una región dentro de otro contenedor. Úsalo para separar áreas con tamaño propio. | `child` |
| Grupo | Agrupa controles relacionados para moverlos o reutilizarlos juntos. | `group` |
| Barra de pestañas y pestaña | Crea una barra y añade sus pestañas. Pon el contenido dentro de cada pestaña. | `tab_bar`, `tab_item` |
| Sección plegable y nodo de árbol | Ocultan o muestran hijos. Usa el árbol para niveles jerárquicos y la sección plegable para bloques. | `collapsing_header`, `tree_node` |
| Tabla | Distribuye hijos en columnas. Configura la cantidad, los encabezados y las opciones que aparecen en el inspector. | `table` |
| Columna y fila | Ordenan hijos vertical u horizontalmente en el orden de la jerarquía. | `positioner_column`, `positioner_row` |
| Cuadrícula y flujo | La cuadrícula usa columnas y espaciado. Flow ajusta los hijos a otra fila cuando falta ancho. | `positioner_grid`, `positioner_flow` |
| Loader y componente | Loader muestra sus hijos según el estado de carga. Componente organiza los hijos bajo un nombre de plantilla. | `loader`, `component` |
| Repetidor | Dibuja instancias de una plantilla. Ajusta la cantidad y el espaciado. En diseño se edita la plantilla. | `repeater` |
| Animación | Aplica un efecto visual a los hijos. Configura duración, curva, bucle y si inicia activa. | `animation` |

## Texto, acciones y espacio

| Widget | Qué hace | Tipo JSON |
| --- | --- | --- |
| Texto, texto de color, ajustado y con viñeta | Muestran contenido. Elige color, ajuste de líneas o viñeta según lo que quieras presentar. | `text`, `text_colored`, `text_wrapped`, `bullet_text` |
| Ayuda contextual | Muestra texto de ayuda al colocar el puntero sobre el marcador. | `help_marker` |
| Botón, botón pequeño, flecha y muestra de color | Ofrecen controles de acción con distinta forma. Para ejecutar lógica, conecta el evento en la aplicación generada. | `button`, `small_button`, `arrow_button`, `color_button` |
| Botón de título | Colócalo como hijo directo de una ventana. El host puede leer su activación del fotograma. | `title_bar_button` |
| Barra de menú | Configura ubicación y elementos del menú en una ventana. Las acciones las conecta la aplicación. | `menu_bar` |
| Separador y separador con texto | Divide visualmente grupos de controles. La variante con texto rotula la sección. | `separator`, `separator_text` |
| Espaciado y espacio fijo | Añaden espacio en el lienzo. Usa tamaño propio cuando necesitas reservar una región concreta. | `spacing`, `dummy` |
| SameLine | Coloca el siguiente elemento en la misma fila. También puedes configurar la propiedad SameLine del control. | `same_line` |

## Entradas y selección

| Widget | Qué hace | Tipo JSON |
| --- | --- | --- |
| Casilla e interruptor | Representan un valor sí/no. Selecciona el valor inicial en el inspector. | `checkbox`, `toggle` |
| Radio y seleccionable | Los radios con la misma variable comparten selección. El seleccionable muestra una opción individual seleccionada o disponible. | `radio`, `selectable` |
| Entrada de texto de una o varias líneas | Define el texto inicial y, si corresponde, el texto de ayuda. Prueba la edición en vista previa. | `input_text`, `input_multiline` |
| Entrada entera y decimal | Permiten escribir números directamente. Usa un formato acorde con el tipo. | `input_int`, `input_float` |
| Deslizadores y ángulo | Acotan valores a un rango. Las variantes de vector exponen varios componentes y la de ángulo edita una orientación. | `slider_int`, `slider_float`, `slider_float2`, `slider_float3`, `slider_angle` |
| Arrastres numéricos | Permiten cambiar valores con el ratón. Ajusta velocidad, formato y límites según el control. | `drag_int`, `drag_float`, `drag_float3` |
| Editores de color RGB y RGBA | Editan un valor de color de la aplicación. RGBA añade el canal de transparencia. | `color_edit3`, `color_edit4` |
| Desplegable y lista de opciones | Seleccionan de una lista definida en el inspector. El desplegable ahorra espacio y la lista mantiene opciones visibles. | `combo`, `listbox` |
| Vistas de lista, cuadrícula y recorrido | Presentan un modelo de elementos. Configura el índice inicial y prueba la selección en vista previa. | `list_view`, `grid_view`, `path_view` |
| Perilla | Ajusta un valor con un control giratorio. Actívala desde el catálogo antes de insertarla. | `knob` |

## Imágenes, gráficos y estado

| Widget | Qué hace | Tipo JSON |
| --- | --- | --- |
| Imagen y botón de imagen | Importa un PNG o JPEG local en el inspector. El botón admite activación y la imagen simple muestra el recurso. | `image`, `image_button` |
| Líneas e histograma | Grafican series numéricas sencillas. Ajusta valores y escalas en el inspector. | `plot_lines`, `plot_histogram` |
| Gráfico 2D | Elige línea, área, barras, dona, dispersión o curva suave. Usa **Editar valores** para pegar una serie. | `chart_2d` |
| Gráfico 3D | Elige barras, dispersión o superficie. Ajusta elevación y azimut. Gira temporalmente con el ratón en vista previa. | `chart_3d` |
| Progreso lineal y circular | Usan una fracción de 0 a 1. Configura el texto superpuesto o central cuando lo necesites. | `progress_bar`, `circular_progress` |
| Indicador de carga | Representa trabajo en curso. Ajusta la velocidad, con cero para detener la animación. | `spinner` |
