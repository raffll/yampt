# GUI Decisions

## Spell Check Dictionary

Use the SJP-based Polish dictionary from `wooorm/dictionaries` (GitHub):
- Source: `https://github.com/wooorm/dictionaries/tree/main/dictionaries/pl`
- Files: `index.aff` → `pl_PL.aff`, `index.dic` → `pl_PL.dic`
- Encoding: `SET UTF-8` — matches our in-memory text (decoded to UTF-8)
- Size: ~4.7MB .dic, ~268KB .aff — comprehensive SJP coverage

Do NOT use:
- JetBrains/hunspell-dictionaries — incomplete, misses common Polish words
- LibreOffice/dictionaries pl_PL — uses `SET ISO8859-2`, encoding mismatch with our UTF-8 pipeline

## Dictionary Encoding Flow

1. ESM files contain raw Windows-1250 bytes
2. `dict_creator` + `dict_writer` write JSON with raw codepage bytes
3. GUI loads JSON via `dict_reader` (raw bytes survive yyjson with `ALLOW_INVALID_UNICODE`)
4. `decode_dict_from_codepage` converts to UTF-8 for display (ImGui is UTF-8-native)
5. On save, `save_user_dict_encoded` converts back to codepage before writing

Guard against double-decode: only decode if `workspace_.slot_count()` increased (new slot, not reused).

If a file is already UTF-8 (e.g. produced by integration tests), `looks_like_utf8` detection skips the decode.

## Config Duplicate Prevention

Always clear `config_.user_dict_paths` and `config_.base_dict_paths` before rebuilding them in shutdown. Previous bug: only base was cleared, causing user paths to accumulate on every run.

## RichEdit Line Ending Handling

RichEdit treats `\r\n` as a single character position. When building byte-to-wchar mappings for syntax/spelling highlighting, skip `\n` bytes that follow `\r` — otherwise token positions drift and coloring is applied to wrong characters.

## Sub-Type Filters

Sub-type filter bars (dialogue types, FNAM types, description types, index types) are always rendered at fixed height to prevent window size jumping. Buttons only appear when `type_filter_solo_` is true (user explicitly solo-selected a type). Sub-type filtering in `rebuild_row_data` is also gated on `type_filter_solo_`.

## Spell Checker Word Tokenization

Bytes >= 0x80 must be treated as word characters in `find_misspelled`. UTF-8 multi-byte sequences (Polish ą, ę, ś, etc.) would otherwise split words at diacritics, causing false positives.

## RichEdit Flickering — Unsolved

The RichEdit controls flicker when selecting text or during frame redraws. Attempted fixes that did NOT help:
- `SWP_NOZORDER` on subsequent `SetWindowPos` calls (only set z-order on first show)
- `WS_CLIPCHILDREN` on the parent SDL/HWND window
- `SW_SHOWNOACTIVATE` instead of `SWP_SHOWWINDOW`

The root cause is likely the OpenGL/ImGui render loop painting the area behind the RichEdit every frame before Windows composites the child window on top. This is a fundamental limitation of mixing Win32 child controls with an OpenGL rendering loop. Possible future solutions:
- Double-buffered RichEdit (not natively supported)
- Render RichEdit to a bitmap and draw it as an ImGui texture
- Replace RichEdit entirely with a custom ImGui text editor widget
- Use `WS_EX_COMPOSITED` on the parent (may conflict with OpenGL)

Do NOT attempt more `SetWindowPos` flag combinations — they have all been tried.

## Status Display Names

The Status column in the record table and the status filter bar buttons must always show a human-readable capitalized name, never the raw internal string. When adding a new status, update both `status_display_name()` in `record_table_model.cpp` and `get_status_display_name_qt()` in `status_filter_bar.cpp`.

Current mapping:
- `untranslated` → "Untranslated"
- `missing` → "Missing"
- `duplicate` → "Duplicate"
- `error` → "Error"
- `translated` → "Translated"
- `reused` → "Reused"
- `adapted` → "Adapted"
- `changed` → "Changed"
- `outdated` → "Outdated"
- `in_progress` → "In Progress"
- `model` → "AI Translated"
- `mismatch` → "Mismatch"
- `propagated` → "Propagated"
- `ambiguous` → "Ambiguous"

## Ambiguous Status Color

Amber/orange: `QColor(230, 180, 60)`.

## Book Preview — Live Update

The Book Preview tab updates the translation pane in real time as the user types in the translation editor (for TEXT records). The update happens inside `on_translation_changed()` — no F5 or manual refresh needed.

## Annotations Panel — Rebuild Button

A "Rebuild" button at the top of the Annotations tab triggers `rebuild_annotations()` and refreshes the current record. Use when loaded dicts change and annotations are stale.

## Annotations Panel — Source Display

Each annotation entry shows the source dict filename in brackets: `Small Chest → Mała skrzynia  [Morrowind_BASE_EN-PL.json]`. Only the filename is shown, not the full path.

## Sidebar — [UNLOADED] Tag

Workspace dict files (JSON/XML) that haven't been loaded into the session show `[UNLOADED]` as the first tag in the sidebar. The tag disappears when the dict is clicked and loaded. Order: `[UNLOADED] [BASE] [WIP] [XX] filename`.

## rebuild_table() Clears Editor Panels

Every exit path in `rebuild_table()` calls `clear_editor_panels()` after `editor_controller_.set_current_row(-1)`. This prevents stale text in the translation window when no row is selected after a table rebuild (filter change, search, status filter).

## Details Panel

The Details panel shows for ANY entry with non-empty `details` field (not just adapted/changed/ambiguous). For `translated` entries with method details (e.g. matched, fingerprint, heuristic), it shows the method name as plain text (no diff highlighting). Diff highlighting still only applies to `adapted` and `changed` statuses. For ambiguous entries, it displays all conflicting translations separated by ` / `.

## Tree Expansion State — Never Modify

Never call `expandAll()`, `collapseAll()`, or manually expand/collapse tree nodes as a side effect of any operation except `display_record`. Use `rebuild_nav_preserving_state()` which saves and restores the expansion state across rebuilds. The user's tree expansion is sacred — operations must not change it.

Default expansion:
- Left panel (nav tree): collapsed
- Right panel (view tree): expanded via `expandAll()` on each new record display

## QMenu Checkbox Indicator — Known Qt Bug

In Qt6 Fusion style (light theme), unchecked checkable menu actions have no visible indicator border — the checkbox area is invisible until checked. This is a known Qt limitation.

Attempted fixes that did NOT work:
- `QMenu::indicator:unchecked { border: 1px solid ... }` — breaks entire menu rendering
- `QPalette::Shadow` set to darker value — no effect on menu indicators
- `QCheckBox::indicator` stylesheet — targets wrong widget type, menu uses different rendering

The only viable fix would be a `QProxyStyle` overriding `drawPrimitive(PE_IndicatorMenuCheckMark)` — significant boilerplate for a cosmetic issue. Not worth implementing unless switching to a fully custom style.

Do NOT attempt stylesheet-based fixes for this — they all break the theme.
