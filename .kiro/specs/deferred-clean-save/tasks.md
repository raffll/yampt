# Implementation Plan

## Overview

Add a settings choice between two yEditor clean output modes: modify the loaded plugins in place (deferred, in-memory, marked dirty for manual save — reusing the deferred-plugin-save model) vs. create new cleaned files in the output directory (immediate, current behavior). The enabling prerequisite is a new `esm_reader_t::set_records` API, because `record_t::id` is `const` and there is no way to erase records or replace the record vector today. Work order: add the core API and the mode field, branch the cleaner's write mechanism, add the setting and settings-page control, orchestrate the mode in the clean caller (including the per-mode prompt and the in-place dirty/rebuild/title sequence), then tests and docs. The default is create-new-files, preserving current behavior after update.

## Tasks

- [ ] 1. Add esm_reader_t::set_records
  - `void set_records(std::vector<record_t> records);` — `m_records = std::move(records)`, reset `ptr_record = nullptr`, clear `m_key`/`m_value` so a fresh `select_record` is required before `get_record`.
  - _Requirements: R4.1, R4.2_

- [ ] 2. Add mode to clean_options_t and affected-plugin fields to clean_result_t
  - `clean_options_t`: add `bool modify_in_place = false;` (default = create-new-files).
  - `clean_result_t`: add `int plugin_idx = -1;` and `bool committed = false;` (retain `written`).
  - _Requirements: R5.1, R5.3_

- [ ] 3. Branch batch_cleaner_t between in-memory commit and file write
  - Set `result.plugin_idx` at the top of `clean_plugin`.
  - Keep the shared kept-records / patched-header / early-return prefix.
  - Extract the removed-summary logging into a helper used by both modes.
  - Modify-in-place branch: build `std::vector<record_t>` of kept records (substitute the patched header for index 0 when changed), commit via `m_scan.mutable_plugin(plugin_idx).set_records(...)`, set `committed=true`, no file I/O.
  - Create-new-files branch: unchanged ofstream write, sets `written=true`, retains `[info] saved` and empty-result logging.
  - _Requirements: R2.1, R2.2, R2.3, R3.1, R3.3, R3.4, R4.3, R5.2_

- [ ] 4. Update clean_all inclusion condition
  - Include a plugin's result when it was actually changed in either mode (`result.committed || result.written`), so header-only changes and in-place commits are reported to the caller.
  - _Requirements: R5.2, R5.3_

- [ ] 5. Add clean_modify_in_place setting to settings_store_t
  - `clean_modify_in_place()` / `set_clean_modify_in_place(bool)`, INI `Cleaning/ModifyInPlace`, default false (mirror `clean_update_master_sizes`).
  - _Requirements: R1.1, R1.2_

- [ ] 6. Add the output-mode control to the Cleaning settings page
  - In `cleaning_settings_view_t`: an "Output" group with two radio buttons — "Modify loaded plugins in place" and "Create new cleaned files" — each with a tooltip describing its outcome (gui-tooltips rule).
  - `load()` checks the radio from `clean_modify_in_place()`; `save()` writes `set_clean_modify_in_place(...)`.
  - _Requirements: R1.3, R1.4_

- [ ] 7. Orchestrate the mode in on_clean_all
  - Read `m_settings.clean_modify_in_place()`.
  - Pre-clean unsaved prompt: keep for create-new-files, skip for modify-in-place.
  - Resolve the output directory only in create-new-files mode; pass `""` in modify-in-place.
  - Set `options.modify_in_place`; call `clean_all`.
  - In modify-in-place mode after cleaning: mark each `result.plugin_idx` dirty, one `rebuild_conflicts()`, `rebuild_nav_preserving_state()`, `emit unsaved_changes_changed(has_any_unsaved())`. Create-new-files does none of these.
  - Retain the `plugin_count() < 1` guard for both modes.
  - _Requirements: R2.4, R3.2, R6.1, R6.2, R6.3, R6.4, R6.5_

- [ ] 8. Unit-test set_records and in-place clean
  - `[u]`: `esm_reader_t::set_records` replaces the record set and invalidates the cached pointer (fresh `select_record` required before `get_record`).
  - `[u]`: `batch_cleaner_t` with `modify_in_place=true` on an in-memory `plugin_scan_t` removes the expected evil-GMST/junk-cell records from `mutable_plugin(idx).get_records()` and reports the affected `plugin_idx`; no file written.
  - _Requirements: R8.1, R8.3_

- [ ] 9. Register new test files in the tests project
  - Add any new `tests.*.cpp` to `yampt.tests.vcxproj` + `.vcxproj.filters` (flat, disk-mirroring).
  - _Requirements: R8_

- [ ] 10. Update documentation
  - CHANGELOG `[NEW]` (yEditor): Clean All can modify loaded plugins in place (review, then save to overwrite originals) or create new cleaned files; selectable in Settings > Cleaning; default creates new files.
  - `docs/yEditor-Manual.md`: describe both output modes in the Cleaning Plugins section (in-place marks plugins with an asterisk, written only on Save) and update the Settings > Cleaning bullet.
  - README + README.bbcode in sync if cleaning is described there.
  - _Requirements: R1, R2, R3_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1, 2, 5], "depends_on": [] },
    { "wave": 2, "tasks": [3, 6], "depends_on": [1, 2, 5] },
    { "wave": 3, "tasks": [4, 7], "depends_on": [3, 6] },
    { "wave": 4, "tasks": [8, 9, 10], "depends_on": [4, 7] }
  ]
}
```

The core-API chain (1 → 3) is the critical path: the in-memory commit cannot exist without `set_records`. The settings chain (5 → 6) is independent and runs in parallel. Orchestration (7) needs both the branched cleaner (3, 4) and the setting (5). Tests (8) need the cleaner branch and API; docs (10) come last.

## Notes

- Default mode is create-new-files (`Cleaning/ModifyInPlace` false) so cleaning never silently starts overwriting originals after an update.
- `record_t::id` is `const`, so modify-in-place rebuilds the whole record vector via `set_records` rather than erasing in place — this is why the new API is required and is the cleanest fit.
- `batch_cleaner_t` stays in yampt.core: it reports affected plugins via `clean_result_t::plugin_idx`; the yEditor caller marks them dirty. The cleaner gains no dependency on `plugin_session_t` or Qt (R5.4, R7.2).
- The clean guard is `plugin_count() < 1` (single plugin is valid); the requirements' "need at least 2 plugins" refers to merged-patch creation, not clean.
- Detection logic (`is_evil_gmst`, `is_junk_cell`, `header_repair_t`) and the merged-patch output-directory settings are unchanged (R7.3, R7.4).
- Pure logic is unit-tested without disk; file output is verified manually per the integration-test rules. Building and running tests is done manually by the user (no-build-or-test rule).
