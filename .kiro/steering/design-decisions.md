# Design Decisions — yampt

## DictMerger: Last-Listed Wins

The user always provides dictionaries from least important to most important: `Morrowind Tribunal Bloodmoon`. The **last** in the user's list has highest priority.

Internally, `dict_merger_t` iterates paths in **reverse order** (`rbegin`→`rend`), then uses first-wins insertion. This means the last-listed path is processed first and its keys win over earlier ones.

The reversal lives **solely inside the merger constructor**. No caller (CLI, GUI convert, GUI create, GUI make_dict_with_base) should reverse paths before passing them. All callers pass paths in user-facing order (least important → most important).

The log counter `counter_rejected` counts later dicts that tried to provide a different value for an existing key but were ignored (first-wins after reversal).

Do NOT add `search->second = elem.second;` to the "different value" branch. Do NOT reverse paths at call sites. Do NOT change the merger's internal iteration order.

## text_match_index_ — First Entry Wins

`build_text_match_index()` stores one entry per `old_text` (first-wins). It skips entries where `new_text == old_text` (untranslated). When multiple records in the merged base dict share the same `old_text` with different translations, the entry is marked as ambiguous.

Since the merger processes the last-listed dict first, its records appear first in each chapter's vector. This means the highest-priority dict also wins for text-match lookups — consistent with the key-based merge behavior.

### Ambiguous Status

When `build_text_match_index` encounters conflicting translations for the same `old_text`:
- `text_match_first_` stores the highest-priority translation (first encountered)
- `text_match_conflicts_` stores ALL translations separated by ` / ` (including the first)
- `insert_via_text_match` creates the entry with:
  - `new_text` = the highest-priority translation (from `text_match_first_`)
  - `status` = "ambiguous"
  - `adapted_from` = all translations joined by ` / ` (from `text_match_conflicts_`)

Ambiguous entries behave like untranslated during convert/create (the converter looks up by key, not by text_match). The adapted_from panel shows all options so the user can pick one.

## Annotation/Glossary Status Filter

The glossary in `annotation_manager_t::rebuild()` only includes entries with trusted statuses. Excluded statuses:
- `changed` — translation carried over but original text differs, may be wrong
- `ambiguous` — conflicting translations, unreliable
- `in_progress` — user is still editing
- `propagated` — auto-filled, may not be verified
- `model` — machine translated, not human-verified
- `error` — has validation issues

Included statuses (glossary sources): `translated`, `reused`, `adapted`.

DIAL topics (hyperlinks) have NO status filter — all DIAL entries contribute regardless of status.

## Glossary Sources

The glossary is built from these record types:
- DIAL — hyperlink topics (kind = `dial_topic`, blue highlight)
- FNAM — NPC/item display names (kind = `glossary_term`, green highlight)
- CELL — cell names (kind = `glossary_term`, green highlight)
- RNAM — race/faction rank names (kind = `glossary_term`, green highlight)
- INDX — skill/attribute names (kind = `glossary_term`, green highlight)

Glossary terms show for all record types. DIAL hyperlinks show for all record types.

## Forbidden Characters in Syntax Highlighter

The composite highlighter marks these characters with orange background (`255, 200, 180`):
- `|`, `~`, `@`, `{`, `}`
- Control characters (0x00–0x1F) except tab (0x09), CR (0x0D), LF (0x0A)

`"` is NOT highlighted — it's a valid character in script records (SCTX/BNAM) as a string delimiter.
`/` is NOT a forbidden character.

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

## CELL Keys Are Cell Names

All CELL dictionary entries use the foreign cell name as `key_text`. For text-keyed types (CELL, DIAL, SCTX, BNAM), the key is the original text itself — not a hash.

When multiple exterior cells share the same name (adjacent grid cells forming one area), `insert_entry_base` silently skips the duplicate insertion if `old_text` and `new_text` both match the existing entry. Only genuinely conflicting translations (different `new_text` for the same key) produce `duplicate` status.

The converter and script parser use `find_by_old_text()` — a secondary index on `old_text` (first-wins) — to find cell translations by the cell name text encountered in the ESM.

The `chapter_t::old_text_index` is populated on `insert()` and only stores the first entry for a given `old_text`. This means if multiple cells share the same foreign name, only the first inserted one is found by `find_by_old_text()`. This matches the original first-wins behavior.

## dict_creator Source File Split

The `dict_creator_t` class is split across multiple `.cpp` files in `yampt/model/`:

- `dict_creator.cpp` — shared helpers used by multiple modes (constructors, `reset_counters`, `insert_duplicate`, `print_log_line`, `make_script_messages`, `differs_only_in_numbers_or_punct`, `adapt_translation`)
- `dict_creator_single.cpp` — single-file mode logic (`make_dict_single`, `insert_entry_single`, `insert_entry_single_with_base`, `insert_as_untranslated`, `insert_with_status`, `insert_via_text_match`)
- `dict_creator_base.cpp` — base mode logic (`make_dict_base`, `insert_entry_base`, cell matching, DIAL matching)
- `dict_creator_base_ordered.cpp` — ordered base mode logic (`make_dict_base_ordered`)

Rule: if a member function is called from more than one `.cpp` file, it belongs in `dict_creator.cpp`. Mode-specific functions stay in their respective files.


## Identical-Text Entries Are Now Approved

Old behavior: when `make-dict` encountered a base entry where `old_text == new_text`, it assigned the `identical` status. Entries with `identical` were skipped during `--convert` and `--create` — proper nouns were not applied to the output.

New behavior: identical-text entries in base dicts receive `translated` (full mode, or partial mode when the English dictionary confirms a proper noun) or `untranslated` (partial mode when English words are detected). The `identical` status no longer exists.

When `make-dict` sees a base entry with `old_text == new_text`, it passes through the base entry's status directly:
- Base status `translated` → user dict entry is `translated` → applied during convert
- Base status `untranslated` → user dict entry is `untranslated` → skipped during convert

Net effect: `--convert` now includes proper noun entries (cells, NPCs, items with unchanged names) in the output, where before it would skip them. This produces more complete translations.

Legacy migration: old dictionaries with `identical` status are migrated to `translated` on load.

## dict_kind_t::base No Longer Gates Editing

`dict_kind_t` still exists and is used for display purposes (the `[BASE]` tag in the sidebar, golden color in selection dialogs). However, it no longer affects editing behavior:

- `dict_document_t::is_read_only()` always returns `false` regardless of `dict_kind_t`
- `commit_edit()` applies edits to all documents regardless of kind
- `session_t::save_all()` saves dirty documents regardless of kind

All dictionaries are saveable and editable in yTranslator. The base/user distinction is purely visual.
