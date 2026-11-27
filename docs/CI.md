# Comprobaciones de GitHub

[English](CI.en.md)

[Repository checks](../.github/workflows/checks.yml) se ejecuta en los pull requests y al subir cambios a `main`, cuando GitHub Actions está habilitado para el repositorio. Usa Ubuntu 24.04 con Ninja y Windows con Visual Studio 2022.

| Comprobación | Qué verifica |
| --- | --- |
| Documentación | Existencia de los enlaces internos y sus títulos. Comprueba que cada archivo de `src/` e `include/` aparezca en los dos mapas. |
| Núcleo | Compilación sin GUI, persistencia, edición del árbol y recuperación de sesiones. |
| Exportación | Generación de proyectos C++, Android y Apple a partir de los diseños de prueba. |

El flujo trabaja con permisos de lectura. Los proyectos generados requieren sus propias pruebas de ejecución. Para cambios de interfaz, paquetes o renderizado, adjunta la evidencia que corresponda según la [guía de validación](VALIDATION.md).

## Compilación gráfica bajo demanda

En **Actions → Desktop build → Run workflow**, elige una plataforma para comprobar el editor gráfico. [El flujo manual](../.github/workflows/desktop.yml) usa Release y las dependencias gráficas de cada sistema.

| Plataforma | Prueba configurada |
| --- | --- |
| Ubuntu | Compilación y arranque con Xvfb, X11 y OpenGL de software. |
| Windows | Compilación con MSVC y comprobación del archivo `imgui_studio.exe`. |
| macOS | Compilación y comprobación del ejecutable y del `Info.plist` dentro del `.app`. |

Para verificar Wayland, inicia el editor dentro de una sesión Wayland como se indica en [Validación reproducible](VALIDATION.md#cambios-gráficos). En macOS queda por comprobar la apertura de la ventana en una sesión gráfica.

## Imágenes de disco para macOS

Con el flujo en la rama principal, ejecuta **Actions → macOS packages → Run workflow**. Descarga los artefactos `ImGuiStudio-macOS-arm64` y `ImGuiStudio-macOS-x86_64` del resultado. Cada uno contiene el DMG para macOS 15 o posterior y su suma SHA-256. El flujo monta la imagen y comprueba la arquitectura, las bibliotecas, la licencia y la documentación del editor. La firma Developer ID, la notarización y el arranque gráfico se verifican aparte. Para publicar, adjunta cada DMG y su suma a la versión correspondiente en GitHub Releases. [Flujo de empaquetado](../.github/workflows/macos-package.yml)

## Repetir la comprobación de documentación

Desde la raíz, ejecuta `python tools/check_docs.py` o `python3 tools/check_docs.py`. El script señala enlaces relativos con rutas o títulos incorrectos. Comprueba que cada documento de raíz, `docs/` y `examples/` tenga su versión en el otro idioma y un enlace para cambiar de idioma. También comprueba los enlaces de los README de los complementos y dependencias y las plantillas de GitHub. Al añadir o mover archivos de código propio, actualiza ambos mapas y los capítulos de [referencia del código](code/README.md).

Consulta [los pasos de compilación y prueba](VALIDATION.md#cambios-de-modelo-y-exportación) para reproducir el núcleo en tu máquina.

La configuración remota se verifica en la pestaña **Actions** cuando se ejecuta en GitHub. Un resultado local solo cubre las herramientas y el entorno anotados en ese resultado.

El flujo manual [Apple export build](../.github/workflows/apple-exports.yml)
está dedicado a las [apps exportadas para macOS e iOS](APPLE_EXPORTS.md).
Compila los proyectos generados con Xcode y comprueba sus bundles.
