# Cómo contribuir

[English](CONTRIBUTING.en.md)


Lee el [mapa del código](docs/SOURCE_MAP.md) y el [estado de soporte](docs/CAPABILITIES.md).
En [Cómo funciona el código](docs/code/README.md) encontrarás los contratos por archivo y las pruebas que cubren cada módulo.
Cada cambio debe identificar el comportamiento actual, el resultado esperado y las plataformas afectadas.

## Tu primera contribución

- **Documentación.** Abre un ejemplo de `examples/`, reproduce el procedimiento descrito, corrige la guía en español e inglés y comprueba los enlaces. Si cambia una etiqueta de la aplicación, compárala con el catálogo de interfaz antes de escribir el nuevo nombre.
- **Editor.** Para un problema de selección, empieza en el lienzo. Para uno de valores o dibujo, revisa el inspector y la vista previa. Incluye el JSON mínimo que reproduce el fallo y prueba con diseño y vista previa. [Mapa del código](docs/SOURCE_MAP.md)
- **Exportación.** Reduce el diseño a un widget y genera el lenguaje afectado. Adjunta el fragmento de `codegen_report.txt` y señala si falla al generar, compilar o ejecutar. Indica el backend gráfico y si se necesita un complemento opcional. [Guía de exportación](docs/EXPORTING.md)

## Ejecutar verificaciones

Desde el directorio del repositorio, configura una compilación del núcleo. Este recorrido sirve para cambios de formato, ediciones del árbol y recuperación y funciona sin una sesión gráfica.

```sh
python tools/check_docs.py
cmake -S . -B build/contrib-core -G Ninja -DBUILD_STUDIO_GUI=OFF
cmake --build build/contrib-core --target verify_persistence verify_sessions verify_document_edit
ctest --test-dir build/contrib-core -R "^(ProjectPersistenceTest|SessionRecoveryTest|DocumentEditingTest)$" --output-on-failure --no-tests=error
```

Para cambios gráficos, usa la [guía de compilación](docs/BUILD_AND_TEST.md) para construir el editor y la [guía de validación](docs/VALIDATION.md) para elegir pruebas en una sesión gráfica. En el pull request indica qué comando ejecutaste, qué resultado obtuviste y en qué host. Si adjuntas una captura para un cambio de interfaz, incluye los pasos para llegar a esa pantalla.

## Reportar y proponer

- Abre un issue con versión o commit, sistema, arquitectura, compilador y backend relevante. En Linux indica Wayland, X11 o WSLg. Para un fallo de exportación añade lenguaje y destino.
- Adjunta un proyecto pequeño con datos de ejemplo y pasos repetibles. Incluye el diagnóstico relevante de Run / Build o el fragmento de `codegen_report.txt`.
- Para funciones nuevas explica quién las usa, en qué parte del editor aparecen y qué archivos o resultados cambian. Usa la [política de seguridad](SECURITY.md) para incidentes sensibles.

## Preparar un cambio

1. Crea una rama y compila siguiendo la [guía de desarrollo](docs/BUILD_AND_TEST.md).
2. Busca primero el flujo actual en modelo, editor, persistencia y generadores. Evita duplicar el comportamiento entre vista previa y exportación.
3. Para cambios del formato, prueba también un proyecto guardado con la versión anterior. Mantén los datos extra y el historial cuando corresponda.
4. Agrega o ajusta una prueba si cambia un contrato que las existentes no cubren. Usa la [guía de validación](docs/VALIDATION.md) para seleccionar pruebas y actualiza las dos versiones de la guía pertinente.
5. Abre un pull request con el motivo, los pasos de prueba, resultados y plataformas pendientes. Para cambios visuales, una captura ayuda a revisar la jerarquía, los tamaños y las traducciones.

La [guía de extensiones](docs/EXTENDING.md) recorre un widget, una traducción y un exportador. Conserva la licencia y procedencia de cualquier dependencia que integres.

Ejecuta el [verificador de documentación](docs/CI.md#repetir-la-comprobación-de-documentación) antes de enviar cambios de rutas o enlaces. En tu pull request indica las comprobaciones que dependen de hardware o gráficos.
