# Requirements — Remove Orphaned Merge Records When a Source Mod Leaves the Load Order (yEditor)

## Background — Current Behavior

The merged patch is an in-memory `merge_patch_store_t` written to `Merged Patch.esp`. Its records carry no provenance beyond a `pinned` bool — there is no record of which source plugin a merge record was derived from.

- On a load-order change, `plugin_session_t` fully reloads: it resets the scan (`m_scan = plugin_scan_t()`) and reloads from the current path list. An existing `Merged Patch.esp` is read back verbatim into the store via `set_merge_plugin_from_loaded(idx)`, which seeds the store from the file's records.
- `rebuild_conflicts()` re-derives conflict entries from the current plugins plus whatever is in the store. It projects each store record as a merge-plugin version of a conflict entry but does **not** prune the store.
- There is **no reconciliation** of the store against the current plugin set. A merge record derived from a mod that is no longer loaded persists as a stale entry in the store and in the saved `Merged Patch.esp`.
- `collect_contributing_plugins()` / `build_master_list()` are recomputed live per save from the current conflict entries, so a removed plugin drops out of the patch's MAST master list on the next save — but the stale record content itself is not removed. This can leave the patch with a record whose master is no longer listed (a record referencing content/IDs from a mod that is gone).
- The only ways a store record is dropped today: explicit "Remove from Merged Patch", a full regenerate (clear + rebuild), or `prune_unchanged` during regenerate.

## Problem

When a user removes a mod from their load order, the merged patch keeps records that were built from that mod. These orphaned records reference a plugin that is no longer present: at best they carry stale overrides, at worst they reference records/IDs from the removed mod that no longer make sense, and the patch's master list may no longer include the mod the record came from. The user has to remember to regenerate or manually hunt down and remove these. The tool should detect that a source mod is gone and drop the merge records that depended on it.

## Problem Constraint — no provenance today

The store does not track which plugin a merge record came from. "This record is orphaned because mod X was removed" cannot be answered from the current store alone. Solving this requires either (a) recording per-merge-record provenance (which source plugins contributed to it) so orphaning can be detected precisely, or (b) a heuristic reconciliation against the current conflict entries. This is the central design question.

## Goal

When the load order changes such that a plugin that contributed to a merge record is no longer loaded, the affected merge records are removed (or flagged for the user), so the merged patch does not retain stale entries derived from mods that are gone. This must not remove records that are still valid (still have a live source), and must not silently destroy user-curated manual copies without the user's awareness.

## User-Facing Outcomes

- After removing a mod from the load order and reloading, merge records that depended solely on the removed mod are dropped from the merged patch. The saved `Merged Patch.esp` no longer contains them.
- Records that still have at least one loaded contributor are kept (and, on the next regenerate, recomputed from the remaining contributors).
- The user is informed what was removed (a log summary and/or a dialog listing the dropped records), so the cleanup is visible, not silent.
- Manually copied (user-curated) records that reference a removed mod are handled deliberately: the design decides whether they are auto-removed like auto records or surfaced for confirmation (since they represent explicit user choices). Default: surface manual orphans for the user rather than silently deleting them.
- Cleanup is safe: nothing is written to disk destructively without the normal save flow; the user can see the result before it persists (subject to the design's chosen timing).

## Requirements

### R1 — Provenance for merge records

1.1 Each merge record gains provenance: the set of source plugins that contributed to it (by filename, so it survives index reshuffles across reloads). Auto-merged records record their contributing plugins (available from the merge group / conflict entry). Manually copied records record the plugin they were copied from.
1.2 Provenance is stored in memory (`merge_record_t` / a parallel structure) and, because it cannot live in the `.esp`, persisted in the session (like the manual-record keys) so it survives reload. If provenance for a seeded record is unknown after loading an externally-produced `Merged Patch.esp`, the record is treated as "unknown provenance" (see R3.4).
1.3 Provenance is additive to the existing store; it does not change the `.esp` format (R6).

### R2 — Detecting orphaned records

2.1 On load / load-order change, after plugins are loaded and the merged patch is seeded, reconcile: for each merge record, compare its recorded contributing plugins against the currently loaded plugin set.
2.2 A record is **fully orphaned** when none of its contributing plugins are loaded. A record is **partially orphaned** when some but not all contributors are gone.
2.3 The reconciliation keys plugins by filename (case-insensitive per the path rules) so it is robust to load-order reordering (reordering is not removal).

### R3 — Acting on orphans

3.1 Fully-orphaned **auto** records are removed from the store.
3.2 Partially-orphaned auto records are kept (they still have a live contributor); they will be recomputed correctly on the next regenerate from the remaining contributors. No immediate content change is required (the design may leave them until regenerate, or recompute eagerly — default: keep and let regenerate handle it).
3.3 Fully-orphaned **manual** (user-curated) records are surfaced to the user rather than silently removed (they are explicit choices). The user confirms removal, or keeps them. Default per Goal: list them and let the user decide; if the design prefers auto-removal for consistency, it must justify and still report them.
3.4 Records of unknown provenance (seeded from an external `.esp` with no session provenance) are treated conservatively: not auto-removed (we cannot prove they are orphaned). They may be reported as "provenance unknown" so the user can regenerate to rebuild provenance.

### R4 — Visibility

4.1 The cleanup reports what it did: a log summary (count of removed records by type) and, when records are removed, an optional dialog listing them (rec_type + id + which removed mod they came from), modeled on existing yEditor list dialogs. Manual orphans are listed for confirmation per R3.3.
4.2 Nothing is removed silently; the user can see the outcome before it is persisted to `Merged Patch.esp` (the removal happens in the store, and the normal save writes it — the design defines whether reconciliation triggers an immediate save or waits for the next save/regenerate).

### R5 — Master list consistency

5.1 After orphan removal, the patch's MAST master list (recomputed live from current conflict entries by `build_master_list`) no longer references removed mods, and it matches the surviving record set — no record remains whose only contributor is absent from the masters.

### R6 — No regression

6.1 The `Merged Patch.esp` format is unchanged; provenance lives in memory + session, not the file.
6.2 With no load-order change (or only reordering), no records are removed.
6.3 Auto-merge output, the store API, and the save path are unchanged except for provenance recording and the reconciliation pass.
6.4 This composes with the manual-record persistence feature (`merge-persist-manual-records`): manual records stay pinned/persisted; orphan cleanup only removes them via the R3.3 confirmed path.

### R7 — Verification

7.1 Pure `[u]` tests (in-memory scan/store, no disk): a merge record whose only contributor is not in the loaded set is detected as fully orphaned and removed; a record with a still-loaded contributor is kept; a manual (pinned) orphan is flagged, not auto-removed; unknown-provenance records are kept; reconciliation is stable under plugin reordering (no removals).
7.2 Provenance recording: an auto-merged record records its contributing plugins; a manual copy records its source plugin; the provenance round-trips through the session persistence.
7.3 `[i]`/manual: load a set with a mod contributing a merge record, remove that mod, reload → the record is dropped from the store and the saved patch; the log/dialog reports it.

## Open Decisions

Resolved:
- Requires per-record provenance (contributing source plugins by filename), persisted in the session since the `.esp` cannot carry it. (R1)
- Fully-orphaned auto records are removed; partially-orphaned kept (recomputed on regenerate). (R3.1, R3.2)
- Cleanup is visible (log + optional dialog), never silent. (R4)
- Reordering is not removal (filename-keyed reconciliation). (R2.3)

Deferred to design:
- Whether fully-orphaned **manual** records are auto-removed (with report) or held for user confirmation (default: confirm). (R3.3)
- Whether reconciliation triggers an immediate save of the cleaned patch or defers to the next save/regenerate (R4.2).
- Whether partially-orphaned auto records are recomputed eagerly or left until the next regenerate (R3.2).
- Exact provenance storage shape (`merge_record_t` field vs. a parallel map keyed by (rec_type,record_id)) and its session-INI serialization format.
- Handling of unknown-provenance records seeded from an external `.esp`: report-only vs. offer "regenerate to rebuild provenance" (R3.4).
- Dependency/order with `merge-persist-manual-records` (shared session provenance/manual-key storage) — the two specs touch the same session persistence and should share it.
