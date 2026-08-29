# Known Issues

Tracked violations that are acknowledged but not yet fixed. Do NOT re-report these in audits.

## main_window.cpp exceeds 1000 lines

`yampt.translator/source/main_window.cpp` exceeds the 1000-line file limit. Table/filter orchestration logic should be extracted into a dedicated controller.

## main_window_setup.cpp connect functions exceed 50-line limit

`connect_menu_signals` (~210 lines), `connect_sidebar_signals` (~87 lines), and `connect_editor_signals` (~238 lines) all exceed the 50-line function limit. The lambda logic inside these functions should be moved into controller methods, leaving only thin `connect()` calls in the setup file.

## translate_all_requested violates Anti-Gravity Rule

The `translate_all_requested` signal handler in `main_window` contains orchestration logic (showing dialogs, running operations, updating multiple views). This should be extracted into a translation controller per the Main Window Anti-Gravity Rule.


## SCDT messagebox segment limited to 254 characters

`scdt_patcher_t::patch_later_message_segment` uses a 1-byte size field per the vanilla Morrowind bytecode format. Translated message segments longer than 254 characters cannot be patched — the converter logs an error and leaves that segment untranslated in the bytecode.

This only affects the vanilla Morrowind engine. OpenMW ignores compiled bytecode entirely and recompiles from the SCTX source text, which has no length limitation.


## Function size violations in stable scanner/merger/engine code

The following functions exceed 50 lines but are stable, rarely modified, and readable as linear top-to-bottom logic. Splitting them would add indirection without meaningful maintainability gain:

- `batch_cleaner_t::clean_plugin()` — 87 lines
- `plugin_scan_t::compute_conflict()` — ~80 lines
- `creator_base_t::make_info()` — 72 lines with 4 nesting levels
- `sub_record_merge_t::apply_intermediate()` — ~68 lines
- `translation_engine_t::translate()` — 62 lines
- `yaml_l10n_writer_t::write()` — 59 lines
- `merge_patch_ops_t::patch_field()` — 7 arguments (only one caller)


## Borderline function size violations in dialog/menu code

These exceed 50 lines but are stable, self-contained dialog/menu builders. Refactor when touching the file for other reasons:

- `make_base_dialog_t::populate_plugin_tree()` — 114 lines
- `view_context_menu_t::show_view_menu()` — 92 lines


## creator_helpers insert functions exceed 2-argument limit

The `insert_entry_*` family in `creator_helpers` (namespace functions plus the `creator_ordered_t::insert_entry_base` wrapper) takes 4-6 arguments each, exceeding the max-2-argument rule. The intended fix is to pass data via structs (`insert_params_t` for key/old/new/type/status, `insert_from_base_params_t` for the three functions that take a `record_entry_t & base_entry`) and update the ~40 call sites across `creator_single.cpp`, `creator_ordered.cpp`, and the internal calls in `creator_helpers.cpp`. Deferred: large blast radius across a core dictionary-generation path, low priority.

## field_def_role case in view_tree_model.cpp exceeds 100 lines

The `field_def_role` case handling in `view_tree_model.cpp` is 103 lines, exceeding the 50-line function limit. The decode logic is inseparable from the model's presentation role (allowed class-split exception for `view_tree_model_t`), but the case block itself should be extracted into a helper.

## exclude sub-record lambda nesting exceeds 3 levels

The exclude sub-record lambda in `view_context_menu.cpp` nests more than 3 levels deep, violating the no-deep-nesting rule. The lambda logic should be flattened with early returns or moved into a named method.

## highlight_coordinator_t is a static-only class

`highlight_coordinator_t` has only static methods and no mutable state. Per the classes-vs-namespaces rule it should be a namespace, not a class.

## insert_duplicate has an unused status parameter

`insert_duplicate` takes a `status` parameter that is never used — dead code. The parameter should be removed and call sites updated.

## esm_converter.hpp and esm_reader.hpp accessors missing const qualifier

Accessor methods in `esm_converter.hpp` and `esm_reader.hpp` that do not mutate state are missing the `const` qualifier.

## spell_checker duplicated case-insensitive search

`spell_checker_t::is_excluded` now uses the shared `string_utils::case_insensitive_equal_utf8`. `is_mwscript_keyword` still has an inline per-byte compare, kept ASCII on purpose because MWScript keywords are ASCII technical tokens; it could still be consolidated onto a shared ASCII helper.

## script_parser trim_last_new_line_chars has an unreachable condition

`script_parser::trim_last_new_line_chars` contains an `|| npos` condition that is unreachable given the preceding checks. The redundant branch should be removed.
