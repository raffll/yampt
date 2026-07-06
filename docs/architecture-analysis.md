# Architecture Analysis

Total: 275 source files, 33,251 lines of code across 5 projects.

## Project Overview

| Project | Role | Output | Lines |
|---------|------|--------|------:|
| yampt.core | Pure C++ library (no Qt) | yampt.core.lib | ~13,500 |
| yampt.qt | Qt bridge library | yampt.qt.lib | ~800 |
| yampt.cli | Console application | yampt.exe | ~250 |
| yampt.translator | Translation workbench (Qt6) | yTranslator.exe | ~12,000 |
| yampt.editor | Plugin conflict viewer (Qt6) | yEditor.exe | ~6,700 |

---

## File Responsibilities

### yampt.core/source/utility/

| File | Lines | Responsibility |
|------|------:|----------------|
| tools.hpp/cpp | 174+310 | God-class: logging, file I/O, byte conversion, dict types |
| string_utils.hpp | ~50 | String helpers (to_lower, trim, normalize_path) |
| record_types.hpp | ~60 | Record type constants and lists |
| status_types.hpp | ~30 | status_t enum definition |
| record_behavior.hpp/cpp | ~150 | Record type → behavior mapping |
| keyword_trie.hpp/cpp | ~120 | Aho-Corasick trie for hyperlink insertion |
| char_diff.hpp/cpp | ~80 | Character-level diff algorithm |
| includes.hpp | ~20 | Common standard library includes |
| color_palette.hpp | ~30 | Color constants for conflict display |
| dict_kind.hpp | ~15 | dict_kind_t enum |
| theme_enums.hpp | ~15 | Theme enumeration |

### yampt.core/source/io/

| File | Lines | Responsibility |
|------|------:|----------------|
| esm_reader.hpp/cpp | 90+175 | Binary ESM/ESP file parser |
| dict_reader.hpp/cpp | ~80+200 | JSON/XML dictionary reader |
| dict_writer.hpp/cpp | ~50+170 | JSON dictionary writer |
| codepage.hpp/cpp | ~40+130 | Windows codepage encode/decode |
| file_list.hpp/cpp | ~60+218 | Folder scanning and file classification |
| eet_reader.hpp/cpp | ~40+150 | EET binary format reader |
| eet_converter.hpp/cpp | ~30+130 | EET → yampt dict conversion |
| yaml_l10n_reader.hpp/cpp | ~40+100 | YAML localization file reader |
| yaml_l10n_writer.hpp/cpp | ~30+80 | YAML localization file writer |
| json_reader.hpp | ~30 | yyjson wrapper helpers |
| profile_reader.hpp/cpp | ~30+100 | MO2/OpenMW profile path reader |

### yampt.core/source/converter/

| File | Lines | Responsibility |
|------|------:|----------------|
| esm_converter.hpp/cpp | 89+179 | Plugin conversion orchestration |
| esm_converter_records.cpp | 518 | Per-record-type conversion logic |
| script_parser.hpp/cpp | 72+503 | MWScript text + SCDT bytecode translator |
| scdt_patcher.hpp/cpp | ~50+160 | Compiled bytecode binary patching |

### yampt.core/source/creator/

| File | Lines | Responsibility |
|------|------:|----------------|
| dict_creator.hpp | 215 | Class declaration (60+ methods, 8+ index maps) |
| dict_creator.cpp | 369 | Shared helpers (constructors, counters, script messages) |
| dict_creator_base.cpp | 761 | Base-mode matching (two ESMs → translation pairs) |
| dict_creator_base_ordered.cpp | 520 | Ordered base mode (record-by-record iteration) |
| dict_creator_single.cpp | 184 | Single-file mode + single-with-base mode |
| cell_matcher.hpp/cpp | ~60+493 | Cell name heuristic matching with translation engine |
| dial_matcher.hpp/cpp | ~40+345 | DIAL topic fingerprint matching |
| text_match_index.hpp/cpp | ~40+130 | Secondary old_text → entry lookup index |
| word_match_utils.hpp/cpp | ~30+80 | Word overlap scoring for heuristic matches |

### yampt.core/source/merger/

| File | Lines | Responsibility |
|------|------:|----------------|
| dict_merger.hpp/cpp | ~60+170 | Dictionary merge (last-listed wins via reverse iteration) |

### yampt.core/source/scanner/

| File | Lines | Responsibility |
|------|------:|----------------|
| plugin_scan.hpp/cpp | 122+238 | Plugin loading + conflict detection |
| plugin_scan_merge.cpp | 326 | Merge patch record management (copy/pin/remove) |
| record_conflict.hpp/cpp | 19+172 | Conflict computation (free functions) |
| record_conflicts.hpp | ~20 | Conflict entry collection type |
| plugin_index.hpp/cpp | ~50+275 | Per-plugin record index (type+id → record_index) |
| sub_record_merge.hpp/cpp | ~50+802 | Sub-record level merge logic |
| merge_patch_ops.hpp/cpp | ~40+170 | High-level merge patch operations |
| auto_merge.hpp/cpp | ~30+349 | Automatic merge for leveled lists + dialogue |
| dial_info_align.hpp/cpp | ~40+150 | INFO record chain alignment for dialogue merge |
| cell_name_fixer.hpp/cpp | ~30+130 | Corrects cell names in DNAM sub-records |
| fog_fixer.hpp/cpp | ~30+130 | Fixes fog density values in merged cells |
| summon_fixer.hpp/cpp | ~30+100 | Fixes summoned creature IDs |
| conflict_enums.hpp | ~50 | conflict_all_t, conflict_this_t enums |

### yampt.core/source/decoder/

| File | Lines | Responsibility |
|------|------:|----------------|
| sub_record_schema.hpp/cpp | 55+947 | Static schema tables (field definitions for all record types) |
| conflict_slots.hpp/cpp | 36+557 | Sub-record slot alignment for generic records |
| conflict_slots_cell.hpp/cpp | 5+400 | Cell-specific slot alignment (FRMR grouping) |
| content_alignment.hpp/cpp | ~50+200 | Content alignment utilities |
| view_tree_format.hpp/cpp | ~40+552 | Field value formatting (hex, flags, enums → display) |
| view_group_def.hpp/cpp | ~30+130 | Group definitions for container/leveled display modes |
| sub_record_iter.hpp | ~60 | Sub-record view iterator (zero-copy parsing) |

### yampt.core/source/translator/

| File | Lines | Responsibility |
|------|------:|----------------|
| translation_engine.hpp/cpp | ~40+175 | CTranslate2 model loading and inference |

### yampt.qt/source/

| File | Lines | Responsibility |
|------|------:|----------------|
| theme_system.hpp/cpp | ~40+191 | Theme switching (dark/light/system) |
| settings_store.hpp/cpp | ~60+414 | QSettings wrapper for INI persistence |
| path_resolver.hpp/cpp | ~30+80 | Platform-aware path resolution |
| conflict_types.hpp | ~30 | QColor helpers for conflict display |

### yampt.translator/source/ (root)

| File | Lines | Responsibility |
|------|------:|----------------|
| main_window.hpp | 212 | God-class header (~50 members, ~40 methods) |
| main_window.cpp | 995 | Core window logic, signals, table rebuild |
| main_window_setup.cpp | 634 | UI setup methods (menus, toolbar, panels) |
| main.cpp | ~30 | Application entry point |

### yampt.translator/source/controller/

| File | Lines | Responsibility |
|------|------:|----------------|
| editor_controller.hpp/cpp | 76+224 | Edit commit, propagation, status changes |
| record_display_controller.hpp/cpp | ~50+130 | Record display logic coordination |

### yampt.translator/source/model/

| File | Lines | Responsibility |
|------|------:|----------------|
| document.hpp | ~40 | Abstract document_t interface |
| dict_document.hpp/cpp | ~60+199 | Dict document (wraps dict_slot) |
| yaml_document.hpp/cpp | ~50+192 | YAML l10n document |
| plugin_document.hpp | ~30 | Plugin document (read-only ESM view) |
| record_table_model.hpp/cpp | 28+264 | Qt table model for record display |
| sidebar_model.hpp/cpp | ~50+245 | Sidebar tree render model |
| table_builder.hpp/cpp | ~40+411 | Table row construction from dict data |
| table_row.hpp | ~30 | table_row_t struct |
| row_source.hpp | ~15 | Row source interface |
| filter_state.hpp | ~20 | Filter state struct |
| make_base_params.hpp | ~15 | Make-base dialog result struct |
| plugin_op.hpp | ~15 | Plugin operation enum |
| menu_action.hpp | ~15 | Menu action enum |

### yampt.translator/source/editor/

| File | Lines | Responsibility |
|------|------:|----------------|
| operation_executor.hpp/cpp | 52+166 | CLI operation wrapper (make-dict, convert, etc.) |
| glossary.hpp/cpp | ~60+200 | Annotation/glossary term building |
| edit_history.hpp/cpp | ~40+150 | Undo/redo history |
| find_replace.hpp/cpp | ~40+150 | Find and replace across entries |
| row_filter.hpp/cpp | ~30+130 | Row filtering (search, status, type) |
| spell_checker.hpp/cpp | ~40+130 | Hunspell spell checking wrapper |
| byte_limit_validator.hpp/cpp | ~30+80 | Byte length validation |

### yampt.translator/source/highlighter/

| File | Lines | Responsibility |
|------|------:|----------------|
| highlight_coordinator.hpp/cpp | 41+123 | Highlight position computation |
| highlight_applier.hpp/cpp | ~40+195 | QTextEdit extra-selection application |
| editor_highlighter.hpp/cpp | ~40+170 | QSyntaxHighlighter subclass |
| glossary_highlighter.hpp/cpp | ~40+130 | Glossary term matching |
| grammar_checker.hpp/cpp | ~40+130 | Grammar/spelling error ranges |
| topic_highlighter.hpp/cpp | ~30+100 | DIAL topic highlighting |
| script_tokenizer.hpp/cpp | ~30+100 | MWScript keyword tokenization |

### yampt.translator/source/session/

| File | Lines | Responsibility |
|------|------:|----------------|
| session.hpp/cpp | 45+218 | Document lifecycle management |
| sidebar_controller.hpp/cpp | 57+181 | Sidebar interaction handling |
| plugin_operations_controller.hpp/cpp | 57+266 | Plugin operation dispatch |
| workspace_watcher.hpp/cpp | ~40+100 | Filesystem watcher for workspace |

### yampt.translator/source/view/

| File | Lines | Responsibility |
|------|------:|----------------|
| table_view.hpp/cpp | ~50+403 | Combined table + filter + search bar view |
| translation_edit_view.hpp/cpp | ~40+297 | Translation text editor widget |
| editor_view.hpp/cpp | ~40+225 | Original/adapted/translation panel |
| filter_tree_view.hpp/cpp | ~40+225 | Type filter tree widget |
| sidebar_view.hpp/cpp | ~40+236 | Sidebar tree widget |
| record_table_view.hpp/cpp | ~30+170 | Record table widget |
| status_filter_view.hpp/cpp | ~30+150 | Status filter buttons |
| annotations_view.hpp/cpp | ~30+130 | Annotations/glossary panel |
| book_preview_view.hpp/cpp | ~30+130 | Book HTML preview |
| history_view.hpp/cpp | ~30+100 | Edit history panel |
| log_view.hpp/cpp | ~30+100 | Operation log panel |
| validation_view.hpp/cpp | ~30+80 | Byte validation panel |
| translation_suggestion_view.hpp/cpp | ~30+80 | Translation engine suggestion panel |
| line_number_gutter.hpp/cpp | ~30+80 | Line number gutter for text editors |
| display_name.hpp/cpp | ~20+60 | Status display name mapping |
| status_display.hpp | ~20 | Status → display name lookup |

### yampt.translator/source/translator/

| File | Lines | Responsibility |
|------|------:|----------------|
| translator.hpp | ~30 | Abstract translator_t interface |
| ctranslate2_translator.hpp/cpp | ~40+130 | CTranslate2 backend |
| deepl_translator.hpp/cpp | ~30+100 | DeepL API backend |
| google_translator.hpp/cpp | ~30+100 | Google Translate API backend |

### yampt.translator/source/dialog/

| File | Lines | Responsibility |
|------|------:|----------------|
| dict_selection_dialog.hpp/cpp | ~40+150 | Dict selection for operations |
| make_base_dialog.hpp/cpp | ~40+258 | Make-base configuration dialog |
| find_replace_dialog.hpp/cpp | ~40+256 | Find/replace dialog |
| merge_dialog.hpp/cpp | ~30+210 | Merge configuration dialog |
| first_run_dialog.hpp/cpp | ~30+100 | First-run wizard |
| spell_context_menu.hpp/cpp | ~20+80 | Spell check right-click menu |
| settings/*.hpp/cpp | ~300 total | Settings dialog pages |

### yampt.editor/source/ (root)

| File | Lines | Responsibility |
|------|------:|----------------|
| editor_window.hpp/cpp | 28+176 | Main window shell (menu, toolbar, config) |
| main.cpp | ~30 | Application entry point |

### yampt.editor/source/model/

| File | Lines | Responsibility |
|------|------:|----------------|
| view_tree_model.hpp/cpp | 228+553 | Qt model for sub-record tree display |
| view_tree_decode.cpp | 624 | Generic record decoding into tree nodes |
| view_tree_decode_cell.cpp | 363 | Cell record decoding (FRMR groups) |
| view_tree_decode_lists.cpp | 313 | Leveled list/faction/container alignment |
| nav_tree_model.hpp/cpp | 84+665 | Qt model for navigation tree (plugin→type→record) |
| nav_tree_filter.hpp/cpp | ~40+130 | Navigation tree filtering |

### yampt.editor/source/controller/

| File | Lines | Responsibility |
|------|------:|----------------|
| merge_controller.hpp/cpp | 87+512 | Merge operations (copy record/sub-record/field) |
| view_context_menu.hpp/cpp | ~40+301 | Right-click context menu logic |

### yampt.editor/source/patcher/

| File | Lines | Responsibility |
|------|------:|----------------|
| patch_builder.hpp/cpp | 65+229 | Merged patch file building and saving |
| plugin_cleaner.hpp/cpp | ~30+100 | ITM record cleaning |

### yampt.editor/source/session/

| File | Lines | Responsibility |
|------|------:|----------------|
| plugin_session.hpp/cpp | ~60+405 | Plugin loading, scan orchestration, config |
| profile_reader.hpp/cpp | ~30+100 | Load-order profile reader |

### yampt.editor/source/view/

| File | Lines | Responsibility |
|------|------:|----------------|
| plugin_workspace_view.hpp/cpp | ~60+471 | Main workspace widget (splits, panels) |
| nav_tree_view.hpp/cpp | ~30+187 | Navigation tree widget |
| record_view.hpp/cpp | ~30+150 | Record tree view widget |
| preview_view.hpp/cpp | ~30+100 | Record value preview panel |
| messages_view.hpp/cpp | ~30+80 | Messages/log panel |
| editor_delegates.hpp | ~20 | Custom Qt delegate for tree editing |

### yampt.cli/source/

| File | Lines | Responsibility |
|------|------:|----------------|
| main.cpp | ~30 | CLI entry point |
| interface/user_interface.hpp/cpp | ~40+207 | Command-line argument parsing + dispatch |

---

## Architectural Issues

### Issue 1: `tools_t` — God Class in Utility Clothing (RESOLVED)

**Resolution:** `tools_t` eliminated entirely. Replaced by:
- `domain_types.hpp/.cpp` — Pure domain types (`rec_type_t`, `dict_t`, `chapter_t`, `record_entry_t`, `entry_t`, `record_t`) + `domain_types_t` static helpers (type conversion, byte conversion)
- `app_logger.hpp/.cpp` — Global logger state (`add_log`, `reset_log`, `get_log`, error/debug/quiet flags, exe_dir)
- `binary_file_io.hpp/.cpp` — Binary file read/write operations
- `string_utils.hpp` (extended) — All string utilities (`erase_null_chars`, `trim_cr`, `replace_non_printable_with_dot`, `case_insensitive_equal`)

31 headers switched from `tools.hpp` to `domain_types.hpp`. Zero coupling to logger/IO for type-only consumers.

### Issue 2: `dict_creator_t` — Strategy Pattern Extraction (RESOLVED)

**Resolution:** Split into a facade + 3 strategy classes + shared infrastructure:
- `dict_creator.hpp/.cpp` (45+59 lines) — Thin facade, picks strategy based on mode detection
- `creator_context.hpp` (82 lines) — Shared state struct (ESM readers, indexes, counters, dict)
- `creator_helpers.hpp/.cpp` (64+721 lines) — All shared logic (insert methods, index builders, script parsing, adapt_translation, determine_status)
- `creator_single.hpp/.cpp` (26+356 lines) — Single-file mode strategy
- `creator_base.hpp/.cpp` (40+684 lines) — Unordered base mode strategy
- `creator_ordered.hpp/.cpp` (35+518 lines) — Ordered base mode strategy

Each mode is its own class with its own `.hpp/.cpp` pair. No class is split across multiple files.

### Issue 3: `view_tree_model_t` — One Class Split Across 4 .cpp Files (2,127 lines)

**Problem:** Same anti-pattern as dict_creator. Cell decode (363 lines), generic decode (624 lines), and list decode (313 lines) are all methods of one class spread across files.

**Proposed extraction:**
1. `view_tree_decoder_t` — Extract generic decode logic (slot building, schema children)
2. `cell_decoder_t` — Extract cell-specific decode (FRMR grouping, referenced objects)
3. `list_decoder_t` — Extract leveled list / faction / container alignment
4. `view_tree_model_t` — Keep as Qt model that delegates to decoders

### Issue 4: `main_window_t` — Persistent God Class (1,629 lines across 2 .cpp files)

**Problem:** Despite extracting 4 controllers, main_window still:
- Owns all table rebuild logic (rebuild_table, rebuild_table_dict, rebuild_table_yaml)
- Manages search/filter state
- Handles config save/load
- Routes signals between all controllers
- Manages spell dictionary scanning
- Handles batch translation initiation
- Owns ~50 member variables

**Proposed extraction:**
1. `table_rebuild_controller_t` — Extract rebuild_table, rebuild_table_dict, rebuild_table_yaml
2. `search_controller_t` — Extract search field handling, filter state persistence
3. `config_persistence_t` — Extract save_config/load_config
4. `shortcut_registry_t` — Extract register_shortcuts and shortcut handlers

### Issue 5: `plugin_scan_t` — Dual Responsibility (686 lines across 2 .cpp files)

**Problem:** Combines plugin loading/conflict detection with merge patch record storage. The merge functionality (15+ methods) is an independent concern.

**Proposed extraction:**
1. `merge_patch_store_t` — Extract all merge record management (copy/pin/remove/find/save)
2. `plugin_scan_t` — Keep plugin loading + conflict computation only, hold reference to merge store

### Issue 6: File-Local `static` Functions (Widespread)

**Problem:** 40 source files use file-local `static` functions. The steering rule says "all logic belongs as static methods on the owning class." The worst offenders:

| File | Static functions | Nature |
|------|:----------------:|--------|
| sub_record_schema.cpp | 119 | Data tables (acceptable — const data, not logic) |
| sub_record_merge.cpp | 27 | Logic functions (violates rule) |
| conflict_slots.cpp | 24 | Logic functions (violates rule) |
| table_builder.cpp | 15 | Logic functions (violates rule) |
| plugin_scan.cpp | 15 | Logic functions (violates rule) |
| view_tree_decode_cell.cpp | 11 | Logic functions (violates rule) |
| patch_builder.cpp | 9 | Logic functions (violates rule) |
| record_behavior.cpp | 9 | Data tables (acceptable) |
| sidebar_model.cpp | 7 | Logic functions (violates rule) |
| annotations_view.cpp | 6 | Logic functions (violates rule) |
| filter_tree_view.cpp | 6 | Const data + helpers (borderline) |

**Note:** `sub_record_schema.cpp` (119 statics) and `record_behavior.cpp` (9 statics) are static const data arrays — this is an acceptable pattern for schema/lookup-table definitions. The rule targets logic functions that should be class methods.

**Fix:** For each file with logic statics, make them `static` methods on the class that owns the .cpp file. For files like `table_builder.cpp` where the class is `table_builder_t`, move helpers inside the class.

### Issue 7: `record_conflict.hpp` — Free Functions Without a Class

**Problem:** `compute_conflict_all()` and `compute_conflict_this()` are declared as free functions in a header. Per the steering rules, they should be static methods on a class.

**Proposed fix:** Create `record_conflict_t` class with these as static methods.

### Issue 8: `conflict_slots.cpp` + `conflict_slots_cell.cpp` — Large Free Functions (957 lines)

**Problem:** `build_conflict_slots()` and `build_cell_slots()` are free functions with 24+ internal statics. Should be a class with static methods.

**Proposed fix:** Create `conflict_slot_builder_t` with `build()` and `build_cell()` as public static methods, and all helpers as private static methods.

---

## Priority Ranking

| # | Issue | Impact | Effort | Status |
|---|-------|--------|--------|--------|
| 1 | `tools_t` domain type extraction | High (reduces coupling everywhere) | Medium | **DONE** |
| 2 | `dict_creator_t` split into strategy classes | High (largest class, most complex) | High | **DONE** |
| 3 | `view_tree_model_t` decoder extraction | Medium (editor-only, already working) | Medium | |
| 4 | `main_window_t` further decomposition | Medium (translator-only) | Medium | |
| 5 | `plugin_scan_t` merge extraction | Medium (editor-only) | Low | **DONE** |
| 6 | File-local static → class methods | Low (style, no functional change) | Low-Medium | |
| 7 | `record_conflict` → class | Low (small file) | Low | **DONE** |
| 8 | `conflict_slots` → class | Low (working correctly) | Low | **DONE** |

---

## What's Already Good

- Clear project separation (core/qt/cli/translator/editor)
- Consistent naming (`_t` suffix, snake_case, file-per-class)
- Well-defined folder responsibilities (view/model/controller/editor/session)
- Tests project with comprehensive coverage (65 test files)
- `cell_matcher_t` and `dial_matcher_t` properly extracted from dict_creator
- `text_match_index_t` extracted as its own class
- `scdt_patcher_t` properly extracted from script_parser
- `editor_controller_t`, `sidebar_controller_t`, `plugin_operations_controller_t` extraction from main_window
- `session_t` as clean document lifecycle manager
- All highlighters properly separated
- Small, focused files in editor/ and highlighter/ folders
