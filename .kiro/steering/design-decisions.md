# Design Decisions — yampt

## DictMerger Uses First-Wins Semantics

`mergeDict` iterates dictionaries in order. When a key already exists in the merged dict with a different value, the **first** dictionary's value is kept. Later dictionaries do NOT overwrite.

The log message says "Warning: replaced ..." and the counter is named `counter_replaced`, but these are misleading — the value is NOT replaced. The warning means "a later dictionary tried to provide a different value for this key, but it was ignored."

The test `"DictMerger merge first-wins precedence"` confirms this: when path1 has "FirstValue" and path2 has "SecondValue" for the same key, the result is "FirstValue".

Do NOT add `search->second = elem.second;` to the "different value" branch. The original behavior is correct.

## XML Dictionary Format Is Frozen

The XML dictionary format is obsolete, poorly structured, and must NOT be "fixed" or improved. It exists solely for backward compatibility with existing dictionary files. Do not refactor the XML tag names, structure, nesting, or escaping scheme. Any changes to the XML format would break all existing dictionaries in the wild.

New features (like JSON export) are the path forward. The XML format stays as-is.

## No std::regex — Use find + Manual Checks

MSVC's `std::regex` implementation is orders of magnitude slower than `string::find`. Do NOT use `std::regex` anywhere in yampt. All pattern matching must use `find`-based approaches:

- Word boundaries: `find(keyword)` then check `isalnum`/`_` on adjacent chars
- Quoted strings: `find('"')` pairs
- Token extraction: character-class loops with `isalnum`, `_`, `.`, `-`

This applies to new code AND to fixes for existing code. When fixing a bug that currently uses regex, replace the regex with find-based logic — do not "improve" the regex.

## esm_ref Is Always esm or esm_ext

`esm_ref` is a reference that points to either `esm` (in `--make` mode) or `esm_ext` (in `--make-base` mode). Both `esm` and `esm_ext` are always set and valid when the creator runs. Do not add null checks, optional wrappers, or validity guards around `esm_ref` usage.

## Use old_text for Records Without Unique IDs

For record types that don't have a unique composite ID (CELL, DIAL, SCTX, BNAM), always use `old_text` as the lookup/matching key — not `key_text`. The `old_text` field contains the original game text and is stable across dictionaries.

All other record types (INFO, FNAM, GMST, RNAM, DESC, INDX, TEXT) have unique `key_text` values and can use `key_text` for lookups.

## CELL Keys Are Always Hashes

All CELL dictionary entries use a 16-char hex FNV-1a hash as `key_text`, regardless of how the cell was matched:

- **Exterior cells**: hash of `GRID[x,y]` coordinate string
- **Interior cells (fingerprint)**: hash of the cell fingerprint (sorted DODTs + sorted ref IDs)
- **Heuristic/name-matched cells**: hash of the foreign cell name
- **Default (wilderness)**: hash of the `sDefaultCellname` value
- **Region names**: hash of the FNAM display name
- **Missing/unmatched**: hash of the cell name

The `key_text` is never used for lookup during conversion. The converter and script parser use `find_by_old_text()` — a secondary index on `old_text` (first-wins) — to find cell translations by the cell name text encountered in the ESM.

The `chapter_t::old_text_index` is populated on `insert()` and only stores the first entry for a given `old_text`. This means if multiple cells share the same foreign name, only the first inserted one is found by `find_by_old_text()`. This matches the original first-wins behavior.

## dict_creator Source File Split

The `dict_creator_t` class is split across multiple `.cpp` files:

- `dict_creator.cpp` — shared helpers used by multiple modes (constructors, `reset_counters`, `insert_duplicate`, `print_log_line`, `make_script_messages`, `differs_only_in_numbers_or_punct`, `adapt_translation`)
- `dict_creator_single.cpp` — single-file mode logic (`make_dict_single`, `insert_entry_single`, `insert_entry_single_with_base`, `insert_as_untranslated`, `insert_with_status`, `insert_via_text_match`)
- `dict_creator_base.cpp` — base mode logic (`make_dict_base`, `insert_entry_base`, cell matching, DIAL matching)
- `dict_creator_base_ordered.cpp` — ordered base mode logic (`make_dict_base_ordered`)

Rule: if a member function is called from more than one `.cpp` file, it belongs in `dict_creator.cpp`. Mode-specific functions stay in their respective files.
