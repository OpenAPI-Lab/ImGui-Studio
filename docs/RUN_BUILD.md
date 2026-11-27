# Run / Build y sus diagnósticos

[English](RUN_BUILD.en.md)


[Compilar el editor](BUILD_AND_TEST.md) prepara ImGui Studio. **Run / Build** opera sobre la aplicación que estás diseñando. Esta separación permite probar una exportación con su toolchain y dependencias propias.

## Flujo y validez del resultado

| Acción | Entrada | Resultado |
| --- | --- | --- |
| Detectar herramientas | Directorio de trabajo, perfil y entorno del editor | Rutas sugeridas. El toolchain se verifica al configurar o compilar. |
| Build | Diseño actual y ajustes del perfil | Exportación nueva por intento, registro y, si llega a completarse, un artefacto ejecutable o un script validado. |
| Run | Resultado válido del diseño y perfil actuales | Lanza lo que se compiló o verificó en ese intento. |
| Probar arranque | Mismo resultado y un destino con prueba de arranque | Ejecuta la comprobación del host generado. |
| Detener | Proceso activo | Cancela el proceso o grupo que controla el editor. |

Prueba con [Barra de título](../examples/barra-de-titulo.json). Elige C++, configura un espacio de trabajo vacío y compila. Abre **Ver código exportado** para inspeccionar la captura del intento. Ejecuta la aplicación, cambia una propiedad y vuelve a compilar. La validez del resultado depende del diseño y de los ajustes usados para construirlo. Cada carpeta de intento queda disponible para diagnóstico.

## Perfiles y persistencia

Puedes crear hasta 32 perfiles. **Guardar perfiles** confirma sus cambios en `build_profiles.json`, dentro del directorio de estado. **Recargar perfiles** vuelve a leerlo y conserva los datos en memoria si falla. Al reiniciar se restaura el perfil activo. Para ejecutar, compila de nuevo en la sesión nueva.

El formato versión 1 usa `schemaVersion`, `activeProfile` y `profiles`. Cada perfil guarda `id`, `name`, `workspace` y `settings`. C++ puede guardar `cppRenderer` y `vulkanRoot`. Rust guarda `cargo` y `rustOffline`. Hay campos separados para las herramientas de C11, C#, Python y Java. **Incluir perfil en proyecto** escribe explícitamente bajo `buildConfig.nativeCpp`. Antes de compartir ese JSON, revisa las rutas que contiene.

## Interpretar un fallo

1. Distingue fase de configuración, compilación o ejecución. Si falta CMake, SDK o una biblioteca, corrige el perfil antes de revisar el widget.
2. Abre **Diagnósticos** y consulta archivo, línea y severidad. Los diagnósticos de dependencias o del host conservan su ubicación aunque carezcan de nodo asociado.
3. Para una ubicación generada elige **Seleccionar widget** cuando esté disponible. Se compara con la captura del intento y el árbol actual.
4. Si el texto aparece solo en el registro, conserva la línea original en el reporte. El parser admite formatos habituales de GCC, Clang, MSVC, Roslyn y CMake.

[Perfiles](../include/imguistudio/build/studio_build_profiles.hpp) · [Controlador](../include/imguistudio/build/studio_build.hpp) · [Procesos](../include/imguistudio/build/studio_process.hpp) · [Diagnósticos](../include/imguistudio/build/studio_build_diagnostics.hpp)
