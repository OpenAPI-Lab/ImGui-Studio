# How the code works

[Español](README.md)

Use this index to find what a file does, why it has a separate role, and what a contribution must preserve. Each chapter links to the code, describes its contract, and identifies concrete tests. The [source map](../SOURCE_MAP.en.md) provides a quick file lookup.

| Chapter | Where to start |
| --- | --- |
| [Model, serialization, and persistence](PROJECT.en.md) | JSON, tree validation, movement, saves, history, and recovery. |
| [Startup, editor UI, and canvas](EDITOR.en.md) | Graphics loop, editor state, selection, and preview rendering. |
| [Widgets and resources](WIDGET_RENDER.en.md) | Texture cache, charts, fields, library, and shared drawing. |
| [Localization](LOCALIZATION.en.md) | Stable keys, fonts, and translation catalogs. |
| [Build engine](BUILD.en.md) | Profiles, processes, export attempts, and diagnostics. |
| [Assistant and extensions](SERVICES.en.md) | Providers, secrets, extension catalog, and proposals. |
| [Generators and bundles](EXPORTERS.en.md) | How a design becomes source files and resources. |
| [Emitted helpers](EMITTED_CONTROLS.en.md) | Widget support code in target languages. |
| [Hosts and templates](RUNTIMES.en.md) | Windows, input, textures, and lifecycles in Vulkan, Android, and Apple targets. |
| [Build and packaging](DISTRIBUTION.en.md) | CMake, CI, CPack, and artifact verification. |
| [Tests and fixtures](TESTS.en.md) | What each test verifies and why it exists. |

Follow the [contribution guide](../../CONTRIBUTING.en.md) for your first change. The [circular progress walkthrough](../EXTENDING.en.md#example-code-walkthrough) traces a property from serialization to emitted code.
