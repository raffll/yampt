# Design — Deferred Plugin Save in yEditor

Decouple "apply a field edit" (in-memory + mark dirty) from "write the plugin file" (explicit Save from the plugin context menu), with an asterisk marker on dirty plugins in the nav tree.

## Mirroring yTranslator

yTranslator's dirty/save model is the template (Consistent-Across-Apps rule). Mapping:

| yTranslator | yEditor equivalent |
|---|---|
| `document_t::is_dirty()` / `set_dirty()` | `plugin_session_t::is_plugin_dirty(idx)` / `mark`/`clear` |
| `session_t::has_any_unsaved()` / `save_all()` | `plugin_session_t::has_any_unsaved()` / `save_all_dirty(controller)` |
| `main_window_t::set_unsaved_changes` → title `"yTranslator *"` | `editor_window_t::set_unsaved_changes` → title `"yEditor *"` |
| `derive_display_name(..., is_dirty)` sidebar marker | nav-tree marker in `display_text_for_file` |
| `derive_context_menu` shows Save when loaded && dirty | context-menu Save enabled when dirty |
| File-menu `on_save` / `on_save_all` | File-menu Save / Save All |
| unload / `closeEvent` / operations prompts | same prompts in yEditor |

## Dirty-state ownership: `plugin_session_t`

`plugin_session_t` already owns per-plugin classification sets (`excluded_plugins`, `patch_plugins`) with getter/setter accessors and threads them into the nav filter. Add a dirty set following the same pattern, plus session-level helpers mirroring `session_t`:

```cpp
// plugin_session.hpp
void mark_plugin_dirty(int plugin_idx);
void clear_plugin_dirty(int plugin_idx);
bool is_plugin_dirty(int plugin_idx) const;
const std::set<int> & dirty_plugins() const;
bool has_any_unsaved() const;              // mirrors session_t::has_any_unsaved

private:
    std::set<int> m_dirty_plugins;
```

- Keyed by **plugin index** (resolved by mirroring the translator's per-item flag): the edit/apply path, nav model, and context menu all already work in terms of `plugin_idx`, and the set is cleared on reload/unload so stale indices cannot leak into a new load.
- `m_dirty_plugins` is cleared wherever the session clears the current load (the same place `scan` is reset / plugins unloaded). Not persisted to INI (R2.4).
- `save_all_dirty` lives on the controller (needs the write path + logging), iterating `dirty_plugins()` and calling `save_plugin(idx)`, mirroring `session_t::save_all()`.

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

The `record_modified` signal already flows to the workspace view; the handler must additionally trigger a nav refresh so the asterisk appears (see UI wiring).

## Save path: new controller method

The whole-plugin write is exactly what Apply did before, so move it into a controller method (Anti-Gravity Rule — R5.1). Place it where plugin/merge orchestration already lives; `merge_controller_t` already writes files (`save_merge_to_file`) and holds the session, so a sibling method fits, or a small dedicated method on the field-edit controller. Design choice: add `save_plugin(int plugin_idx)` to the controller that already owns `m_session` and logging.

```cpp
bool controller::save_plugin(int plugin_idx)
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

## Nav-tree marker: `nav_tree_model_t::display_text_for_file`

The function builds `"[NNN] filename"` into `display_buffer` then prepends one of the priority emoji icons. Add the dirty marker as a suffix on the filename portion so it never competes with the leading icon (R3.2):

- Compute `const bool dirty = filter dirty-set contains plugin_idx;` using the same threading the function already uses for `excluded_plugins()`/`patch_plugins()` (those come through `m_filter`). Thread the dirty set into the nav filter the same way (R3.3), or expose it on the same filter object the model already reads.
- When dirty, append `" *"` after the filename (before the final `QString` assembly), so every icon branch keeps its leading glyph and gains a trailing `*`.

Marker glyph (Open Decision resolved): trailing `" *"` — unambiguous, does not collide with the emoji icon set, familiar "unsaved" convention.

Threading: `nav_tree_model_t` reads `m_filter.excluded_plugins()` / `m_filter.patch_plugins()`. Add `m_filter.dirty_plugins()` fed from `plugin_session_t::dirty_plugins()` at the point the filter is built/updated (`apply_effective_filter` / `build_effective_filter` in `plugin_workspace_view`). No new global access.

## Context menu: `view_context_menu_t::build_source_file_menu`

Add a Save action to the existing source-file menu (which currently has Exclude/Include and Guard-Patch toggles):

```cpp
auto * save_action = menu.addAction(QCoreApplication::translate("yEditor", "Save"),
    [this, info]() {
        if (m_<controller>.save_plugin(info.plugin_idx))
            m_nav_view.rebuild_preserving_state();
    });
save_action->setToolTip(QCoreApplication::translate("yEditor", "Write in-memory changes to the plugin file"));
save_action->setEnabled(m_session.is_plugin_dirty(info.plugin_idx)); // greyed when clean (R4.2)
```

`view_context_menu_t` already holds `m_session`, `m_merge`, and `m_nav_view`; it needs a reference to whichever controller exposes `save_plugin` (wire it in the constructor where the other controller refs are injected).

## UI wiring: `plugin_workspace_view_t`

- The `record_modified` handler (already connected) refreshes the record view after Apply; add a `m_nav_view.rebuild_preserving_state()` (or a lighter row refresh) so the asterisk shows immediately (R3.4). Confirm `record_modified(false, path)` is the source-plugin case.
- `build_effective_filter` includes the session's `dirty_plugins()` in the filter state passed to the nav model.
- Unload/new-load path clears `m_dirty_plugins` (R2.3) — done in the session where scan is reset.

## Window title asterisk: `editor_window_t::set_unsaved_changes`

Mirror `main_window_t::set_unsaved_changes` exactly: a `bool m_has_unsaved_changes` guard, and `setWindowTitle(dirty ? "yEditor *" : "yEditor")`. The workspace view calls it after Apply (dirty=true) and after Save / Save All (`dirty = m_session->has_any_unsaved()`), just as the translator does through its `set_unsaved_changes` callback.

## File-menu Save / Save All: `editor_window_t`

Mirror the translator's File menu. Add `Save` and `Save All` actions (tooltips like "Save the active plugin" / "Save all modified plugins"), wired to `editor_window_t::on_save` / `on_save_all` which delegate to controller methods:
- `on_save`: save the currently selected plugin if dirty (selection comes from the nav view), then update title.
- `on_save_all`: `save_all_dirty`, refresh nav, update title.
Each commits any pending field edit first (the yEditor analog of the translator's `commit_current_edit()` before save — here, ensure the preview's in-progress edit is applied or discarded consistently).

## Unsaved prompts: mirror the translator

- **Unload** a dirty plugin → Save/Discard/Cancel, mirroring `sidebar_controller`'s per-document prompt. Hook where yEditor unloads/removes a plugin.
- **Close** → `editor_window_t::closeEvent` checks `m_session->has_any_unsaved()` and prompts Save/Discard/Cancel, honoring Cancel by ignoring the event — a direct copy of `main_window_t::closeEvent`.
- **Before operations** (create/load merged patch, clean) → if `has_any_unsaved()`, prompt to save first, mirroring `plugin_operations_controller`. Hook in `merge_controller_t` entry points.
All strings via `QCoreApplication::translate("yEditor", ...)`.

## Apply button text (`preview_view_t`)

Change the Apply button tooltip from "Commit field edit to disk" to an in-memory phrasing, e.g. "Apply field edit to the loaded plugin". Update yEditor manual/README to match (sync-docs-with-code). The button label stays "Apply".

## Files

Modified (yampt.editor):
- `session/plugin_session.hpp/.cpp` — dirty set + accessors, `has_any_unsaved`; clear on reload.
- `controller/field_edit_controller.cpp` — `commit_to_source` marks dirty, drops the write.
- `controller/merge_controller.hpp/.cpp` (or field_edit controller) — `save_plugin(int)`, `save_all_dirty`; unsaved prompt before operations.
- `model/nav_tree_model.cpp` — dirty marker in `display_text_for_file`.
- `model/` nav filter state struct — carry dirty set (mirror excluded/patch).
- `controller/view_context_menu.hpp/.cpp` — Save action (gated on dirty) + controller ref.
- `view/plugin_workspace_view.cpp` — refresh nav + update title on `record_modified`; include dirty set in filter build; clear on unload; unload prompt.
- `editor_window.hpp/.cpp` — `set_unsaved_changes` + title asterisk; File-menu Save / Save All (`on_save`/`on_save_all`); `closeEvent` unsaved prompt.
- `view/preview_view.cpp` — Apply tooltip text.

Docs: `docs/yEditor-Manual.md`, `README.md`, `docs/README.bbcode`, `CHANGELOG.md` (2.0beta, `[CHANGE]` — Apply no longer writes immediately; `[NEW]` — plugin Save) per changelog/doc rules.

## Testing

- `[u]` unit tests for the dirty-set accessors on a session-like holder if the logic is extractable without file I/O (mark/clear/query, cleared on reset).
- File-write behavior reuses the existing `binary_file_io::write_file` path; validated at integration/manual level (writing touches disk, so not in the `[u]` suite per unit-test rules).
- Manual verification: Apply updates memory + marks asterisk, file unchanged; Save writes file + clears asterisk; Save greyed when clean; write failure keeps dirty and logs error.
