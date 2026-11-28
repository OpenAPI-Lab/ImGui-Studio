# Referencia técnica de ImGui Studio

[English](REFERENCE.en.md)

Implementación nativa del estudio en **C++17** usando **Dear ImGui**, **GLFW** y **OpenGL 3**.

El código de ImGui Studio se distribuye bajo la [licencia MIT](../LICENSE).

## Estructura
- `CMakeLists.txt`: Descarga `glfw` (3.4) e `imgui` (v1.91.5) mediante archivos con SHA-256 fijado; permite compilar el núcleo sin dependencias gráficas.
- `include/imguistudio/project/studio_types.hpp`: Modelo de proyecto, widgets, estilos ImGui, contenedores y helpers de árbol.
- `include/imguistudio/project/studio_snapshots.hpp`: Historial de versiones con respaldo automático no destructivo y comparación estructural.
- `include/imguistudio/export/studio_codegen.hpp`: Generador de código C++17 (`.hpp`, `RenderStudioUI()` y `.cpp` completo).
- `include/imguistudio/export/studio_codegen_native.hpp`: Backend C++ con estado, IDs, diagnósticos y rangos por nodo.
- `include/imguistudio/export/studio_cpp_export.hpp`: Exportación de un proyecto CMake independiente a una carpeta nueva.
- `include/imguistudio/widgets/studio_numeric_format.hpp`: Validación de formatos compartida por preview y generador.
- `tests/verify_codegen.cpp`: Genera casos que se compilan y ejecutan con Dear ImGui real.
- `include/imguistudio/build/studio_process.hpp`: Procesos nativos sin shell, pipes y cancelación.
- `include/imguistudio/build/studio_build.hpp`: Fases CMake, logs, artefactos y comprobación del diseño.
- `tests/verify_process.cpp` / `tests/verify_build.cpp`: Procesos y Run/Build reales.
- `include/imguistudio/build/studio_build_profiles.hpp` / `tests/verify_build_profiles.cpp`: Perfiles versionados, conflictos e incorporación al proyecto.
- `include/imguistudio/build/studio_build_diagnostics.hpp` / `tests/verify_build_diagnostics.cpp`: Diagnósticos incrementales y relación con nodos del diseño.
- `src/editor/studio_ui.hpp` / `src/editor/studio_ui.cpp`: Interfaz IDE nativa en Dear ImGui (paleta drag-and-drop, jerarquía, lienzo interactivo, modo vista previa real, inspector, visor de código e historial).
- `src/app/main.cpp`: Punto de entrada nativo GLFW + OpenGL3.
- `tests/verify_core.cpp`: Pruebas unitarias en C++17 puro sin dependencias gráficas.
- `include/imguistudio/project/studio_serialization.hpp`: Importación y exportación de proyectos JSON, validación y conservación de campos desconocidos.
- `include/imguistudio/project/studio_project_io.hpp`: Lectura limitada y guardado mediante archivo temporal, flush y reemplazo nativo.
- `tests/verify_persistence.cpp` / `tests/fixtures`: Pruebas con cuatro plantillas y los 60 tipos del catálogo histórico.
- `include/imguistudio/project/studio_session_store.hpp`: Historial persistente, leases nativas, detección de conflictos y recuperación de sesiones.
- `tests/verify_sessions.cpp` / `tests/verify_session_ui.cpp`: Pruebas de interrupciones, concurrencia, corrupción y render del diálogo de recuperación.
- `src/editor/studio_preview.hpp` / `src/editor/studio_preview.cpp`: Preview nativo, estado transitorio, observaciones de geometría e inspector por tipo.
- `tests/verify_preview.cpp`: Pruebas de render, clics, entrada de texto, selección e independencia del documento.
- `include/imguistudio/project/studio_document_edit.hpp`: Operaciones atómicas de insertar, ordenar, reparentar y posicionar, con validación de jerarquía.
- `src/editor/studio_canvas.hpp` / `src/editor/studio_canvas.cpp`: Selección por geometría visible, resaltado, gestos y destinos de arrastre.
- `tests/verify_document_edit.cpp` / `tests/verify_canvas.cpp`: Conservación de datos y pruebas reales de gestos del editor, undo/redo y cancelación.
- `third_party/nlohmann`: JSON for Modern C++ 3.11.3, MIT. Consulta [hash y procedencia](../third_party/README.md).

## Cobertura del editor

La cobertura de edición, vista previa y exportación varía por widget.
Consulta el [historial de cambios](../CHANGELOG.md) para ver los resultados
de validación por plataforma.

## Archivos de proyecto

Al abrir ImGui Studio, la pantalla de inicio ofrece **Nuevo proyecto**,
**Abrir proyecto** y los últimos proyectos abiertos o guardados. Para abrir
un diseño, navega a su carpeta y selecciona el JSON. En Windows se usa el
selector del sistema cuando está disponible.

Si quedan cambios recuperables de una sesión interrumpida, el aviso
aparece sobre el inicio. Guardar retira la copia temporal de la sesión.
Confirmar **Descartar cambios** o **Descartar** en el aviso elimina la
copia correspondiente. **Más tarde** la conserva para otro arranque.

### Mi biblioteca

Abre **Mi biblioteca** desde el panel **Widgets**. Elige **Crear widget**
y ponle un nombre. Puedes empezar con **Lienzo en blanco**, con un
control incluido o con una copia del widget seleccionado en el proyecto.
El diseñador abre un borrador independiente con paleta, jerarquía,
inspector y vista previa. Si empiezas en blanco, el editor guarda varios
controles como un grupo. **Guardar en Mi biblioteca** conserva el widget
y vuelve al proyecto.

Usa **Insertar** desde la biblioteca o arrastra el widget desde
**Widgets propios**, debajo de los incluidos. Cada instancia tiene
identificadores y estado independientes. Los cambios en la definición de
la biblioteca se aplican a futuras inserciones.

**Añadir desde archivo** lee un JSON de widget de ImGui Studio.
Prueba [Tarjeta de proceso](../examples/tarjeta-de-proceso.widget.json)
o utiliza **Exportar widget** para compartir uno propio. El archivo
incluye el árbol de controles y sus recursos de imagen incrustados.
Cada inserción queda en el JSON del proyecto y usa los exportadores
existentes, según la compatibilidad de los controles que contiene.

### Extensiones del catálogo

En **Widgets → Mi biblioteca → Catálogo** puedes instalar o retirar
[imgui-knobs](https://github.com/altschuler/imgui-knobs/tree/6b4b5923129c1e8fa4cd30f8c59439643d44f22f),
una perilla MIT incluida en la selección de
[extensiones de Dear ImGui](https://github.com/ocornut/imgui/wiki/Useful-Extensions#knobs).
Al instalar, el editor descarga de GitHub la cabecera, la implementación
y la licencia de la revisión indicada y las verifica byte por byte.
La copia verificada queda en `extensions/imgui-knobs.json` dentro del
directorio de estado. La vista previa usa el adaptador revisado que ya
incluye el editor. No se compila ni carga código descargado en ejecución.

Tras instalar, **Perilla** aparece en **Extensiones instaladas**. El
inspector permite configurar valor inicial, rango, velocidad y formato.
Retirar la extensión oculta su entrada de la paleta. Los proyectos
conservan las perillas y puedes reactivar su vista previa al reinstalar.

La vista previa y el proyecto generado en **C++ con GLFW/OpenGL** se
verificaron en Windows y Ubuntu bajo WSL. El proyecto C++ incluye las
fuentes de imgui-knobs y su licencia MIT, por lo que puede construirse
sin el editor. C11, Python, C#, Java y Rust generan un marcador y un
aviso para la perilla. macOS queda sin probar.

### Widgets de estado y progreso

En la paleta **Estado y progreso** están **Interruptor**,
**Indicador de carga** y **Progreso circular**. Configura el estado
inicial del interruptor, las vueltas por segundo del indicador o la
fracción y el texto central del progreso en el inspector. Una velocidad
de cero detiene el indicador. El progreso sin texto central muestra el
porcentaje. Abre [Estado y progreso](../examples/estado-y-progreso.json)
para probar los tres juntos.

La vista previa y la exportación **C++** usan el mismo código de dibujo.
El proyecto CMake incluye `studio_status_widgets.hpp`. El estado del
programa expone el valor booleano, la velocidad y la fracción para
actualizarlos durante la ejecución. C11, Python, C#,
Java y Rust generan un marcador y un aviso para estos controles.

El editor de nodos queda como candidato a complemento. **imnodes** ofrece
la interfaz de nodos, pines y enlaces bajo MIT. Una integración de Studio
necesitaría guardar el grafo en el proyecto y validar la revisión de
Dear ImGui en Windows y Linux. [Proyecto imnodes](https://github.com/Nelarius/imnodes)

### Idioma de la interfaz

Elige **Idioma** en la pantalla de inicio o en **Vista → Idioma**.
Español, inglés, francés, coreano, japonés y chino simplificado están incluidos.
El cambio se aplica al instante y se recuerda al volver a abrir el editor.
Los catálogos cubren el inicio, los menús, la recuperación, la paleta, el
inspector, el lienzo, el visor de código y los controles de compilación.
Consulta [la cobertura y los textos pendientes](LOCALIZATION.md).

Para añadir un idioma, crea un archivo UTF-8 como `locales/en-gb.json`
dentro del directorio de estado de ImGui Studio y vuelve a abrir el editor.
El nombre del archivo identifica el idioma. Usa letras minúsculas, números
y guiones. Las claves son las de
[`studio_locale_strings.inc`](../include/imguistudio/localization/studio_locale_strings.inc).
Las traducciones ausentes o inválidas toman el valor en inglés.

```json
{
  "name": "English (UK)",
  "strings": {
    "WelcomeNew": "Create a project",
    "Recents": "Recently opened projects"
  }
}
```

El directorio de estado es `%LOCALAPPDATA%\ImGuiStudio` en Windows,
`$XDG_STATE_HOME/ImGuiStudio` o `~/.local/state/ImGuiStudio` en Linux y
`~/Library/Application Support/ImGuiStudio` en macOS. Si defines
`IMGUI_STUDIO_STATE_DIR`, se usa esa ruta. La selección se guarda en
`language.json`. El editor utiliza fuentes instaladas para latín extendido,
cirílico y CJK. En Linux puede usar DejaVu Sans y Noto Sans CJK. Bajo WSL
también busca las fuentes de Windows. Se pueden indicar fuentes TrueType
locales en el subdirectorio `fonts` del estado con los nombres
`latin.ttf`, `korean.ttf`, `japanese.ttf` y `chinese.ttf`.

- Archivo > Abrir proyecto JSON (`Ctrl+O`): navegar y seleccionar un archivo.
- Guardar proyecto (`Ctrl+S`) y Guardar como (`Ctrl+Shift+S`): guardado real en disco.
- Guardar como requiere confirmar el reemplazo de otro archivo.
- Nuevo/Abrir/Cerrar detectan cambios sin guardar y permiten guardar, descartar o cancelar.
- Guardar Snapshot persiste una versión en el historial auxiliar del proyecto, o en
  la recuperación si todavía no tiene ruta. No equivale a guardar el archivo principal.
- Los controles interactivos de preview usan una copia separada del diseño.
- Las ediciones del inspector y la geometría pasan por el historial; los cambios de
  un arrastre o una sesión de escritura se agrupan en un paso de undo.

Los proyectos de formatos anteriores conservan campos opcionales, propiedades
todavía no interpretadas, colores, precisión numérica y datos de extensiones. También
se conservan los tipos desconocidos, con un aviso. Copias, undo y snapshots incluyen
esos datos. La comparación de JSON es semántica: no conserva espacios ni orden de claves.

El formato legado no tiene número de versión obligatorio; esta entrega no añade uno
ni reemplaza silenciosamente el formato. Se valida estructura, tipos de campos
interpretados, IDs únicos y jerarquía de tipos conocidos. Límite: 32 MiB, 100.000 nodos
y profundidad de widgets 128. Un archivo rechazado no reemplaza el proyecto abierto.

El widget histórico `same_line` y la propiedad `sameLine` son representaciones
compatibles de la colocación en una misma línea. Sus datos se conservan al guardar.

El guardado escribe un temporal exclusivo en el mismo directorio y lo reemplaza
solamente después de completar y vaciar la escritura. Las rutas Windows usan UTF-16
y los cuadros de ruta usan UTF-8. No se exporta configuración de proveedores AI.

## Historial persistente y recuperación

Un proyecto `interfaz.json` tiene su historial en
`interfaz.json.imguistudio/history.json`. Al abrir el proyecto se cargan sus snapshots;
las versiones manuales y los cambios del historial se escriben inmediatamente.
Se mantiene la política de 35 snapshots, retirando primero los automáticos más
antiguos. No se añade historial ni información de sesión al JSON del proyecto.

Los checkpoints de recuperación incluyen el diseño actual, snapshots y pilas de
undo/redo (hasta 40 estados por pila). Se escriben al haber cambios, con un intervalo
de dos segundos entre intentos, y al guardar/cambiar/cerrar la sesión. Los snapshots
automáticos esperan 15 segundos de inactividad. Un corte antes
del próximo checkpoint todavía puede perder los cambios más recientes.

La recuperación se almacena en `recovery/` bajo:

- Windows: `%LOCALAPPDATA%/ImGuiStudio`.
- Linux: `$XDG_STATE_HOME/ImGuiStudio`, o `$HOME/.local/state/ImGuiStudio`.
- macOS: `$HOME/Library/Application Support/ImGuiStudio`.
- `IMGUI_STUDIO_STATE_DIR` permite elegir otra ubicación, también para pruebas.

Al iniciar se ofrecen las sesiones interrumpidas. **Recuperar copia** restaura el
documento, snapshots y undo/redo como proyecto sin ruta: se usa Guardar como para
elegir el destino. El archivo original no se escribe durante la recuperación. La
copia antigua se retira solamente después de escribir una nueva copia durable.
**Más tarde** conserva los registros; Archivo > Recuperar sesiones anteriores
permite volver a buscarlos cuando el proyecto actual no tiene cambios pendientes.
Descartar una copia requiere una acción explícita de confirmación.

Los locks pertenecen a handles del sistema operativo y se liberan al terminar el
proceso. Los archivos `.lock` se conservan para evitar carreras entre procesos; su
existencia no indica una sesión activa. Los registros de editores activos no se
ofrecen como sesiones interrumpidas.

El guardado del proyecto comprueba si su contenido cambió desde que se abrió o
guardó; en ese caso solicita Guardar como. El historial también verifica su revisión
bajo lock. Los metadatos dañados, de versiones desconocidas o cambiados por otra
instancia se conservan sin sobrescribirlos y el editor muestra el error. Si Guardar
como apunta a un destino con historial existente, ese historial se protege y el
historial de la sesión actual permanece en recuperación; se debe elegir otro destino
para trasladarlo. Un problema al guardar historial no invalida un proyecto ya guardado.

Formato auxiliar: `schemaVersion: 1`; máximo de 64 MiB por archivo. Un exceso produce
un error, no un recorte silencioso. La búsqueda inspecciona hasta 200 registros e
informa si alcanza ese límite. Si se mueve un proyecto con sus metadatos a otra ruta,
el historial requiere reasociación; no se reutiliza silenciosamente una identidad
distinta. El undo/redo normal sigue siendo de sesión; se persiste para recuperar una
sesión interrumpida, no para reabrir arbitrariamente una sesión guardada y cerrada.

## Pruebas sin interfaz gráfica (desde la raíz del repositorio)

```sh
cmake -S . -B build/core -DBUILD_STUDIO_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build/core --config Debug
ctest --test-dir build/core -C Debug --output-on-failure
```

Este modo no descarga GLFW ni ImGui y funciona sin GPU o pantalla. Las aserciones de
las pruebas permanecen activas también en Release. El único header JSON necesario
está incluido en el repositorio.

Las fixtures de `tests/fixtures/` se incluyen en el repositorio.
Su `manifest.json` registra el origen de los casos de compatibilidad.

## Controles nativos: alcance de la tercera entrega

El renderer tiene 47 rutas de controles/contenedores verificadas en un contexto
Dear ImGui. Las ventanas raíz las sigue dibujando el canvas. Esto no equivale a
paridad completa con los 60 tipos y todas las propiedades del catálogo histórico.

- Botones normales, pequeños, flecha y color; seleccionables, checkbox y radios.
- Texto normal, coloreado, ajustado, con viñeta y ayuda contextual; separadores y dummy.
- Entrada de texto normal/multilínea con `std::string`, sin el antiguo buffer de
  256 bytes. Se usa el adaptador oficial `misc/cpp/imgui_stdlib` de la misma versión
  de Dear ImGui, sin una nueva biblioteca externa.
- Entradas numéricas int/float; sliders int/float/float2/float3/ángulo; drags
  int/float/float3; editores de color RGB/RGBA, combos y listbox.
- Progreso y plots básicos; child, grupos, árbol, secciones plegables, pestañas,
  columnas/filas, grid y tablas con títulos de columnas.
- Menús en la barra de título o en el borde superior de la ventana.
  El resto de los bordes se señala como pendiente. Los elementos de menú
  todavía requieren acciones definidas por la aplicación.

Los radios comparten selección por `varName`; el primer radio del grupo determina
el valor inicial. Los otros valores interactivos se almacenan por ID de nodo en el
renderer, no en el documento ni en su JSON. **Reiniciar preview** restaura valores,
selección inicial de pestañas, secciones plegables y geometría. Salir de preview
también restaura la geometría del diseño.

El inspector incorpora las propiedades correspondientes a esos controles: valores
iniciales, rangos, formatos numéricos, paso/velocidad, vectores, colores, elementos
de listas, selección, progreso, columnas y espaciado. Posición explícita, sangría,
SameLine e ID ImGui permanecen serializados. Los controles de texto del inspector
tampoco usan buffers de tamaño fijo. Mostrar el inspector sin editar no normaliza
los datos importados. Las ediciones siguen pasando por undo y recuperación.

Los campos recién interpretados conservan sus valores originales mientras no se
editen; también se conservan extensiones y componentes adicionales de un vector.
Los formatos numéricos incompatibles utilizan un formato de preview predeterminado,
con aviso, conservando el original en el documento. No se pasan especificadores
de texto, punteros o múltiples conversiones a controles numéricos.

El tema se aplica a las ventanas del diseño y se restaura antes de dibujar el editor.
Cada ventana puede heredar sus valores o usar ajustes propios de color,
opacidad, borde, redondeo y relleno. El inspector incluye los perfiles
**ImGui**, **Bionic**, **macOS**, **Minimal** y **Cyber**.
Las sombras y los iconos se dibujan con primitivas de Dear ImGui.

Límites explícitos:

- El canvas usa los controles nativos también en diseño, con selección/arrastre de
  widgets anidados. Las limitaciones de los tipos no soportados siguen vigentes.
- **Imagen** y **Botón de imagen** están en la paleta de controles. En el
  inspector, introduce la ruta a un PNG o JPEG local y pulsa **Importar imagen**.
  Se guarda dentro del JSON y se conserva al duplicar el control o deshacer
  una edición. El lienzo y la vista previa conservan la proporción al ajustar
  la imagen al tamaño del control. El botón responde al clic en la vista previa.
  La importación admite hasta 2 MiB por archivo y 4096 × 4096 píxeles.
  Los seis lenguajes exportan PNG y JPEG. C++, C11 y Python los incrustan
  en el código, C# y Rust los incorporan al compilar y Java los carga
  como recursos del classpath. Las aplicaciones generadas cargan y
  liberan las texturas automáticamente. El estado de cada botón de
  imagen indica si se activó en el frame actual. El README de cada
  exportación explica cómo conectar otro backend y conservar sus recursos.
- **Botón de título** está en la paleta de controles y se añade como hijo
  directo de una ventana. El inspector permite elegir lado izquierdo, centro,
  lado derecho o posición libre, además del icono, estilo y redondeo.
  Arrastrar el botón horizontalmente en el lienzo lo deja en posición libre
  y se puede deshacer. En la vista previa, el clic registra la activación
  del botón. Los botones se ocultan cuando la ventana usa **Sin título**.
  C++, C11, Python, C#, Java y Rust exportan la posición, el aspecto y el
  estado de clic por frame. Java admite clic con ratón en estos botones.
  Rust incluye un puente C++ compilado por Cargo.
- El inspector de una ventana permite mostrar el título, alinearlo y
  configurar el plegado y los botones de minimizar, maximizar y cerrar.
  En la vista previa se puede restaurar el tamaño y **Reiniciar preview**
  vuelve a abrir las ventanas cerradas. El diseño conserva su geometría.
  Una ventana sin la propiedad **Cerrar** explícita conserva el
  comportamiento anterior del editor nativo.
- **Apariencia de la ventana** permite cambiar altura, márgenes, icono,
  transparencia y separador de la barra. En ventanas con barra de menú
  superior, Dear ImGui utiliza el mismo relleno vertical para ambos
  encabezados. El cuerpo conserva el tamaño normal de sus controles.
  **Mostrar tirador inferior** recupera el indicador de redimensionado
  de Dear ImGui. La esquina sigue funcionando con el indicador oculto.
- **En la barra de título** está disponible para hijos directos de la
  ventana de tipo texto, texto de color, botón, botón pequeño, botón de
  flecha, casilla y entrada de una línea. Se pueden distribuir en la
  barra y arrastrar a posición libre. **Sin título** oculta estos
  controles. Consulta la [cobertura de la exportación](#generador-y-exportación-c-verificados)
  para ver los ajustes compatibles y sus límites.
- Para insertar un menú en la misma barra, selecciona un menú hijo de
  la ventana y elige **Borde de Menú → Barra de título**. Puedes alinearlo
  o arrastrarlo dentro de la barra. Selecciona la ventana en el lienzo
  y arrastra cualquiera de los dos tiradores laterales de su título
  para cambiar el ancho de la barra.
- **Animación** está en la paleta de contenedores. El inspector guarda el
  efecto, la duración, la curva, el bucle y el estado inicial. El lienzo
  mantiene los controles quietos durante la edición. En la vista previa,
  el contenedor aplica opacidad, rebote vertical, deslizamiento horizontal,
  escala o giro del matiz del tema. Los controles siguen siendo interactivos
  y el botón de reproducción solo cambia el estado de la sesión de preview.
  C++, C11, Python, C#, Java y Rust exportan los cinco efectos, las curvas
  y el control de reproducción. El estado generado conserva el indicador
  de reproducción y el inicio del ciclo por contenedor. La salida Rust
  mide el contenido para ajustar la altura de los paneles automáticos.
- **Loader**, **Componente** y **Repetidor** están en la paleta de contenedores.
  Loader carga y descarga sus propios controles en la vista previa. El
  inspector guarda el nombre de origen y el estado inicial. Componente
  muestra sus controles bajo el nombre de la plantilla. Repetidor mantiene
  una plantilla editable y muestra hasta tres muestras adicionales. El
  inspector guarda el número de instancias y el espaciado. C11, C++, Python,
  C#, Java y Rust exportan los tres contenedores. El Repetidor generado dibuja
  sus instancias con identificadores propios y valores compartidos por
  plantilla. La salida Rust ajusta la altura automática con la medida de
  cada ventana hija del fotograma anterior.
- La paleta **Vistas** incluye lista, cuadrícula y recorrido. El inspector
  edita los elementos del modelo, el índice inicial y el borde. La cuadrícula
  también permite cambiar el número de columnas y el espaciado. En la vista
  previa se puede seleccionar una fila o celda y recorrer los elementos con
  las flechas. El modelo vacío se muestra como **Sin elementos**. La selección
  durante la vista previa es temporal. El tirador de la esquina cambia el
  ancho y el alto de la vista en el lienzo. Los seis generadores conservan
  los elementos, la geometría y la selección inicial. Las aplicaciones
  exportadas mantienen su propia selección mientras están abiertas.
- **Flujo (Flow)** acomoda los hijos visibles en filas y los pasa a la fila
  siguiente cuando sobrepasan el ancho. La separación y el borde se ajustan
  en el inspector. Al arrastrar dentro de una fila, el lado izquierdo o derecho
  indica la posición antes o después. Los generadores de C++, C, Python,
  C#, Java y Rust reproducen el salto de fila. En Rust, el binding imgui-rs
  0.12 calcula la altura automática a partir del último cuadro dibujado.
- Los tamaños se aplican donde lo permite el control Dear ImGui: botones, áreas de
  texto/lista, child y tablas. El alto de checkbox, radio, sliders y otros controles
  estándar depende del texto, fuente y tema, no de una altura CSS arbitraria.
- Tablas/grid admiten de 1 a 511 columnas en preview, según la versión fijada de
  Dear ImGui. Un exceso muestra aviso sin recortar el documento.
- Los datos de texto UTF-8 se conservan, pero no se han incorporado fuentes con
  cobertura CJK ni se ha validado IME en todas las plataformas.
- **Gráfico 2D** ofrece línea, área, barras, dona, dispersión y curva suave.
  El inspector edita el título, el color, las etiquetas y los límites
  automáticos o manuales. En vista previa, el puntero muestra el valor
  bajo el cursor en los gráficos cartesianos. C11, C++, Python, C#, Java y
  Rust exportan los seis modos y guardan los valores de la serie en el
  estado generado para actualizarlos en ejecución.
- **Gráfico 3D** ofrece barras, dispersión y superficie. El inspector
  permite ajustar el color, el título, la elevación y el azimut. En
  diseño, **Shift + arrastrar** cambia la orientación en un paso de
  deshacer y **Esc** cancela el gesto. En vista previa, arrastra para
  girar temporalmente. Las barras usan una cuadrícula de 4 × 3 y la
  superficie toma el primer valor de la serie como fase. C11, C++, Python,
  C#, Java y Rust exportan los tres modos y permiten girar con el ratón.
  La serie y la orientación se guardan en el estado de la interfaz generada.
- En gráficos 2D, gráficos 3D, gráficos de líneas e histogramas, **Editar valores** abre la serie para
  escribir o pegar números separados por comas o espacios. Usa punto decimal.
  **Aplicar valores** actualiza la serie en un paso de Deshacer. El inspector
  también permite editar el texto superpuesto y, en gráficos 2D, de líneas
  e histogramas, los límites de escala.
- Los callbacks de los botones quedan fuera de la vista previa.
- Las pruebas de interacción cubren checkbox, radios, combo/lista, int/float, slider,
  texto largo UTF-8 y pestañas. Los demás controles se someten a render sin aserciones
  y conservación de datos; no se afirma haber automatizado todos sus gestos.

## Canvas de edición: cuarta entrega

La barra superior tiene accesos directos a **IDE** (modo diseño) y
**Vista previa**. **Widgets** queda arriba a la izquierda y **Jerarquía**
debajo, al lado de **Código generado**. El panel de código se puede
ocultar desde **Vista**.

El modo diseño usa los mismos controles Dear ImGui que el preview. Los valores de
checkbox, campos y selectores están bloqueados en diseño: un clic selecciona el
nodo y lo resalta. Pestañas, árboles y secciones plegables conservan navegación
para poder acceder a sus hijos. La selección prioriza el nodo anidado visible,
respetando clipping y ventanas superpuestas. El fondo vacío deselecciona.

El inspector **Colores del widget** muestra los colores aplicables al
control seleccionado, como el fondo de un campo, el texto seleccionado, las
marcas, las filas seleccionadas y el estado de los botones. Desactivar un
color propio restaura el color heredado. Los nuevos colores y la
visibilidad del tirador se guardan en el proyecto y se aplican en el editor
nativo. La compatibilidad con los generadores queda pendiente.

- Arrastrar un widget de flujo permite ordenar o cambiar de contenedor. En un
  contenedor, el centro significa **dentro** y sus bordes **antes/después**; una
  línea y un mensaje indican el destino. Las filas usan inserción horizontal en
  el canvas, y la jerarquía usa siempre orden vertical.
- **Alt+arrastrar** convierte el widget a posición libre. Un widget que ya tiene
  `useCursorPos` se mueve en posición libre directamente. La posición se calcula
  respecto a su ventana/child y se ajusta a pasos de 8 si la cuadrícula está activa.
- Los separadores, los espaciadores y el espacio fijo se pueden arrastrar
  directamente. Selecciona el widget y usa su tirador para ajustar su tamaño.
  El espaciador tiene un contorno en modo diseño para que puedas seleccionarlo.
  Prueba estos ajustes abriendo [Barra y lienzo libre](../examples/barra-y-lienzo-libre.json).
- **Ctrl+clic** agrega o quita widgets de la selección desde el canvas o la
  jerarquía. El inspector y los comandos de un solo nodo usan el último widget
  agregado. Un clic simple vuelve a seleccionar un solo nodo.
- Arrastra cualquier miembro de una selección que comparta contenedor para
  mover el bloque conservando el orden de los widgets. En el canvas, **Alt+arrastrar**
  mueve juntos los miembros visibles de un mismo contenedor en posición libre.
  Con la cuadrícula activa se alinea el widget desde el que se arrastra y los
  demás conservan su separación. Las ventanas raíz se reordenan desde la jerarquía.
- La paleta permite arrastrar controles a contenedores, o nuevas ventanas al canvas.
  Se mantiene también la inserción rápida por clic. Un control normal necesita un
  contenedor válido; no se coloca silenciosamente en la raíz del documento.
- En **Jerarquía**, se puede arrastrar entre filas/contenedores y usar Subir/Bajar.
  Esto permite ordenar ventanas raíz y trabajar con nodos que no están visibles
  en el canvas, sin eliminar su contenido.
- Durante un arrastre, acerca el cursor al borde superior o inferior de la lista
  de **Jerarquía** para recorrerla. Las pestañas y los botones quedan a la vista.
  Mantén el cursor sobre el centro de un contenedor cerrado que admita la selección
  durante algo más de medio segundo para desplegarlo y poder apuntar a sus hijos.
- En diseño, mantén pulsado el **botón central del ratón** y arrastra para
  desplazar el canvas. Pulsa **Esc** para cancelar el gesto. Usa **Ctrl + rueda**
  sobre el canvas para ajustar el zoom entre 50 % y 150 % alrededor del cursor.
  También puedes usar **Vista → Zoom del lienzo**. Pulsa **Inicio** sobre el
  canvas o usa **Restablecer vista del canvas** en **Vista** para volver al
  origen y al 100 %. El zoom y el desplazamiento quedan fuera del historial y
  del JSON. La vista previa abre al 100 % y conserva el encuadre de diseño
  al regresar.
- Al arrastrar un widget para reordenarlo, cambiarlo de contenedor o colocarlo
  en posición libre, acerca el cursor a un borde del canvas para desplazar la
  vista automáticamente. El desplazamiento se detiene al alejarte del borde,
  soltar o pulsar **Esc**. Si cancelas el arrastre, la vista queda en el punto
  alcanzado.
- Durante el gesto se muestra un marcador o rectángulo fantasma. El documento
  cambia únicamente al soltar y genera un solo paso de undo. Escape o pérdida de
  seguimiento del ratón cancelan el gesto. Si otro comando modifica el proyecto
  durante el arrastre, el gesto no reemplaza esa nueva versión.
- Se rechazan ciclos, destinos inexistentes y relaciones incompatibles, como una
  ventana dentro de otro contenedor o una pestaña fuera de una barra de pestañas.
  Las operaciones conservan IDs, propiedades, coordenadas y datos desconocidos;
  posición libre cambia explícitamente `x/y`, `useCursorPos` y `sameLine`.
- Las ventanas diseñadas mantienen movimiento por título y redimensionado nativo,
  con las restricciones del proyecto. En diseño, una ventana sin título se mueve
  desde el inspector para evitar confundir su fondo con un gesto sobre widgets.
- Selecciona un botón, botón de color, panel hijo, barra de progreso o espacio
  fijo y arrastra el tirador de la esquina inferior derecha. La guía muestra el
  tamaño hasta soltar. Con la cuadrícula activa, las dimensiones cambian en
  pasos de 8. El gesto conserva el layout y sólo modifica los ejes desplazados.
  Si la esquina está recortada, desplázate hasta verla o usa el inspector.
- Los campos de texto de una línea, campos numéricos, deslizadores, controles
  numéricos de arrastre y desplegables tienen un tirador horizontal al borde
  derecho del control. El ancho excluye la etiqueta y conserva la altura.
  Los campos con botones de incremento y los controles vectoriales respetan
  el espacio mínimo necesario para sus componentes.
- Los campos multilínea, las listas, los gráficos de líneas y los histogramas
  tienen un tirador en la esquina inferior derecha del control. La etiqueta
  queda fuera del cálculo. Arrastra en vertical para ajustar solo la altura
  y conservar el ancho automático si estaba activo.
- Las filas seleccionables, Flow, las tablas y el layout de cuadrícula
  también admiten ajuste de ancho y alto. Una cuadrícula vacía con altura
  automática tiene un área de edición en diseño. Los editores de color RGB
  y RGBA tienen un tirador horizontal al borde del control, antes de la
  etiqueta.

Las observaciones de geometría se registran por nodo, padre, ventana y zona visible;
no son datos persistentes ni se añaden al JSON. Los checkpoints de recuperación sólo
incluyen cambios ya aplicados, nunca el fantasma de un gesto pendiente.

Pendientes: QA de todos los contenedores y ventanas superpuestas en los
tres sistemas. Un widget totalmente recortado o sin
render nativo puede seleccionarse desde la jerarquía. El reparentado de tipos de
contenedor desconocidos se rechaza hasta conocer sus reglas, conservando sus datos.

## Compilación y smoke test desde la raíz

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native --config Debug
ctest --test-dir build/native -C Debug --output-on-failure
```

Con generadores de configuración única (Ninja/Make):

```sh
./build/native/bin/imgui_studio --smoke-test
```

En Windows con Ninja, el ejecutable es `build/native/bin/imgui_studio.exe`. Con
Visual Studio, un build Debug genera `build/native/bin/Debug/imgui_studio.exe`.
`--smoke-test` crea una ventana oculta,
inicializa OpenGL y dibuja tres frames sin guardar `imgui.ini`. No comprueba todos los
gestos ni sustituye las pruebas visuales de cada plataforma. El smoke test desactiva
la persistencia de sesiones, por lo que no escribe en el directorio de datos del usuario.

Validación realizada en Windows con GCC 16.2/CMake 4.4.2: ocho pruebas CTest del
núcleo y veinte en la compilación nativa, más smoke test correcto. También se probó
recuperación entre dos procesos, terminando el primero con `std::_Exit` para omitir
la limpieza de C++. La validación en Linux se describe en la sección de WSL.
En una verificación posterior se compiló y arrancó el editor con MSVC.
La ejecución en macOS sigue pendiente. La prueba UI dibuja
el diálogo en un contexto ImGui sin ventana; no automatiza todos los botones o gestos.
`NativeCanvasEditingTest` prueba el editor con eventos de ratón/teclado: selección
anidada, valores bloqueados, arrastre desde canvas/paleta/jerarquía, snap, un paso
undo por gesto, undo/redo, cancelación y conservación del documento. No sustituye
la QA manual de scroll, DPI y ventanas superpuestas de cada sistema.
Consulta la [matriz de capacidades](CAPABILITIES.md) para el estado de las
salidas por lenguaje, el asistente y los instaladores.

## Run / Build nativo

Abre «Proyecto nativo → Run / Build...» o «Vista → Run / Build» y elige
«C++», «C11», «C#», «Python», «Java» o «Rust» en «Destino». Configura el directorio, las herramientas
y configuración Debug/Release/RelWithDebInfo/MinSizeRel. «Detectar herramientas»
busca CMake y compiladores en el entorno; CMake comprueba el toolchain al configurar.
El compilador vacío permite la detección propia de CMake. Con MSVC puede requerirse
iniciar el editor desde un Developer Command Prompt. No se instalan herramientas.

Build exporta el documento actual a una carpeta nueva por intento. CMake configura
y compila `studio_export` para C++ o `studio_export_c` para C11. Para C#, el panel
usa el ejecutable dotnet del perfil, restaura NuGet en nuget-packages del directorio
de trabajo y compila StudioExport.csproj. Run abre StudioExport.dll con dotnet
sin restaurar ni compilar. El SDK elegido debe poder compilar net8.0.
Para Python, Build comprueba sintaxis, versión e importación de imgui-bundle.
Run usa el mismo CPython y comprueba que main.py coincida con la copia verificada.
Para Java, Build compila con javac y destino Java 17. Run usa las clases ya
compiladas y el mismo directorio local de JAR.
Para Rust, Build usa Cargo con la configuración Debug o Release. Run ejecuta
el binario resultante.
Run abre el ejecutable de ese proyecto; Probar arranque ejecuta tres frames ocultos.
Preview continúa dentro del editor y no equivale a Run. Si cambia el documento, Run
se desactiva hasta compilarlo de nuevo. Cambiar de destino también desactiva Run.
Se compila una copia del documento vivo, sin
necesidad de guardarlo ni alterar el JSON o el historial.

Las fuentes locales de GLFW, Dear ImGui y cimgui permiten reutilizar dependencias.
C11 tiene rutas separadas para cimgui y su revisión de Dear ImGui, diferentes a C++.
Las rutas son opcionales;
son rutas aportadas por el usuario y no se verifica su hash/versionado. Sin ellas,
el scaffold descarga las versiones fijadas de la entrega anterior. La configuración
requiere CMake >= 3.16, compilador C++17 y C, un generador disponible y OpenGL con las
dependencias de GLFW de cada SO. Las faltas/errores aparecen en el log de CMake.

Los subprocesos usan CreateProcessW y un Job Object en Windows; fork/execv y grupo
de procesos en POSIX. Los argumentos se pasan individualmente, sin shell. Stdin es
EOF, stdout/stderr se capturan juntos, y el editor consulta hasta 64 KiB por frame.
El panel conserva hasta 1 MiB de salida reciente y muestra el código de salida.
Detener cancela la operación; cerrar el editor también termina el proceso activo.
En Windows se cancela el job con descendientes. En POSIX se cancela el grupo;
programas que creen otra sesión/grupo pueden escapar de ese alcance.

Las pruebas de integración usan el mismo controlador del panel. Compilan proyectos
C++ y C11 con CMake/Ninja, ejecutan su prueba de arranque, comprueban la cancelación,
rechazan cambios de destino y verifican diagnósticos de compilación y configuración.
La prueba de C11 se activa con STUDIO_TEST_C_EXPORT. Las pruebas reutilizan fuentes descargadas
para evitar red. La prueba de procesos cubre argumentos, UTF-8, stderr, EOF, salida
de 2 MiB, códigos de error y cancelación/destrucción de árboles. La revisión manual
de todos los botones y la validación en MSVC y macOS quedan pendientes.

Límites: sin cola paralela ni progreso porcentual.
Cada intento conserva su exportación/build en disco para inspección; no hay limpieza
automática. Run requiere un Build previo del diseño actual.

## Perfiles Build persistentes y configuración del proyecto

El panel permite duplicar, renombrar, seleccionar y eliminar perfiles (hasta 32).
«Guardar perfiles» conserva todos los perfiles y el seleccionado; «Recargar perfiles»
descarta los cambios locales y lee el archivo de nuevo. Los cambios permanecen en
memoria hasta guardar. Una recarga fallida conserva los perfiles en memoria y bloquea
el guardado hasta cargar un archivo válido. Cambiar los ajustes o el directorio de
trabajo desactiva Run hasta compilar con esa configuración; renombrar un perfil no
invalida el ejecutable por sí solo. Al reiniciar se restauran los perfiles, no procesos
ni ejecutables anteriores: es necesario compilar antes de Run.

El archivo es `build_profiles.json` dentro del directorio de estado de ImGuiStudio
(LOCALAPPDATA en Windows, Application Support en macOS, XDG_STATE_HOME o
`~/.local/state` en Linux). `IMGUI_STUDIO_STATE_DIR` permite elegir otro directorio.
Abrir el editor no escribe este archivo. Smoke test y sesiones de test con persistencia
desactivada no cargan/guardan perfiles del usuario.

El formato nativo es versión 1: `schemaVersion`, `activeProfile` y `profiles`, cada
perfil con `id`, `name`, `workspace` y `settings`. Settings contiene `cmake`, `generator`,
`compiler`, `configuration`, `glfwSource` e `imguiSource`. `target` distingue
`cpp`, `c`, `csharp`, `python`, `java` y `rust`. Los perfiles anteriores usan C++ por defecto.
Java guarda `jdkHome` y `javaBinding`. Python guarda `python` y `pythonBinding`.
C# guarda `dotnet`. Rust guarda `cargo` y `rustOffline`. C11 también guarda
`cCompiler`, `cimguiSource` y `cImguiSource`. El espacio `buildConfig.nativeCpp`
se mantiene por compatibilidad. Se conservan propiedades
desconocidas del documento, perfil y settings. Se limita a 1 MiB, profundidad 64 y
8192 bytes por campo de texto conocido. Datos inválidos o versiones futuras no se
sobrescriben. Guardar usa archivo temporal/reemplazo, lease de SO y comparación con
los bytes leídos para detectar cambios de otra sesión; ante conflicto, recargar.

Los proyectos antiguos pueden omitir `buildConfig`. Cualquier
`buildConfig` desconocido que llegue en JSON se conserva.
«Incluir perfil en proyecto» añade explícitamente:

```json
{"buildConfig":{"nativeCpp":{"schemaVersion":1,"profile":{
  "id":"local","name":"C++ local","workspace":"ruta de trabajo",
  "settings":{"cmake":"cmake","configuration":"Debug"}
}}}}
```

La operación participa en undo/redo, marca el proyecto modificado y exige guardar
el JSON para persistir. Conserva los otros campos de buildConfig y campos desconocidos
de un namespace nativo válido; rechaza reemplazar un buildConfig no objeto o un
namespace de versión futura/corrupto. «Importar perfil del proyecto» copia el perfil
nativo a uno local con ID nuevo; no lo ejecuta ni guarda automáticamente y permite
revisar rutas antes de Build. No se interpretan ni ejecutan comandos arbitrarios
guardados en propiedades desconocidas. Incluir el perfil comparte las rutas locales
en el JSON; normalmente basta guardar perfiles locales sin adjuntarlos al proyecto.

La prueba de persistencia usa otro proceso para restaurar el perfil activo, verifica
conflictos entre sesiones, leases, archivos inválidos/futuros, límites y conservación
de campos opacos al adjuntar/importar. La integración CMake compila con settings
guardados y recargados, y bloquea Run al cambiar configuración o directorio.

## Diagnósticos de compilación y navegación

En Run / Build, expandir «Diagnósticos» y después el registro para ver severidad,
fase, mensaje y ubicación. «Ver código exportado» abre una captura de los archivos fuente y CMake tomada al iniciar ese Build, con números de línea y
resaltado. No lee archivos arbitrarios mencionados en logs. Las modificaciones
externas posteriores no aparecen en esta captura, ni se modifica el proyecto.

Los errores de `ui.hpp` (C++), `studio_ui.h` (C11), `StudioUI.cs` (C#),
`main.py` (Python), `StudioUI.java` (Java) o `ui.rs` (Rust) se relacionan con el rango más específico de un nodo del
generador. «Seleccionar widget» cambia la selección del diseñador; se bloquea si
el documento cambió o ese nodo ya no existe. Errores en dependencias o código de
arranque conservan su ubicación y no se atribuyen a un widget. Los avisos del
generador aparecen junto a los del compilador, distinguidos por fase/origen.

Se reconocen formatos de texto habituales GCC/Clang (`archivo:línea:columna`),
MSVC y Roslyn (`archivo(línea,columna): error C...` o `CS...`), errores globales del compilador/linker
y CMake (`Error/Warning at archivo:línea` y mensajes globales). El parser consume
fragmentos, CRLF y última línea sin salto final, elimina ANSI/hipervínculos de los
registros y une continuaciones de CMake. La salida original reciente queda en el log.
Los mensajes de Run permanecen en el log, sin interpretarlos como salida de compilador.

Límites: 1000 registros, líneas de entrada hasta 16 KiB; al alcanzar el límite,
errores nuevos pueden desplazar avisos/notas. Los formatos desconocidos, mensajes
localizados, trazas de templates y variantes de cada toolchain pueden quedar solo
en el log. La asociación de archivos usa rutas normalizadas léxicamente; aliases
por symlinks u otras diferencias de ruta pueden quedar sin asociación. No es un
editor externo, un parser completo de todos los compiladores ni análisis de runtime.

BuildDiagnosticsTest verifica muestras GCC/Clang/MSVC/CMake, fragmentos de un byte,
colores, mensajes multilínea, rutas, rangos anidados y límites. La integración real
inyecta `#error` con `#line` en una exportación durante configure: el Build falla,
el registro apunta al widget esperado y la captura original sigue disponible.
La inyección solo ocurre en el fixture de test. La compilación real verificada es
Windows/GCC; MSVC/Clang se han probado aquí mediante muestras de formato, no ejecutados.

## Generador y exportación C++ verificados

El visor ofrece Header, Render y archivo completo; «Exportar CMake...» solicita una
carpeta nueva cuyo padre ya exista. Produce `ui.hpp`, `main.cpp`, `CMakeLists.txt`,
`README.md` y `codegen_report.txt`. Nunca reemplaza una carpeta existente. Si se
interrumpe el proceso puede quedar una carpeta incompleta; se debe elegir otra ruta.
La exportación no modifica ni guarda el proyecto abierto.

El scaffold fija versiones y hashes de Dear ImGui/GLFW, compila el adaptador oficial
para `std::string` y no depende del editor, JSON, historial ni credenciales AI.
Permite configurar y compilar con CMake y ejecutar `--smoke-test`. La descarga inicial
necesita red; Linux requiere las dependencias de sistema de GLFW/OpenGL. El arranque
usa OpenGL 3.0/GLSL 130 y en macOS solicita 3.2 core/GLSL 150.

Se corrigieron literales flotantes, nombres reservados y colisiones de variables,
IDs de ventanas/widgets, formatos numéricos, cadenas escapadas y texto sin límite
de 256 bytes. Los radios comparten estado sólo cuando tienen el mismo `varName`
original. Los combos, listas, vectores, colores y gráficos usan los datos del proyecto.
Las ramas invisibles no generan controles ni estado. Header usa funciones inline
y se prueba su enlace desde dos unidades de traducción; Render incluye el estado y
sus dependencias, sin `main`. Cada interfaz exportada conserva los nombres públicos
`ApplyStudioStyle` y `RenderStudioUI`; incluir una sola interfaz por unidad de traducción.

El generador cubre las rutas básicas disponibles en preview y las ventanas. Widgets
avanzados pendientes producen placeholders y avisos; sus hijos visibles se recorren.
El informe incluye IDs y líneas de `ui.hpp`. C++, C11, Python, C#, Java
y Rust exportan el menú incrustado y los widgets de título admitidos
por la vista previa.
También exportan los controles de cerrar, minimizar, maximizar y restaurar
la ventana de Dear ImGui, la alineación e icono del título, la altura y los
márgenes de la barra, y los colores, transparencia y estilos por ventana.
Minimizar deja visible la barra. Maximizar usa el área de trabajo del
viewport de la aplicación. Prueba estos ajustes abriendo
[Barra de título](../examples/barra-de-titulo.json). Las acciones propias de
botones y menús requieren código de aplicación.

En C# y Java, los controles integrados como casillas y campos de texto se
dibujan con el cuerpo abierto. Los botones de título dedicados y los
controles de ventana siguen disponibles al plegar. En Linux, el ejemplo
exportado a C#, Java y Rust se verificó en Ubuntu bajo WSL. C# carga el GLFW
de su paquete NuGet antes de iniciar Silk.NET cuando la DLL de la aplicación
se distribuye junto a la carpeta `runtimes`.

CTest compila Header y Render en ocho casos: las cuatro plantillas,
todos los tipos del catálogo histórico, proyecto vacío, el ejemplo de barra de título
y un caso de borde (colisiones,
palabras reservadas, UTF-8, texto largo/NUL, formato inválido, combos/gráficos vacíos).
Ejecuta sus frames ImGui, comprueba estado y enlace entre TUs. También compila y
arranca el modo completo con GLFW/OpenGL. El scaffold se configuró y compiló por
separado y pasó su propio `ExportSmoke`; se reutilizaron las fuentes descargadas
mediante `FETCHCONTENT_SOURCE_DIR_GLFW` y `FETCHCONTENT_SOURCE_DIR_IMGUI`.
Las salidas generadas se compilan con avisos tratados como errores. Se ejecutaron
los casos de CTest en Windows y Ubuntu WSL con GCC.

### Renderizador Vulkan opcional para C++

En **Proyecto nativo → Ejecutar / Compilar**, selecciona **C++** y **Vulkan**
en **Renderizador C++**. El perfil guarda la selección. En el diálogo de
exportación C++ también puedes elegir el renderizador del proyecto que vas
a guardar. OpenGL es la opción inicial de los perfiles nuevos y anteriores.

El destino Vulkan usa GLFW 3.4 y genera su propio arranque para Windows y
Linux. Requiere un controlador Vulkan, los encabezados y la biblioteca de
enlace. En Windows se pueden usar los archivos del SDK o los de la toolchain.
En Ubuntu/Debian están en `libvulkan-dev`. Usa **Ruta del SDK de Vulkan**
para una instalación en otra carpeta. El CMake exportado acepta
`-DVulkan_ROOT=...` y diagnostica si faltan los archivos de desarrollo.
Puedes abrir [Imágenes y entrada](../examples/vulkan-imagenes.json) y exportarlo
con cualquiera de los dos renderizadores.

Para ejecutar la prueba del ejemplo con CTest, habilita
`STUDIO_TEST_VULKAN_EXPORT` al configurar el editor. Si el SDK está fuera
de las rutas habituales, define `STUDIO_VULKAN_TEST_ROOT`. La prueba
verifica la configuración, la entrada y la presentación de texturas
PNG y JPEG.

## Exportación Python

Selecciona Python en el panel de código. Puedes copiar el script o exportarlo a
una carpeta nueva. La exportación incluye main.py, requirements.txt, README.md
y codegen_report.txt. El informe detalla omisiones y rangos de líneas por nodo.

El script genera ventanas, controles básicos, entradas, listas, gráficos simples,
tablas, pestañas y layouts con Dear ImGui Bundle 1.93.0. Usa un diccionario de
estado independiente del proyecto. Las opciones de chrome personalizado,
widgets avanzados y acciones de aplicación quedan pendientes. Los widgets sin
soporte aparecen como marcadores y conservan el render de sus hijos visibles.

Ejecuta main.py con CPython y el binding indicado en requirements.txt.
El paquete Linux de imgui-bundle probado en WSL abre sus ventanas mediante
X11. En una sesión Wayland necesita XWayland. El editor nativo y las
exportaciones C++ y C11 se probaron también con Wayland directo.
La opción --smoke-test ejecuta tres frames gráficos ocultos. También se puede
usar «Proyecto nativo → Run / Build → Destino: Python». Elige un CPython 3.10+
con imgui-bundle 1.93.0 o indica en «Ruta del binding» la carpeta que contiene
imgui_bundle y su directorio dist-info. La ruta debe ser compatible con el
intérprete y la plataforma. En Windows, si PATH encuentra otro Python, indica
la ruta al ejecutable CPython o al del entorno virtual.

Build verifica la sintaxis sin ejecutar el script del diseño y carga el binding
para verificarlo. Run acepta la copia exportada si sus bytes siguen iguales.
Python usa rutas explícitas y prescinde de PYTHONPATH. Los paquetes del propio
intérprete o entorno virtual siguen disponibles. El editor no invoca pip.

Para verificar la salida con CTest, configura STUDIO_PYTHON_TEST_EXECUTABLE con
la ruta del intérprete. STUDIO_PYTHON_TEST_BINDING_DIR permite indicar un
directorio aislado que contenga el binding. Con el editor habilitado, estas
opciones añaden NativePythonProjectBuildTest, que comprueba la verificación,
el arranque gráfico, la cancelación, el rechazo de una versión incompatible y
el bloqueo de una copia cambiada. Sin estas opciones, la compilación
nativa y las pruebas C++ siguen disponibles sin dependencias Python.

## Exportación C# para ImGui.NET

Selecciona «C#» en el panel de código. «Exportar .NET...» escribe StudioUI.cs,
StudioExport.csproj, Program.cs y el informe por widget en una carpeta nueva.
La exportación usa .NET 8, ImGui.NET 1.91.6.1 y Silk.NET 2.23.0. Ejecuta
`dotnet run` desde esa carpeta para abrir la interfaz en una ventana GLFW/OpenGL.
NuGet restaura las dependencias al compilar. Se requiere un contexto OpenGL 3.3.
También puedes compilar y ejecutar desde «Proyecto nativo → Run / Build → C#».

`dotnet run -- --smoke-test` prueba tres frames en una ventana oculta y comprueba
errores OpenGL. `dotnet run -- --headless-test` dibuja tres frames en memoria,
sin ventana ni GPU. StudioUI.cs también puede integrarse en otro host. Llama a
StudioUI.ApplyStyle al crear el contexto, conserva una UIState por interfaz y
llama a StudioUI.Render en cada frame.

Los controles básicos, contenedores y gráficos simples usan los datos del
proyecto. Los widgets avanzados pendientes muestran un marcador y se enumeran
en codegen_report.txt, junto con el rango de líneas de StudioUI.cs. Las acciones
de aplicación y el chrome personalizado requieren implementación adicional.
Los campos de texto tienen capacidad de al menos 4096 bytes, con margen para
valores iniciales de mayor tamaño.

Para activar GeneratedCSharpRuntime en CTest, configura
STUDIO_DOTNET_TEST_EXECUTABLE con la ruta de dotnet. La prueba selecciona el SDK
8, usa una caché NuGet local al directorio de compilación y compila y ejecuta
siete exportaciones en memoria con avisos tratados como errores. Activa
STUDIO_TEST_CSHARP_WINDOW para comprobar también dos exportaciones en una ventana
GLFW/OpenGL oculta. En una compilación del editor, estas opciones añaden además
NativeCSharpProjectBuildTest, que verifica restauración, compilación, arranque,
cancelación y diagnóstico de Roslyn vinculado a un widget en Windows.

## Exportación Java

Selecciona «Java» en el panel de código y usa «Exportar Java…». La carpeta nueva
contiene StudioUI.java, Main.java, pom.xml, el informe de generación y los pasos
de compilación. Main abre la ventana con GLFW/OpenGL. StudioUI expone el estado,
el estilo y el render para integrarlos también en otra aplicación. Los widgets y
propiedades pendientes se enumeran con sus ubicaciones en codegen_report.txt.

La salida apunta a Java 17 y usa imgui-java 1.90.0 con LWJGL 3.3.6. Se puede
compilar directamente con javac y ejecutar con java si los JAR y los nativos
están en el classpath. El pom.xml ofrece la opción Maven para resolverlos. La
prueba de esta entrega usa el JDK directamente en Windows.

En «Proyecto nativo → Run / Build → Destino: Java» selecciona el directorio
raíz del JDK y la carpeta con los JAR, incluidos los nativos de tu sistema.
Si el campo del JDK está vacío, el editor busca javac en PATH y java junto a él.
Build usa esos JAR locales y Run inicia Main con las clases compiladas.
Para verificar GLFW/OpenGL, elige «Probar arranque». El editor no descarga JAR
ni invoca Maven o Gradle en este flujo. 

GeneratedJavaRuntime se activa indicando STUDIO_JAVA_TEST_JDK (directorio del
JDK) y STUDIO_JAVA_TEST_BINDING_DIR (directorio con los JAR de imgui-java-app,
imgui-java-binding, imgui-java-lwjgl3, lwjgl, lwjgl-glfw, lwjgl-opengl y sus
nativos de LWJGL). Compila con javac --release 17 -Xlint:all -Werror y ejecuta
siete proyectos con --smoke-test. Con el editor habilitado, estas opciones añaden
NativeJavaProjectBuildTest, que verifica la compilación, el arranque, la cancelación
y el vínculo de un error javac con su widget. Los JAR se proporcionan por separado.

## Exportación C11

Selecciona «C» en el panel de código. «Exportar C…» guarda studio_ui.h,
main.c, el puente backend.cpp, un proyecto CMake y el informe por widget
en una carpeta nueva. Compila con CMake y ejecuta studio_export_c para abrir
la interfaz con GLFW/OpenGL. CTest ejecuta el modo de tres frames ocultos.

La interfaz y su punto de entrada se compilan como C11. El proyecto también
requiere un compilador C++ para cimgui y los backends oficiales. CMake descarga
cimgui 1.91.6, su revisión correspondiente de Dear ImGui y GLFW 3.4 con hashes
SHA-256 fijados. Las rutas locales pueden pasarse con FETCHCONTENT_SOURCE_DIR
si ya dispones de esas revisiones. La exportación se verificó en Windows con
GCC y Ninja. También puedes compilar y ejecutar desde «Proyecto nativo → Run / Build».

La salida comparte cobertura de widgets y estilos con el generador C++, con
buffers editables C de capacidad fija para texto y avisos para las omisiones
conocidas. Para probar las siete exportaciones C, activa STUDIO_TEST_C_EXPORT.
STUDIO_C_TEST_GLFW_SOURCE, STUDIO_C_TEST_CIMGUI_SOURCE y STUDIO_C_TEST_IMGUI_SOURCE
permiten reutilizar las dependencias descargadas.

## Exportación Rust

Selecciona **Rust** en el panel de código y exporta a una carpeta nueva.
El proyecto Cargo incluye `ui.rs` con el estilo, estado y widgets, y
`main.rs` con el host GLFW/OpenGL. El binding es imgui-rs 0.12.0.
Dentro de esa carpeta puedes compilar con `cargo build`, abrir la aplicación
con `cargo run` o comprobar el arranque con `cargo run -- --smoke-test`.

La exportación desde el editor funciona sin Cargo. Para compilar necesitas
Rust, Cargo y un toolchain C++ compatible. El host de ejemplo usa OpenGL 3.3.
La biblioteca puede compilarse con `cargo build --no-default-features` para
integrarla en otro host. Consulta `codegen_report.txt` para ver las omisiones
de widgets y propiedades y los rangos de código por ID.

En **Run / Build**, selecciona **Rust** y elige **Debug** o **Release**.
Si dejas vacío el ejecutable, el editor busca Cargo en PATH y en los
directorios habituales de Rustup. Si `rustc` falta en PATH, el editor busca
el compilador junto a Cargo. **Crates desde la caché local de Cargo**
activa `--offline` para la resolución de crates. El script de GLFW puede
obtener por separado sus bibliotecas precompiladas en Windows y macOS.
Build compila una copia nueva del diseño. Run lanza el ejecutable
que informó Cargo para ese diseño y perfil. Los diagnósticos de `ui.rs`
pueden abrir el código generado y seleccionar el widget de origen.

En Windows, Cargo 1.100 con el target MSVC compiló y ejecutó siete proyectos
exportados. El controlador Run / Build se verificó en Debug y Release.
Para repetir las pruebas gráficas y de Build/Run en CTest,
configura `STUDIO_RUST_TEST_CARGO` con la ruta a Cargo y prepara su caché de
crates. En Ubuntu 24.04 sobre WSL 2 se verificó Run / Build en Debug con
Cargo y rustc 1.100 para Linux, incluido el arranque gráfico, la cancelación
y la navegación de un error de rustc. Release en WSL y las pruebas en macOS
quedan pendientes.

## Asistente IA nativo

Abre **Vista → Asistente IA**. El panel ofrece Ollama, LM Studio, OpenAI,
Anthropic y Google Gemini. Elige el proveedor y el modelo, o pulsa
**Detectar modelos**. Puedes indicar otra URL base. **Guardar ajustes**
conserva el proveedor, las URLs y los modelos en el directorio de datos
de la aplicación.

Usa **Clave temporal** si el servidor requiere autenticación. Para las URLs
oficiales de OpenAI, Anthropic y Gemini el editor también acepta
`OPENAI_API_KEY`, `ANTHROPIC_API_KEY` y `GEMINI_API_KEY` respectivamente.
Las claves escritas en el panel se descartan al cambiar de proveedor o URL.
Los ajustes, el proyecto y las exportaciones excluyen las claves.

Abre **Claves guardadas en el sistema** para guardar, cargar o borrar una
clave. Cada entrada corresponde a un proveedor y su URL base. Guardar usa
el Administrador de credenciales de Windows o el llavero de la sesión en
Linux. Cargar copia la clave al campo temporal.

En Linux, CMake habilita el llavero si `pkg-config` encuentra libsecret 0.19
o superior. Guardar requiere una sesión D-Bus con un servicio de secretos y
un llavero disponible. Puedes usar `STUDIO_ASSISTANT_SECRET_SERVICE=OFF`
para compilar sin libsecret. En macOS se integró Keychain Services y queda
pendiente comprobarlo en una Mac. Las claves de sesión y las variables de
entorno siguen disponibles en los tres sistemas.

Al pulsar **Enviar**, el proveedor elegido recibe la consulta, el historial
reciente de esta conversación y un resumen con el nombre, las ventanas y
el widget seleccionado del proyecto. Si la respuesta propone widgets, el panel
muestra el JSON antes de aplicar. **Agregar al diseño** añade las ventanas en
un paso de Deshacer. Si el proyecto cambió desde la consulta, pide una
propuesta basada en el diseño actual.

CMake utiliza libcurl 7.85 o superior si encuentra sus archivos de desarrollo.
Puedes indicar `CURL_DIR` cuando el paquete esté en una ruta personalizada.
`STUDIO_ASSISTANT_NETWORK=OFF` compila con la conexión desactivada. El resto del
editor también funciona si faltan los archivos de desarrollo de libcurl.
`AssistantApiTest` verifica el panel y las reglas de URL. Con libcurl y un
intérprete Python disponible, `AssistantLoopbackTest` prueba los cinco formatos
contra un servidor local simulado. En Ubuntu WSL se verificó con las cabeceras
de libcurl extraídas en el directorio de compilación y la biblioteca del
sistema, mediante `CURL_INCLUDE_DIR` y `CURL_LIBRARY_RELEASE`.

## Compilación Nativa (Linux / macOS / Windows)

Desde la raíz del repositorio, con Ninja y un compilador C++17:

```bash
cmake -S . -B build/native -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --target imgui_studio
```

- **Windows** — `build/native/bin/imgui_studio.exe`.
- **Linux** — `build/native/bin/imgui_studio`.
- **macOS** — `open "build/native/bin/ImGui Studio.app"`. La compilación y
  el arranque siguen pendientes de verificación en una Mac.

### Ubuntu en WSL 2

Desde la raíz del proyecto en Ubuntu, usa un directorio de compilación propio
para Linux. El arranque gráfico requiere que WSLg esté disponible.

```bash
cmake -S . -B build/wsl-native -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/wsl-native -j 4
ctest --test-dir build/wsl-native --output-on-failure -j 3
./build/wsl-native/bin/imgui_studio --smoke-test
```

Si ya tienes las fuentes de GLFW 3.4 y Dear ImGui 1.91.5, puedes pasar sus rutas
con `FETCHCONTENT_SOURCE_DIR_GLFW` y `FETCHCONTENT_SOURCE_DIR_IMGUI`.
En Ubuntu 24.04.1 LTS sobre WSL 2 con GCC 13.3 pasaron las 20 pruebas
configuradas, incluido Build/Run de un proyecto C++ exportado. Mesa informó
avisos de EGL/Zink y el editor completó los tres fotogramas.

## Paquetes de escritorio

Activa `STUDIO_PACKAGING` en una compilación separada. CPack genera el paquete
y su suma SHA-256 en `packages` dentro del directorio de compilación. Esta
opción usa Python 3 para preparar los iconos. La compilación normal del editor
conserva sus requisitos anteriores.

```bash
cmake -S . -B build/package -DSTUDIO_PACKAGING=ON -DBUILD_CORE_TEST=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/package --config Release
cpack --config build/package/CPackConfig.cmake -C Release
```

- **Windows** genera un ZIP portátil con el ejecutable y las DLL detectadas.
  La preparación de DLL requiere CMake 3.21 o posterior. Descomprime el ZIP y
  abre `bin/imgui_studio.exe`. Para generar el asistente de instalación y
  accesos directos, usa `cpack -G NSIS` con el mismo archivo de configuración
  y `makensis` disponible en el PATH del proceso. Con NSIS 3.13 portátil se
  genera el instalador EXE y su pantalla de licencia MIT. La firma
  Authenticode y la validación del aviso de elevación en Windows quedan
  pendientes.
- **Ubuntu/Debian** genera un DEB con dependencias declaradas, icono y acceso
  en el menú de aplicaciones. Instala `dpkg-dev` y `file` para calcular
  dependencias. El paquete declara también las bibliotecas que GLFW carga
  en ejecución para cada backend habilitado. Compila en la distribución
  de destino para usar sus nombres de paquetes y versiones de bibliotecas.
  Si la compilación usa libsecret,
  el DEB declara la dependencia incluso al compilar desde un SDK extraído.
  Para distribuirlo, configura
  `STUDIO_PACKAGE_MAINTAINER` con los datos del mantenedor.
- **macOS** usa `-G DragNDrop` desde una Mac. El
  [flujo de empaquetado](CI.md#imágenes-de-disco-para-macos) prepara el DMG
  para Intel y Apple Silicon con macOS 15 o posterior. La ejecución del
  flujo, la validación del DMG y la firma siguen pendientes.

Se comprobó el ZIP extraído en Windows con el PATH reducido a las rutas del
sistema. El DEB se instaló, arrancó y retiró con una base `dpkg` aislada en
Ubuntu WSL. En Debian 13 se comprobó la instalación y retirada con APT y
el arranque con X11 virtual. Windows instala `share/doc/ImGuiStudioNative/LICENSE.txt` y
los DEB instalan `/usr/share/doc/imgui-studio/copyright`. Ambos incluyen
los avisos de dependencias en el subdirectorio `third-party`.

Los destinos C++ opcionales se descargan por separado. Genera sus ZIP con
los objetivos `studio_android_pack`, `studio_macos_pack` y `studio_ios_pack`.
En el editor, selecciona el destino en **Código generado → C++** e indica
la carpeta descomprimida del complemento.

Después de compilar, ejecuta la comprobación de paquete desde CTest:

```bash
ctest --test-dir build/package -C Release -R 'WindowsZipPackageTest|LinuxDebPackageTest' --output-on-failure
```

Cada prueba genera un paquete temporal, verifica SHA-256, la licencia,
las guías y los ejemplos, y arranca el editor distribuido. La prueba DEB
también verifica instalación y retirada en una
raíz temporal mediante `dpkg`, `dpkg-deb` y `fakeroot`.

Para comprobar un ZIP o DEB ya generado, usa `packaging/verify_package.py`
con `--artifact` y `--format ZIP` o `--format DEB`. Conserva el archivo
`.sha256` al lado del paquete. En Ubuntu indica también `--dpkg /usr/bin/dpkg`,
`--dpkg-deb /usr/bin/dpkg-deb` y `--fakeroot /usr/bin/fakeroot`.
Añade `--require-libsecret` para comprobar la dependencia de una compilación
con el llavero de Linux habilitado.

En Windows, el siguiente comando comprueba el SHA-256 del EXE y busca
instalaciones previas, entradas del registro y accesos directos en conflicto.

```powershell
.\packaging\verify_windows_installer.ps1 `
  -Installer .\build\package-win\packages\ImGuiStudio-1.0.0-Windows-AMD64.exe `
  -PreflightOnly
```

Ejecuta el mismo comando sin `-PreflightOnly` desde una terminal de
administrador para probar el ciclo completo. La prueba instala en una
carpeta temporal junto a `packages`, verifica el registro y los accesos del
menú Inicio, arranca el editor y comprueba que el desinstalador los retire.

Para probar el núcleo con los encabezados generados, configura CMake siguiendo
la [guía de pruebas](BUILD_AND_TEST.md#elegir-pruebas).

