# Exportar a Vulkan

[English](VULKAN.en.md)

El destino **C++ / Vulkan** genera una aplicación Dear ImGui con GLFW para Windows y Linux. El proyecto exportado incluye su propio host. Puedes compilarlo en otro equipo con un toolchain C++ y Vulkan.

## Exportar y probar

1. Abre [Imágenes y entrada](../examples/vulkan-imagenes.json).
2. En **Código generado**, exporta C++, elige **Vulkan** e indica una carpeta nueva.
3. Configura, compila y ejecuta la prueba básica en esa carpeta.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target studio_export
ctest --test-dir build --output-on-failure
```

Abre `build/studio_export.exe` en Windows o `./build/studio_export` en Linux. Marca la casilla y redimensiona la ventana. Las imágenes PNG y JPEG deberían seguir visibles. `codegen_report.txt` relaciona avisos y líneas generadas con los IDs del diseño.

## Dependencias y diagnóstico

| Etapa | Qué hace falta | Qué revisar si falla |
| --- | --- | --- |
| Configuración | Encabezados Vulkan y biblioteca de enlace del cargador. | En Windows sirve el SDK de LunarG o los archivos de tu toolchain. En Ubuntu el paquete habitual es `libvulkan-dev`. |
| Ejecución | Cargador, controlador y superficie de presentación. | Si faltan extensiones de superficie, comprueba el backend de ventana y la sesión. Si el host no elige un dispositivo, revisa el controlador y su soporte de presentación. |
| SDK en otra carpeta | Ruta al SDK para CMake. | Configura con `"-DVulkan_ROOT=/ruta/al/sdk"` o indica **Ruta del SDK de Vulkan** en Run / Build. |

El host crea la superficie y el swapchain, y vuelve a preparar el destino de presentación al redimensionar. La exportación añade `studio_vulkan_images.hpp` cuando hay imágenes. Para usar el diseño con un host propio, conserva `ui.hpp` y adapta la creación y liberación de texturas al ciclo de tu renderizador.

[Referencia de FindVulkan](https://cmake.org/cmake/help/latest/module/FindVulkan.html) · [SDK de LunarG](https://vulkan.lunarg.com/sdk/home)

## Pruebas del repositorio

Desde la raíz, configura un build con GUI y pruebas activadas. Si el SDK está en una carpeta personalizada, agrega `-DSTUDIO_VULKAN_TEST_ROOT=...` a la configuración.

```sh
cmake -S . -B build/check-vulkan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DSTUDIO_TEST_VULKAN_EXPORT=ON
cmake --build build/check-vulkan --target verify_codegen verify_build
ctest --test-dir build/check-vulkan -R '^(GeneratedVulkanRuntime|NativeVulkanProjectBuildTest)$' --output-on-failure
```

- **GeneratedVulkanRuntime** comprueba el diagnóstico de SDK ausente en una configuración separada. Después verifica cinco fotogramas presentados, redimensionado, entrada de la casilla y comandos de dibujo PNG/JPEG.
- **NativeVulkanProjectBuildTest** usa Run / Build, rechaza una ruta de SDK inválida y comprueba compilación y arranque. Cambiar a OpenGL invalida el resultado anterior.

Si habilitas una capa de validación, indica cuál estuvo cargada y sus mensajes. Registra también el controlador y si Linux usó Wayland directo o XWayland. La prueba básica de una exportación de usuario verifica arranque. Las pruebas instrumentadas verifican además los comportamientos descritos arriba. Consulta [validación reproducible](VALIDATION.md).

En Linux, con `VK_LAYER_KHRONOS_validation` instalada, puedes repetir la
prueba instrumentada y ver qué capa cargó el loader:

```sh
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LOADER_DEBUG=layer \
  ctest --test-dir build/check-vulkan -R '^GeneratedVulkanRuntime$' -V
```

Busca `Insert instance layer "VK_LAYER_KHRONOS_validation"` y guarda la salida
si hay un error. El nombre del dispositivo y el backend aparecen en la línea
`Vulkan presented 5 frames`.
