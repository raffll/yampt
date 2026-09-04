# Implementation Plan

## Overview

Fix yEditor so records copied into the merged patch by hand survive regeneration. Today every manual "Copy … to Merged Patch" writes a non-pinned store record, and regeneration clears the whole store and rebuilds only the automatic records plus a pinned save-around that nothing ever populates — so manual work is lost. The fix marks manual copies as user-curated (reuse the existing `pinned` flag) so the existing `collect_pinned_records` / `restore_pinned_records` save-around protects them, keeps auto records non-pinned/transient, guards `prune_unchanged` against pinned records, rewords the now-false "discard manual changes" warning, and persists the manual-record keys across sessions so a reloaded patch stays protected. Work order: core guard + optional helper first, then the manual-copy pinning, then the warning reword, then cross-session persistence, then tests and docs. Write the failing test before the fix (test-before-fix rule).

## Tasks

- [ ] 1. Guard prune_unchanged and add a set_pinned helper (core)
  - `auto_merge_t::prune_unchanged`: skip records with `pinned == true` (never prune a user-curated record).
  - `merge_patch_store_t::set_pinned(rec_type, record_id, bool)` (optional helper for the load re-apply step, avoids re-copying content).
  - _Requirements: R3.3, R5 (helper)_

- [ ] 2. Manual copies pin the record (editor)
  - In `merge_controller_t`, change the terminal store write of `copy_whole_record` / `copy_cell_record` / `copy_sub_record` / `copy_group` / `copy_field` from `copy_record_to_merge_raw` to `pin_record_to_merge`. Ensure `ensure_merge_record` seeds a pinned base record. Route `remove_sub_record` / `remove_group` rewrites through `pin_record_to_merge` too (a partially-edited manual record stays user-curated).
  - Leave `auto_merge_t` writes non-pinned (unchanged).
  - _Requirements: R1.1, R1.2, R1.3, R2.1, R2.2_

- [ ] 3. Confirm regeneration preserves manual records
  - Verify `create_merge_records` collect→execute→restore order yields manual-wins (restore overwrites same-key auto). No code change expected beyond tasks 1–2; add the covering test in task 6.
  - _Requirements: R3.1, R3.2_

- [ ] 4. Reword the regenerate confirmation (editor)
  - In `create_merged_patch`, replace the "discard manual changes" message with "Recompute the automatic merge? Records you copied in manually are kept." (tr-wrapped). Keep Yes/No.
  - _Requirements: R4.2_

- [ ] 5. Persist manual-record keys across sessions (editor + core)
  - `plugin_session_t`: add `m_manual_merge_keys` (vector of (rec_type,record_id)) + accessors + save/restore under `merge/manual_records` (delimited, like `merge/excluded_plugins`).
  - Manual copy/remove ops add/remove the key and persist the session (same `save_session_state` call the exclude/guard actions use).
  - After the merged patch is loaded/seeded, re-apply pinning (`set_pinned`) to store records whose keys are in the persisted list; drop keys with no matching record.
  - _Requirements: R5.1, R5.2, R4.1_

- [ ] 6. Tests (write before the fixes they cover)
  - `[u]`: manual copy → record pinned + collected; regenerate simulation (collect → clear → re-add auto → restore) keeps manual with manual content and overrides same-key auto; `prune_unchanged` keeps a pinned record whose content equals a source; remove clears the key so it is not resurrected; session manual-key round-trip + re-apply pins matching / drops unmatched.
  - Register new test files in `yampt.tests.vcxproj` + `.filters`.
  - _Requirements: R7.1, R7.2, R7.3_

- [ ] 7. Update documentation
  - CHANGELOG `[FIX]` (yEditor): regenerating the merged patch no longer discards manually copied records.
  - `docs/yEditor-Manual.md`: manual copies persist across regeneration and sessions; removal is permanent.
  - README + README.bbcode in sync if the merged-patch workflow is described.
  - _Requirements: R1, R3, R4_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1], "depends_on": [] },
    { "wave": 2, "tasks": [2, 4], "depends_on": [1] },
    { "wave": 3, "tasks": [3, 5], "depends_on": [2] },
    { "wave": 4, "tasks": [6, 7], "depends_on": [3, 5] }
  ]
}
```

The core guard/helper (1) comes first. Manual-copy pinning (2) and the warning reword (4) follow. Regeneration confirmation (3) and cross-session persistence (5) build on the pinning. Tests and docs (6, 7) last — though per test-before-fix the individual failing tests are authored just before each covered change.

## Notes

- Reuse the existing `pinned` flag as the user-curated marker — the save-around already keys on it; the bug is simply that nothing pins manual copies. No new provenance field.
- Auto-merge output for a fresh patch (no manual records) is unchanged; auto records stay non-pinned and are recomputed each regenerate.
- During regenerate, pinned manual records are held in `create_merge_records`'s local snapshot while `execute()` clears/rebuilds, so `prune_unchanged` cannot see them; the pinned-skip guard is an explicit invariant for any future prune path.
- The `.esp` format is unchanged — provenance lives in memory + the session INI, not in the file. Cross-session preservation relies on re-applying the pin from the persisted key list after the patch is loaded.
- Building and running tests is done manually by the user (no-build-or-test rule); write the failing test before each fix (test-before-fix rule).
