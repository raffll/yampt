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
- `nav_tree_model_t::display_text_for_file` builds each plugin row as an icon + `[NNN] filename`, choosing the icon by priority (excluded / guard patch / merge / master / overwrite / regular).
- `view_context_menu_t::build_source_file_menu` builds the source-plugin nav right-click menu (currently: Exclude/Include from Merged Patch, Mark/Unmark Guard Patch).
- The Apply button (`preview_view_t`) tooltip currently reads "Commit field edit to disk".

## Problem

Applying a field edit writes the whole plugin file to disk immediately. The user cannot make several edits and decide when to persist them, and there is no visual indication that a loaded plugin has unsaved in-memory changes. This is risky (every edit rewrites the file) and gives no batching or review point before touching the plugin on disk.

## Consistency With yTranslator (authoritative pattern to mirror)

yTranslator already implements the exact dirty/save model this feature needs. Per the Consistent-Across-Apps rule, yEditor must mirror it rather than invent a new approach. The yTranslator pattern:

- **Per-item dirty flag**: `document_t::is_dirty()` / `set_dirty(bool)`.
- **Session-level queries**: `session_t::has_any_unsaved()`, `save_all()`, `unsaved_docs()`.
- **Window title asterisk**: `main_window_t::set_unsaved_changes(bool)` toggles the title between `"yTranslator"` and `"yTranslator *"`.
- **Sidebar item marker**: `derive_display_name(entry, is_loaded, is_dirty)` → `display_name_t::set_dirty(true)` renders a per-file dirty mark; `update_sidebar_item(path)` refreshes it.
- **Context-menu Save gated on dirty**: `derive_context_menu` shows Save only when the item is loaded and dirty.
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

2.1 `plugin_session_t` tracks a set of dirty plugin indices with mark/clear/query accessors, consistent with the existing `excluded_plugins`/`patch_plugins` accessor pattern, and exposes `has_any_unsaved()` and `save_all_dirty()` mirroring `session_t::has_any_unsaved()`/`save_all()`.
2.2 Dirty state is keyed so it remains correct across nav-tree rebuilds (which preserve state) for the current load; the identifier used must stay stable for the loaded session.
2.3 Unloading plugins / loading a new set clears dirty state.
2.4 Dirty state is in-session only; it is not persisted to the INI (an unsaved edit does not survive an app restart, and nothing auto-writes on exit as part of this feature).

### R2b — Window title asterisk (mirror `set_unsaved_changes`)

2b.1 yEditor shows an asterisk in the window title (e.g. `"yEditor *"`) while any plugin is dirty, and removes it when none are, mirroring `main_window_t::set_unsaved_changes`.
2b.2 The title updates immediately on Apply (dirty) and after Save / Save All when nothing remains unsaved.

### R3 — Nav-tree dirty marker

3.1 `nav_tree_model_t::display_text_for_file` shows an asterisk marker on a plugin row when that plugin is dirty.
3.2 The marker composes with the existing icon/label scheme without breaking the icon-priority rendering (excluded / guard / merge / master / overwrite / regular).
3.3 The nav model obtains dirty state through the same threading mechanism already used for `excluded_plugins()`/`patch_plugins()` (via the filter/session), not by reaching into unrelated globals.
3.4 The marker appears immediately after Apply and disappears immediately after Save, via the existing nav rebuild/refresh path.

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

6.1 Unloading a dirty plugin prompts Save / Discard / Cancel, mirroring the per-document unload prompt in `sidebar_controller`.
6.2 Closing yEditor with any dirty plugin prompts Save / Discard / Cancel and honors Cancel (mirroring `closeEvent`).
6.3 Running an operation that could invalidate unsaved edits (e.g. creating/loading the merged patch, cleaning) prompts to save first when any plugin is dirty, mirroring `plugin_operations_controller`.
6.4 All prompt strings are localized with `QCoreApplication::translate("yEditor", ...)`.

### R7 — Orchestration placement and no regression

7.1 The save-to-disk logic lives in a controller (e.g. the field-edit or merge controller area), not in `editor_window_t` or the view, per the Anti-Gravity Rule; the context menu and File-menu actions call controller methods.
7.2 Merge-patch editing, exclude/guard-patch marking, conflict recomputation, and all other existing behaviors are unchanged.
7.3 The whole-plugin write still writes the complete record set (`plugin.get_records()`) as today — no partial-file writes.

### R8 — Verification

8.1 Behavior is verifiable: after Apply, the record content in memory reflects the edit and the plugin file on disk is unchanged until Save; after Save, the file on disk reflects the edit and the marker clears.
8.2 Any pure logic extracted (e.g. dirty-set management) is covered by `[u]` unit tests where it does not require file I/O. File-writing behavior is validated by the existing write path and integration-level checks, not new unit tests that touch disk in the `[u]` suite.

## Open Decisions (resolve during design)

- **Marker glyph/placement**: match how yTranslator's `display_name_t::set_dirty` renders its marker so both apps look the same, adapted to the nav tree's icon-prefixed rows. Confirm the exact glyph/placement against the translator's rendering.
- **Column-header mirror**: whether the record-view column header (which mirrors nav icons per the icon-consistency rule) should also show the dirty marker, or whether the marker is nav-tree-only. yTranslator marks only its sidebar, so nav-tree-only is the consistent default.

Resolved by mirroring yTranslator:
- Dirty key → plugin index (session-scoped, cleared on reload), analogous to the translator's per-document flag.
- Save scope → both per-plugin (context menu) and Save All (File menu), matching `on_save`/`on_save_all`.
- Unsaved prompts → included on unload, close, and before operations, matching the translator.
