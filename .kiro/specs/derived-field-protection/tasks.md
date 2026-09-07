# Tasks — Derived Field Protection

- [ ] 1. Create the central derived-field definition in yampt.core
  - Add `yampt.core/source/utility/derived_fields.hpp/.cpp` with the fixed table and
    `is_derived_sub_record`, `is_derived_field`, `all_entries`.
  - Add the file to `yampt.core.vcxproj` and its `.filters`.
  - _Requirements: 1_

- [ ] 2. Unit-test the derived-field definition (test-first)
  - Add `yampt.tests/source/tests.derived_fields.cpp`: positive cases for each entry
    (SCPT:SCDT, SCPT:SCHD fields 1-5, LEVI:INDX, LEVC:INDX) and negative cases.
  - Register the test file in `yampt.tests.vcxproj` and `.filters`.
  - _Requirements: 1, 8_

- [ ] 3. Implement the recompute pass in yampt.core (test-first)
  - Add `yampt.tests/source/tests.derived_recompute.cpp` FIRST with failing cases:
    SCPT SCHD Script Data Size / Local Var Size correction, LEVI/LEVC INDX correction,
    non-derived passthrough, idempotency (recompute twice == once).
  - Add `yampt.core/source/scanner/derived_recompute.hpp/.cpp` implementing
    `derived_recompute_t::recompute`; reuse `sub_record_merge_t::parse_sub_records` /
    `reconstruct_record` and `domain_types` size helpers.
  - Leave SCHD Num Shorts/Longs/Floats unchanged (documented limitation).
  - Add both files to the respective vcxproj/filters.
  - _Requirements: 6, 7, 8_

- [ ] 4. Invoke the recompute pass in the merged-patch build
  - In `merge_controller_t::create_merge_records`, after `reapply_locks()`, iterate merged
    records and write back `derived_recompute_t::recompute(rec_type, content)`.
  - _Requirements: 6.3, 7.1_

- [ ] 5. Exclude system-derived sub-records from the auto-merge
  - In `create_merge_records`, union `SCPT:SCDT`, `LEVI:INDX`, `LEVC:INDX` into
    `merge_config_t.ignored_sub_records` alongside the user rules.
  - _Requirements: 3_

- [ ] 6. Grey and disable editing of derived fields in the record view
  - In `view_tree_decode.cpp` / `view_tree_decode_cell.cpp`, set `is_ignored` for rows/fields
    matching `derived_fields`, in addition to the user-ignore check.
  - Ensure the record view does not set `Qt::ItemIsEditable` for a derived field/sub-record.
  - _Requirements: 2.1, 2.3_

- [ ] 7. Reject derived-field edits defensively
  - In `field_edit_controller_t::commit_field_edit`, early-return with a status-bar reason if
    the target field is derived.
  - _Requirements: 2.2_

- [ ] 8. Refuse locks on a derived target
  - In `view_context_menu_t::build_lock_for` / `build_lock_menu`, treat a derived
    sub-record/field target as unlockable (greyed), keeping whole-record locks allowed.
  - _Requirements: 4_

- [ ] 9. Show derived entries greyed and non-toggleable in the exclusion UIs
  - In `build_sub_record_ignore_menu`, add the Include/Exclude action `setEnabled(false)` for
    a derived target and do not wire the toggle.
  - In `merge_settings_view.cpp`, list system-derived entries as disabled, non-removable
    items, never written to `SubRecordRules/IgnoreConflict`.
  - _Requirements: 5_

- [ ] 10. Update documentation
  - `docs/yEditor-Manual.md`: derived fields show greyed, cannot be edited or locked, and are
    kept correct automatically in the merged patch.
  - `CHANGELOG.md`: `[NEW]`/`[CHANGE]` entries under yEditor per the changelog-categories rule.
  - _Requirements: 8.2_

- [ ] 11. Final verification
  - Confirm unit tests pass; confirm no regression in record body/sub-record Size handling
    (Requirement 7.2). (Build/tests run by the user per the no-build-or-test rule.)
  - _Requirements: 7, 8_
