# Dictionary Entry Statuses

Every entry in a yampt dictionary has a status that tells you how it was created, whether it's ready for use, and what action (if any) is needed.

## Which statuses are used during conversion?

When you run Convert or Create, only **approved** entries are applied to the plugin. Everything else is skipped — the original text stays unchanged.

**Applied:**
- Translated
- Matched
- Heuristic

**Skipped:**
- Identical, Adapted, Reused, Changed, Outdated, Ambiguous
- In Progress, Model, Propagated
- Untranslated, Missing, Duplicate, Mismatch, Error

This means you can safely work on a partial translation — only finished entries affect the output.

---

## Base dictionary statuses

These appear when you create a base dictionary by comparing two language versions of the same ESM/ESP.

| Status    | What it means                                                    | What to do                                                   |
|-----------|------------------------------------------------------------------|--------------------------------------------------------------|
| Matched   | Successfully paired between both files                           | Ready to use                                                 |
| Heuristic | Cell/topic matched by translation engine                         | Review — usually correct                                     |
| Missing   | Foreign text exists but couldn't find a native match             | Check the suggested candidates in the panel below the editor |
| Duplicate | Same key appeared multiple times with different translations     | Pick the correct one from the alternatives shown below       |
| Mismatch  | Extra content in the native file that has no foreign counterpart | Can be ignored — not useful for translation                  |

## Translation statuses

These appear when you create a dictionary from a plugin using a base dictionary.

### First time — using a base dictionary

You have a base dictionary (from make-base) and create a user dictionary for a plugin.

| Status       | What it means                                               | What to do                                             |
|--------------|-------------------------------------------------------------|--------------------------------------------------------|
| Translated   | Translation found in base dict                              | Ready to use                                           |
| Identical    | Base dict has original = translation (e.g. "Dagoth Ur")     | Review — proper noun (correct) or untranslated (fix)   |
| Adapted      | Source differs from base only in numbers/punctuation        | Review the auto-adjusted translation                   |
| Changed      | Source differs from base, translation carried over          | Update the translation for the new source              |
| Reused       | Same original text found in another entry                   | Review — translation may fit                           |
| Ambiguous    | Multiple conflicting translations found for the same text   | Pick the correct one from alternatives shown below     |
| Untranslated | No match found anywhere in the base                         | Needs translation                                      |

### Updating — using old user dict as base

You already have a user dictionary from a previous version and re-run make-dict for the updated plugin.

| Status       | What it means                                                      | What to do                                                   |
|--------------|--------------------------------------------------------------------|--------------------------------------------------------------|
| Translated   | Source unchanged, translation preserved                            | Nothing — still ready                                        |
| Identical    | Source unchanged, original = translation                           | Nothing — same review as before                              |
| Adapted      | Source differs only in numbers/punctuation, auto-adjusted          | Review — usually correct                                     |
| Changed      | Source changed, approved translation carried over                  | Update the translation for the new source                    |
| Reused       | New entry, same text found in another approved entry               | Review — translation may fit                                 |
| Ambiguous    | New entry, multiple conflicting translations for the same text     | Pick the correct one from alternatives shown below           |
| Untranslated | New entry with no match, or source changed and nothing to preserve | Needs translation                                            |
| In Progress  | Source unchanged, unfinished work preserved                        | Continue where you left off                                  |
| Model        | Source unchanged, machine translation preserved                    | Review and approve or retranslate                            |
| Propagated   | Source unchanged, auto-filled translation preserved                | Review and approve                                           |
| Error        | Source unchanged, entry with validation error preserved            | Fix the validation issue                                     |
| Outdated     | Source changed, unfinished translation carried over                | Fix — it was in progress when source changed                 |

## Work-in-progress statuses

These are set by your actions in yTranslator.

| Status      | What it means                                              |
|-------------|------------------------------------------------------------|
| In Progress | You started editing but haven't finalized                  |
| Model       | Machine-translated by the built-in translation engine      |
| Propagated  | Auto-filled from another entry with the same original text |
| Error       | The entry has a validation problem (check the indicator)   |

---

## Changed vs Adapted vs Outdated — detailed explanation

All three statuses mean the source text in the new plugin differs from what the base dictionary had.

### Changed

The source text changed and the base had an **approved** translation. The old translation is carried forward for reference.

Example: Base had `"The guard looks at you."` → `"Strażnik patrzy na ciebie."` (status: Translated). New plugin has `"The guard glares at you."`.

What you see:
- **Original**: `"The guard glares at you."`
- **Translation**: `"Strażnik patrzy na ciebie."` — old translation, probably needs updating
- **Suggestions panel**: `"The guard looks at you."` — old source for reference

What to do: Rewrite the translation to match the new original, then change status to Translated.

### Adapted

The source text differs only in numbers or punctuation, and the base had an approved translation. The tool automatically swaps those values.

Example: Base had `"You need 500 gold."` → `"Potrzebujesz 500 złota."`. New plugin has `"You need 1000 gold."`.

What you see:
- **Original**: `"You need 1000 gold."`
- **Translation**: `"Potrzebujesz 1000 złota."` — auto-adjusted
- **Suggestions panel**: the original base translation before adaptation

What to do: Usually correct — just verify. If wrong, edit manually.

### Outdated

The source text changed and the base had an **unfinished** translation (In Progress, Model, or Error). Your incomplete work is preserved but flagged.

Example: You were editing `"Find the cave"` → `"Znajdź jask..."` (status: In Progress). The mod updates to `"Find the hidden cave"`.

What you see:
- **Original**: `"Find the hidden cave"`
- **Translation**: `"Znajdź jask..."` — your incomplete work, now based on outdated source
- **Suggestions panel**: `"Find the cave"` — old source for reference

What to do: Finish the translation based on the new source text.

---

## Status preservation across plugin updates

When you create a new dictionary for an updated plugin using your old dictionary as the base, statuses are preserved:

- **Key matches, source text unchanged** → status and translation kept exactly as they were.
- **Key matches, source text changed, old status was approved** → `Changed` (old translation kept for reference).
- **Key matches, source text changed, old status was In Progress/Model/Error** → `Outdated` (incomplete translation kept, flagged for review).
- **Key matches, source text changed, old status was Untranslated/Identical** → `Untranslated` (nothing to preserve, fresh start).
- **Key matches, only numbers/punctuation differ, old status approved** → `Adapted` (translation auto-adjusted).
- **New entries not in old dict** → `Untranslated`.
- **Old entries no longer in the plugin** → not included in the new dict. If the record was renamed (different ID, same text), it will appear as `Reused` under the new ID. If the text was genuinely removed, it simply won't be in the new dictionary — your old dict still has it if you need to reference it.

This means your work is never lost when a mod updates. Translated entries stay translated, in-progress entries are flagged as outdated if the source changed, and only genuinely new text starts as untranslated.

---

## Status transitions

How statuses change as entries move through the workflow.

### Creating a base dictionary (make-base)

```
Two ESMs compared
       │
       ├── Record paired successfully ──────────── Matched / Heuristic
       ├── Foreign exists, no native match ─────── Missing
       ├── Native exists, no foreign match ─────── Mismatch
       └── Same key, different translations ────── Duplicate
```

### Creating a user dictionary (make-dict with base)

```
Plugin entry + base dict
       │
       ├── Key found, old == new
       │      ├── Base status is user status ───── preserved (Untranslated / In Progress / etc.)
       │      └── Base status is base status ───── Identical
       │
       ├── Key found, old matches, old ≠ new
       │      ├── Base status is user/translated ── preserved (Translated / In Progress / etc.)
       │      └── Base status is base status ───── Translated
       │
       ├── Key found, old differs, base approved
       │      ├── Numbers/punctuation only ─────── Adapted
       │      └── Text changed ─────────────────── Changed
       │
       ├── Key found, old differs, base not approved
       │      ├── Was In Progress/Model/Error ──── Outdated (work preserved)
       │      ├── Was Untranslated/Identical ───── Untranslated (fresh start)
       │      └── Was Propagated/other ─────────── Changed (discarded)
       │
       ├── Key not found, approved text match ──── Reused / Ambiguous
       └── Key not found, no match ─────────────── Untranslated
```

Note: text matching only considers entries with approved statuses
(Translated, Matched, Heuristic). Entries with Untranslated,
In Progress, Model, etc. are not used as sources for reuse.

### User actions in yTranslator

```
Any status ──── user edits translation ──────────── In Progress
Any status ──── user accepts / finalizes ─────────── Translated
Any status ──── model translates ─────────────────── Model
Any status ──── propagation from another entry ───── Propagated
Any status ──── validation fails ─────────────────── Error
```

### Updating to a new plugin version (make-dict with old user dict as base)

```
Old user dict + new plugin
       │
       ├── Key exists, source text unchanged ────── status preserved as-is
       ├── Key exists, source changed, was approved ── Changed
       ├── Key exists, source changed, was in progress ── Outdated
       ├── Key exists, source changed, was untranslated ── Untranslated
       ├── Key exists, only numbers differ ──────── Adapted (if base was approved)
       ├── New entry, approved text match ───────── Reused
       └── New entry, no match at all ───────────── Untranslated

       Deleted entries (no longer in plugin) are not included.
       Renamed entries (same text, new ID) appear as Reused if the
       old entry had an approved status.
```

### Conversion (applying to plugin)

```
       Translated ──────────┐
       Matched ─────────────┼──── APPLIED to plugin
       Heuristic ───────────┘

       Identical ───────────┐
       Adapted ─────────────┤
       Reused ──────────────┤
       Changed ─────────────┤
       Outdated ────────────┤
       Ambiguous ───────────┤
       In Progress ─────────┼──── SKIPPED (original text unchanged)
       Model ───────────────┤
       Propagated ──────────┤
       Untranslated ────────┤
       Missing ─────────────┤
       Duplicate ───────────┤
       Mismatch ────────────┤
       Error ───────────────┘
```

---

## The suggestions panel

For entries with status Missing, Duplicate, Ambiguous, Adapted, Changed, or Outdated, a panel appears below the translation editor showing context:

- **Missing**: possible native matches (one per line)
- **Duplicate**: alternative translations from other records with the same key
- **Ambiguous**: all conflicting translations found in the base dictionaries
- **Adapted**: the source translation that was modified
- **Changed**: what the original text used to be before the update
- **Outdated**: what the original text used to be before the update
