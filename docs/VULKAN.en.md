# Export to Vulkan

[Español](VULKAN.md)

The **C++ / Vulkan** target generates a Dear ImGui application with GLFW for Windows and Linux. The export includes its own host. You can build it on another machine with a C++ and Vulkan toolchain.

## Export and try it

1. Open [Images and input](../examples/vulkan-imagenes.json).
2. Under **Generated code**, export C++, select **Vulkan**, and choose a new directory.
3. Configure, build, and run the basic check from that directory.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target studio_export
ctest --test-dir build --output-on-failure
```

Open `build/studio_export.exe` on Windows or `./build/studio_export` on Linux. Toggle the checkbox and resize the window. The PNG and JPEG should remain visible. `codegen_report.txt` maps diagnostics and generated lines to design node IDs.

## Dependencies and diagnosis

| Stage | Requirement | What to check on failure |
| --- | --- | --- |
| Configure | Vulkan headers and the loader link library. | On Windows, use the LunarG SDK or your toolchain's development files. On Ubuntu, the usual package is `libvulkan-dev`. |
| Run | Loader, driver, and presentation surface. | If surface extensions are missing, check the window backend and session. If the host cannot select a device, check the driver and its presentation support. |
| Custom SDK location | SDK path for CMake. | Configure with `"-DVulkan_ROOT=/path/to/sdk"` or set the Vulkan SDK directory in Run / Build. |

The host creates the surface and swapchain and recreates the presentation target on resize. The export adds `studio_vulkan_images.hpp` for designs with images. To use the design with your own host, keep `ui.hpp` and adapt texture creation and release to your renderer's lifecycle.

[FindVulkan reference](https://cmake.org/cmake/help/latest/module/FindVulkan.html) · [LunarG SDK](https://vulkan.lunarg.com/sdk/home)

## Repository tests

From the repository root, configure a GUI build with tests enabled. For a custom SDK location, add `-DSTUDIO_VULKAN_TEST_ROOT=...` to the configuration.

```sh
cmake -S . -B build/check-vulkan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DSTUDIO_TEST_VULKAN_EXPORT=ON
cmake --build build/check-vulkan --target verify_codegen verify_build
ctest --test-dir build/check-vulkan -R '^(GeneratedVulkanRuntime|NativeVulkanProjectBuildTest)$' --output-on-failure
```

- **GeneratedVulkanRuntime** verifies the missing-SDK diagnostic in a separate configure step. It then checks five presented frames, resizing, checkbox input, and PNG/JPEG draw commands.
- **NativeVulkanProjectBuildTest** exercises Run / Build, rejects an invalid SDK path, and checks compilation and startup. Switching to OpenGL invalidates the earlier result.

If you enable a validation layer, record which layer loaded and any messages. Include the driver and whether Linux used native Wayland or XWayland. A user export's basic smoke test checks startup. The repository's instrumented tests also exercise the behavior above. See [reproducible validation](VALIDATION.en.md).

On Linux, with `VK_LAYER_KHRONOS_validation` installed, rerun the instrumented
test and inspect which layer the loader used:

```sh
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LOADER_DEBUG=layer \
  ctest --test-dir build/check-vulkan -R '^GeneratedVulkanRuntime$' -V
```

Look for `Insert instance layer "VK_LAYER_KHRONOS_validation"` and keep the
output if it reports an error. The `Vulkan presented 5 frames` line names the
device and window backend.
