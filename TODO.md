# TODO

$(SolutionDir) still created in release folder

find/replace window is weirdly created not in the center but anchored to center by top/left corner (but only windows)

find replace should be incorporated into filter by panel, then filter by will show all records that will be affected by replace. Keep only replace all and undo all.

ENAM - enchantment: bad validation on chnagig text <- NPCS

record flags should have combobox
record signature should be non editable

CELL data flag should be combobox <- ANY flags in other record tpes also

ENDT - Enchantmrnt data FLAGS is uknown

comboboxes should show by default same value as edit box

merged patch header dont show plugin name in left panel

right click menu item on subrecord to exclude it from comparison/merged patch

show hexes insetad of <bytes>

editor:
- split nav tree into two tabs: Plugins (current ESP tree) and Lua (handler registrations grouped by mod). Add QTabBar + QStackedWidget to nav_tree_view_t. Remove "Lua Handlers" section from the plugins tree model.
- plugin_workspace_view.cpp: rename `fi`→`file_info`, `dir`→`directory`, `dlg`→`filter_dialog`, `msg`→`message`
- plugin_workspace_view.cpp: split `on_advanced_filter` (58 lines), `save_session_state` (55), `restore_session_state` (58)

translator:
- make_base_dialog.cpp: split `populate_plugin_tree` (105 lines, depth 4+)
- editor_view.cpp: split `highlight_adapted_diff` (60 lines), `load_script_entry` (55), `setup_connections` (55)
- filter_tree_view.cpp: split `build_rows` (68 lines)
- glossary.cpp: flatten `find_glossary_matches` (depth 4 nesting)
- script_tokenizer.cpp: rename `a`→`left_char`, `b`→`right_char`, `t`→`token`
- translation_suggestion_view.cpp: rename `fn`→`glossary_function`, `dir`→`directory`, `msg`→`message`
- sidebar_view.cpp: rename `pos`→`position`, `ext`→`extension`
- language_settings_view.cpp: remove `} // namespace` comment
- plugin_operations_controller.cpp: split `on_plugin_operation` (148 lines, depth 4) — extract helper per plugin_op_t case
- plugin_operations_controller.cpp: rename `norm`→`normalized_path`, `pre`→`preselected`, `sep`→`separator_pos`
- web_translator.cpp: extract `navigate_json_path` from `extract_response` (52 lines, depth 4)
- web_translator.cpp: rename `url`→`request_url`, `data`→`response_data`, `path`→`response_path`
- sidebar_controller.cpp: rename `sep`→`separator_pos`

core:
- cell_matcher.cpp: split `match_interior_cells_heuristic` (186 lines, depth 4) into smaller functions
- dial_matcher.cpp: split `match_by_translation` (187 lines, depth 4) into smaller functions
- creator_base.cpp: split `make_info` (86 lines)
- creator_helpers.cpp: rename `a`→`first_text`, `b`→`second_text` in `differs_only_in_numbers_or_punct`
- creator_helpers.cpp: rename `c`→`character` in `is_digit`, `ch`→`character` in `is_proper_noun`
- creator_base.cpp: rename `msg`→`message` (6 occurrences), `inam`→`info_id`
- creator_ordered.cpp: rename `msg`→`message`, `inam`→`info_id`
- dial_matcher.cpp: rename `inam`→`info_id`, `name`→`topic_name`
- cell_matcher.cpp: rename `c`→`character`, `dodt`→`door_destination`, `id`→`cell_id`
- loc_generator.cpp: rename `ch`→`character`, `uch`→`unsigned_char`, `stem`→`filename_stem`
- translation_engine.cpp: split `translate` (73 lines) — extract batching and option setup
- translation_engine.cpp: rename `t`→`token`
- file_list.cpp: rename `fe`→`file_entry`
- auto_merge.cpp: rename `pi`→`plugin_idx`, `v`→`version_idx`, `ei`→`entry_idx`, `mi`→`merge_idx`
- plugin_scan.cpp: split `compute_conflict` (75 lines, depth 4) — extract slot iteration
- plugin_scan.cpp: rename `pi`→`plugin_idx`, `mi`→`merge_idx`, `sr`→`slot_result`, `vi`→`version_idx`, `fi`→`field_idx`
- domain_types.cpp: rename `str`→`byte_array` in conversion functions

excluded subrecords white background, grey font
add option to exclude subrecord from menu on row
excludion pattern from merged patch, what it is doinmg now? replace by excluded subrecords
add option to exclude name pattern from merged patch

Paths -> Output Paths

Cleaning -> add option to fix master order, based on current loaded

DELETED icons instead of strikeout

editor panels size 1/2
left panel columns 3/1

enable editing global option, not per plugin
preview tab first, chnage name to edit if esit enabled
