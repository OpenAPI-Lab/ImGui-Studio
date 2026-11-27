# Exportar a macOS e iOS

[English](APPLE_EXPORTS.en.md)

Desde ImGui Studio puedes generar proyectos C++ para macOS e iOS usando sus
complementos. En macOS, el host usa AppKit. En iOS, usa UIKit y escenas con
entrada de un dedo. Los dos renderizan con Metal.

## Preparar los complementos

```sh
cmake -S . -B build/packs -G Ninja -DBUILD_STUDIO_GUI=OFF -DBUILD_CORE_TEST=OFF
cmake --build build/packs --target studio_macos_pack studio_ios_pack
```

En `build/packs/optional-packs/` encontrarás los ZIP. Descomprime el destino
que quieras usar. En **Código generado → C++**, elige **Exportar macOS** o
**Exportar iOS**, selecciona la carpeta con `template.version` e indica una
carpeta de salida nueva.

El proyecto generado contiene `ui.hpp`, el host Objective-C++ en `main.mm`,
CMake, `Info.plist` y `codegen_report.txt`. El host carga las texturas del
diseño y las libera al desmontar el renderizador. Conecta las acciones de
tu aplicación después de `RenderStudioUI`.

## Compilar con Xcode

Se necesita una Mac con Xcode, los SDK del destino y CMake 3.24 o posterior
para compilar el proyecto generado. Desde la carpeta exportada de macOS:

```sh
cmake -S . -B build-macos -G Xcode
cmake --build build-macos --config Debug --target studio_export
open build-macos/Debug/studio_export.app
```

Desde la carpeta exportada de iOS, configura el SDK del simulador:

```sh
cmake -S . -B build-simulator -G Xcode -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build build-simulator --config Debug --target studio_export
open build-simulator/StudioExport.xcodeproj
```

En Xcode, elige el esquema `studio_export` y un simulador. Para probar en
un dispositivo, configura un build aparte con `CMAKE_OSX_SYSROOT=iphoneos`,
un identificador propio en `STUDIO_BUNDLE_ID` y un equipo de firma en Xcode.
El teclado en pantalla y la composición IME en iOS requieren integración
adicional. [Toolchains Apple en CMake](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-ios-tvos-visionos-or-watchos)

## Qué cubre cada prueba

`AppleCppExportGenerationTest` descomprime ambos ZIP y comprueba que los
proyectos se generan a partir del mismo diseño. `AppleCppExportBuildTest`
puede habilitarse con `-DSTUDIO_TEST_APPLE_BUILD=ON` en una Mac y usa Xcode
para compilar el proyecto macOS y el de iOS Simulator. Verifica la presencia
de ambos ejecutables y la sintaxis de sus `Info.plist`.

```sh
cmake -S . -B build/check-apple -G Ninja -DBUILD_STUDIO_GUI=OFF -DSTUDIO_TEST_APPLE_BUILD=ON
cmake --build build/check-apple --target verify_codegen
ctest --test-dir build/check-apple -R '^AppleCppExportBuildTest$' --output-on-failure
```

Ejecuta estos comandos desde la raíz de ImGui Studio para compilar una
exportación de prueba para cada plataforma.

El [flujo manual Apple export build](../.github/workflows/apple-exports.yml)
configura esa compilación en un runner macOS. La generación ya se ha
verificado en Windows y WSL. Quedan pendientes el primer build en Xcode,
el arranque gráfico, las pruebas táctiles y la firma para dispositivo.
