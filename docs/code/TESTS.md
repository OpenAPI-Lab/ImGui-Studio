# Código de las pruebas y los fixtures

[English](TESTS.en.md)

Cada ejecutable, script o proyecto generado verifica un contrato distinto. Los casos de `tests/fixtures/` mantienen entradas representativas del formato y sus revisiones. Los proyectos de `examples/` se usan para verificar un recorrido desde la UI.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [verify_core.cpp](../../tests/verify_core.cpp) | Ejercita tipos y rutas básicas del generador sin GUI. | Fija las reglas que debe conservar una compilación mínima. |
| [verify_persistence.cpp](../../tests/verify_persistence.cpp) | Abre fixtures, guarda diseños y compara los datos recuperados. | Comprueba compatibilidad y campos desconocidos al cambiar JSON. |
| [verify_sessions.cpp](../../tests/verify_sessions.cpp) | Ejecuta escenarios de revisión, locks y recuperación con procesos auxiliares. | La concurrencia y el fin abrupto requieren procesos reales. |
| [verify_document_edit.cpp](../../tests/verify_document_edit.cpp) | Comprueba planes de movimiento, restricciones y resultados del árbol. | Aísla reglas del modelo de eventos gráficos. |
| [verify_canvas.cpp](../../tests/verify_canvas.cpp) | Alimenta eventos Dear ImGui y comprueba gestos de diseño. | Detecta desajustes entre geometría, destino y cambio confirmado. |
| [verify_preview.cpp](../../tests/verify_preview.cpp) | Prueba render, entrada, geometría y recursos en preview. | Comprueba que un gesto temporal no modifique el modelo. |
| [verify_session_ui.cpp](../../tests/verify_session_ui.cpp) | Dibuja el diálogo de recuperación con estado aislado. | Comprueba la integración de la UI con los datos recuperables. |
| [verify_codegen.cpp](../../tests/verify_codegen.cpp) | Produce fixtures de exportación y comprueba contenido y preservación. | El código generado sirve de entrada para las pruebas de compilación y runtime. |
| [studio_chrome_interactions.hpp](../../tests/studio_chrome_interactions.hpp) | Añade gestos y aserciones de ventana a casos emitidos. | Permite comprobar acciones más allá de compilar el código fuente. |
| [verify_status_widgets.cpp](../../tests/verify_status_widgets.cpp) | Verifica estado y empaquetado del helper de los widgets de estado. | El helper aparece solo en diseños que lo usan. |
| [verify_extensions.cpp](../../tests/verify_extensions.cpp) | Comprueba instalación, biblioteca y resultados de exportar extensiones. | Asegura que contenido alterado no active una extensión. |
| [verify_process.cpp](../../tests/verify_process.cpp) | Prueba argumentos, salida, EOF y cancelación de procesos. | Los casos cruzan el límite del proceso y cubren reglas de cada host. |
| [verify_build.cpp](../../tests/verify_build.cpp) | Conduce builds y arranques con el controlador real. | Prueba que el resultado corresponde al diseño y que el fallo se puede diagnosticar. |
| [verify_build_profiles.cpp](../../tests/verify_build_profiles.cpp) | Comprueba perfiles persistentes, revisiones y adjuntos al proyecto. | Permite cambiar configuración sin perder formatos o datos de otras sesiones. |
| [verify_build_diagnostics.cpp](../../tests/verify_build_diagnostics.cpp) | Prueba formatos de compilador, fragmentos y selección por rangos. | Un cambio de parser debe conservar texto y asociación dentro de los límites. |
| [verify_assistant.cpp](../../tests/verify_assistant.cpp) | Comprueba contratos de petición y validez de propuestas. | El código puede verificarse con entradas locales sin proveedores reales. |
| [verify_assistant_http.py](../../tests/verify_assistant_http.py) | Sirve respuestas locales para formatos de red del asistente. | El test verifica lo enviado y la interpretación sin credenciales reales. |
| [verify_python_export.py](../../tests/verify_python_export.py) | Ejercita artefactos Python con las herramientas configuradas. | Comprueba la salida en su intérprete y binding. |
| [verify_android_export.cmake](../../tests/verify_android_export.cmake) | Genera un proyecto desde el pack Android y revisa sus archivos. | Comprueba el ensamblado independientemente de instalar el APK. |
| [verify_apple_export.cmake](../../tests/verify_apple_export.cmake) | Genera macOS/iOS y puede compilar ambos con Xcode. | Separa validación de archivo de validación con SDK. |
| [verify_c_export.cmake](../../tests/verify_c_export.cmake) | Configura, compila y ejecuta casos C11 con cimgui. | Verifica que el puente C++ permita un host e interfaz C. |
| [verify_csharp_export.cmake](../../tests/verify_csharp_export.cmake) | Compila proyectos .NET y usa el modo de prueba emitido. | El runtime verifica código y estado que el generador por sí solo no puede ejecutar. |
| [verify_java_export.cmake](../../tests/verify_java_export.cmake) | Compila y arranca Java con JDK y bindings configurados. | Comprueba recursos, ejecución y compatibilidad real del destino. |
| [verify_rust_export.cmake](../../tests/verify_rust_export.cmake) | Compila proyectos Cargo y ejecuta su prueba de arranque. | Verifica el código Rust y los puentes nativos que el proyecto necesita. |
| [verify_vulkan_export.cmake](../../tests/verify_vulkan_export.cmake) | Construye y ejecuta el fixture de Vulkan. | Comprueba configuración, presentación e imágenes con un backend real. |

## Cómo elegir una prueba

Empieza con el caso más próximo a la edición. Un cambio en JSON necesita un fixture anterior y uno rechazado. Un cambio de render requiere un contexto Dear ImGui y, si afecta código emitido, una ejecución del destino. Un cambio de instalación requiere el archivo CPack real. Reproduce los comandos desde los CTest documentados y registra qué SDK se usó. [Guía de validación](../VALIDATION.md) · [Procedencia de fixtures](../../tests/fixtures/manifest.json)

