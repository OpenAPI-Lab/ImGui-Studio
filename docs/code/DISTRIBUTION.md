# Código de compilación y empaquetado

[English](DISTRIBUTION.en.md)

CMake conecta fuentes, dependencias y pruebas. CPack añade los archivos de distribución. Las comprobaciones de cada plataforma verifican artefactos desde su destino final.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [CMakeLists.txt](../../CMakeLists.txt) | Declara versión, requisito de CMake y punto de entrada. | Hace que el repositorio pueda configurarse desde su raíz. |
| [ImGuiStudio.cmake](../../cmake/ImGuiStudio.cmake) | Registra objetivos, dependencias fijadas, complementos y CTest. | La opción sin GUI mantiene el núcleo independiente de la pila gráfica. |
| [StudioPackaging.cmake](../../packaging/StudioPackaging.cmake) | Instala ejecutable, licencias, guías y recursos. Define generadores CPack. | Declara bibliotecas GLFW cargadas en ejecución para que el DEB pueda resolverlas. |
| [make_icons.py](../../packaging/make_icons.py) | Genera los iconos que espera cada tipo de paquete. | La compilación prepara los recursos reproducibles antes de instalar. |
| [imgui-studio.desktop](../../packaging/imgui-studio.desktop) | Declara la entrada de menú de Linux. | El comando y el icono deben coincidir con los nombres instalados. |
| [verify_package.py](../../packaging/verify_package.py) | Comprueba sumas, carga instalada y arranque del ZIP o DEB. | Compara la documentación con su fuente. El DEB se instala y retira bajo una raíz aislada. |
| [verify_windows_installer.ps1](../../packaging/verify_windows_installer.ps1) | Verifica preflight y, en modo completo, instalación, accesos, registro y retirada. | El preflight detecta conflictos antes de tocar una instalación. El ciclo real necesita el entorno adecuado. |
| [studio_status_source.hpp.in](../../packaging/studio_status_source.hpp.in) | Incrusta la implementación de los controles de estado. | El exportador puede copiar el helper sin depender de archivos del editor en ejecución. |
| [studio_knobs_sources.hpp.in](../../packaging/studio_knobs_sources.hpp.in) | Incrusta cabecera, implementación y licencia fijadas de la perilla. | Las comprobaciones del catálogo y las exportaciones usan la revisión revisada. |
| [studio_vulkan_source.hpp.in](../../packaging/studio_vulkan_source.hpp.in) | Incrusta el host Vulkan y su adaptador de imágenes. | Los proyectos emitidos pueden construirse de forma independiente. |
| [check_docs.py](../../tools/check_docs.py) | Comprueba enlaces, traducciones e inventario de código. | Detecta rutas movidas y archivos con documentación ausente en CI. |
| [checks.yml](../../.github/workflows/checks.yml) | Ejecuta el verificador del repositorio y pruebas centrales. | Las contribuciones se verifican sin una sesión gráfica. |
| [desktop.yml](../../.github/workflows/desktop.yml) | Compila el editor gráfico para un host elegido. | Permite verificar configuración de plataforma de forma controlada. |
| [apple-exports.yml](../../.github/workflows/apple-exports.yml) | Compila proyectos exportados macOS e iOS con Xcode. | Comprueba la aplicación que genera Studio y su bundle. |
| [macos-package.yml](../../.github/workflows/macos-package.yml) | Genera y comprueba el DMG del editor para Intel y Apple Silicon. | Comprueba el contenido real de la imagen mediante montaje y validación del bundle. |

## Verificación

Ejecuta `python tools/check_docs.py` y la [prueba del paquete](../BUILD_AND_TEST.md#distribución) del host. Si cambias un área de instalación, comprueba su contenido en el artefacto generado y que el editor arranque desde él. [Flujos de GitHub](../CI.md)

