# TODO

## Keyboard shortcuts
- F8 — copy original to translated
- F9 — set status to In Progress
- F10 — set status to Translated
- Del — set status to Untranslated (clear translated column)

## History should restore status
Undo/revert in yTranslator should also restore the entry's status (not just the text). Handle propagated, model, and other auto-set statuses correctly when reverting.

## Polish UI localization
Move hardcoded UI strings to YAML l10n files so the interface can be displayed in Polish.

## Linux build support
No Windows-only dependencies identified. Needs CMake/build scripts for Linux.

## Merge as GUI option
Add explicit merge functionality in the GUI. Currently users can load multiple dicts and the merge happens implicitly during convert. Consider a dedicated merge button/dialog.
