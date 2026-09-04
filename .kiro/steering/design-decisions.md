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

Both glossary terms and DIAL topics (hyperlinks) are built only from entries with status `translated`. Every other status — `changed`, `ambiguous`, `in_progress`, `propagated`, `model`, `reused`, `adapted`, `error`, `untranslated` — is excluded. Unverified translations do not contribute topic links or glossary terms.

This applies to both `glossary_t::collect_dial_entries` (hyperlinks) and `glossary_t::collect_glossary_entries` (FNAM/CELL/RNAM/INDX terms): each skips any entry whose `status != status_t::translated`.

Inflection annotations are separate — they come from loaded `.top`/`.mrk` localization files via `inflection_store_t`, not from dictionary record status.

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

The `dict_creator_t` is a thin facade in `yampt.core/source/creator/`. The actual logic is split into strategy classes:

- `dict_creator.hpp/.cpp` — Thin facade, picks strategy based on mode detection
- `creator_context.hpp` — Shared state struct (ESM readers, indexes, counters, dict)
- `creator_helpers.hpp/.cpp` — All shared logic (insert methods, index builders, script parsing, adapt_translation, determine_status)
- `creator_single.hpp/.cpp` — Single-file mode strategy
- `creator_base.hpp/.cpp` — Unordered base mode strategy
- `creator_ordered.hpp/.cpp` — Ordered base mode strategy

Each mode is its own class with its own `.hpp/.cpp` pair.


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


## Codepage Lists Must Be Sorted

Whenever codepages are listed in a combo box or UI element, they must be in ascending numeric order (e.g. 1250, 1251, 1252). Never put a "default" or "most common" codepage first if it breaks numeric ordering.


## Copy to Merged Patch — Individual Sub-Records Always Allowed

Never restrict the user to copying only entire groups. Individual sub-record copy must always be available, even for sub-records that belong to a group. The merge operation must handle placing the sub-record at the correct position based on its content identity, not by occurrence order. If a sub-record belongs to a group that doesn't exist in the merge yet, the merge must create the appropriate structure to receive it — not silently misplace it or force the user to copy the whole group first.


## SCVR Sub-Record: No Null Terminator Needed

`convert_scvr` does NOT append `'\0'` to the replacement text. This is correct — not a bug.

SCVR is a structured binary field (5-byte prefix + variable/cell name), not a simple null-terminated string sub-record like NAME or DNAM. OpenMW's reader uses `strnlen(ptr, size)` which handles both null-terminated and non-null-terminated data. OpenMW's writer (`writeHString`) also writes SCVR without a null terminator.

Do NOT "fix" this by adding `new_text += '\0'` to `convert_scvr`.


## esm_reader_t::scan_sub_records — Break on Zero-Size Is Correct

`scan_sub_records` breaks the scan loop when `found_size == 0`. This is a defensive guard against corrupt data, not a bug.

In TES3 format, no legitimate sub-record has zero size — `DELE` is 4 bytes, all others have positive sizes. A zero in the size field means the record data is malformed. Breaking prevents parsing garbage as valid sub-records.

If we used `continue` (advancing by `sub_record_header_size + 0 = 8`), we'd still advance but would be reading into corrupted territory. Breaking is the safer choice — the record is already broken, so nothing useful follows.

Do NOT "fix" this by replacing `break` with `continue` or `scan_pos += sub_record_header_size`.


## check_all_identical Duplication in view_tree_decode Files

`view_tree_decode.cpp` and `view_tree_decode_cell.cpp` both define a file-local `static bool check_all_identical(...)`. This is a consequence of the allowed class-split exception for `view_tree_model_t` — `static` functions cannot be shared across translation units. Both copies are used. Accepted as-is.

## spell_checker_t::is_excluded — Linear Scan Is Acceptable

`is_excluded` does a linear scan over `m_excluded_words` for each word during spell checking. The exclusion list is small (50–200 entries) and Hunspell's `spell()` call dominates the cost. The linear scan is noise. Accepted as-is — no hash set needed.


## Copy Original vs Reset to Original — Two Distinct Operations

- **Copy Original (F8)** — copies `old_text` into `new_text` and sets status `in_progress`. The user intends to start editing from the original as a base. Uses `document_t::commit()` with `in_progress` intent.
- **Delete/Clear (Del key)** — copies `old_text` into `new_text` and sets status `untranslated`. The user intends to discard the translation entirely. Uses `document_t::reset_to_original()`.

Both produce the same `new_text` but different statuses. The distinction matters because only `untranslated` entries are eligible for the Translate button, while `in_progress` entries are not.


## Translate Button Error Feedback — append_log Is Sufficient

When the Translate button is clicked with invalid state (no document, no row, non-untranslated entry), error messages are written via `m_translation_tab->append_log(...)`. This is not a visibility problem because the Translate button itself lives on the Auto Translate tab — if the user can click it, they can see the feedback. No status bar message needed.


## Plugin Icons Must Be Consistent Across Panels

The navigation tree (left panel) and the record view column headers (right panel) must show the same icon for each plugin. The icon logic lives in two places — `nav_tree_model.cpp::display_text_for_file` and `view_tree_model.cpp::headerData` — and must produce identical results for the same plugin index. When adding or changing an icon, update both locations.

Icon priority (first match wins):
1. 🔒 — excluded from merged patch
2. 🛡 — guard patch
3. ⚙ — merged patch
4. ✍ — editing enabled
5. 📜 — master file (.esm)
6. ⚡ — loaded from MO2 overwrite folder
7. 📄 — regular plugin (default)


## Record View Header: Use CE_HeaderSection, Draw Text Manually

`record_colored_header_t::paintSection` (yampt.editor/source/view/record_view.cpp) draws each plugin column header (icon + filename, in a per-plugin conflict color). It MUST paint the background with `QStyle::CE_HeaderSection` (background/border only) and then draw the text itself with `painter->drawText` into the section `rect`.

Do NOT use `QStyle::CE_Header` here. `CE_Header` draws both the section background AND the label; on the Windows style this results in the header text rendering blank for the plugin columns (confirmed: `headerData` returned the correct text, e.g. `"📜 TR_Mainland.esm"` len=18, but nothing appeared on screen). `CE_Header` combined with `SE_HeaderLabel` for the text rect produced invisible text.

Do NOT compute the text rectangle via `style()->subElementRect(QStyle::SE_HeaderLabel, ...)`. Draw into the passed-in `rect` (with a small left inset, e.g. `rect.adjust(4, 0, -4, 0)`). Drawing directly into `rect` is the coordinate space that reliably renders (verified by a fill-rect probe that showed all sections paint correctly).

The header text/color come from `view_tree_model_t::headerData` (DisplayRole for the icon+name string, ForegroundRole for the conflict color); fall back to `palette().color(QPalette::ButtonText)` when ForegroundRole is invalid (e.g. section 0). The model side was never the problem — the bug was purely in how the section was painted.

This only manifested with more than one plugin column, because a single plugin column is the stretched last section and happened to render, masking the issue.


## Propagation Marks the Source Entry as `propagated` Too — By Design

`dict_document_t::commit(row, new_text, intent)` sets the edited entry's status to `intent`, then calls `propagate(old_text, new_text)`. When propagation touches one or more sibling records (same `old_text`), the code deliberately overwrites the source entry's status with `propagated` as well:

```cpp
if (result.propagated_count > 0)
    entry.status = status_t::propagated;
```

This is intentional, NOT a bug. When a translation propagates, the user wants the entire set of identical records — source included — marked `propagated` so they read as one consistent propagated group. The source is not exempt.

Do NOT "fix" this by preserving the source entry's `intent` status when propagation occurs. Do NOT report it as "propagation overwrites user intent on the source entry." The overwrite is the desired behavior.

Note: when `propagated_count == 0` (no siblings shared the `old_text`), the source keeps `intent` — the overwrite only happens when propagation actually occurred.
