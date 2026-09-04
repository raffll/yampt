# Implementation Plan

## Overview

Add a whole-dictionary spell check to yTranslator. A new `misspelled` status is added to the core status type (not approved, so convert/create skip it automatically). A pure predicate decides whether an entry's `new_text` contains a real misspelling, ignoring all-uppercase acronyms/tags plus the existing exclusions and MWScript keywords. A controller iterates the active dict document, flags offending entries as `misspelled` without propagating, and reports a summary. The status becomes filterable with its own color, display name ("Misspelled"), and tooltip. Work order: core status + presentation first, then the shared all-caps helper and the pure predicate, then the controller orchestration and action wiring, then tests and docs.

## Tasks

- [ ] 1. Add the `misspelled` status to core
  - `status_types.hpp`: add `misspelled` to `enum class status_t`, add `{ status_t::misspelled, "misspelled" }` to `status_entries`, bump the array size.
  - Confirm `is_approved_status` stays `== translated` (misspelled auto-skipped by convert/create; no converter/parser edits).
  - _Requirements: R1.1, R1.2, R1.3_

- [ ] 2. Present the `misspelled` status in the GUI
  - `status_display.hpp`: `status_display_name` case → `translate("yTranslator", "Misspelled")`.
  - `theme_system`: add `color_name_t::status_misspelled` + `get_status_color` case + light/dark color table entries.
  - `status_filter_view.cpp`: add to `status_order`; add a `get_status_tooltip` case.
  - `dict_document_t::supported_statuses()`: add `misspelled`.
  - _Requirements: R2.1, R2.2, R2.3, R2.4_

- [ ] 3. Add the all-uppercase helper
  - `string_utils::is_all_uppercase_utf8(std::string_view)` — true when the token has at least one cased letter and no lowercase letter (UTF-8 aware).
  - _Requirements: R3.1, R3.2_

- [ ] 4. Add the pure flagging predicate
  - `spell_scan::translation_has_misspelling(const std::string & new_text, const spell_checker_t & checker)` — reuse `checker.find_misspelled`, skip all-uppercase tokens, return true if any real misspelling remains. Keeps existing exclusions/keyword skips (already inside `check_word`).
  - _Requirements: R3.3, R4.2, R6.2_

- [ ] 5. Add the scan controller
  - New (or existing) controller method that: guards active doc is a `dict_document_t` and the native checker `is_loaded()` (else report "native dictionary required" and stop); iterates `data_mut()`, skips `untranslated`/empty `new_text`, runs the predicate; flags matches as `misspelled` via `modified_records_insert` (no propagation); sets dirty; refreshes table/counts; reports a summary.
  - Keep functions ≤50 lines / ≤3 nesting; split into `flag_document` + `report` helpers if needed.
  - _Requirements: R4.1, R4.3, R4.4, R4.5, R5.1, R5.2, R6.1_

- [ ] 6. Wire the action
  - Add the menu/toolbar action and connect it to the controller method (thin — no logic in `main_window`). Wrap the label/tooltip in tr().
  - _Requirements: R6.1_

- [ ] 7. Tests
  - `[u]`: `is_all_uppercase_utf8` (acronyms true; mixed/lower/digit/empty false); `translation_has_misspelling` with an unloaded checker → clean, all-caps-only → clean; status round-trip (`status_to_string`/`string_to_status`); `is_approved_status(misspelled)==false`.
  - `[i]` (temp dir, fixture `.aff/.dic`): a deliberate typo → flagged; clean text → not flagged. Clean up temp files.
  - Register any new test files in `yampt.tests.vcxproj` + `.filters`.
  - _Requirements: R8.1, R8.2, R8.3, R8.4_

- [ ] 8. Update documentation
  - CHANGELOG `[NEW]` (yTranslator): whole-dictionary spell check flagging entries with a "Misspelled" status (all-caps acronyms ignored); flagged entries are not applied during conversion.
  - `docs/yTranslator-Manual.md`: describe the operation and the "Misspelled" status.
  - README + README.bbcode in sync if spell checking is described.
  - _Requirements: R1, R2, R4_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1, 3], "depends_on": [] },
    { "wave": 2, "tasks": [2, 4], "depends_on": [1, 3] },
    { "wave": 3, "tasks": [5], "depends_on": [2, 4] },
    { "wave": 4, "tasks": [6, 7], "depends_on": [5] },
    { "wave": 5, "tasks": [8], "depends_on": [6, 7] }
  ]
}
```

Core status (1) and the all-caps helper (3) are independent starts. Presentation (2) needs the status; the predicate (4) needs the helper (and the checker, which already exists). The controller (5) needs both the status-aware document API and the predicate. Action wiring (6) and tests (7) follow; docs (8) last.

## Notes

- `misspelled` is not approved (`is_approved_status` unchanged), so `--convert`/`--create` skip it with zero converter/parser changes — this is the intended behavior per the status-definitions rule.
- The all-caps skip is applied in the scan predicate only, leaving the live editor highlighter's behavior unchanged. The shared `is_all_uppercase_utf8` helper makes extending it to the live path a one-line change later if wanted.
- Flagging does not propagate: spell-tagging is not a translation change. Writes go through `modified_records_insert` + `set_dirty` (or `commit_status`), never `commit()`.
- The scan uses the already-loaded native `spell_checker_t` (the one the main window loads on language change), not a second dictionary.
- Orchestration lives on a controller per the Anti-Gravity rule; the main window only wires the action.
- Building and running tests is done manually by the user (no-build-or-test rule).
