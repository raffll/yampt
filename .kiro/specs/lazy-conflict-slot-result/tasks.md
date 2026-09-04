# Tasks — Lazy Conflict Slot Result in yEditor

Order: measure on current code first, then lock behavior with tests, then refactor core, then wire the view, then remove the diagnostic and update the changelog. All alignment/conflict logic stays in `yampt.core`.

## 1. Measurement (temporary, on current eager code)

- [ ] 1.1 Add a temporary `[debug]` log at the end of `plugin_scan_t::rebuild_conflicts()` summing retained `slot_result` bytes (`contents` + `parsed`/`aligned` overhead) across all entries and, separately, `esm_reader_t::m_records` bytes across all plugins. (R5.1, R5.2)
- [ ] 1.2 User loads the large (POTI) profile once and reports the two numbers; record the `slot_result` figure as the expected saving and the `m_records` figure as the remaining baseline. (R5.3)

## 2. Lock behavior with unit tests (before refactor)

- [ ] 2.1 Add `yampt.tests/source/tests.plugin_scan_slot_result.cpp` (register in vcxproj + filters). (R7.3)
- [ ] 2.2 `compute_conflict` tests on an in-memory `plugin_scan_t` with synthetic multi-version records: identical override ⇒ `conflict_all` no-conflict; differing sub-record ⇒ conflict; a deleted version handled; assert `conflict_all` and per-version `conflict_this` equal documented expected values. Must pass on current code. (R1.3, R7.1)
- [ ] 2.3 `build_slot_result` tests: alignment `aligned` slot keys/indices match the eager alignment for the same versions, including (a) a merge-plugin version and (b) a reduced-versions (hide-duplicates-style) entry. (Written now; compiles once task 4.3 adds the method — sequence 2.1-2.2 first if `build_slot_result` does not yet exist, then extend this file in task 4.) (R7.2)

## 3. Core: extract content gathering

- [ ] 3.1 Add private `gathered_versions_t` + `plugin_scan_t::gather_version_contents(const conflict_entry_t &)` in plugin_scan.hpp/.cpp, reproducing the existing per-version content loop exactly (merge branch via `m_merge_store`, plugin branch via `m_plugins[idx]->esm`, `is_deleted` flag). Non-const (moves the esm read cursor). (R1.2)

## 4. Core: transient + on-demand alignment

- [ ] 4.1 Rewrite `compute_conflict` to gather via `gather_version_contents` and build a LOCAL `slot_result_t sr` (no assignment to `entry.slot_result`); point the existing accumulation loop / `slot_eval_context_t` at the local. Keep the accumulation body verbatim. (R1.1, R1.3, R1.4)
- [ ] 4.2 Remove the `std::unique_ptr<slot_result_t> slot_result` member from `conflict_entry_t`; add `#include` of `decoder/conflict_slots.hpp` to plugin_scan.hpp (now needed by value). (R2.1)
- [ ] 4.3 Add public `slot_result_t plugin_scan_t::build_slot_result(const conflict_entry_t &)` returning by value, using `gather_version_contents` + `conflict_slots::build`, operating on the entry's current `versions`. (R3.1, R3.2)
- [ ] 4.4 Remove `entry.slot_result.reset();` from `recompute_single_conflict`; leave the rest of the method unchanged. (R2.2)

## 5. Core: extend build_slot_result tests

- [ ] 5.1 Complete task 2.3 assertions now that `build_slot_result` exists (merge-plugin version case + reduced-versions case). (R7.2)

## 6. Editor: build alignment on demand in the view

- [ ] 6.1 `view_tree_model_t::set_record_generic` gains a `plugin_scan_t &` parameter (hpp + cpp); the `set_record` dispatch passes `scan` to the generic and dial cases; `set_record_dial` forwards `scan` to `set_record_generic`. (R3.4)
- [ ] 6.2 In `set_record_generic`, replace the `entry.slot_result` read (and the `build_occurrence_based` fallback branch) with `const slot_result_t sr = scan.build_slot_result(entry);` + `content_alignment_t::build_from_slot_result(sr, align_ctx);`. Include `<decoder/conflict_slots.hpp>`. (R3.3, R6.2)
- [ ] 6.3 Grep for `build_occurrence_based`; if no caller remains, remove it and its declaration (dead code); otherwise leave unchanged. (R3.3)

## 7. Verify merged patch and recompute paths

- [ ] 7.1 Confirm `merge_controller_t::refresh_after_merge` (recompute + `display_record`) renders a freshly-merged record correctly under on-demand build (merge column included via `m_merge_store` branch in `gather_version_contents`). No code change expected; verify by reading the path. (R4.2, R4.3)

## 8. Remove diagnostic and finalize

- [ ] 8.1 Remove the temporary `[debug]` measurement log from `rebuild_conflicts()` (no `[debug]` left in committed code). (R5.2)
- [ ] 8.2 Add a single CHANGELOG line under the current unreleased section, yEditor, `[FIX]`: yEditor uses significantly less memory when loading very large load orders. No manual/README/BBCode change (no user-visible feature or label). (changelog-categories, readme rules)

## Notes

- yEditor-only; yTranslator has no plugin scan.
- All alignment/conflict logic stays in `yampt.core`; the editor only threads `scan` into the view and calls `build_slot_result`.
- Conflict results (`conflict_all`, `conflict_this`), nav colors, and counts must be bit-for-bit unchanged — task 2 tests guard this.
- No caching added (R3.5); on-demand build is one alignment per record selection.
- The baseline `esm_reader_t::m_records` copy is only measured, not changed; a follow-up is out of scope.
- Building and running tests are done manually by the user (project no-build rule); tasks produce tests as artifacts, no "run tests" step.
