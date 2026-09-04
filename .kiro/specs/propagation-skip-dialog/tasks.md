# Implementation Plan

## Overview

Stop propagation from silently dropping same-text entries that cannot hold a translation. Today `dict_document_t::propagate` skips any target whose record type fails byte-limit (or character) validation for the new text and returns only a count. This feature captures each skip (type, id, reason, encoded length, limit) during the existing single propagate loop, carries the list out through `commit` to `main_window_t::commit_current_edit`, and shows an informational dialog listing the entries that were not updated. The skip decision itself is unchanged. Work order: result structs first, then the propagate capture and commit fold, then the dialog, then the surface point, then tests and docs.

## Tasks

- [ ] 1. Add the result structs
  - In `document.hpp`: `propagation_skip_t { rec_type_t type; std::string key_text; std::string reason; size_t byte_count; size_t limit; }` and `propagate_result_t { int count; std::vector<propagation_skip_t> skipped; }`.
  - Add `std::vector<propagation_skip_t> skipped;` to `commit_result_t`.
  - _Requirements: R2.1_

- [ ] 2. Capture skips in propagate and fold into commit
  - Change `dict_document_t::propagate` to return `propagate_result_t`. Reuse the single `validator.validate(type, new_text)` call: on `error`, push a `propagation_skip_t` (from `reason`/`byte_count`/`limit`) and `continue`; otherwise write/count as today.
  - In `commit`, set `result.propagated_count = propagation.count` and `result.skipped = std::move(propagation.skipped)`.
  - Keep the loop flat (early continue + blank line); split into a `propagate_to_record` helper if it exceeds the size/nesting limits.
  - _Requirements: R1.1, R1.2, R1.3, R2.2, R6.2_

- [ ] 3. Add the skipped-entries dialog
  - `propagation_skip_dialog_t` in `yampt.translator/source/dialog/`, modeled on `dict_selection_dialog_t`: label + table/list (Type / Entry / Reason), OK button. If a `QTableWidget`, set 24px row height. Format each row from `propagation_skip_t` (type via `domain_types::type_to_str`, `key_text`, reason + numbers). All strings tr()-wrapped.
  - Add files to `yampt.translator.vcxproj` + `.filters`.
  - _Requirements: R3.1, R3.2, R3.3, R3.4_

- [ ] 4. Surface the dialog after commit
  - In `main_window_t::commit_current_edit`, after the existing propagation status message/sync, if `commit_output.result.skipped` is non-empty construct and `exec()` the dialog (thin routing). Keep the "Propagated to N entries" message.
  - _Requirements: R4.1, R4.2, R4.3_

- [ ] 5. Unit-test the propagate skip reporting
  - `[u]`: in-memory `dict_document_t` with two same-`old_text` entries (CELL 63, FNAM 31); commit a translation that is valid for CELL but over 31 bytes for FNAM. Assert the FNAM is reported skipped (reason/byte_count/limit correct) and unchanged, the valid target propagated, and `propagated_count` counts only valid writes. Add a no-skip case asserting an empty `skipped` list.
  - Register any new test file in `yampt.tests.vcxproj` + `.filters`.
  - _Requirements: R7.1, R7.2_

- [ ] 6. Update documentation
  - CHANGELOG `[CHANGE]` (yTranslator): propagation now reports entries it could not update because the translation is invalid/too long for their record type, instead of skipping them silently.
  - `docs/yTranslator-Manual.md`: note the reporting dialog in the propagation section.
  - README + README.bbcode in sync if propagation is described.
  - _Requirements: R1, R4_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1], "depends_on": [] },
    { "wave": 2, "tasks": [2, 3], "depends_on": [1] },
    { "wave": 3, "tasks": [4], "depends_on": [2, 3] },
    { "wave": 4, "tasks": [5, 6], "depends_on": [4] }
  ]
}
```

The result structs (1) unblock both the propagate/commit change (2) and the dialog (3, which consumes `propagation_skip_t`). Surfacing (4) needs both. Tests (5) need the propagate change; docs (6) last.

## Notes

- No second pass and no second `propagate` call: skips are captured in the same loop that already validates each candidate (cleanest-approach rule).
- The skip criterion (`validate(...).level == error`) is unchanged — only reporting is added. Caution-level (INFO 512–1024) still propagates and is not reported.
- The dialog owns its per-row formatting; `main_window_t` only decides whether to show it (Anti-Gravity rule).
- `commit_result_t` gains one field; existing consumers of the other fields are unaffected.
- Building and running tests is done manually by the user (no-build-or-test rule).
