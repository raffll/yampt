# Requirements — Drag & Drop a Decoded Cell Value Between Plugin Columns (yEditor)

## Background — Current Behavior

yEditor's record view shows one record's decoded sub-records/fields as rows, with one column per plugin version that contains that record, plus (when present) the merged-patch column.

- `record_view_t` (yampt.editor/source/view/record_view.cpp) wraps a `QTreeView` driven by `view_tree_model_t`. Column 0 is the field/label column; columns 1..N are per-plugin-version. `setup_tree()` currently sets `m_tree->setDragEnabled(false); m_tree->setAcceptDrops(false);`.
- `view_tree_model_t` maps a cell to (record, subrecord, field, plugin column):
  - `column_plugin_indices()[column-1]` → plugin index; `record_index_for_column(column)` → the record index in that plugin; `is_merge_column(section)` / `merge_column()` identify the merged-patch column.
  - `data(index, field_def_role)` → `const field_def_t *` (which field); `data(index, sub_record_occurrence_role)` → `sub_record_occurrence_t { sub_type, occurrence, object_ref_index }` (which subrecord); `record_type()` / `record_id()` (which record).
  - **Drag/drop scaffolding is already partly wired**: `flags()` gives `Qt::ItemIsDragEnabled` to non-merge plugin columns and `Qt::ItemIsDropEnabled` to the merge column; `supportedDragActions()`/`supportedDropActions()` return `Qt::CopyAction`; `canDropMimeData()` accepts formats `"application/x-yampt-record"` and `"application/x-yampt-subrecord"`; but `mimeData()` returns `nullptr` (not implemented).
- Applying a decoded field value: `field_edit_controller_t::commit_field_edit(const field_edit_request_t &)` locates the subrecord by (`sub_type`, `occurrence`, `object_ref_index`), validates and encodes, then dispatches:
  - `plugin_idx != -1` → `commit_to_source`: `mutable_plugin(idx).select_record(index)` → `replace_record(patched)` → `mark_plugin_dirty(idx)` → `recompute_single_conflict` → `emit field_edited({...})` + `emit record_modified(false, path)`.
  - `plugin_idx == -1` → `commit_to_merge`: `copy_record_to_merge_raw`/`pin_record_to_merge` → `recompute_single_conflict` → `emit record_modified(true, {})`.
  - `field_edit_request_t { record_type, record_id, sub_type, occurrence, object_ref_index, bit_index, plugin_idx, record_index, field_def_t field, input_text, codepage }`.
- `preview_view_t::update_selection(index, model, cell_value)` is the existing blueprint that builds a `field_edit_request_t` from a selected cell (reads `field_def_role`, `sub_record_occurrence_role`, `plugin_idx`, `record_index`, codepage) and `on_apply_clicked()` calls `commit_field_edit`.
- "Enable Editing": a toolbar `QToolButton` in `editor_window.cpp` → `plugin_workspace_view_t::set_editing_enabled(bool)` → `editable_column_set_t`. `editable_column_set_t::is_editable(column)`: false for column < 1; true for the merge column always; otherwise `m_editing_enabled`. The ✍ icon marks editing-enabled plugins (nav side).
- Copy-into-merge: `merge_controller_t::copy_sub_record` / `copy_field` / `copy_group` / `copy_whole_record` / `copy_cell_record`, each ending with `refresh_after_merge` + `save_merged_patch`.
- History: `edit_log_t` (append-only, no revert) with `edit_log_entry_t { timestamp, plugin_filename, description }`, `field_edit_record_t`, `record_removal_record_t`, and `record_field_edit` / `record_record_removal`. `field_edit_controller_t::field_edited` is logged via `record_field_edit` and shown by `history_view_t`. **Merge copies (`merge_controller_t::copy_*`) and merge field-edits are NOT recorded in History today** — a gap this feature must close for merge drops.
- Precedent for drag & drop: `nav_tree_model_t::mimeData()` builds `"application/x-yampt-record"` with payload `"<plugin_idx>\t<rec_type>\t<record_id>"`; `nav_tree_view_t::eventFilter()` handles `DragEnter`/`DragMove`/`Drop`.

## Problem

To copy a decoded value from one plugin's version of a record into another plugin (or into the merged patch), the user must select the source cell, open the Edit tab, retype or reselect the value, and apply — or use the context menu to copy into the merge. There is no direct way to grab a cell and drop it onto the target column. Dragging is the natural gesture for "make this column's value match that one", and the model is already half-wired for it.

## Goal

When "Enable Editing" is on, allow dragging a decoded cell value from one plugin column and dropping it:
- onto another plugin column → apply the dragged value as a field edit to that plugin's version of the record (same effect as an Edit-tab apply);
- onto the merged-patch column → copy the value into the merge (same effect as "Copy field/sub-record to Merged Patch").
Both operations are recorded in the History tab.

## User-Facing Outcomes

- With "Enable Editing" on, a decoded field cell in a plugin column can be dragged. The drag carries that field's value and its identity (record, subrecord occurrence, field).
- Dropping onto a different plugin column applies the dragged value to the same field of that plugin's record version: the target cell updates, the plugin is marked dirty (asterisk), conflicts recompute — identical to committing that value via the Edit tab.
- Dropping onto the merged-patch column copies the value into the merge for that field/sub-record — identical to the existing "Copy to Merged Patch" for that field.
- Both a plugin-to-plugin drop and a plugin-to-merge drop add a History entry describing what was applied/copied.
- With "Enable Editing" off, plugin columns are not editable, so a drop onto a (non-merge) plugin column does nothing; the merge column remains a valid drop target (it is always editable), consistent with `is_editable`.
- Dropping onto the same cell, an incompatible target (different field/subrecord identity), or a non-editable target is rejected without changing anything.

## Requirements

### R1 — Enable drag & drop on the record view

1.1 `record_view_t::setup_tree` enables dragging and dropping (`setDragEnabled(true)`, `setAcceptDrops(true)`, appropriate drop indicator), reusing the model's existing `flags()` (drag on non-merge plugin columns, drop on the merge column) and extending drop to plugin columns as needed (see R4).
1.2 The drag is only meaningful for editable source cells that carry a decoded field value; the model's `flags()` already restricts `ItemIsDragEnabled` to non-merge plugin columns — the design confirms whether editing-enabled state gates the drag start or only the drop apply.

### R2 — Implement the drag payload (mimeData)

2.1 `view_tree_model_t::mimeData(indexes)` produces a `QMimeData` (currently returns `nullptr`) carrying enough to reconstruct a field edit at the drop target: the source field value plus the field identity — record type, record id, sub_type, occurrence, object_ref_index, the field descriptor (or enough to re-resolve `field_def_role`), the source plugin/column, and the source value string.
2.2 It uses a yampt-specific MIME format (the scaffolding already references `"application/x-yampt-subrecord"`); the payload encoding follows the `nav_tree_model_t::mimeData` precedent (a delimited string, or a structured encoding the drop handler parses).
2.3 A drag is only started for a cell that maps to a concrete decoded field (has `field_def_role`), not for label/"Signature"/"Record Flags"/non-existent cells — mirroring the guards in `preview_view_t::update_selection`.

### R3 — Drop onto a plugin column → field edit

3.1 Dropping a dragged value onto a cell in a (non-merge) plugin column, when that column is editable (`editable_column_set_t::is_editable`), builds a `field_edit_request_t` for the **target** column's plugin/record and the **target** field identity, with `input_text` = the dragged value, and calls `field_edit_controller_t::commit_field_edit`.
3.2 The target field identity must match the dragged field's kind so the value is meaningful (e.g. dropping a NAME value onto a NAME field). The design specifies the compatibility rule (same `sub_type`/field, or same decoded field semantics); incompatible drops are rejected.
3.3 The apply reuses `commit_to_source` unchanged (dirty-mark, conflict recompute, `field_edited`/`record_modified` signals). No new commit logic is introduced — the drop constructs the request and calls the existing controller (the blueprint being `preview_view_t`).

### R4 — Drop onto the merged-patch column → copy into merge

4.1 Dropping onto the merged-patch column copies the dragged value into the merge for that field/sub-record. It reuses the existing merge-copy path (`field_edit_controller_t::commit_to_merge` via a `field_edit_request_t` with `plugin_idx == -1`, OR `merge_controller_t::copy_field`/`copy_sub_record` — the design picks the one that matches the granularity of a single decoded field drop and keeps behavior identical to the context-menu copy).
4.2 The merge column is always a valid drop target when it exists (it is always editable per `is_editable`), regardless of the "Enable Editing" toggle.
4.3 The merge drop triggers the same post-copy refresh/save the existing copy path does (`recompute_single_conflict` / `refresh_after_merge` / `save_merged_patch` as appropriate to the chosen path).

### R5 — History recording

5.1 A plugin-to-plugin drop is recorded in History. This already happens via `field_edited` → `edit_log_t::record_field_edit` when `commit_to_source` runs, so reusing that path records it automatically.
5.2 A plugin-to-merge drop is recorded in History. Since merge copies/`commit_to_merge` do NOT emit `field_edited` today (the gap noted in Background), this feature adds a History entry for the merge drop — either by having the merge-drop path emit/record a `field_edit_record_t`, or by extending the merge-copy path to log. The design specifies which, so "both are recorded" (the TODO requirement) holds.
5.3 History entries describe the operation meaningfully (target plugin/merge, record, field, value), consistent with existing `field_edit_record_t` formatting.

### R6 — Gating and rejection

6.1 Drops onto non-editable plugin columns (Enable Editing off) are rejected (no change), matching `is_editable`.
6.2 Drops that do not resolve to a compatible target field are rejected without side effects.
6.3 Dropping a cell onto itself (same plugin column, same field) is a no-op.
6.4 `canDropMimeData` accepts the drop only for valid target cells/columns and the yampt MIME format.

### R7 — No regression

7.1 The context-menu "Copy to Merged Patch" operations, the Edit-tab apply, record removal, and all existing record-view behavior are unchanged. Drag & drop is an additional input path to the same controllers.
7.2 With drag & drop enabled at the view level, normal selection/click behavior in the tree is preserved (dragging is initiated only from an editable field cell; a plain click still selects).
7.3 Plugin icon consistency (nav tree vs. header) and other view behaviors are untouched.

### R8 — Verification

8.1 Pure `[u]` tests (no UI/disk): the payload encode/decode round-trips (build a payload from a source field identity+value, parse it back to the same fields); the target-compatibility predicate accepts a matching field and rejects a mismatched one; the request-building logic (payload + target column → `field_edit_request_t`) produces the expected request. These are extracted as pure functions so they test without a live `QTreeView`.
8.2 Manual verification: with Enable Editing on, drag a decoded value from plugin A's column onto plugin B's matching field → B updates, B is marked dirty, a History entry appears; drag onto the merged-patch column → the merge gets the value and a History entry appears; with Enable Editing off, a drop onto a plugin column does nothing while the merge column still accepts.

## Open Decisions

Resolved:
- Reuse existing apply paths: `commit_field_edit`/`commit_to_source` for plugin drops; the existing merge-copy path for merge drops. No new commit logic. (R3, R4)
- Merge drops must be added to History (they are not logged today). (R5.2)
- Follow the `nav_tree_model_t`/`nav_tree_view_t` drag precedent and complete the already-scaffolded `view_tree_model_t` drag/drop. (R1, R2)
- Enable-Editing gates plugin-column drops; the merge column is always a valid drop target. (R4.2, R6.1)

Deferred to design:
- Whether the source drag is gated by Enable-Editing at drag start, or always draggable with the gate applied at drop (R1.2).
- Exact MIME payload encoding and how the field descriptor is carried/re-resolved at the target (`field_def_t` is a pointer into the model) (R2.1).
- The field-compatibility rule for a valid drop (same sub_type + field, or broader decoded-field-type match) (R3.2).
- For merge drops, whether to route through `commit_to_merge` (single field, `plugin_idx == -1`) or `merge_controller_t::copy_field`/`copy_sub_record`, and correspondingly where the History entry is added (R4.1, R5.2).
- Whether the drop handler lives on `record_view_t` (event handling) delegating to a controller, or a small drop-coordinator, keeping orchestration off the view per the app's controller pattern.
