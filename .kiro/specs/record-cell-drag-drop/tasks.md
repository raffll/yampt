# Implementation Plan

## Overview

Complete yEditor's record-view drag & drop so a decoded field value can be dragged from one plugin column and dropped onto another plugin column (applied as a field edit) or onto the merged-patch column (copied into the merge), with both recorded in History. The model already declares the drag/drop flags and MIME formats but `mimeData()` returns null and the view has drag/drop disabled. The work reuses the existing apply paths (`field_edit_controller_t::commit_field_edit` → `commit_to_source`/`commit_to_merge`) — no new commit logic — plus a small pure payload/compat/request module for testability, and closes the History gap for merge applies. Work order: pure payload module first, then model `mimeData`/`canDropMimeData`, then view drop handling + controller routing, then the History fix for merge, then tests and docs.

## Tasks

- [ ] 1. Add the pure drag-payload module
  - `record_field_drag_t { record_type, record_id, sub_type, occurrence, object_ref_index, schema_field_index, source_plugin_idx, value }` and pure `encode`/`decode` (delimited, following the nav_tree_model payload precedent).
  - `is_compatible(payload, target_identity)` predicate (same sub_type + schema field index).
  - `make_request_from_drop(target_identity, payload)` → `field_edit_request_t` (plugin_idx from target; -1 for merge; input_text = payload.value).
  - Add files to `yampt.editor.vcxproj` + `.filters`.
  - _Requirements: R2.1, R2.2, R3.1, R3.2, R4.1_

- [ ] 2. Implement view_tree_model mimeData and drop acceptance
  - `mimeData(indexes)`: for a concrete decoded field cell (has `field_def_role`, not label/Signature/Record Flags/non-existent), build `record_field_drag_t` from the roles + `full_value_at`, encode to `"application/x-yampt-subrecord"`; otherwise return nullptr.
  - Extend `canDropMimeData` to accept the subrecord format on the merge column (always) and on an editable non-merge plugin column, rejecting incompatible/self/non-field targets.
  - _Requirements: R2.1, R2.3, R6.2, R6.3, R6.4_

- [ ] 3. Enable drag/drop and handle drops in the record view
  - `record_view_t::setup_tree`: `setDragEnabled(true)`, `setAcceptDrops(true)`, drop indicator.
  - Handle `dropEvent` (or eventFilter, per nav_tree_view precedent): decode payload, resolve the drop target cell → target identity (plugin idx / record index / field identity / merge flag), check `editable_column_set_t::is_editable` and `is_compatible`, then emit a drop signal (or call a coordinator) with payload + target. Keep apply logic out of the view.
  - _Requirements: R1.1, R1.2, R6.1_

- [ ] 4. Route drops to the apply paths
  - Wire the record-view drop to a controller/coordinator (thin from the view): plugin target → `make_request_from_drop` → `field_edit_controller_t::commit_field_edit` (→ `commit_to_source`); merge target → same with `plugin_idx == -1` (→ `commit_to_merge`).
  - Surface a validation failure (invalid value for target type) the same way an Edit-tab apply failure is surfaced.
  - _Requirements: R3.1, R3.3, R4.1, R4.2, R4.3_

- [ ] 5. Record merge applies/drops in History
  - Make `field_edit_controller_t::commit_to_merge` emit `field_edited` (target = merge filename) so the existing `field_edited → edit_log_t::record_field_edit → history_view_t` wiring logs merge drops uniformly (also fixes the pre-existing gap for Edit-tab merge applies). Plugin drops already log via `commit_to_source`.
  - _Requirements: R5.1, R5.2, R5.3_

- [ ] 6. Tests
  - `[u]`: `encode`/`decode` round-trip; `is_compatible` accepts matching field, rejects mismatched; `make_request_from_drop` builds the expected request for a plugin target and for a merge target (`plugin_idx == -1`).
  - Register new test files in `yampt.tests.vcxproj` + `.filters`.
  - _Requirements: R8.1_

- [ ] 7. Update documentation
  - CHANGELOG `[NEW]` (yEditor): drag a decoded value between plugin columns to apply it, or onto the merged-patch column to copy it into the merge; both recorded in History (merge field applies now appear in History too).
  - `docs/yEditor-Manual.md`: describe the drag gesture, the two drop targets, the Enable-Editing gate for plugin targets (merge always accepts), and the History recording.
  - README + README.bbcode in sync if record-view editing is described.
  - _Requirements: R3, R4, R5_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1], "depends_on": [] },
    { "wave": 2, "tasks": [2, 5], "depends_on": [1] },
    { "wave": 3, "tasks": [3], "depends_on": [2] },
    { "wave": 4, "tasks": [4], "depends_on": [3, 5] },
    { "wave": 5, "tasks": [6, 7], "depends_on": [4] }
  ]
}
```

The pure payload module (1) unblocks the model's `mimeData`/`canDropMimeData` (2) and is used by request building later. The History fix (5) is independent of the view and can land early. View drop handling (3) needs the model side; routing (4) needs both the view drop signal and the History fix. Tests/docs (6, 7) last.

## Notes

- No new commit logic: plugin drops reuse `commit_field_edit`/`commit_to_source`, merge drops reuse `commit_field_edit` with `plugin_idx == -1`/`commit_to_merge` — the same paths the Edit tab and context menu already use.
- The model already declares drag/drop flags and MIME formats; only `mimeData` (was nullptr), `canDropMimeData` acceptance, and the view-level enable/handling are added.
- `field_def_t` is a model pointer, so the payload ships field *identifiers* (sub_type, occurrence, object_ref_index, schema field index) and the target re-resolves the field via the model — mirroring how `preview_view_t` derives the request from roles.
- Enable Editing gates plugin-column drops; the merge column is always a valid drop target (per `editable_column_set_t::is_editable`).
- Making `commit_to_merge` emit `field_edited` also fixes the existing gap where Edit-tab merge applies were not recorded in History.
- Payload encode/decode, compatibility, and request-building are pure functions so they unit-test without a live `QTreeView`.
- Building and running tests is done manually by the user (no-build-or-test rule).
