# Design — Drag & Drop a Decoded Cell Value Between Plugin Columns (yEditor)

## Context (current mechanics)

- **View** — `record_view_t` wraps a `QTreeView` on `view_tree_model_t`. `setup_tree()` sets `setDragEnabled(false)`/`setAcceptDrops(false)`. Selecting a cell fires `selection_changed` → `plugin_workspace_view_t::on_view_selection_changed` → `preview_view_t::update_selection`.
- **Model mapping** — col 0 = label; cols 1..N per plugin version. `column_plugin_indices()[col-1]` = plugin idx; `record_index_for_column(col)`; `is_merge_column(section)`/`merge_column()`; `record_type()`/`record_id()`; `data(index, field_def_role)` = `const field_def_t *`; `data(index, sub_record_occurrence_role)` = `sub_record_occurrence_t{sub_type, occurrence, object_ref_index}`; `full_value_at(index)` = raw cell string.
- **Drag/drop scaffolding (partial)** — `flags()`: `ItemIsDragEnabled` on non-merge plugin cols, `ItemIsDropEnabled` on merge col. `supportedDragActions`/`supportedDropActions` = `CopyAction`. `mimeData()` → `nullptr`. `canDropMimeData()` accepts `"application/x-yampt-record"` / `"application/x-yampt-subrecord"`.
- **Apply path** — `field_edit_controller_t::commit_field_edit(field_edit_request_t)`: resolves subrecord by (sub_type, occurrence, object_ref_index), validates+encodes, then `commit_to_source` (plugin) or `commit_to_merge` (plugin_idx == -1). `field_edit_request_t` fields: record_type, record_id, sub_type, occurrence, object_ref_index, bit_index, plugin_idx, record_index, field, input_text, codepage.
- **Blueprint** — `preview_view_t::update_selection(index, model, cell_value)` builds `field_edit_request_t` from a cell (field_def_role, sub_record_occurrence_role, plugin_idx = merge ? -1 : column_plugin_indices()[col-1], record_index = record_index_for_column(col), codepage, field). `on_apply_clicked` sets `input_text` and calls `commit_field_edit`.
- **Enable Editing** — toolbar `QToolButton` → `plugin_workspace_view_t::set_editing_enabled` → `editable_column_set_t`. `is_editable(col)`: false if col<1; true if col == merge column; else `m_editing_enabled`.
- **Merge copy** — `merge_controller_t::copy_field`/`copy_sub_record` (+ others) → `merge_patch_ops_t::patch_*` → `copy_record_to_merge_raw` → `refresh_after_merge` → `save_merged_patch`.
- **History** — `edit_log_t` append-only; `field_edited` (from `commit_to_source`) → `record_field_edit` → `history_view_t`. `commit_to_merge` and `merge_controller_t::copy_*` do NOT log today.
- **Precedent** — `nav_tree_model_t::mimeData` → `"application/x-yampt-record"`, payload `"<plugin_idx>\t<rec_type>\t<record_id>"`; `nav_tree_view_t::eventFilter` handles DragEnter/Move/Drop.

## Design Goals

Complete the record-view drag/drop (R1, R2) so a decoded field value drags from a plugin column and drops onto another plugin column (field edit, R3) or the merge column (copy into merge, R4), both recorded in History (R5), gated correctly (R6), reusing existing apply paths with no new commit logic (R7) and extracting pure encode/compat/request logic for `[u]` tests (R8). Honor architecture rules: orchestration on a controller not the view/window (Anti-Gravity), one class per file, `_t`/snake_case, tr() for any user text, no-deep-nesting.

## Decision: complete the model's mimeData with a field payload

Implement `view_tree_model_t::mimeData(indexes)` to emit `"application/x-yampt-subrecord"` carrying the source field's identity + value. Payload struct (encoded to a delimited byte string, decoded back — a pure pair of functions in a small `record_drag_payload` namespace/struct so it is testable):

```cpp
struct record_field_drag_t
{
    std::string record_type;
    std::string record_id;
    std::string sub_type;
    int occurrence = 0;
    int object_ref_index = -1;
    int schema_field_index = -1;   // to re-resolve the field at the target
    int source_plugin_idx = -1;
    std::string value;             // the decoded source value string
};
```

`mimeData` builds this from the dragged index (guarding: must have `field_def_role`, not label/Signature/Record Flags/non-existent — mirror `preview_view_t::update_selection`), encodes it, and sets it on the `QMimeData`. If the index is not a concrete field cell, return `nullptr` (no drag) (R2.3).

Rationale: `field_def_t` is a pointer into the model, so the payload carries the *identifiers* (`sub_type`, `occurrence`, `object_ref_index`, and the schema field index) needed to re-resolve the target field via the model at the drop point, plus the value string. This mirrors how `preview_view_t` re-derives the field from roles rather than shipping the pointer (R2.1).

## Decision: drop handling in the view, orchestration in a controller

`record_view_t` (the QTreeView owner) handles the Qt drop events (`dragEnterEvent`/`dragMoveEvent`/`dropEvent`, or an `eventFilter` like `nav_tree_view_t`), since that is where Qt delivers them. But it does not contain the apply logic — it decodes the payload, resolves the drop target cell → target identity, and calls a coordinator/controller method. This keeps the view thin and orchestration on a controller (Anti-Gravity, consistent-across-apps with how `preview_view_t` delegates to `field_edit_controller_t` / `merge_controller_t`).

Drop resolution at the target index:
- target column, `is_merge = model->is_merge_column(col)`, editable = `m_editable_columns->is_editable(col)`.
- target field identity from the target index's roles (`field_def_role`, `sub_record_occurrence_role`) + `record_type`/`record_id` + `record_index_for_column(col)` + plugin idx.

### canDropMimeData / acceptance (R6.4)

Extend `canDropMimeData` to accept `"application/x-yampt-subrecord"` on: the merge column (always), or a non-merge plugin column when editable. Reject when the target is not editable, not a field cell, or the target field is incompatible with the payload (R6.1, R6.2). Self-drop (same plugin col + same field identity) → reject (R6.3).

## Decision: plugin drop reuses commit_field_edit (R3)

On drop onto an editable non-merge plugin column, build a `field_edit_request_t` for the **target** plugin/record and the **target** field identity, with `input_text = payload.value`, and call `field_edit_controller_t::commit_field_edit`. This is exactly `preview_view_t`'s request-building, factored into a pure helper `make_request_from_drop(target, payload)` so it is unit-testable (R8.1). `commit_to_source` then does dirty-mark + conflict recompute + `field_edited` (which logs History automatically, R5.1) + `record_modified`. No new commit logic (R3.3, R7.1).

Compatibility rule (R3.2): the target field must have the same `sub_type` and the same schema field (`schema_field_index`) as the payload — i.e. you can drop a NAME onto a NAME field, not onto an unrelated field. The compatibility check is a pure predicate `is_compatible(payload, target)` (R8.1). Mismatched → rejected in `canDropMimeData`/`dropEvent`.

## Decision: merge drop reuses commit_to_merge and adds History (R4, R5.2)

On drop onto the merge column, build the same `field_edit_request_t` but with `plugin_idx == -1` and call `commit_field_edit` → `commit_to_merge` (single decoded field granularity, matching a single-cell drop; identical effect to the Edit-tab apply targeting the merge column). This is cleaner than routing through `merge_controller_t::copy_field` because the payload is a single field value and `commit_to_merge` already exists and recomputes conflicts.

History gap: `commit_to_merge` does not emit `field_edited` today. To satisfy "both recorded in History" (R5.2), the merge drop path records a `field_edit_record_t` in `edit_log_t`. Cleanest option (design pick): have `field_edit_controller_t::commit_to_merge` emit `field_edited` too (with the merge as the target "plugin"), so the existing `field_edited → record_field_edit` wiring logs it uniformly — this also fixes the pre-existing gap for Edit-tab merge applies. The `field_edit_record_t.plugin_filename` becomes the merge filename (e.g. "Merged Patch.esp") so the description reads naturally. (If emitting from `commit_to_merge` risks changing other behavior, the fallback is to log at the drop coordinator after a successful merge commit; the design prefers the uniform signal.)

Post-merge refresh is whatever `commit_to_merge` already triggers (`recompute_single_conflict`, `record_modified(true, {})` → workspace refresh + `save_merged_patch`), unchanged (R4.3).

## Component Changes

| Area | Change |
|------|--------|
| `view_tree_model.cpp` | implement `mimeData()` (encode `record_field_drag_t`); extend `canDropMimeData` for `"application/x-yampt-subrecord"` + editable/compat checks |
| `record_drag_payload.hpp/.cpp` (new, yampt.editor) | `record_field_drag_t` + pure `encode`/`decode`; `is_compatible`; `make_request_from_drop` |
| `record_view.cpp` | `setDragEnabled(true)`/`setAcceptDrops(true)`; drop event handling → decode payload, resolve target, call coordinator/controller |
| drop coordinator / existing controller | route: plugin target → `commit_field_edit`; merge target → `commit_field_edit` (plugin_idx == -1) |
| `field_edit_controller.cpp` | `commit_to_merge` also emits `field_edited` (uniform History for merge applies/drops) |
| `plugin_workspace_view.cpp` | wire the record-view drop signal to the controller (thin), pass `editable_column_set_t` for gating |

Files added to `yampt.editor.vcxproj` + `.filters`.

## Data Flow

Drag start (editable field cell) → `mimeData` encodes `record_field_drag_t`. Drop:
- onto editable plugin col, compatible field → `make_request_from_drop` → `commit_field_edit` → `commit_to_source` → target dirty, conflicts recompute, `field_edited` logs History.
- onto merge col → `make_request_from_drop` with `plugin_idx = -1` → `commit_field_edit` → `commit_to_merge` → merge updated, conflicts recompute, `field_edited` (merge) logs History, `save_merged_patch`.
- otherwise → rejected in `canDropMimeData`/`dropEvent`, no change.

## Error Handling

- Non-field / label / Signature / non-existent source cell → `mimeData` returns nullptr, no drag.
- Non-editable plugin target, incompatible field, or self-drop → drop rejected, nothing changes.
- `commit_field_edit` validation failure (e.g. dragged value invalid for the target type/limit) → returns `{false, message}`; the drop reports it (tooltip/status) and makes no change, exactly as an Edit-tab apply failure.

## Testing Strategy (R8)

`[u]` (pure, no `QTreeView`/disk):
- `record_drag_payload::encode`/`decode` round-trip a `record_field_drag_t`.
- `is_compatible(payload, target)` — true for same sub_type + schema field index; false for mismatched sub_type or field.
- `make_request_from_drop(target, payload)` — produces a `field_edit_request_t` with the target's plugin_idx/record_index/field identity and `input_text == payload.value`; and with `plugin_idx == -1` for a merge target.

Manual (needs the UI): the drag/drop gestures, dirty marker, History entries, and Enable-Editing gating per R8.2. Building/running tests is manual (no-build-or-test rule). Test names: `owner::member, description`, e.g. `"record_drag_payload::decode, round-trips encoded field"`, `"record_drag_payload::is_compatible, rejects mismatched field"`.

## Files Touched

| File | Change |
|------|--------|
| `yampt.editor/source/model/view_tree_model.cpp` | `mimeData`, `canDropMimeData` extension |
| `yampt.editor/source/view/record_drag_payload.hpp/.cpp` (new) | payload encode/decode, compat, request builder |
| `yampt.editor/source/view/record_view.cpp` | enable drag/drop, drop event handling |
| `yampt.editor/source/controller/field_edit_controller.cpp` | `commit_to_merge` emits `field_edited` |
| `yampt.editor/source/view/plugin_workspace_view.cpp` | wire drop → controller (thin), pass editable set |
| `yampt.editor/yampt.editor.vcxproj` + `.filters` | register new files |
| `yampt.tests/*` + vcxproj/.filters | `[u]` payload/compat/request tests |

## Documentation

- CHANGELOG `[NEW]` (yEditor): with Enable Editing on, a decoded value can be dragged from one plugin column onto another to apply it as a field edit, or onto the merged-patch column to copy it into the merge; both are recorded in History. (Also note merge field applies now appear in History.)
- `docs/yEditor-Manual.md`: describe the drag gesture, the two drop targets and their effects, that it requires Enable Editing for plugin targets (the merge column always accepts), and that both actions appear in History.
- README + README.bbcode in sync if the record view / merge editing is described. No internal detail (MIME formats, controllers) in user docs.
