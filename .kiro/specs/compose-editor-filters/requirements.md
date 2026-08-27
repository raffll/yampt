# Requirements — Compose yEditor Filters

## Overview

yEditor's record navigation can be narrowed by three independent controls: the "Conflicts Only" toolbar toggle, the "Advanced Filters..." dialog (conflict-all, conflict-this, record type, deleted, Lua criteria), and the search field (ID/Name text with case and regex options). The underlying navigation filter already evaluates every criterion as a logical AND, so the data layer is ready to combine them.

The view layer is not. Each of the three controls rebuilds a filter state from only its own inputs and pushes the whole thing to the model, overwriting whatever the other controls had set. Toggling Conflicts Only wipes any active advanced filter and search; applying an advanced filter drops Conflicts Only; only search happens to preserve advanced criteria. The controls therefore behave as last-writer-wins rather than composing. On top of that, the view keeps two overlapping copies of "the current filter" (`m_last_filter_state` and `m_last_quick_filter`) plus two active flags, which drift apart depending on which control ran last.

This feature makes all three controls compose. A record is shown only if it satisfies every active control at once. Conflicts Only acts as a shortcut that presets the conflict-all dimension. The duplicated view state is collapsed into one source of truth per control.

## Terminology

- **Conflicts Only** — the toolbar toggle that restricts the tree to conflicting and benign-override records.
- **Advanced filter** — the criteria set through the "Advanced Filters..." dialog: conflict-all set, conflict-this set, record-type set, deleted flag, Lua severity and interface sets.
- **Search filter** — the ID/Name text query from the toolbar search field, plus its case-sensitive and regex options.
- **Effective filter** — the single combined `filter_state_t` produced by merging Conflicts Only, the advanced filter, and the search filter, then applied to the navigation model.
- **conflict-all dimension** — the `filter_conflict_all` flag and `conflict_all_set` in `filter_state_t`.

## Requirements

### R1 — All filter controls compose

**User story:** As a user comparing plugins, I want Conflicts Only, the advanced filter, and search to all apply together, so I can narrow to exactly the records I care about without one control erasing another.

Acceptance criteria:
1. WHEN more than one filter control is active THEN a record SHALL be shown only if it satisfies every active control (logical AND).
2. WHEN the user changes any one control THEN the other active controls SHALL remain in effect.
3. Applying the advanced filter SHALL NOT clear Conflicts Only or the search text.
4. Toggling Conflicts Only SHALL NOT clear the advanced filter or the search text.
5. Setting or clearing the search text SHALL NOT clear Conflicts Only or the advanced filter.

### R2 — Conflicts Only is a shortcut for the conflict-all dimension

**User story:** As a user, I want Conflicts Only to work as a quick preset for showing conflicts, so I do not have to open the advanced dialog for the common case.

Acceptance criteria:
1. WHEN Conflicts Only is on THEN the effective conflict-all dimension SHALL be `{conflict, override_benign}`, regardless of the advanced dialog's conflict-all selection.
2. WHEN Conflicts Only is off THEN the effective conflict-all dimension SHALL be whatever the advanced filter specifies (possibly none).
3. The advanced filter's other dimensions (conflict-this, type, deleted, Lua) SHALL continue to compose regardless of the Conflicts Only state.
4. Conflicts Only SHALL affect only the conflict-all dimension; it SHALL NOT alter search or any other advanced dimension.

### R3 — Single source of truth for filter state

**User story:** As a maintainer, I want the view to hold one authoritative copy of each control's input, so the applied filter cannot drift from what the controls show.

Acceptance criteria:
1. The view SHALL store the advanced filter criteria and the search criteria as separate, non-overlapping inputs, alongside the existing Conflicts Only flag.
2. The effective filter SHALL be derived from those inputs each time any input changes, through a single build-and-apply path.
3. The overlapping/duplicated members that previously tracked "the current filter" and its active flags SHALL be removed once the single path replaces them.
4. WHEN the effective filter has no active criteria THEN the navigation tree SHALL show all records (filter cleared).

### R4 — Reset clears every control

**User story:** As a user, I want the existing Reset button to clear all combined filters at once.

Acceptance criteria:
1. WHEN the user clicks Reset THEN Conflicts Only, the advanced filter, and the search filter SHALL all return to their empty/default state and the tree SHALL show all records.
2. Reset SHALL NOT change Hide Duplicates.

### R5 — Advanced dialog reflects current state

**User story:** As a user reopening the advanced dialog, I want it to show the advanced criteria currently in effect, so I can adjust rather than start over.

Acceptance criteria:
1. WHEN the advanced dialog opens THEN it SHALL be pre-populated with the advanced filter criteria currently applied.
2. The dialog SHALL NOT display the Conflicts Only shortcut as if it were an advanced conflict-all selection (the shortcut is a separate control).

## Out of Scope

- Changing how any single criterion matches a record (the model's `passes()` logic is unchanged).
- Persisting filter state across sessions.
- Adding new filter dimensions or new controls.
- Hide Duplicates behavior (it remains a separate toggle, untouched).
- Changes to the yTranslator filtering UI.
