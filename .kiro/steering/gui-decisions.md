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
