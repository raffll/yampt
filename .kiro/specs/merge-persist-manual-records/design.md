# Design — Persist Manually-Copied Records Across Merge Regeneration (yEditor)

## Context (current mechanics)

- `merge_patch_store_t` (yampt.core/scanner/merge_patch_store.cpp): flat `std::vector<merge_record_t{rec_type,record_id,content,bool pinned=false}>`, keyed by (rec_type,record_id) via linear scan. Mutators: `add`, `add_pinned`, `update_or_add` (pinned=false), `update_or_add_pinned` (pinned=true), `remove`, `is_pinned`, `find_content`, `collect_pinned`, `restore_pinned`, `records()`.
- `plugin_scan_t`: `copy_record_to_merge_raw` → `update_or_add` (pinned=false); `pin_record_to_merge` → `update_or_add_pinned` (pinned=true); `collect_pinned_records`/`restore_pinned_records` delegate to the store; `clear_merge_records` → `store.clear()`.
- Auto-merge (`auto_merge_t::execute`): `clear_merge_records()` first, then rebuild via `copy_record_to_merge_raw` (all non-pinned); `prune_unchanged` drops a store record whose content equals the winning source version.
- Manual copies (`merge_controller_t::copy_*`, `ensure_merge_record`): all end in `copy_record_to_merge_raw` (non-pinned).
- Regenerate (`merge_controller_t::create_merged_patch` → `create_merge_records`): `collect_pinned_records()` → `execute()` → `restore_pinned_records()`. Only pinned survive; nothing pins manually → nothing survives.
- Load: `set_merge_plugin_from_loaded(idx)` seeds the store from the on-disk `Merged Patch.esp` records via `store.add(...)` (pinned=false). Session INI (`plugin_session_t`) persists load source/base path and the excluded/patch plugin lists only.

## Design Goals

Make manual copies durable across regeneration by marking them user-curated at copy time (R1), keep auto records transient (R2), let the existing save-around protect them with manual-wins semantics (R3), reword the now-false warning (R4), and preserve the marking across sessions (R5) — all with no change to the `.esp` format or auto-merge output (R6). Respect architecture rules (core stays Qt-free; ≤50-line functions; reuse existing utilities).

## Decision: reuse the `pinned` flag as the "user-curated" marker

The store already has exactly the concept needed: `pinned` means "protected across regenerate" and the save-around keys on it. Rather than add a parallel `manual` field, **reuse `pinned`** and make every manual copy pin. This is the least-churn path and the flag's existing semantics ("survives regenerate") match the requirement precisely.

Concretely: change the terminal step of the manual copy operations from `copy_record_to_merge_raw` (non-pinned) to `pin_record_to_merge` (pinned upsert). Auto-merge keeps calling `copy_record_to_merge_raw` (non-pinned). Result:
- auto records: pinned=false → cleared and recomputed each regenerate;
- manual records: pinned=true → collected before clear, restored after.

### Rejected alternative

A separate `bool manual` on `merge_record_t` plus a second collect/restore path. Rejected: duplicates the exact behavior `pinned` already provides; the only pre-existing `pinned` user is `field_edit_controller_t`'s re-pin, which is itself a "user edited this in the merge" case — i.e. also user-curated, so unifying under `pinned` is semantically correct, not a hack.

## Component 1 — Manual copies pin (yampt.editor)

In `merge_controller_t`, the terminal store write in each manual copy op changes from:

```cpp
m_session.scan().copy_record_to_merge_raw(rec_type, record_id, content);
```

to:

```cpp
m_session.scan().pin_record_to_merge(rec_type, record_id, content);
```

Applies to `copy_whole_record`, `copy_cell_record`, `copy_sub_record`, `copy_group`, `copy_field`. For the sub-record/group/field ops, `ensure_merge_record` seeds a base record if absent — that seed must also be pinned (R1.3), and the subsequent patch write is pinned, so the record stays user-curated through patching. `update_or_add_pinned` already sets `pinned=true` on an existing entry, so patching a record that was already pinned keeps it pinned.

`remove_sub_record` / `remove_group` (which rewrite the merge record after deleting a sub-part) currently call `copy_record_to_merge_raw`; since they operate on a record already in the merge (user-curated), they should also pin (keep it user-curated). The design routes their rewrite through `pin_record_to_merge` too, so a partially-edited manual record does not silently become non-curated.

No change to `auto_merge_t` (stays non-pinned) — satisfies R2.

## Component 2 — Restore semantics: manual wins, prune skips pinned (yampt.core)

`restore_pinned_records` already: for each pinned record, if an entry with the same key exists it overwrites content and sets `pinned=true`, else it pushes. Since restore runs **after** `execute()`, a manual record overwrites any auto record with the same key → **manual wins** (R3.2). No change needed there beyond confirming order.

`prune_unchanged` (in `auto_merge_t`, runs inside `execute()` before restore) drops store records whose content equals the winning source version. At that point the store contains only the freshly-built auto records (the store was cleared at the start of `execute()`); the pinned manual records are NOT yet restored (they're in the local `pinned_records` snapshot held by `create_merge_records`). So `prune_unchanged` cannot drop a manual record during regenerate — they aren't in the store yet. Therefore R3.3 is satisfied by ordering alone during regenerate.

However, `prune_unchanged` could still drop a pinned record in any future path that prunes a store already containing pinned records. To be safe and explicit, add a guard: `prune_unchanged` skips records with `pinned == true`. This is a one-line correctness guard and documents the invariant "pinned records are never pruned". (Low risk, aligns with the "pinned survives" semantics.)

## Component 3 — Reword the regenerate confirmation (yampt.editor)

In `create_merged_patch`, the "discard manual changes" `QMessageBox` is now false for pinned manual records. Reword to reflect the new guarantee:

- Title: `tr("Regenerate Merged Patch")`
- Text: `tr("Recompute the automatic merge? Records you copied in manually are kept.")`
- Buttons: Yes/No as today.

If the design finds any residual case where a manual record can still be lost (it should not, given Component 1–2), the wording is scoped to that case truthfully. Default: the reworded, reassuring text above.

## Component 4 — Cross-session preservation (yampt.editor + core)

On load, `set_merge_plugin_from_loaded` seeds the store from the `.esp` as `pinned=false`, so after a reload the user's manual records look identical to auto records and would be wiped by the next regenerate. The `.esp` format carries no provenance and must not change (R6.4).

Solution: persist the set of manual record keys in the session, re-apply the flag on load.

- `plugin_session_t` gains a manual-key list: `std::vector<std::pair<std::string,std::string>> m_manual_merge_keys;` (rec_type, record_id), with accessors and save/restore in `save_session_state` / `restore_session_state` under a `merge/manual_records` INI value (a delimited list, analogous to `merge/excluded_plugins`). Keys are recorded whenever a manual copy pins a record and removed when the record is removed from the merge.
- On load, after the merged patch is seeded into the store, re-pin every store record whose (rec_type,record_id) is in `m_manual_merge_keys` (call `pin_record_to_merge` with the seeded content, or a lighter `store.set_pinned(key)` helper). Keys with no matching store record are dropped (R5.2).

To keep `merge_controller_t` and `plugin_session_t` in sync, the copy/remove ops that pin/unpin also update the session's manual-key list (add on pin, remove on remove-from-merge), then persist the session (same `save_session_state` call the exclude/guard actions use).

The design may add a tiny `merge_patch_store_t::set_pinned(rec_type, record_id, bool)` helper for the re-apply step rather than re-writing content through `pin_record_to_merge`; either is acceptable, `set_pinned` is cleaner (no content copy).

## Data Flow

- **Manual copy:** copy op → `pin_record_to_merge` (pinned=true) + record key in `plugin_session_t::m_manual_merge_keys` + persist session.
- **Regenerate:** `collect_pinned_records()` (now includes manual) → `execute()` (clears, rebuilds auto as non-pinned, prune skips pinned but none present yet) → `restore_pinned_records()` (manual re-inserted, overwrites any same-key auto) → save.
- **Reload:** load `.esp` → store seeded non-pinned → re-pin store records whose keys are in the session's manual list → subsequent regenerate preserves them.
- **Remove from merge:** `remove_from_merge` + drop key from session + persist.

## Error Handling

- Manual key with no matching store record on load → dropped silently (R5.2).
- Regenerate with no manual records → identical to today (empty pinned set).

## Testing Strategy (R7)

`[u]` (in-memory, no disk):
- After a manual copy op (driven at the `plugin_scan_t` level), the record is pinned; `collect_pinned_records` returns it.
- Regenerate simulation: seed pinned manual + non-pinned auto → `collect_pinned` → `clear` → re-add auto → `restore_pinned` → manual present with manual content; same-key auto overridden by manual.
- `prune_unchanged` skips a pinned record whose content equals a source version.
- Remove clears the key so collect/restore does not resurrect it.
- Session manual-key list round-trip (if placed in `plugin_session_t`): save → restore yields the same keys; re-apply pins matching store records and drops unmatched keys.

`[i]` only if a test exercises the actual `.esp` seed + re-pin on disk. Building/running tests is manual (no-build-or-test rule). Names: `owner::member, description`, e.g. `"merge_patch_store_t::restore_pinned, manual record overrides auto record"`, `"auto_merge_t::prune_unchanged, keeps pinned record"`.

## Files Touched

| File | Change |
|------|--------|
| `yampt.editor/source/controller/merge_controller.cpp` | manual copy/remove ops pin via `pin_record_to_merge`; update session manual-key list; reword regenerate confirmation |
| `yampt.core/source/scanner/auto_merge.cpp` | `prune_unchanged` skips pinned records |
| `yampt.core/source/scanner/merge_patch_store.hpp/.cpp` | optional `set_pinned(rec_type, record_id, bool)` helper for load re-apply |
| `yampt.editor/source/session/plugin_session.hpp/.cpp` | `m_manual_merge_keys` + accessors + save/restore under `merge/manual_records`; re-apply pin on load |
| `yampt.editor/source/controller/merge_controller.cpp` (load path) or `plugin_workspace_view.cpp` | after merged patch load, re-pin store records matching persisted manual keys |
| `yampt.tests/*` + vcxproj/.filters | `[u]` pin/restore/prune/session-key tests |

## Documentation

- CHANGELOG `[FIX]` (yEditor): regenerating the merged patch no longer discards records you copied in manually — they are kept and the automatic records are recomputed around them. (`[FIX]` — the feature was supposed to preserve manual work; the save-around existed but never protected manual copies.)
- `docs/yEditor-Manual.md`: in the merged-patch section, state that manual copies persist across regeneration and across sessions, and that removing a manual record is permanent.
- README + README.bbcode in sync if the merged-patch workflow is described. No internal detail (pinned flag) in user docs.
