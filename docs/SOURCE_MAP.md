# Mapa de fuentes

[English](SOURCE_MAP.en.md)

La [arquitectura](ARCHITECTURE.md) explica por qué se organizan así las capas. Las tablas identifican dónde hacer un cambio y qué código se ocupa del comportamiento.

## Raíz y distribución

| Archivo o carpeta | Responsabilidad |
| --- | --- |
| [`CMakeLists.txt`](../CMakeLists.txt) | Entrada pública para configurar desde la raíz. |
| [`tools/check_docs.py`](../tools/check_docs.py) | Comprueba enlaces de la documentación y el inventario de fuentes. |
| [`.github/workflows/checks.yml`](../.github/workflows/checks.yml) | Ejecuta comprobaciones de repositorio en Windows y Linux. |
| [`.github/workflows/desktop.yml`](../.github/workflows/desktop.yml) | Compila y comprueba el editor gráfico bajo demanda por plataforma. |
| [`.github/workflows/apple-exports.yml`](../.github/workflows/apple-exports.yml) | Compila con Xcode exportaciones de prueba para macOS e iOS Simulator. |
| [`cmake/ImGuiStudio.cmake`](../cmake/ImGuiStudio.cmake) | Define objetivos, dependencias fijadas, plantillas, pruebas y configuración del ejecutable. |
| [`packaging/StudioPackaging.cmake`](../packaging/StudioPackaging.cmake) | Instalación de binarios, documentación, licencias, iconos y CPack. |
| [`packaging/make_icons.py`](../packaging/make_icons.py) | Genera iconos para empaquetado. |
| [`packaging/imgui-studio.desktop`](../packaging/imgui-studio.desktop) | Entrada del editor en los menús de escritorio Linux. |
| [`packaging/verify_package.py`](../packaging/verify_package.py) | Comprueba SHA-256, licencias, carga del ZIP y ciclo aislado del DEB. |
| [`packaging/verify_windows_installer.ps1`](../packaging/verify_windows_installer.ps1) | Valida el EXE, accesos, registro y retirada con un ciclo de instalación. |
| [`packaging/studio_knobs_sources.hpp.in`](../packaging/studio_knobs_sources.hpp.in) | Incrusta fuentes y licencia fijadas de la perilla. |
| [`packaging/studio_status_source.hpp.in`](../packaging/studio_status_source.hpp.in) | Incrusta controles de estado para los proyectos exportados. |
| [`packaging/studio_vulkan_source.hpp.in`](../packaging/studio_vulkan_source.hpp.in) | Incrusta la implementación del host Vulkan y sus imágenes. |

## Editor, recursos y servicios

| Archivo o carpeta | Responsabilidad |
| --- | --- |
| [`src/editor/studio_editor_appearance.hpp`](../src/editor/studio_editor_appearance.hpp) | Paletas, redondeo y preferencias del editor. |
| [`src/editor/studio_project_file_picker.hpp`](../src/editor/studio_project_file_picker.hpp) | Navegación de carpetas, lista de proyectos y destino de guardado. |
| [`src/app/main.cpp`](../src/app/main.cpp) | Arranque, ciclo de fotogramas y pruebas rápidas de ventana y fuentes. |
| [`src/editor/studio_ui.hpp`](../src/editor/studio_ui.hpp) | Estado de StudioApp, edición, ventanas y servicios del editor. |
| [`src/editor/studio_ui.cpp`](../src/editor/studio_ui.cpp) | Navegación, diálogos, archivo, biblioteca, exportación y panel Build. |
| [`src/editor/studio_canvas.hpp`](../src/editor/studio_canvas.hpp) | Contratos del lienzo y de sus observaciones y gestos. |
| [`src/editor/studio_canvas.cpp`](../src/editor/studio_canvas.cpp) | Coordenadas, arrastre, selección, jerarquía y confirmación de ediciones. |
| [`src/editor/studio_preview.hpp`](../src/editor/studio_preview.hpp) | Estado y datos observables de vista previa. |
| [`src/editor/studio_preview.cpp`](../src/editor/studio_preview.cpp) | Dibuja controles, presenta propiedades y administra interacción temporal. |
| [`src/app/studio_fonts.hpp`](../src/app/studio_fonts.hpp) | Carga fuentes del host o del directorio de estado y arma el atlas. |
| [`src/widgets/studio_images.hpp`](../src/widgets/studio_images.hpp) | Contrato de caché de texturas de la vista previa. |
| [`src/widgets/studio_images.cpp`](../src/widgets/studio_images.cpp) | Decodifica recursos y gestiona texturas del editor. |
| [`src/widgets/studio_chart2d.hpp`](../src/widgets/studio_chart2d.hpp) | Renderización de gráficas 2D en la vista previa. |
| [`src/widgets/studio_chart3d.hpp`](../src/widgets/studio_chart3d.hpp) | Renderización de gráficas 3D en la vista previa. |
| [`src/services/studio_extensions.hpp`](../src/services/studio_extensions.hpp) | Tipos y entradas del catálogo que consume el editor. |
| [`src/services/studio_extensions.cpp`](../src/services/studio_extensions.cpp) | Verifica el contenido fijado y conserva el estado de instalación. |
| [`src/services/studio_assistant_panel.hpp`](../src/services/studio_assistant_panel.hpp) | Estado y contrato del panel y de la conversación. |
| [`src/services/studio_assistant_panel.cpp`](../src/services/studio_assistant_panel.cpp) | Solicitudes, ajustes, diálogos de claves y revisión de propuestas. |
| [`src/services/studio_assistant_http.hpp`](../src/services/studio_assistant_http.hpp) | Interfaz de transporte que consume el asistente. |
| [`src/services/studio_assistant_http.cpp`](../src/services/studio_assistant_http.cpp) | Conexión HTTP con libcurl y manejo del resultado. |
| [`src/services/studio_assistant_vault.hpp`](../src/services/studio_assistant_vault.hpp) | Interfaz del almacén de credenciales. |
| [`src/services/studio_assistant_vault.cpp`](../src/services/studio_assistant_vault.cpp) | Adaptadores para WinCred, Secret Service y Keychain. |

## Modelo, archivo y operaciones

| Archivo o carpeta | Responsabilidad |
| --- | --- |
| [`include/imguistudio/project/studio_types.hpp`](../include/imguistudio/project/studio_types.hpp) | Define proyecto, nodo, propiedades, tipo y estado persistente. |
| [`include/imguistudio/project/studio_widget_registry.inc`](../include/imguistudio/project/studio_widget_registry.inc) | Relaciona tipos con nombres y metadatos del catálogo interno. |
| [`include/imguistudio/project/studio_serialization.hpp`](../include/imguistudio/project/studio_serialization.hpp) | Convierte JSON y modelo, valida límites y conserva campos adicionales. |
| [`include/imguistudio/project/studio_document_edit.hpp`](../include/imguistudio/project/studio_document_edit.hpp) | Operaciones de insertar, mover, ordenar y posicionar con validación. |
| [`include/imguistudio/project/studio_project_io.hpp`](../include/imguistudio/project/studio_project_io.hpp) | Lecturas acotadas, guardado temporal y verificación de cambios. |
| [`include/imguistudio/project/studio_snapshots.hpp`](../include/imguistudio/project/studio_snapshots.hpp) | Versiones, comparación y política de conservación. |
| [`include/imguistudio/project/studio_session_store.hpp`](../include/imguistudio/project/studio_session_store.hpp) | Checkpoints, historial durable, locks y directorio de estado. |
| [`include/imguistudio/widgets/studio_widget_library.hpp`](../include/imguistudio/widgets/studio_widget_library.hpp) | Formato y almacenamiento de widgets reutilizables. |
| [`include/imguistudio/widgets/studio_window_appearance.hpp`](../include/imguistudio/widgets/studio_window_appearance.hpp) | Campos y valores para personalización de ventanas. |
| [`include/imguistudio/widgets/studio_image_data.hpp`](../include/imguistudio/widgets/studio_image_data.hpp) | Valida y maneja recursos de imagen embebidos en el documento. |
| [`include/imguistudio/widgets/studio_numeric_format.hpp`](../include/imguistudio/widgets/studio_numeric_format.hpp) | Restringe formatos numéricos usados por controles. |
| [`include/imguistudio/widgets/studio_knob.hpp`](../include/imguistudio/widgets/studio_knob.hpp) | Configuración de la perilla compartida por editor y generador. |
| [`include/imguistudio/widgets/studio_status_widgets.hpp`](../include/imguistudio/widgets/studio_status_widgets.hpp) | Primitivas C++ de interruptor, spinner y progreso circular. |
| [`include/imguistudio/localization/studio_locale.hpp`](../include/imguistudio/localization/studio_locale.hpp) | Carga catálogos, selecciona idioma y resuelve claves. |
| [`include/imguistudio/localization/studio_locale_strings.inc`](../include/imguistudio/localization/studio_locale_strings.inc) | Claves y textos base de español e inglés. |
| [`include/imguistudio/localization/studio_locale_extra.inc`](../include/imguistudio/localization/studio_locale_extra.inc) | Traducciones de francés, coreano, japonés y chino simplificado. |
| [`include/imguistudio/services/studio_assistant_api.hpp`](../include/imguistudio/services/studio_assistant_api.hpp) | Modelos, reglas de URL y formatos por proveedor. |
| [`include/imguistudio/services/studio_assistant_proposal.hpp`](../include/imguistudio/services/studio_assistant_proposal.hpp) | Valida una propuesta y permite aplicarla como edición del documento. |
| [`include/imguistudio/build/studio_build_profiles.hpp`](../include/imguistudio/build/studio_build_profiles.hpp) | Formato y persistencia de perfiles de compilación por proyecto. |
| [`include/imguistudio/build/studio_build.hpp`](../include/imguistudio/build/studio_build.hpp) | Coordina exportación temporal, Build, Run y prueba de arranque. |
| [`include/imguistudio/build/studio_build_diagnostics.hpp`](../include/imguistudio/build/studio_build_diagnostics.hpp) | Interpreta diagnósticos y enlaza archivos, líneas y nodos. |
| [`include/imguistudio/build/studio_process.hpp`](../include/imguistudio/build/studio_process.hpp) | Subprocesos con argumentos separados, salida capturada y control de cancelación. |

## Generadores y plantillas

| Archivo o carpeta | Responsabilidad |
| --- | --- |
| [`include/imguistudio/export/studio_codegen.hpp`](../include/imguistudio/export/studio_codegen.hpp) | Punto de inclusión del generador principal. |
| [`include/imguistudio/export/studio_codegen_native.hpp`](../include/imguistudio/export/studio_codegen_native.hpp) | Emisión de interfaz nativa, estructura de estado y diagnósticos. |
| [`include/imguistudio/export/studio_cpp_export.hpp`](../include/imguistudio/export/studio_cpp_export.hpp) | Prepara el proyecto C++ y escribe archivos en un destino nuevo. |
| [`include/imguistudio/export/studio_c_export.hpp`](../include/imguistudio/export/studio_c_export.hpp) | Scaffold C11, dependencias cimgui y runtime de ejemplo. |
| [`include/imguistudio/export/studio_python_export.hpp`](../include/imguistudio/export/studio_python_export.hpp) | Programa, recursos y dependencias CPython/imgui-bundle. |
| [`include/imguistudio/export/studio_csharp_export.hpp`](../include/imguistudio/export/studio_csharp_export.hpp) | Proyecto .NET 8, interfaz ImGui.NET y recursos. |
| [`include/imguistudio/export/studio_java_export.hpp`](../include/imguistudio/export/studio_java_export.hpp) | Clases Java y proyecto de compilación, bindings y recursos. |
| [`include/imguistudio/export/studio_rust_export.hpp`](../include/imguistudio/export/studio_rust_export.hpp) | Proyecto Cargo, estado Rust y puentes cuando se necesitan. |
| [`include/imguistudio/export/detail/studio_codegen_vulkan.hpp`](../include/imguistudio/export/detail/studio_codegen_vulkan.hpp) | Genera el punto de entrada que conecta GLFW con Vulkan. |
| [`templates/studio_vulkan.hpp`](../templates/studio_vulkan.hpp) | Host Vulkan, ciclo de superficie y presentación. |
| [`templates/studio_vulkan_images.hpp`](../templates/studio_vulkan_images.hpp) | Subida y ciclo de vida de texturas Vulkan. |
| [`include/imguistudio/export/studio_android_export.hpp`](../include/imguistudio/export/studio_android_export.hpp) | Valida y combina el complemento Android con el diseño. |
| [`templates/android`](../templates/android) | Descriptor, actividad, NDK, Gradle y host OpenGL ES. |
| [`include/imguistudio/export/studio_apple_export.hpp`](../include/imguistudio/export/studio_apple_export.hpp) | Valida el complemento y ensambla proyecto macOS/iOS. |
| [`templates/apple`](../templates/apple) | Host Metal común, adaptador de texturas y descriptores por sistema. |
| [`include/imguistudio/export/detail/studio_codegen_titlebar.hpp`](../include/imguistudio/export/detail/studio_codegen_titlebar.hpp) | Ayudas de barra de título en las exportaciones nativas. |
| [`include/imguistudio/export/detail/studio_managed_titlebar.hpp`](../include/imguistudio/export/detail/studio_managed_titlebar.hpp) | Representación compartida de controles de título para bindings. |
| [`include/imguistudio/export/detail/studio_codegen_image.hpp`](../include/imguistudio/export/detail/studio_codegen_image.hpp) | Ayudas y adaptadores de imagen para fuentes generadas. |
| [`include/imguistudio/export/detail/studio_managed_images.hpp`](../include/imguistudio/export/detail/studio_managed_images.hpp) | Soporte compartido para imágenes en bindings. |
| [`include/imguistudio/export/detail/studio_images_java_rust.hpp`](../include/imguistudio/export/detail/studio_images_java_rust.hpp) | Adaptadores de imagen de los destinos Java y Rust. |
| [`include/imguistudio/export/detail/studio_codegen_animation.hpp`](../include/imguistudio/export/detail/studio_codegen_animation.hpp) | Emisión de ayudas para animaciones nativas. |
| [`include/imguistudio/export/detail/studio_managed_animation.hpp`](../include/imguistudio/export/detail/studio_managed_animation.hpp) | Parámetros y lógica compartida de animación entre bindings. |
| [`include/imguistudio/export/detail/studio_animation_java.hpp`](../include/imguistudio/export/detail/studio_animation_java.hpp) | Implementación generada de animaciones Java. |
| [`include/imguistudio/export/detail/studio_animation_rust.hpp`](../include/imguistudio/export/detail/studio_animation_rust.hpp) | Implementación generada de animaciones Rust. |
| [`include/imguistudio/export/detail/studio_codegen_chart2d.hpp`](../include/imguistudio/export/detail/studio_codegen_chart2d.hpp) | Ayudas C/C++ generadas para gráficas 2D. |
| [`include/imguistudio/export/detail/studio_codegen_chart3d.hpp`](../include/imguistudio/export/detail/studio_codegen_chart3d.hpp) | Ayudas C/C++ generadas para gráficas 3D. |
| [`include/imguistudio/export/detail/studio_chart2d_csharp.hpp`](../include/imguistudio/export/detail/studio_chart2d_csharp.hpp) | Render generado de gráficas 2D para C#. |
| [`include/imguistudio/export/detail/studio_chart3d_csharp.hpp`](../include/imguistudio/export/detail/studio_chart3d_csharp.hpp) | Render generado de gráficas 3D para C#. |
| [`include/imguistudio/export/detail/studio_titlebar_csharp.hpp`](../include/imguistudio/export/detail/studio_titlebar_csharp.hpp) | Soporte de título generado para C#. |
| [`include/imguistudio/export/detail/studio_chart2d_java.hpp`](../include/imguistudio/export/detail/studio_chart2d_java.hpp) | Render generado de gráficas 2D para Java. |
| [`include/imguistudio/export/detail/studio_chart3d_java.hpp`](../include/imguistudio/export/detail/studio_chart3d_java.hpp) | Render generado de gráficas 3D para Java. |
| [`include/imguistudio/export/detail/studio_titlebar_java.hpp`](../include/imguistudio/export/detail/studio_titlebar_java.hpp) | Soporte de título generado para Java. |
| [`include/imguistudio/export/detail/studio_chart2d_python.hpp`](../include/imguistudio/export/detail/studio_chart2d_python.hpp) | Render generado de gráficas 2D para Python. |
| [`include/imguistudio/export/detail/studio_chart3d_python.hpp`](../include/imguistudio/export/detail/studio_chart3d_python.hpp) | Render generado de gráficas 3D para Python. |
| [`include/imguistudio/export/detail/studio_titlebar_python.hpp`](../include/imguistudio/export/detail/studio_titlebar_python.hpp) | Soporte de título generado para Python. |
| [`include/imguistudio/export/detail/studio_chart2d_rust.hpp`](../include/imguistudio/export/detail/studio_chart2d_rust.hpp) | Render generado de gráficas 2D para Rust. |
| [`include/imguistudio/export/detail/studio_chart3d_rust.hpp`](../include/imguistudio/export/detail/studio_chart3d_rust.hpp) | Render generado de gráficas 3D para Rust. |
| [`include/imguistudio/export/detail/studio_titlebar_rust.hpp`](../include/imguistudio/export/detail/studio_titlebar_rust.hpp) | Soporte de título generado para Rust. |

## Pruebas y referencias

| Archivo o carpeta | Responsabilidad |
| --- | --- |
| [`tests/verify_core.cpp`](../tests/verify_core.cpp) | Contrato del modelo y del generador básico. |
| [`tests/verify_persistence.cpp`](../tests/verify_persistence.cpp) | Fixtures, lecturas, escrituras y datos desconocidos. |
| [`tests/verify_sessions.cpp`](../tests/verify_sessions.cpp) | Recuperación, revisiones del historial y concurrencia. |
| [`tests/verify_document_edit.cpp`](../tests/verify_document_edit.cpp) | Ediciones y validación de árboles. |
| [`tests/verify_canvas.cpp`](../tests/verify_canvas.cpp) | Gestos, selección y edición en el lienzo. |
| [`tests/verify_preview.cpp`](../tests/verify_preview.cpp) | Interacción, geometría y render de imágenes. |
| [`tests/verify_session_ui.cpp`](../tests/verify_session_ui.cpp) | Comportamiento del diálogo de recuperación. |
| [`tests/verify_codegen.cpp`](../tests/verify_codegen.cpp) | Generación por lenguaje, fixtures y salidas de prueba. |
| [`tests/studio_chrome_interactions.hpp`](../tests/studio_chrome_interactions.hpp) | Ayuda a verificar el título personalizado en proyectos generados. |
| [`tests/verify_status_widgets.cpp`](../tests/verify_status_widgets.cpp) | Proyectos con interruptores, indicadores y progreso circular. |
| [`tests/verify_extensions.cpp`](../tests/verify_extensions.cpp) | Instalación, biblioteca y exportación de extensión verificada. |
| [`tests/verify_process.cpp`](../tests/verify_process.cpp) | Argumentos, salida y cancelación de subprocesos. |
| [`tests/verify_build.cpp`](../tests/verify_build.cpp) | Compilación y ejecución a través del controlador del editor. |
| [`tests/verify_build_profiles.cpp`](../tests/verify_build_profiles.cpp) | Persistencia, destinos y compatibilidad de perfiles. |
| [`tests/verify_build_diagnostics.cpp`](../tests/verify_build_diagnostics.cpp) | Análisis y navegación de errores. |
| [`tests/verify_assistant.cpp`](../tests/verify_assistant.cpp) | Contratos del asistente y validación de propuestas. |
| [`tests/verify_assistant_http.py`](../tests/verify_assistant_http.py) | Servidor de prueba y formatos de proveedor sobre loopback. |
| [`tests/verify_python_export.py`](../tests/verify_python_export.py) | Pruebas del artefacto Python. |
| [`tests/fixtures`](../tests/fixtures) | Casos de compatibilidad y datos de entrada de regresión. |
| [`examples`](../examples) | Proyectos para realizar un recorrido manual del editor. |
| [`third_party/README.md`](../third_party/README.md) | Origen y avisos de dependencias vendorizadas. |
| [`tests/verify_android_export.cmake`](../tests/verify_android_export.cmake) | Orquesta la comprobación de generación del complemento Android. |
| [`tests/verify_apple_export.cmake`](../tests/verify_apple_export.cmake) | Orquesta la comprobación de generación de los complementos macOS/iOS. |
| [`tests/verify_c_export.cmake`](../tests/verify_c_export.cmake) | Orquesta la comprobación de runtime C11. |
| [`tests/verify_csharp_export.cmake`](../tests/verify_csharp_export.cmake) | Orquesta la comprobación de runtime C#. |
| [`tests/verify_java_export.cmake`](../tests/verify_java_export.cmake) | Orquesta la comprobación de runtime Java. |
| [`tests/verify_rust_export.cmake`](../tests/verify_rust_export.cmake) | Orquesta la comprobación de runtime Rust. |
| [`tests/verify_vulkan_export.cmake`](../tests/verify_vulkan_export.cmake) | Orquesta la comprobación de runtime C++ Vulkan. |
