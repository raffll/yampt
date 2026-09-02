# Implementation Plan

## Overview

Add an opt-in "backup on save" feature to both yTranslator and yEditor: before a save overwrites an existing file, copy it to `<name>.bak` in the same folder. One shared boolean setting (`Backup/OnSave`, default off) drives a new "Backup" settings tab in both apps. The core work is a single shared, best-effort `binary_file_io::backup_file` helper (yampt.core, no Qt), hooked in at each app's overwrite point: the yTranslator session before it delegates to a writing document's `save()`, and the yEditor `merge_controller_t::save_plugin` / `save_merged_patch`. With the setting off, behavior is byte-for-byte identical to today. Work order: setting + core helper first (independent), then the settings pages and their dialog registration, then the two save-path hooks, then tests and docs.

## Tasks

- [ ] 1. Add backup_on_save setting to settings_store_t
  - `backup_on_save()` / `set_backup_on_save(bool)`, INI key `Backup/OnSave`, default false (mirror `clean_evil_gmst_enabled`).
  - _Requirements: R1.1, R1.2, R1.3_

- [ ] 2. Add binary_file_io::backup_file helper
  - `bool backup_file(const std::string & path)` in yampt.core io: if `path` exists, `std::filesystem::copy_file` to `path + ".bak"` with `overwrite_existing`; if missing, return false and do nothing.
  - Use `string_utils::utf8_to_path` for the filesystem bridge; use `std::error_code` (never throw).
  - On copy error log `[warning]` and return false; on success log `[info]` and return true.
  - _Requirements: R3.1, R3.2, R3.3, R3.4_

- [ ] 3. Create the Backup settings page in yTranslator
  - `backup_settings_view_t` in `yampt.translator/source/dialog/settings/`: one group box, one checkbox `tr("Back up files before overwriting")` with a tooltip; `load`/`save` against `backup_on_save()`.
  - Add files to `yampt.translator.vcxproj` + `.filters`.
  - _Requirements: R2.1, R2.2, R2.4_

- [ ] 4. Create the Backup settings page in yEditor
  - Identical `backup_settings_view_t` in `yampt.editor/source/dialog/settings/` (same label, tooltip, INI key).
  - Add files to `yampt.editor.vcxproj` + `.filters`.
  - _Requirements: R2.1, R2.2, R2.4, R2.5_

- [ ] 5. Register the Backup page in both settings dialogs
  - `translator_settings_dialog_t` and `editor_settings_dialog_t`: construct the view, `addItem(tr("Backup"))` + matching `addWidget(wrap_in_scroll_area(...))`, `load` in constructor, `save` in `apply_all`.
  - _Requirements: R2.3_

- [ ] 6. Hook backup into the yTranslator save path
  - Give `session_t` a `set_backup_on_save(bool)` and cache the flag (set from `settings.backup_on_save()` at startup and when the settings dialog applies), keeping `session_t` free of the Qt settings type.
  - In `save_all()`, before delegating to each dirty document that will actually write, call `binary_file_io::backup_file(doc->path())`.
  - Add a `document_t::will_write() const` predicate (or reuse each document's existing early-return conditions) so no `.bak` is created for no-op saves (loc/eet/non-writing yaml).
  - _Requirements: R4.1, R4.2, R4.3_

- [ ] 7. Hook backup into the yEditor save path
  - In `merge_controller_t::save_plugin`, before `binary_file_io::write_file`, if `m_settings.backup_on_save()` call `backup_file(plugin_path)`.
  - In `save_merged_patch`, after resolving the output path and before `save_merge_to_file`, if enabled call `backup_file(output_path)`.
  - Leave the `patch_builder_t::save` temp-file/rename logic untouched.
  - _Requirements: R5.1, R5.2, R5.3, R6.2_

- [ ] 8. Integration-test backup_file
  - `[i]`: existing file → `.bak` with identical bytes; second save → `.bak` overwritten with the newer pre-save content; missing file → returns false, creates nothing. Use the system temp dir and clean up.
  - Add any new test file to `yampt.tests.vcxproj` + `.filters`.
  - _Requirements: R7.1_

- [ ] 9. Update documentation
  - CHANGELOG `[NEW]` (Both Apps): backup-to-.bak before overwriting on save, toggled in Settings → Backup, off by default.
  - `docs/yTranslator-Manual.md` and `docs/yEditor-Manual.md`: describe the Backup setting and the `.bak` behavior.
  - README + README.bbcode in sync if they list features/settings.
  - _Requirements: R1, R2, R3_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1, 2], "depends_on": [] },
    { "wave": 2, "tasks": [3, 4], "depends_on": [1] },
    { "wave": 3, "tasks": [5, 6, 7], "depends_on": [1, 2, 3, 4] },
    { "wave": 4, "tasks": [8, 9], "depends_on": [2, 5, 6, 7] }
  ]
}
```

The setting (1) and the core helper (2) are independent and come first. The two settings pages (3, 4) need only the setting. Dialog registration (5) needs the pages; both save hooks (6, 7) need the setting and the helper. Tests (8) need the helper; docs (9) last.

## Notes

- Default is off (`Backup/OnSave` false) so no `.bak` files appear for existing users after an update; behavior is byte-for-byte identical with the setting off (R6.1).
- The backup helper lives in yampt.core (`binary_file_io`) so both apps and tests share one implementation; the Qt setting is read only in the app layer (session / merge controller), never in yampt.core (dependency rules).
- Backup is best-effort: a copy failure logs a warning and the save proceeds. `backup_file` never throws.
- First save of a new file makes no `.bak` — `backup_file` no-ops when the target does not yet exist (covers first-time merged-patch generation).
- The merged-patch atomic temp/rename write in `patch_builder_t::save` is not modified; the backup is layered at the caller boundary.
- Building and running tests is done manually by the user (no-build-or-test rule).
