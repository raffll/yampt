# TODO

$(SolutionDir) still created in release folder

make dictionary dont need base dict, create/convert need

Translator settings: propose better name: and also populate dynamically with providers file

## Audit Findings

- highlight_coordinator_t::to_lower_ascii only handles ASCII (A-Z), breaks annotation matching for non-English characters (Ö, Ą, É). Replace with string_utils::to_lower.
- web_translator_t::send_chat_request glossary_fn returns substituted text instead of term pairs for AI system prompt. Should use find_glossary_matches and format as reference list.
- script_parser_t::find_keyword lacks word boundary checks (unlike creator_helpers). Use find_whole_word and skip npos entries.
- script_parser_t member variables missing m_prefix (18 members: type, merger, record_key, source_path, old_script, old_scdt, new_script, is_done, line, line_lc, old_text, new_line, new_text, pos, keyword_pos, keyword, error).
- includes.hpp is a monolithic catch-all header pulling 17 std headers into 15 TUs. Eliminate and replace with per-file includes.
- README.md: Lua handler feature misplaced under "Viewing" section — should describe it as a navigation tab.
- Stale error status on BNAM entries: error status persists in saved dict after the offending content is fixed. Re-validate on load or on display and auto-clear error when text passes validation.
- Translation settings page: add Local Models tab alongside Web Providers tab. JSON configs in models/ describing available CTranslate2 models (name, path, supported languages). Similar discovery pattern to web providers — data-driven, no hardcoding.
- web_translator_t::send_chat_request sends wrong JSON body for both Claude and ChatGPT — hardcodes both top-level "system" field (Claude) and system role in messages array (OpenAI) regardless of provider. Branch on provider format or let config define message structure.
- yampt.editor.vcxproj.filters stale: contains 14 yampt.core .cpp files no longer compiled (project links yampt.core.lib). Also missing preview_view.cpp and editor_delegates.hpp entries.
- glossary_t dead code: load_enchantments, get_enchantment, has_enchantment, set_use_trie_matching, get_use_trie_matching — never called in production. Remove or wire up.
- edit_history_t dead code: load_from_file, save_to_file, is_modified_this_session — implemented but never called. Remove or wire up persistence.
- rec_type_t::gmdt dead enum value — declared and mapped to string but never used in any logic.
- README.md line 55: ChatGPT and Google Cloud providers missing from web translation feature list.
- Steering web-translator-architecture: `method` field documented but never parsed; `query` body_format and `settings` array undocumented.
- plugin_operations_controller_t::on_plugin_operation is 150 lines (3x limit). Extract per-operation methods.
- view_context_menu.cpp build_merge_remove_menu: binary_ranges[context.col] accessed without bounds check — out-of-bounds crash if col exceeds vector size.
- merge_controller.cpp copy_cell_record: std::stoul without try/catch — crashes on invalid string input.
- binary_file_io.cpp: tellg() failure (-1) not checked before cast to size_t for reserve() — causes std::bad_alloc on non-seekable streams.
- resource_paths.hpp: data_dir() declared but never defined or called. Remove dangling declaration.
- file_list.hpp: classify() and detect_language() exist as both static members AND free functions with divergent implementations. Remove one set.
- make_base_dialog_t::populate_plugin_tree() is 114 lines. Extract collect_plugins, group_by_root, populate_tree_items, select_best_match.
- creator_helpers namespace: nearly every function has 4-6 arguments (max 2 allowed). Create insert_params_t struct.
- view_context_menu.cpp show_view_menu() is 92 lines. Extract determine_row_kind() and exclude-sub-record helper.
- creator_base_t::make_info() is 72 lines with 4 nesting levels. Extract enrich_speaker_info and DIAL key tracking.