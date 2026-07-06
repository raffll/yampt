# Violations Backlog

Tracked steering rule violations discovered during full codebase audit (2026-07-06). Fix opportunistically when touching affected files.

## Functions Over 50 Lines

| File | Function | ~Lines |
|------|----------|-------:|
| editor/controller/view_context_menu.cpp | `show_view_menu` | 263 |
| editor/model/nav_tree_model.cpp | `data()` | 241 |
| editor/model/view_tree_decode_cell.cpp | multiple decode functions | 112–214 |
| translator/main_window_setup.cpp | `connect_editor_signals` | 210 |
| translator/main_window_setup.cpp | `connect_menu_signals` | 164 |
| editor/session/plugin_session.cpp | session function | 130 |
| translator/session/plugin_operations_controller.cpp | `on_plugin_operation` | 127 |
| cell_matcher.cpp | `match_interior_cells_heuristic` | 120 |
| dial_matcher.cpp | `match_by_translation` | 110 |
| translator/session/dict_operations_controller.cpp | `start_batch_translation` | 99 |
| translator/controller/record_display_controller.cpp | `load_record` | 91 |
| script_parser.cpp | `convert_script` | 85 |
| editor/controller/merge_controller.cpp | `save_merge_to_file` | 83 |
| editor/controller/view_context_menu.cpp | `show_nav_menu` | 80 |
| creator_base.cpp | `make_info` | 80 |
| translator/highlighter/highlight_coordinator.cpp | `find_annotation_highlights` | 79 |
| translator/main_window_setup.cpp | `setup_sidebar` | 77 |
| editor/model/view_tree_model.cpp | model function | 76 |
| editor/editor_window.cpp | `setup_menu_bar` | 76 |
| translator/session/plugin_operations_controller.cpp | `build_dict_entries` | 73 |
| translation_engine.cpp | `translate` | 70 |
| cell_matcher.cpp | `match_exterior_cells` | 70 |
| dial_matcher.cpp | `match_by_inam` | 70 |
| translator/view/editor_view.cpp | view function | 69 |
| cell_matcher.cpp | `match_interior_cells` | 65 |
| editor/session/plugin_session.cpp | session function | 65 |
| translator/model/record_table_model.cpp | `sort` | 61 |
| translator/editor/byte_limit_validator.cpp | `validate` | 61 |
| cell_matcher.cpp | `make_cell_fingerprint` | 60 |
| creator_helpers.cpp | `insert_entry_single_with_base` | 60 |
| creator_base.cpp | `make_bnam` | 60 |
| translation_engine.cpp | `load` | 55 |
| esm_converter.cpp | `make_header` | 55 |
| esm_converter.cpp | `convert_info` | 55 |
| esm_converter.cpp | `convert_bnam` | 55 |
| esm_converter.cpp | `convert_scpt` | 55 |
| creator_helpers.cpp | `make_script_messages` | 55 |
| script_parser.cpp | `convert_line_unquoted` | 55 |
| dial_matcher.cpp | `report_unmatched` | 55 |

## Functions With More Than 2 Parameters

| File | Function | Params |
|------|----------|-------:|
| merge_controller.hpp | `copy_field` | 7 |
| operation_executor | `make_base` | 7 |
| creator_helpers.hpp | `insert_entry_base` | 6 |
| creator_helpers.hpp | `insert_with_status` | 6 |
| creator_helpers.hpp | `insert_duplicate` | 6 |
| esm_converter.hpp | constructor | 6 |
| script_parser.hpp | constructor | 6 |
| merge_controller.hpp | `copy_cell_record` | 5 |
| merge_controller.hpp | `copy_sub_record` | 5 |
| creator_helpers.hpp | `insert_entry_single` | 5 |
| creator_helpers.hpp | `insert_entry_single_with_base` | 5 |
| creator_helpers.hpp | `insert_changed_entry` | 5 |
| creator_helpers.hpp | `insert_unapproved_changed` | 5 |
| creator_helpers.hpp | `insert_adapted_entry` | 5 |
| cell_matcher.hpp | constructor | 5 |
| dial_matcher.hpp | constructor | 5 |
| word_match_utils.hpp | `compute_best_match` | 5 |
| edit_history | `record_change` | 5 |
| find_replace | `replace_current` | 5 |
| word_match_utils.hpp | `check_all_same_name` | 4 |
| find_replace | `find_next` | 4 |
| find_replace | `replace_all` | 4 |
| creator_ordered.cpp | `process_bnam` | 4 |
| merge_controller.hpp | `remove_sub_record` | 4 |

## Nesting Deeper Than 3 Levels

| File | Function |
|------|----------|
| editor/controller/view_context_menu.cpp | `show_view_menu` (5+ levels) |
| editor/model/nav_tree_model.cpp | `data()` (4+ levels) |
| script_parser.cpp | `convert_script` (4 levels) |
| cell_matcher.cpp | `match_interior_cells_heuristic` (4 levels) |
| dial_matcher.cpp | `match_by_translation` (4 levels) |
| creator_base.cpp | `make_info` (4 levels) |
| creator_helpers.cpp | `make_script_messages` (4 levels) |
| translator/session/dict_operations_controller.cpp | `start_batch_translation` (4 levels) |
| translator/main_window_setup.cpp | `connect_editor_signals` (lambda nesting) |

## Classes That Should Be Namespaces

| File | Class | Reason |
|------|-------|--------|
| io/dict_writer.hpp | `dict_writer_t` | Single static `write()` method, no state |
| highlighter/highlight_applier.hpp | `highlight_applier_t` | Only static methods, no state |
| highlighter/highlight_coordinator.hpp | `highlight_coordinator_t` | Only static methods, no state |

## Magic Numbers

| File | Numbers | Meaning |
|------|---------|---------|
| translation_engine.cpp | 4, 0.6f, 1.5f, 3, 10 | beam_size, length_penalty, repetition_penalty, no_repeat_ngram_size, max_length_factor |
| esm_converter.cpp | 156, 159, 179, 185, 191, 230, 234, 241 | Windows-1250 char codes for Polish detection |
| cell_matcher.cpp | 14695981039346656037ULL, 1099511628211ULL | FNV-1a hash constants |
| byte_limit_validator.cpp | 63, 31, 32, 512, 1024 | Sub-record byte limits (CELL, FNAM, RNAM, INFO) |
| highlight_applier.cpp | QColor(40,55,75), QColor(35,60,40) | Annotation/grammar highlight colors |
| dict_selection_dialog.cpp | QColor(130,130,130), QColor(180,140,80) | Base dict color, golden color |
| settings_store.cpp | 250, 200, 150, 1200, 800, 100, 0.5f | Default window geometry values |

## Comments in Code

| File | Comment |
|------|---------|
| esm_converter.cpp | `// convert_gmdt();` (commented-out code) |
| script_parser.cpp | `/* special case if say keyword */` |
| script_parser.cpp | `/* first parameter is sound file name, so we don't need it */` |
| translator/main_window.hpp | `// rebuild_table helpers` |

## Duplicate File Names Across Projects

| File Name | Locations |
|-----------|-----------|
| appearance_settings_view | yampt.translator + yampt.editor |

## Short Variable Names (Under 5 Characters)

Widespread across the codebase. Most common: `i`, `j`, `k`, `c`, `e`, `f`, `s`, `fi`, `ni`, `it`, `ss`, `re`, `ui`, `pos`, `end`, `buf`, `col`, `row`, `dir`, `ext`, `doc`, `now`, `sep`, `fmt`, `term`, `word`, `load`, `last`, `time`, `stem`, `code`, `info`.

Loop indices (`i`, `j`, `k`) and iterator variables (`it`) are the most common offenders. Fix when refactoring affected functions.

## No Violations Found

- No Qt includes in yampt.core
- No files over 1000 lines
- No classes split across multiple .cpp files
- No snake_case violations
- No `_panel`/`_tab`/`_bar`/`_widget` suffixes on view classes
- No generic class names (manager, provider, service, etc.)
- Consistent `_t` suffix on all types
- Consistent `m_` prefix on member variables
- No cross-project relative path includes
