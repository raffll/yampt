# Implementation Plan

## Overview

Fix yEditor's leveled-list auto-merge so the DATA calc flags ("Calc from All Levels", and for LEVI "Calc for Each Item") and NNAM chance-none reflect all contributing mods, not just the last-listed plugin. Today `leveled_list_merge_t::merge` unions the list entries correctly but takes the whole header — including DATA and NNAM — verbatim from the winning version, so a mod that enabled a calc flag has it dropped if a later plugin didn't. The fix extracts DATA/NNAM from every version, ORs the flags, applies a defined NNAM policy (default minimum, to be confirmed), and splices the merged values into the rebuilt header while leaving the entry merge and the rest of the header unchanged. The change is confined to `sub_record_merge.cpp`. Write the failing flag-merge test first (test-before-fix).

## Tasks

- [ ] 1. Per-version DATA/NNAM extraction (core)
  - Add file-local `extract_leveled_data_flags(content) -> uint32_t` (0 if DATA absent) and `extract_leveled_chance_none(content) -> optional<uint8_t>` in `sub_record_merge.cpp`, using `sub_record_iter_t`.
  - _Requirements: R4.1, R1.3, R2.3_

- [ ] 2. OR the calc flags and apply the NNAM policy (core)
  - In `leveled_list_merge_t::merge`, compute `merged_flags = OR of extract_leveled_data_flags over all version_contents`, and `nnam = min of present chance-none values` (policy default; confirm against engine/reference before finalizing — isolated to this step).
  - _Requirements: R1.1, R1.2, R2.1, R2.2_

- [ ] 3. Splice merged DATA/NNAM into the rebuilt header (core)
  - Parameterize `extract_list_header` (or wrap it) so the produced header uses `merged_flags` for DATA and the policy value for NNAM, emitting all other header sub-records verbatim from the winning version. Emit a DATA/NNAM sub-record even if the winning version lacked it but contributors provided values.
  - _Requirements: R3.1, R3.3_

- [ ] 4. Changed-gate correctness (core)
  - Ensure `merge` reports `changed=true` when merged DATA/NNAM differ from the winning version even if entries are identical (the existing `record == winning_content` comparison should already yield this; confirm).
  - _Requirements: R5.3_

- [ ] 5. Tests (write before tasks 2–3)
  - `[u]` on `leveled_list_merge_t::merge`: differing DATA → OR; "Calc from All Levels" from one contributor present; LEVI "Calc for Each Item" OR'd; contributor missing DATA contributes 0; NNAM differing → min, equal → that value, none → omitted; all-equal flags → byte-identical to today and no spurious change; entry union/deletion/count/sort unchanged.
  - Register new/updated test file in `yampt.tests.vcxproj` + `.filters`.
  - _Requirements: R6.1, R6.2, R6.3, R7.1_

- [ ] 6. Update documentation
  - CHANGELOG `[FIX]` (yEditor): merged leveled lists now combine calc flags from all contributors instead of the last plugin only.
  - `docs/yEditor-Manual.md`: note calc flags are combined across contributors (and the chance-none policy once confirmed).
  - README + README.bbcode in sync if leveled-list merging is described.
  - _Requirements: R1, R2_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1], "depends_on": [] },
    { "wave": 2, "tasks": [5], "depends_on": [1] },
    { "wave": 3, "tasks": [2, 3], "depends_on": [1] },
    { "wave": 4, "tasks": [4], "depends_on": [2, 3] },
    { "wave": 5, "tasks": [6], "depends_on": [4] }
  ]
}
```

Extraction helpers (1) come first. Per test-before-fix the flag-merge tests (5) are authored before the merge/header changes (2, 3). The changed-gate check (4) follows. Docs (6) last.

## Notes

- Scope is LEVI/LEVC auto-merge only, confined to `sub_record_merge.cpp`; no editor or other record-type changes. Manual copies of leveled lists are unaffected.
- Entry union/deletion/max-count/sort is unchanged — only the header's DATA (and NNAM) are now merged instead of taken verbatim from the winning version.
- DATA is OR'd as a whole u32 by default (only low bits are defined; confirm no reserved-bit hazard, else mask to defined bits).
- The NNAM chance-none policy (default: minimum) is the one judgemental choice; it is isolated to task 2 and must be confirmed against engine semantics / a reference tool before finalizing. Reference tool sources are not present in this workspace.
- When all contributors already agree on DATA/NNAM (the common case), output is byte-for-byte identical to today.
- Building and running tests is done manually by the user (no-build-or-test rule); write the failing flag-merge test before the change (test-before-fix rule).
