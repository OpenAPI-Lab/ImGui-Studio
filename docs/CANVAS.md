# Lienzo, selección y disposición

[English](CANVAS.en.md)

## Elegir entre flujo y posición libre

Abre [Barra y lienzo libre](../examples/barra-y-lienzo-libre.json) en **Archivo → Abrir proyecto JSON…** y entra en modo **IDE**. En flujo, el orden en **Jerarquía** determina la secuencia. Una fila o columna controla la dirección y **Flow** vuelve a empezar una fila cuando falta ancho. En posición libre, cada control usa `x` e `y` respecto a su contenedor. Cambia a posición libre con **Alt+arrastrar** o activa **Posición explícita** en el inspector.

| Gesto | Resultado esperado |
| --- | --- |
| Arrastrar al centro de un contenedor compatible | Inserta el nodo como hijo. |
| Arrastrar al borde de otro nodo | Inserta antes o después en el mismo nivel del destino. |
| Ctrl+clic y arrastrar varios controles del mismo contenedor | Los mueve como bloque en su orden de hermanos. |
| Arrastrar desde una esquina de tamaño, si se muestra | Ajusta los ejes admitidos. Un campo de una línea cambia de ancho y un panel hijo admite ambos ejes. |
| Botón central y Ctrl+rueda | Desplazamiento y zoom de 50 % a 150 %. **Inicio** normaliza la vista. |
| Esc durante un arrastre | Cancela el cambio en el diseño. Si hubo desplazamiento automático, el encuadre puede quedar desplazado. |

El indicador anticipa la ubicación. El documento se actualiza al soltar el control y el gesto completo se puede deshacer. Si no se ve un widget, selecciónalo desde la jerarquía y comprueba si el padre lo recorta. En **Vista previa** prueba interacciones. Vuelve a **IDE** para cambiar los valores iniciales.

## Contrato del árbol

`planDocumentMove` recibe un hueco en la lista original de hermanos, de 0 a su tamaño. Si mueve elementos dentro de la misma lista, ajusta ese índice para compensar las extracciones. Verifica el contenedor, ancestros, profundidad y que la selección comparta padre. Una pestaña necesita una barra de pestañas. Un nodo no puede moverse dentro de su propio subárbol. El resultado conserva los IDs.

`positionDocumentNodes` rechaza coordenadas no finitas y, al confirmar, activa `useCursorPos` y desactiva `sameLine`. `resizeDocumentWidget` valida los ejes del tipo y ajusta el valor a pasos de ocho cuando la cuadrícula está activa. Un cambio inválido deja el proyecto de entrada intacto. Un cambio sin diferencias no crea un paso de deshacer.

## Verificación

`DocumentEditingTest` comprueba el modelo y `NativeCanvasEditingTest` añade eventos de ratón y teclado, selección, arrastre, cancelación y undo/redo. Para reproducir, compila ambos objetivos en una [configuración gráfica de pruebas](BUILD_AND_TEST.md#elegir-pruebas) y filtra CTest con `DocumentEditingTest|NativeCanvasEditingTest`. Para un problema de coordenadas, adjunta el proyecto, el zoom inicial, el widget, el destino y una captura del indicador.

[Reglas del documento](../include/imguistudio/project/studio_document_edit.hpp) · [Gestos del lienzo](../src/editor/studio_canvas.cpp)
