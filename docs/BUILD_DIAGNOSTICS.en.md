# Build diagnostics and source navigation

[Español](BUILD_DIAGNOSTICS.md)

## Find the cause of a failed build

Open **Native project → Run / Build…**, select a profile, and choose **Build**. Expand **Diagnostics**. Identify the first relevant error in its phase and compare it with the recent raw output.

| Phase or origin | What to inspect |
| --- | --- |
| Configure | Generator, compiler, SDK, and libraries required by the target. A CMake error can point to its configuration file. |
| Build | Open **View exported code** to see the attempt snapshot. If **Select widget** is available, inspect the property that produced the line. |
| Generator | Read the diagnostic and `codegen_report.txt` for partially supported controls and properties. |
| Run | Check the exit code, runtime dependencies, and log. Run output is retained as text. |

## Navigation contract

`DiagnosticStream` reads stdout and stderr fragments. It parses common GCC/Clang, MSVC/Roslyn, and CMake locations. It also handles split lines, CRLF, and a final message without a newline. Individual lines longer than 16 KiB are omitted from structured diagnostics. The pane retains up to 1,000 entries, and a new error can displace a warning or note.

Mapping uses the normalized path and the generator's node range. For example, given a parent at lines 10–100 of `ui.hpp` and a child at 20–35, an error at line 25 points to the child. The same line in `external.hpp` keeps its own location. After editing the project, build again so **Select widget** uses a snapshot that matches the design.

## Verification and reporting

Run `BuildDiagnosticsTest` to cover fragmented input, compiler codes, paths, and nested ranges. `NativeProjectBuildTest` exercises the cycle with an exported C++ project and a controlled fixture error. In a report, include the target, generator, compiler, phase, first relevant line, range from the generated report, and a minimal project. Replace private paths or data before attaching the log.

[Profiles and execution](RUN_BUILD.en.md) · [Parser code](../include/imguistudio/build/studio_build_diagnostics.hpp) · [Test cases](../tests/verify_build_diagnostics.cpp)
