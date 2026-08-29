# Design — Deferred Plugin Save in yEditor

Decouple "apply a field edit" (in-memory + mark dirty) from "write the plugin file" (explicit Save from the plugin context menu), with an asterisk marker on dirty plugins in the nav tree.

## Mirroring yTranslator

yTranslator's dirty/save model is the template (Consistent-Across-Apps rule). Mapping:

| yTranslator | yEditor equivalent |
|---|---|
| `document_t::is_dirty()` / `set_dirty()` | `plugin_session_t::is_plugin_dirty(idx)` / `mark`/`clear` |
| `session_t::has_any_unsaved()` / `save_all()` | `plugin_session_t::has_any_unsaved()` / `save_all_dirty(controller)` |
| `main_window_t::set_unsaved_changes` → title `"yTranslator *"` | `editor_window_t::set_unsaved_changes` → title `"yEditor *"` |
| `display_name_t::set_dirty` → `to_string()` leading `"* "` | nav-tree marker in `file_node_display_text` |
| sidebar context menu shows Save when loaded && dirty | context-menu Save enabled when dirty |
| File-menu `on_save` / `on_save_all` | File-menu Save / Save All |
| unload / `closeEvent` / operations prompts | same prompts in yEditor |

## Dirty-state ownership: `plugin_session_t`

`plugin_session_t` already owns per-plugin classification sets (`excluded_plugins`, `patch_plugins`) with getter/setter accessors and threads them into the nav filter. Add a dirty set following the same pattern, plus session-level helpers mirroring `session_t`:

```cpp
// plugin_session.hpp
void mark_plugin_dirty(int plugin_idx);      // resolves idx -> filename internally
void clear_plugin_dirty(int plugin_idx);     // resolves idx -> filename internally
bool is_plugin_dirty(int plugin_idx) const;  // resolves idx -> filename internally
const std::set<std::string> & dirty_plugins() const;
bool has_any_unsaved() const;                // mirrors session_t::has_any_unsaved

private:
    std::set<std::string> m_dirty_plugins;   // filenames, like m_excluded_plugins / m_patch_plugins
```

- Keyed by **plugin filename**, identical to `m_excluded_plugins` / `m_patch_plugins`. This is the corrected decision (the earlier "plugin index" plan is rejected): `load_plugins_internal` does `m_scan = plugin_scan_t()` and reassigns indices on every load, so an index-keyed dirty set would point at the wrong plugin after a reload. Filenames are stable, survive nav rebuilds, and are consulted with the same `count(filename)` pattern the nav model already uses for the other sets.
- The public API still takes `plugin_idx` (callers — apply path, context menu — already work in those terms). Each method translates `plugin_idx → m_scan.plugin_filename(plugin_idx)` at the boundary and stores/queries the filename. `dirty_plugins()` returns the filename set so the nav filter can point at it exactly like `excluded_plugins()`.
- Filename-collision caveat: keying by filename assumes filenames are unique within a load, the same assumption the existing `excluded_plugins`/`patch_plugins` sets already make. If two loaded plugins ever shared a filename, marking one dirty would mark both and saving one would clear the other's marker. This matches the established behavior of the sibling sets and is accepted; it is not introduced by this feature. If it ever needs fixing, fix it for all three sets together (e.g. key by normalized full path).
- `m_dirty_plugins` is cleared wherever the session clears the current load: alongside the `m_scan = plugin_scan_t()` reset in BOTH `unload_all()` and `load_plugins_internal()`. Not persisted to INI (R2.4).
- `save_all_dirty` lives on `merge_controller_t` (it needs the write path + logging), iterating a **copy** of `dirty_plugins()` (because `save_plugin` calls `clear_plugin_dirty`, which mutates the set — iterating the live set would invalidate the iterator) and calling `save_plugin(idx)`, mirroring `session_t::save_all()`.

## Apply path: `field_edit_controller_t::commit_to_source`

Change so it mutates memory and marks dirty, without writing the file:

```cpp
auto & plugin = m_session.scan().mutable_plugin(request.plugin_idx);
plugin.select_record(request.record_index);
plugin.replace_record(patched_content);          // unchanged in-memory mutation

m_session.mark_plugin_dirty(request.plugin_idx);  // NEW
m_session.scan().recompute_single_conflict(request.record_type, request.record_id); // unchanged
emit record_modified(false, plugin_path);         // keep signal; path still identifies the plugin
```

Removed: the immediate `binary_file_io::write_file(...)` call and its failure branch (that logic moves to Save). `commit_to_merge` is untouched (R1.3).

The `record_modified` signal already flows to the workspace view. The source-plugin branch of that handler currently logs `"[info] saved " + saved_path` — that line MUST be removed, because Apply no longer saves; leaving it would log a save that never happened. The source branch instead marks dirty, refreshes the nav so the asterisk appears, and sets the title asterisk (see UI wiring). The merge branch (`is_merge_edit == true`, still calling `save_merged_patch()`) is unchanged.

## Save path: new controller method

The whole-plugin write is exactly what Apply did before, so move it into a controller method (Anti-Gravity Rule — R5.1). Placement is committed to **`merge_controller_t`**: it already holds `m_session`, the logger (`m_log`), and `m_nav_view`, and already writes files via `save_merge_to_file`. This avoids adding a new controller dependency to the context menu (which already holds a `merge_controller_t & m_merge`). Add `save_plugin(int plugin_idx)` and `save_all_dirty()` there.

```cpp
bool merge_controller_t::save_plugin(int plugin_idx)
{
    auto & plugin = m_session.scan().mutable_plugin(plugin_idx);
    const auto & path = m_session.scan().plugin_path(plugin_idx);
    const bool written = binary_file_io::write_file(plugin.get_records(), path);
    if (!written)
    {
        m_log("[error] failed to save " + path);   // stays dirty (R4.4)
        return false;
    }
    m_session.clear_plugin_dirty(plugin_idx);
    m_log("[info] saved " + path);
    return true;
}
```

The caller (context menu) refreshes the nav tree after a successful save so the marker clears (R3.4, R4.3).

Note on error surfacing: today a failed write in `commit_to_source` returns `edit_result_t{false, "failed to write plugin file"}`, which the preview shows inline at the moment of Apply. After deferral, the in-memory `replace_record` cannot fail that way, and the only write failure moves to Save — reported via `m_log` in the Log tab, not inline in the preview. This is a deliberate, minor UX shift (write errors now appear in the log, not on the edit panel), consistent with how the merged-patch save already reports failures.

## Nav-tree marker: `nav_tree_model_t::file_node_display_text`

(The function is `file_node_display_text(const file_node_t &)` — there is no `display_text_for_file`.) It builds `"[NNN] filename"` into `display_buffer` then, via first-match-wins early returns, prepends one of the priority emoji icons and returns `icon + display_buffer` in each branch. There is no single final-assembly point, so bake the marker into `display_buffer` (prefix `"* "` to the buffer before any branch runs). Each branch then returns `icon + " " + "* " + [NNN] filename`, so the layout is `<icon> * [NNN] filename`:

- Compute `const bool dirty = m_filter.dirty_plugins() && m_filter.dirty_plugins()->count(filename);` — the same `count(filename)` lookup already used for `excluded_plugins()`/`patch_plugins()` (all reached through the model's embedded `m_filter`). This is why the dirty set is filename-keyed (R2.2): it must be consultable by filename here.
- When dirty, prepend `"* "` to `display_buffer` once, before the icon-priority branches.

Marker glyph (Open Decision resolved): leading `"* "`, matching yTranslator's `display_name_t::to_string()` exactly (it prepends `"* "` when dirty). On yEditor's icon-prefixed rows the `"* "` sits after the icon glyph and before `[NNN]`. It is text, not an icon, so the *Plugin Icons Must Be Consistent Across Panels* rule does not require mirroring it in `view_tree_model.cpp::headerData` (R3.5). Nav-tree-only, matching yTranslator marking only its sidebar.

Threading: `nav_tree_filter_t` carries `const std::set<std::string> * m_excluded_plugins` / `m_patch_plugins` as pointers, set via `set_excluded_plugins(const std::set<std::string>*)`. Add a parallel `set_dirty_plugins(const std::set<std::string>*)` + `dirty_plugins()` accessor. Wire it once in the workspace constructor beside the existing lines, pointing straight at the session's set:

```cpp
m_nav_view->set_dirty_plugins(&m_session->dirty_plugins());
```

The pointer stays valid across `set`-style whole-set replacements because it refers to the same `m_dirty_plugins` member (assignment mutates in place), exactly as the excluded/patch pointers already do. No new global access, and no change to `build_effective_filter` (which composes only the record filter, not the plugin sets).

## Context menu: `view_context_menu_t::build_source_file_menu`

Add a Save action to the existing source-file menu (which currently has Exclude/Include and Guard-Patch toggles):

```cpp
auto * save_action = menu.addAction(QCoreApplication::translate("yEditor", "Save"),
    [this, info]() {
        if (m_merge.save_plugin(info.plugin_idx))
            m_nav_view.rebuild_preserving_state();
    });
save_action->setToolTip(QCoreApplication::translate("yEditor", "Write in-memory changes to the plugin file"));
save_action->setEnabled(m_session.is_plugin_dirty(info.plugin_idx)); // greyed when clean (R4.2)
```

`view_context_menu_t` already holds `plugin_session_t & m_session`, `merge_controller_t & m_merge`, and `nav_tree_view_t & m_nav_view` — `save_plugin` lives on `m_merge`, so no new controller reference or constructor wiring is needed. The plugin index comes from `info.plugin_idx` (a `nav_tree_model_t::node_info_t`); `build_source_file_menu` is only reached for source-file root nodes.

## UI wiring: `plugin_workspace_view_t`

The current handler is:

```cpp
connect(m_edit_controller, &field_edit_controller_t::record_modified, this,
    [this](bool is_merge_edit, const std::string & saved_path) {
        rebuild_nav_preserving_state();
        if (is_merge_edit)
            m_merge_controller->save_merged_patch();
        else
            log_message("[info] saved " + saved_path);   // <-- REMOVE (Apply no longer saves)
    });
```

- Source case (`is_merge_edit == false`): drop the `"[info] saved"` log; the record is dirty, not saved. `rebuild_nav_preserving_state()` already runs, so the asterisk shows immediately (R3.4). Add `set_unsaved_changes(true)` on the window (R2b.2). `record_modified(false, path)` is the source-plugin case (confirmed).
- Merge case: unchanged — still `save_merged_patch()`.
- Note: a second signal, `preview_view_t::edit_committed`, is ALSO connected and re-displays the record via `display_record_in_view` after an Apply. It does not need dirty/title changes (the `record_modified` handler already marks dirty). Just be aware both fire per Apply; the dirty mark + title update belong only in the `record_modified` handler to avoid double work.
- The dirty set reaches the nav model through the filter pointer wired in the constructor (`set_dirty_plugins(&m_session->dirty_plugins())`), NOT through `build_effective_filter` (which composes only the record filter). No change to `build_effective_filter`.
- Unload/new-load path clears `m_dirty_plugins` (R2.3) — done in the session where scan is reset (`unload_all()` and `load_plugins_internal()`).

## Window title asterisk: `editor_window_t::set_unsaved_changes`

Mirror `main_window_t::set_unsaved_changes` exactly: a `bool m_has_unsaved_changes` guard, and `setWindowTitle(dirty ? "yEditor *" : "yEditor")`. The workspace view calls it after Apply (dirty=true) and after Save / Save All (`dirty = m_session->has_any_unsaved()`), just as the translator does through its `set_unsaved_changes` callback.

## File-menu Save / Save All: `editor_window_t`

Mirror the translator's File menu. Add `Save` and `Save All` actions (tooltips like "Save the active plugin" / "Save all modified plugins"), wired to `editor_window_t::on_save` / `on_save_all` which delegate to `merge_controller_t` methods:
- `on_save`: save the currently selected plugin if dirty (selection comes from the nav view), then update title.
- `on_save_all`: `save_all_dirty`, refresh nav, update title.

**Pending-edit decision (corrects the vague "commit pending edit first"):** yEditor's Apply is an explicit button, not an auto-commit-on-selection model — there is no continuously-tracked "pending edit" to flush the way yTranslator's `commit_current_edit()` flushes an active editor. In-progress text typed in the preview field but not yet Applied is NOT committed by Save / Save All; it stays in the field, and only records already Applied (i.e. already in the dirty set) are written. This is deliberate: implicitly applying half-typed field text to a plugin the user did not Apply would risk writing an unintended edit. Save/Save All therefore write exactly the plugins in the dirty set and touch nothing that was never Applied.

## Unsaved prompts: mirror the translator

- **Unload All** → a SINGLE Save/Discard/Cancel prompt when `has_any_unsaved()` (yEditor has no per-plugin unload, so one prompt covers everything). Hook in `plugin_workspace_view_t::on_unload_all()`, BEFORE it calls `m_session->unload_all()` (which resets `m_scan` and discards edits). Save → `save_all_dirty()` then unload; Discard → unload; Cancel → return without unloading.
- **Load a new set** → Open Folder / Open MO2 Profile / Open OpenMW Config / restore-session all funnel through `load_plugins_internal`, which resets `m_scan` and discards in-memory edits. If `has_any_unsaved()`, prompt Save/Discard/Cancel BEFORE the reset (R6.4). Hook at the load entry points in the workspace view (or a single guard invoked by all of them), not inside `load_plugins_internal` itself (which has no UI access). Without this, loading a folder while dirty silently loses edits.
- **Close** → `editor_window_t::closeEvent` checks `m_session->has_any_unsaved()` and prompts Save/Discard/Cancel, honoring Cancel by ignoring the event — a direct copy of `main_window_t::closeEvent` (which today only does `save_config(); event->accept();`).
- **Before create / regenerate merge** → `merge_controller_t::create_merged_patch`. If `has_any_unsaved()`, prompt **Save / Cancel** (no Discard — see Merge semantics below). Save → `save_all_dirty()` then continue; Cancel → return without merging. This prompt fires FIRST, before the existing "regenerate discards manual merge changes" confirmation.
- **Before clean** → `plugin_workspace_view_t::on_clean_all()` (drives `batch_cleaner_t` directly, NOT `merge_controller_t`). Save / Discard / Cancel.
- **Load existing merged patch** → NO prompt. `load_existing_merged_patch` reads/rewrites no source records, so unsaved source edits are not at risk.
All strings via `QCoreApplication::translate("yEditor", ...)`.

## Merge semantics: in-memory edits are the source of truth

`plugin_scan_t::read_record_content` returns the in-memory `esm` record — the same one `commit_to_source::replace_record` mutated. `auto_merge_t` (during create) and every `copy_*` operation read through this path. So a deferred, unsaved source edit is ALREADY what the merge consumes.

This is intended: the merge uses the on-screen state, not the on-disk file. The only risk is that the merge could be written from edits that are not yet on disk, so disk and merge disagree. The create/regenerate Save-or-Cancel prompt (above) closes that: the user either saves all dirty plugins first (disk == merge), or cancels (nothing changes). No new merge code is needed — only the prompt and the `create_merged_patch` return-value change below.

`create_merged_patch` is currently `void`; the workspace's `on_create_merged_patch` runs post-create steps (re-display the selected record, update status) unconditionally after calling it. Change `create_merged_patch` to return `bool` (false when the user cancels at the Save/Cancel or regenerate prompt) so the workspace skips those post-steps on cancel. This is the fix for the Cancel-can't-propagate problem.

## Apply button text (`preview_view_t`)

Change the Apply button tooltip from "Commit field edit to disk" to an in-memory phrasing, e.g. "Apply field edit to the loaded plugin". Update yEditor manual/README to match (sync-docs-with-code). The button label stays "Apply".

## Files

Modified (yampt.editor):
- `session/plugin_session.hpp/.cpp` — filename-keyed dirty set + accessors (idx→filename at the boundary), `dirty_plugins()`, `has_any_unsaved`; clear beside the `m_scan` reset in `unload_all()` and `load_plugins_internal()`.
- `controller/field_edit_controller.cpp` — `commit_to_source` marks dirty, drops the `write_file` call + failure branch.
- `controller/merge_controller.hpp/.cpp` — `save_plugin(int)`, `save_all_dirty()` (iterate a copy of the dirty set); `create_merged_patch` returns `bool` and shows the Save/Cancel unsaved prompt before merging; load-existing merge gets no prompt.
- `model/nav_tree_model.cpp` — dirty marker baked into `display_buffer` in `file_node_display_text` (consulted by filename).
- `model/nav_tree_filter.hpp` — `set_dirty_plugins(const std::set<std::string>*)` + `dirty_plugins()` pointer accessor (mirror excluded/patch pointers).
- `controller/view_context_menu.cpp` — Save action (gated on dirty) calling `m_merge.save_plugin`; no new ref needed.
- `view/plugin_workspace_view.cpp` — drop `"[info] saved"` in the source `record_modified` branch, refresh nav + `set_unsaved_changes(true)`; wire `set_dirty_plugins(&m_session->dirty_plugins())` in the constructor; unload prompt in `on_unload_all`; load-guard prompt at all load entry points; clean prompt in `on_clean_all`; `on_create_merged_patch` skips its post-create view refresh when `create_merged_patch` returns false (cancel).
- `editor_window.hpp/.cpp` — `set_unsaved_changes` + title asterisk; File-menu Save / Save All (`on_save`/`on_save_all` delegating to `merge_controller_t`); `closeEvent` unsaved prompt.
- `view/preview_view.cpp` — Apply tooltip text.

Docs: `docs/yEditor-Manual.md`, `README.md`, `docs/README.bbcode`, `CHANGELOG.md` (2.0beta, `[CHANGE]` — Apply no longer writes immediately; `[NEW]` — plugin Save) per changelog/doc rules.

## Testing

- `[u]` unit tests for the dirty-set accessors on a session-like holder if the logic is extractable without file I/O (mark/clear/query, cleared on reset).
- File-write behavior reuses the existing `binary_file_io::write_file` path; validated at integration/manual level (writing touches disk, so not in the `[u]` suite per unit-test rules).
- Manual verification: Apply updates memory + marks asterisk, file unchanged; Save writes file + clears asterisk; Save greyed when clean; write failure keeps dirty and logs error.
