# ImGui Studio · iOS C++

[Español](#español) · [English](#english)

## Español

Descomprime el complemento. En **Código generado → C++ → Exportar iOS**,
indica la carpeta `iOS-Cpp` que contiene `template.version` y una carpeta
nueva donde guardar el proyecto.

En una Mac con CMake 3.24 o posterior y Xcode con el SDK y un simulador iOS:

```sh
cmake -S . -B build-simulator -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
open build-simulator/StudioExport.xcodeproj
```

Selecciona el esquema `studio_export` y un simulador en Xcode. Para un
iPhone o iPad, configura en una carpeta de compilación distinta:

```sh
cmake -S . -B build-device -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DSTUDIO_BUNDLE_ID=org.ejemplo.app
open build-device/StudioExport.xcodeproj
```

Selecciona tu equipo en **Signing & Capabilities** y usa un identificador
único. La plantilla apunta a iOS 15 en adelante. Ofrece entrada de un dedo
mediante UIKit y usa Metal. El teclado en pantalla y la composición IME
requieren integración adicional. Ajusta las dimensiones de la interfaz para
la pantalla de destino. La primera configuración descarga Dear ImGui y,
si hay imágenes, stb_image con sus hashes fijados. Conecta las acciones
después de `RenderStudioUI` y revisa `codegen_report.txt`.

## English

Extract the optional pack. In **Generated code → C++ → Export iOS**, choose the `iOS-Cpp` directory containing `template.version` and a new output directory.

On a Mac with CMake 3.24 or later, Xcode, the iOS SDK, and an iOS simulator, run from the exported directory:

```sh
cmake -S . -B build-simulator -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
open build-simulator/StudioExport.xcodeproj
```

Select the `studio_export` scheme and a simulator in Xcode. For an iPhone or iPad, configure a separate build directory:

```sh
cmake -S . -B build-device -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DSTUDIO_BUNDLE_ID=org.example.app
open build-device/StudioExport.xcodeproj
```

Select your team in **Signing & Capabilities** and use a unique bundle identifier. The template targets iOS 15 or later. It uses Metal and supports single-finger input through UIKit. On-screen keyboard and IME composition require additional integration. Size the interface for the target screen. The first configure downloads Dear ImGui and, for designs with images, stb_image from hash-pinned archives. Connect application actions after `RenderStudioUI` and review `codegen_report.txt`.
