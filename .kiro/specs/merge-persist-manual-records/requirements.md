# Requirements — Persist Manually-Copied Records Across Merge Regeneration (yEditor)

## Background — Current Behavior

yEditor's merged patch is an in-memory `merge_patch_store_t` (`plugin_scan_t::m_merge_store`) written to `Merged Patch.esp`.

- `merge_record_t { std::string rec_type; std::string record_id; std::string content; bool pinned = false; }`, held in a flat vector keyed by (rec_type, record_id).
- Store mutators: `add`/`add_pinned`, `update_or_add` (pinned=false), `update_or_add_pinned` (pinned=true), `remove`, `is_pinned`, `find_content`, `collect_pinned`, `restore_pinned`.
- **Automatic merge** — `auto_merge_t::execute()` begins with `m_scan.clear_merge_records()` (wipes the whole store), then rebuilds it from scratch by re-scanning all included plugins and dispatching (leveled lists, dialogues, three-way). Every record it writes goes through `copy_record_to_merge_raw` → `update_or_add` → **pinned = false**.
- **Manual copies** — every context-menu "Copy … to Merged Patch" action (`merge_controller_t::copy_whole_record` / `copy_cell_record` / `copy_sub_record` / `copy_group` / `copy_field`, plus `ensure_merge_record`) also ends in `copy_record_to_merge_raw` → `update_or_add` → **pinned = false**.
- **Regeneration** — `merge_controller_t::create_merged_patch()` (toolbar "Create merged patch"): if the store already has records, it warns "This will regenerate the merged patch and discard manual changes. Continue?", then `create_merge_records()`:
  1. `auto pinned_records = m_session.scan().collect_pinned_records();` — snapshots only `pinned == true` records.
  2. `auto_merge_t::execute()` — clears the store and rebuilds.
  3. `m_session.scan().restore_pinned_records(pinned_records);` — re-inserts the pinned snapshot.
- The only code that ever sets `pinned = true` is `field_edit_controller_t`, and only to **re-pin a record that was already pinned** (`if is_merge_pinned → pin_record_to_merge else → copy_record_to_merge_raw`). No user action ever initiates a pin.

The store carries no other provenance: no source-plugin tag, no auto-vs-manual marker beyond `pinned`.

## Problem

There is already a save-around-regenerate mechanism (`collect_pinned_records` / `restore_pinned_records`) intended to protect user-curated merge records. But **nothing user-facing ever pins a record**, so every manually copied record is `pinned = false` and is wiped by `clear_merge_records()` during regeneration. The "discard manual changes" warning is accurate: rebuilding the merged patch after auto-merge throws away everything the user copied in by hand. The protection mechanism exists but is inert.

The user's expectation (and the natural one): records I copied into the patch by hand should survive when I regenerate — the auto-merge should recompute the automatic records and leave my manual additions in place.

## Goal

Make manually-copied merge records survive regeneration. Manual copies are marked as user-curated (pinned) at the moment they are copied, so the existing `collect_pinned_records` / `restore_pinned_records` save-around actually protects them. After a regenerate, the auto-merged records are recomputed fresh and the user's manual records are restored on top.

## User-Facing Outcomes

- Copying a record / sub-record / group / field into the merged patch marks it as user-curated.
- Clicking "Create merged patch" again (regenerate) recomputes the automatic merge but **keeps** the records the user copied in manually. The "discard manual changes" warning is removed or reworded, because manual changes are no longer discarded.
- A manual record that the auto-merge would also produce is kept as the user's version (the manual copy wins), not silently replaced by the auto version.
- Removing a manual record from the merged patch ("Remove from Merged Patch") still removes it, and it does not reappear on regenerate.
- The behavior is visible/consistent: the user can regenerate freely without losing hand-assembled edits.

## Requirements

### R1 — Manual copies are marked user-curated

1.1 Every manual "Copy … to Merged Patch" operation marks the resulting store record as pinned (user-curated), so it is captured by `collect_pinned_records`. This covers `copy_whole_record`, `copy_cell_record`, `copy_sub_record`, `copy_group`, `copy_field`, and the `ensure_merge_record` seeding step.
1.2 The mechanism is the existing pinned flag / `pin_record_to_merge` (→ `update_or_add_pinned`), or an equivalent user-curated marker. The design decides whether to reuse `pinned` directly or introduce a clearer `manual` provenance field; default is to reuse `pinned` since the save-around already keys on it (least churn), unless a separate concept is needed to keep pin semantics distinct.
1.3 Sub-record/group/field copies that patch onto an existing store record keep that record marked user-curated after the patch (the record remains pinned; patching does not clear the flag).

### R2 — Automatic-merge records stay non-curated

2.1 `auto_merge_t` continues to write its records as non-pinned (not user-curated), so they are the ones cleared and recomputed on each regenerate. No change to auto-merge's write path.
2.2 A record produced by auto-merge is transient across regenerates; a record copied by the user is durable across regenerates.

### R3 — Regeneration preserves manual records

3.1 `create_merge_records()` continues to `collect_pinned_records()` before and `restore_pinned_records()` after `auto_merge_t::execute()`. With R1 in place, the collected set now actually contains the user's manual records, so they survive.
3.2 When a manual (pinned) record and an auto-merged record share the same (rec_type, record_id), the restore step must ensure the **manual record wins** (restore overwrites the auto version). `restore_pinned_records` currently replaces the content of an existing store entry and sets it pinned, or pushes if absent — confirm this yields "manual wins" and, if the auto version was pruned by `prune_unchanged`, that the manual record is still restored (pushed). The design verifies the collect/restore ordering against `prune_unchanged`.
3.3 `prune_unchanged` (which drops a store record whose content equals the winning plugin version) must not drop a user-curated record even if its content happens to equal a source version — a manual copy that duplicates a source is still the user's explicit choice. The design decides whether `prune_unchanged` skips pinned records (recommended) or whether restore-after-prune already covers it.

### R4 — Removal and the warning

4.1 "Remove from Merged Patch" removes the record from the store (including its pinned flag) so it is neither present nor restored on the next regenerate. Unchanged behavior, but now meaningful since the record was pinned.
4.2 The "This will regenerate the merged patch and discard manual changes" confirmation is removed or reworded (e.g. "Regenerate the automatic merge? Your manually copied records are kept."), because manual changes are no longer discarded. If any edge case can still lose data, the design keeps a truthful warning scoped to that case.

### R5 — Persistence across sessions

5.1 Manual records already persist across sessions via the on-disk `Merged Patch.esp` (the store is seeded from it on load through `set_merge_plugin_from_loaded`). However, that seeding marks all seeded records `pinned = false`. On reload, a previously-manual record loses its user-curated marking and would again be wiped by the next regenerate. The design addresses this: either (a) re-mark records as user-curated on load is impossible without provenance, so (b) persist the set of manual record keys somewhere (e.g. a sidecar or session INI list of manual (rec_type,record_id) keys), or (c) accept that after reload the user must re-copy — the design picks the cleanest that actually preserves the guarantee across sessions. Default direction: persist the manual-record keys in the session (`plugin_session_t`) alongside excluded/patch lists, and re-apply the user-curated flag to matching store records after the merged patch is loaded.
5.2 If a persisted manual key no longer matches any store record on load (the record was removed from the .esp externally), it is silently dropped.

### R6 — No regression

6.1 Auto-merge output for a fresh patch (no manual records) is byte-for-byte unchanged.
6.2 The store API, keying, and save path (`save_merge_to_file`, master list) are unchanged except for provenance marking.
6.3 `field_edit_controller_t`'s existing re-pin behavior continues to work (a pinned record stays pinned through an edit).
6.4 The merged patch file format (`Merged Patch.esp`) is unchanged — provenance lives in memory/session, not in the .esp.

### R7 — Verification

7.1 Pure `[u]` tests (in-memory `plugin_scan_t` / `merge_patch_store_t`, no disk): after a manual copy, the record is pinned; `collect_pinned_records` returns it; simulating regenerate (`clear_merge_records` + re-add auto records + `restore_pinned_records`) leaves the manual record present with the manual content; an auto record sharing the key is overridden by the manual one; `prune_unchanged` does not drop a pinned record whose content equals a source version.
7.2 `[u]`: removing a manual record clears it so a subsequent collect/restore does not bring it back.
7.3 Session persistence (R5) is validated at whatever level the design places it (`[u]` for the key set round-trip if in `plugin_session_t`; `[i]` if it touches the .esp on disk).
7.4 Manual verification: copy several records into the merged patch, click Create merged patch again, confirm the manual records remain and the auto records are recomputed.

## Open Decisions

Resolved:
- Reuse the existing `collect_pinned_records` / `restore_pinned_records` save-around; the fix is to actually mark manual copies as user-curated so they are collected. (R1, R3)
- Auto-merge records stay non-curated and are recomputed each regenerate. (R2)
- Manual record wins over an auto record with the same key on restore. (R3.2)

Deferred to design:
- Reuse the `pinned` bool directly vs. introduce a distinct `manual`/user-curated provenance field (R1.2).
- Whether `prune_unchanged` skips pinned records or restore-after-prune already covers it (R3.3).
- Cross-session preservation (R5): persist manual-record keys in `plugin_session_t` and re-apply on load, vs. accept re-copy after reload. Default: persist keys in the session and re-mark on load.
- Exact rewording/removal of the regenerate confirmation (R4.2).
