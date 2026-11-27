# Localization implementation

[Español](LOCALIZATION.md)

Labels resolve through stable keys. Translation can change visible text while identity stays with the same ImGui control.

## File contracts

| File | What it does | Why it exists and what to preserve |
| --- | --- | --- |
| [studio_locale.hpp](../../include/imguistudio/localization/studio_locale.hpp) | `StudioLocale` resolves catalogs and `ScopedStudioLocale` installs the active context for a surface. | `surfaceText` handles plain text and `surfaceLabel` preserves identity. Missing or invalid translations fall back to English. |
| [studio_locale_strings.inc](../../include/imguistudio/localization/studio_locale_strings.inc) | Defines `StudioText` and the base Spanish and English text through `STUDIO_TEXT`. | The same list feeds the enum and catalogs. Preserve keys used by community packs. |
| [studio_locale_extra.inc](../../include/imguistudio/localization/studio_locale_extra.inc) | Registers the other four built-in translations by key. | Update its entries when introducing a key and verify atlas coverage for their characters. |

## Example and verification

Use `surfaceLabel(StudioText::SaveProject)` for a control that needs identity. Use `surfaceText` for non-interactive text. Switch languages without losing the control's state and run `--font-smoke-test` with the relevant fonts available. [Language guide](../LOCALIZATION.en.md) · [Font atlas](../../src/app/studio_fonts.hpp)

