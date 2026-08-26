# TODO

$(SolutionDir) still created in release folder

Wire up edit_history_t::load_from_file / save_to_file — persist undo history across sessions so users can revert after restart.

Wire up glossary_t::find_glossary_matches in send_chat_request — provide AI providers with term→translation pairs instead of substituted text.

## Audit Findings

- web_translator_t::send_chat_request glossary_fn returns substituted text instead of term pairs for AI system prompt. Replace with structured reference data: glossary term→translation pairs (from find_glossary_matches), DIAL topic names to preserve, and optionally 2-3 example translations from the same document for style reference.
- edit_history_t dead code: load_from_file, save_to_file, is_modified_this_session — implemented but never called. Remove or wire up persistence.
- creator_helpers namespace: nearly every function has 4-6 arguments (max 2 allowed). Create insert_params_t struct.
- content_alignment.cpp: most methods take 5-8 arguments (fill_key_indices=8, fit_merge_column=7, emit_key_slots=7). Create alignment context struct.
- yaml_l10n_writer doesn't quote YAML-reserved bare words (true, false, null, yes, no) or numeric strings — produces technically ambiguous YAML for external tools.

[02:22] Need at least 2 plugins loaded to create a merged patch
[02:26] [error] need at least 2 plugins loaded to clean

editor: Search -> Apply

trabslator: filter/replace buttons not flat

backup for saved plugins

adavnced filters should be splite 50-50


