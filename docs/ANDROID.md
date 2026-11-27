# Exportar y probar una aplicación Android

[English](ANDROID.en.md)

ImGui Studio genera un proyecto C++ con Gradle, NativeActivity y OpenGL ES 3.
El complemento aporta los archivos del host. El NDK y el resto de las
herramientas se necesitan al compilar el proyecto generado.

## Preparar y exportar

Desde la raíz del repositorio, genera el complemento en un directorio separado:

```sh
cmake -S . -B build/packs -G Ninja -DBUILD_STUDIO_GUI=OFF -DBUILD_CORE_TEST=OFF
cmake --build build/packs --target studio_android_pack
```

El resultado queda en `build/packs/optional-packs/`. Descomprime el ZIP.
Abre [Imágenes y entrada](../examples/vulkan-imagenes.json) en el editor.
En **Código generado → C++ → Exportar Android**, indica la carpeta
`Android-Cpp` que contiene `template.version` y una ruta nueva de salida.

## Compilar el APK

| Herramienta | Versión de la plantilla |
| --- | --- |
| JDK | 17 o 21 |
| Gradle y Android Gradle Plugin | Gradle 8.9, AGP 8.7.3 |
| Android SDK | Platform 35 y Build Tools 34.0.0 |
| Herramientas nativas | NDK 27.2.12479018 y CMake 3.22.1 del SDK |

Abre el proyecto generado en Android Studio o ejecuta `gradle assembleDebug`
desde su raíz con el SDK configurado. El APK queda en
`build/outputs/apk/debug/`. El ID predeterminado es
`org.imguistudio.exported`. Para un proyecto propio, pásale a Gradle
`-PstudioApplicationId=org.ejemplo.app`. Ajusta la etiqueta visible en
`AndroidManifest.xml`.

## Ejecutar y reportar

Usa un emulador x86-64 o un dispositivo ARM64 con OpenGL ES 3. Registra el
modelo, API y arquitectura. Si usas ADB, consulta `adb devices` y elige
explícitamente el equipo de prueba.

```sh
adb -s SERIAL install -r build/outputs/apk/debug/StudioExportAndroid-debug.apk
adb -s SERIAL shell am start -W -n org.imguistudio.exported/android.app.NativeActivity
```

Comprueba el arranque, las imágenes PNG/JPEG, el toque, la rotación y la
reanudación después de enviar la app a segundo plano. Si cambiaste el ID,
usa ese mismo ID al iniciar. Consulta `codegen_report.txt` para los avisos
del diseño y el log de Android para errores del host. La plantilla necesita
integración adicional del teclado en pantalla y la composición IME.

## Qué cubren las pruebas

`AndroidCppExportGenerationTest` descomprime el ZIP producido por CMake
y genera un proyecto con imágenes y una perilla. Comprueba los archivos
esperados, la conservación de una exportación existente y el rechazo de
un complemento ausente. La compilación del APK y el arranque se prueban
aparte. Consulta [las capacidades verificadas](CAPABILITIES.md) y la
[plantilla Gradle](../templates/android/build.gradle).
