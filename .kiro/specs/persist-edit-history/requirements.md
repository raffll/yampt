# Requirements — Persist Edit History Across Sessions

## Overview

yTranslator records an edit history for every translation change, shown in the History panel and used for revert. Today that history lives only in memory and is lost when the app closes. This feature persists each dictionary's edit history to a sidecar file next to the dictionary, so users can revert earlier translations even after restarting. A new Settings page lets the user turn the feature off; it is on by default.

Implementing this exposes a pre-existing correctness problem: the in-memory history is a single window-wide store keyed only by record type and key, with no document qualifier, and it is never cleared when switching documents. Two open dictionaries that both contain the same record (e.g. a CELL named "Balmora") therefore share one history and can cross-contaminate reverts. Persisting per-dictionary history requires fixing this scoping so each dictionary's history is isolated.

## Terminology

- **Edit history** — the per-entry list of previous (text, status, timestamp) values recorded by `edit_history_t` when a translation changes.
- **History file** — a sidecar file storing one dictionary's edit history, named `<dictionary path>.history`, in the same directory as the dictionary.
- **Document/dictionary path** — the on-disk path of a loaded dictionary, available as `document_t::path()`.
- **History panel** — the History tab in the bottom-left info tabs that lists past values with a Revert button.

## Requirements

### R1 — Per-dictionary history isolation

**User story:** As a translator, I want each dictionary's history kept separate, so reverting in one dictionary never restores text from a different dictionary that happens to share a record key.

Acceptance criteria:
1. Edit history entries SHALL be scoped to the dictionary they belong to; two open dictionaries with the same record type and key SHALL have independent histories.
2. WHEN the user reverts an entry THEN the restored text and status SHALL come only from the active dictionary's history.
3. The scoping fix SHALL apply to recording, retrieving, and reverting history.

### R2 — Save history to a sidecar file

**User story:** As a translator, I want my edit history saved alongside the dictionary so it is not lost when I close the app.

Acceptance criteria:
1. WHEN history saving is enabled AND a dictionary is saved THEN the app SHALL write that dictionary's history to `<dictionary path>.history`.
2. WHEN history saving is enabled AND the app closes THEN the app SHALL save the history of all open dictionaries.
3. The history file SHALL contain only the entries belonging to that dictionary.
4. WHEN history saving is disabled THEN no history file SHALL be written.

### R3 — Load history on open

**User story:** As a translator, I want my saved history available again when I reopen a dictionary, so I can revert changes made in a previous session.

Acceptance criteria:
1. WHEN a dictionary is opened or switched to AND a matching `.history` file exists THEN the app SHALL load that file's entries into the edit history.
2. WHEN the loaded history is present THEN the History panel and the Revert action SHALL work against it exactly as for in-session history.
3. WHEN no history file exists THEN the dictionary SHALL open normally with an empty history for its records.
4. Loading SHALL NOT be gated by the save-history setting (an existing file is still usable even if the user later disabled saving).

### R4 — History settings page

**User story:** As a translator, I want to control whether history is persisted.

Acceptance criteria:
1. Settings SHALL include a "History" page with a single "Save history" checkbox.
2. The setting SHALL default to enabled.
3. The setting SHALL persist across sessions in the application settings.
4. WHEN the user unchecks it and applies THEN subsequent saves SHALL NOT write history files.

### R5 — Remove dead session-only code

**User story:** As a maintainer, I want unused code removed once persistence exists.

Acceptance criteria:
1. `edit_history_t::is_modified_this_session` SHALL be removed, along with its backing state if nothing else uses it.
2. Removal SHALL NOT change any remaining behavior.

## Out of Scope

- Persisting history for non-dictionary documents (plugins, YAML localization).
- A global cross-dictionary history view (history remains per-dictionary).
- Pruning, size limits, or expiry of history files.
- Migrating or versioning the history file format (pre-1.0; existing format is used as-is).
