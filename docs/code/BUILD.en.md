# Build engine, profiles, and processes

[Español](BUILD.md)

Build exports a copy of the design, processes it with the toolchain, and retains the attempt's result. The UI uses controller states to enable Run and report errors.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [studio_build.hpp](../../include/imguistudio/build/studio_build.hpp) | `BuildSettings` and the controller resolve tools, prepare an attempt, and run target-specific phases. | An artifact is valid only for its attempt's design and settings. Diagnostics must retain the original snapshot. |
| [studio_build_profiles.hpp](../../include/imguistudio/build/studio_build_profiles.hpp) | `BuildProfileStore` validates and persists profiles. `attachBuildProfile` explicitly attaches one to JSON. | Local paths stay in the profile unless explicitly shared. An invalid or concurrently revised file is preserved. |
| [studio_build_diagnostics.hpp](../../include/imguistudio/build/studio_build_diagnostics.hpp) | `DiagnosticStream` consumes fragments and `diagnosticNode` maps locations to node ranges. | The path must match the generated file. The narrowest range containing the line wins. |
| [studio_process.hpp](../../include/imguistudio/build/studio_process.hpp) | `ProcessCommand` describes executable, arguments, and directory. `NativeProcess` captures output and manages the child. | Arguments are passed individually, stdin is closed, and cancellation acts on the editor-owned job or process group. |

### C++ example

This example simulates two compiler output fragments. The diagnostic appears after the line is complete and maps to the node with the most specific range.

```cpp
#include "imguistudio/build/studio_build_diagnostics.hpp"
#include <cassert>

int main() {
    using namespace ImGuiStudio;
    const auto source = std::filesystem::absolute("generated");
    DiagnosticStream stream;
    std::vector<BuildDiagnostic> entries;
    stream.reset(source, source, {{"panel", {10, 100}}, {"button", {20, 35}}},
                 "Build");

    stream.feed("ui.hpp:25:7: error: invalid ", entries);
    stream.feed("expression\n", entries);
    stream.finish(entries);
    assert(entries.size() == 1);
    assert(entries.front().nodeId == "button");
}
```

## Verification

Run `NativeProcessTest`, `BuildProfilesPersistenceTest`, `BuildDiagnosticsTest`, and `NativeProjectBuildTest`. For a new target, cover toolchain failure, cancellation, and a design changed after Build. [Run / Build](../RUN_BUILD.en.md) · [Diagnostics](../BUILD_DIAGNOSTICS.en.md)

