# TODO

$(SolutionDir) still created in release folder

Wire up edit_history_t::load_from_file / save_to_file — persist undo history across sessions so users can revert after restart.

Wire up glossary_t::find_glossary_matches in send_chat_request — provide AI providers with term→translation pairs instead of substituted text.

## Audit Findings

- web_translator_t::send_chat_request glossary_fn returns substituted text instead of term pairs for AI system prompt. Replace with structured reference data: glossary term→translation pairs (from find_glossary_matches), DIAL topic names to preserve, and optionally 2-3 example translations from the same document for style reference.
- Translation settings page: add Local Models tab alongside Web Providers tab. JSON configs in models/ describing available CTranslate2 models (name, path, supported languages). Similar discovery pattern to web providers — data-driven, no hardcoding.
- edit_history_t dead code: load_from_file, save_to_file, is_modified_this_session — implemented but never called. Remove or wire up persistence.
- make_base_dialog_t::populate_plugin_tree() is 114 lines. Extract collect_plugins, group_by_root, populate_tree_items, select_best_match.
- creator_helpers namespace: nearly every function has 4-6 arguments (max 2 allowed). Create insert_params_t struct.
- view_context_menu.cpp show_view_menu() is 92 lines. Extract determine_row_kind() and exclude-sub-record helper.
- creator_base_t::make_info() is 72 lines with 4 nesting levels. Extract enrich_speaker_info and DIAL key tracking.
- batch_cleaner_t::clean_plugin() is 87 lines with 4-level nesting. Extract record filtering logic and output writing.
- plugin_scan_t::compute_conflict() is ~80 lines with 4-level nesting. Extract slot evaluation loop into its own method.
- sub_record_merge_t::apply_intermediate() is ~68 lines. Extract three-way merge loop logic.
- content_alignment.cpp: most methods take 5-8 arguments (fill_key_indices=8, fit_merge_column=7, emit_key_slots=7). Create alignment context struct.
- merge_patch_ops_t::patch_field() takes 7 arguments. Create a patch_field_params_t struct.
- translation_engine_t::translate() is 62 lines. Extract tokenization and result assembly into helpers.
- yaml_l10n_writer.cpp write() is 59 lines. Extract block scalar writing and quoted value writing into helpers.
- yaml_l10n_writer doesn't quote YAML-reserved bare words (true, false, null, yes, no) or numeric strings — produces technically ambiguous YAML for external tools.
