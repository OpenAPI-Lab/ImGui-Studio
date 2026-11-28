# ImGui Studio

[English](README.en.md)

Editor visual de interfaces con Dear ImGui. La edición de escritorio permite
diseñar ventanas y widgets, previsualizar la interfaz y exportar el proyecto.
El editor está escrito en C++17 y usa GLFW y OpenGL.

## Instalar una versión

Para Windows x64, Ubuntu 24.04 x64 y Debian 13 x64, consulta la
[guía de instalación y verificación](docs/INSTALLATION.md). El DMG de macOS
está pendiente de compilación y verificación.

## Usar ImGui Studio

Sigue el [manual del editor](docs/WORKFLOW.md) para aprender qué hace cada
panel y construir un diseño paso a paso. Consulta el
[catálogo de widgets](docs/WIDGETS.md) para elegir controles.
El manual también explica [cómo abrir y guardar con el selector](docs/WORKFLOW.md#abrir-y-guardar-con-el-selector) y [cómo cambiar la apariencia de Studio](docs/WORKFLOW.md#personalizar-la-apariencia-de-studio).

## Estructura del repositorio

| Ruta | Contenido |
| --- | --- |
| `src/app/` y `src/editor/` | Arranque, interfaz, lienzo y vista previa. |
| `src/widgets/` y `src/services/` | Gráficos, imágenes, catálogo y conexiones del asistente. |
| `include/imguistudio/` | Cabeceras internas agrupadas por proyecto, widgets, localización, exportación, compilación y servicios. |
| `templates/` | Plantillas de exportación y complementos por plataforma |
| `tests/` y `examples/` | Pruebas, proyectos de muestra y fixtures |
| `cmake/` y `packaging/` | Configuración de compilación, distribución y verificación de instaladores |
| `docs/` | Guías en español e inglés y referencia técnica |
| `third_party/` | Fuentes de terceros y avisos de licencia |
| `tools/` | Herramientas de mantenimiento del repositorio. |

El [mapa de fuentes](docs/SOURCE_MAP.md) indica dónde cambiar cada función.

Los [cambios de la versión](CHANGELOG.md) incluyen las plataformas verificadas
y los límites conocidos. La [cobertura de idiomas](docs/LOCALIZATION.md)
describe los idiomas integrados y los paquetes comunitarios.

## Compilar el editor

Usa CMake 3.16 o posterior, Ninja y un compilador C++17. La primera
configuración descarga GLFW, Dear ImGui y stb con hashes fijados.

- **Windows** — usa MSYS2 UCRT64 con GCC o una terminal de desarrollo de
  Visual Studio con MSVC. CMake, Ninja y el compilador deben estar en el PATH.
- **Ubuntu/Debian** — instala `build-essential`, `cmake`, `ninja-build`,
  `pkg-config`, `libgl-dev`, `xorg-dev`, `libwayland-dev`,
  `wayland-protocols` y `libxkbcommon-dev`.
- **macOS** — usa Clang y el SDK de macOS de las herramientas de línea de
  comandos de Xcode. Instala también CMake y Ninja.

Para activar las descargas del catálogo y las conexiones del asistente,
instala las bibliotecas de desarrollo de libcurl 7.85 o posterior.

Desde la raíz, ejecuta:

```sh
cmake -S . -B build/native -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --target imgui_studio
```

| Sistema | Salida con Ninja |
| --- | --- |
| Windows | `build/native/bin/imgui_studio.exe` |
| Linux | `build/native/bin/imgui_studio` |
| macOS | `build/native/bin/ImGui Studio.app` |

El editor se ejecuta en una sesión gráfica. En WSL usa WSLg.

## Exportación y paquetes

- Las salidas de escritorio incluyen C++, C, Python, C#, Java y Rust. C++
  ofrece OpenGL y una plantilla Vulkan para Windows y Linux.
- Android, macOS e iOS tienen complementos C++ opcionales, con instrucciones
  de compilación en cada plantilla.
- Consulta la [guía de exportación](docs/EXPORTING.md) para integrar el código
  y la [guía de compilación](docs/BUILD_AND_TEST.md) para generar ZIP, EXE y DEB.

## Documentación

| Guía | Qué explica |
| --- | --- |
| [Instalar ImGui Studio](docs/INSTALLATION.md) | Qué archivo elegir, cómo comprobar su suma y cómo instalarlo o retirarlo. |
| [Manual del editor](docs/WORKFLOW.md) | Inicio, paneles, edición, vista previa, guardado, ejecución y ejemplos. |
| [Lienzo y disposición](docs/CANVAS.md) | Flujo, posición libre, gestos, reglas del árbol y pruebas. |
| [Barras de título y acciones](docs/TITLE_BAR.md) | Controles integrados, menús y manejo de eventos en el host. |
| [Guardado y recuperación](docs/RECOVERY.md) | Versiones, restauración, recuperación y conflictos de guardado. |
| [Diagnósticos de compilación](docs/BUILD_DIAGNOSTICS.md) | Fases, navegación al widget y datos para reportar un fallo. |
| [Catálogo de widgets](docs/WIDGETS.md) | Qué hace cada control del registro y cuándo utilizarlo. |
| [Compilar, probar y empaquetar](docs/BUILD_AND_TEST.md) | Dependencias, opciones CMake y pruebas por área. |
| [Capacidades](docs/CAPABILITIES.md) | Funciones disponibles, plataformas verificadas y pendientes. |
| [Arquitectura](docs/ARCHITECTURE.md) y [mapa de fuentes](docs/SOURCE_MAP.md) | Qué hace cada parte, por qué existe y dónde modificarla. |
| [Cómo funciona el código](docs/code/README.md) | Contratos por archivo, ejemplos y pruebas que protegen cada comportamiento. |
| [Formato de proyecto](docs/PROJECT_FORMAT.md) | JSON, historial, concurrencia y recuperación. |
| [Exportación e integración](docs/EXPORTING.md) | Código generado, herramientas y complementos por destino. |
| [Vulkan](docs/VULKAN.md) | Dependencias, exportación de imágenes y pruebas de presentación. |
| [Android](docs/ANDROID.md) | Preparación del complemento, APK y comprobaciones en emulador. |
| [macOS e iOS](docs/APPLE_EXPORTS.md) | Complementos Metal, Xcode y compilación para simulador. |
| [Run / Build y diagnósticos](docs/RUN_BUILD.md) | Perfiles, ciclo de compilación y errores por widget. |
| [Biblioteca y catálogo](docs/WIDGET_LIBRARY.md) | Instancias, almacenamiento y validación de extensiones. |
| [Asistente y proveedores](docs/ASSISTANT.md) | Conexión, credenciales y aplicación de propuestas. |
| [Validación reproducible](docs/VALIDATION.md) | Comandos de pruebas y datos que debe incluir un reporte. |
| [Comprobaciones de GitHub](docs/CI.md) | Qué revisa la integración continua y cómo repetir las comprobaciones. |
| [Ampliar el editor](docs/EXTENDING.md) | Cómo añadir un widget, un exportador o una traducción. |
| [Referencia técnica](docs/REFERENCE.md) | Detalles de controles, diagnósticos, pruebas y requisitos. |

## Contribuir

Consulta [CONTRIBUTING](CONTRIBUTING.md). Para datos sensibles, utiliza
el flujo indicado en [SECURITY](SECURITY.md).

## Licencia

El código propio usa la [licencia MIT](LICENSE). Las dependencias conservan
[sus licencias y avisos](third_party/README.md).
