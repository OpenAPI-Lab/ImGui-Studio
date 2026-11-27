# Extending the editor

[Español](EXTENDING.md)


## A widget

1. Define its type, serialized name, and defaults in [the model](../include/imguistudio/project/studio_types.hpp) and [the registry](../include/imguistudio/project/studio_widget_registry.inc). Preserve stored names when extending an existing type.
2. Add its inspector fields and rendering to [the preview](../src/editor/studio_preview.cpp). Keep temporary interaction values separate from initial values. Define its sizing and container behavior.
3. Implement rendering in supported generators, or preserve a diagnostic for the placeholder. Check whether it needs resources, helpers, title bar support, or another dependency.
4. Check save/load, rendering, and the generated output. [The status widgets](../include/imguistudio/widgets/studio_status_widgets.hpp) show how to share C++ implementation between preview and export.

## Example code walkthrough

Trace **Circular progress** before changing a control. Open [the example project](../examples/estado-y-progreso.json), select `job_progress`, and locate its `fraction` and `overlay` properties.

| Location | What to check and why |
| --- | --- |
| [Widget registry](../include/imguistudio/project/studio_widget_registry.inc) | `circular_progress` is the saved type ID. Keeping it lets earlier designs open. |
| [Serialization](../include/imguistudio/project/studio_serialization.hpp) | `readProps` and `propsJson` read and write node properties. When adding one, test a project saved before it existed. |
| [Preview and inspector](../src/editor/studio_preview.cpp) | Follow `WidgetType::CircularProgress` and its inspector fields. Verify the intended effects of editing initial values and using temporary ones. |
| [Shared drawing code](../include/imguistudio/widgets/studio_status_widgets.hpp) | The helper gives preview and generated C++ the same rendering implementation. |
| [C++ generator](../include/imguistudio/export/studio_codegen_native.hpp) | Emits the call and generated state. For other targets, review the generated report for unsupported properties. |

If you change the radius or colors, check preview and a generated C++ project. For a format change, open, save, and reopen the sample. Document the observable behavior in both languages. [Validation guide](VALIDATION.en.md)

## A template or exporter

Start with [CppBundle](../include/imguistudio/export/studio_cpp_export.hpp) and the [optional Android template](../include/imguistudio/export/studio_android_export.hpp). Validate the descriptor, preserve `codegen_report.txt`, include required dependencies, and document the build command. Review the license and source revision when integrating an extension. Include applicable third-party notices in generated projects.

## A translation

Add or update the key in [English/Spanish](../include/imguistudio/localization/studio_locale_strings.inc) and its counterpart in [the additional catalog](../include/imguistudio/localization/studio_locale_extra.inc). Route labels through localization and preserve the ImGui ID when visible text varies. Check a longer language, a shorter language, and required glyphs. See [localization](LOCALIZATION.en.md) for external packs.
