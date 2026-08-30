# Requirements — Clean Output Mode in yEditor

## Background — Current Behavior

yEditor's "clean" operation removes unwanted records from loaded plugins (evil GMSTs, junk cells) and optionally repairs the plugin header (master file sizes, version 1.3). The user triggers it from a toolbar button, which calls `plugin_workspace_view_t::on_clean_all()`:

```cpp
const auto output_path = m_merge_controller->resolve_output_directory();
...
batch_cleaner_t cleaner(m_session->scan(), log_fn);
clean_options_t options; // evil_gmst, junk_cell, update_master_sizes, update_version
cleaner.set_options(options);
const auto results = cleaner.clean_all(output_path);
```

`batch_cleaner_t` (yampt.core/source/scanner/batch_cleaner.cpp) iterates every non-master, non-merge plugin. For each, `clean_plugin(plugin_idx, output_directory)`:

1. Reads `esm.get_records()` and builds `std::vector<const record_t *> kept_records`, skipping evil GMSTs and junk cells (TES3 header and everything else are kept).
2. Optionally rebuilds the TES3 header via `header_repair_t` (master sizes / version).
3. **Writes a new file immediately** to `output_directory + "/" + filename` via an `std::ofstream`, streaming each kept record's `content` (patched header for record 0), and sets `result.written = true`.

So cleaning writes files to disk in a single click, to whatever directory `merge_controller_t::resolve_output_directory()` resolves (per-mode Folder / MO2 / OpenMW). The in-memory plugins are NOT modified — only files are written. There is no dirty state and no review step.

## Relationship to the Completed `deferred-plugin-save` Feature

The `deferred-plugin-save` spec (all tasks complete) already built the deferred-save infrastructure this feature reuses for one of its two modes:

- `plugin_session_t` dirty API: `mark_plugin_dirty(int)`, `clear_plugin_dirty(int)`, `is_plugin_dirty(int)`, `dirty_plugins()`, `has_any_unsaved()` — filename-keyed, cleared on reload/unload.
- Nav-tree `"* "` marker driven by `nav_tree_filter_t` dirty-plugin pointer, rendered in `file_node_display_text`.
- Window-title asterisk via `editor_window_t::set_unsaved_changes(bool)` / `unsaved_changes_changed`.
- Manual save: `merge_controller_t::save_plugin(int)` / `save_all_dirty()` writing `plugin.get_records()` to `plugin_path(idx)` via `binary_file_io::write_file`.
- The canonical in-memory edit path `field_edit_controller_t::commit_to_source`: `mutable_plugin(idx)` → `replace_record` → `mark_plugin_dirty(idx)` → `recompute_single_conflict`, with NO disk write.

Field edits already defer to manual save. Cleaning is the remaining operation that still writes immediately. This feature makes cleaning able to use that same deferred model, selectable via a settings checkbox.

## Problem

Cleaning always writes plugin files to disk immediately, into a separate output directory. The user cannot review what cleaning removed before it touches disk, cannot batch cleaning with other edits into one save, and cannot discard a clean by not saving. But some users still want the fire-and-forget behavior of exporting cleaned copies without altering the loaded plugins. A single mode cannot serve both. yEditor needs a user-selectable choice between modifying the loaded plugins in place (deferred, reviewable) and exporting cleaned copies to new files (immediate).

## Goal

Add a settings checkbox that selects one of two clean output modes:

1. **Modify in place** (deferred): cleaning mutates the loaded plugins in memory and marks them dirty; the user saves manually (Save overwrites the original plugin file). Reuses the `deferred-plugin-save` model.
2. **Create new files** (immediate): cleaning writes cleaned files to the configured output directory, leaving the loaded plugins untouched. This is the current behavior.

## User-Facing Outcomes

- A checkbox in the yEditor Cleaning settings page chooses between "Modify plugins in place" and "Create new files".
- In **Modify in place** mode: clicking Clean removes the matching records from the loaded plugins in memory; the view and conflicts update, and each changed plugin gets an asterisk. Nothing is written until the user saves. Saving overwrites the original plugin file. Not saving discards the clean.
- In **Create new files** mode: clicking Clean immediately writes cleaned copies to the output directory. The loaded plugins are unchanged (no asterisk). This matches today's behavior.
- Header repair (master sizes / version 1.3), when enabled, follows the selected mode: applied in memory in "modify in place", written into the exported copy in "create new files".

## Requirements

### R1 — Clean output-mode setting

1.1 A persisted boolean setting selects the clean output mode, following the existing `Cleaning/` INI section and the `settings_store_t` getter/setter pattern (e.g. `Cleaning/ModifyInPlace`). Getter `clean_modify_in_place()` / setter `set_clean_modify_in_place(bool)`.
1.2 The setting has a sensible default. Default is **create new files** (setting false), preserving today's behavior for existing users so cleaning never silently starts overwriting originals after an update.
1.3 The Cleaning settings page (`cleaning_settings_view_t`) gains a control for the mode with a tooltip (gui-tooltips rule), loaded/saved via the existing `load`/`save` methods; no settings-dialog registration change is needed.
1.4 The control clearly communicates the two mutually-exclusive outcomes (modify the loaded plugins in place vs. write cleaned copies to the output directory).

### R2 — Modify-in-place mode (deferred, in-memory)

2.1 In modify-in-place mode, `batch_cleaner_t` commits the kept record set back into the in-memory plugin instead of writing a file; it performs NO file I/O in this mode.
2.2 Header repair (master sizes / version), when enabled, is applied to the in-memory TES3 header record (record 0) as part of the committed set.
2.3 A plugin is only committed/marked when cleaning actually changed it (records removed or header repaired), matching the current early-return `if (result.total_removed == 0 && !has_header_changes) return;`.
2.4 The yEditor caller marks each changed plugin dirty (`m_session->mark_plugin_dirty(result.plugin_idx)`), recomputes conflicts once (`rebuild_conflicts()`), rebuilds the nav tree preserving state, and updates the title asterisk (`emit unsaved_changes_changed(has_any_unsaved())`).
2.5 Saving a cleaned (dirty) plugin uses the existing `merge_controller_t::save_plugin` / `save_all_dirty` path unchanged, writing the cleaned in-memory record set back to the plugin's own path.

### R3 — Create-new-files mode (immediate, on-disk)

3.1 In create-new-files mode, `batch_cleaner_t` writes the cleaned record set to the configured output directory under the plugin's own filename, exactly as today (`resolve_output_directory()` + `<dir>/<filename>`, streamed via the writer).
3.2 The in-memory plugins are NOT modified and NOT marked dirty in this mode; no asterisk appears.
3.3 Header repair, when enabled, is applied to the exported file's header record, as today.
3.4 The empty-result log (`"[info] no records to clean"`) and per-plugin `"[info] saved ..."` logging are retained for this mode.

### R4 — In-memory record removal API (for modify-in-place)

4.1 `esm_reader_t` gains an API to replace its whole record set, because `record_t::id` is `const` and `std::vector<record_t>` elements cannot be moved/erased in place. The API rebuilds `m_records` from a supplied set (e.g. `void set_records(std::vector<record_t>)` swapping into `m_records`).
4.2 The new API keeps `esm_reader_t`'s invariants intact: `get_records()` returns the new set, and any cached record pointer (`ptr_record`) is invalidated/reset so a subsequent `select_record` is required before `get_record`.
4.3 The cleaner builds a `std::vector<record_t>` of kept records (copying kept records, substituting the patched header for record 0 when header repair changed it) and commits it via this API on `m_scan.mutable_plugin(idx)`.

### R5 — batch_cleaner_t mode plumbing

5.1 The clean output mode is carried into `batch_cleaner_t` (e.g. a `bool modify_in_place` field on `clean_options_t`, defaulting to the same default as R1.2). `batch_cleaner_t` picks the write mechanism based on it.
5.2 In modify-in-place mode, `clean_all` / `clean_plugin` do not need an output directory; in create-new-files mode they use the output directory. The API accommodates both without forcing an unused parameter to be meaningful in the wrong mode (the output directory may be passed but ignored in modify-in-place mode, or the mode is read from options and the directory used only when needed — the design will define the exact shape).
5.3 `clean_result_t` carries the affected `plugin_idx` (added) so the caller can mark it dirty in modify-in-place mode. The `written` flag is retained only if still meaningful for create-new-files mode; otherwise removed.
5.4 Marking a plugin dirty is done by the yEditor caller, not `batch_cleaner_t` (which lives in yampt.core and must not depend on `plugin_session_t` or Qt). The cleaner reports affected plugins; the caller marks them in modify-in-place mode.

### R6 — Clean orchestration in yEditor

6.1 `plugin_workspace_view_t::on_clean_all()` reads the mode from settings and configures `clean_options_t` accordingly.
6.2 In create-new-files mode it resolves the output directory (`resolve_output_directory()`) as today; in modify-in-place mode it does not require it.
6.3 After cleaning in modify-in-place mode, it performs the dirty-mark / conflict-recompute / nav-rebuild / title-update sequence (R2.4). In create-new-files mode it does none of those (no in-memory change).
6.4 The pre-clean "unsaved changes" prompt behavior: in create-new-files mode, keep the current prompt (writing files is a real operation over possibly-stale state). In modify-in-place mode, the prompt is unnecessary (cleaning only adds more in-memory dirty state); the design will specify whether it is skipped or retained per mode.
6.5 The "need at least 2 plugins" guard is retained for both modes.

### R7 — No regression

7.1 Merged-patch creation/saving, field-edit Apply, exclude/guard-patch marking, and all other existing behaviors are unchanged.
7.2 `batch_cleaner_t` stays in yampt.core and gains no dependency on yampt.editor/yampt.qt types.
7.3 The detection logic (`is_evil_gmst`, `is_junk_cell`, header repair) is unchanged; only the output mechanism is selected by mode.
7.4 The merged-patch output-directory settings (`Paths/OutputDir*`) and `resolve_output_directory()` are unchanged; create-new-files clean continues to use them.

### R8 — Verification

8.1 Modify-in-place is verifiable: after Clean, the affected plugins' in-memory records no longer contain the removed records and the plugins are marked dirty, while the files on disk are unchanged until Save; after Save, the files reflect the cleaned record set and the markers clear.
8.2 Create-new-files is verifiable: after Clean, the output directory contains cleaned copies and the loaded plugins are unchanged (no dirty state).
8.3 Pure logic is covered by `[u]` unit tests without file I/O: `esm_reader_t::set_records` replaces the record set and invalidates the cached pointer; `batch_cleaner_t` in modify-in-place mode on an in-memory scan removes the expected records and reports the affected plugin indices. No `[u]` test writes to disk (create-new-files file output is validated by integration-level checks, not `[u]` tests).

## Open Decisions

Resolved:
- Two modes selected by a settings checkbox (Option A): modify-in-place (deferred/in-memory) vs. create-new-files (immediate/on-disk). (R1)
- Default mode → create new files, preserving current behavior after update. (R1.2)
- "Create new files" target → the configured output directory under the same filename (current behavior), not a renamed copy next to the original. (R3.1)
- Removal mechanism for modify-in-place → new `esm_reader_t` record-set-replacement API (`set_records`), required because `record_t::id` is `const`. (R4)
- Who marks dirty → the yEditor caller, not `batch_cleaner_t`, to keep yampt.core free of yEditor/Qt deps. (R5.4)
- Conflict recompute (modify-in-place) → one `rebuild_conflicts()` after cleaning, then a single nav rebuild. (R2.4)

Deferred to design:
- Exact `clean_all`/`clean_plugin` signature shape for carrying the output directory only when needed (R5.2).
- Whether the pre-clean unsaved prompt is skipped in modify-in-place mode or kept for both (R6.4).
- Whether `clean_result_t::written` is retained (R5.3).
