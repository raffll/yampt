# Implementation Plan

## Overview

Persist yTranslator's edit history to a per-dictionary `<dictpath>.history` sidecar file so revert works after restart, controlled by a new "History" settings page (on by default). The enabling prerequisite is fixing history scoping: today the single window-wide `edit_history_t` keys entries only by `type:key` with no document qualifier and never clears on switch, so dictionaries sharing a record key cross-contaminate. Work order: fix scoping (key qualification via a `history_key_t` struct) and update call sites, then filtered save/merge load, then the settings toggle and page, then the lifecycle hooks, then dead-code removal, tests, and docs.

## Tasks

- [ ] 1. Add history_key_t and qualify the history key
  - Add `history_key_t { std::string document_path; rec_type_t type; std::string key; }` in `edit_history.hpp`.
  - Change `make_key` to build `document_path + "|" + type_to_str(type) + ":" + key`.
  - _Requirements: R1_

- [ ] 2. Update edit_history_t public API to take history_key_t
  - Change `record_change` (drop unused `new_value`), `get_history`, `revert` to accept `history_key_t`.
  - _Requirements: R1_

- [ ] 3. Update all history call sites to pass the document path
  - `commit_orchestrate`, `find_replace_t` (uses `m_active_doc`), History-panel revert handler and batch-revert handler (main_window_setup.cpp), `record_display_controller` as needed.
  - _Requirements: R1_

- [ ] 4. Make save/load filter and merge by document
  - `save_to_file(path, document_path)` writes only entries whose key begins with `document_path + "|"`.
  - `load_from_file(path, document_path)` merges entries; remove the `m_entries.clear()` so other open dictionaries' history survives.
  - _Requirements: R2, R3_

- [ ] 5. Unit-test qualified keying and save/load filtering
  - `[u]`: same type+key under two document paths yield independent `get_history`/`revert`.
  - `[i]` (temp files): `save_to_file` writes only the matching document's entries; `load_from_file` merges without clearing; round-trip preserves value/timestamp/status.
  - _Requirements: R1, R2, R3_

- [ ] 6. Add save_history setting to settings_store_t
  - `save_history()` / `set_save_history(bool)`, INI `Editor/SaveHistory`, default true (mirror `sidebar_visible`).
  - _Requirements: R4_

- [ ] 7. Create the History settings view
  - New `history_settings_view_t` (`dialog/settings/history_settings_view.hpp/.cpp`) mirroring the appearance view: one `QCheckBox` "Save history" (tooltip, tr()), `load`/`apply`.
  - _Requirements: R4_

- [ ] 8. Add the History page to the settings dialog
  - In `translator_settings_dialog_t`: member pointer, `addItem(tr("History"))`, `addWidget(wrap_in_scroll_area(...))` in matching order, `load` in ctor tail, `apply` in `apply_all()`.
  - _Requirements: R4_

- [ ] 9. Load history on document open/switch
  - In `switch_document`, after `m_active_doc` set and `kind()==dict`: if `<path>.history` exists, `load_from_file(<path>.history, path)`.
  - _Requirements: R3_

- [ ] 10. Save history on dict save and on app close
  - `on_save` and `on_save_all`: if `save_history()`, after `doc->save()` write `<path>.history` filtered to that dict.
  - `closeEvent`: if `save_history()`, save each open dict's history.
  - _Requirements: R2_

- [ ] 11. Remove dead session-only code
  - Remove `edit_history_t::is_modified_this_session` and `m_session_modified` after confirming no other references.
  - _Requirements: R5_

- [ ] 12. Register new files in the tests project
  - Add the new test file (and `history_settings_view.cpp` if compiled by tests) to `yampt.tests.vcxproj` + `.vcxproj.filters`.
  - _Requirements: R1, R2, R3_

- [ ] 13. Update documentation
  - CHANGELOG `[NEW]` (yTranslator): history saved next to each dictionary, restored on reopen, toggle in Settings > History.
  - `docs/yTranslator-Manual.md`: History panel persistence + History settings page.
  - README + README.bbcode in sync: history persists across sessions.
  - _Requirements: R2, R3, R4_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1, 6], "depends_on": [] },
    { "wave": 2, "tasks": [2, 4, 7], "depends_on": [1, 6] },
    { "wave": 3, "tasks": [3, 5, 8], "depends_on": [2, 4, 7] },
    { "wave": 4, "tasks": [9, 10, 11], "depends_on": [3, 4, 8] },
    { "wave": 5, "tasks": [12, 13], "depends_on": [5, 9, 10, 11] }
  ]
}
```

The scoping chain (1 → 2 → 3) is the critical path and must land before the lifecycle hooks (9, 10) are meaningful. The settings chain (6 → 7 → 8) is independent and can proceed in parallel. Dead-code removal (11) waits until nothing references the removed method.

## Notes

- Chosen approach is Option 1 (path-qualified keys), not per-document `edit_history_t` ownership — smaller blast radius for the same result, and it fixes the pre-existing cross-document collision as a side effect.
- `load_from_file` must stop clearing the whole map; it merges so multiple open dictionaries keep their histories.
- Loading is NOT gated by the save-history setting — an existing `.history` file stays usable even if the user later disables saving.
- Pure keying/filter logic is unit-tested without UI or disk; file round-trips use temp files per the integration-test rules.
- Building and running tests is done manually by the user (no-build-or-test rule).
