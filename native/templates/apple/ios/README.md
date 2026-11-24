# ImGui Studio · iOS C++

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
