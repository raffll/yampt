# Design — Persist Edit History Across Sessions

## Context (current mechanics)

- `edit_history_t m_edit_history` is a single value member of `main_window_t` (main_window.hpp), constructed once with the window, passed by reference to `find_replace_t`, `record_display_controller`, and `commit_orchestrate`.
- Keying: `make_key(type, key)` returns `type_to_str(type) + ":" + key` (edit_history.cpp). The store is `std::unordered_map<std::string, std::vector<history_entry_t>> m_entries` plus `std::set<std::string> m_session_modified`.
- Public API: `record_change(type, key, old_value, new_value, old_status)`, `get_history(type, key)`, `revert(type, key, history_index)`, plus already-implemented-but-unused `load_from_file(path)` / `save_to_file(path)` (JSON keyed by the compound key, storing value/timestamp/status) and `is_modified_this_session(type, key)`.
- `switch_document` never clears or loads history; history persists in memory across switches (source of the cross-document collision).
- Document lifecycle: `session_t::open` → `handle_open_dict` builds a `dict_document_t`; `switch_document(doc)` sets `m_active_doc`. Save: `on_save` / `on_save_all` → `doc->save()` → `dict_writer_t::write(encoded, m_path)`. `dict_document_t::path()` returns `m_path` (the on-disk path). Close/exit: `on_unload_requested` → `session_t::close`; `closeEvent` prompts and may `save_all`.
- Settings dialog `translator_settings_dialog_t`: category `QListWidget` + `QStackedWidget`, views loaded in ctor tail, saved in `apply_all()`, Apply emits `settings_applied("all")`. Bool setting pattern in `settings_store_t`: `value("Editor/X", default).toBool()` / `setValue`.
- Minimal settings view template: `translator_appearance_settings_view_t` (single control, `load`/`save`).

## Design Goals

Fix history scoping (R1), add sidecar persistence (R2, R3), add a settings toggle (R4), remove dead code (R5) — honoring architecture rules (≤2 args via struct, `_t` suffix, snake_case, tr() wrapping, testable pure logic, one class per file).

## Decision: qualify history keys by document path (Option 1)

Change `make_key` to include the owning document path so history is partitioned per dictionary:

```
compound_key = path + "|" + type_to_str(type) + ":" + key
```

This isolates each dictionary's history (R1) and makes a `.history` file naturally contain only that dictionary's entries (R2.3). It also fixes the existing cross-document collision.

Rejected alternative (Option 2): move `edit_history_t` ownership into `dict_document_t` (per-document instance). Cleaner conceptually but relocates ownership out of `main_window` and forces every collaborator (`find_replace_t`, `record_display_controller`, `commit_orchestrate`, revert handlers) to reach history through the active document. Larger blast radius for the same user-visible result; not chosen.

### Threading the path through the API

`record_change`, `get_history`, `revert` gain a document-path parameter. To respect the ≤2-argument rule and keep call sites readable, introduce a small key-context struct rather than adding positional args:

```cpp
struct history_key_t
{
    std::string document_path;
    rec_type_t type;
    std::string key;
};
```

`make_key(const history_key_t &)` builds the qualified string. The three methods take `history_key_t` plus their remaining data:
- `record_change(const history_key_t &, const std::string & old_value, status_t old_status)` — drop the unused `new_value` (it is `(void)`-discarded today), which also keeps the arg count down.
- `get_history(const history_key_t &)`
- `revert(const history_key_t &, size_t history_index)`

Call sites that must pass the active document's path:
- `commit_orchestrate` path (records the pre-edit value) — the active document is in scope.
- `find_replace_t` — already constructed with `document_t *& m_active_doc`; use its path when recording each replacement.
- History-panel revert handler and the batch-revert handler (main_window_setup.cpp) — use `m_active_doc->path()`.

### Save/load filtering by document

- `save_to_file(path, document_path)`: writes only entries whose key begins with `document_path + "|"`. Keeps one dictionary's file clean even though the in-memory store still holds all open dictionaries' entries.
- `load_from_file(path, document_path)`: merges the file's entries into `m_entries` under their qualified keys (the file already stores qualified keys once written by the new scheme). Merge, not clear — other open dictionaries' entries must survive.

Because save/load now filter/merge by document, `load_from_file` must no longer `m_entries.clear()` (it currently wipes everything). This is the one behavioral change to the existing persistence code.

## Component Changes

### 1. edit_history_t (editor/edit_history.hpp/.cpp)

- Add `history_key_t` struct.
- Change `make_key` to build the qualified key from `history_key_t`.
- Update `record_change` / `get_history` / `revert` signatures to take `history_key_t`.
- Change `load_from_file` / `save_to_file` to take a `document_path` and filter/merge by it; `load_from_file` stops clearing the whole map.
- Remove `is_modified_this_session` and `m_session_modified` (R5) — confirm no other references first.

### 2. settings_store_t (yampt.qt)

Add, mirroring `sidebar_visible`:
```cpp
bool save_history() const;              // "Editor/SaveHistory", default true
void set_save_history(bool value);
```

### 3. history_settings_view_t (dialog/settings/history_settings_view.hpp/.cpp) — new

Mirror `translator_appearance_settings_view_t`: a `QWidget` with one `QCheckBox` "Save history" (tooltip, `tr()`), `load(const settings_store_t &)` → `setChecked(settings.save_history())`, `apply(settings_store_t &)` → `settings.set_save_history(checked)`.

### 4. translator_settings_dialog_t

Add a 5th "History" page: member view pointer, `addItem(tr("History"))`, `addWidget(wrap_in_scroll_area(m_history_view))` in matching order, `m_history_view->load(m_settings)` in ctor tail, `m_history_view->apply(m_settings)` in `apply_all()`.

### 5. main_window lifecycle hooks

- **Load** — in `switch_document`, after `m_active_doc` is set and `kind() == dict`: build the sidecar path `path + ".history"`; if it exists, `m_edit_history.load_from_file(history_path, path)`.
- **Save (per dict)** — in `on_save` (single) and `on_save_all` (loop over saved docs): if `m_settings.save_history()`, after `doc->save()`, `m_edit_history.save_to_file(doc->path() + ".history", doc->path())`.
- **Save (on exit)** — in `closeEvent`, if `m_settings.save_history()`, iterate open dicts and save each one's history file.

Per the Main Window Anti-Gravity guidance, the sidecar-path derivation and the save/load calls are thin one-liners routed through `m_edit_history`; if this grows, it would move to a small history-persistence helper. For this scope the calls sit alongside the existing save/switch code.

## Data Flow

Edit → `commit_orchestrate` records via `record_change({active_path, type, key}, old_value, old_status)` into `m_entries[active_path|type:key]`.
Save dict → if enabled, `save_to_file(path.history, path)` writes only `path|*` entries.
Open/switch dict → if `path.history` exists, `load_from_file(path.history, path)` merges its entries.
Revert → History panel/batch handler calls `revert({active_path, type, key}, index)`; restored text/status applied to the active document only.

## Error Handling

- Missing history file on open → no-op (open proceeds with empty history for that dict).
- Unreadable/corrupt history file → `load_from_file` already swallows JSON parse errors and returns; open proceeds.
- Unwritable sidecar path → `save_to_file` opens an ofstream; failure is silent (the dict itself still saved). Optionally log `[warning] could not write history for "<path>"`.
- Disabling the setting stops writing but does not delete existing files.

## Testing Strategy

Pure `[u]` tests on `edit_history_t` (no disk):
- Qualified keying: `record_change` under two different `document_path`s with same type+key produces independent `get_history` results; `revert` returns the right document's entry.
- (Integration `[i]`, temp files) `save_to_file` writes only the matching document's entries; `load_from_file` merges without clearing other documents' entries; round-trip preserves value/timestamp/status.

Per unit-test rules, keep the keying/filter logic testable without UI: the filter-by-document-prefix and key building are pure and covered directly. Settings UI and lifecycle wiring verified manually.

## Files Touched

| File | Change |
|------|--------|
| `yampt.translator/source/editor/edit_history.hpp/.cpp` | `history_key_t`, qualified `make_key`, updated method signatures, filtered save/merge load, remove `is_modified_this_session` |
| `yampt.qt/source/settings_store.hpp/.cpp` | `save_history` / `set_save_history` (default true) |
| `yampt.translator/source/dialog/settings/history_settings_view.hpp/.cpp` (new) | single-checkbox settings view |
| `yampt.translator/source/dialog/settings/translator_settings_dialog.hpp/.cpp` | add History page |
| `yampt.translator/source/main_window.cpp` | load in `switch_document`, save in `on_save`/`on_save_all`/`closeEvent` |
| `yampt.translator/source/main_window_setup.cpp` | update revert handlers + find_replace/commit call sites to pass `history_key_t` |
| `yampt.translator/source/editor/commit_orchestrator.*`, `find_replace.*`, `controller/record_display_controller.*` | pass document path into history calls |
| `yampt.tests/source/tests.edit_history.cpp` (new or extend) | keying + save/load tests |
| `yampt.tests/yampt.tests.vcxproj` + `.filters` | register new files (view compiled by tests if needed, new test file) |

## Documentation

- CHANGELOG `[NEW]` (yTranslator): edit history saved next to each dictionary and restored on reopen; toggle in Settings > History.
- `docs/yTranslator-Manual.md`: History panel note about persistence + new History settings page (default on, `.history` sidecar files).
- README / README.bbcode: history already listed ("history with undo/revert"); add that it persists across sessions, keeping both in sync.
