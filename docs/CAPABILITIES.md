# Capacidades y validación

[English](CAPABILITIES.en.md)

| Superficie | Disponible | Validación y alcance |
| --- | --- | --- |
| Editor de escritorio | Dear ImGui, GLFW y OpenGL. | Compilación y arranque en Windows con GCC y MSVC. Wayland directo y XWayland comprobados en Ubuntu 24.04 bajo WSL. Configuración macOS pendiente de ejecución. |
| Proyecto | Inicio, recientes, guardado, historial, undo/redo y recuperación. | Pruebas de persistencia, ediciones, conflictos y sesiones. Guarda como otro archivo cuando se detecta un conflicto. |
| Diseño | Lienzo libre, jerarquía, inspector, título configurable, vista previa, imágenes y gráficas. | El soporte puede variar por control y por propiedad. La referencia de edición detalla los tipos compatibles. |
| Biblioteca y extensiones | Widgets propios, importación/exportación y catálogo curado imgui-knobs. | La perilla tiene preview y salida C++. Los otros exportadores generan un marcador para ese tipo. |
| C++ OpenGL | Fuentes y proyecto CMake con GLFW. | Pruebas de generación y ejecución en escritorio. |
| C++ Vulkan | Proyecto CMake con GLFW y Vulkan. | Presentación, redimensionado, entrada e imágenes en Windows y Ubuntu WSL. |
| C11, Python, C#, Java y Rust | Proyectos de escritorio con sus respectivos toolchains. | Hay resultados de ejecución en Windows y WSL. Los diagnósticos de cada proyecto identifican elementos parciales o sin salida. |
| Android C++ | Complemento Gradle/CMake con NativeActivity y OpenGL ES 3. | APK arm64 y x86-64. Arranque, tacto, texturas, rotación y reanudación en emulador Android 15 x86-64. Equipo ARM64 físico y teclado en pantalla pendientes. |
| macOS/iOS C++ | Plantillas opcionales Metal con AppKit y UIKit. | Generación verificada en Windows y WSL. Compilación y ejecución en SDK Apple pendientes. |
| Idiomas | Inglés, español, francés, coreano, japonés y chino simplificado. | Catálogos externos compatibles. Consulta [cadenas y glifos pendientes](LOCALIZATION.md). |
| Asistente | Adaptadores para Ollama, LM Studio, OpenAI, Anthropic y Gemini. | Se verificaron solicitudes con un servidor simulado. Keychain en Mac y conexiones reales requieren comprobación. |
| Paquetes | ZIP portátil y EXE para Windows, DEB para Ubuntu 24.04 y Debian 13, y flujo DMG para macOS. | ZIP en Windows, ciclo EXE en Wine aislado, ciclo DEB en Ubuntu WSL e instalación con APT y arranque con X11 virtual en Debian 13. UAC nativo, firma y validación del DMG pendientes. |
