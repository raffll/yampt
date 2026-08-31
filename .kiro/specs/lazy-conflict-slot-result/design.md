# Design — Lazy Conflict Slot Result in yEditor

All logic stays in `yampt.core` (pure, no Qt). The editor's only change is that the record-view model asks the scan to build the alignment on demand instead of reading a stored pointer. Conflict-detection results are unchanged; the alignment that was retained per entry becomes transient (computed inside `compute_conflict`) and on-demand (rebuilt for the single displayed record).

## Overview of the change

Today (eager, retained):

```
rebuild_conflicts()
  └─ for each multi-version entry: compute_conflict(entry)
       ├─ gather contents[] + is_deleted[]
       ├─ entry.slot_result = make_unique<slot_result_t>(conflict_slots::build(...))   // STORED forever
       ├─ accumulate over *entry.slot_result -> entry.conflict_all / conflict_this
       └─ (returns; slot_result stays resident)

view display (generic/dial):
  set_record_generic(entry)
     └─ if (entry.slot_result) build_from_slot_result(*entry.slot_result)  // read stored
        else build_occurrence_based()                                       // lossy fallback
```

After (transient + on-demand):

```
rebuild_conflicts()
  └─ for each multi-version entry: compute_conflict(entry)
       ├─ gather contents[] + is_deleted[]
       ├─ slot_result_t sr = conflict_slots::build(...)   // LOCAL, discarded on return
       ├─ accumulate over sr -> entry.conflict_all / conflict_this
       └─ (returns; sr destroyed)

view display (generic/dial):
  set_record_generic(scan, entry)
     └─ slot_result_t sr = scan.build_slot_result(entry)   // built on demand, for THIS record
        build_from_slot_result(sr)
```

`conflict_entry_t::slot_result` is removed. Nothing retains a `slot_result_t` beyond the record currently being displayed.

## Component 1 — Content gathering extracted (yampt.core)

`compute_conflict` currently inlines the per-version content gathering (plugin_scan.cpp ~296-317). This same logic is needed by the new on-demand builder, so it is extracted into a private helper to avoid duplication (DRY):

```cpp
// plugin_scan.hpp (private)
struct gathered_versions_t
{
    std::vector<std::string> contents;
    std::vector<bool> is_deleted;
};

gathered_versions_t gather_version_contents(const conflict_entry_t & entry) const;
```

Implementation reproduces the existing loop exactly, including the merge branch:

```cpp
gathered_versions_t plugin_scan_t::gather_version_contents(const conflict_entry_t & entry) const
{
    const size_t ver_count = entry.versions.size();
    gathered_versions_t out;
    out.contents.resize(ver_count);
    out.is_deleted.assign(ver_count, false);

    for (size_t i = 0; i < ver_count; ++i)
    {
        const auto & ver = entry.versions[i];

        if (ver.plugin_idx == m_merge_plugin_idx)
        {
            out.contents[i] = m_merge_store.record_content(ver.record_index);
            continue;
        }

        m_plugins[ver.plugin_idx]->esm.select_record(ver.record_index);
        out.contents[i] = m_plugins[ver.plugin_idx]->esm.get_record().content;

        const auto & plugin_entries = m_plugins[ver.plugin_idx]->index.entries();
        if (ver.record_index < plugin_entries.size() && plugin_entries[ver.record_index].has_dele)
            out.is_deleted[i] = true;
    }

    return out;
}
```

Note this method calls `esm.select_record` (non-const on `esm_reader_t`) via `m_plugins[idx]->esm`. `compute_conflict` is already non-const and mutates via the same call; `build_slot_result` is used from a const view path. Two options, decided here:

- The `esm_reader_t::select_record` only sets `ptr_record` / clears `m_key`/`m_value` (cursor state), not logical content. `gather_version_contents` and `build_slot_result` are therefore declared **non-const** on `plugin_scan_t` (they move the read cursor), and the view holds a non-const `plugin_scan_t &` — which it already does: `view_tree_model_t::set_record(plugin_scan_t & scan, ...)` and `record_view_t::display_record(plugin_scan_t & scan, ...)` take non-const references. So no const-correctness change is forced. `gather_version_contents`/`build_slot_result` are non-const, matching `compute_conflict`.

## Component 2 — compute_conflict uses a local (yampt.core)

`compute_conflict` is rewritten to gather via the helper and keep the alignment local:

```cpp
void plugin_scan_t::compute_conflict(conflict_entry_t & entry)
{
    const auto gathered = gather_version_contents(entry);
    const auto & is_deleted = gathered.is_deleted;

    const slot_result_t sr =
        conflict_slots::build(entry.rec_type, gathered.contents, is_deleted);

    conflict_accumulator_t accum;
    for (const auto & slot : sr.aligned)
    {
        // ... unchanged accumulation, reading sr.parsed / sr.aligned ...
    }

    entry.conflict_all = accum.worst_all;

    if (accum.worst_all <= conflict_all_t::only_one)
        return;

    apply_worst_this(entry, accum, is_deleted);
}
```

The accumulation body (policy lookup, `slot_values`, `find_schema`, `slot_eval_context_t`, `evaluate_schema_fields` / `evaluate_flag_bits`, `accum.accumulate`) is copied verbatim — it already reads through a `const slot_result_t & sr`, so pointing `sr` at the local instead of `*entry.slot_result` is the only change. `slot_eval_context_t` still binds `const slot_result_t & sr` to the local.

`conflict_slots::build` has both a copy and an rvalue overload. The eager code used `std::move(contents)`. Because `gather_version_contents` returns the contents by value and `compute_conflict` does not need them afterwards, it can still move: `conflict_slots::build(entry.rec_type, std::move(gathered.contents), is_deleted)` where `gathered` is a local mutable copy. (`build_slot_result` below has the same freedom.)

## Component 3 — build_slot_result on demand (yampt.core)

New public method returning by value:

```cpp
// plugin_scan.hpp (public)
slot_result_t build_slot_result(const conflict_entry_t & entry);
```

```cpp
slot_result_t plugin_scan_t::build_slot_result(const conflict_entry_t & entry)
{
    auto gathered = gather_version_contents(entry);
    return conflict_slots::build(entry.rec_type, std::move(gathered.contents), gathered.is_deleted);
}
```

- Operates on `entry.versions` as-is, so the hide-duplicates `filtered` entry (reduced versions) and the merged-patch column (a version with `plugin_idx == m_merge_plugin_idx`) are handled uniformly (R3.2, R4.3).
- Returns by value; NRVO/move applies. The caller (view) holds it as a local for the duration of one record build.

## Component 4 — conflict_entry_t member removed (yampt.core)

`plugin_scan.hpp`:

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
    // slot_result member removed
};
```

`recompute_single_conflict` (plugin_scan.cpp ~578): remove the line `entry.slot_result.reset();`. The rest (clearing versions of the merge plugin, re-adding from `m_merge_store`, setting `conflict_all = only_one`, then `if (versions.size() >= 2) compute_conflict(entry)`) is unchanged.

With the member gone, `conflict_slots.hpp`'s `slot_result_t` no longer needs to be forward-declared or included by `plugin_scan.hpp` for the member; but `build_slot_result` returns `slot_result_t` by value, so `plugin_scan.hpp` must now **include** `decoder/conflict_slots.hpp` (previously it only needed a forward declaration for the `unique_ptr`). This is a same-project include within yampt.core (`#include "../decoder/conflict_slots.hpp"` or the project convention), no cross-project change.

## Component 5 — view builds on demand (yampt.editor)

`view_tree_model_t::set_record_generic` currently takes `(record_context_t & context, const conflict_entry_t & entry)` and reads `entry.slot_result`. It needs the scan. `set_record` already holds `plugin_scan_t & scan`; thread it through:

- `set_record_generic(record_context_t & context, const conflict_entry_t & entry)` → `set_record_generic(plugin_scan_t & scan, record_context_t & context, const conflict_entry_t & entry)`.
- `set_record_dial(scan, context, entry)` already has `scan`; it calls `set_record_generic` — pass `scan` through.
- The `set_record` dispatch (view_tree_model.cpp ~60-90) passes `scan` to the generic/dial cases.

New body:

```cpp
void view_tree_model_t::set_record_generic(
    plugin_scan_t & scan, record_context_t & context, const conflict_entry_t & entry)
{
    const auto col_count = context.col_count;
    auto & all_subs = context.all_sub_records;

    std::vector<sub_slot_t> unified_slots;
    std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);

    const slot_result_t sr = scan.build_slot_result(entry);
    alignment_context_t align_ctx { all_subs, col_count, unified_slots, col_type_indices };
    content_alignment_t::build_from_slot_result(sr, align_ctx);

    for (const auto & slot : unified_slots)
        m_rows.push_back(build_slot_row(col_count, all_subs, col_type_indices, slot));
}
```

The previous `if (entry.slot_result) ... else build_occurrence_based(...)` branch is gone; generic/dial always get the real alignment (this also fixes the hide-duplicates lossy-fallback gap, R3.3).

`content_alignment_t::build_occurrence_based`: after this change it has no caller from generic/dial. Design decision: check for any other caller; if none remain, leave the function in place (it is small and harmless) unless it is provably dead, in which case remove it and its declaration to avoid dead code. This is resolved during implementation by a grep for `build_occurrence_based`; the task notes it explicitly.

`view_tree_model.cpp` includes `decoder/conflict_slots.hpp` (for the local `slot_result_t`) — a cross-project include via angle brackets per the include convention (`#include <decoder/conflict_slots.hpp>`).

## Component 6 — measurement diagnostic (yampt.core, temporary)

Before the refactor, add a temporary `[debug]` log at the end of `rebuild_conflicts()`:

```cpp
// TEMPORARY — removed before completion (R5.2)
size_t slot_bytes = 0;
for (const auto & entry : m_entries)
{
    if (!entry.slot_result) continue;
    for (const auto & c : entry.slot_result->contents) slot_bytes += c.size();
    for (const auto & p : entry.slot_result->parsed)   slot_bytes += p.size() * sizeof(sub_record_view_t);
    for (const auto & a : entry.slot_result->aligned)  slot_bytes += a.indices.size() * sizeof(size_t);
}
size_t record_bytes = 0;
for (const auto & plugin : m_plugins)
    for (const auto & rec : plugin->esm.get_records()) record_bytes += rec.content.size();

app_logger_t::add_log("[debug] slot_result retained ~" + std::to_string(slot_bytes) +
    " bytes, esm records ~" + std::to_string(record_bytes) + " bytes\r\n");
```

The user loads their POTI profile once and reports the two numbers. This is done on the CURRENT eager code (member still present), so it measures the retained cost being removed and the remaining baseline. It is deleted before the refactor is committed. If a lighter-weight approach is preferred, the same two sums can be gathered in a throwaway helper; the exact form is not load-bearing.

## Ordering

Measure on current code first (Component 6), record the numbers, then perform the refactor (Components 1-5) and remove the diagnostic. The reported saving is the measured `slot_result` figure.

## Files

Modified (yampt.core):
- `scanner/plugin_scan.hpp` — remove `slot_result` member from `conflict_entry_t`; add `gather_version_contents` (private) + `build_slot_result` (public) declarations; add `#include` of `decoder/conflict_slots.hpp` (now needed by value).
- `scanner/plugin_scan.cpp` — extract `gather_version_contents`; rewrite `compute_conflict` to use a local `slot_result_t`; add `build_slot_result`; drop `slot_result.reset()` in `recompute_single_conflict`; add + later remove the measurement diagnostic in `rebuild_conflicts`.

Modified (yampt.editor):
- `model/view_tree_model.hpp` — `set_record_generic` gains `plugin_scan_t &`.
- `model/view_tree_model.cpp` — dispatch passes `scan` to generic/dial cases.
- `model/view_tree_decode_lists.cpp` — `set_record_generic` builds on demand via `scan.build_slot_result(entry)`; `set_record_dial` forwards `scan`; include `<decoder/conflict_slots.hpp>`.
- Possibly `decoder/content_alignment.*` — only if `build_occurrence_based` is confirmed dead and removed.

No vcxproj/filters changes (no files added or removed).

Docs: this is an internal memory/performance change with no new user-visible feature or label. Per changelog rules it is a `[FIX]`/performance note at most; the requirements' user-facing outcome is "uses less memory / loads no slower". A single yEditor `[FIX]`-style CHANGELOG line ("yEditor uses significantly less memory when loading very large load orders") is appropriate; no manual/README change (no feature, no label). Not a `[NEW]`.

## Testing (pure `[u]`, no file I/O)

Written before the refactor to lock behavior (R7.3):

- `plugin_scan_t::compute_conflict` — build an in-memory `plugin_scan_t` with synthetic multi-version records (identical override ⇒ no conflict; differing sub-record ⇒ conflict; deleted version) and assert `entry.conflict_all` and per-version `conflict_this` match documented expected values. This test passes on both the old and new code — it guards that the refactor does not change conflict results.
- `plugin_scan_t::build_slot_result` — assert the returned `slot_result_t.aligned` slot keys/indices match the alignment the eager build produced for the same versions, including (a) a version that is the merge plugin and (b) a reduced-versions (hide-duplicates-style) entry.
- No test constructs `conflict_entry_t::slot_result` (member is gone); tests use `build_slot_result` for alignment assertions.

Building and running tests are done manually by the user (project no-build rule); tasks produce tests as artifacts, no "run tests" step.
