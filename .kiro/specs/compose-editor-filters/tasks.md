# Implementation Plan

## Overview

Make yEditor's three filter controls compose instead of overwriting each other. The navigation model already ANDs every dimension, so the work is confined to `plugin_workspace_view_t`: replace the drifting duplicated state (`m_last_filter_state`, `m_last_quick_filter`, `m_filter_active`, `m_has_filter_active`) with three independent inputs (`m_conflicts_only`, `m_advanced_filter`, `m_search_filter`), and route every control through one pure composer plus one apply path. Conflicts Only becomes a shortcut that presets the conflict-all dimension. Work order: add the pure `compose_filter` and unit-test it first, then rewire the view's inputs and the four entry points, then register the test and update docs.

## Tasks

- [ ] 1. Add the pure compose_filter function
  - Add `nav_tree_model_t::filter_state_t compose_filter(bool conflicts_only, const filter_state_t & advanced, const filter_state_t & search)` as a pure function (free function in a new `model/filter_composer.hpp/.cpp`, or a namespace function beside `nav_tree_filter`).
  - Logic: start from `advanced`; overlay the six search fields from `search`; if `conflicts_only`, set conflict-all to `{conflict, override_benign}` (overriding advanced's conflict-all); otherwise leave advanced's conflict-all.
  - No Qt, no UI, no I/O — pure state in/out.
  - _Requirements: R1, R2_

- [ ] 2. Unit-test compose_filter
  - `[u]` tests: shortcut-only; advanced conflict-all preserved when shortcut off; shortcut overrides advanced conflict-all while other advanced dims survive; advanced+search fields both present; all-empty equals default-constructed state.
  - Follow naming convention `"compose_filter, <description>"` (or `namespace::compose_filter, ...`).
  - _Requirements: R1, R2, R3_

- [ ] 3. Replace view filter members with the three inputs
  - In `plugin_workspace_view.hpp`: remove `m_filter_active`, `m_last_filter_state`, `m_has_filter_active`, `m_last_quick_filter`; add `m_advanced_filter` and `m_search_filter` (keep `m_conflicts_only`).
  - Declare private `build_effective_filter() const` and `apply_effective_filter()`.
  - _Requirements: R3_

- [ ] 4. Implement build/apply through the composer
  - `build_effective_filter()` returns `compose_filter(m_conflicts_only, m_advanced_filter, m_search_filter)`.
  - `apply_effective_filter()`: if result equals default-constructed state → `m_nav_view->clear_filter()`, else `m_nav_view->set_filter(result)`; then `update_status()`.
  - _Requirements: R1, R3_

- [ ] 5. Rewire on_filter_changed (Conflicts Only)
  - Reduce `on_filter_changed()` to `apply_effective_filter();` (shortcut logic now lives in the composer). `set_conflicts_only` still sets `m_conflicts_only` then calls it.
  - _Requirements: R1, R2_

- [ ] 6. Rewire on_advanced_filter
  - Pre-populate `filter_dialog_t` from `m_advanced_filter` (advanced dims only).
  - On accept, translate `filter_dialog_t::filter_state_t` into the advanced dimensions of `m_advanced_filter` (conflict-all, conflict-this, type, deleted, Lua); do not touch search fields or Conflicts Only; then `apply_effective_filter()`.
  - _Requirements: R1, R5_

- [ ] 7. Rewire apply_search
  - Write the six search fields into `m_search_filter` (same computation as today: `filter_by_id = in_id && !query.empty()`, etc.); then `apply_effective_filter()`. No advanced/conflicts fields touched.
  - _Requirements: R1_

- [ ] 8. Update reset_all_filters
  - Set `m_conflicts_only = false; m_advanced_filter = {}; m_search_filter = {};` then `apply_effective_filter()`. Leave `m_hide_duplicates` untouched.
  - _Requirements: R4_

- [ ] 9. Register the new test (and composer file) in the tests project
  - Add `tests.filter_composer.cpp` (and `filter_composer.cpp` if created as a new file) to `yampt.tests.vcxproj` + `.vcxproj.filters`, flat, in sync.
  - _Requirements: R1, R2, R3_

- [ ] 10. Update documentation
  - CHANGELOG `[CHANGE]` (yEditor): Conflicts Only, Advanced Filters, and search now combine instead of replacing each other; Conflicts Only is a shortcut for showing conflicts.
  - `docs/yEditor-Manual.md`: the three filter controls stack; Conflicts Only is a preset composed with the rest.
  - README + README.bbcode kept in sync if they describe yEditor filtering.
  - _Requirements: R1, R2_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1], "depends_on": [] },
    { "wave": 2, "tasks": [2, 3], "depends_on": [1] },
    { "wave": 3, "tasks": [4], "depends_on": [3] },
    { "wave": 4, "tasks": [5, 6, 7, 8], "depends_on": [4] },
    { "wave": 5, "tasks": [9, 10], "depends_on": [2, 5, 6, 7, 8] }
  ]
}
```

The pure composer (1) is the critical prerequisite; once it exists and is tested (2) and the members are swapped (3), the single apply path (4) unlocks the four entry-point rewrites (5–8), which are independent of each other. Test registration and docs (9, 10) land last.

## Notes

- The navigation model is unchanged — `passes()` already ANDs all dimensions and `set_filter` replaces wholesale. All work is view-side state plus one pure function.
- Conflicts Only is a shortcut, not an advanced selection: the composer overrides the conflict-all dimension when it is on, so the two never conflict and cannot produce an empty-set wipeout.
- Empty-vs-active is decided by comparing the composed state to a default-constructed `filter_state_t` (defaulted `operator==`), removing the `m_has_filter_active` bookkeeping.
- The composer is extracted as a pure function specifically so it is unit-testable without instantiating the Qt view, per the testable-interfaces rule.
- Hide Duplicates stays a separate toggle and is not part of composition or reset.
- Building and running tests is done manually by the user (no-build-or-test rule).
