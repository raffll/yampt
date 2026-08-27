# TODO

Wire up edit_history_t::load_from_file / save_to_file — persist undo history across sessions so users can revert after restart.
- edit_history_t dead code: load_from_file, save_to_file, is_modified_this_session — implemented but never called. Remove or wire up persistence.

## Audit Findings


- creator_helpers namespace: nearly every function has 4-6 arguments (max 2 allowed). Create insert_params_t struct.

backup for saved plugins

when you create base dict, show in log why cell is missing or mismatched