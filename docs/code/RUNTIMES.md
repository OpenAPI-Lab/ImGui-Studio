# Hosts y plantillas de exportación

[English](RUNTIMES.en.md)

Los hosts generados conectan la interfaz a una ventana, el backend gráfico y la entrada del destino. Esta parte del código se ejecuta en la aplicación del usuario.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [studio_codegen_vulkan.hpp](../../include/imguistudio/export/detail/studio_codegen_vulkan.hpp) | `vulkanMain` emite el punto de entrada con el tipo de estado y la ruta de imágenes si se necesitan. | Mantiene el host separado de la salida UI para poder elegir el renderizador al exportar. |
| [studio_vulkan.hpp](../../templates/studio_vulkan.hpp) | Aloja GLFW, dispositivo Vulkan, swapchain y el ciclo de presentación. | El cambio de tamaño puede exigir recrear recursos asociados a la presentación. Verifica que se destruyan después de su uso. |
| [studio_vulkan_images.hpp](../../templates/studio_vulkan_images.hpp) | Sube texturas y administra las vistas y descriptores que usa ImGui. | La textura depende del dispositivo y de la sincronización del backend. Libérala antes de destruirlo. |
| [main.cpp](../../templates/android/main.cpp) | Aloja NativeActivity, EGL, entrada y render OpenGL ES. | La superficie Android cambia durante el ciclo de vida. Mantén su inicialización y retirada coordinadas con recursos de ImGui. |
| [AndroidManifest.xml](../../templates/android/AndroidManifest.xml) | Declara NativeActivity y la configuración Android de la app. | El nombre de la biblioteca y la actividad deben corresponder al código construido. |
| [build.gradle](../../templates/android/build.gradle) | Fija el plugin, niveles SDK, NDK, arquitecturas y build CMake. | Coordina Gradle con la compilación nativa. Actualiza también la guía si cambian los requisitos. |
| [settings.gradle](../../templates/android/settings.gradle) | Configura repositorios y nombre del proyecto Gradle. | Permite configurar el exportado de forma independiente del editor. |
| [CMakeLists.txt](../../templates/android/CMakeLists.txt) | Configura Dear ImGui, el backend Android y la biblioteca nativa. | Los recursos de imagen pueden cambiar las dependencias. Mantén sus versiones y hashes coherentes. |
| [template.version](../../templates/android/template.version) | Identifica el formato del pack Android. | El exportador lo valida antes de usar los archivos del pack. |
| [CMakeLists.txt](../../templates/apple/CMakeLists.txt) | Selecciona SDK, frameworks y propiedades del bundle macOS o iOS. | El código compartido requiere el entorno y los frameworks de su destino. |
| [main.mm](../../templates/apple/main.mm) | Aloja la UI con AppKit o UIKit y el bucle Metal. | Mantiene entrada y render de cada plataforma separados del código de diseño. |
| [studio_metal_images.hpp](../../templates/apple/studio_metal_images.hpp) | Carga y mantiene texturas Metal usadas por los recursos generados. | La vida de una textura debe cubrir los comandos de dibujo que la usan. |
| [Info.plist](../../templates/apple/macos/Info.plist) | Describe el bundle macOS. | El identificador y ejecutable deben coincidir con la configuración generada. |
| [Info.plist](../../templates/apple/ios/Info.plist) | Describe el bundle iOS. | Conserva los campos de arranque y configuración para el destino UIKit. |
| [template.version](../../templates/apple/macos/template.version) | Identifica el formato del pack macOS. | Permite al exportador verificar el destino del pack antes de copiarlo. |
| [template.version](../../templates/apple/ios/template.version) | Identifica el formato del pack iOS. | Evita seleccionar metadatos de escritorio para un proyecto móvil. |
| [title_bar_help.hpp](../../examples/title-bar-action/title_bar_help.hpp) | Conecta el pulso del botón de título con una ventana de ayuda que pertenece al host. | El pulso dura un fotograma. Conserva el booleano de la ventana entre llamadas a render. |

## Verificación

Ejecuta `GeneratedVulkanRuntime` y `NativeVulkanProjectBuildTest` para Vulkan. Las pruebas `AndroidCppExportGenerationTest` y `AppleCppExportGenerationTest` comprueban generación. Repite el arranque, la entrada y el ciclo de vida con el SDK y el dispositivo o simulador correspondiente cuando cambies un host. [Vulkan](../VULKAN.md) · [Android](../ANDROID.md) · [Apple](../APPLE_EXPORTS.md)

