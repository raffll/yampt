# GUI Plugin Operations — Decisions & Findings

## Sidebar Structure

The sidebar has two sections:
- **--- Loaded ---** — manually loaded items (via Open, Open Base, Load Plugin menus)
- **--- Workspace ---** / **--- FolderName ---** — auto-scanned from `workspace/` directory

Both sections behave identically for click/context menu. The only difference is persistence:
- Loaded items are saved to `yampt_gui.ini` and restored on startup
- Workspace items are auto-scanned on startup and after each operation

## Item Tags

- `[BASE]` — dict loaded via "Open Base" or filename contains `_BASE_`
- `[EN]`, `[PL]`, `[DE]`, `[FR]`, `[RU]` — language detected on ESM/ESP plugins via file size checksum
- `* ` prefix — dict has unsaved changes

## ESM Language Detection

Detection uses file size lookup against known vanilla master sizes. Instant — no file I/O beyond stat.

Known sizes:
- EN: 79837557, 9631798, 4565686
- DE: 80640776, 9797295, 6069165
- PL: 80105097, 9658076, 4626565
- FR: 80681814, 10015689, 4697358
- RU: 79857000, 9702000, 4625000

Only files named `morrowind.esm`, `tribunal.esm`, or `bloodmoon.esm` (case-insensitive) are checked.

## dict_creator_t Argument Order (Make Base)

`dict_creator_t(path, path_ext)`:
- `path` (first arg) = "to" language → its values become `new_text`
- `path_ext` (second arg) = "from" language → its values become `old_text`

GUI/CLI pass: `dict_creator_t(native_path, foreign_path)` so that `old_text`=foreign, `new_text`=native.

User intent: "right-click EN, select PL" → EN-to-PL dict. Internally: `dict_creator_t(PL, EN)`.

## Make Base Output Naming

Format: `{ForeignName}[+{NativeName}]_BASE_{FL}-{NL}-{timestamp}.json`

- ForeignName = right-clicked plugin filename without extension
- NativeName = selected plugin filename without extension (only if different from foreign)
- FL/NL = detected language codes, or `XX` if unknown
- timestamp = YYYYMMDDHHmmss

## Dict Merger Order (Convert/Create)

`dict_merger_t` uses first-wins semantics. The GUI reverses the user-selected list before passing to the merger so that the LAST item in the selection dialog wins (overrides earlier ones).

User sees: Morrowind, Tribunal, Bloodmoon (top to bottom).
Internally reversed: Bloodmoon first → wins over Tribunal → wins over Morrowind.

## esm_converter_t Error Handling

Use `converter.is_loaded()` to gate output writing, NOT `tools_t::has_error()`. The converter can log `[error]` messages for individual records without it being fatal. The CLI uses `is_loaded()` and so does the GUI.

## tools_t Debug Flag

`tools_t::set_debug(true)` enables silent log messages (script parser traces). Without it, `add_log(msg, true)` discards the message entirely — not appended to log1.

CLI: `--debug` flag enables it.
GUI: never enabled — script parser traces don't appear in the Log tab.

## Workspace Dict Loading

When a workspace json/xml is clicked:
1. Check if already loaded (by path) — if yes, just switch to it
2. If not loaded: `workspace_.load_dict(path, kind)` where `kind` = `dict_kind_t::base` if filename contains `_BASE_`, otherwise `dict_kind_t::user`
3. No `rebuild_sidebar()` call — workspace dicts don't appear in the Loaded section

## Workspace Dicts Are Not Persisted

Dicts whose path starts with the workspace directory are excluded from:
- The "--- Loaded ---" sidebar section
- The `yampt_gui.ini` config persistence

They only appear in workspace sections and are auto-discovered.

## Filter State Is Global

One `type_filter_` and `status_filter_` apply to all dicts. Switching dicts does NOT save/restore per-dict filter state. The filter bar stays as-is.

## Status Filter Bar Fixed Height

`setFixedHeight(26)` prevents collapse when no status buttons are visible.

## sub_type_bar_ Ghost Widget (Fixed)

The old code created `sub_type_bar_ = new QWidget(this)` without adding it to any layout. It sat at (0,0) and stole clicks from the "All" button. Removed.

## Status Colors

Single source of truth: `status_colors.hpp` with `get_status_color()` inline function. Used by both `record_table_model.cpp` and `status_filter_bar.cpp`.

## Context Menu Structure

Plugin items (Loaded section, kind=2):
```
Make Dict
Make Dict with Base
Make Base
---
Convert
Create
---
Unload
```

Workspace ESM/ESP items (kind=3):
```
Make Dict
Make Dict with Base
Make Base
---
Convert
Create
---
Delete
```

Workspace JSON/XML items (kind=3):
```
Delete
```

Dict items (Loaded section, kind=0 or 1):
```
Save
Save As...
---
Unload
```

## Make Base Dialog

Shows a QListWidget (not combobox) of all loaded plugins except the right-clicked one. Pre-selects the item with matching filename (case-insensitive). Double-click accepts.
