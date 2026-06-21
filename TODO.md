# TODO

## Keyboard shortcuts
- F8 — copy original to translated
- F9 — set status to In Progress
- F10 — set status to Translated
- Del — set status to Untranslated (clear translated column)

## Load TES3 top-level files and generate annotations
Parse loaded plugin's FNAM, CELL, DIAL names and use them as annotation/glossary sources for the current dict.

## Scan Lua files for translatable strings
Parse OpenMW Lua scripts (l10n YAML, or string literals) to extract translatable text for dictionary creation.

## History should restore status
Undo/revert in yTranslator should also restore the entry's status (not just the text). Handle propagated, model, and other auto-set statuses correctly when reverting.

## Better difference highlighting in adapted_from panel
Improve the diff highlighting between the current text and the adapted_from source to make changes more visible.

## Hyperlink spec
Document how hyperlinks work in Morrowind dialogues, how yampt detects and applies them during conversion, and how the annotation system highlights them. Cover: DIAL topic matching, `@` prefix in INFO NAME, hyperlink insertion during convert with `--add-hyperlinks`, and the hyperlink highlighter in yTranslator.

## Polish UI localization
Move hardcoded UI strings to YAML l10n files so the interface can be displayed in Polish.

## Linux build support
No Windows-only dependencies identified. Needs CMake/build scripts for Linux.

## Merge as GUI option
Add explicit merge functionality in the GUI. Currently users can load multiple dicts and the merge happens implicitly during convert. Consider a dedicated merge button/dialog.
