# Security and reporting

[Español](SECURITY.md)


For issues that can expose data or execute unintended actions, use GitHub private reporting if enabled. If you cannot find a private channel, ask the maintainer for one before sharing details. Include the version, platform, impact, minimal reproduction, and redacted logs. Use the [contribution workflow](CONTRIBUTING.en.md) for ordinary issues.

## Data and execution

- Project and widget JSON can contain embedded images. Inspect them before attaching them. Keep credentials, history, and private data out of public reports.
- Run / Build executes tools and, when you select Run, the application they produce. Selected SDKs and paths enter the build chain with the permissions of the account running the editor.
- Sending a prompt sends it, recent context, and a design summary to the selected provider. Proposals are reviewed and applied through an explicit action. Keys are handled separately from projects.
- The catalog verifies pinned imgui-knobs files. Preview uses the adapter built into the editor.

See the [project contracts](docs/PROJECT_FORMAT.en.md) and [technical reference](docs/REFERENCE.en.md) for persistence and operation boundaries.
