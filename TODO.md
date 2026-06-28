# TODO

Sorted by effort. Settings dialog takes priority.

---

## S — a few hours

### Settings dialog for both apps [S, PRIORITY]
Settings dialog similar to Notepad++/Visual Studio. Covers both yTranslator and yEditor. Central place for keyboard shortcuts, paths, encoding, spell check language, translation provider API keys, theme preferences.

Key setting: **Native language / Foreign language** pair (e.g. Native=PL, Foreign=EN). Selecting these auto-configures:
- Codepage/encoding (e.g. PL → Windows-1250, RU → Windows-1251)
- Spell check dictionary (e.g. PL → pl_PL, DE → de_DE)
- AI translation model target language (e.g. PL → eng_Latn→pol_Latn)
- Make-base default language tags
- Partial mode Hunspell dictionary (Foreign language → en_US, de_DE, etc.)

### Enable DeepL and Google translation providers [S]
`deepl_provider_t` and `google_provider_t` are fully implemented and compiled. The `source_combo_` dropdown is created but `setVisible(false)`. Unhide the combo, add all provider names to it, connect their `translation_finished` signals, and add an API key settings field for DeepL.

### Enable glossary function in translation tab [S]
`set_glossary_fn()` is declared and the member stored but never called from `main_window_t`. Wire it to `annotation_manager_` so translations get glossary term replacements suggested alongside model output.

### Remove dead `search_manager_t` [S]
Complete class in `controller/search_manager.hpp/.cpp`, compiled but never included or used anywhere. Superseded by `search_engine_t`. Remove from vcxproj and delete files.

### Remove `model_downloader_t` [S]
Model will be shipped separately. Delete `provider/model_downloader.hpp/.cpp`, remove from vcxproj.

### Expose yEditor toolbar buttons [S]
7 buttons (`btn_load_`, `btn_new_`, `btn_save_`, `btn_merge_`, `btn_filter_`, `cmb_type_filter_`, `edt_search_`) are fully functional but all `setVisible(false)`. Also the Save menu action is hidden. Unhide when ready.

### Keyboard shortcuts [S]
- F8 — copy original to translated
- F9 — set status to In Progress
- F10 — set status to Translated
- Del — set status to Untranslated (clear translated column)

No shortcut handling code exists yet (`QKeySequence`/`setShortcut` not used anywhere in translator). Shortcuts panel lives inside the settings dialog.

### History should restore status [S]
Undo/revert in yTranslator should also restore the entry's status (not just the text). Handle propagated, model, and other auto-set statuses correctly when reverting.

### Script parser: skip bytecode modification when no translation found [S]
When `new_text == old_text` (cell/topic not in dictionary), the script parser still runs the full erase/insert cycle on SCTX and SCDT (which is a no-op but risks size byte validation false positives). After the size byte validation loop confirms the correct bytecode occurrence, check `if (new_text == old_text)` and advance `pos_c += old_text.size()` then return. The `insert_new_text` function should also early-return when `new_text == old_text`.

### Better explanation of partial mode [S]
Clarify in the GUI (tooltip, help text, or info panel) how the English dictionary comparison works in partial mode.

### Hyperlink spec [S]
Document how hyperlinks work in Morrowind dialogues, how yampt detects and applies them during conversion, and how the annotation system highlights them.

---

## M — a day or two

### Enable model downloader in GUI [M] — CANCELLED
~~Model will be shipped separately. Remove `model_downloader_t` instead (see S tasks).~~

### Better difference highlighting in adapted_from panel [M]
Improve the diff highlighting between the current text and the adapted_from source to make changes more visible.

### Merge as GUI option [M]
Add explicit merge functionality in the GUI. Currently users can load multiple dicts and the merge happens implicitly during convert. Add a dedicated merge button/dialog in yTranslator.

### Configurable source dictionary for partial mode [M]
Allow changing the Hunspell dictionary used in partial mode from English to another language (e.g. German, French). Currently hardcoded to `en_US.aff`/`en_US.dic`. Add a dropdown or file selector in the make-base dialog.

### Load TES3 top-level files and generate annotations [M]
Parse loaded plugin's FNAM, CELL, DIAL names and use them as annotation/glossary sources for the current dict. `plugin_document_t` exists as a stub placeholder — all methods are empty/return defaults.

### Hyperlinks more accurate [M]
Read OpenMW source code for exact hyperlink detection rules and improve matching accuracy in yampt's annotation/conversion system.

### EET file format converter to JSON [M]
Import `.eet` files (ESP-ESM Translator binary format) and convert to yampt's JSON dictionary format. The EET format is already reverse-engineered and documented in steering.

### Script parser: unquoted multi-word cell names [M]
Commands like `ShowMap Ald Velothi` use unquoted multi-word cell names. The token extractor only grabs the first word. Need special handling for commands where the cell name is the last parameter — take everything to end-of-line. Low frequency in practice (most mods use quotes).

### Version / Release [M]
- First public release was build735
- README show 735 state
- Update readme
- Fix build folder structure
- Update documents
- Include documents

---

## L — a week or more

### Merged Patch (yEditor) [L]
The merge infrastructure exists in yEditor (hidden `btn_merge_`, `on_create_merged_patch` handler, leveled list + dialogue merging, drag-and-drop support) but the workflow isn't exposed yet. Remaining:
- Default folder for folder/MO2/OpenMW
- Settings dialog, configurable paths
- Show button/dialog
- Make sure good conflicts are merged
- Show/hide merged patch column
- Allow drag and drop to merged patch

### Expand all remaining hex sub-records in yEditor [L]
Many sub-records still display as raw hex dumps. Add schemas for all common sub-records using OpenMW source as reference. Priority: DIAL DATA, DOOR DATA, LOCK DATA, AI_T, AI_F, AI_E, AI_A, BSGN, NAM0, WHGT, NAM5, XSCL, INTV, FLTV, INDX.

### Dark theme [L]
- Dark theme
- Unhardcode colors (status_colors.hpp, grammar_checker warning_format, etc.)
- Match better colors for dark theme

### Polish UI localization [L]
Move hardcoded UI strings to YAML l10n files so the interface can be displayed in Polish.

### Scan Lua files for translatable strings [L]
Parse OpenMW Lua scripts (l10n YAML, or string literals) to extract translatable text for dictionary creation.

---

## XL — multi-week

### Linux build support [XL]
No Windows-only dependencies identified. Needs CMakeLists.txt for Linux.
