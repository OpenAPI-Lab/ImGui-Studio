# ImGui Studio · Android C++

[Español](#español) · [English](#english)

## Español

Complemento opcional. Descomprime el ZIP y conserva la carpeta `Android-Cpp`.
En ImGui Studio selecciona C++ en Código generado, pulsa **Exportar Android**
y elige la carpeta que contiene `template.version`. Escribe una ruta nueva
para el proyecto exportado.

### Compilar el proyecto exportado

Abre esta carpeta como proyecto Gradle en Android Studio. Configura Gradle
8.9 y un JDK 17 o 21. En SDK Manager instala Android SDK Platform 35, Build Tools
34.0.0, NDK (Side by side) 27.2.12479018 y CMake 3.22.1. Con una instalación
de Gradle 8.9 y el SDK configurado también puedes ejecutar:

```sh
gradle assembleDebug
```

El APK de depuración se genera en `build/outputs/apk/debug/`. Usa un
dispositivo arm64 o emulador x86_64, con Android 7.0 o posterior y OpenGL ES 3.
Se puede cambiar el identificador con `-PstudioApplicationId=org.ejemplo.app`
y el nombre visible en `AndroidManifest.xml`.

El proyecto descarga Dear ImGui 1.91.5 y, cuando hay imágenes, stb_image,
con hashes fijados en CMake. `ui.hpp` contiene la interfaz y su estado.
`main.cpp` aloja Dear ImGui en NativeActivity y pasa la entrada al backend
Android. Conecta tus acciones después de `RenderStudioUI` y consulta
`codegen_report.txt` para ver avisos de cobertura. Ajusta las dimensiones
de tus ventanas a la pantalla de destino. Esta plantilla requiere una
integración adicional para el teclado en pantalla y la composición IME.

## English

Extract the optional ZIP and keep the `Android-Cpp` directory. In ImGui Studio, select C++ in **Generated code**, choose **Export Android**, and point the exporter to the directory containing `template.version`. Choose a new output directory for the exported project.

### Build the exported project

Open the exported directory as a Gradle project in Android Studio. Use Gradle 8.9 and JDK 17 or 21. In SDK Manager, install Android SDK Platform 35, Build Tools 34.0.0, NDK (Side by side) 27.2.12479018, and CMake 3.22.1. With Gradle 8.9 and the SDK configured, you can also run:

```sh
gradle assembleDebug
```

The debug APK is in `build/outputs/apk/debug/`. Use an arm64 device or an x86_64 emulator with Android 7.0 or later and OpenGL ES 3. Set the application ID using `-PstudioApplicationId=org.example.app` and the display name in `AndroidManifest.xml`.

CMake downloads Dear ImGui 1.91.5 and, for designs with images, stb_image. Both downloads are hash-pinned. `ui.hpp` defines the interface and its state. `main.cpp` hosts Dear ImGui in NativeActivity and forwards input to the Android backend. Connect application actions after `RenderStudioUI` and review `codegen_report.txt` for coverage warnings. Size the windows for the target screen. On-screen keyboard and IME composition require additional integration.
