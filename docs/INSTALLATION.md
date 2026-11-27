# Instalar ImGui Studio

[English](INSTALLATION.en.md)

## Elegir el archivo

Selecciona el instalador de tu sistema en la sección **Assets** de la versión correspondiente. Los archivos `.sha256` sirven para verificar el binario. En Linux, elige el DEB de tu distribución. [GitHub Releases](https://github.com/OpenAPI-Lab/ImGui-Studio/releases)

| Sistema | Archivo de la versión 1.0.0 |
| --- | --- |
| Windows x64 | `ImGuiStudio-1.0.0-Windows-AMD64.exe` |
| Ubuntu 24.04 x64 | `imgui-studio_1.0.0_ubuntu24.04_amd64.deb` |
| Debian 13 x64 | `imgui-studio_1.0.0_debian13_amd64.deb` |

El instalador de Windows está sin firma Authenticode. El DMG del editor para macOS sigue pendiente de compilación y verificación. Para trabajar desde el código fuente, consulta la [guía de compilación](BUILD_AND_TEST.md).

## Windows

Abre el EXE y sigue el asistente. Después inicia **ImGui Studio** desde el menú Inicio. Para desinstalar, usa **Aplicaciones instaladas** en la configuración de Windows o el acceso directo de desinstalación del menú Inicio.

En PowerShell, calcula la suma y compárala con el primer valor del archivo `.sha256`:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath '.\ImGuiStudio-1.0.0-Windows-AMD64.exe'
```

## Ubuntu y Debian

Abre una terminal en la carpeta de descarga. Ejecuta el par de comandos de tu distribución para verificar e instalar el paquete:

```sh
# Ubuntu 24.04
sha256sum -c imgui-studio_1.0.0_ubuntu24.04_amd64.deb.sha256 &&
  sudo apt install ./imgui-studio_1.0.0_ubuntu24.04_amd64.deb

# Debian 13
sha256sum -c imgui-studio_1.0.0_debian13_amd64.deb.sha256 &&
  sudo apt install ./imgui-studio_1.0.0_debian13_amd64.deb
```

APT resuelve las bibliotecas declaradas en el paquete. Abre **ImGui Studio** desde el menú de aplicaciones o ejecuta `imgui_studio` dentro de una sesión gráfica. El paquete admite Wayland y X11. Bajo WSL usa WSLg. Puedes retirar el programa con `sudo apt remove imgui-studio`.

## Abrir el primer proyecto

En el inicio, elige **Nuevo proyecto**, **Abrir proyecto** o una entrada de **Recientes**. El aviso de recuperación aparece si queda una copia recuperable de una sesión anterior. Usa la [guía de primer proyecto](WORKFLOW.md) para editar, previsualizar y exportar un diseño.

Los complementos de exportación C++ para Android, macOS e iOS se descargan aparte. Se usan para generar proyectos para esas plataformas desde el editor de escritorio. [Guía de exportación](EXPORTING.md#destinos-opcionales)
