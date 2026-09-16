# FRMR Is Not a Group — It Is a Reference Keyed by RefNum

FRMR (a placed-object reference inside a CELL) does NOT behave like the other grouped sub-record structures in this codebase (leveled-list entries, keyed lists NPCO/NPCS/FACT, ARMO body parts). Do not model it as one. Its identity, override, and deletion rules come straight from how the Morrowind engine and OpenMW actually load cell references, and they are different from every other group.

This file records that model. It is derived from OpenMW's own loader at `C:\S\openmw`:
- `components/esm3/loadcell.cpp` — `Cell::getNextRef`, `adjustRefNum`, `NAM0` handling, `saveTempMarker`.
- `components/esm3/cellref.cpp` / `cellref.hpp` — `CellRef::load` / `save`, the full sub-record set of one reference.
- `components/esm/formid.hpp` — `FormId` (`RefNum`) layout.
- `apps/openmw/mwworld/cellstore.cpp` — `CellStore::loadRef` / `loadRefs`, the cross-plugin replay.

## The Reference Identity Is RefNum, Not the FRMR Integer

The 4 bytes stored in an `FRMR` sub-record are a raw `uint32`. OpenMW splits it (`FormId::fromUint32`): the **low 24 bits are the object index**, the **high 8 bits are a content-file (plugin) index**. Together they form a `RefNum { mIndex, mContentFile }`.

- `adjustRefNum` (loadcell.cpp): if the high 8 bits name one of the current plugin's parent (master) files, the high byte is stripped and the ref is understood as an **override of an existing reference already introduced by that master**. Otherwise the high byte is 0 (or invalid) and the ref is a **new reference added by the current plugin**, tagged with the current plugin's index.
- Consequence: the same raw FRMR integer in two different plugins can denote two different objects, and an override of a master's reference is explicitly encoded in the high byte. **Keying a reference by the bare object index (low bits) alone is wrong** — the identity is the full `RefNum` (object index + owning content file).

So when yampt reasons about "the same placed object across plugin versions", the key is the RefNum, reconstructed the way `adjustRefNum` does it, not `read_frmr_index`'s raw value taken at face value.

### The Low Index Alone Collapses Unrelated Objects

Do not key a reference on the raw 24-bit index alone: multiple plugins each ADD a new reference and independently reuse the same low index (e.g. local index 5), so the low index collides across plugins for objects that are not the same object at all. A real case: `green_1000_01`, `ingred_fire_petal_01`, and `light_com_candle_07` all carry local index 5 but were added by three different plugins, so they are three distinct references — masking to the low index wrongly aligns them into one group.

### Reconstruct the RefNum With The Master List — It Is Available

The full `adjustRefNum` reconstruction needs each version's master list and its own load index. That context IS available in `plugin_scan_t`: each loaded plugin has its parsed `MAST` list (`parse_master_list`) and its load index, so `plugin_scan_t::resolve_frmr(plugin_idx, raw_frmr)` reproduces `adjustRefNum` and returns a stable 64-bit identity `(resolved_plugin_idx << 32) | ref_index`. High byte 0 → the containing plugin owns a new ref; high byte = a local master index → resolve that master by filename to its global load index (an override of that master's ref). This is the authoritative identity — use it, do not approximate.

The conflict comparator uses exactly this: `plugin_scan_t::build_slot_result` precomputes, per version, one resolved identity per FRMR (in parse order) via `resolve_frmr`, and passes them into `conflict_slots::build` as `slot_result_t::ref_identities`. `build_cell` / `extract_cell_refs` key each reference group by that resolved identity, NOT by the masked index. A master ref and a plugin's override of it resolve to the same identity and align; two plugins independently adding a new ref at the same low index resolve to different identities and stay separate. There is no masked-index fallback on this path — a missing identity is a caller bug that logs `[error]`, it is not silently masked.

The pure `sub_record_merge_t` merge path also keys by the resolved identity, not by a masked index or a NAME heuristic. `merge_input_t` carries `ref_identities` (one resolved RefNum per FRMR per version, in parse order); `auto_merge_t::process_three_way` fills it for CELL via `plugin_scan_t::cell_ref_identities` (same `resolve_frmr` the comparator uses), and mirrors the patch-priority winner swap onto the identities so content and identity stay aligned. `merge_frmr_groups_phase` builds a per-version `identity -> group` map by zipping each version's `partition_cell` groups with its identity list, and matches master/winner/intermediate groups by identity. There is no object-`NAME` guard and no raw-index key on this path anymore — identity is authoritative, exactly as in the comparator. OpenMW's own `CellStore::loadRef` (cellstore.cpp) treating "same RefNum, different `mRefID`" as not-the-same-object is the same principle, expressed through the RefNum key rather than a separate NAME check.

The yEditor record-view display path keys FRMR grouping the same way. `view_tree_decode_cell.cpp` used to group references per column by the masked low-24 index (`read_frmr_ref_index`), which collapsed unrelated refs — `green_1000_01`, `ingred_fire_petal_01`, `light_com_candle_07` all at local index 5 rendered as one row. It now groups by a composite `ref_key_t { uint64_t refnum; std::string object_id }` (declared in `sub_record_iter.hpp`): `refnum` is the resolved identity threaded in from `slot_result_t::ref_identities` (the same per-version, per-FRMR-in-parse-order data the comparator computes via `resolve_frmr`), and `object_id` is the reference's `NAME` sub-record. Two refs align only when both match — so a master ref and its override share one row, while distinct objects reusing a low index stay separate. `cell_ref_view_t` still carries the masked `object_index`, but only to render the `#<index>` label number; it is never the grouping key. A missing identity for an FRMR ordinal logs `[error]` (caller bug), it is not silently masked.

## The Merge Unit Is the Whole Reference, Last-Writer-Wins

OpenMW does NOT field-merge references. In `CellStore::loadRef` (cellstore.cpp) references are replayed in content-file (load) order into a map keyed by `RefNum`; a later plugin carrying the same `RefNum` **overwrites the entire earlier `CellRef`**. There is no per-field three-way merge of ANAM, lock level, position, or anything else — the whole reference from the last writer wins.

- A reference is a flat `CellRef` whose optional sub-records are: `NAM0`(temp marker, skipped), `NAME`(id), `XSCL`, `ANAM`(owner), `BNAM`(global var), `XSOL`(soul), `CNAM`(faction), `INDX`(faction rank), `XCHG`, `INTV`(charge/uses), `NAM9`(count), `DODT`+`DNAM`(teleport), `FLTV`(lock), `KNAM`(key), `TNAM`(trap), `UNAM`(blocked), `DATA`(position). This whole set is one unit.
- There is therefore no engine concept of "merge only ANAM and copy the rest from another plugin." Any yampt feature that merges a single reference field (e.g. ANAM owner) is a yampt-level convenience, not something the engine does — so it must be built as: take one plugin's **whole** reference as the base and substitute the one field, never a genuine field-level three-way. The emitted reference must remain a valid, complete `CellRef`.

## Absence Is Not Deletion

A plugin simply not containing a reference means nothing — the reference is unchanged, inherited from whoever last wrote it. Absence must be ignored, never treated as a delete. (This is the general merge-removal rule's one exception carved out for FRMR: for FRMR, only an explicit signal deletes.)

- Explicit deletion of a reference is a `DELE` sub-record inside that reference (`isDeleted` in `getNextRef`/`loadData`), or the moved-reference mechanism (`MVRF`+`CNDT` pairs, tracked separately as leased/moved refs). Those are the only ways a reference goes away.
- Do NOT infer deletion from "present in master, absent in plugin." Do NOT resurrect a reference that carries `DELE`.

## NAM0 Is a Skippable Temp-Ref Counter

`NAM0` is the count of "temp" references and is a performance hint only. OpenMW skips it on load (`loadIdImpl` skips a leading `NAM0`; `Cell::loadCell` reads it into `mRefNumCounter` but `saveTempMarker` only re-emits it when nonzero and it is not required for correctness). yampt does not need to emit `NAM0` when writing merged references — an absent or stale NAM0 does not break OpenMW. (Vanilla is stricter about the counter, but yampt targets OpenMW output here.)

Because `NAM0` is a meaningless per-plugin count, it must not count toward conflict detection either: a CELL that differs only in `NAM0` is not a conflict or an override. This is table-driven, not an inline type check. The CELL `NAM0` row in `cell_sub_rules` (`record_behavior.cpp`) carries `sub_rule_flag_t::ignore_conflict` alongside `skip_emit`; `record_conflict::find_conflict_policy` reads that flag (from the matched rule only, never the wildcard, so it stays scoped to `NAM0`) into `conflict_policy_t::ignore_conflict`; and `plugin_scan_t::compute_conflict` skips any aligned slot whose policy has `ignore_conflict`, so the `NAM0` slot never reaches the conflict accumulator or per-version status. The record-view display already omits the `NAM0` row separately.

## Rules For yampt

- Model a FRMR as a reference keyed by its full `RefNum` (low-24 object index + high-8 content-file byte), reconstructed per `adjustRefNum`. Never key by the raw object index alone, and never treat FRMR as one of the keyed-list/group merge families.
- The reference is an atomic unit for engine purposes. When yampt offers a single-field merge (such as merging only the ANAM owner), implement it as "take one plugin's complete reference and swap that one field," keeping the rest of that reference intact — not a field-by-field three-way across plugins.
- Never treat a reference's absence in a plugin as a deletion. Honor only explicit `DELE` (and the moved-ref `MVRF` mechanism) as removal.
- Do not emit `NAM0` for merged references.
- FRMR handling stays out of the generic keyed-list/element-wise merge tables. It is its own concern with its own identity and override rule; adding a FRMR feature is not the same as adding a keyed-list type.
