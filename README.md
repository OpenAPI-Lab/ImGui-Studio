# ImGui Studio

Editor visual de interfaces con Dear ImGui. La edición de escritorio permite
diseñar ventanas y widgets, previsualizar la interfaz y exportar el proyecto.
El editor está escrito en C++17 y usa GLFW y OpenGL.

## Estructura del repositorio

| Ruta | Contenido |
| --- | --- |
| `native/src/` y `native/include/` | Editor nativo, modelo de proyecto y generadores |
| `native/templates/` | Plantillas de exportación y complementos por plataforma |
| `native/tests/` y `native/examples/` | Pruebas, proyectos de muestra y fixtures |
| `native/packaging/` | Empaquetado y verificación de instaladores |
| `native/third_party/` | Fuentes de terceros y avisos de licencia |

Los [cambios de la versión](CHANGELOG.md) incluyen las plataformas verificadas
y los límites conocidos. La [cobertura de idiomas](native/COBERTURA_IDIOMAS.md)
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
- Consulta la [documentación del editor](native/README.md) para ver las
  pruebas, la cobertura de cada exportador y cómo generar ZIP, EXE y DEB.

## Licencia

El código propio usa la [licencia MIT](LICENSE). Las dependencias conservan
[sus licencias y avisos](native/third_party/README.md).
