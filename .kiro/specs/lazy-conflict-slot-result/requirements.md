# Requirements — Lazy Conflict Slot Result in yEditor

## Background — Current Behavior

When yEditor loads plugins, `plugin_scan_t::rebuild_conflicts()` (yampt.core/source/scanner/plugin_scan.cpp) groups every record from every plugin into `m_entries` keyed by `rec_type\0record_id`, then calls `compute_conflict(entry)` for **every entry that has 2 or more versions**:

```cpp
for (auto & entry : m_entries)
{
    if (entry.versions.size() >= 2)
        compute_conflict(entry);
}
```

`compute_conflict(conflict_entry_t & entry)` (plugin_scan.cpp ~279) does two things:

1. Gathers a full copy of every version's raw record bytes into `std::vector<std::string> contents` (reading from `m_merge_store` for the merge plugin, otherwise from `m_plugins[idx]->esm`), plus a per-version `is_deleted` flag.
2. Builds the slot alignment and **stores it permanently on the entry**:

```cpp
entry.slot_result =
    std::make_unique<slot_result_t>(conflict_slots::build(entry.rec_type, std::move(contents), is_deleted));

conflict_accumulator_t accum;
const auto & sr = *entry.slot_result;
for (const auto & slot : sr.aligned) { ... }   // accumulate conflict_all / conflict_this

entry.conflict_all = accum.worst_all;
if (accum.worst_all <= conflict_all_t::only_one)
    return;                                     // slot_result already stored, even for non-conflicts
apply_worst_this(entry, accum, is_deleted);
```

`slot_result_t` (yampt.core/source/decoder/conflict_slots.hpp) holds:

```cpp
struct slot_result_t
{
    std::vector<std::string> contents;                         // a SECOND full copy of every version's bytes
    std::vector<std::vector<sub_record_view_t>> parsed;        // views into contents
    std::vector<aligned_slot_t> aligned;
    std::vector<bool> is_deleted;
};
```

The `slot_result` is retained on `conflict_entry_t` for the entire session:

```cpp
struct conflict_entry_t
{
    std::string rec_type;
    std::string record_id;
    std::string display_name;
    std::string dial_name;
    conflict_all_t conflict_all = conflict_all_t::unknown;
    bool has_dele = false;
    std::vector<record_version_t> versions;
    std::unique_ptr<slot_result_t> slot_result;   // <-- retained per entry
};
```

## Consumers of `slot_result` (complete inventory)

There are exactly four touch points across the whole codebase (yampt.core, yampt.editor, yampt.qt, yampt.tests). No serialization, session-state, or disk format touches it — it is a purely in-memory, session-lived cache.

1. WRITE (build) — `plugin_scan_t::compute_conflict`, plugin_scan.cpp ~319: `entry.slot_result = make_unique<...>`.
2. WRITE (reset) — `plugin_scan_t::recompute_single_conflict`, plugin_scan.cpp ~580: `entry.slot_result.reset();` immediately before re-running `compute_conflict`.
3. READ (transient, internal) — `plugin_scan_t::compute_conflict`, plugin_scan.cpp ~323 onward: `const auto & sr = *entry.slot_result;` used only inside the function for the accumulation loop and `evaluate_schema_fields` / `evaluate_flag_bits` (via `slot_eval_context_t`). Nothing outside the function reads it during the scan — a local `slot_result_t` fully satisfies this use.
4. READ (view display) — `view_tree_model_t::set_record_generic`, yampt.editor/source/model/view_tree_decode_lists.cpp ~244:

```cpp
if (entry.slot_result)
{
    alignment_context_t align_ctx { all_subs, col_count, unified_slots, col_type_indices };
    content_alignment_t::build_from_slot_result(*entry.slot_result, align_ctx);
}
else
{
    content_alignment_t::build_occurrence_based(align_ctx);   // lossy fallback
}
```

This is the ONLY view-side read. It is reached only by the `generic` decode mode and by `dial` (which delegates to `set_record_generic`). The `cell`, `leveled`, `faction`, `container`, `armor`, and `info` decode modes never read `slot_result` — they rebuild alignment from `context.all_sub_records` via `content_alignment_t::align`.

The hide-duplicates display path (`plugin_workspace_view_t::display_record_in_view`, plugin_workspace_view.cpp ~695) builds a fresh `conflict_entry_t filtered;` with a de-duplicated `versions` list and a null `slot_result`, so it already runs through the lossy `build_occurrence_based` fallback today. Any lazy build must operate on the entry's **current** `versions`, not a canonical lookup.

## Problem

`slot_result` is built eagerly and retained for every multi-version record — including records whose versions turn out to be identical (no conflict), which is the majority in a large load order. Each retained `slot_result` holds a second full copy of every version's record bytes (`contents`) plus per-sub-record `parsed` overhead and per-slot `aligned` vectors. On very large load orders (reported: ~1000 plugins ⇒ ~7 GB resident) this retention is a dominant, growing memory cost. The view only ever needs the alignment for the single record the user is currently viewing, and the accumulation inside `compute_conflict` only needs it transiently.

## Goal

Stop retaining `slot_result` on `conflict_entry_t`. Compute the alignment transiently inside `compute_conflict` (local, discarded on return) and build it on demand at display time for the one record being shown. Peak retained alignment memory drops from O(all multi-version records × version bytes) to O(one displayed record).

## User-Facing Outcomes

- yEditor uses substantially less memory after loading a large profile; the amount saved is measured, not assumed (R5).
- Loading a profile is no slower (and generally faster: the eager per-entry alignment build is removed from the load pass).
- Selecting a record still shows the full, correctly-aligned side-by-side comparison, including for records shown in hide-duplicates mode and for the merged-patch column.
- Conflict colors, conflict counts, and per-version conflict status are unchanged.

## Requirements

### R1 — Transient alignment in conflict computation

1.1 `compute_conflict` builds the `slot_result_t` as a **local** value used only for the accumulation loop; it does NOT assign it to `entry.slot_result`.
1.2 The content-gathering logic (per-version `contents` from `m_merge_store` for the merge plugin, otherwise from `m_plugins[idx]->esm`, plus `is_deleted`) is preserved exactly, including the merge-plugin branch.
1.3 `entry.conflict_all` and per-version `conflict_this_t` status (`apply_worst_this`) are computed identically to today — bit-for-bit the same conflict results for the same input.
1.4 The early return for non-conflicting records (`accum.worst_all <= conflict_all_t::only_one`) no longer leaves any retained state behind, because nothing is stored.

### R2 — Remove the retained member

2.1 The `std::unique_ptr<slot_result_t> slot_result` member is removed from `conflict_entry_t` (nothing stores or persists it after R1/R3).
2.2 `recompute_single_conflict`'s `entry.slot_result.reset();` is removed (it becomes meaningless once nothing is stored). The subsequent `if (entry.versions.size() >= 2) compute_conflict(entry);` is retained.
2.3 No other field of `conflict_entry_t` changes. The struct remains default-constructed and passed by reference everywhere (it is never copied; the removal does not change copyability requirements but does simplify the type).

### R3 — On-demand alignment build for display

3.1 A new public method on `plugin_scan_t` materializes the alignment for a given entry by value, reproducing the exact content-gathering of `compute_conflict` (R1.2): e.g. `slot_result_t build_slot_result(const conflict_entry_t & entry) const`. It lives in yampt.core because it needs `m_plugins` / `m_merge_store` access.
3.2 The builder operates on the entry's current `versions` (so the hide-duplicates `filtered` entry with a reduced version list produces a correct alignment for exactly those columns, and the merged-patch column is included when present).
3.3 `view_tree_model_t::set_record_generic` builds the alignment on demand via the new method instead of reading `entry.slot_result`, and feeds it to `content_alignment_t::build_from_slot_result`. The lossy `build_occurrence_based` fallback is no longer used for generic/dial records (they now always get the real alignment). Whether the fallback remains for any other caller is decided in design; if it has no remaining caller it may be left in place unused or removed.
3.4 `set_record_generic` (and `set_record_dial`, which delegates to it) receive the `plugin_scan_t &` needed to call the builder; it is threaded from `view_tree_model_t::set_record(plugin_scan_t & scan, ...)`, which already has it. No new ownership or global access is introduced.
3.5 The on-demand build runs once per record selection for the single displayed record only. No caching is added in this spec; if profiling later shows repeated re-selection cost matters, a single-entry cache can be added separately.

### R4 — Merged patch and per-record recompute unchanged

4.1 The auto-merge / patch-building path (`auto_merge`, `sub_record_merge`, `merge_patch_ops`, `merge_patch_store`, `plugin_cleaner`, `patch_builder`) does not use `slot_result` and is unchanged.
4.2 `merge_controller_t::refresh_after_merge` (recompute + `display_record`) works under the new model: `recompute_single_conflict` recomputes conflict levels with a local alignment, and the subsequent `display_record` builds the alignment on demand from current `m_merge_store` content, so a freshly-merged record displays correctly.
4.3 The merged-patch column, when present as one of an entry's versions, is included in the on-demand build exactly as it was in the eager build (via the `m_merge_store` content branch).

### R5 — Measurement

5.1 Before implementing the refactor, a temporary diagnostic reports, after `rebuild_conflicts()`, the total retained `slot_result` memory (sum of `contents` bytes plus `parsed`/`aligned` container overhead across all entries) and, separately, the baseline `esm_reader_t::m_records` bytes across all plugins. This quantifies the expected saving and the remaining baseline floor on the user's real profile.
5.2 The diagnostic is `[debug]`-tagged logging (per naming-conventions log style) and is removed before the change is considered complete (no `[debug]` left in committed code).
5.3 The saving is reported to the user as a concrete before/after from their profile, not an estimate.

### R6 — No regression

6.1 Conflict detection results (`conflict_all`, per-version `conflict_this`), nav-tree colors, and the conflict count in the status label are identical to before for the same load order.
6.2 The record view renders identically for all decode modes (generic, dial, cell, leveled, faction, container, armor, info), including hide-duplicates mode and the merged-patch column.
6.3 `batch_cleaner_t`, field-edit Apply, exclude/guard-patch marking, session save/restore, and all other behaviors are unchanged.
6.4 `plugin_scan_t` / `conflict_entry_t` stay in yampt.core with no new dependency on yampt.editor / yampt.qt / Qt.

### R7 — Verification

7.1 Pure `[u]` unit tests (in-memory synthetic records, no file I/O) assert that, for representative multi-version inputs, `compute_conflict` yields the same `conflict_all` and per-version `conflict_this` after the refactor as the documented expected values (guarding R1.3).
7.2 A `[u]` test asserts `plugin_scan_t::build_slot_result(entry)` produces an alignment equivalent to what the eager build produced (same `aligned` slot keys/indices) for the same versions, including a case where one version is the merge plugin and a hide-duplicates-style reduced-versions case.
7.3 Tests are written before the refactor and fail against nothing (they encode expected conflict outcomes) — they lock the behavior so the refactor cannot silently change conflict results. No test writes to disk. Building and running tests are done manually by the user (project no-build rule).

## Open Decisions

Resolved:
- Approach → full lazy build (Option A): transient local in `compute_conflict`, on-demand build at display, member removed. Chosen over the minimal "don't retain for non-conflicts" patch because measurement (~7 GB at ~1000 plugins) shows retention is a dominant cost and the lazy build also fixes the hide-duplicates lossy-fallback correctness gap.
- On-demand builder location → `plugin_scan_t::build_slot_result` in yampt.core (needs `m_plugins`/`m_merge_store`).
- Builder operates on the entry's current `versions` (handles hide-duplicates and merge column uniformly).
- No caching in this spec (R3.5).
- Measure first, then refactor (R5).

Deferred to design:
- Exact signature/return convention of `build_slot_result` (by value; whether it takes the entry or an explicit versions list).
- Whether `content_alignment_t::build_occurrence_based` retains any caller or becomes dead (R3.3).
- Whether the baseline `esm_reader_t::m_records` copy warrants a follow-up spec (out of scope here; only measured, not changed).
