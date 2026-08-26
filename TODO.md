# TODO

$(SolutionDir) still created in release folder

make dictionary dont need base dict, create/convert need

Translator settings: propose better name: and also populate dynamically with providers file

Wire up edit_history_t::load_from_file / save_to_file — persist undo history across sessions so users can revert after restart.

Wire up glossary_t::find_glossary_matches in send_chat_request — provide AI providers with term→translation pairs instead of substituted text.

## Audit Findings

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
- Steering web-translator-architecture: `method` field documented but never parsed; `query` body_format and `settings` array undocumented.
- plugin_operations_controller_t::on_plugin_operation is 150 lines (3x limit). Extract per-operation methods.
- view_context_menu.cpp build_merge_remove_menu: binary_ranges[context.col] accessed without bounds check — out-of-bounds crash if col exceeds vector size.
- merge_controller.cpp copy_cell_record: std::stoul without try/catch — crashes on invalid string input.
- binary_file_io.cpp: tellg() failure (-1) not checked before cast to size_t for reserve() — causes std::bad_alloc on non-seekable streams.
- file_list.hpp: classify() and detect_language() exist as both static members AND free functions with divergent implementations. Remove one set.
- make_base_dialog_t::populate_plugin_tree() is 114 lines. Extract collect_plugins, group_by_root, populate_tree_items, select_best_match.
- creator_helpers namespace: nearly every function has 4-6 arguments (max 2 allowed). Create insert_params_t struct.
- view_context_menu.cpp show_view_menu() is 92 lines. Extract determine_row_kind() and exclude-sub-record helper.
- creator_base_t::make_info() is 72 lines with 4 nesting levels. Extract enrich_speaker_info and DIAL key tracking.
- translation_suggestion_view.cpp advance_line_queue(): active_provider() called without null check — crash if provider becomes unavailable mid-queue.
- view_tree_format.cpp decode_field() is 194 lines (4x limit). Split into per-type-family helpers (integers, floats, strings, flags, enums).
- batch_cleaner_t::clean_plugin() is 87 lines with 4-level nesting. Extract record filtering logic and output writing.
- plugin_scan_t::compute_conflict() is ~80 lines with 4-level nesting. Extract slot evaluation loop into its own method.
- sub_record_merge_t::apply_intermediate() is ~68 lines. Extract three-way merge loop logic.
- content_alignment.cpp: most methods take 5-8 arguments (fill_key_indices=8, fit_merge_column=7, emit_key_slots=7). Create alignment context struct.
- merge_patch_ops_t::patch_field() takes 7 arguments. Create a patch_field_params_t struct.
- translation_engine_t::load() swallows all exceptions with catch(...) — logs nothing about the failure cause. Log exception message.
- translation_engine_t::translate() is 62 lines. Extract tokenization and result assembly into helpers.
- yaml_l10n_writer.cpp write() is 59 lines. Extract block scalar writing and quoted value writing into helpers.
- scdt_patcher_t::patch_later_message_segment uses 1-byte size field — silently truncates for message texts >255 chars, corrupting bytecode. Add overflow check or switch to 2-byte encoding.
- yTranslator-Manual.md: `heuristic` status missing from Entry Statuses section. It's assigned by cell_matcher/dial_matcher and users encounter it in make-base results.
- encode_from_utf8 (Windows) silently applies best-fit character mapping for unencodable characters — no error reported. byte_limit_validator approves text that will be corrupted on save. Add WC_NO_BEST_FIT_CHARS flag and warn user on encoding loss.
- Duplicate file opening: session normalize_path only replaces backslashes, doesn't canonicalize case or resolve ../ — two paths to the same file on Windows can open separate documents, causing data loss on save.
- text_match_index ambiguity detection uses substring find on pipe-separated list — false positives if a translation is a substring of another (e.g. "cat" found inside "concatenate"). Use split-by-pipe then exact comparison.
- word_match_utils::count_shared_words inflates score when source contains duplicate words — each occurrence counts separately. Can cause wrong cell heuristic matches. Deduplicate source words before counting.
- yaml_l10n_writer doesn't quote YAML-reserved bare words (true, false, null, yes, no) or numeric strings — produces technically ambiguous YAML for external tools.