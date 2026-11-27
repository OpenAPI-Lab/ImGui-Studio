# Widget and resource implementation

[Español](WIDGET_RENDER.md)

The model stores values and assets. Helpers interpret them and the renderer owns the session's graphics objects. This boundary lets multiple backends consume the same input.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [studio_images.hpp](../../src/widgets/studio_images.hpp) | Defines `ImageImport` and `NativeImageCache`. | Separates serialized asset data from context-owned textures. |
| [studio_images.cpp](../../src/widgets/studio_images.cpp) | Imports images, decodes with stb, creates textures, and releases cache entries. | The cache avoids per-frame uploads. Release requires a live graphics context. |
| [studio_chart2d.hpp](../../src/widgets/studio_chart2d.hpp) | `sampleIndices`, `cartesian`, `pie`, and `render` draw the 2D modes. | Sampling bounds drawing work. Check scales, empty series, and pointer-to-data mapping. |
| [studio_chart3d.hpp](../../src/widgets/studio_chart3d.hpp) | `Projection` and point and face helpers project 3D modes onto the draw list. | Orientation affects visual face order. Preserve ranges and keep design orientation separate from transient orbit. |
| [studio_widget_library.hpp](../../include/imguistudio/widgets/studio_widget_library.hpp) | `StudioWidgetLibrary` validates, reads, and writes definitions. `instantiateLibraryWidget` copies and remaps IDs. | Instances must remain independent and retain the assets they need after insertion. |
| [studio_image_data.hpp](../../include/imguistudio/widgets/studio_image_data.hpp) | Validates base64 asset data, dimensions, and bounds. `ExportAssets` organizes output assets. | Exporters require validated bytes and the backend decides how to create textures. Check PNG, JPEG, and rejected inputs. |
| [studio_window_appearance.hpp](../../include/imguistudio/widgets/studio_window_appearance.hpp) | Registers numeric properties, colors, window presets, and title eligibility. | The model and generator need common rules to preserve values and reproduce appearance. |
| [studio_numeric_format.hpp](../../include/imguistudio/widgets/studio_numeric_format.hpp) | `safeNumericFormat` accepts formats compatible with the expected numeric value. | Preserves Dear ImGui's variadic contract. When a format is rejected, keep the original in the design and render with a valid fallback. |
| [studio_knob.hpp](../../include/imguistudio/widgets/studio_knob.hpp) | `knobSettings` interprets a knob's range, value, and speed. | Shared settings keep preview and C++ consistent. Test degenerate ranges and values outside the range. |
| [studio_status_widgets.hpp](../../include/imguistudio/widgets/studio_status_widgets.hpp) | Implements `toggle`, `spinner`, `ring`, and `circularProgress` on top of Dear ImGui. | CMake embeds this code in generated projects that use it. Keep its direct Dear ImGui dependency contract. |

## Verification

Open [Status and progress](../../examples/estado-y-progreso.json) and [the image project](../../examples/vulkan-imagenes.json). Run `StatusWidgetExportTest`, `NativeImagePreviewTest`, and `NativeWidgetPreviewTest`. Check assets in both the editor and the affected exported backend. [Library](../WIDGET_LIBRARY.en.md)

