# Export and test an Android app

[Español](ANDROID.md)

ImGui Studio generates a C++ project with Gradle, NativeActivity, and
OpenGL ES 3. The pack supplies the host files. The NDK and other tools
are required to build the generated project.

## Prepare and export

From the repository root, build the optional pack in a separate directory:

```sh
cmake -S . -B build/packs -G Ninja -DBUILD_STUDIO_GUI=OFF -DBUILD_CORE_TEST=OFF
cmake --build build/packs --target studio_android_pack
```

The archive is under `build/packs/optional-packs/`. Extract it. Open
[Images and input](../examples/vulkan-imagenes.json) in the editor.
Under **Generated code → C++ → Export Android**, select the `Android-Cpp`
folder containing `template.version` and a new output directory.

## Build the APK

| Tool | Template version |
| --- | --- |
| JDK | 17 or 21 |
| Gradle and Android Gradle Plugin | Gradle 8.9, AGP 8.7.3 |
| Android SDK | Platform 35 and Build Tools 34.0.0 |
| Native tools | NDK 27.2.12479018 and SDK CMake 3.22.1 |

Open the generated project in Android Studio or run `gradle assembleDebug`
from its root with the SDK configured. The APK is under
`build/outputs/apk/debug/`. The default ID is `org.imguistudio.exported`.
For your own application, pass `-PstudioApplicationId=org.example.app`
to Gradle. Set the display label in `AndroidManifest.xml`.

## Run and report

Use an x86-64 emulator or ARM64 device with OpenGL ES 3. Record the model,
API level, and architecture. If using ADB, run `adb devices` and pick the
test device explicitly.

```sh
adb -s SERIAL install -r build/outputs/apk/debug/StudioExportAndroid-debug.apk
adb -s SERIAL shell am start -W -n org.imguistudio.exported/android.app.NativeActivity
```

Check startup, PNG/JPEG images, touch, rotation, and resuming from the
background. If you changed the ID, use that same ID to launch. Consult
`codegen_report.txt` for design warnings and the Android log for host errors.
The template needs additional integration for the on-screen keyboard and IME.

## Test coverage

`AndroidCppExportGenerationTest` extracts the CMake-built ZIP and generates
a project with images and a knob. It checks expected files, preservation
of an existing export, and rejection of a missing pack. APK compilation
and startup are validated separately. See the [verified capabilities](CAPABILITIES.en.md)
and the [Gradle template](../templates/android/build.gradle).
