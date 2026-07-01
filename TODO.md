# TODO

Sorted by effort. Settings dialog takes priority.

---

### ~~Reorganize all project folders [S]~~ ✓
Completed. Core `model/` split into `creator/`, `merger/`, `converter/`, `translator/`. `plugin_scan/` split into `scanner/` + `decoder/`. `file_list` moved to `io/`, `dict_kind` moved to `utility/`. Translator: `controller/` → `editor/`, `translate/` → `translator/`, `highlight/` → `highlighter/`.

### ~~Rename highlight files [S]~~ ✓
Completed. `composite_highlighter` → `editor_highlighter`, `syntax_highlighter` → `script_tokenizer`, `annotation_highlighter` → `glossary_highlighter`, `hyperlink_highlighter` → `topic_highlighter`. Both file names and class names renamed across the entire solution.

### Settings dialog for both apps [S, PRIORITY]
Settings dialog similar to Notepad++/Visual Studio. Covers both yTranslator and yEditor. Central place for keyboard shortcuts, paths, encoding, spell check language, translation provider API keys, theme preferences.

Replace `editor_config_t` (hand-rolled INI parser) with `QSettings` — Qt handles all of this natively. Eliminates ~200 lines of manual parsing/writing boilerplate.

Key setting: **Native language / Foreign language** pair (e.g. Native=PL, Foreign=EN). Selecting these auto-configures:
- Codepage/encoding (e.g. PL → Windows-1250, RU → Windows-1251)
- Spell check dictionary (e.g. PL → pl_PL, DE → de_DE)
- AI translation model target language (e.g. PL → eng_Latn→pol_Latn)
- Make-base default language tags
- Partial mode Hunspell dictionary (Foreign language → en_US, de_DE, etc.)

### Enable DeepL and Google translation providers [S]
`deepl_translator_t` and `google_translator_t` are fully implemented and compiled. The `source_combo_` dropdown is created but `setVisible(false)`. Unhide the combo, add all provider names to it, connect their `translation_finished` signals, and add an API key settings field for DeepL.

### Enable glossary function in translation tab [S]
`set_glossary_fn()` is declared and the member stored but never called from `main_window_t`. Wire it to `glossary_` so translations get glossary term replacements suggested alongside model output.

### `record_table_model_t::update_row` still takes `const std::string & status` [S]
Should be `status_t` — missed during the enum migration (task 17). Change parameter type and update callers.

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
- we need unit tests for that, so this chnage dont break anytrhing

### Better explanation of partial mode [S]
Clarify in the GUI (tooltip, help text, or info panel) how the English dictionary comparison works in partial mode.

### Hyperlink spec [S]
Document how hyperlinks work in Morrowind dialogues, how yampt detects and applies them during conversion, and how the annotation system highlights them.
- annotations have to highlitht them same way
- how @ is used, implement also

### Comprehensive esm_converter_t tests [S]
The current converter tests use synthetic ESM files assembled in memory. Add a small real ESM file (a few records of each type) to the test data and verify all `convert_*` functions produce correct output. Cover every record type: SCTX, BNAM, CELL, FNAM, INFO, DIAL, GMST, DESC, TEXT, RNAM, INDX, PGRD, ANAM, SCVR, DNAM, CNDT.

---

## M — a day or two

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
- we need unit tests for that, so this chnage dont break anytrhing

### Version / Release [M]
- First public release was build735
- current README show build735 state
- Update readme
- Fix build folder structure
- Update documents
- Include documents
- we have hardcoded version 0.25 in code, in vcpkg.json
- I want version to be 0.gitbuild, so 0.735 for example

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

### Generate TOP/CEL/MRK files from dictionaries [XL]
Create `.top`, `.cel`, `.mrk` files based on DIAL/CELL dictionaries and the language spell checker dictionary. These are OpenMW cell/topic name files used by the engine for localized map markers and journal topics. Include generated files as annotation sources in yTranslator.

### Lua handlers comparison/conflicts detector (yEditor) [XL]
Detect conflicts between OpenMW Lua handler registrations across multiple mods (e.g. two mods registering `addSkillLevelUpHandler` that both return `false`). Show conflicts in yEditor's comparison view.

---

## Refactoring — later

### Split `tools_t` into focused utilities
`tools_t` is a god class: data types (`dict_t`, `chapter_t`, `record_entry_t`, `rec_type_t`), file I/O (`read_file`, `write_text`, `write_file`), byte conversion, string manipulation, and global logging state. Split into:
- `dict_types.hpp` — `dict_t`, `chapter_t`, `record_entry_t`, `rec_type_t` (already partially in `record_types.hpp`/`status_types.hpp`)
- `esm_file_io.hpp` — `read_file`, `write_text`, `write_file`, `create_file`
- `log.hpp` — `add_log`, `get_log`, `reset_log`, `has_error`, `set_debug`, `set_quiet`
- `byte_utils.hpp` — `convert_string_byte_array_to_uint`, `convert_uint_to_string_byte_array`

### Split `dict_creator_t` (header: 230 lines, base impl: 1937 lines)
The class mixes unrelated matching algorithms behind one interface. Extract:
- Cell matching (fingerprint, heuristic, exterior coords, default, region) → `cell_matcher_t`
- DIAL matching (INAM index, translation-based) → `dial_matcher_t`
- Script matching (SCTX/BNAM native message pairing) → stays in `dict_creator_base.cpp` (less code)
- Text-match index (adaptation, ambiguity detection) → `text_match_index_t`

### Reduce `main_window_t` (1923 + 852 lines)
Extract self-contained concerns into dedicated classes:
- Workspace/watcher logic → `workspace_watcher_t`
- Commit+propagation logic → already in `editor_controller_t` but `main_window` still has `commit_dict_edit`, `commit_yaml_edit`, `sync_propagated_rows` — push into controller
- Highlight management (`extra_selections_state_t`, `apply_extra_selections`, `find_annotation_highlights`, `build_highlight_selections`) → `highlight_coordinator_t`

### Reduce `plugin_workspace_view_t` (1316 lines) in yEditor
Does loading, parsing, merging, filtering, drag-drop, persistence — all in one "view". Also embeds two child tree views (nav + record view) and a messages panel instead of composing separate widgets. Extract:
- Plugin loading + unloading orchestration → `plugin_session_t` in `model/`
- Merge patch creation (`create_merge_records`, `refresh_after_merge`) → `patch_builder_t` in `patcher/`
- Nav tree (left panel) → `nav_tree_view_t` in `view/` (owns its `QTreeView` + nav_tree_model)
- Record view tree (right panel) → `record_view_t` in `view/` (owns its `QTreeView` + view_tree_model)
- The parent view keeps only: toolbar, splitter layout, connect signals between child views and model classes

### Split `plugin_scan_t` god class (757 lines)
Mixes: loading, conflict detection, merge plugin management, TES3 binary header construction, leveled list merging, dialogue merging, ITM detection. Extract:
- `patch_builder_t` in `yampt.editor/patcher/` — `copy_record_to_merge`, `remove_from_merge`, `save_merge`, `build_tes3_header`, `merge_leveled_list`, `merge_dialogue`
- Slimmed `plugin_scan_t` stays in core `scanner/` — loading, indexing, conflict detection, ITM only

### Split `view_tree_decode.cpp` (1166 lines) in yEditor
Contains 5+ unrelated record-type decoders. Split by family:
- `view_tree_decode_cell.cpp` — CELL ref groups, DODT matching, object indices
- `view_tree_decode_lists.cpp` — leveled lists, factions, containers
- `view_tree_decode_generic.cpp` — schema-based children, hex dump fallback

### Split `conflict_slots.cpp` (742 lines)
Five independent strategy implementations behind one dispatch. Extract:
- `conflict_slots_cell.cpp` — CELL ref group slot building (most complex)
- Keep leveled/faction/container strategies in main file (shorter, similar pattern)

### Split `esm_converter.cpp` (760 lines)
16 independent `convert_*` methods + shared helpers. Split:
- `esm_converter.cpp` — constructor, dispatcher, shared helpers
- `esm_converter_records.cpp` — all per-record-type conversion methods

### Extract color logic from `nav_tree_model.cpp` (733 lines)
The model's `data()` method computes QBrush colors from conflict enums. Extract conflict-to-color mapping to a shared `conflict_colors.hpp` utility used by both nav_tree_model and view_tree_model.

### Rename: view_tree_ → record_tree
Rename view_tree_model, view_tree_decode, and related types/files to record_tree_* for clarity.

---

## Feature Ideas — Replacing External Tools

Features observed in xTranslator, EET4, TESTool, tes3cmd, MWEdit, TES3Merge, xEdit, Wrye Mash, and Enchanted Editor that yampt/yTranslator/yEditor could absorb.

### Plugin Cleaning (TESTool, tes3cmd, xEdit)
- Remove dirty GMST records (including the "evil 72")
- Remove records identical to master (ITM — Identical To Master)
- Remove identical cell references inside CELL records
- Remove identical AMBI/WHGT fields from cells
- Remove empty CELL records (no refs, no LAND/PGRD)
- Remove empty DIAL records (no INFOs, unless journal type)
- Remove duplicate objects (same ID, two records)
- Restricted vs. aggressive cleaning modes
- Batch clean multiple plugins at once
- Preserve file timestamps option
- yEditor already has ITM detection — expose as a one-click clean action

### Conflict Report Generation (TESTool, xEdit)
- Generate plain-text conflict report for a set of plugins
- Show which records conflict across which plugins
- Optionally skip mergeable conflicts (leveled lists, objects)
- yEditor already shows this visually — add export-to-text option

### Leveled List Merging (TESTool, TES3Merge)
- Merge leveled creature + leveled item lists across active plugins
- Only add new entries (minimum necessary to cover all sources)
- Merge PC Level / Each Item flags (any-enabled wins)
- None chance = minimum non-zero value across sources
- yEditor already has this infrastructure — expose and polish

### Object Merging (TESTool)
- Identify records defined in multiple plugins with different field changes
- Merge non-conflicting field changes (one mod changes weight, another changes model)
- Attribute-level comparison for packed sub-records (WPDT, NPDT, AODT, etc.)
- Last-modified-date wins for true conflicts
- Generate Merged_Objects.esp
- yEditor has partial support — extend to all object types

### Dialogue Merging (TESTool)
- Merge dialogue INFO ordering conflicts between plugins
- Preserve correct INFO chain (PNAM/NNAM links)
- Generate Merged_Dialogs.esp

### Plugin Header Management (TESTool, Wrye Mash)
- Update master file sizes in plugin headers
- Update plugin version to 1.3
- Reassign masters (Wrye Mash's master swap feature)
- Change load order metadata

### CSV Import/Export (MWEdit)
- Export records (NPCs, weapons, spells, etc.) to CSV for spreadsheet editing
- Import CSV back into plugin
- Useful for bulk stat changes, item rebalancing, translation via spreadsheet

### Script Compiler with Diagnostics (MWEdit)
- Full MWScript compiler with better error messages than TESCS
- Color-coded script editor
- Function parameter type checking (expects NPC ID vs item ID)
- Detect broken/deprecated functions
- Warn about reserved words used as variables
- Max length checks for variables and statements
- Three warning levels (weak/default/strong)
- Script import/export to text files
- Script templates

### Find Text Across All Records (MWEdit, xEdit)
- Search all loaded records for text string matches
- Results list with record ID, type, and context
- Double-click to jump to/edit the record
- Regex support (xTranslator)

### Batch Auto-Translation (xTranslator)
- Apply dictionary matches automatically to all untranslated entries
- Heuristic matching with confidence threshold
- Online translation API integration (already partially in yampt: DeepL, Google, CTranslate2)

### Diff Viewer Between Plugin Versions (xTranslator)
- Show differences between original and updated source strings
- Useful when a mod updates and translations need review

### BSA/BA2 Archive Extraction (xTranslator, xEdit)
- Extract files from BSA archives for inspection
- Useful for finding mesh/texture/sound paths referenced in records

### Regex Search and Replace (xTranslator)
- Regex-based find/replace across translations
- Batch mode (apply to all matching entries)

### Alias/Variable Integrity Check (xTranslator)
- Verify that format variables (%s, %d, {0}, etc.) in translations match the source
- Flag entries where placeholders are missing or reordered

### Plugin Active List Management (TESTool, Wrye Mash)
- View/edit active plugin list
- Sort by load order, date, dependencies
- One-click "fix load order" based on master dependencies

### Record Copying Between Plugins (MWEdit)
- Drag records from one loaded plugin to another
- Copy/rename dialogue topics with all child INFOs
- Duplicate INFO records within a topic

### Automatic Backup on Save (MWEdit)
- Sequential backup files (.001, .002, .003...)
- Never overwrite a previous backup
- Complete edit history through file versions

### "Just Fix It" One-Click Mode (TESTool)
- Single button that runs: clean all active plugins + merge leveled lists + merge objects
- No user interaction needed beyond clicking the button
- Good for end-users who don't understand individual operations

### PEX Script Decompilation (xTranslator)
- Decompile Papyrus PEX scripts for translation
- Not directly relevant for Morrowind but relevant if yampt expands to Skyrim/Fallout

### Custom Dictionary / Glossary During Translation (xTranslator)
- Inline dictionary built from existing translation pairs
- Suggests translations as you type
- yampt already has annotations/glossary — could add autocomplete suggestions

### Fuz (Voice File) Mapping and Player (xTranslator)
- Map dialogue entries to their voice .fuz files
- Play audio for the current entry to help with context
- For Morrowind: map Say commands to .wav files

### Encoding Detection and Conversion (xTranslator)
- Support all known encodings for each game
- Detect encoding automatically from file content
- yampt already has Windows-1250/1251/1252 — could add more (Cyrillic, Asian)
