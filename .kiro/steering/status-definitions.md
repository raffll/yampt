# Status Definitions

## Conversion Behavior

Only entries with status `translated` are applied during `--convert` and `--create` operations. All other entries are skipped — the original plugin text stays unchanged. No exceptions.

Approved status:
- `translated`

Skipped statuses:
- `to_verify`, `untranslated`, `adapted`, `reused`, `changed`, `outdated`, `ambiguous`
- `in_progress`, `model`, `propagated`
- `missing`, `duplicate`, `mismatch`, `heuristic`, `error`

The `details` field has no effect on conversion — only `status` determines whether an entry is applied.

## Make-Base Statuses

Assigned by `make-base` when comparing two language versions of the same file. All successfully matched entries receive status `translated`. The matching method is stored in the `details` field as metadata.

| Status | Meaning | old_text | new_text | details |
|--------|---------|----------|----------|---------|
| `translated` | Successfully matched | foreign text | native text | method (matched, fingerprint, coords, heuristic, exact, info, wilderness, region) |
| `missing` | Foreign record exists but no native match found | foreign text | foreign text | native candidates (pipe-separated) |
| `duplicate` | Multiple records share the same key | first foreign text | first native text | alternative translations (pipe-separated) |
| `mismatch` | Native record exists but no foreign match (extra native content) | empty | native text | — |

### Full Mode (default)

All entries get status `translated` regardless of whether old_text equals new_text. Identical-text entries are treated as proper nouns that are correct as-is.

Use for fully translated ESMs where old==new means the word is the same in both languages (proper noun).

### Partial Mode (`--partial` CLI flag / GUI radio button)

Entries where old_text differs from new_text get status `translated`.

Entries where old_text equals new_text are checked against an English Hunspell dictionary (`dictionaries/en_US.aff` + `dictionaries/en_US.dic`):
- Text is tokenized by non-alphanumeric characters; tokens shorter than 3 characters are ignored
- If no tokens are found in the English dictionary → `to_verify` (likely a proper noun, needs user confirmation)
- If any token is found in the English dictionary → `untranslated` (contains English — needs translation)

Use for partially translated ESMs (e.g. EET-imported dictionaries) where old==new may mean the entry was never translated.

## Single Mode Statuses

Assigned by `make-dict` when creating a dictionary from a plugin using a base dict.

| Status | Meaning | old_text | new_text |
|--------|---------|----------|----------|
| `translated` | Base dict has a translation (old ≠ new), or base entry has status `translated` with old == new (proper noun) | plugin text | translation from base (or same as old for proper nouns) |
| `untranslated` | No match in base dict, or base entry has status `untranslated` with old == new (partial mode flagged) | plugin text | plugin text |
| `adapted` | Matched by old_text via text_match_index (different key) | plugin text | adapted translation |
| `changed` | Key matches base but old_text differs (source text changed between versions) | plugin text | base translation (may be outdated) |
| `reused` | Matched by old_text in text_match_index (first-wins) | plugin text | reused translation |
| `ambiguous` | Multiple conflicting translations for same old_text in base | plugin text | highest-priority translation |

For identical-text entries (base has old==new), the base entry's status is passed through directly. If the base says `translated` (proper noun in full mode), the user dict entry is `translated`. If the base says `to_verify` (partial mode, no English words found), the user dict entry is `to_verify`. If the base says `untranslated` (partial mode, English words found), the user dict entry is `untranslated`.

## User/GUI Statuses

Assigned by user actions in yTranslator.

| Status | Meaning |
|--------|---------|
| `in_progress` | User started editing but hasn't finalized |
| `model` | Translated by the CTranslate2 translation model |
| `propagated` | Translation auto-filled from another record with same old_text |
| `error` | Entry has a validation error |

## Translate Button Behavior

The Translate button always sets the entry status to `model` after a successful translation. This applies regardless of document type (dict or YAML). The status is set via `set_pending_status(status_t::model)` before the translation call, and `commit_current_edit()` applies it immediately after the result is placed in the editor.

## Details Field

The `details` field (renamed from `adapted_from`) uses `|` (pipe) as separator for multiple values. The GUI displays each value on a separate line in the Details panel.

The details panel is shown for any entry with a non-empty `details` field. Diff-highlighting (marking differences between old_text and the adaptation source) applies only to entries with status `adapted` or `changed`.

| Context | details contains |
|---------|-----------------|
| Base-mode entries | Match method: `matched`, `fingerprint`, `coords`, `heuristic`, `exact`, `info`, `wilderness`, `region` |
| `missing` entries | All unmatched native candidates (for DIAL/CELL) |
| `duplicate` entries | All alternative translations from duplicate records |
| `adapted` entries | The base translation that was modified to produce the adaptation |
| `changed` entries | The original old_text from the base (what it used to be) |
| `outdated` entries | The original old_text from the base |
| `ambiguous` entries | All conflicting translations |

## Legacy Migration

No migration or backward compatibility code. This is pre-1.0 software — old dictionary formats are not supported. If a dict has obsolete statuses or fields, it must be regenerated.

Do NOT add migration paths, version checks, or fallback reads for old formats.

## Auto-Save Before Operations

All operations (make-dict, make-base, convert, create) prompt the user to save unsaved changes before proceeding. This prevents silent data loss when converting with stale dict files.
