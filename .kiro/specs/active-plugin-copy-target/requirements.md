# Requirements — Active Plugin as Copy Target in yEditor

## Background — Current Behavior

yEditor can copy records, sub-records, groups, and fields from one loaded plugin into a single in-memory "merged patch", which is written to disk as `Merged Patch.esp`. The mechanics:

- `plugin_scan_t` holds one merge target: `int m_merge_plugin_idx` and one `merge_patch_store_t m_merge_store` (plugin_scan.hpp). The merge target is either a phantom plugin created by `set_merge_plugin(filename)` (an empty `loaded_plugin_t` with just a path) or an already-loaded plugin tagged by `set_merge_plugin_from_loaded(int)`.
- Copied content lands in `m_merge_store` via `copy_record_to_merge_raw(rec_type, record_id, content)`. It does NOT go into any plugin's `esm_reader_t` records.
- `rebuild_conflicts()` (plugin_scan.cpp) skips `m_merge_plugin_idx` during normal record ingest, then re-projects every `m_merge_store` record as a phantom column keyed to `m_merge_plugin_idx`. `read_record_content` and `compute_conflict` special-case `m_merge_plugin_idx` to read from the store instead of an esm.
- The merged patch is written by `merge_controller_t::save_merged_patch` → `save_merge_to_file` → `patch_builder_t`, always to `Merged Patch.esp` (hardcoded in several places), with a TES3 header + master list built from contributing plugins.
- The record-view context menu (`view_context_menu_t::show_view_menu`) offers "Copy Record/Sub-Record/Group/Field to Merged Patch" (`build_copy_to_merge_menu` / `build_source_copy_menu`) when the clicked column is a source plugin and a merged patch exists (`has_merge()`).

Separately, yEditor has a deferred in-place edit path for normal loaded plugins:

- `field_edit_controller_t::commit_to_source` writes an edited record into a loaded plugin's own `esm_reader_t`: `mutable_plugin(idx)` → `select_record(record_index)` → `replace_record(patched_content)` → `mark_plugin_dirty(idx)` → `recompute_single_conflict(...)`.
- Dirty state is tracked per filename in `plugin_session_t` (`mark_plugin_dirty`/`clear_plugin_dirty`/`is_plugin_dirty`/`dirty_plugins`/`has_any_unsaved`), shown as a `"* "` prefix in the nav tree and a `yEditor *` window title.
- `merge_controller_t::save_plugin(int)` writes a plugin's own `esm_reader_t` records back to its own path via `binary_file_io::write_file` and clears its dirty flag.

The copy content producers (`copy_whole_record`, `copy_cell_record`, `copy_sub_record`, `copy_group`, `copy_field`, and the `sub_record_merge_t` / `merge_patch_ops_t` helpers) each compute a full record-content string and only the final call writes it to the merge store — they are target-agnostic up to that terminal step.

## Problem

The only copy target is the single merged patch, written to a hardcoded `Merged Patch.esp`. A user often wants to build or edit a **specific, named patch plugin** — pulling selected records from other plugins into one plugin they choose, saved back to that plugin's own file, reviewed in the same side-by-side comparison. Today that requires the merged-patch workflow (auto-merge semantics, fixed filename, store-backed phantom column) which does not fit "I want to hand-assemble this one plugin."

## Goal

Let the user designate one loaded plugin as the **active plugin** (a writable copy target). Copy operations then target the active plugin: copied content is committed into the active plugin's own `esm_reader_t` records (reusing the deferred in-place edit path), the active plugin appears as a column in the comparison view exactly as a normal loaded plugin does, and it is saved back to its own file via the normal dirty/save path. If no active plugin is set, the same copy action instead creates a new user-named plugin, adds the record to it, loads it, and (optionally) designates it active.

## User-Facing Outcomes

- Right-click a loaded plugin in the navigation tree → "Mark as Active Plugin" (and "Unmark as Active Plugin" to clear). Exactly one plugin can be active at a time. The active plugin is visually indicated in the nav tree.
- Right-click a record / sub-record / group / field in another plugin's column → "Copy to Active Plugin" (and the sub-record/group/field variants). The selected content is copied into the active plugin.
- The copied record appears in the active plugin's column in the comparison view immediately, and the active plugin is marked with the unsaved `"* "` asterisk. Saving the active plugin (right-click → Save, or Save All) writes the changes back to the active plugin's own file.
- If no active plugin is set when the user invokes copy, yEditor prompts for a new plugin filename, creates that plugin containing the copied record, loads it, and marks it active so subsequent copies target it.
- The active-plugin designation persists across sessions, like the excluded/guard-patch designations.
- The merged patch and the active plugin are independent: both can exist at once and appear as separate columns.
- The user can select two loaded mods in the navigation tree and create a new empty patch plugin that masters exactly those two, immediately designated as the active plugin, ready to receive copied records that resolve conflicts between the two.

## Requirements

### R1 — Active-plugin designation (core + session)

1.1 `plugin_scan_t` gains a separate `int m_active_plugin_idx = -1` designation, distinct from `m_merge_plugin_idx`. It is a pure marker of an already-loaded, real plugin — NO backing store. Accessors: `set_active_plugin(int)`, `clear_active_plugin()`, `active_plugin_idx()`, `is_active_plugin(int)`, `has_active_plugin()`.
1.2 The active plugin is NOT added to the `rebuild_conflicts` ingest-skip, NOT projected from a store, and NOT special-cased in `read_record_content` / `compute_conflict`. It is a normal loaded plugin that already produces a real column with real esm content; the designation only marks which plugin is the copy target.
1.3 Exactly one plugin may be active. Setting a new active plugin replaces the previous designation.
1.4 A plugin may not be simultaneously the merged patch and the active plugin; setting one on a plugin already serving as the other is rejected or clears the other (design decides which; default: reject with a log message, since the merged patch is store-backed and the active plugin is esm-backed).
1.5 The designation is stored on `plugin_session_t` as a single filename (`std::string m_active_plugin`), with `active_plugin()` / `set_active_plugin(const std::string &)`, mapped to/from the scan's index by filename (the scan uses an index; the session persists a filename, matching how excluded/patch sets store filenames).

### R2 — Session persistence

2.1 The active-plugin filename persists in `yEditor.ini` via `plugin_session_t::save_session_state` / `restore_session_state`, as a single scalar value (e.g. `session/active_plugin`), analogous to the `merge/excluded_plugins` / `merge/patch_plugins` lists but scalar.
2.2 On restore, the filename is re-mapped to the loaded plugin's index after plugins load; if the named plugin is not present in the restored session, the active designation is silently dropped (no error).
2.3 Changing the active designation from the context menu persists immediately (same `save_session_state(settings_dir + "yEditor.ini")` call the Exclude/Guard actions use) and rebuilds the nav tree.

### R3 — Copy into the active plugin (in-place, esm-backed)

3.1 The existing content producers (`copy_whole_record`, `copy_cell_record`, `copy_sub_record`, `copy_group`, `copy_field`) are reused to compute the record content. Only the terminal landing step changes: instead of `copy_record_to_merge_raw` (store), the content is committed into the active plugin's own `esm_reader_t` via the `commit_to_source` pattern (`mutable_plugin` → `select_record` → `replace_record`), then `mark_plugin_dirty(active_idx)` and `recompute_single_conflict(...)`.
3.2 When the record already exists in the active plugin, `replace_record` overwrites it in place (select the active plugin's existing version first).
3.3 When the record does NOT yet exist in the active plugin, it must be ADDED. `esm_reader_t` currently has no append/add API and `record_t::id` is `const`, so a new `esm_reader_t` method is required to append a new record (constructing it in place) and refresh its index. This is the one genuine new core capability (R6).
3.4 After a copy, the active plugin's column shows the copied record immediately, the plugin is marked dirty (asterisk), and conflicts recompute for that record. No file is written until the user saves (deferred model) — copies into the active plugin do NOT write to disk on each copy (unlike the merged-patch copy, which calls `save_merged_patch` every time).
3.5 The sub-record / group / field variants target the active plugin's current version of the record as the base to patch (mirroring `ensure_merge_record`, but the "ensure" seeds/looks up the record in the active plugin's esm rather than the merge store).

### R4 — Active plugin as a comparison column

4.1 The active plugin already appears as a normal loaded-plugin column (R1.2); no phantom-column projection is added. "Shows in comparison like the merged patch" is satisfied because it is a real column whose content updates live as records are copied in.
4.2 The active plugin's column is visually distinguished in the nav tree (an icon/marker), following the plugin-icon precedence rules (a new marker slot for "active", coordinated across `nav_tree_model` and `view_tree_model::headerData` per the plugin-icons-consistent rule).
4.3 The record-view context menu, when the clicked column is the active plugin, offers removal/edit affordances consistent with a normal editable plugin (field edit is already available via Enable Editing); no store-style "remove from merged patch" is shown for the active plugin.

### R5 — Copy-to-new-plugin fallback (no active plugin set)

5.1 When a copy action is invoked and no active plugin is set, yEditor prompts for a new plugin filename (a `QInputDialog`, title/prompt wrapped for translation).
5.2 A new plugin file is created at the resolved output directory (reusing `resolve_output_directory()` per load mode) with the chosen filename, containing the copied record. `patch_builder_t::save` builds the TES3 header (version 1.3) and a master list derived from the source plugin(s) of the copied record (reuse `collect_contributing_plugins` / `build_master_list` logic scoped to the one record).
5.3 The new plugin is then loaded into the scan (`load_plugin`), conflicts rebuilt, and it is designated the active plugin so subsequent copies target it.
5.4 If the chosen filename already exists as a loaded plugin or on disk, the design defines the behavior (default: refuse and ask again, to avoid clobbering; the user can instead mark the existing plugin active and copy into it).
5.5 The new plugin is created on disk immediately (it must exist to be loaded as a column), but subsequent copies into it follow the deferred in-place model (R3.4) and require Save to persist.

### R5b — Create a new patch plugin for two selected mods

5b.1 The user can select exactly two loaded, non-merge plugins in the navigation tree and invoke "Create Patch for Selected…" (label TBD in design). yEditor prompts for a new plugin filename.
5b.2 A new plugin file is created at the resolved output directory (`resolve_output_directory()`) with a TES3 header (version 1.3) whose master list is exactly the two selected plugins, in load order. It starts empty (header only, no records) — it is a patch scaffold, not a copy of either mod.
5b.3 The new plugin is loaded into the scan (`load_plugin`), conflicts are rebuilt, and it is designated the active plugin, so the user's subsequent record/sub-record/field copies (R3) land in it, resolving conflicts between the two selected mods.
5b.4 This reuses the R5 new-plugin creation machinery (prompt, `patch_builder_t::save`, load, set-active); it differs from R5 only in that (a) it is triggered from a two-plugin nav selection rather than as a no-active copy fallback, (b) the master list is the two selected plugins explicitly rather than derived from a single copied record's source, and (c) the plugin starts empty. The two flows share one creation helper parameterized by "which masters" and "initial record(s)".
5b.5 Selection guard: the action is offered only when exactly two non-merge plugins are selected. With a different count, the action is absent (or disabled). Filename collision behaves as R5.4.
5b.6 Master ordering: the two masters are written in their current load order (the lower-priority plugin first), so the patch loads after both and its overrides win. If either selected plugin is itself a master (`.esm`) vs a plugin (`.esp`), both are still listed as masters of the patch (the patch is an `.esp` that overrides both).

### R6 — esm_reader_t append capability (core)

6.1 `esm_reader_t` gains a method to append a new record from `(rec_type, record_id, content)`, constructing the `record_t` in place (working around the `const` `record_t::id`) and updating `m_records` plus the reader's derived index so the new record is immediately selectable and appears in `get_records()`.
6.2 The method keeps `esm_reader_t` invariants intact (cached `ptr_record` reset; `get_records()` returns the new set; `plugin_index_t` for that plugin refreshed so the new record participates in `rebuild_conflicts`).
6.3 This is pure yampt.core; no Qt, no editor dependency.

### R7 — Save path

7.1 Saving the active plugin uses the existing `merge_controller_t::save_plugin(active_idx)` (writes `esm_reader_t` records to the plugin's own path, clears dirty). Save All (`save_all_dirty`) already covers it since the active plugin is a normal dirty plugin.
7.2 The unsaved asterisk (nav tree + window title) and the `has_any_unsaved` prompt behavior apply to the active plugin with no special-casing.

### R8 — No regression

8.1 The merged-patch workflow (create, copy to merged patch, remove from merged patch, save `Merged Patch.esp`) is unchanged. The active plugin is a parallel, independent concept; `m_merge_plugin_idx` and its store are untouched.
8.2 `plugin_scan_t` / core stays free of Qt and editor dependencies. The prompt/dialog and filename mapping live in the editor.
8.3 Conflict detection, nav colors, and counts are unaffected except that the active plugin (a normal plugin) contributes its records as it always would; copying a record in changes that record's conflict state via the normal `recompute_single_conflict`.
8.4 Field-edit Apply, exclude/guard-patch marking, cleaning, and session save/restore continue to work; the new `session/active_plugin` key is additive.

### R9 — Verification

9.1 Pure `[u]` unit tests (in-memory synthetic records, no file I/O): `esm_reader_t` append adds a selectable record present in `get_records()`; copying a record into an active plugin (via the core landing path) results in the active plugin's esm containing the record with the copied content; designating/clearing active updates `is_active_plugin`/`has_active_plugin`; active and merge designations can coexist on different plugins and are rejected on the same plugin per R1.4.
9.2 The copy-to-new-plugin file creation is validated at integration level (writes a loadable single-record plugin with a correct TES3 header/masters), not in `[u]` tests.
9.3 Tests are written before the code they cover where they encode fixed expected behavior (test-before-fix / new-feature testing). Building and running tests are done manually by the user (project no-build rule).

## Open Decisions

Resolved:
- Active plugin is a SEPARATE `m_active_plugin_idx`, not a reuse of `m_merge_plugin_idx` — because the merge idx is store-backed (phantom column, store diversion in read/compute) while the active plugin must be esm-backed and saved to its own file, and the two must coexist. (R1)
- Copy into active plugin routes through the `commit_to_source` in-place path, deferred save (no per-copy disk write). (R3)
- No-active-plugin fallback creates a new user-named plugin via `patch_builder_t`, loads it, marks it active. (R5)
- The "patch two mods" flow is the same new-plugin machinery as R5, differing only in trigger (two-plugin nav selection), masters (the two selected plugins), and starting empty. One shared `create_new_plugin` helper serves both. (R5b)
- Active designation persisted as a single scalar filename in `[session]`. (R2)
- Reuse existing copy content producers; only the terminal landing step branches by target. (R3.1)

Deferred to design:
- R1.4: reject vs. auto-clear when marking a plugin active that is already the merged patch (and vice versa).
- R5b: exact action label ("Create Patch for Selected…"); whether the nav tree is switched to multi-select or the second plugin is chosen via a follow-up picker; whether `patch_builder_t::save` already allows a header-only (zero-record) plugin or needs an allow-empty path.
- R4.2: exact icon/marker for the active plugin and its precedence among the existing plugin icons.
- R5.4: exact behavior when the new-plugin filename collides with an existing plugin/file.
- Whether "Copy to Active Plugin" and the merged-patch copy actions appear together in the menu when both a merged patch and an active plugin exist (both offered) or are mutually gated.
- The exact shape of the new `esm_reader_t` append method (single append vs. rebuild-records) given `record_t::id` is const.
