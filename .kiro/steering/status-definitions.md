# Status Definitions

## Conversion Behavior

Only entries with **approved** statuses are applied during `--convert` and `--create` operations. All other entries are skipped — the original plugin text stays unchanged.

Approved statuses:
- `translated`
- `matched`, `fingerprint`, `coords`, `heuristic`, `exact`, `info`, `wilderness`, `region`

Skipped statuses:
- `identical`, `adapted`, `reused`, `changed`, `ambiguous`
- `in_progress`, `model`, `propagated`
- `untranslated`, `missing`, `duplicate`, `mismatch`, `error`

## Base Mode Statuses

Assigned by `make-base` when comparing two language versions of the same file.

| Status | Meaning | old_text | new_text | adapted_from |
|--------|---------|----------|----------|--------------|
| `matched` | Paired by key or fingerprint | foreign text | native text | — |
| `fingerprint` | Interior cell matched by door fingerprint | foreign cell name | native cell name | — |
| `coords` | Exterior cell matched by grid coordinates | foreign cell name | native cell name | — |
| `heuristic` | Matched via translation engine word overlap | foreign name | native name | — |
| `exact` | Heuristic tie resolved (all candidates identical) | foreign name | native name | — |
| `info` | INFO matched via DIAL+INAM composite key | foreign text | native text | — |
| `wilderness` | sDefaultCellname GMST | foreign name | native name | — |
| `region` | Region display name (FNAM of REGN) | foreign name | native name | — |
| `missing` | Foreign record exists but no native match found | foreign text | foreign text | native candidates (pipe-separated) |
| `duplicate` | Multiple records share the same key | first foreign text | first native text | alternative translations (pipe-separated) |
| `mismatch` | Native record exists but no foreign match (extra native content) | empty | native text | — |

## Single Mode Statuses

Assigned by `make-dict` when creating a dictionary from a plugin using a base dict.

| Status | Meaning | old_text | new_text |
|--------|---------|----------|----------|
| `translated` | Base dict has a real translation (old ≠ new) | plugin text | translation from base |
| `identical` | Base dict has old == new (proper noun or untranslated pass-through) | plugin text | same as old |
| `adapted` | Matched by old_text via text_match_index (different key) | plugin text | adapted translation |
| `changed` | Key matches base but old_text differs (source text changed between versions) | plugin text | base translation (may be outdated) |
| `reused` | Matched by old_text in text_match_index (first-wins) | plugin text | reused translation |
| `ambiguous` | Multiple conflicting translations for same old_text in base | plugin text | highest-priority translation |
| `untranslated` | No match in base dict at all | plugin text | plugin text |

## User/GUI Statuses

Assigned by user actions in yTranslator.

| Status | Meaning |
|--------|---------|
| `in_progress` | User started editing but hasn't finalized |
| `model` | Translated by the CTranslate2 translation model |
| `propagated` | Translation auto-filled from another record with same old_text |
| `error` | Entry has a validation error |

## adapted_from Field

The `adapted_from` field uses `|` (pipe) as separator for multiple values. The GUI displays each value on a separate line.

| Status | adapted_from contains |
|--------|----------------------|
| `adapted` | The base translation that was modified to produce the adaptation |
| `changed` | The original old_text from the base (what it used to be) |
| `ambiguous` | All conflicting translations |
| `missing` | All unmatched native candidates (for DIAL/CELL) |
| `duplicate` | All alternative translations from duplicate records |

## Auto-Save Before Operations

All operations (make-dict, make-base, convert, create) prompt the user to save unsaved changes before proceeding. This prevents silent data loss when converting with stale dict files.
