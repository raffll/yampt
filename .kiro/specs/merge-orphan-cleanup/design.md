# Design — Remove Orphaned Merge Records When a Source Mod Leaves the Load Order (yEditor)

## Context (current mechanics)

- `merge_patch_store_t`: flat `vector<merge_record_t{rec_type,record_id,content,pinned}>`, keyed by (rec_type,record_id). No source-plugin provenance.
- Load-order change: `plugin_session_t` resets `m_scan = plugin_scan_t()` and reloads; existing `Merged Patch.esp` seeded into the store via `set_merge_plugin_from_loaded` (all `pinned=false`).
- `rebuild_conflicts()` projects store records as merge-plugin versions but does not prune the store. No reconciliation against the current plugin set anywhere.
- `collect_contributing_plugins()` (merge_controller.cpp): for each conflict entry with a merge version, adds every non-merge plugin index that also has a version → derived live. `build_master_list` builds the MAST list from those, recomputed per save.
- Auto-merge (`auto_merge_t`) builds groups as `record_group_t { rec_type, record_id, versions[{plugin_idx, record_index}] }` — the contributing plugins for each merged record are known at merge time but not stored on the merge record.

## Design Goals

Give merge records provenance (which source plugins they came from), persisted in the session (R1); on load reconcile provenance against the loaded plugin set to find orphans (R2); remove fully-orphaned auto records, keep partially-orphaned, and surface manual orphans for confirmation (R3); report everything (R4); keep the master list consistent (R5); change nothing in the `.esp` format or auto output (R6). Respect architecture rules (core Qt-free; reconciliation logic pure/testable; ≤50-line functions).

## Decision: provenance keyed by (rec_type, record_id) → set of source filenames

Rather than a `merge_record_t` field (which would need to survive the `.esp` round-trip and it cannot), store provenance as a parallel structure keyed by the same (rec_type,record_id) the store uses, holding a set of contributing source **filenames**:

```cpp
// yampt.core, alongside the merge store (e.g. merge_provenance_t or a member of plugin_scan_t)
using merge_key_t = std::pair<std::string, std::string>;           // (rec_type, record_id)
std::map<merge_key_t, std::set<std::string>> m_merge_provenance;   // key -> contributing source filenames
```

Filenames (not indices) so it is stable across reloads/reordering (R2.3). Populated:
- **Auto-merge:** when `auto_merge_t` writes a merged record, it knows the `record_group_t::versions` → their plugin filenames. Record them as that key's provenance. (auto_merge writes go through `plugin_scan_t`, so `plugin_scan_t` exposes a `set_merge_provenance(key, filenames)` that auto-merge calls alongside `copy_record_to_merge_raw`.)
- **Manual copy:** `merge_controller_t::copy_*` know the `plugin_idx` copied from → its filename. Record it as the key's provenance (single-element set, or union if patched from several).

Provenance is persisted in the session (Component 3) because the store is rebuilt from the `.esp` on load, which carries no provenance.

### Rejected alternative

A heuristic that reconciles store records against current conflict entries without stored provenance (e.g. "if no loaded plugin has this record_id, it's orphaned"). Rejected: it cannot distinguish a record the merge *created/derived* from removed content vs. a record whose id legitimately exists only in the patch; and it would misfire for records intentionally unique to the patch. Explicit provenance is the correct, precise basis.

## Component 1 — Provenance recording (core + editor)

- `plugin_scan_t`: add `m_merge_provenance` and `set_merge_provenance(rec_type, record_id, contributors)` / `merge_provenance(key)` / `clear_merge_provenance()`. `clear_merge_records` also clears provenance for consistency (auto-merge repopulates it).
- `auto_merge_t`: in `process_leveled_list` / `process_dialogue` / `process_three_way`, after writing the merged record, call `set_merge_provenance` with the group's version plugin filenames.
- `merge_controller_t::copy_*`: after landing the manual copy, call `set_merge_provenance` with the source plugin filename (and union for group/field patches that pulled from that plugin). This composes with the `merge-persist-manual-records` pinning (same call sites).

## Component 2 — Reconciliation pass (core, pure)

A pure function so it is unit-testable without a live session:

```cpp
// yampt.core
struct orphan_report_t
{
    std::vector<merge_key_t> fully_orphaned_auto;
    std::vector<merge_key_t> fully_orphaned_manual;
    std::vector<merge_key_t> partially_orphaned;   // informational
    std::vector<merge_key_t> unknown_provenance;
};

orphan_report_t reconcile_merge_orphans(
    const std::vector<merge_record_t> & store_records,
    const std::map<merge_key_t, std::set<std::string>> & provenance,
    const std::set<std::string> & loaded_filenames);   // case-insensitive compare
```

For each store record:
- provenance missing → `unknown_provenance` (kept, R3.4).
- all contributors ∉ loaded → fully orphaned; bucket by `pinned` (manual vs auto).
- some contributors gone, some present → `partially_orphaned` (kept, R3.2).
- all present → fine.

Filename comparison uses `string_utils` case-insensitive equality (path rules). The function returns a report; it does NOT mutate — the caller decides what to remove (keeps the pure/testable boundary).

## Component 3 — Session persistence of provenance (editor + core)

The `.esp` cannot carry provenance, so persist it in the session, sharing storage with the `merge-persist-manual-records` manual-key list (that spec already adds a per-key session list). Extend it: `plugin_session_t` persists, per manual/auto merge record, its (rec_type, record_id) → contributing filenames. Practically:

- `plugin_session_t` gains `std::map<merge_key_t, std::set<std::string>> m_merge_provenance` with save/restore under `merge/provenance` (a serialized list; format defined in implementation — e.g. one line per key `rec_type|record_id|file1;file2`).
- On copy/auto-merge, provenance is mirrored into the session and persisted.
- On load, after the `.esp` seeds the store, provenance is restored from the session into `plugin_scan_t::m_merge_provenance` before reconciliation runs.

Because this overlaps `merge-persist-manual-records`'s session additions, the two specs share one session-side provenance structure (manual-key list becomes "the provenance map, with pinned tracked separately or inferred"). The design coordinates: implement the provenance map once; `merge-persist-manual-records` uses it for pinning, this spec uses it for orphan detection. (Open Decision: sequencing/ownership of the shared session storage.)

## Component 4 — Applying the cleanup (editor)

After load (in the merge init path, e.g. `merge_controller_t::load_existing_merged_patch` or right after `plugin_session_t` finishes loading and seeding), run reconciliation and act:

1. `reconcile_merge_orphans(store.records(), provenance, loaded_filenames)`.
2. Remove `fully_orphaned_auto` keys from the store (`remove_from_merge`) — these are safe to drop (R3.1).
3. For `fully_orphaned_manual`: show a dialog listing them (rec_type + id + which removed mod), let the user confirm removal or keep (R3.3). Remove the confirmed ones.
4. `partially_orphaned` and `unknown_provenance`: keep; include in the log/report (R3.2, R3.4).
5. Log a summary (counts by type) and, if anything was removed, an optional list dialog (R4.1), modeled on existing yEditor list dialogs.
6. `rebuild_conflicts()`; if the store changed, mark the patch to be saved on the next save (or save immediately — Open Decision R4.2). Default: reconcile in the store, let the existing save flow persist (do not force a destructive write without the user seeing the result).

Master-list consistency (R5) follows automatically: `build_master_list` is recomputed from current conflict entries at save, and after removing orphaned records no surviving record depends on an absent master.

## Data Flow

Load-order change → reload → `.esp` seeds store (non-pinned) → session restores provenance (and manual pins from `merge-persist-manual-records`) → `reconcile_merge_orphans` → remove fully-orphaned auto; confirm+remove manual orphans; keep partial/unknown → log/dialog report → `rebuild_conflicts` → save via normal flow. Auto/manual writes keep provenance current going forward.

## Error Handling

- No provenance for a record (external `.esp`) → unknown, kept, reported (R3.4).
- Reordering only → all contributors still loaded → no orphans (R6.2).
- A contributor filename that differs only by case/path form → matched via case-insensitive path compare (not treated as removed).

## Testing Strategy (R7)

Pure `[u]` (in-memory, no disk) — reconciliation is a pure function:
- fully-orphaned auto record (all contributors absent) → in `fully_orphaned_auto`.
- record with one loaded contributor → not orphaned.
- pinned/manual orphan → in `fully_orphaned_manual`, not auto.
- no-provenance record → `unknown_provenance`.
- reorder loaded plugins → empty report (stable).
- partial orphan → `partially_orphaned`, kept.
- provenance session round-trip (`[u]` if in `plugin_session_t` serialization; the map save/restore).

`[i]`/manual for the full load→remove→reload→dropped-from-`.esp` path. Building/running tests is manual (no-build-or-test rule). Names: `owner::member, description`, e.g. `"reconcile_merge_orphans, fully orphaned auto record removed"`, `"reconcile_merge_orphans, reordering keeps all records"`.

## Files Touched

| File | Change |
|------|--------|
| `yampt.core/source/scanner/plugin_scan.hpp/.cpp` | `m_merge_provenance` + accessors; clear with `clear_merge_records` |
| `yampt.core/source/scanner/merge_orphans.hpp/.cpp` (new) | pure `reconcile_merge_orphans` + `orphan_report_t` |
| `yampt.core/source/scanner/auto_merge.cpp` | record provenance per merged record |
| `yampt.editor/source/controller/merge_controller.cpp` | record provenance on manual copy; run reconciliation after load; apply removals; report |
| `yampt.editor/source/session/plugin_session.hpp/.cpp` | persist/restore provenance map (`merge/provenance`), shared with merge-persist-manual-records |
| `yampt.editor/source/dialog/*` (new or reuse) | orphan-report / manual-orphan-confirm list dialog |
| `yampt.tests/*` + vcxproj/.filters | `[u]` reconciliation + provenance round-trip tests |

## Documentation

- CHANGELOG `[NEW]` (yEditor): removing a mod from the load order now drops the merged-patch records that came only from that mod (reported to you); records still backed by a loaded mod are kept.
- `docs/yEditor-Manual.md`: describe that the merged patch is reconciled against the load order on load — orphaned automatic records are removed and reported, manual copies from a removed mod are listed for you to confirm.
- README + README.bbcode in sync if the merged-patch workflow is described. No internal detail (provenance map) in user docs.
