# Requirements — Backup Saved Files (.bak) in Both Apps

## Background — Current Behavior

Both yampt GUI apps overwrite files in place on save, with no backup of the previous version.

### yTranslator save path

- `session_t::save_all()` (yampt.translator/source/session/session.cpp) iterates open documents and calls the virtual `document_t::save()` on each dirty one.
- `dict_document_t::save()` (yampt.translator/source/model/dict_document.cpp) re-encodes records and calls `dict_writer_t::write(encoded, m_path)`.
- `yaml_document_t::save()` (yampt.translator/source/model/yaml_document.cpp) builds entries and calls `yaml_l10n_writer_t::write(m_path, entries, key_order)`.
- `loc_document_t::save()` and `eet_document_t::save()` are no-ops.
- The actual byte write happens in yampt.core: `dict_writer_t::write` opens `std::ofstream(path, std::ios::binary)` and streams JSON directly to the document's own `m_path` — no temp file, no rename, no backup. `yaml_l10n_writer_t::write` does the same for its path.

### yEditor save path

- `merge_controller_t::save_plugin(int plugin_idx)` (yampt.editor/source/controller/merge_controller.cpp) writes the in-memory records back over the plugin's own path via `binary_file_io::write_file(plugin.get_records(), path)`. `save_all_dirty()` loops the dirty set calling `save_plugin`.
- `merge_controller_t::save_merged_patch()` → `save_merge_to_file()` → `patch_builder_t::save(output_path, ...)`. This one already writes to `output_path + ".tmp"` and then `std::filesystem::rename(temp, output)` (with a `std::filesystem::copy_file(..., overwrite_existing)` fallback). This is the only `std::filesystem::copy_file` usage in the codebase.
- The byte write for source plugins happens in yampt.core: `binary_file_io::write_file(const std::vector<record_t> &, const std::string & path)` opens `std::ofstream(path, std::ios::binary)`.

### Shared infrastructure

- `settings_store_t` (yampt.qt/source/settings_store.hpp/.cpp) is shared by both apps. Boolean settings follow a `value("Section/Key", default).toBool()` getter and `setValue("Section/Key", value)` setter pattern (e.g. `clean_evil_gmst_enabled` / `set_clean_evil_gmst_enabled`).
- Both apps use a settings dialog built from a left `QListWidget` category list and a right `QStackedWidget`. Pages live in each app's `dialog/settings/` folder as `QWidget` subclasses exposing `load(const settings_store_t &)` and `save(settings_store_t &) const` (some use `apply`). `cleaning_settings_view_t` (yEditor) is the canonical boolean-checkbox page template. The `addItem` order in the category list must match the `addWidget` order in the stack.
- Path joining is done with `string_utils::join_path` (never string concatenation), per the path-handling rule. There is NO shared file-copy helper anywhere; the only copy is the inline one in `patch_builder_t::save`.

## Problem

When a save overwrites an existing file, the previous content is lost. A user who saves a bad edit, a truncated write, or the wrong codepage has no way to recover the prior version. Every other tool in this space (TES3Edit, tes3cmd) keeps a backup of the file it modifies. yampt has no such safety net.

## Goal

Add an opt-in backup: before a save overwrites an existing file, copy the current on-disk file to a `.bak` alongside it. Controlled by a single on/off checkbox in a new "Backup" settings tab, present in both yTranslator and yEditor, backed by one shared setting.

## User-Facing Outcomes

- A new "Backup" tab in the Settings dialog of both apps, containing one checkbox: enable/disable backup on save.
- When backup is enabled and a save is about to overwrite an existing file, the current file is first copied to `<original>.bak` in the same folder. The `.bak` reflects the file's content from before this save.
- When backup is disabled (the default preserves current behavior), saves overwrite in place with no `.bak`, exactly as today.
- If the target file does not exist yet (first save of a new file), no `.bak` is created — there is nothing to back up.
- A backup that already exists is overwritten by the newer backup (one `.bak` per file, always the immediately-previous version).
- The setting is shared: toggling it in one app affects the other, since both read the same INI key.

## Requirements

### R1 — Backup setting

1.1 A persisted boolean setting controls backup-on-save, added to the shared `settings_store_t` following the existing getter/setter + INI pattern: `backup_on_save()` / `set_backup_on_save(bool)` under a `Backup/OnSave` key.
1.2 Default is **off** (false), preserving today's behavior for existing users after an update.
1.3 Because `settings_store_t` lives in yampt.qt and is shared, the single getter/setter pair serves both apps.

### R2 — Backup settings page (both apps)

2.1 Each app gains a `backup_settings_view_t` in its own `dialog/settings/` folder (`yampt.translator/source/dialog/settings/` and `yampt.editor/source/dialog/settings/`), a `QWidget` subclass with a single checkbox, following the `cleaning_settings_view_t` template and exposing `load(const settings_store_t &)` and `save(settings_store_t &) const`.
2.2 The checkbox has a tooltip (gui-tooltips rule) describing that the previous file version is copied to a `.bak` before each overwrite.
2.3 Each settings dialog (`translator_settings_dialog_t`, `editor_settings_dialog_t`) registers the page: one `addItem(tr("Backup"))` and one matching `addWidget(wrap_in_scroll_area(m_backup_view))` in the same position, plus `load` in the constructor and `save` in `apply_all`.
2.4 Panel padding follows the panel-padding rule; row/label conventions match the sibling pages. All strings wrapped for translation (`tr(...)`), consistent with the localization rule.
2.5 The two apps use the same INI key, same class name (`backup_settings_view_t` — same-named files in the two apps are allowed per naming-conventions), and the same checkbox label/tooltip text, per the consistent-across-apps rule.

### R3 — Backup mechanism (shared, before overwrite)

3.1 A shared, reusable file-backup helper is added (not duplicated per app). It performs: if the target path exists on disk, copy it to `<target>.bak` in the same directory, overwriting any existing `.bak`. If the target does not exist, it does nothing.
3.2 The helper lives in yampt.core io (`binary_file_io` is the file-IO home; a `backup_file(const std::string & path)` function there), keeping it usable from both the translator and editor save paths and from tests. Path handling uses the standard mechanisms (`std::filesystem` with `string_utils` UTF-8 ↔ path bridging where a `std::string` path crosses into `std::filesystem`), never manual separator concatenation.
3.3 The `.bak` name is exactly the original filename plus a `.bak` suffix (e.g. `Morrowind_en_pl.json` → `Morrowind_en_pl.json.bak`; `MyPlugin.esp` → `MyPlugin.esp.bak`), in the same folder as the original.
3.4 Backup failure (e.g. copy error) does not abort the save; it is logged (`[warning]`) and the save proceeds. The primary operation is the save; the backup is best-effort.

### R4 — yTranslator save integration

4.1 The backup is taken before the document's bytes are written over its path, gated by `backup_on_save()`.
4.2 It covers the real write paths: dictionary saves (`dict_writer_t::write` over `dict_document_t::m_path`) and YAML saves (`yaml_l10n_writer_t::write` over `yaml_document_t::m_path`). The design will specify the single cleanest place to hook it (per-document just before its writer call, or in the writer entry itself) so both document types are covered without duplicating logic and without backing up no-op documents.
4.3 No-op saves (`loc_document_t`, `eet_document_t`, non-dirty documents) do not create a `.bak`.

### R5 — yEditor save integration

5.1 The backup is taken before a source plugin is overwritten in `save_plugin`, gated by `backup_on_save()`: back up the plugin's own path, then `binary_file_io::write_file(...)` as today.
5.2 The merged-patch save (`patch_builder_t::save`) already writes via a temp file and renames over the destination. When backup is enabled, the destination merged patch (if it already exists) is backed up before it is replaced. The design will specify whether this reuses the same `backup_file` helper at the `save_merged_patch` boundary (cleanest, since the temp/rename dance is internal to `patch_builder_t`).
5.3 `save_all_dirty` produces one `.bak` per overwritten plugin (through `save_plugin`).

### R6 — No regression

6.1 With the setting off, save behavior is byte-for-byte identical to today in both apps (no `.bak`, no temp files added to the translator path).
6.2 The merged-patch temp-file/rename logic in `patch_builder_t::save` is unchanged; backup is layered at the caller boundary, not inside the atomic write.
6.3 Existing settings pages, their order, and their persistence are unchanged aside from the added Backup page.

### R7 — Verification

7.1 The backup helper is unit-testable (`[i]` integration tag, since it touches disk) in the system temp directory: given an existing file, `backup_file` produces a `.bak` with identical content and overwrites a prior `.bak`; given a missing file, it does nothing and reports no error. Temp files are cleaned up (per the unit/integration test rules — file I/O is integration-tagged, not `[u]`).
7.2 The setting getter/setter round-trips through `settings_store_t` (verified via the standard settings pattern).
7.3 Manual verification: with the toggle on, saving over an existing dictionary/plugin/merged patch yields a `.bak` holding the pre-save content; with the toggle off, no `.bak` appears.

## Open Decisions

Resolved:
- One shared setting under `Backup/OnSave`, default off. (R1)
- Backup format: single `<name>.<ext>.bak` next to the original, overwriting any prior `.bak` (one backup, always the immediately-previous version). (R3.3)
- Backup helper lives in `binary_file_io` (yampt.core), shared by both apps and tests. (R3.2)
- Backup is best-effort: failure logs a warning and does not abort the save. (R3.4)

Deferred to design:
- Exact hook point in the yTranslator save path (per-document before its writer call vs. inside the writer entry) so both dict and YAML are covered once (R4.2).
- Exact hook point for the merged-patch backup (at `save_merged_patch` boundary vs. `save_merge_to_file`) given the internal temp/rename in `patch_builder_t::save` (R5.2).
- Whether `backup_file` returns a status the caller logs, or logs internally.
