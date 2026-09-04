# Design — Merge the Leveled-List Calc Flag and Chance-None Across Contributors (yEditor)

## Context (current mechanics)

- `auto_merge_t::process_leveled_list` → `leveled_list_merge_t::merge(merge_input_t)` (yampt.core/scanner/sub_record_merge.cpp). `merge_input_t { rec_type, record_id, version_contents }` — `version_contents` holds ALL contributing version record blobs.
- `merge`: builds item-count maps per version (`build_item_count_map(extract_list_items(...))`); an item in the first master but absent from a later version is dropped (`is_item_deleted`); surviving items take max count across later versions (`compute_merged_count`); `sort_merged_items`; then `build_merged_list_record(rec_type, header_part, merged_items)` where `header_part = extract_list_header(winning_content)`.
- `extract_list_header(content)` (sub_record_merge.cpp ~820): iterates sub-records, stops at the first INAM/CNAM/INTV (entry data), skips INDX, and copies every other header sub-record verbatim — including **DATA** and **NNAM** — **from the winning (last) version only**.
- `build_merged_list_record` writes: `rec_type` + size + 8 zero bytes + `header_part` + INDX(count) + per-item (INAM|CNAM + INTV). So DATA/NNAM come solely from `header_part`, i.e. the winning version.
- Flags (from `sub_record_schema.cpp` ~488): LEVI DATA u32 bit0 = "Calc for Each Item", bit1 = "Calc from All Levels"; LEVC DATA u32 bit0 = "Calc from All Levels". NNAM = chance-none (byte percentage).
- `merge` returns `{ changed, content }`; `changed=false` when the rebuilt record equals the winning content (then auto-merge skips writing it).

## Design Goals

OR-merge the DATA calc flags across all contributing versions (R1), apply a defined NNAM policy (R2), leave the rest of the header and the entry-merge untouched (R3), read the flags from every version (R4), scope to LEVI/LEVC auto-merge (R5), and produce byte-identical output when contributors agree (R6). Respect architecture: pure logic in `leveled_list_merge_t`, ≤50-line functions, reuse `sub_record_iter_t`.

## Decision: extract DATA/NNAM per version, OR the flags, apply NNAM policy, splice into the header

The entry merge is already correct and stays. The change is confined to how the output header's DATA (and NNAM) are computed. Instead of taking the winning version's DATA/NNAM verbatim inside `header_part`, compute merged values and substitute them.

### Step 1 — extract per-version DATA and NNAM

Add small helpers (file-local static in sub_record_merge.cpp, reusing `sub_record_iter_t`):

```cpp
static uint32_t extract_leveled_data_flags(const std::string & content);  // 0 if DATA absent
static std::optional<uint8_t> extract_leveled_chance_none(const std::string & content); // nullopt if NNAM absent
```

Run these over every entry in `version_contents`.

### Step 2 — OR the DATA flags

```cpp
uint32_t merged_flags = 0;
for (const auto & content : version_contents)
    merged_flags |= extract_leveled_data_flags(content);
```

OR the whole u32 (R1.2 default): only the low bits are defined and vanilla data has the rest zero; the design confirms during implementation that real data carries no meaningful reserved bits (if a hazard is found, mask to the defined bits per record type). A version without DATA contributes 0 (R1.3).

### Step 3 — NNAM chance-none policy

Default: **minimum** across contributors that have NNAM (additive-union rationale: adding mods/content should not make the list more likely to produce nothing). A version without NNAM does not constrain the minimum (R2.3). If all present values agree, the result is that value (R2.2). If no version has NNAM, omit NNAM (engine treats absent as 0 chance-none).

Note: this NNAM policy is the one genuinely judgemental choice. The requirement flags it as an Open Decision to confirm against engine semantics / a reference tool (TES3Merge). The reference sources are not present in this workspace, so the design proceeds with **minimum** as the documented default and explicitly marks it for confirmation before implementation; if confirmation shows winning-plugin or max is correct, only Step 3 changes (isolated).

### Step 4 — rebuild the header with merged DATA/NNAM

Change `extract_list_header` (or wrap it) so the produced `header_part` uses `merged_flags` for DATA and the policy result for NNAM, while copying all other header sub-records verbatim from the winning version (R3.1). Cleanest shape: `extract_list_header` gains parameters (merged DATA, merged NNAM) and, as it iterates, substitutes the DATA sub-record's payload with `merged_flags` and the NNAM payload with the policy value, emitting all other header sub-records unchanged. If the winning version lacked DATA but a contributor had flags, emit a DATA sub-record with `merged_flags`. Same for NNAM per policy.

### Step 5 — changed gate

Because merged DATA can differ from the winning version's DATA even when entries are identical, `merge` must still report `changed=true` in that case (R5.3). Since `build_merged_list_record` now embeds `merged_flags`/NNAM, the existing `record == winning_content` comparison naturally returns false when the flags differ — so the gate works without special-casing, as long as the comparison is against the true winning content (unchanged). Confirm this holds.

### Rejected alternative

Post-processing the winning record's DATA byte in place after `build_merged_list_record`. Rejected: fragile offset math into a rebuilt blob; computing `merged_flags`/NNAM up front and letting `build_merged_list_record`/`extract_list_header` emit them is cleaner and keeps the record construction in one place.

## Component Changes

| Area | Change |
|------|--------|
| `sub_record_merge.cpp` | `extract_leveled_data_flags`, `extract_leveled_chance_none` (per-version); OR flags + NNAM policy in `leveled_list_merge_t::merge`; `extract_list_header` substitutes merged DATA/NNAM (rest verbatim from winning) |
| (schema) `sub_record_schema.cpp` | none — flag bit definitions already present; referenced for bit positions |

No editor changes: `auto_merge_t::process_leveled_list` already passes all `version_contents`; the fix is internal to the core merge.

## Data Flow

`process_leveled_list` → `leveled_list_merge_t::merge(version_contents)` → entry union (unchanged) + per-version DATA/NNAM extraction → `merged_flags = OR`, `nnam = min` → `build_merged_list_record` with a header that carries merged DATA/NNAM and the winning version's other header sub-records → `{changed, content}`; `changed=true` if entries OR flags differ from winning → `copy_record_to_merge_raw`.

## Error Handling

- Version missing DATA → contributes 0 to the OR; output still has a valid DATA (merged_flags, possibly 0).
- Version missing NNAM → does not constrain the min; if none have NNAM, omit it.
- Malformed sub-record iteration → the existing `sub_record_iter_t` bounds handling applies; a version that fails to parse contributes nothing to the flag OR (treated as 0), consistent with the additive stance.

## Testing Strategy (R7)

Pure `[u]` (synthetic LEVI/LEVC blobs, no disk):
- two versions, different DATA → merged DATA == OR; "Calc from All Levels" from one contributor present.
- LEVI "Calc for Each Item" OR'd in.
- contributor missing DATA → contributes 0; output DATA valid.
- NNAM differing → output == policy (min) result; equal → that value; none present → NNAM omitted.
- all-equal flags → output byte-identical to today; changed gate reports change only when flags differ from winning.
- entry union/deletion/max-count/sort unchanged (regression guard on the existing behavior).

Building/running tests is manual (no-build-or-test rule); write the failing flag-merge test before the change (test-before-fix). Names: `owner::member, description`, e.g. `"leveled_list_merge_t::merge, ORs calc-from-all-levels across versions"`, `"leveled_list_merge_t::merge, identical flags produce no change"`.

## Files Touched

| File | Change |
|------|--------|
| `yampt.core/source/scanner/sub_record_merge.cpp` | flag extraction, OR merge, NNAM policy, header substitution |
| `yampt.tests/*` + vcxproj/.filters | `[u]` leveled-list flag-merge tests |

## Documentation

- CHANGELOG `[FIX]` (yEditor): merged leveled lists now combine the "Calc from All Levels" / "Calc for Each Item" flags from all contributing mods instead of taking them from the last plugin only, so creature and item lists spawn/generate as the combined mods intend. (`[FIX]` — the merge was dropping contributors' flags; now it reflects them.)
- `docs/yEditor-Manual.md`: in the merge description of leveled lists, note that calc flags are combined across contributors (and describe the chance-none policy once confirmed).
- README + README.bbcode in sync if leveled-list merging is described. No internal detail (DATA bits) in user docs.
