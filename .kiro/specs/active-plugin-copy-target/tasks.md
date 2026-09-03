# Tasks — Active Plugin as Copy Target in yEditor

Order: core designation + esm append first (with unit tests), then the editor copy landing branch, then the no-active new-plugin fallback, then menu/nav/persistence wiring, then docs. Core logic stays in `yampt.core`; dialog/menu/mapping in `yampt.editor`.

## 1. Core: esm_reader append

- [ ] 1.1 Add `esm_reader_t::append_record(const std::string & record_id, const std::string & content)` (esm_reader.hpp/.cpp): push a new `record_t`, invalidate cached `ptr_record`/`m_key`/`m_value`. (R6.1, R6.2)
- [ ] 1.2 Resolve the `record_t` const-id vector-growth question: attempt direct `push_back`; if the const `id` member makes the vector move/copy ill-formed on reallocation, switch record storage to `std::vector<std::unique_ptr<record_t>>` or `std::deque<record_t>`. Document the chosen approach in code. (R6.1, design Component 2)
- [ ] 1.3 Unit tests `tests.esm_reader_append.cpp` (register in vcxproj + filters): appended record present in `get_records()`, selectable, cached selection invalidated. (R9.1)

## 2. Core: plugin_scan active designation + re-index

- [ ] 2.1 Add `m_active_plugin_idx` + `set_active_plugin`/`clear_active_plugin`/`active_plugin_idx`/`is_active_plugin`/`has_active_plugin` to plugin_scan.hpp/.cpp. `set_active_plugin` rejects the merge index. Do NOT modify rebuild_conflicts / read_record_content / compute_conflict. (R1.1–R1.4)
- [ ] 2.2 Add `plugin_scan_t::refresh_index(int plugin_idx)` rebuilding that one plugin's `plugin_index_t` from its esm (so an appended record is ingested by the next `rebuild_conflicts` / recompute). (R6.2)
- [ ] 2.3 Unit tests `tests.plugin_scan_active.cpp` (vcxproj + filters): designation getters; active+merge coexist on different plugins; active-on-merge rejected. (R9.1)

## 3. Core: landing a copied record into a loaded plugin

- [ ] 3.1 Unit test (extend task 2.3 file or new): a helper that, given an in-memory scan + active idx + (rec_type, record_id, content), appends-or-replaces the record in the active plugin's esm and the record's version in `entries()` (after `refresh_index` + `recompute_single_conflict`) points at the active plugin with the copied content. Covers absent (append) and present (replace) cases. (R9.1)

## 4. Editor: copy landing branch

- [ ] 4.1 Add `copy_target_t { merged_patch, active_plugin }` and `merge_controller_t::land_copied_record(target, rec_type, record_id, content)`. merged_patch = current tail (copy_record_to_merge_raw + refresh + save_merged_patch); active_plugin = commit_to_source-style (select+replace or append+refresh_index) + mark_plugin_dirty + recompute + refresh, NO disk write. (R3.1–R3.4)
- [ ] 4.2 Add `ensure_active_record` (analogue of `ensure_merge_record`) that looks up / seeds the record in the active plugin's esm and returns current content to patch. (R3.5)
- [ ] 4.3 Parameterize `copy_whole_record` / `copy_cell_record` / `copy_sub_record` / `copy_group` / `copy_field` by `copy_target_t`, routing the terminal step through `land_copied_record` and the "ensure base" step through the merge-store or active-plugin variant per target. Content production (`read_source_content`, `sub_record_merge_t`, `merge_patch_ops_t`) unchanged. (R3.1)

## 5. Editor: new-plugin creation (shared helper)

- [ ] 5.1 Add `new_plugin_spec_t { filename; master_plugin_indices; seed_records; }` and `merge_controller_t::create_new_plugin(spec)`: prompt/validate filename externally, resolve path via `resolve_output_directory()`, refuse on collision (R5.4), build masters via `build_master_list` scoped to `master_plugin_indices` (sorted by load order), `patch_builder_t::save` with `seed_records` (may be empty → header-only), `load_plugin`, `rebuild_conflicts`, set active (scan + session), persist, rebuild nav. Confirm/allow `patch_builder_t::save` writing a zero-record plugin. (R5.1–R5.5, R5b.2, R5b.6)
- [ ] 5.2 No-active copy fallback: when a copy action fires and `!has_active_plugin()`, prompt for a filename and call `create_new_plugin` with masters = the copied record's source plugin(s) and one seed record (the copied content). (R5)
- [ ] 5.3 Two-mod patch flow: from a two-plugin nav selection, prompt for a filename and call `create_new_plugin` with masters = the two selected plugin indices (load order) and no seed records (empty scaffold). (R5b.1–R5b.4, R5b.6)

## 6. Editor: session persistence

- [ ] 6.1 Add `plugin_session_t::m_active_plugin` (filename) + `active_plugin()`/`set_active_plugin`. (R1.5)
- [ ] 6.2 Persist in `save_session_state` (`session/active_plugin`) and restore in `restore_session_state`, resolving filename→index after plugins load; drop silently if absent. (R2.1–R2.3)

## 7. Editor: context menu + nav marker

- [ ] 7.1 `view_context_menu_t::build_source_file_menu`: add "Mark as Active Plugin" / "Unmark as Active Plugin" (reject if plugin is the merged patch, log). Handler sets/clears session + scan designation, persists, rebuilds nav. Tooltip per gui-tooltips. (R7, R1.4)
- [ ] 7.2 `show_view_menu`: extend copy dispatch so a source column offers "Copy … to Active Plugin" (active set) or "Copy … to New Plugin…" (none set), alongside the existing "Copy … to Merged Patch" when `has_merge()`. Parameterize `build_copy_to_merge_menu` / `build_source_copy_menu` by `copy_target_t` + label to avoid duplication. (R4.3, R5.2, design Component 7)
- [ ] 7.4 Two-mod patch action: enable multi-select on the nav tree; in the nav menu, when exactly two non-merge plugins are selected, offer "Create Patch for Selected…" → prompt filename → task 5.3 flow. Absent/disabled otherwise. Tooltip per gui-tooltips. (R5b.1, R5b.5, design Component 4b/7)
- [ ] 7.3 Add an active-plugin icon/marker in the plugin-icon precedence, updated in BOTH `nav_tree_model.cpp` and `view_tree_model.cpp::headerData` (plugin-icons-consistent). Wire an active-plugin marker into the nav model in `plugin_workspace_view.cpp` like excluded/patch/dirty. (R4.2)

## 8. Documentation

- [ ] 8.1 `docs/yEditor-Manual.md`: describe marking a plugin active, copying records into it (saved to its own file, deferred), and the copy-to-new-plugin fallback. Prose, no internals. (manual-style)
- [ ] 8.2 `README.md` + `docs/README.bbcode` (mirror) + `CHANGELOG.md` (unreleased, yEditor, `[NEW]`). No tests/scripts/build details. (changelog-categories, readme rules, bbcode-sync)

## Notes

- yEditor-only; yTranslator has no plugin scan.
- Active plugin is SEPARATE from the merged patch: `m_active_plugin_idx` is a pure marker (no store, no ingest-skip, no read/compute special-case). Both can coexist.
- Copies into the active plugin are DEFERRED (mark dirty, Save writes) — unlike merged-patch copies which write each time.
- The one genuine new core capability is `esm_reader_t::append_record` (needed for records not yet in the active plugin); mind the const `record_t::id`.
- Building and running tests are done manually by the user (project no-build rule); tasks produce tests as artifacts, no "run tests" step.
- Confirm the deferred plugin-save infrastructure (mark_plugin_dirty / save_plugin / asterisk) is reused unchanged; do not add a parallel save path.
