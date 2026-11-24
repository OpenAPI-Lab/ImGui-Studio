# ImGui Studio · macOS C++

Descomprime el complemento. En **Código generado → C++ → Exportar macOS**,
indica la carpeta `macOS-Cpp` que contiene `template.version` y una carpeta
nueva donde guardar el proyecto.

En una Mac con CMake 3.24 o posterior y Xcode con el SDK de macOS:

```sh
cmake -S . -B build -G Xcode
cmake --build build --config Debug
open build/Debug/studio_export.app
```

La plantilla apunta a macOS 12 en adelante. Usa Metal, MetalKit y el backend
AppKit de Dear ImGui para entrada de ratón y teclado. La primera configuración
descarga Dear ImGui y, si hay imágenes, stb_image con sus hashes fijados.
`ui.hpp` contiene el estado y la interfaz. Consulta `codegen_report.txt` y
conecta las acciones de tu aplicación después de `RenderStudioUI`.

Puedes cambiar el identificador con `-DSTUDIO_BUNDLE_ID=org.ejemplo.app`.
La firma y la notarización para distribuir el .app se configuran aparte.
