# Implementation Plan

## Overview

Make yEditor drop merged-patch records that came only from a mod the user removed from the load order. Today the store carries no source-plugin provenance and nothing reconciles it against the current plugin set, so records derived from removed mods persist stale in `Merged Patch.esp`. This adds per-record provenance (contributing source filenames), persisted in the session because the `.esp` cannot carry it; a pure reconciliation function that classifies each merge record as fully-orphaned (auto or manual), partially-orphaned, or unknown-provenance against the loaded plugin set; and a load-time cleanup that removes fully-orphaned auto records, confirms manual orphans with the user, keeps the rest, and reports what it did. Work order: provenance storage + recording first, then the pure reconciliation function (with tests), then session persistence, then the load-time apply + report, then docs. This spec shares session-side provenance storage with `merge-persist-manual-records`.

## Tasks

- [ ] 1. Add provenance storage and recording (core)
  - `plugin_scan_t`: `m_merge_provenance` (map (rec_type,record_id) → set of source filenames) + `set_merge_provenance` / `merge_provenance` / clear-with-`clear_merge_records`.
  - `auto_merge_t`: record the group's version plugin filenames as provenance when writing each merged record.
  - _Requirements: R1.1, R1.3_

- [ ] 2. Record provenance on manual copy (editor)
  - `merge_controller_t::copy_*`: after landing a manual copy, set provenance = the source plugin filename (union for group/field patches pulling from that plugin). Composes with the manual-copy pinning from `merge-persist-manual-records`.
  - _Requirements: R1.1_

- [ ] 3. Pure reconciliation function (core)
  - `merge_orphans.hpp/.cpp`: `orphan_report_t` + `reconcile_merge_orphans(store_records, provenance, loaded_filenames)` classifying each record (fully-orphaned auto/manual, partial, unknown). Filename compare via `string_utils` case-insensitive. Pure, no mutation.
  - Write `[u]` tests first (task 6) — this function is the testable core.
  - _Requirements: R2.1, R2.2, R2.3, R3.4_

- [ ] 4. Persist provenance across sessions (editor + core)
  - `plugin_session_t`: persist/restore the provenance map under `merge/provenance`; mirror provenance on copy/auto-merge; restore into `plugin_scan_t` after the `.esp` seeds the store. Share the storage with `merge-persist-manual-records`'s manual-key list (one provenance structure; pinned tracked alongside).
  - _Requirements: R1.2, R6.4_

- [ ] 5. Apply cleanup on load + report (editor)
  - After load/seed + provenance restore, run `reconcile_merge_orphans`. Remove `fully_orphaned_auto`. Show a confirm dialog listing `fully_orphaned_manual` (rec_type + id + removed mod) and remove only the confirmed. Keep partial/unknown. Log a summary and, when records were removed, an optional list dialog. `rebuild_conflicts`; persist via the normal save flow (no forced destructive write). Master list stays consistent via existing `build_master_list`.
  - _Requirements: R3.1, R3.2, R3.3, R4.1, R4.2, R5.1_

- [ ] 6. Tests (write reconciliation tests before the apply code)
  - `[u]`: fully-orphaned auto removed; record with a loaded contributor kept; pinned/manual orphan flagged not auto-removed; unknown-provenance kept; reordering → empty report; partial orphan kept. Provenance session round-trip.
  - Register new test files in `yampt.tests.vcxproj` + `.filters`.
  - _Requirements: R7.1, R7.2_

- [ ] 7. Update documentation
  - CHANGELOG `[NEW]` (yEditor): merged-patch records from a removed mod are dropped (and reported); records still backed by a loaded mod are kept.
  - `docs/yEditor-Manual.md`: describe load-time reconciliation, auto-orphan removal + report, and manual-orphan confirmation.
  - README + README.bbcode in sync if the merged-patch workflow is described.
  - _Requirements: R3, R4_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1, 3], "depends_on": [] },
    { "wave": 2, "tasks": [2, 4], "depends_on": [1] },
    { "wave": 3, "tasks": [6], "depends_on": [3, 4] },
    { "wave": 4, "tasks": [5], "depends_on": [3, 4] },
    { "wave": 5, "tasks": [7], "depends_on": [5] }
  ]
}
```

Provenance storage/recording (1) and the pure reconciliation function (3) are the independent cores. Manual-copy provenance (2) and session persistence (4) build on storage. Tests (6) cover reconciliation + round-trip before the apply step (5) wires it into the load path. Docs (7) last. Per test-before-fix, the reconciliation `[u]` tests are authored before task 5.

## Notes

- Provenance is by source **filename** (not index) so it is stable across reloads and reordering; reordering is never treated as removal.
- The `.esp` format is unchanged — provenance lives in memory + the session INI, restored after the patch is seeded on load.
- This spec shares session-side provenance/manual storage with `merge-persist-manual-records`; implement the provenance map once and let both features use it (manual pinning + orphan detection). Sequence the two so the shared session storage is built once.
- Fully-orphaned auto records are removed automatically; manual (user-curated) orphans are surfaced for confirmation, never silently deleted; partial and unknown-provenance records are kept and reported.
- Reconciliation is a pure function returning a report; the caller performs removals — keeps the core testable in-memory.
- Building and running tests is done manually by the user (no-build-or-test rule); author the reconciliation tests before the apply code (test-before-fix rule).
