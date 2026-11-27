# Localization coverage

[Español](LOCALIZATION.md)

The editor includes English, Spanish, French, Korean, Japanese, and Simplified Chinese. All six built-in catalogs provide a translation for every catalog key, including the editor's native controls.

Some strings are displayed in their original form:

- Specialized widget choices such as chart variants, icons, and visual profiles.
- Internal messages from exporters, dependencies, history, and assistant services. The Build panel's status and tool detection labels use the interface language.
- Compiler, operating system, and provider output.
- Names and descriptions supplied by extension authors or widget creators.

Documents, code, project widget names, and conversations retain their authors' text. The input test feeds UTF-8 text into Dear ImGui. The glyph test checks all six catalogs against the editor's font atlas. System IME composition still requires validation.

Community language packs use a JSON catalog. Follow the [language pack instructions](REFERENCE.en.md#interface-language-and-fonts) for the schema and discovery rules. Keep ImGui widget IDs stable as visible labels change. Fonts come from the host or the editor's state directory, so coverage depends on the fonts available there. Run `--font-smoke-test` to check the loaded atlas.
