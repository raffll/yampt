# Design — Clean Output Mode in yEditor

## Context (current mechanics)

- **Clean trigger** — `plugin_workspace_view_t::on_clean_all()` (yampt.editor/source/view/plugin_workspace_view.cpp). It prompts unsaved via `prompt_unsaved(true)` (Save → `m_merge_controller->save_all_dirty()`, Cancel → abort), guards `plugin_count() < 1` (NOT `< 2` — the two-plugin guard belongs to merged-patch creation, not clean), resolves `m_merge_controller->resolve_output_directory()`, builds `clean_options_t` from settings, and calls `cleaner.clean_all(output_path)`. It performs **no** in-memory model change and **no** nav/conflict/title refresh, because cleaning writes new files.
- **Clean engine** — `batch_cleaner_t` (yampt.core/source/scanner/batch_cleaner.hpp/.cpp), pure yampt.core (no Qt, no session). `clean_all(output_directory)` iterates plugins, skipping merge and `.esm` masters, calls `clean_plugin(plugin_idx, output_directory)`, and keeps results where `total_removed > 0`. `clean_plugin`:
  1. Reads records via the **const** accessor `m_scan.plugin(idx).get_records()` — never touches the in-memory model today.
  2. Builds `std::vector<const record_t *> kept_records`, always keeping `TES3`, skipping evil GMSTs and junk cells.
  3. If header repair is enabled and record 0 is TES3, copies its content into a local `patched_header` and runs `header_repair_t::update_master_sizes` / `update_version_to_1_3`.
  4. Early return: `const bool has_header_changes = !patched_header.empty() && patched_header != kept_records[0]->content; if (result.total_removed == 0 && !has_header_changes) return result;`
  5. Writes `output_directory + "/" + filename` via `std::ofstream`, streaming `patched_header` for index 0 (when changed) else each kept record's `content`; sets `result.written = true`; logs `[info] saved "<path>"`. On open failure logs `[error] cannot write <path>`.
- **Structs** — `clean_options_t { bool evil_gmst=true; bool junk_cell=true; bool update_master_sizes=false; bool update_version=false; }`. `clean_result_t { std::string plugin_filename; int evil_gmst_removed; int junk_cell_removed; int total_removed; int master_sizes_updated; bool version_updated; bool written; }`.
- **In-memory edit reference** — `field_edit_controller_t::commit_to_source`: `mutable_plugin(idx).select_record(index)` → `replace_record(content)` → `m_session.mark_plugin_dirty(idx)` → `recompute_single_conflict(type, id)` → `emit record_modified(false, plugin_path)`. This edits ONE record; `replace_record` mutates `content`/`size`/`modified` of the currently selected record.
- **esm_reader_t** — `std::vector<record_t> m_records`, cached `record_t * ptr_record`. `record_t::id` is `const` (domain_types.hpp), so records can be mutated in content but not reassigned an id, and there is **no** API to delete records or replace the whole vector. `select_record(i)` sets `ptr_record = &m_records.at(i)` and clears m_key/m_value; `get_record()` dereferences `ptr_record`.
- **plugin_scan_t** — `mutable_plugin(idx)` returns the reader by ref; `rebuild_conflicts()` clears and rebuilds `m_entries`; `recompute_single_conflict(type, id)` is merge-plugin oriented.
- **Session dirty API** — `plugin_session_t`: `mark_plugin_dirty(int)`, `clear_plugin_dirty(int)`, `is_plugin_dirty(int)`, `dirty_plugins()`, `has_any_unsaved()` — filename-keyed `std::set`, cleared on load/unload.
- **Manual save** — `merge_controller_t::save_plugin(int)` = `binary_file_io::write_file(mutable_plugin(idx).get_records(), plugin_path(idx))` then `clear_plugin_dirty(idx)`. `save_all_dirty()` loops the dirty set. `save_plugin`/`save_all_dirty` are what the view calls in `on_save_current` / `on_save_all`, each followed by `rebuild_nav_preserving_state()` + `emit unsaved_changes_changed(has_any_unsaved())`.
- **Settings** — `cleaning_settings_view_t` (four checkboxes, `load`/`save`), `settings_store_t` `Cleaning/*` bool getter/setter pattern. No dialog-registration change is needed to add a control to the existing page.

## Design Goals

Add a user-selectable clean output mode (R1) with two behaviors: modify-in-place/deferred (R2, R4) and create-new-files/immediate (R3), plumb the mode through `batch_cleaner_t` without adding yEditor/Qt deps (R5), orchestrate the mode in the yEditor caller (R6), regress nothing (R7), and keep the logic testable (R8) — honoring the architecture rules (yampt.core purity, ≤2 args via struct where needed, `_t` suffix, snake_case, tr()/translate() wrapping, one class per file).

## Decision: mode carried on clean_options_t, mechanism selected in batch_cleaner_t

Add `bool modify_in_place = false;` to `clean_options_t` (default false = create-new-files = current behavior, R1.2/R5.1). `batch_cleaner_t` reads it and branches the output mechanism inside `clean_plugin`. The caller sets it from the new setting.

### clean_plugin signature (R5.2)

Keep the current signature `clean_plugin(int plugin_idx, const std::string & output_directory)` and `clean_all(const std::string & output_directory)`. In modify-in-place mode the `output_directory` argument is ignored (the caller passes an empty string). This avoids a second overload or an unused-but-meaningful parameter: the mode lives in `m_options`, the directory is used only in the create-new-files branch. Rationale: keeps one call path, one result vector, minimal surface change; the "ignored in the wrong mode" is documented and the caller passes `""` in-place.

### Rejected alternative

Split `clean_plugin` into `clean_plugin_to_file` / `clean_plugin_in_place`. Rejected: duplicates the kept-records/header-repair computation (DRY violation). Instead the shared computation (kept records + patched header + early return) runs once, then a single `if (m_options.modify_in_place)` picks commit-in-memory vs. write-file.

## Decision: esm_reader_t gains set_records (R4)

Because `record_t::id` is `const` and there is no erase/replace-vector API, modify-in-place needs a way to swap the whole record set. Add:

```cpp
void esm_reader_t::set_records(std::vector<record_t> records);
```

Implementation: `m_records = std::move(records); ptr_record = nullptr; m_key = {}; m_value = {};`. This keeps invariants (R4.2): `get_records()` returns the new set, and the cached `ptr_record` is invalidated so a subsequent `select_record` is required before `get_record`. This is the cleanest fit — `record_t::id` const means we rebuild the vector wholesale rather than erase in place.

The cleaner builds a `std::vector<record_t>` of kept records (R4.3): copy each kept record; for index 0, when header repair changed it, construct a `record_t` with the patched header content (id stays `"TES3"`, size = patched content size, `modified = true`). It commits via `m_scan.mutable_plugin(idx).set_records(std::move(new_records))`.

Note: `clean_plugin` currently reads through the **const** `plugin(idx)` accessor. Modify-in-place must read through `mutable_plugin(idx)` (or read via const then commit via mutable). The design reads the record set once (either accessor returns the same vector), computes kept records, then in the in-place branch commits via `mutable_plugin(idx).set_records(...)`.

## Component Changes

### 1. clean_options_t / clean_result_t (yampt.core/source/scanner/batch_cleaner.hpp)

- Add `bool modify_in_place = false;` to `clean_options_t` (R5.1).
- Add `int plugin_idx = -1;` to `clean_result_t` so the caller can mark it dirty (R5.3). Set it at the top of `clean_plugin` alongside `plugin_filename`.
- Retain `written` — still meaningful in create-new-files mode (it records that the export happened); it stays `false` in modify-in-place mode (R5.3). A separate `bool committed = false;` distinguishes in-place commits so `clean_all` can decide inclusion and the caller can act.

### 2. batch_cleaner_t::clean_plugin (yampt.core/source/scanner/batch_cleaner.cpp)

Shared prefix unchanged: build `kept_records`, compute `patched_header`, `has_header_changes`, early-return on no-op. Then:

```cpp
if (m_options.modify_in_place)
{
    std::vector<record_t> new_records;
    new_records.reserve(kept_records.size());
    for (size_t i = 0; i < kept_records.size(); ++i)
    {
        if (i == 0 && has_header_changes)
            new_records.push_back(record_t{ kept_records[0]->id, patched_header, patched_header.size(), true });
        else
            new_records.push_back(*kept_records[i]);
    }

    log_removed_summary(result, removed_log);
    m_scan.mutable_plugin(plugin_idx).set_records(std::move(new_records));
    result.committed = true;
    return result;
}

// create-new-files branch = current ofstream write, unchanged, sets result.written
```

`log_removed_summary` is a small helper (or inline) that emits the same `[info] <file>: removed N` + removal lines + master/version notes for both modes, so logging (R3.4) is consistent. Create-new-files retains its `[info] saved "<path>"`; modify-in-place does not log a save path (nothing written).

### 3. batch_cleaner_t::clean_all (R5.2)

Inclusion condition currently is `result.total_removed > 0`. Change to include a plugin when it was actually changed in either mode: `if (result.committed || result.written || result.total_removed > 0)`. Since header-only changes with zero removals are meaningful in both modes now (they mutate/export the header), include them too: gate on `result.committed || result.written`. In create-new-files a header-only change already writes the file today but was excluded from the returned vector — this fixes that inconsistency for the caller's benefit. (No user-visible regression: the file is still written as before.)

### 4. esm_reader_t (yampt.core/source/io/esm_reader.hpp/.cpp)

Add `void set_records(std::vector<record_t> records);` (R4.1): moves into `m_records`, resets `ptr_record`/m_key/m_value (R4.2).

### 5. settings_store_t (yampt.qt/source/settings_store.hpp/.cpp)

Add, mirroring `clean_update_master_sizes`:

```cpp
bool clean_modify_in_place() const;              // "Cleaning/ModifyInPlace", default false
void set_clean_modify_in_place(bool value);
```

Default false preserves today's create-new-files behavior after update (R1.2).

### 6. cleaning_settings_view_t (yampt.editor/source/dialog/settings/cleaning_settings_view.hpp/.cpp)

Add a mode control that communicates two mutually-exclusive outcomes (R1.4). Use two `QRadioButton`s in a titled group ("Output") — clearer than a single checkbox for a two-outcome choice:

- "Modify loaded plugins in place" — tooltip: `Clean the loaded plugins in memory; save to overwrite the originals`.
- "Create new cleaned files" — tooltip: `Write cleaned copies to the output directory, leaving loaded plugins unchanged`.

Add member `QRadioButton * m_modify_in_place_radio` / `m_create_new_files_radio`. `load()` sets the checked radio from `settings.clean_modify_in_place()`; `save()` writes `settings.set_clean_modify_in_place(m_modify_in_place_radio->isChecked())`. Both radios get `setToolTip` (gui-tooltips rule). No settings-dialog registration change (R1.3).

### 7. plugin_workspace_view_t::on_clean_all (yampt.editor/source/view/plugin_workspace_view.cpp)

```cpp
const bool modify_in_place = m_settings.clean_modify_in_place();

// Pre-clean unsaved prompt: keep for create-new-files (writes over possibly-stale state);
// skip for modify-in-place (cleaning only adds more in-memory dirty state). (R6.4 decision)
if (!modify_in_place && m_session->has_any_unsaved())
{
    const auto answer = prompt_unsaved(true);
    if (answer == QMessageBox::Cancel) return;
    if (answer == QMessageBox::Save) m_merge_controller->save_all_dirty();
}

if (m_session->scan().plugin_count() < 1) { log_message("[error] no plugins loaded to clean"); return; }

std::string output_path;
if (!modify_in_place)
{
    output_path = m_merge_controller->resolve_output_directory();
    if (output_path.empty()) { log_message("[error] cannot determine output directory"); return; }
    log_message("[info] clean: output=" + output_path);
}

batch_cleaner_t cleaner(m_session->scan(), [this](const std::string & m){ log_message(m); });
clean_options_t options;
options.evil_gmst = m_settings.clean_evil_gmst_enabled();
options.junk_cell = m_settings.clean_junk_cell_enabled();
options.update_master_sizes = m_settings.clean_update_master_sizes();
options.update_version = m_settings.clean_update_version();
options.modify_in_place = modify_in_place;
cleaner.set_options(options);

const auto results = cleaner.clean_all(output_path);
if (results.empty()) { log_message("[info] no records to clean"); return; }

if (modify_in_place)
{
    for (const auto & result : results)
        m_session->mark_plugin_dirty(result.plugin_idx);

    m_session->scan().rebuild_conflicts();
    rebuild_nav_preserving_state();
    emit unsaved_changes_changed(m_session->has_any_unsaved());
}
```

The dirty-mark / single `rebuild_conflicts()` / nav-rebuild / title-update sequence (R2.4, R6.3) runs only in modify-in-place mode. Create-new-files does none of it (R3.2, R6.3). The `plugin_count() < 1` guard is retained for both (R6.5 — corrected from the requirement's "2 plugins"; the actual clean guard is `< 1`). Per the Anti-Gravity rule this stays a thin block; the marking loop is trivial routing to the session and scan, not new orchestration logic. If it grows, it moves to a clean controller.

### 8. Manual save (R2.5) — unchanged

Saving a cleaned dirty plugin uses the existing `save_plugin` / `save_all_dirty` → `binary_file_io::write_file(get_records(), plugin_path)` path. Because `set_records` replaced the in-memory vector, `get_records()` returns the cleaned set and the write reflects the clean.

## Data Flow

Create-new-files (default): Clean → prompt/save if dirty → resolve output dir → `clean_all(dir)` → per plugin write `<dir>/<filename>`, `written=true`. In-memory model untouched, no dirty, no asterisk.

Modify-in-place: Clean (no prompt) → `clean_all("")` → per changed plugin `mutable_plugin(idx).set_records(cleaned)`, `committed=true`, `plugin_idx` set → caller marks each dirty, `rebuild_conflicts()`, nav rebuild, title asterisk. Save → `write_file(get_records(), path)` overwrites original, `clear_plugin_dirty`, asterisk clears. Not saving → clean discarded on unload/reload.

## Error Handling

- Create-new-files: unwritable output path → `[error] cannot write <path>` (unchanged), that plugin's result has `written=false`.
- Modify-in-place: no file I/O, so no write errors; `set_records` cannot fail.
- Empty result set → `[info] no records to clean` (both modes, R3.4 retained; also applies in-place).

## Testing Strategy (R8)

Pure `[u]` tests, no disk:
- `esm_reader_t::set_records` — after `set_records(new_vector)`, `get_records()` returns the new set; calling `get_record()` before a fresh `select_record` is invalid (ptr reset). Build a small in-memory reader by constructing records and swapping them in.
- `batch_cleaner_t` modify-in-place — on an in-memory `plugin_scan_t` holding a plugin with a known evil-GMST/junk-cell record, `clean_all("")` with `modify_in_place=true` removes the expected records from `mutable_plugin(idx).get_records()` and reports the affected `plugin_idx` in the result. No file written.

Create-new-files file output is validated by integration-level checks / manual verification, not `[u]` tests (R8.3). Building and running tests is done manually by the user (no-build-or-test rule).

Test names follow `owner::member, description` with `[u]` tag, e.g. `"esm_reader_t::set_records, replaces set and resets pointer"`, `"batch_cleaner_t::clean_all, modify in place removes records"`.

## Files Touched

| File | Change |
|------|--------|
| `yampt.core/source/scanner/batch_cleaner.hpp` | `modify_in_place` on options; `plugin_idx`/`committed` on result |
| `yampt.core/source/scanner/batch_cleaner.cpp` | branch commit-in-memory vs write-file; shared log helper; `clean_all` inclusion condition |
| `yampt.core/source/io/esm_reader.hpp/.cpp` | `set_records(std::vector<record_t>)` |
| `yampt.qt/source/settings_store.hpp/.cpp` | `clean_modify_in_place` / `set_clean_modify_in_place` (default false) |
| `yampt.editor/source/dialog/settings/cleaning_settings_view.hpp/.cpp` | two-radio Output group + load/save |
| `yampt.editor/source/view/plugin_workspace_view.cpp` | read mode, per-mode prompt/output-dir, in-place dirty/rebuild/title sequence |
| `yampt.tests/source/tests.esm_reader.cpp` / `tests.batch_cleaner.cpp` (new or extend) | `[u]` tests for set_records + in-place clean |
| `yampt.tests/yampt.tests.vcxproj` + `.filters` | register new test file(s) if added |

## Documentation

- CHANGELOG `[NEW]` (yEditor): Clean All can now modify loaded plugins in place (review, then save to overwrite the originals) or create new cleaned files; selectable in Settings > Cleaning; default creates new files.
- `docs/yEditor-Manual.md` — Cleaning Plugins section: describe the two output modes and that in-place cleaning marks plugins with an asterisk and is written only on Save; keep the existing create-new-files description as one of the two modes. Update the Settings > Cleaning bullet.
- README / README.bbcode in sync if cleaning is described there (add the in-place option). Per changelog rules, no build/refactor internals in user docs.
