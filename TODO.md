# TODO

$(SolutionDir) still created in release folder

make dictionary dont need base dict, create/convert need

Translator settings: propose better name: and also populate dynamically with providers file

Wire up edit_history_t::load_from_file / save_to_file — persist undo history across sessions so users can revert after restart.

Wire up glossary_t::find_glossary_matches in send_chat_request — provide AI providers with term→translation pairs instead of substituted text.

## Audit Findings

- web_translator_t::send_chat_request glossary_fn returns substituted text instead of term pairs for AI system prompt. Should use find_glossary_matches and format as reference list.
- Translation settings page: add Local Models tab alongside Web Providers tab. JSON configs in models/ describing available CTranslate2 models (name, path, supported languages). Similar discovery pattern to web providers — data-driven, no hardcoding.
- web_translator_t::send_chat_request sends wrong JSON body for both Claude and ChatGPT — hardcodes both top-level "system" field (Claude) and system role in messages array (OpenAI) regardless of provider. Branch on provider format or let config define message structure.
- glossary_t dead code: load_enchantments, get_enchantment, has_enchantment, set_use_trie_matching, get_use_trie_matching — never called in production. Remove or wire up.
- edit_history_t dead code: load_from_file, save_to_file, is_modified_this_session — implemented but never called. Remove or wire up persistence.
- plugin_operations_controller_t::on_plugin_operation is 150 lines (3x limit). Extract per-operation methods.
- make_base_dialog_t::populate_plugin_tree() is 114 lines. Extract collect_plugins, group_by_root, populate_tree_items, select_best_match.
- creator_helpers namespace: nearly every function has 4-6 arguments (max 2 allowed). Create insert_params_t struct.
- view_context_menu.cpp show_view_menu() is 92 lines. Extract determine_row_kind() and exclude-sub-record helper.
- creator_base_t::make_info() is 72 lines with 4 nesting levels. Extract enrich_speaker_info and DIAL key tracking.
- view_tree_format.cpp decode_field() is 194 lines (4x limit). Split into per-type-family helpers (integers, floats, strings, flags, enums).
- batch_cleaner_t::clean_plugin() is 87 lines with 4-level nesting. Extract record filtering logic and output writing.
- plugin_scan_t::compute_conflict() is ~80 lines with 4-level nesting. Extract slot evaluation loop into its own method.
- sub_record_merge_t::apply_intermediate() is ~68 lines. Extract three-way merge loop logic.
- content_alignment.cpp: most methods take 5-8 arguments (fill_key_indices=8, fit_merge_column=7, emit_key_slots=7). Create alignment context struct.
- merge_patch_ops_t::patch_field() takes 7 arguments. Create a patch_field_params_t struct.
- translation_engine_t::translate() is 62 lines. Extract tokenization and result assembly into helpers.
- yaml_l10n_writer.cpp write() is 59 lines. Extract block scalar writing and quoted value writing into helpers.
- scdt_patcher_t::patch_later_message_segment uses 1-byte size field — silently truncates for message texts >255 chars, corrupting bytecode. Add overflow check or switch to 2-byte encoding.
- encode_from_utf8 (Windows) silently applies best-fit character mapping for unencodable characters — no error reported. byte_limit_validator approves text that will be corrupted on save. Add WC_NO_BEST_FIT_CHARS flag and warn user on encoding loss.
- Duplicate file opening: session normalize_path only replaces backslashes, doesn't canonicalize case or resolve ../ — two paths to the same file on Windows can open separate documents, causing data loss on save.
- yaml_l10n_writer doesn't quote YAML-reserved bare words (true, false, null, yes, no) or numeric strings — produces technically ambiguous YAML for external tools.
