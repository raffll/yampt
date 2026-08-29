# Tasks — Deferred Plugin Save in yEditor

Mirror yTranslator's dirty/save model. Order: session dirty state, Apply change, Save controller methods, nav marker, title asterisk, context menu, File menu, prompts, UI wiring, Apply text, docs.

## 1. Session dirty state (mirror session_t)

- [ ] 1.1 Add to `plugin_session_t`: `m_dirty_plugins` set + `mark_plugin_dirty`, `clear_plugin_dirty`, `is_plugin_dirty`, `dirty_plugins`, and `has_any_unsaved()` (mirrors `session_t::has_any_unsaved`), following the `excluded_plugins`/`patch_plugins` pattern. (R2.1, R2b)
- [ ] 1.2 Clear `m_dirty_plugins` on load reset / unload. (R2.3)

## 2. Apply commits to memory, marks dirty, no write

- [ ] 2.1 In `commit_to_source`: keep `replace_record` + `recompute_single_conflict`; remove the `write_file` call + failure branch; add `mark_plugin_dirty(request.plugin_idx)`; keep emitting `record_modified(false, plugin_path)`. (R1.1, R1.2)
- [ ] 2.2 Confirm `commit_to_merge` untouched. (R1.3)

## 3. Save controller methods

- [ ] 3.1 Add `save_plugin(int) -> bool` to the controller owning `m_session` + logging: write `get_records()` to `plugin_path(idx)`; success → `clear_plugin_dirty` + `[info] saved`; failure → `[error] failed to save`, keep dirty. (R5.2, R7.1, R7.3)
- [ ] 3.2 Add `save_all_dirty()` iterating `dirty_plugins()` calling `save_plugin`, mirroring `session_t::save_all`. (R2.1, R5.1)

## 4. Nav-tree dirty marker (mirror sidebar marker)

- [ ] 4.1 Extend nav filter state to carry the dirty plugin set (mirror `excluded_plugins`/`patch_plugins`). (R3.3)
- [ ] 4.2 In `display_text_for_file`, render the dirty marker matching how yTranslator's `display_name_t::set_dirty` shows it, adapted to the icon-prefixed row (preserve leading icon glyph). (R3.1, R3.2)

## 5. Window title asterisk (mirror set_unsaved_changes)

- [ ] 5.1 Add `editor_window_t::set_unsaved_changes(bool)` with a guard member, toggling title `"yEditor"` / `"yEditor *"`, copying `main_window_t::set_unsaved_changes`. (R2b.1)
- [ ] 5.2 Call it from the workspace view after Apply (true) and after Save/Save All (`has_any_unsaved()`). (R2b.2)

## 6. Context-menu Save (mirror derive_context_menu gating)

- [ ] 6.1 Inject the save controller into `view_context_menu_t`. (R7.1)
- [ ] 6.2 In `build_source_file_menu`, add localized "Save" with tooltip, enabled only when `is_plugin_dirty(info.plugin_idx)`; on trigger call `save_plugin` then `rebuild_preserving_state()` + update title. (R4.1–R4.5)

## 7. File-menu Save / Save All (mirror on_save/on_save_all)

- [ ] 7.1 Add File-menu `Save` and `Save All` actions with tooltips; wire to `editor_window_t::on_save` / `on_save_all`. (R5.1)
- [ ] 7.2 `on_save` saves the selected dirty plugin; `on_save_all` calls `save_all_dirty`; both refresh nav and update title. Commit/flush any pending preview edit first. (R5.2)

## 8. Unsaved prompts (mirror translator)

- [ ] 8.1 Unload-a-dirty-plugin prompt (Save/Discard/Cancel), mirroring `sidebar_controller`. (R6.1)
- [ ] 8.2 `editor_window_t::closeEvent` prompt when `has_any_unsaved()`, honoring Cancel, copying `main_window_t::closeEvent`. (R6.2)
- [ ] 8.3 Before merge/clean operations, prompt to save when dirty, mirroring `plugin_operations_controller`. (R6.3)
- [ ] 8.4 All prompt strings via `QCoreApplication::translate("yEditor", ...)`. (R6.4)

## 9. UI wiring

- [ ] 9.1 On `record_modified` (source case): refresh nav so the asterisk appears + update title. (R3.4, R2b.2)
- [ ] 9.2 Include `dirty_plugins()` in `build_effective_filter`. (R3.3)

## 10. Apply button text

- [ ] 10.1 In `preview_view.cpp`, change the Apply tooltip from "Commit field edit to disk" to in-memory phrasing; keep label "Apply". (R1.4)

## 11. Documentation

- [ ] 11.1 `docs/yEditor-Manual.md`: Apply updates the loaded plugin in memory and marks it unsaved; Save (context menu) / Save All (File menu) write to disk; unsaved prompts on unload/close/operations. (manual-style)
- [ ] 11.2 `README.md` + `docs/README.bbcode` (mirror) + `CHANGELOG.md` (2.0beta): `[CHANGE]` Apply no longer writes immediately; `[NEW]` plugin Save / Save All and unsaved indicators. No tests/scripts/build details. (changelog-categories)

## 12. Unit tests (where pure)

- [ ] 12.1 `[u]` tests for dirty-set + `has_any_unsaved` accessors without file I/O: mark/clear/query, any-unsaved, cleared on reset. (R8.2)

## Notes

- yEditor-only; deliberately mirrors yTranslator's dirty/save model per Consistent-Across-Apps.
- Whole-plugin write unchanged (`get_records()`), just deferred.
- Building/running tests are manual (project no-build rule); no "run tests" step.
