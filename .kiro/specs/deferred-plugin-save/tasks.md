# Tasks — Deferred Plugin Save in yEditor

Mirror yTranslator's dirty/save model. Order: session dirty state, Apply change, Save controller methods, nav marker, title asterisk, context menu, File menu, prompts, UI wiring, Apply text, docs.

## 1. Session dirty state (mirror session_t)

- [x] 1.1 Add to `plugin_session_t`: `std::set<std::string> m_dirty_plugins` (filename-keyed, like `m_excluded_plugins`/`m_patch_plugins`) + `mark_plugin_dirty(int)`, `clear_plugin_dirty(int)`, `is_plugin_dirty(int)` (each translating `plugin_idx → plugin_filename(idx)` at the boundary), `dirty_plugins()` returning the filename set, and `has_any_unsaved()` (mirrors `session_t::has_any_unsaved`). Do NOT key by index. (R2.1, R2.2, R2b)
- [x] 1.2 Clear `m_dirty_plugins` beside the `m_scan = plugin_scan_t()` reset in BOTH `unload_all()` and `load_plugins_internal()`. (R2.3)

## 2. Apply commits to memory, marks dirty, no write

- [x] 2.1 In `commit_to_source`: keep `replace_record` + `recompute_single_conflict`; remove the `write_file` call + failure branch; add `mark_plugin_dirty(request.plugin_idx)`; keep emitting `record_modified(false, plugin_path)`. (R1.1, R1.2)
- [x] 2.2 Confirm `commit_to_merge` untouched. (R1.3)

## 3. Save controller methods (on merge_controller_t)

- [x] 3.1 Add `merge_controller_t::save_plugin(int) -> bool`: write `scan.mutable_plugin(idx).get_records()` to `scan.plugin_path(idx)`; success → `clear_plugin_dirty` + `[info] saved`; failure → `[error] failed to save`, keep dirty. Placed on `merge_controller_t` (already owns `m_session`, `m_log`, `m_nav_view`, does file writes). (R5.2, R7.1, R7.3)
- [x] 3.2 Add `merge_controller_t::save_all_dirty()` iterating a **copy** of `dirty_plugins()` (save_plugin mutates the set via clear) calling `save_plugin`, mirroring `session_t::save_all`. (R2.1, R5.1)

## 4. Nav-tree dirty marker (mirror sidebar marker)

- [x] 4.1 Add `nav_tree_filter_t::set_dirty_plugins(const std::set<std::string>*)` + `dirty_plugins()` pointer accessor, mirroring the excluded/patch pointers. (R3.3)
- [x] 4.2 In `file_node_display_text` (the actual function name), prepend `"* "` to `display_buffer` once when `m_filter.dirty_plugins()->count(filename)`, before the icon-priority early-return branches, so the row renders `<icon> * [NNN] filename` — matching yTranslator's leading `"* "` in `display_name_t::to_string()`. Text, not an icon — no `view_tree_model.cpp::headerData` change (R3.5). (R3.1, R3.2)

## 5. Window title asterisk (mirror set_unsaved_changes)

- [x] 5.1 Add `editor_window_t::set_unsaved_changes(bool)` with a guard member, toggling title `"yEditor"` / `"yEditor *"`, copying `main_window_t::set_unsaved_changes`. (R2b.1)
- [x] 5.2 Call it from the workspace view after Apply (true) and after Save/Save All (`has_any_unsaved()`). (R2b.2)

## 6. Context-menu Save (mirror derive_context_menu gating)

- [x] 6.1 Inject the save controller into `view_context_menu_t`. (R7.1)
- [x] 6.2 In `build_source_file_menu`, add localized "Save" with tooltip, enabled only when `is_plugin_dirty(info.plugin_idx)`; on trigger call `save_plugin` then `rebuild_preserving_state()` + update title. (R4.1–R4.5)

## 7. File-menu Save / Save All (mirror on_save/on_save_all)

- [x] 7.1 Add File-menu `Save` and `Save All` actions with tooltips; wire to `editor_window_t::on_save` / `on_save_all`, delegating to `merge_controller_t`. (R5.1)
- [x] 7.2 `on_save` saves the selected dirty plugin; `on_save_all` calls `save_all_dirty`; both refresh nav and update title. Do NOT implicitly commit in-progress (un-Applied) preview text — Save writes only already-Applied (dirty) records. (R5.2)

## 8. Unsaved prompts (mirror translator)

- [x] 8.1 In `plugin_workspace_view_t::on_unload_all()`, if `has_any_unsaved()` show ONE Save/Discard/Cancel prompt BEFORE `m_session->unload_all()` (yEditor has no per-plugin unload). Save → `save_all_dirty()` then unload; Discard → unload; Cancel → return. (R6.1)
- [x] 8.2 `editor_window_t::closeEvent` prompt when `has_any_unsaved()`, honoring Cancel (event->ignore), copying `main_window_t::closeEvent`. (R6.2)
- [x] 8.3 Before create/regenerate merge (`merge_controller_t::create_merged_patch`): if dirty, prompt Save / Cancel (no Discard) — Save → `save_all_dirty()` then continue; Cancel → return false. Fires before the existing regenerate confirmation. Load-existing merge gets NO prompt. Before clean (`plugin_workspace_view_t::on_clean_all()`): Save / Discard / Cancel. (R6.3, R6c)
- [x] 8.4 Change `create_merged_patch` to return `bool`; in `on_create_merged_patch`, skip the post-create record re-display + status update when it returns false. (R6c.2, resolves Cancel propagation)
- [x] 8.5 Load-a-new-set prompt (Save/Discard/Cancel) at all load entry points (Open Folder, Open MO2, Open OpenMW, restore session) BEFORE the scan reset in `load_plugins_internal`, closing the silent-data-loss gap. (R6.4)
- [x] 8.6 All prompt strings via `QCoreApplication::translate("yEditor", ...)`. (R6.5)

## 9. UI wiring

- [x] 9.1 On `record_modified` (source case): remove the `"[info] saved"` log line, refresh nav so the asterisk appears + `set_unsaved_changes(true)`. Merge case unchanged. Leave the separate `preview_view_t::edit_committed` handler as-is (it only re-displays the record; do not add a second dirty/title update there). (R3.4, R2b.2)
- [x] 9.2 Wire `m_nav_view->set_dirty_plugins(&m_session->dirty_plugins())` in the workspace constructor beside the excluded/patch pointer wiring. (Not via `build_effective_filter`, which composes only the record filter.) (R3.3)

## 10. Apply button text

- [x] 10.1 In `preview_view.cpp`, change the Apply tooltip from "Commit field edit to disk" to in-memory phrasing; keep label "Apply". (R1.4)

## 11. Documentation

- [x] 11.1 `docs/yEditor-Manual.md`: Apply updates the loaded plugin in memory and marks it unsaved; Save (context menu) / Save All (File menu) write modified source plugins to disk; the merged patch saves automatically as you edit it (Save All is not the merge-save button); creating the merged patch offers to save unsaved plugins first or cancel; unsaved prompts on unload/close/clean/load. (manual-style)
- [x] 11.2 `README.md` + `docs/README.bbcode` (mirror) + `CHANGELOG.md` (2.0beta): `[CHANGE]` Apply no longer writes immediately; `[NEW]` plugin Save / Save All and unsaved indicators. No tests/scripts/build details. (changelog-categories)

## 12. Unit tests (where pure)

- [x] 12.1 `[u]` tests for dirty-set + `has_any_unsaved` accessors without file I/O: mark/clear/query, any-unsaved, cleared on reset. (R8.2)

## Notes

- yEditor-only; deliberately mirrors yTranslator's dirty/save model per Consistent-Across-Apps.
- Whole-plugin write unchanged (`get_records()`), just deferred.
- Building/running tests are manual (project no-build rule); no "run tests" step.
