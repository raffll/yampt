# TODO

Sorted by effort.

## L — a week or more

### Show optional absent sub-records in grey [L]
xEdit shows sub-records that are defined in the record schema but absent from the actual data as greyed-out placeholder rows (e.g. SCRI - ScriptID, CNAM - Female Body Part Name, ENAM - EnchantID). Requires a canonical sub-record order definition per record type. Missing entries appear in their expected position with grey text and empty plugin columns.

### Cell reference hierarchy in view tree [L]
Cell references are currently displayed flat in the right panel. Refactor to show them nested:
- Cell Header sub-records at top level (DATA, AMBI, WHGT, etc.)
- Persistent group node (references before NAM0)
- Temporary group node (references after NAM0)
- Each reference as a collapsible node labeled with object index + NAME ID
- Reference sub-records (FRMR, NAME, DODT, DNAM, XSCL, ANAM, DATA) as children of their reference node
Matches xEdit's cell reference layout.

### DIAL INFO list alignment in view tree [M]
When a DIAL record is selected, show a list of INFO INAM IDs below the DIAL sub-records, aligned/paired across plugins by INAM key. Shows which INFOs each plugin has and where mods insert new entries in the INFO chain. Same alignment pattern as CELL FRMR references — matched by ID, unmatched entries show as empty in plugins that don't have them.

### Codepage-aware text display in yEditor [S]
`format_value` currently hardcodes Windows-1252 for codepage-to-UTF-8 conversion. Should read the codepage from app settings (same setting as the merge/CLI uses: 1252 for English, 1250 for Polish, 1251 for Russian). Pass it through to the view tree formatting layer.

### Truncate long text in view tree cells + preview pane [M]
Multi-line sub-record values (BOOK TEXT, SCPT SCTX) misalign across columns because rows have different text lengths. Fix:
1. Truncate to a single line in the tree cell (show first ~80 chars + ellipsis). Full text available via tooltip.
2. Add a preview pane below the view tree that shows the full content of the selected cell. Clicking a TEXT cell displays its complete text in the preview.

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

### Split `view_tree_decode.cpp` (1166 lines) in yEditor — DONE
Split into view_tree_decode.cpp (shared), view_tree_decode_cell.cpp, view_tree_decode_lists.cpp.

### Split `conflict_slots.cpp` (742 lines) — DONE
Extracted conflict_slots_cell.cpp with dedicated conflict_slots_cell.hpp header.

### Split `esm_converter.cpp` (760 lines) — DONE
Split into esm_converter.cpp (shared helpers) and esm_converter_records.cpp.

### Extract color logic from `nav_tree_model.cpp` (733 lines) — DONE
Already extracted to conflict_types.hpp (lighter_hsl, conflict_all_background, conflict_this_foreground).

### Rename: view_tree_ → record_tree — LATER
Rename view_tree_model, view_tree_decode, and related types/files to record_tree_* for clarity. Deferred — large rename touching MOC files, best done with VS refactoring tools.

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
