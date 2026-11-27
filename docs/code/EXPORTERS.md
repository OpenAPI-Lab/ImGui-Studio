# Generación de código y ensamblado

[English](EXPORTERS.en.md)

Los generadores producen código, diagnósticos y rangos de líneas. Los bundles agregan un host, recursos e instrucciones y se escriben en un destino nuevo. El panel de código puede producir texto sin invocar el compilador.

## Contrato de los archivos

| Archivo | Qué hace | Por qué existe y qué conservar |
| --- | --- | --- |
| [studio_codegen.hpp](../../include/imguistudio/export/studio_codegen.hpp) | Expone la entrada al generador C++ mediante una cabecera de inclusión. | Mantiene una ruta estable para los llamadores aunque el backend esté en otra cabecera. |
| [studio_codegen_native.hpp](../../include/imguistudio/export/studio_codegen_native.hpp) | `generateCppDetailed` recorre nodos y construye código, estado, avisos y rangos en `GeneratedCpp`. | La vista de código, los bundles y los diagnósticos dependen de estos resultados. Comprueba escape de texto, nombres y rangos cuando cambie el emisor. |
| [studio_cpp_export.hpp](../../include/imguistudio/export/studio_cpp_export.hpp) | `CppBundle` reúne archivos. `generateCppBundle` selecciona renderizador y `writeNewExportDirectory` materializa la salida. | La selección de renderizador gobierna el host y texturas. El destino debe ser nuevo para proteger otros proyectos. |
| [studio_c_export.hpp](../../include/imguistudio/export/studio_c_export.hpp) | Emite la interfaz C11, un host C y el puente C++ para cimgui. | El código de interfaz debe compilar como C11. El puente aloja dependencias C++ y mantiene claro el límite ABI. |
| [studio_python_export.hpp](../../include/imguistudio/export/studio_python_export.hpp) | Emite `main.py`, el estado y el requisito de imgui-bundle. | El runtime carga el binding esperado. Revisa la sintaxis y la ejecución para asegurar que no solo se produjo texto. |
| [studio_csharp_export.hpp](../../include/imguistudio/export/studio_csharp_export.hpp) | Crea la interfaz ImGui.NET, el proyecto y host .NET y recursos si existen. | El host debe cargar los recursos y gestionar el contexto. Verifica la ruta headless y la ventana si cambian bindings gráficos. |
| [studio_java_export.hpp](../../include/imguistudio/export/studio_java_export.hpp) | Emite `StudioUI.java`, `Main.java`, POM y recursos del classpath. | La ejecución depende del JDK, los JAR y nativos correctos. Conserva la correspondencia recurso–ruta generada. |
| [studio_rust_export.hpp](../../include/imguistudio/export/studio_rust_export.hpp) | Emite `ui.rs`, `main.rs`, Cargo y puentes nativos cuando se requieren. | La interfaz mantiene estado y vida de recursos separados del host. Las funciones opcionales deben seguir compilando con su configuración. |
| [studio_android_export.hpp](../../include/imguistudio/export/studio_android_export.hpp) | `generateAndroidCppBundle` valida el pack e integra el diseño con Gradle y NativeActivity. | Los archivos obligatorios provienen del descriptor. Se validan antes de escribir el destino. |
| [studio_apple_export.hpp](../../include/imguistudio/export/studio_apple_export.hpp) | `generateAppleCppBundle` combina la interfaz, el pack Metal y los datos del destino macOS o iOS. | Conserva los campos generados y el contrato de cada plataforma. Generar un bundle y compilarlo con Xcode son comprobaciones distintas. |

## Ejemplo

`generateCppBundle(project, CppRenderer::OpenGL)` devuelve el conjunto de archivos. Antes de llamar a `writeGeneratedBundle(bundle, destination)`, presenta los diagnósticos al usuario. El informe incluye IDs y rangos para investigar pérdidas de cobertura. Para integrar `RenderStudioUI(state)` en otro host, consulta [el ejemplo compilable](../../examples/title-bar-action/README.md).

## Verificación

Ejecuta `CppExportGenerationTest`, las pruebas `GeneratedCpp_*` y las pruebas de generación Android/Apple. Para cada lenguaje, elige su prueba de runtime en [validación](../VALIDATION.md).

