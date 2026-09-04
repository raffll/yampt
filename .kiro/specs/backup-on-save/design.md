# Design — Backup Saved Files (.bak) in Both Apps

## Context (current mechanics)

- **yTranslator save** — `session_t::save_all()` loops dirty docs and calls `document_t::save()`. `dict_document_t::save()` re-encodes and calls `dict_writer_t::write(encoded, m_path)`; `yaml_document_t::save()` calls `yaml_l10n_writer_t::write(m_path, entries, key_order)`. `loc_document_t::save()` / `eet_document_t::save()` are empty. The byte write lives in yampt.core: `dict_writer_t::write` and `yaml_l10n_writer_t::write` each open `std::ofstream(path, std::ios::binary)` directly on the document's own path. No temp file, no backup.
- **yEditor save (source plugin)** — `merge_controller_t::save_plugin(int)`: `binary_file_io::write_file(mutable_plugin(idx).get_records(), plugin_path(idx))`, then `clear_plugin_dirty`. `save_all_dirty()` loops the dirty set.
- **yEditor save (merged patch)** — `save_merged_patch()` → `save_merge_to_file(output_path, ...)` → `patch_builder_t::save(output_path, ...)`, which writes `output_path + ".tmp"` via `binary_file_io::write_file` then `std::filesystem::rename(temp, output)` with a `copy_file(overwrite_existing)` fallback. Only `copy_file` usage in the codebase.
- **binary_file_io** (yampt.core/source/io) — `read_file`, `write_text`, `write_file(records, path)`, `create_file(records, path)`. No copy helper.
- **settings_store_t** (yampt.qt) — shared by both apps. Bool pattern: `value("Section/Key", default).toBool()` / `setValue("Section/Key", value)`. Declared as getter/setter pairs in the header.
- **Settings dialogs** — `translator_settings_dialog_t` and `editor_settings_dialog_t`: `QListWidget` categories + `QStackedWidget` pages; each page a `QWidget` in `dialog/settings/` with `load`/`save`; `wrap_in_scroll_area` wraps each. `addItem` order is index-linked to `addWidget` order via `currentRowChanged → setCurrentIndex`. `cleaning_settings_view_t` is the boolean-page template.
- **Paths** — `string_utils::join_path`, `path_to_utf8`/`utf8_to_path` bridge `std::string` ↔ `std::filesystem::path` (important for non-ASCII on Windows).

## Design Goals

Add one shared opt-in setting (R1), a "Backup" page in both apps (R2), a single shared best-effort backup helper (R3), and hook it into both apps' overwrite points (R4, R5) with zero behavior change when off (R6), keeping the logic testable (R7) and honoring the architecture rules (yampt.core purity for the helper, one class per file, `_t`/snake_case, tr() wrapping, consistent-across-apps).

## Decision: shared helper `binary_file_io::backup_file`

Add to yampt.core io:

```cpp
// binary_file_io.hpp
bool backup_file(const std::string & path);
```

```cpp
// binary_file_io.cpp
bool binary_file_io::backup_file(const std::string & path)
{
    const auto source = string_utils::utf8_to_path(path);

    std::error_code exists_error;
    if (!std::filesystem::exists(source, exists_error) || exists_error)
        return false;

    const auto backup = string_utils::utf8_to_path(path + ".bak");

    std::error_code copy_error;
    std::filesystem::copy_file(
        source, backup, std::filesystem::copy_options::overwrite_existing, copy_error);

    if (copy_error)
    {
        app_logger_t::add_log("[warning] backup failed for \"" + path + "\": " + copy_error.message() + "\r\n");
        return false;
    }

    app_logger_t::add_log("[info] backup written \"" + path + ".bak\"\r\n");
    return true;
}
```

Rationale: `binary_file_io` is the file-IO home (R3.2); this keeps one implementation shared by both apps and tests. The `.bak` name is the full original name + `.bak` (R3.3). Missing source → returns false, no error (R3.1). Copy failure logs `[warning]` and returns false; callers ignore the return and proceed with the save (R3.4). It never throws (uses `std::error_code`), so it cannot abort a save.

### Rejected alternative

A Qt-based helper in yampt.qt using `QFile::copy`. Rejected: the yTranslator dict/yaml writers and the yEditor plugin writer are all in yampt.core / call into yampt.core, and the merged-patch path already uses `std::filesystem`. Putting the helper in yampt.core keeps it reusable everywhere and unit-testable without Qt.

## Decision: gate at the save call sites, not inside the low-level writers

The `backup_on_save()` setting is read in the app layer (translator session / editor controller), not in yampt.core, because `settings_store_t` is a yampt.qt type and yampt.core must stay Qt-free. The core helper is unconditional (it just copies if the file exists); the *decision to call it* lives in the app.

## Component Changes

### 1. settings_store_t (yampt.qt/source/settings_store.hpp/.cpp)

Add, mirroring `clean_evil_gmst_enabled`:

```cpp
bool backup_on_save() const;               // "Backup/OnSave", default false
void set_backup_on_save(bool value);
```

Default false preserves current behavior (R1.2). One pair serves both apps (R1.3).

### 2. backup_file helper (yampt.core/source/io/binary_file_io.hpp/.cpp)

Add `backup_file` as above (R3).

### 3. backup_settings_view_t — yTranslator (yampt.translator/source/dialog/settings/backup_settings_view.hpp/.cpp)

`QWidget` subclass modeled on `cleaning_settings_view_t`:

- One `QGroupBox(tr("Backup"))` containing `QCheckBox m_backup_check` labeled `tr("Back up files before overwriting")`, tooltip `tr("Copy the previous file version to a .bak alongside it before each save")`.
- `load(const settings_store_t & settings)` → `m_backup_check->setChecked(settings.backup_on_save())`.
- `save(settings_store_t & settings) const` → `settings.set_backup_on_save(m_backup_check->isChecked())`.
- Panel padding per panel-padding rule (mixed content, `(2,2,2,2)` outer; the page is wrapped in a scroll area by the dialog).

### 4. backup_settings_view_t — yEditor (yampt.editor/source/dialog/settings/backup_settings_view.hpp/.cpp)

Identical class (same name, same label/tooltip, same INI key via the shared setting) per consistent-across-apps (R2.5). Same-named files across the two apps are allowed (naming-conventions).

### 5. translator_settings_dialog_t (yampt.translator/source/dialog/settings/translator_settings_dialog.cpp)

- Construct `m_backup_view = new backup_settings_view_t(this)`.
- `m_category_list->addItem(tr("Backup"))` and `m_content_stack->addWidget(wrap_in_scroll_area(m_backup_view))` in matching position (append after the last existing page).
- `m_backup_view->load(m_settings)` in the constructor; `m_backup_view->save(m_settings)` in `apply_all()`.

### 6. editor_settings_dialog_t (yampt.editor/source/dialog/settings/editor_settings_dialog.cpp)

Same registration: `addItem(tr("Backup"))`, matching `addWidget(wrap_in_scroll_area(m_backup_view))`, `load` in constructor, `save` in `apply_all`.

### 7. yTranslator save hook (R4)

The two real writers are `dict_document_t::save()` and `yaml_document_t::save()`, each of which knows its own `m_path` and whether it actually writes. The cleanest single hook that covers both without touching yampt.core writers and without backing up no-op docs is at each document's `save()`, immediately before its writer call, gated by the setting.

The document does not own a `settings_store_t`. Two clean options; the design picks **B**:

- Option A — pass a "backup enabled" bool into `save()`. Rejected: changes the `document_t::save()` virtual signature for all document types, including the no-op ones.
- Option B — the session performs the backup before delegating to `save()`, because the session drives saving and can read the setting. `session_t` already exposes per-document paths (documents expose `path()`), and `session_t::save_all()` is the loop. Add the backup at the point the session decides to save a dirty document:

```cpp
// session_t::save_all(), per dirty document, before doc->save()
if (m_backup_on_save && document_writes_to_disk(*doc))
    binary_file_io::backup_file(doc->path());
doc->save();
```

`m_backup_on_save` is a bool the session is told once (set from `settings.backup_on_save()` when the dialog applies and at startup), keeping `session_t` free of the Qt settings type. `document_writes_to_disk` is true for dict and yaml native docs, false for loc/eet/non-writing docs — mirror the same early-return conditions those `save()` bodies use (e.g. yaml only writes when native and has modified indices), exposed via a small `bool document_t::will_write() const` predicate so the session backs up only when the document will actually overwrite (R4.3). The design will confirm the predicate name; the intent is: back up exactly when `save()` would write bytes.

This keeps the backup in the app layer (session), reads the setting once, and covers both writing document types through one call site. No `.bak` for no-op saves (R4.3).

### 8. yEditor save hook (R5)

`merge_controller_t` already holds `m_settings`. In `save_plugin(int plugin_idx)`, before the write:

```cpp
const auto & path = m_session.scan().plugin_path(plugin_idx);
if (m_settings.backup_on_save())
    binary_file_io::backup_file(path);
const bool written = binary_file_io::write_file(plugin.get_records(), path);
```

For the merged patch, hook at the `save_merged_patch()` boundary (before `save_merge_to_file`), because the temp/rename atomic write is internal to `patch_builder_t::save` and must stay unchanged (R6.2):

```cpp
// save_merged_patch(), after resolving output_path, before save_merge_to_file
if (m_settings.backup_on_save())
    binary_file_io::backup_file(output_path);
```

`backup_file` no-ops when the merged patch does not yet exist (first generation), so first-time patch creation makes no `.bak` (R3.1). `save_all_dirty` calls `save_plugin` per plugin → one `.bak` each (R5.3).

## Data Flow

- **Off (default):** setting false → no `backup_file` call in either app → identical to today (R6.1).
- **yTranslator, on:** save_all → for each dirty writing doc, `backup_file(doc->path())` (copies existing file to `.bak`) → `doc->save()` overwrites.
- **yEditor plugin, on:** save_plugin → `backup_file(plugin_path)` → `write_file` overwrites.
- **yEditor merged patch, on:** save_merged_patch → `backup_file(output_path)` → `patch_builder_t::save` writes temp + renames over output.

## Error Handling

- Missing source file → `backup_file` returns false silently (nothing to back up).
- Copy failure → `[warning]` logged, returns false, save proceeds anyway (R3.4). `backup_file` never throws (uses `std::error_code`).

## Testing Strategy (R7)

`[i]` integration tests (touch disk, use `std::filesystem::temp_directory_path()`, clean up):
- `binary_file_io::backup_file` — write a temp file with known bytes, call `backup_file`, assert `<path>.bak` exists with identical bytes; write different bytes, call again, assert `.bak` now holds the newer pre-save content (overwrite); call on a non-existent path, assert returns false and creates nothing.

`[u]` (no disk): the settings round-trip is covered by the standard `settings_store_t` pattern; no new pure logic beyond the helper (which is inherently file I/O, hence `[i]`). Test names follow `owner::member, description`, e.g. `"binary_file_io::backup_file, copies existing file to bak"`, tagged `[i]`.

Building and running tests is done manually by the user (no-build-or-test rule).

## Files Touched

| File | Change |
|------|--------|
| `yampt.qt/source/settings_store.hpp/.cpp` | `backup_on_save` / `set_backup_on_save` (default false) |
| `yampt.core/source/io/binary_file_io.hpp/.cpp` | `backup_file(const std::string & path)` |
| `yampt.translator/source/dialog/settings/backup_settings_view.hpp/.cpp` | new Backup page |
| `yampt.editor/source/dialog/settings/backup_settings_view.hpp/.cpp` | new Backup page |
| `yampt.translator/source/dialog/settings/translator_settings_dialog.cpp/.hpp` | register Backup page |
| `yampt.editor/source/dialog/settings/editor_settings_dialog.cpp/.hpp` | register Backup page |
| `yampt.translator/source/session/session.cpp/.hpp` | backup-before-save in `save_all`; `set_backup_on_save`; will-write predicate use |
| `yampt.translator/source/model/document.hpp` (+ doc impls) | `will_write()` predicate if needed for R4.3 |
| `yampt.editor/source/controller/merge_controller.cpp` | backup in `save_plugin` and `save_merged_patch` |
| `yampt.translator/yampt.translator.vcxproj` + `.filters` | add backup_settings_view files |
| `yampt.editor/yampt.editor.vcxproj` + `.filters` | add backup_settings_view files |
| `yampt.tests/source/tests.binary_file_io.cpp` (new or extend) + vcxproj/.filters | `[i]` backup_file test |

## Documentation

- CHANGELOG `[NEW]` under **Both Apps**: files can be backed up to a `.bak` before overwriting on save; enable it in Settings → Backup; off by default.
- `docs/yTranslator-Manual.md` and `docs/yEditor-Manual.md`: add a Backup settings description — when enabled, the previous version of each saved file is copied to a `.bak` in the same folder before the new version is written.
- README + README.bbcode kept in sync if they enumerate settings/features (add the backup option). Per changelog rules, no build/refactor internals in user docs.
