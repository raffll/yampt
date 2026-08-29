# Requirements — Deferred Plugin Save in yEditor

## Background — Current Behavior

yEditor lets a user edit a sub-record field in a loaded plugin. The Apply button routes through `field_edit_controller_t::commit_field_edit`, which for a source plugin (`plugin_idx != -1`) calls `commit_to_source`:

```cpp
auto & plugin = m_session.scan().mutable_plugin(request.plugin_idx);
plugin.select_record(request.record_index);
plugin.replace_record(patched_content);                          // in-memory mutation
const bool written = binary_file_io::write_file(plugin.get_records(), plugin_path); // immediate disk write
```

So an edit is applied to the in-memory plugin AND written straight to the plugin file on disk in the same click. There is no dirty state and no explicit save step; every field Apply rewrites the whole plugin file immediately.

The alternative path `commit_to_merge` (`plugin_idx == -1`) writes into the merge store instead of the source plugin and is unrelated to this change.

Related existing pieces:
- `plugin_session_t` already tracks per-plugin sets (`excluded_plugins`, `patch_plugins`) with getters/setters and persists them; the nav filter consults these.
- `nav_tree_model_t::file_node_display_text` builds each plugin row as an icon + `[NNN] filename`, choosing the icon by priority (excluded / guard patch / merge / master / overwrite / regular).
- `view_context_menu_t::build_source_file_menu` builds the source-plugin nav right-click menu (currently: Exclude/Include from Merged Patch, Mark/Unmark Guard Patch).
- The Apply button (`preview_view_t`) tooltip currently reads "Commit field edit to disk".

## Problem

Applying a field edit writes the whole plugin file to disk immediately. The user cannot make several edits and decide when to persist them, and there is no visual indication that a loaded plugin has unsaved in-memory changes. This is risky (every edit rewrites the file) and gives no batching or review point before touching the plugin on disk.

## Consistency With yTranslator (authoritative pattern to mirror)

yTranslator already implements the exact dirty/save model this feature needs. Per the Consistent-Across-Apps rule, yEditor must mirror it rather than invent a new approach. The yTranslator pattern:

- **Per-item dirty flag**: `document_t::is_dirty()` / `set_dirty(bool)`.
- **Session-level queries**: `session_t::has_any_unsaved()`, `save_all()`, `all_dirty()`.
- **Window title asterisk**: `main_window_t::set_unsaved_changes(bool)` toggles the title between `"yTranslator"` and `"yTranslator *"`.
- **Sidebar item marker**: `display_name_t::set_dirty(true)` → `to_string()` prepends `"* "` to render a per-file dirty mark.
- **Context-menu Save gated on dirty**: the sidebar context menu shows Save only when the item is loaded and dirty.
- **Save / Save All in the File menu**: `on_save` (active item) and `on_save_all` (all dirty), each committing then saving and clearing the title flag when nothing remains unsaved.
- **Unsaved prompts** with Save/Discard/Cancel: on unload (per item), on close (`closeEvent`), and before operations (`plugin_operations_controller`).

yEditor's equivalents: item = loaded plugin (`plugin_idx`), sidebar = nav tree, active-window title = `editor_window_t`, operations = merge/clean.

## Goal

Decouple applying a field edit from writing the plugin file, mirroring yTranslator's dirty/save model:

1. Apply commits the edit to the in-memory plugin only and marks the plugin as having unsaved changes.
2. The plugin's nav-tree row shows a dirty marker (asterisk) while it has unsaved changes, and the yEditor window title shows an asterisk while any plugin is unsaved (mirroring `set_unsaved_changes`).
3. A Save action in the plugin's right-click menu (gated on dirty) writes the in-memory plugin to disk and clears the marker; a File-menu Save / Save All mirrors yTranslator.
4. Unsaved plugins prompt Save/Discard/Cancel on unload, on close, and before operations, mirroring yTranslator.

## User-Facing Outcomes

- Clicking Apply on a sub-record field updates the record in memory immediately (the view reflects it, conflicts recompute) but does NOT write the plugin file.
- A plugin with unsaved edits is visibly marked with an asterisk in the nav tree.
- Right-clicking a source plugin offers Save, which writes the plugin to disk and removes the asterisk.
- Saving only writes plugins the user chose to save; nothing is written implicitly on Apply.

## Requirements

### R1 — Apply commits to memory and marks dirty (no disk write)

1.1 `commit_to_source` applies the patched record to the in-memory plugin (`replace_record`) and recomputes the single conflict, exactly as today, but no longer writes the plugin file.
1.2 On a successful in-memory apply of a source-plugin edit, the plugin is marked dirty in the session.
1.3 The `commit_to_merge` path (merge store) is unchanged; it does not participate in plugin dirty state.
1.4 The Apply button's user-facing text/tooltip no longer claims it writes to disk; it reflects an in-memory apply.

### R2 — Session tracks unsaved plugins

2.1 `plugin_session_t` tracks a set of dirty plugins with mark/clear/query accessors, consistent with the existing `excluded_plugins`/`patch_plugins` accessor pattern, and exposes `has_any_unsaved()` mirroring `session_t::has_any_unsaved()`. `save_all_dirty()` lives on the controller that owns the write path and logging (see R7.1), mirroring `session_t::save_all()`.
2.2 Dirty state is keyed by **plugin filename**, identical to how `excluded_plugins`/`patch_plugins` are keyed. Plugin indices are reassigned on every load (`load_plugins_internal` resets the scan), so an index-keyed set would silently point at the wrong plugin after a reload. Filename keys stay stable across nav-tree rebuilds and are looked up with the same `count(filename)` pattern the nav model already uses. The mark/clear/query API translates `plugin_idx → plugin_filename(idx)` at the boundary so callers can keep working in terms of `plugin_idx`.
2.3 Unloading plugins / loading a new set clears dirty state (cleared in the same place `plugin_session_t` resets `m_scan` — both `unload_all()` and `load_plugins_internal()`).
2.4 Dirty state is in-session only; it is not persisted to the INI (an unsaved edit does not survive an app restart, and nothing auto-writes on exit as part of this feature).

### R2b — Window title asterisk (mirror `set_unsaved_changes`)

2b.1 yEditor shows an asterisk in the window title (e.g. `"yEditor *"`) while any plugin is dirty, and removes it when none are, mirroring `main_window_t::set_unsaved_changes`.
2b.2 The title updates immediately on Apply (dirty) and after Save / Save All when nothing remains unsaved.

### R3 — Nav-tree dirty marker

3.1 `nav_tree_model_t::file_node_display_text` shows a `"* "` marker on a plugin row when that plugin is dirty, rendered exactly like yTranslator's `display_name_t::to_string()` (a leading `"* "` prefix), adapted to the icon-prefixed row: the `"* "` goes right after the leading icon and before `[NNN] filename`.
3.2 The marker composes with the existing icon/label scheme without breaking the icon-priority rendering (excluded / guard / merge / master / overwrite / regular). Every icon branch keeps its leading glyph; the `"* "` sits between the glyph and the `[NNN]` label.
3.3 The nav model obtains dirty state through the same threading mechanism already used for `excluded_plugins()`/`patch_plugins()` — a pointer to the session's dirty filename set carried on `nav_tree_filter_t` and consulted with `count(filename)` — not by reaching into unrelated globals.
3.4 The marker appears immediately after Apply and disappears immediately after Save, via the existing nav rebuild/refresh path.
3.5 The `"* "` marker is text, not a member of the icon set. The *Plugin Icons Must Be Consistent Across Panels* rule governs icons only, so the marker does not require mirroring in the record-view column header (`view_tree_model.cpp::headerData`). This is stated explicitly so the icon-consistency rule is not misapplied.

### R4 — Save action in the plugin right-click menu

4.1 `view_context_menu_t::build_source_file_menu` gains a Save action for source plugins.
4.2 Save is enabled only when the plugin is dirty; otherwise it is disabled (greyed), consistent with the project's grey-when-not-applicable convention.
4.3 Save writes the in-memory plugin records to the plugin's own path using the same write path Apply used previously (`binary_file_io::write_file(plugin.get_records(), plugin_path)`), then clears the plugin's dirty state and refreshes the nav tree.
4.4 On write failure, the plugin stays dirty and the failure is logged in the yEditor log style; the file is not left half-written beyond what the writer guarantees.
4.5 The Save action label/tooltip is localized (`QCoreApplication::translate("yEditor", ...)`) consistent with the other actions in this menu, and has a tooltip per the gui-tooltips rule.

### R5 — File-menu Save / Save All (mirror yTranslator)

5.1 yEditor's File menu offers Save (write the currently selected/active dirty plugin) and Save All (write every dirty plugin), mirroring yTranslator's `on_save`/`on_save_all` and their tooltips.
5.2 Each save commits any pending field edit first (equivalent to yTranslator committing the current edit before saving), writes, logs in yEditor style, and clears the title asterisk when nothing remains unsaved.

### R6 — Unsaved prompts (mirror yTranslator)

6.1 yEditor has only "Unload All" (no per-plugin unload). So Unload All shows a SINGLE Save / Discard / Cancel prompt when any plugin is dirty (not one prompt per plugin). Save → `save_all_dirty()` then unload; Discard → unload without saving; Cancel → do not unload. This is the yEditor-appropriate mirror of `sidebar_controller`'s unload prompt.
6.2 Closing yEditor with any dirty plugin prompts Save / Discard / Cancel and honors Cancel (mirroring `closeEvent`).
6.3 Running an operation that could invalidate unsaved edits prompts when any plugin is dirty, mirroring `plugin_operations_controller`. Hook points and prompt shapes:
   - **Create / regenerate merged patch** (`merge_controller_t::create_merged_patch`) → Save / Cancel (see R6c; no Discard, because Discarding then merging would bake in edits the user just chose not to keep on disk).
   - **Clean** (`plugin_workspace_view_t::on_clean_all()`, which runs `batch_cleaner_t` directly and is NOT part of `merge_controller_t`) → Save / Discard / Cancel.
   - **Load existing merged patch** does NOT prompt (see R6c — it reads no source records).
6.4 **Loading a new set of plugins** (Open Folder, Open MO2 Profile, Open OpenMW Config, and the restore-session path — all funnel through `load_plugins_internal`, which resets `m_scan` and discards in-memory edits) prompts Save / Discard / Cancel when any plugin is dirty, BEFORE the scan is reset. Without this, loading a folder while a plugin is dirty silently loses edits. The prompt must fire ahead of the reset in every load entry point, not only on explicit unload.
6.5 All prompt strings are localized with `QCoreApplication::translate("yEditor", ...)`.

### R6c — In-memory edits are the source of truth for the merge; create prompts Save or Cancel

Context: `plugin_scan_t::read_record_content` returns the in-memory `esm` record (the one `commit_to_source` mutated). Every merge read — `auto_merge_t` during create, and all the `copy_*` operations — goes through this path. So a deferred (unsaved) source edit is already visible to the merge.

6c.1 This is intended, not a bug: what the user sees on screen (the in-memory edit) is exactly what the merge uses. The merge does NOT read the on-disk file. R7.2's "unchanged behaviors" is qualified by this: the merge now reflects unsaved edits because Apply no longer writes immediately.
6c.2 Before **creating / regenerating** the merged patch, if any plugin is dirty, prompt **Save unsaved plugins / Cancel** (two options — no Discard). Save writes all dirty plugins to disk first, then the merge proceeds; Cancel aborts the merge entirely and changes nothing. This guarantees that when a merge is generated, the source plugins on disk match what went into the merge, so disk and merge never silently disagree.
6c.3 This create-time prompt is distinct from the generic "regenerate discards manual merge changes" confirmation that already exists. Both may appear: the Save/Cancel unsaved prompt fires first (before any merge work), then the existing regenerate confirmation.
6c.4 **Loading** an existing merged patch (`load_existing_merged_patch`) does NOT read or rewrite source records, so it needs no save prompt. Only create/regenerate does. (Corrects R6.3, which lumped create and load together.)

### R7 — Orchestration placement and no regression

7.1 The save-to-disk logic lives in a controller (e.g. the field-edit or merge controller area), not in `editor_window_t` or the view, per the Anti-Gravity Rule; the context menu and File-menu actions call controller methods.
7.2 Merge-patch editing, exclude/guard-patch marking, conflict recomputation, and all other existing behaviors are unchanged.
7.3 The whole-plugin write still writes the complete record set (`plugin.get_records()`) as today — no partial-file writes.

### R8 — Verification

8.1 Behavior is verifiable: after Apply, the record content in memory reflects the edit and the plugin file on disk is unchanged until Save; after Save, the file on disk reflects the edit and the marker clears.
8.2 Any pure logic extracted (e.g. dirty-set management) is covered by `[u]` unit tests where it does not require file I/O. File-writing behavior is validated by the existing write path and integration-level checks, not new unit tests that touch disk in the `[u]` suite.

## Open Decisions

All resolved.

Resolved by mirroring yTranslator:
- Marker glyph/placement → leading `"* "`, exactly as yTranslator's `display_name_t::to_string()` (which prepends `"* "` when dirty). On yEditor's icon-prefixed rows it sits after the icon and before `[NNN] filename`.
- Column-header mirror → nav-tree-only. yTranslator marks only its sidebar; the `"* "` is text, not an icon, so the icon-consistency rule does not apply (R3.5).
- Dirty key → plugin **filename** (session-scoped, cleared on reload/unload), keyed identically to the existing `excluded_plugins`/`patch_plugins` sets. (An index key was rejected: indices are reassigned on every load, so a dirty index would point at the wrong plugin after a reload, and it could not use the `count(filename)` lookup the nav model already relies on. Callers still pass `plugin_idx`; the session translates to filename at the boundary.)
- Save scope → both per-plugin (context menu) and Save All (File menu), matching `on_save`/`on_save_all`.
- Unsaved prompts → included on unload, close, before clean, before **create/regenerate merge** (Save/Cancel), and before **loading a new set** (the load funnel resets the scan). Loading an existing merge does not prompt. Matches the translator and closes the silent-data-loss gap.
- Save-to-disk placement → `merge_controller_t` (it already owns `m_session`, the logger, `m_nav_view`, and performs file writes via `save_merge_to_file`). `save_plugin(int)` and `save_all_dirty()` are added there; the context menu and File menu call these methods.
- Merge reads in-memory edits (the on-screen state) by design; create/regenerate prompts Save or Cancel so disk and merge stay consistent (R6c). `create_merged_patch` must return a bool (or otherwise signal Cancel) so the workspace caller can skip its post-create view refresh when the user cancels.
