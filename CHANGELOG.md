# Cambios

[English](CHANGELOG.en.md)

## 1.0.0 — 2026-11-24

- Editor visual nativo con pantalla de inicio, proyectos recientes y recuperación
  de cambios sin guardar.
- Lienzo con colocación libre, jerarquía editable, inspector, personalización
  de la barra de título y vista previa.
- Biblioteca con widgets incluidos, creación desde un lienzo en blanco y
  catálogo de extensiones. El primer complemento integrado es imgui-knobs.
- Interfaz en inglés, español, francés, coreano, japonés y chino simplificado,
  con soporte para paquetes de idioma comunitarios.
- Exportación de escritorio a C++, C, Python, C#, Java y Rust. La cobertura
  varía por widget, lenguaje y plataforma.
- Salida Vulkan para C++ en Windows y Linux, además de la salida OpenGL.
- Complementos opcionales C++ para Android con OpenGL ES y para macOS e
  iOS con Metal.

### Verificación y límites

- El editor y el ZIP portátil se probaron en Windows. El ciclo del instalador
  EXE se verificó con Wine en un entorno aislado. La firma Authenticode y el
  aviso de elevación en Windows nativo quedan pendientes.
- El DEB de Ubuntu 24.04 x64 se instaló, arrancó y retiró bajo WSL tanto con
  Wayland directo como con XWayland. El DEB de Debian 13 x64 se instaló y
  retiró con APT y arrancó con X11 virtual en un contenedor Debian 13.
- El APK de ejemplo se compiló para arm64 y x86-64. El arranque, las imágenes,
  el toque, la rotación y la reanudación se probaron en un emulador Android 15
  x86-64. Quedan pendientes el teclado en pantalla y la prueba en un teléfono.
- La generación de los proyectos de macOS e iOS se comprobó en Windows y WSL.
  Su compilación y ejecución con Xcode, y la prueba del DMG, siguen pendientes.

Consulta los [instaladores y las sumas SHA-256](docs/INSTALLATION.md).
