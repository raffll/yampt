# Requirements — Merge the Leveled-List Calc Flag and Chance-None Across Contributors (yEditor)

## Background — Current Behavior

yEditor auto-merges leveled lists (LEVI = item lists, LEVC = creature lists) additively.

- `auto_merge_t::process_leveled_list` calls `leveled_list_merge_t::merge` (yampt.core/scanner/sub_record_merge.cpp).
- The merge unions the list entries across versions by (ident, level): an entry present in the first master but missing from a later version is treated as an intentional deletion and dropped; surviving entries take the max count across later versions; the merged entries are sorted and rebuilt.
- The rebuilt record's header is produced by `extract_list_header(winning_content)` — it copies every header sub-record **except** the entry sub-records (INAM/CNAM/INTV/INDX) verbatim from the **winning (last-listed) version only**. This header includes:
  - **DATA** — the leveled-list flags (u32). For LEVI: bit 0 = "Calc for Each Item", bit 1 = "Calc from All Levels". For LEVC: bit 0 = "Calc from All Levels". (from `sub_record_schema.cpp`)
  - **NNAM** — "chance none" (the percentage chance the list produces nothing).
- Because the header is taken solely from the winning plugin, the DATA calc flags and NNAM are **not merged** across contributors. If two mods enable different calc flags on the same list, the last-listed plugin's flags silently win; the others are discarded. Same for NNAM.

Reference tools (TES3Merge) OR the calc flags across contributors so that if any mod enables "Calc from All Levels" / "Calc for Each Item", the merged list enables it.

## Problem

The user observed that the creatures leveled lists behave in ways they can't figure out and that "calculate from all levels" is not being handled. Confirmed in code: the leveled-list merge unions the *entries* correctly but takes the DATA calc flags (and NNAM chance-none) only from the winning plugin. So when several mods contribute to the same leveled list with different calc-flag settings, the merged list reflects only the last plugin's flags — a mod that turned on "Calc from All Levels" or "Calc for Each Item" has its setting dropped if a later plugin didn't set it. This produces spawn/loot behavior that doesn't match the union of the contributing mods, which is exactly the surprising behavior reported.

## Goal

Merge the leveled-list DATA calc flags (and decide the correct policy for NNAM chance-none) across all contributing versions rather than taking them from the winning plugin alone, so the merged list's flags reflect the combined intent of the contributors — matching the additive/union philosophy already used for the list entries and consistent with reference merge tools.

## User-Facing Outcomes

- When multiple mods contribute to the same leveled list, the merged list's calc flags are the OR (union) of the contributors' flags: if any contributor enables "Calc from All Levels" (or, for LEVI, "Calc for Each Item"), the merged list enables it.
- Leveled-list spawn/loot behavior after merging matches the combined intent of the source mods, resolving the "does things I can't figure out" surprise for creature lists.
- The list entries continue to merge as they do today (union by ident/level, deletions honored) — this change only fixes the flags/header, not the entry set.
- NNAM chance-none is handled by a defined, documented policy (see R2) rather than silently inheriting the winning plugin's value when that may be wrong.

## Requirements

### R1 — OR-merge the DATA calc flags

1.1 The merged leveled list's DATA flag value is the bitwise OR of the DATA flags across all contributing versions (all versions in the merge group), for both LEVI and LEVC.
1.2 This applies to the defined calc bits: LEVI bit 0 ("Calc for Each Item") and bit 1 ("Calc from All Levels"); LEVC bit 0 ("Calc from All Levels"). Undefined/reserved bits in DATA are handled by the design (default: OR the whole u32, since only the low bits are defined and vanilla data has the rest zero; the design confirms no reserved-bit hazard).
1.3 If a contributor lacks a DATA sub-record (malformed/absent), it contributes 0 to the OR (no flags), and the merge still produces a valid DATA in the output.

### R2 — NNAM chance-none policy

2.1 NNAM (chance none, a byte percentage) is currently taken from the winning plugin. The design defines the correct merge policy and documents it. Candidate policies: (a) keep winning-plugin value (status quo, but now deliberate); (b) take the minimum across contributors (most generous — the list is least likely to produce nothing, matching an additive "more content" philosophy); (c) take the maximum. Default direction: **minimum** across contributors, consistent with the additive union philosophy (more mods contributing content should not make the list emptier), unless the design finds a reason to prefer winning-plugin.
2.2 Whatever policy is chosen, it is applied consistently and only when contributors actually differ; if all NNAM values agree, the result is that value.
2.3 A contributor lacking NNAM is treated per the design (default: treated as absent/does-not-constrain for a min policy; a leveled list without NNAM defaults to 0 chance-none in the engine).

### R3 — Preserve everything else in the header

3.1 All other header sub-records (anything that is not the entry sub-records INAM/CNAM/INTV/INDX and not DATA/NNAM) continue to be taken from the winning version verbatim, unchanged from today. Only DATA (and NNAM per R2) get merged.
3.2 The entry-merge logic (union by ident/level, deletion detection, max count, sort) is unchanged.
3.3 The output record structure (record header bytes, INDX count, entry ordering, sub-record layout) is otherwise identical to today's `build_merged_list_record` output aside from the merged DATA/NNAM values.

### R4 — Merge input carries all versions' flags

4.1 `leveled_list_merge_t::merge` receives (or extracts from) all contributing version contents, so it can read each version's DATA (and NNAM) to compute the OR/policy. The current `merge_input_t::version_contents` already holds all versions; the flags are extracted from each rather than only the winning one.
4.2 The winning version is still used for the non-merged header remainder (R3.1) and remains the base for entry-winner selection as today.

### R5 — Scope

5.1 This affects only LEVI and LEVC via `leveled_list_merge_t`. No other record type's merge changes.
5.2 It applies during automatic merge (`auto_merge_t::process_leveled_list`). It does not change manual copy behavior (a manually copied leveled list is copied verbatim; this is about the auto-merge output).
5.3 The "changed?" gate (`leveled_list_merge_t::merge` returns `changed=false` when the rebuilt record equals the winning content) must account for the merged flags: if OR-merging the flags changes DATA relative to the winning version, that counts as a change and the merged record is emitted.

### R6 — No regression

6.1 When all contributors already share the same DATA/NNAM (the common case), the output is byte-for-byte identical to today.
6.2 The entry union/deletion/count/sort behavior is unchanged and covered by any existing leveled-list tests.
6.3 Record types other than LEVI/LEVC are unaffected.

### R7 — Verification

7.1 Pure `[u]` tests (in-memory synthetic LEVI/LEVC contents, no disk) for `leveled_list_merge_t::merge`:
- two versions with different DATA flags → merged DATA is the OR; "Calc from All Levels" set by one contributor is present in the output.
- LEVI "Calc for Each Item" from one contributor OR'd in.
- a contributor missing DATA contributes 0; output DATA still valid.
- NNAM policy: contributors with differing NNAM → output equals the chosen policy result (e.g. minimum); equal NNAM → that value.
- all-equal flags → output identical to today (no spurious change), and the changed-flag gate behaves (change reported only when flags actually differ from the winning version).
- entry union/deletion/count/sort unchanged (regression guard).
7.2 Manual verification: merge a load order where two mods set different calc flags on the same creature list; confirm the merged list has both flags (OR) and spawns as expected.

## Open Decisions

Resolved:
- OR-merge (union) the DATA calc flags across all contributing versions, for LEVI and LEVC. (R1)
- Only DATA (and NNAM per policy) are merged; the rest of the header stays from the winning version. (R3)
- Scope is LEVI/LEVC auto-merge only. (R5)

Deferred to design:
- NNAM chance-none policy: minimum (default) vs. winning-plugin vs. maximum (R2.1). Confirm the engine semantics of chance-none to pick correctly.
- Whether to OR the entire DATA u32 or mask to the defined bits (R1.2) — confirm no reserved-bit hazard in real data.
- Exact extraction of DATA/NNAM from each version content (reuse `sub_record_iter_t` in `extract_list_header`, or a small dedicated extractor).
