# yEditor Auto-Save Investigation — FNAM Edit Not Persisted

## Summary Answer

There are **two bugs**:

1. **Primary bug**: When the user makes a field edit (e.g. FNAM) via the Edit panel and clicks Apply, the save writes to the **output directory** (`resolve_active_output_path`), not the original plugin's disk path. If the output dir differs from the plugin's source location (the common case in MO2 mode where the default output is `../../overwrite`), the file is written to the wrong place and the user never sees the change in their source plugin. This is not "auto-save not working" — the save succeeds, but to an unexpected location.

2. **Secondary bug**: Field edits go through `save_active_plugin()` which bypasses the dirty-tracking system (`mark_plugin_dirty` / `clear_plugin_dirty`). This means `has_any_unsaved()` always returns false after a field edit. The window title never gets the `*` unsaved indicator, the History tab is never populated, and the close-event "Save unsaved changes?" prompt never fires for field-edited content. If `save_active_plugin` fails (empty path, write error), the loss is silent — the user gets an `[error]` log line only.

---

## Call Chain: Apply → Disk

```
preview_view_t::on_apply_clicked()
  ↓
  m_edit_controller->commit_field_edit(m_pending_request)   [field_edit_controller.cpp:87]
    ↓
    m_session.scan().find_active_content(rec_type, record_id)   [plugin_scan.cpp:678]
    │  Returns null if record not in active store → returns {false,"record content not found"}
    ↓ (success)
    field_encoder::encode_field(...)
    field_encoder::patch_sub_record(...)
    commit_to_merge(request, patched_content)
      ↓
      m_session.scan().copy_record_to_active_raw(rec_type, record_id, patched_content)
        ↓  [plugin_scan.cpp:610]  — writes to m_active_store only, no dirty flag
      m_session.scan().recompute_single_conflict(...)
      emit record_modified()   ← Qt signal
  ↓ (success)
  emit edit_committed()   ← Qt signal

[signal: record_modified → plugin_workspace_view.cpp:171]
  refresh_all_views()
  m_merge_controller->save_active_plugin()   ← THE SAVE
    ↓  [merge_controller.cpp:908]
    resolve_active_output_path()
      ↓  [merge_controller.cpp:870]
      resolve_output_directory()
        → returns empty if load_base_path is empty  ← SILENT FAILURE POINT A
        → returns base_path + settings.output_dir_*  (may differ from original plugin path)
      → output_dir + plugin_filename (active plugin's name)
    ↓
    save_active_to_file(output_path, ...)   [merge_controller.cpp:963]
      ↓
      scan.active_record_count() iterations → builder.add_record_raw(...)
      patch_builder_t::save(output_path, ...)   [patch_builder.cpp]
        → writes to output_path + ".tmp" then renames

[signal: edit_committed → plugin_workspace_view.cpp:179]
  refresh_all_views()   (duplicate refresh, harmless)
```

---

## Root Cause Analysis

### Bug 1 — Wrong Save Target

`save_active_plugin()` resolves the output path as:

```cpp
// merge_controller.cpp:870-881
std::string resolve_active_output_path() const
{
    const auto output_dir = resolve_output_directory();   // base_path + settings output subdir
    const int active_idx = m_session.scan().active_plugin_index();
    const std::string filename = active_idx >= 0
        ? m_session.scan().plugin_filename(active_idx)
        : std::string(merged_patch::filename);
    return QDir(output_dir).filePath(filename).toStdString();
}
```

This is correct for the **Merged Patch** workflow: users create a new merged patch and it lives in the output dir. But when a **loaded existing plugin** is set as active (via right-click → "Set as Active Plugin"), field edits also write to the output directory — not back to the plugin's original location (`m_session.scan().plugin_path(active_idx)`).

For MO2 mode, the default output dir is `../../overwrite`. For folder mode with no relative path set, the output dir equals the base folder — so the save DOES overwrite in place. This means the bug manifests in MO2/OpenMW mode but not in folder mode with no output-subdir setting.

### Bug 2 — Dirty Tracking Not Updated

`save_active_plugin()` never calls:
- `m_session.mark_plugin_dirty(active_idx)` — before saving
- `m_session.clear_plugin_dirty(active_idx)` — after saving
- the `unsaved_changes_changed` callback — to update window title

Consequences:
- The window title `*` indicator never fires for field edits.
- `has_any_unsaved()` remains false → close-event prompt never fires → changes made after last manual save could be lost if `save_active_plugin` silently failed.
- `edit_log_t::record_field_edit` is never called from the field edit path, so the History tab is always empty after edits. (`m_edit_history` in `plugin_workspace_view_t` is populated by context-menu merge operations but not by direct field edits via the Edit panel.)

### Bug 3 — No FNAM edit path to `edit_log`

The field edit workflow (Apply button path through `field_edit_controller_t`) never calls `m_edit_history.record_field_edit(...)`. The History tab in yEditor only shows merge-copy operations from `view_context_menu.cpp`, not edits made via the Edit panel. This means there is no undo/revert path for field edits.

### Silent Failure on Empty Path

If `load_base_path` is empty (e.g. session not properly loaded), `resolve_output_directory()` returns empty, and `save_active_plugin()` logs an `[error]` to the Log tab and returns silently. The user would need to be looking at the Log tab to see this. The Apply button reports success regardless.

---

## Evidence — Key File/Symbol Citations

| Symbol | File | Line | Note |
|---|---|---|---|
| `field_edit_controller_t::commit_to_merge` | `field_edit_controller.cpp` | 121 | emits `record_modified`; no dirty flag |
| `plugin_scan_t::copy_record_to_active_raw` | `plugin_scan.cpp` | 610 | writes only to `m_active_store` |
| `plugin_workspace_view_t::setup_connections` | `plugin_workspace_view.cpp` | 171 | `record_modified` → `save_active_plugin()` |
| `merge_controller_t::save_active_plugin` | `merge_controller.cpp` | 908 | resolves output path, calls `save_active_to_file` |
| `merge_controller_t::resolve_active_output_path` | `merge_controller.cpp` | 870 | returns `output_dir + active_filename` |
| `merge_controller_t::resolve_output_directory` | `merge_controller.cpp` | 893 | returns empty if `load_base_path` is empty |
| `editable_column_set_t::is_editable` | `editable_column_set.cpp` | 8 | only active column is editable |
| `merge_controller_t::save_plugin` | `merge_controller.cpp` | 934 | manual save → saves to original path, clears dirty |
| `edit_log_t::record_field_edit` | `edit_log.cpp` | 20 | never called from field edit path |

---

## Recommended Fixes

### Fix A — Save loaded plugins back to their original path

When the active plugin is a **loaded plugin** (not the merged patch, not a "Create New Plugin" virtual entry), `save_active_plugin()` should save to `m_session.scan().plugin_path(active_idx)` instead of the output directory. The output-dir path should only be used for the merged patch and newly-created virtual plugins.

Distinguish via: `active_idx < static_cast<int>(m_session.scan().plugin_count()) - 1` (loaded) vs. a virtual new one (at `plugin_count()` index with an empty path). Or check whether `plugin_path(active_idx)` is non-empty and the filename matches a loaded plugin.

Concretely: in `resolve_active_output_path()`, if `active_idx >= 0` and the plugin was loaded from a real path (not a virtual new plugin), return `m_session.scan().plugin_path(active_idx)` directly instead of `output_dir + filename`.

### Fix B — Integrate dirty tracking with field edits

After a successful field edit, the active plugin should be marked dirty (`m_session.mark_plugin_dirty(active_idx)`) in the `record_modified` handler. After `save_active_plugin` succeeds, call `m_session.clear_plugin_dirty(active_idx)` and emit `unsaved_changes_changed`. This makes the window `*` indicator and close-event prompt behave consistently.

### Fix C — Populate edit history from field edits

In the `record_modified` connection lambda (or in `commit_to_merge`), call `m_edit_history.record_field_edit(...)` with the record type, record ID, field name, and new value so the History tab reflects edits made via the Edit panel.

---

## What Was NOT Broken

- The `is_editable` check correctly prevents editing columns other than the active plugin's column.
- The `find_active_content` guard in `commit_field_edit` correctly prevents edits when the record is not in the active store.
- `patch_builder_t::save` correctly writes the entire active store to disk (the file content is correct).
- The signal chain from Apply → `record_modified` → `save_active_plugin` fires correctly every time.
