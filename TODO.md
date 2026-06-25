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

## Script parser: unquoted multi-word cell names
Commands like `ShowMap Ald Velothi` use unquoted multi-word cell names. The regex-based token extractor only grabs the first word (`Ald`). Need special handling for commands where the cell name is the last parameter — take everything to end-of-line. Affects `ShowMap`, `CenterOnCell`, and potentially others. Low frequency in practice (most mods use quotes).

## Script parser: skip bytecode modification when no translation found
When `new_text == old_text` (cell/topic not in dictionary), the script parser currently still runs the full erase/insert cycle on SCTX and SCDT (which is a no-op but risks size byte validation false positives). Solution: after the size byte validation loop confirms the correct bytecode occurrence, check `if (new_text == old_text)` and advance `pos_c += old_text.size()` then return — skipping the actual modification. The `insert_new_text` function should also early-return when `new_text == old_text`. This prevents the "unknown error" bug with mod scripts like `"ulyne henim"->PositionCell, pX1, pY1, pZ1, 0, "GetPCCell"` where a function name is misused as a cell parameter.

## Polish UI localization
Move hardcoded UI strings to YAML l10n files so the interface can be displayed in Polish.

## Linux build support
No Windows-only dependencies identified. Needs CMake/build scripts for Linux.

## Merge as GUI option
Add explicit merge functionality in the GUI. Currently users can load multiple dicts and the merge happens implicitly during convert. Consider a dedicated merge button/dialog.

## Better explanation of partial mode dictionary comparison
Clarify in the GUI (tooltip, help text, or info panel) how the English dictionary comparison works in partial mode: what "To Verify" means (identical text passed through the dictionary check — no English words found, likely a proper noun but needs confirmation), versus "Untranslated" (English words detected — genuinely untranslated).

## Configurable source dictionary for partial mode
Allow changing the Hunspell dictionary used in partial mode from English to another language (e.g. German, French). Currently hardcoded to `en_US.aff`/`en_US.dic`. Should be a dropdown or file selector in the make-base dialog.

## Expand all remaining hex sub-records in yEditor
Many sub-records still display as raw hex dumps (`<N bytes>` with `0000: XX XX XX...`). Add schemas for all common sub-records using OpenMW source as reference for field layouts. Priority records: DIAL DATA, DOOR DATA, LOCK DATA, AI_T, AI_F, AI_E, AI_A, BSGN, NAM0, WHGT, NAM5, XSCL, INTV, FLTV, INDX (skill/magic effect index).
