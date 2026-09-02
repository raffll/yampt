# Requirements — Report Skipped Entries During Propagation (yTranslator)

## Background — Current Behavior

When the user commits a translation in yTranslator, the edit propagates to every other entry that shares the same `old_text`.

- `dict_document_t::commit(const table_row_t & row, const std::string & new_text, status_t intent)` (yampt.translator/source/model/dict_document.cpp, ~line 108) writes the edited entry, then for non-`model`/non-`error` intents calls `propagate(entry.old_text, new_text)` and, if the returned count > 0, sets the source entry's status to `propagated` (by design, per the design-decisions rule).
- `int dict_document_t::propagate(const std::string & old_text, const std::string & new_text)` (~line 137) constructs a `byte_limit_validator_t`, sets its codepage, trims the source `old_text`, then iterates every record in `m_data`. For each candidate whose trimmed `old_text` matches and whose `new_text` differs from the incoming text, it runs:

  ```cpp
  if (validator.validate(type, new_text).level == validation_level_t::error)
      continue;   // silently skipped — not written, not counted
  ```

  Otherwise it writes `new_text`, sets status `propagated`, records the modification, and increments `count`. It returns the plain `int` count. **There is no record of which entries were skipped, and the user is never told.**
- `commit_result_t { std::string new_text; status_t status; int propagated_count; bool success; }` (yampt.translator/source/model/document.hpp) is the only carrier out of `commit`. There is no `propagate_result` struct and nothing that lists skipped entries.
- `byte_limit_validator_t::validate(rec_type_t type, const std::string & utf8_value)` returns `validation_result_t { validation_level_t level; size_t byte_count; size_t limit; std::string reason; }`. Per-type limits: CELL/DIAL 63, FNAM 31, RNAM 32, INFO 512 caution / 1024 error; other types unlimited. Errors also fire for unmappable characters and forbidden characters. So the exact overflow reason and numbers are already available at the skip point.
- The commit is driven by `main_window_t::commit_current_edit()` (yampt.translator/source/main_window.cpp, ~516-587) via `commit_orchestrator::execute(...)` which returns `commit_output_t { commit_result_t result; bool glossary_updated; }`. After commit, the post-propagation block (~line 572) shows `statusBar()->showMessage(tr("Propagated to %1 entries")...)` and calls `m_editor_controller.sync_propagated_rows(...)`. This is the natural place to surface a "skipped entries" report.
- Existing modal dialogs live in yampt.translator/source/dialog/. `dict_selection_dialog_t` (QDialog + QVBoxLayout + label + list/tree widget + `QDialogButtonBox`) is the reusable read-only-list dialog pattern.

## Problem

A translation propagated from one record type can be too long for another record type that shares the same original text. Example: a 40-character translation entered on a CELL (limit 63) propagates to an FNAM entry (limit 31) that shares the same `old_text`; the FNAM entry silently fails validation and is skipped. The FNAM keeps its old translation with no indication that it was left behind. The user believes propagation covered everything. This silent divergence is easy to miss and hard to debug.

## Goal

When propagation skips one or more target entries because the incoming translation would exceed their record type's byte limit (or otherwise fail validation for that type), show the user a dialog listing exactly which entries were not updated and why, instead of skipping them silently.

## User-Facing Outcomes

- Committing a translation still propagates to all valid same-text entries exactly as today.
- If one or more same-text entries were skipped because the translation is invalid for their record type (byte overflow, unmappable/forbidden character), a dialog appears after the commit listing each skipped entry: its record type, its identifier, the reason (e.g. "exceeds 31 byte limit"), and the relevant numbers (encoded length vs. limit).
- The dialog is informational — it does not change the skip decision (the entries genuinely cannot hold the text). The user can read it, close it, and decide to fix those entries manually.
- If nothing was skipped, no dialog appears (unchanged behavior; the existing "Propagated to N entries" status message still shows).

## Requirements

### R1 — Capture skipped entries during propagation

1.1 `propagate` records, for each target entry it skips due to `validate(...).level == error`, the information needed to report it: record type, entry identifier (`key_text`), the reason string from `validation_result_t::reason`, the encoded byte length (`byte_count`), and the type's limit (`limit`).
1.2 The existing skip decision is unchanged — entries that fail validation are still not written and not counted in `propagated_count`. Only the fact that they were skipped is now captured.
1.3 Caution-level results (e.g. INFO > 512 but ≤ 1024) are NOT skips and are NOT reported — they propagate as today. Only `error`-level validation is a skip.

### R2 — Carry the skipped list out of commit

2.1 The skipped-entry information is surfaced from `propagate` through `commit` to the caller. The cleanest carrier is a new field on the propagation result (e.g. a `std::vector<propagation_skip_t>` on `commit_result_t`, or a dedicated `propagate_result_t` returned by `propagate` and folded into `commit_result_t`). The design will pick one shape; the requirement is that `main_window_t::commit_current_edit` can obtain the list after `commit`.
2.2 `propagate`'s return contract may change from a bare `int` to a struct carrying both the count and the skipped list, OR keep the count and add an out-parameter/member — the design decides, but it must not require a second pass over the data (no calling `propagate` twice, per the cleanest-approach rule).
2.3 `commit_orchestrator::execute` forwards the skipped list unchanged as part of `commit_output_t`/`commit_result_t`.

### R3 — Skipped-entries dialog

3.1 A modal dialog lists the skipped entries, one row per entry, showing at minimum: record type, entry id (`key_text`), and the reason/length detail. The design will choose list vs. table; if a `QTableWidget` is used it sets 24px row height (gui-compact-tables rule).
3.2 The dialog is read-only and informational, closed with a single OK/Close button (`QDialogButtonBox`), modeled on the existing `dict_selection_dialog_t` structure.
3.3 All strings are wrapped for translation (`tr(...)`), per the localization rule. It lives in yampt.translator/source/dialog/.
3.4 The dialog is shown only when the skipped list is non-empty.

### R4 — Surface after commit

4.1 The dialog is shown from the post-propagation point in `main_window_t::commit_current_edit` (~line 572), after the table/rows are synced, using the skipped list from the commit result.
4.2 The existing "Propagated to N entries" status message is preserved for the successfully propagated count. The skipped dialog is additive, not a replacement.
4.3 Per the Main Window Anti-Gravity rule, if surfacing the dialog involves more than a trivial "if non-empty, construct and exec the dialog", the construction/formatting logic goes on a controller or the dialog class itself, not as new orchestration inside `main_window_t`. Building a dialog and calling `exec()` on it is acceptable thin routing; formatting each skipped row belongs in the dialog.

### R5 — Scope

5.1 This applies to the interactive commit path that goes through `commit_current_edit` / `commit_orchestrator` (the editor commit, save/save-all triggered commits, row-change commits). The `commit` calls that pass `status_t::in_progress` inline (e.g. the inline table edit path, Copy Original) also propagate; the design will specify whether they surface the dialog too or only the main editor commit does. Default: surface wherever `commit_current_edit` runs; the inline paths that do not go through the orchestrator are out of scope unless trivially covered.
5.2 The feature does not change which entries propagate, the first-wins/text-match rules, or the source-entry `propagated` status behavior (all unchanged).

### R6 — No regression

6.1 With no skips, behavior is identical to today (no dialog; same status message; same `propagated_count`).
6.2 `propagate`'s skip criterion (`validate(...).level == error`) is unchanged; only reporting is added.
6.3 The `commit_result_t` consumers that read `new_text`/`status`/`propagated_count`/`success` are unaffected by an added field.

### R7 — Verification

7.1 Pure `[u]` test (no disk, no UI): construct an in-memory `dict_document_t` (or drive `propagate` directly) with two same-`old_text` entries of different types where the new text is valid for one and an error for the other (e.g. CELL valid, FNAM over 31 bytes); commit/propagate and assert the result reports exactly the FNAM entry as skipped with the correct reason and lengths, while the valid entry propagated and `propagated_count` counts only it.
7.2 A no-skip case reports an empty skipped list and the same `propagated_count` as today.
7.3 Manual verification: propagate a long translation from a CELL to a shared-text FNAM; the FNAM is not updated and the dialog lists it with "exceeds 31 byte limit".

## Open Decisions

Resolved:
- The skip decision is unchanged; only reporting is added. (R1.2, R6.2)
- Caution-level (INFO 512–1024) is not a skip and not reported. (R1.3)
- The dialog is informational/read-only, modeled on `dict_selection_dialog_t`, shown only when non-empty, from the post-propagation point in `commit_current_edit`. (R3, R4)
- No second pass over the data — the skipped list is captured during the single existing propagate loop. (R2.2)

Deferred to design:
- Exact carrier shape: new field on `commit_result_t` vs. a `propagate_result_t` returned by `propagate` and folded in (R2.1).
- Whether the inline-edit / Copy-Original commit paths (which call `commit` directly, not via the orchestrator) also surface the dialog (R5.1).
- List vs. table widget for the dialog, and exactly which columns/fields per row (R3.1).
