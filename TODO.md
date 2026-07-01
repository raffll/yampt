# TODO

Sorted by effort.

## L — a week or more

### Merged Patch (yEditor) [L]
The merge infrastructure exists in yEditor (toolbar exposed, settings paths wired, session state saved). Remaining:
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
