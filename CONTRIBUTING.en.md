# Contributing

[Español](CONTRIBUTING.md)


Read the [source map](docs/SOURCE_MAP.en.md) and the [support matrix](docs/CAPABILITIES.en.md).
[How the code works](docs/code/README.en.md) describes file contracts and lists the tests that cover each module.
Describe current behavior, expected behavior, and affected platforms in each contribution.

## Your first contribution

- **Documentation.** Open an example from `examples/`, follow the procedure, update both languages, and check the links. If a UI label changes, verify its spelling in the interface catalog before documenting it.
- **Editor.** Start in the canvas for selection issues. Inspect the inspector and preview for value or drawing issues. Include a minimal JSON design and test in both design and preview modes. [Source map](docs/SOURCE_MAP.en.md)
- **Exports.** Reduce the design to one widget and export the affected language. Include the relevant excerpt from `codegen_report.txt` and identify whether generation, compilation, or startup failed. Specify the graphics backend and any required export pack. [Export guide](docs/EXPORTING.en.md)

## Run the checks

From the repository root, configure a core-only build. It covers format changes, tree edits, and recovery and runs without a graphical session.

```sh
python tools/check_docs.py
cmake -S . -B build/contrib-core -G Ninja -DBUILD_STUDIO_GUI=OFF
cmake --build build/contrib-core --target verify_persistence verify_sessions verify_document_edit
ctest --test-dir build/contrib-core -R "^(ProjectPersistenceTest|SessionRecoveryTest|DocumentEditingTest)$" --output-on-failure --no-tests=error
```

For graphics changes, use the [build guide](docs/BUILD_AND_TEST.en.md) to build the editor and the [validation guide](docs/VALIDATION.en.md) to select tests in a graphical session. In your pull request, include the command, result, and host. For UI screenshots, include the steps to reach the screen.

## Report and propose

- Open an issue with the version or commit, OS, architecture, compiler, and relevant backend. On Linux, specify Wayland, X11, or WSLg. For export issues, identify the language and destination.
- Attach a small sample project and reproducible steps. Include the relevant Run / Build output or excerpt from `codegen_report.txt`.
- For features, explain who uses them, where they appear in the editor, and which files or outputs change. Use the [security policy](SECURITY.en.md) for sensitive issues.

## Prepare a change

1. Create a branch and build using the [development guide](docs/BUILD_AND_TEST.en.md).
2. Trace the existing path through the model, editor, persistence, and exporters. Reuse behavior across preview and export where possible.
3. When changing the file format, test a project saved by an earlier version. Preserve extra data and history where applicable.
4. Add or adjust a test when the changed behavior lacks coverage. Use the [validation guide](docs/VALIDATION.en.md) to select tests and update both versions of the relevant guide.
5. Open a pull request with your rationale, test steps, results, and pending platforms. A screenshot helps reviewers check hierarchy, spacing, and translations for visual changes.

The [extension guide](docs/EXTENDING.en.md) walks through widgets, translations, and exporters. Preserve the license and provenance of any dependency you integrate.

Run the [documentation check](docs/CI.en.md#repeat-the-documentation-check) before submitting path or link changes. In your pull request, identify checks that need graphics or specific hardware.
