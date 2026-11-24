# ImGui Studio · Android C++

Complemento opcional. Descomprime el ZIP y conserva la carpeta `Android-Cpp`.
En ImGui Studio selecciona C++ en Código generado, pulsa **Exportar Android**
y elige la carpeta que contiene `template.version`. Escribe una ruta nueva
para el proyecto exportado.

## Compilar el proyecto exportado

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
