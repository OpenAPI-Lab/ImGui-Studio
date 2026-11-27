# Run / Build and diagnostics

[Español](RUN_BUILD.md)


[Building the editor](BUILD_AND_TEST.en.md) produces ImGui Studio. **Run / Build** operates on the application you are designing. This boundary lets you test an export with its own toolchain and dependencies.

## Workflow and artifact validity

| Action | Input | Result |
| --- | --- | --- |
| Detect tools | Workspace, profile, and editor environment | Suggested paths. Configure or Build checks the toolchain. |
| Build | Current design and profile settings | A new export attempt, its log, and an executable artifact or validated script on success. |
| Run | A valid result for the current design and profile | Starts what the attempt built or verified. |
| Test startup | The same result and a target that supports the check | Runs the generated host's startup check. |
| Stop | An active process | Cancels the process or group controlled by the editor. |

Try [the title bar example](../examples/barra-de-titulo.json). Pick C++, choose an empty workspace, and build. Open **View exported code** to inspect the snapshot for that attempt. Run the application, change a property, and build again. A result is valid for the design and settings used to build it. Each attempt directory remains available for diagnosis.

## Profiles and persistence

Create up to 32 profiles. **Save profiles** commits changes to `build_profiles.json` in the state directory. **Reload profiles** reads it again and preserves in-memory data if reload fails. The active profile is restored at startup. Build again before running in a new session.

Version 1 uses `schemaVersion`, `activeProfile`, and `profiles`. Each profile stores `id`, `name`, `workspace`, and `settings`. C++ can store `cppRenderer` and `vulkanRoot`. Rust stores `cargo` and `rustOffline`. C11, C#, Python, and Java have separate tool settings. The command to include a profile in a project explicitly writes `buildConfig.nativeCpp`. Review its paths before sharing the JSON.

## Read a failure

1. Identify whether configure, build, or run failed. If CMake, an SDK, or a library is missing, fix the profile before inspecting the widget.
2. Open **Diagnostics** to see severity, file, and line. Dependencies and host code can have locations without a node association.
3. For generated code, use **Select widget** when available. The editor compares the attempt snapshot against the current tree.
4. If output appears only in the log, include the original line in a report. The parser handles common GCC, Clang, MSVC, Roslyn, and CMake formats.

[Profiles](../include/imguistudio/build/studio_build_profiles.hpp) · [Controller](../include/imguistudio/build/studio_build.hpp) · [Processes](../include/imguistudio/build/studio_process.hpp) · [Diagnostics](../include/imguistudio/build/studio_build_diagnostics.hpp)
