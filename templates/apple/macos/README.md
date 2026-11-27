# ImGui Studio · macOS C++

[Español](#español) · [English](#english)

## Español

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

## English

Extract the optional pack. In **Generated code → C++ → Export macOS**, choose the `macOS-Cpp` directory containing `template.version` and a new output directory.

On a Mac with CMake 3.24 or later and Xcode with the macOS SDK, run from the exported directory:

```sh
cmake -S . -B build -G Xcode
cmake --build build --config Debug
open build/Debug/studio_export.app
```

The template targets macOS 12 or later. It uses Metal, MetalKit, and the Dear ImGui AppKit backend for mouse and keyboard input. The first configure downloads Dear ImGui and, for designs with images, stb_image from hash-pinned archives. `ui.hpp` defines the interface and state. Read `codegen_report.txt` and connect application actions after `RenderStudioUI`.

Set the bundle identifier using `-DSTUDIO_BUNDLE_ID=org.example.app`. Configure signing and notarization separately when distributing the app bundle.
