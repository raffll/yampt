# 3-Way Merge Truth Table

This is the authoritative behavior spec for yampt's record/field merge. It enumerates every version combination for 2, 3, and 4 ESPs. It applies uniformly to every mergeable unit: a whole sub-record value, a schema field, a single flag bit (sub-record FLAG fields AND the 16-byte record-header FLAGS), a leveled-list occurrence, a keyed-list entry, and an FRMR owner.

## Terms

- **Base (B)** — the first/lowest-priority version (the master the others were built from). Always `versions.front()`.
- **Winner (W)** — the last/highest-priority version. Always `versions.back()`.
- **Intermediate (I, I1, I2)** — versions between base and winner, in load order (I1 lower priority than I2).
- **Priority** — later in the version list = higher priority. On an irreconcilable disagreement, the higher-priority version wins.

## The Single Rule

Start from Base. Every version that **changed a unit relative to Base** overlays its value onto the result. If two or more versions changed the same unit to **different** values, the **highest-priority (latest) changer wins**. A version that left a unit equal to Base is not a changer for that unit and contributes nothing there.

Equivalent predicate (per unit), used as `intermediate_claims_bit` / `intermediate_claims_span`:

```
a version V claims the unit  ⟺  V changed it vs Base  AND  no higher-priority version also changed it
result = highest-priority claimer's value, else Base
```

Key consequence: **W == B does NOT mean "keep Base".** If an intermediate changed the unit and the winner did not, the intermediate's value stands. Reverting to Base in that case is the bug this spec exists to prevent.

Deletion is a change like any other (see "Removal" at the end).

---

## 2 ESPs — [B, W]

No intermediates. The winner is the only possible changer.

| # | B | W | Result | Reason |
|---|---|---|--------|--------|
| 2.1 | x | x | x | nobody changed → Base |
| 2.2 | x | y | **y** | W changed → W wins |

`changed` (record differs from Base) is true only in 2.2.

This is the case currently dropped: a base + one plugin that edits a unit MUST yield the plugin's value (2.2), not Base.

---

## 3 ESPs — [B, I, W]

I is lower priority than W.

| # | B | I | W | Result | Reason |
|---|---|---|---|--------|--------|
| 3.1 | x | x | x | x | nobody changed → Base |
| 3.2 | x | x | y | **y** | only W changed → W |
| 3.3 | x | y | x | **y** | only I changed → I (W==B is NOT "keep Base") |
| 3.4 | x | y | y | **y** | I and W agree on the change → y |
| 3.5 | x | y | z | **z** | I and W disagree → higher priority (W) wins |

Only 3.1 is unchanged. 3.3 is the classic "intermediate change survives an unchanged winner" case.

---

## 4 ESPs — [B, I1, I2, W]

Priority: W > I2 > I1 > B. I1 and I2 are intermediates, I2 higher priority than I1.

| # | B | I1 | I2 | W | Result | Reason |
|---|---|----|----|---|--------|--------|
| 4.1 | x | x | x | x | x | nobody changed → Base |
| 4.2 | x | x | x | y | **y** | only W changed → W |
| 4.3 | x | x | y | x | **y** | only I2 changed → I2 |
| 4.4 | x | y | x | x | **y** | only I1 changed → I1 |
| 4.5 | x | y | y | x | **y** | I1 & I2 agree, W unchanged → y |
| 4.6 | x | y | z | x | **z** | I1 & I2 disagree, W unchanged → higher priority (I2) wins |
| 4.7 | x | x | y | z | **z** | I2 & W changed, disagree → W wins |
| 4.8 | x | y | x | z | **z** | I1 & W changed, disagree → W wins |
| 4.9 | x | y | z | w | **w** | three-way disagreement → W (highest) wins |
| 4.10 | x | y | y | y | **y** | everyone who changed agrees → y |
| 4.11 | x | y | z | z | **z** | I1 changed to y, I2 & W changed to z → W wins (z) |
| 4.12 | x | y | y | z | **z** | I1 & I2 changed to y, W changed to z → W wins (z) |

General N-version rule (any number of intermediates): scan versions from highest priority (W) down to Base; the first one whose value differs from Base is the result; if none differ, the result is Base.

---

## Per-bit application (flags)

Flag fields (sub-record FLAG and the record-header FLAGS dword at header offset 12) apply the table **independently per bit**. Two plugins toggling different bits both take effect; two toggling the same bit resolve by priority. Example, header Persistent bit only:

- [B=Yes, W=No] → No (2.2)
- [B=Yes, I=No, W=Yes] → No (3.3: I cleared it, W left it as Base, I's clear stands)
- [B=No, I=Yes, W=No] → Yes (3.3 mirror)

## Per-occurrence / per-entry application

- **Leveled lists:** each (item id, occurrence) position and its PC level resolve by the table; item counts and levels are per-changer-wins, not summed.
- **Keyed lists (NPCO/NPCS/FACT reactions):** each entry keyed by its id resolves by the table; union of additions, deletions honored.
- **FRMR owner (ANAM):** resolved by the table only when the winner did not change ANAM vs Base; otherwise winner wins.

## Removal is a change

A version that removes a unit present in Base is a changer whose value is "absent". It participates in the table exactly like a value change: an intermediate removal with an unchanged winner stands (3.3 shape); a winner that keeps the unit while an intermediate removes it → winner wins. Absence in a plugin that never had the unit is NOT a removal (FRMR/reference exception aside — see frmr-reference-model).

---

## Empty / Absent Rules

"Empty" has two distinct meanings that must not be conflated: **absent in the merge** (the unit is not present in that version's record) versus **absent in conflict detection** (a column that is skipped from comparison). They use different mechanisms.

### Sentinel

Absence is represented by the sentinel `non_existent_value` (`"\x00_NE"`, defined in `record_conflict.hpp`), never by an empty string. An empty string is a real value (a zero-length sub-record) and is distinct from absent.

### Merge semantics — presence/absence resolves like any other change

A sub-record/field is either **present (P)** or **absent (∅)** in each version. Presence is itself a mergeable unit resolved by the same rule: relative to Base, a version that flipped presence is a changer; highest-priority changer wins. `v`, `w` denote distinct present values.

Two outcomes are possible in the merged patch: the unit is **written** (with some value) or **omitted**.

#### 2 ESPs — [B, W]

| # | B | W | Result | Reason |
|---|---|---|--------|--------|
| P2.1 | ∅ | ∅ | omit | never existed |
| P2.2 | ∅ | P(v) | **v** | W added → added |
| P2.3 | P(v) | ∅ | **omit** | W removed → removal wins |
| P2.4 | P(v) | P(v) | v | present, unchanged |
| P2.5 | P(v) | P(w) | **w** | W changed value → W wins |

#### 3 ESPs — [B, I, W]

| # | B | I | W | Result | Reason |
|---|---|---|---|--------|--------|
| P3.1 | ∅ | ∅ | ∅ | omit | never existed |
| P3.2 | ∅ | P(v) | ∅ | **v** | I added, W==Base(absent) → addition stands |
| P3.3 | ∅ | ∅ | P(v) | **v** | W added → added |
| P3.4 | ∅ | P(v) | P(v) | **v** | I & W added same → v |
| P3.5 | ∅ | P(v) | P(w) | **w** | both added, differ → W wins |
| P3.6 | P(v) | ∅ | P(v) | **omit** | I removed, W==Base → removal stands |
| P3.7 | P(v) | P(v) | ∅ | **omit** | W removed → removal wins |
| P3.8 | P(v) | ∅ | ∅ | **omit** | I & W removed → omit |
| P3.9 | P(v) | ∅ | P(w) | **w** | I removed, W changed value → W wins |
| P3.10 | P(v) | P(w) | ∅ | **omit** | I changed value, W removed → W (removal) wins |
| P3.11 | P(v) | P(w) | P(v) | **w** | I changed value, W==Base → I's value stands |

#### 4 ESPs — [B, I1, I2, W]  (priority W > I2 > I1 > B)

| # | B | I1 | I2 | W | Result | Reason |
|---|---|----|----|---|--------|--------|
| P4.1 | ∅ | ∅ | ∅ | ∅ | omit | never existed |
| P4.2 | ∅ | P(v) | ∅ | ∅ | **v** | only I1 added → v |
| P4.3 | ∅ | ∅ | P(v) | ∅ | **v** | only I2 added → v |
| P4.4 | ∅ | ∅ | ∅ | P(v) | **v** | only W added → v |
| P4.5 | ∅ | P(v) | P(w) | ∅ | **w** | two adds differ, W absent → highest (I2) wins |
| P4.6 | ∅ | P(v) | ∅ | P(w) | **w** | I1 & W add, differ → W wins |
| P4.7 | P(v) | ∅ | ∅ | ∅ | **omit** | all removed → omit |
| P4.8 | P(v) | ∅ | P(v) | P(v) | **omit** | I1 removed, others==Base → removal (highest changer I1) stands |
| P4.9 | P(v) | P(v) | ∅ | P(v) | **omit** | I2 removed, W==Base → removal stands |
| P4.10 | P(v) | ∅ | ∅ | P(w) | **w** | removals below, W changed value → W wins |
| P4.11 | P(v) | P(w) | ∅ | ∅ | **omit** | I1 changed value, I2 & W removed → W (removal) wins |

General rule (any arity): scan versions highest-priority → Base; the first version whose **presence-or-value** differs from Base is the result (its value if present, omit if it removed); if none differ, keep Base.

#### Notes

- A version absent for a unit that Base also lacks (and this version doesn't introduce) is not a changer — it contributes nothing.
- Removal by an intermediate with an unchanged winner is kept (P3.6, P4.8/P4.9), per merge-removal-is-intentional. The merged patch omits the unit.
- **FRMR/reference exception:** for placed references, mere absence is NOT a removal — only an explicit `DELE` (or the moved-ref mechanism) removes. See frmr-reference-model. This overrides the removal rows above for FRMR only.
- Repeatable/keyed units (leveled entries, NPCO/NPCS/FACT, ENAM slots) apply these presence tables per key, not per record; union of keys with per-key resolution.

### Conflict-detection semantics — `skip_non_existent`

Separately from the merge, conflict **status/coloring** can ignore absent columns. When a sub-record rule (or the record's wildcard rule) carries `skip_non_existent`, `compute_conflict` writes `non_existent_value` for absent/deleted columns and uses `compute_conflict_all_skip_empty` / `compute_conflict_this_skip_empty`, which drop those columns before comparing:

- All absent, or exactly one present → `only_one` (no conflict shown).
- All present values equal → `no_conflict`.
- Present values differ → `conflict` / `override_benign` per the normal rule, computed over present columns only.

This only affects how the conflict is displayed; it does not change the merge result, which always follows the merge table above (including treating absence as add/remove). A record type without `skip_non_existent` compares absent columns as the literal sentinel, so an add/remove shows as a difference.

## Where this is implemented

- Predicate: `intermediate_claims_bit` / `intermediate_claims_span` in `sub_record_merge.cpp`.
- Header FLAGS: `record_header_flags::merge_flags` in `decoder/record_header_flags.cpp`.
- The 2-version case must be reachable: `auto_merge_t::should_skip_group` must not skip non-leveled 2-version groups, and `sub_record_merge_t::merge_generic` must resolve 2 versions as "winner wins if it differs from Base" rather than returning unchanged.
