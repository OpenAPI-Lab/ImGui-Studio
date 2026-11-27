# Primer proyecto y flujo del editor

[English](WORKFLOW.en.md)


1. Abre el editor y elige **Nuevo proyecto**. Para recorrer un diseño ya guardado, abre [Barra y lienzo libre](../examples/barra-y-lienzo-libre.json).
2. Inserta una ventana y controles desde **Widgets**. En **Jerarquía** puedes elegir el contenedor que los recibe y reorganizar el árbol.
3. Selecciona un nodo y usa el **Inspector** para ajustar las propiedades. Las opciones visibles dependen del tipo. En las ventanas puedes configurar título, menú, controles de ventana y apariencia.
4. Usa **Vista previa** para probar clics y valores. El editor guarda valores de diseño y la vista previa mantiene por separado los valores temporales de interacción.
5. Guarda el proyecto como JSON. En **Código generado** elige un lenguaje y exporta hacia una carpeta nueva. Abre el README del resultado para compilarlo.
6. Para automatizar ese paso durante el diseño, abre **Run / Build**, elige el destino y configura sus herramientas. Compila y prueba el arranque. Cuando cambias el diseño, vuelve a compilar antes de ejecutar.

## Inicio, guardado e historial

| Acción | Qué hace y cuándo usarla |
| --- | --- |
| **Nuevo proyecto** | Abre un diseño nuevo. Si el lienzo está vacío, usa **Editar → Nueva ventana** para crear el contenedor raíz. |
| **Abrir proyecto** | Abre un diseño JSON. Introduce su ruta o usa **Examinar** en Windows. **Proyectos recientes** vuelve a abrir una ruta ya usada. |
| **Guardar proyecto** / **Guardar proyecto como…** | Escribe el JSON actual. Usa **Ctrl+S** para guardar o **Ctrl+Shift+S** para elegir otro archivo. Confirma el reemplazo de un archivo existente. |
| **Guardar versión** | Captura un snapshot en el historial. Para actualizar el archivo principal usa **Guardar proyecto**. |
| **Vista → Historial de versiones** | Muestra los snapshots, sus diferencias y opciones para restaurar o eliminar versiones. Al restaurar, guarda el diseño si quieres aplicar ese estado al JSON principal. |
| **Recuperar sesión anterior** | Cuando hay una copia recuperable, **Recuperar copia** restaura el trabajo como un proyecto sin ruta. Guarda la copia con **Guardar proyecto como…**. **Más tarde** conserva la copia y confirmar **Descartar…** la elimina. |

**Ctrl+O** abre el diálogo de proyecto. Usa **Ctrl+Z** para deshacer y **Ctrl+Y** o **Ctrl+Shift+Z** para rehacer. El [contrato de proyecto](PROJECT_FORMAT.md) explica qué se escribe en cada archivo.

## Qué hace cada panel

| Panel o control | Para qué sirve |
| --- | --- |
| **Widgets** | Busca el tipo de control en la paleta. Selecciona primero una ventana o un contenedor compatible y pulsa un widget para insertarlo. También puedes arrastrarlo directamente a un destino. |
| **Jerarquía** | Muestra el árbol completo, incluidos nodos que quedan fuera del área visible. Selecciona, reordena con **Subir/Bajar** o arrastra a otro contenedor compatible. |
| **Lienzo** | Diseña con las ventanas y controles nativos. El modo **IDE** usa el clic para seleccionar. **Vista previa** permite interactuar con los controles. |
| **Inspector de Propiedades** | Ajusta etiqueta, variable, ID ImGui, visibilidad, tamaño y posición cuando el tipo lo permite. También edita valores iniciales, opciones, colores y propiedades específicas. **Duplicar** copia el nodo y **Eliminar** lo retira. |
| **Código generado** | Muestra la salida del lenguaje seleccionado. **Copiar** copia lo que ves y **Exportar** genera un proyecto con los archivos necesarios para compilarlo. |
| **Run / Build** | Usa las herramientas del perfil para generar y probar una versión del diseño. **Probar arranque** ejecuta la comprobación del host generado cuando está disponible. |
| **Vista → Asistente IA** | Abre el selector de proveedores, modelo y conexión. Puedes revisar una propuesta antes de aplicarla como edición. [Configuración y datos que se envían](ASSISTANT.md) |
| **Vista → Idioma** | Cambia el idioma de la interfaz. La selección se guarda para el siguiente arranque. [Idiomas y paquetes comunitarios](LOCALIZATION.md) |

## Practicar con un diseño

Si quieres profundizar en un paso, consulta [Lienzo y disposición](CANVAS.md), [Barras de título](TITLE_BAR.md), [Guardado y recuperación](RECOVERY.md) o [Diagnósticos de compilación](BUILD_DIAGNOSTICS.md).

1. Crea una ventana y selecciónala en la jerarquía. Inserta un texto y un botón. En el inspector, cambia las etiquetas a **Estado del sistema** y **Actualizar**.
2. Con el botón seleccionado, arrástralo sobre el lienzo para cambiar su orden. Usa **Alt+arrastrar** para cambiarlo a posición libre. Arrastra el tirador de tamaño o ajusta **Ancho** y **Alto** en el inspector cuando estén disponibles. **Vista → Ajustar a cuadrícula** alinea los gestos a pasos de 8.
3. Usa **Ctrl+clic** para añadir o quitar elementos de la selección. Si los controles comparten contenedor, arrastra la selección para agrupar el movimiento. En el lienzo, mantén el botón central para desplazar la vista y usa **Ctrl+rueda** para el zoom. **Inicio** restablece la vista. **Esc** cancela el gesto de arrastre en curso.
4. Selecciona la ventana. En **Barra de título** elige mostrar el título, el plegado y los botones de ventana. Para un menú incrustado, agrega un menú como hijo de la ventana y selecciona **Borde de Menú → Barra de título**. Los tiradores laterales del título ajustan su ancho. Usa el ejemplo [Barra de título](../examples/barra-de-titulo.json) para ver una configuración completa.
5. Selecciona un control y revisa **Colores del widget**. Al habilitar un color propio se sobrescribe su color heredado. Selecciona una ventana para configurar su apariencia. El alcance de las propiedades depende del control y el exportador. [Referencia de controles](REFERENCE.md#controles-nativos-alcance-de-la-tercera-entrega)
6. Inserta una casilla, elige su **Valor inicial** y cambia a **Vista previa**. Pruébala con el ratón. **Reiniciar vista previa** recupera el estado inicial y **IDE** vuelve a la edición. Cambiar una casilla en la vista previa no cambia el valor inicial del diseño.
7. Guarda el proyecto como JSON. En **Código generado**, elige C++ y **Exportar CMake…**. Indica una carpeta nueva cuyo directorio padre exista. Lee el README generado y `codegen_report.txt` para ver los pasos de compilación y los límites del destino. Conecta la acción de **Actualizar** en la aplicación que hospeda el código. [Ejemplo de acción en C++](../examples/title-bar-action/README.md)

## Qué camino usar al terminar el diseño

| Quiero… | Qué opción usar |
| --- | --- |
| Conservar un diseño editable | **Archivo → Guardar proyecto**. |
| Probar los valores y la distribución | **Vista previa**. Trabaja dentro de ImGui Studio. |
| Integrar código en otra aplicación | **Código generado → Exportar**. Para C++, el modo `.hpp` muestra el encabezado y `Render()` la sección de render; la exportación completa añade el host. |
| Abrir una aplicación independiente | **Proyecto nativo → Ejecutar / Compilar…**. Selecciona el destino y el directorio de trabajo, busca las herramientas y pulsa **Build**. Usa **Run** al finalizar. Vuelve a compilar si cambias el diseño o la configuración usada. |
| Generar para Android, macOS o iOS | En **Código generado** selecciona C++ y usa el complemento del destino. [Destinos opcionales](EXPORTING.md#destinos-opcionales) |

## Qué ejemplo abrir

| Archivo | Qué probar |
| --- | --- |
| [barra-de-titulo.json](../examples/barra-de-titulo.json) | Menú, botones y personalización del título. |
| [barra-y-lienzo-libre.json](../examples/barra-y-lienzo-libre.json) | Posición, tamaño y jerarquía de controles. |
| [estado-y-progreso.json](../examples/estado-y-progreso.json) | Interruptor, indicador de carga y progreso circular. |
| [vulkan-imagenes.json](../examples/vulkan-imagenes.json) | Casilla y recursos PNG/JPEG en exportación C++. |
| [tarjeta-de-proceso.widget.json](../examples/tarjeta-de-proceso.widget.json) | Importación en **Widgets → Mi biblioteca → Añadir desde archivo**. |

## Biblioteca

**Crear widget** abre un borrador independiente. El lienzo en blanco admite guardar varios elementos agrupados. **Insertar** crea una copia con identificadores propios, por lo que editar la definición afecta las próximas inserciones. El **Catálogo** activa extensiones verificadas de la selección del editor. [Crear, importar e instalar widgets](WIDGET_LIBRARY.md)

## Si algo no aparece o no funciona

| Situación | Qué revisar |
| --- | --- |
| Un widget no entra o no se ve | Selecciona una ventana o contenedor compatible. Búscalo en **Jerarquía** y revisa su visibilidad, posición, tamaño y si el padre lo recorta. |
| Una casilla no cambia al pulsarla | Activa **Vista previa** para probar el clic. Para cambiar el estado con el que empieza, vuelve a **IDE** y edita su valor inicial. |
| Desaparecieron el menú o los botones del título | Comprueba que la ventana muestre el título y que los controles estén debajo de esa ventana en la jerarquía. |
| **Run** está deshabilitado | Revisa si **Build** terminó y si cambió el diseño, destino o configuración. Compila de nuevo con el perfil actual. [Diagnósticos de Run / Build](RUN_BUILD.md) |
| Un control aparece como marcador en el resultado | Abre `codegen_report.txt` y busca el ID del nodo. Compara la cobertura del lenguaje con la función que intentas usar. [Capacidades](CAPABILITIES.md) |
