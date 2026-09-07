# Requirements — Derived Field Protection

## Introduction

Some fields inside a TES3 record are **derived**: their value is a function of other bytes
in the same record (a length, a count, or a compiled blob). They are not independent data.
If the user edits such a field directly, or if a merge/copy/lock operation leaves it out of
sync with the data it describes, the record is corrupt and the game or OpenMW misreads every
byte after it.

yEditor currently exposes several derived fields as freely editable (notably the SCPT `SCHD`
count/size fields), and its partial merge-lock operations can byte-splice a record without
reconciling a derived count. This feature introduces a **system-derived exclusion tier**: a
fixed, code-defined set of derived fields that are non-editable, excluded from merge,
non-lockable, shown greyed and non-toggleable in the exclusion UIs, and recalculated when the
merged patch is built.

## Terminology

- **Derived field** — a value that must equal a computation over other bytes (a size, a
  count, or compiled bytecode).
- **Authoritative data** — the data a derived field describes (the actual sub-record bytes,
  the item list, the compiled data).
- **User exclusion** — the existing, user-editable sub-record ignore list
  (`SubRecordRules/IgnoreConflict`, default `CELL:NAM0`).
- **System-derived exclusion** — the new tier defined in code, not user-editable.

## Requirements

### Requirement 1 — Central definition of derived fields

**User story:** As a maintainer, I want one authoritative list of derived fields, so that
every part of the app treats them consistently and no record type is special-cased in
scattered places.

#### Acceptance Criteria
1. WHEN any component needs to know whether a sub-record or field is derived THEN it SHALL
   query a single shared definition in `yampt.core`.
2. The definition SHALL express entries at sub-record granularity (`record:sub`) and at
   field granularity (`record:sub:field`).
3. The definition SHALL include: SCPT `SCDT` (whole sub-record); SCPT `SCHD` Num Shorts,
   Num Longs, Num Floats, Script Data Size, Local Var Size (fields); LEVI and LEVC `INDX`
   (whole sub-record).
4. No consuming component SHALL hardcode a record-type name to detect a derived field.

### Requirement 2 — Derived fields are not editable

**User story:** As a user, I want derived fields to be uneditable, so that I cannot corrupt a
record by typing a value that no longer matches the data it describes.

#### Acceptance Criteria
1. WHEN the record view displays a derived field THEN it SHALL render greyed and non-editable
   (no in-place editor).
2. WHEN an edit is somehow submitted for a derived field THEN the edit path SHALL reject it
   and report the reason in the status bar.
3. The greyed presentation SHALL reuse the existing ignored/excluded visual treatment.

### Requirement 3 — Derived fields do not participate in merge

**User story:** As a user, I want derived fields skipped by the auto-merge, so that a derived
value is never taken from a conflicting plugin and left inconsistent.

#### Acceptance Criteria
1. WHEN the merged patch is generated THEN derived sub-records/fields SHALL be excluded from
   conflict resolution, in the same manner as user-excluded sub-records.
2. The exclusion of derived fields SHALL be applied automatically, without the user adding a
   rule.

### Requirement 4 — Derived fields are not lockable

**User story:** As a user, I want lock/unlock unavailable for a derived field, so that I
cannot freeze a value that must instead be recalculated.

#### Acceptance Criteria
1. WHEN the user opens a lock context menu on a target that IS a derived field or derived
   sub-record THEN Lock/Unlock SHALL be shown greyed (disabled).
2. WHEN the user locks a whole record that merely CONTAINS derived fields THEN the lock SHALL
   be allowed, and the derived fields SHALL still be recalculated after the lock re-applies
   (see Requirement 6).

### Requirement 5 — Derived fields appear greyed and non-toggleable in exclusion UIs

**User story:** As a user, I want to see derived fields in the exclusion places I already
know, marked as permanently excluded, so that I understand why they are inert and am not
able to change that.

#### Acceptance Criteria
1. WHEN the sub-record context menu is shown for a derived sub-record/field THEN the
   Include/Exclude action SHALL be present but disabled (greyed), and SHALL NOT change any
   setting when the disabled item is interacted with.
2. WHEN the merge settings exclusion list is shown THEN system-derived entries SHALL be
   listed as disabled (greyed) items that the user cannot remove.
3. System-derived entries SHALL NOT be written into the user exclusion setting
   (`SubRecordRules/IgnoreConflict`).

### Requirement 6 — Derived fields are recalculated in the merged patch

**User story:** As a user, I want derived values in the merged patch to be correct
automatically, so that the output plugin is valid regardless of how records were assembled.

#### Acceptance Criteria
1. WHEN a merged-patch record is finalized THEN a recompute pass SHALL set each derived value
   from its authoritative data:
   - SCPT `SCHD` Script Data Size = length of `SCDT`.
   - SCPT `SCHD` Local Var Size = length of `SCVR`.
   - LEVI/LEVC `INDX` = number of merged list items.
2. The recompute pass SHALL be idempotent: running it on an already-consistent record SHALL
   leave it unchanged (so a whole-record lock re-apply is unaffected).
3. The recompute pass SHALL run after the auto-merge and after locks are re-applied.
4. WHERE a derived value cannot be reliably recomputed from available data (SCPT `SCHD`
   Num Shorts/Longs/Floats, because yampt does not parse the typed variable table and never
   adds or removes script variables) THE value SHALL be carried unchanged; it remains
   non-editable, excluded, and non-lockable, but is not rewritten.
5. The ENAM effect-slot count is implicit in the number of ENAM sub-records and requires no
   separate recompute beyond the existing slot merge.

### Requirement 7 — No corruption of records that only contain derived fields

**User story:** As a user, I want records such as scripts and leveled lists to remain valid
through copy, merge, and lock, so that the merged patch never breaks them.

#### Acceptance Criteria
1. WHEN a SCPT, LEVI, LEVC, ENCH, SPEL, or ALCH record is written to the merged patch by any
   path (auto-merge, copy, whole-record lock) THEN its derived values SHALL be consistent
   with its authoritative data in the output.
2. The structural record body Size and sub-record Size fields SHALL remain correct (these are
   already recomputed on every reconstruct and are out of scope for new work, but must not
   regress).

### Requirement 8 — Documentation and tests

**User story:** As a maintainer, I want the behavior documented and covered by tests, so that
it stays correct.

#### Acceptance Criteria
1. The recompute logic and the derived-field query SHALL be unit-tested with in-memory data
   (no file I/O), including idempotency.
2. WHEN this feature ships THEN the yEditor manual and CHANGELOG SHALL describe that derived
   fields are shown greyed, cannot be edited or locked, and are kept correct automatically.
