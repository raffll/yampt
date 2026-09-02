# Design — Report Skipped Entries During Propagation (yTranslator)

## Context (current mechanics)

- **commit** — `dict_document_t::commit(row, new_text, intent)` (dict_document.cpp ~108): writes the edited entry; for intents other than `model`/`error`, `result.propagated_count = propagate(entry.old_text, new_text)` and sets the source status to `propagated` when count > 0.
- **propagate** — `int dict_document_t::propagate(const std::string & old_text, const std::string & new_text)` (~137-183): builds `byte_limit_validator_t validator; validator.set_codepage(m_codepage);`, trims `old_text`, loops every `[type, chapter]` and record `i`:
  - `if (record.new_text == new_text) continue;`
  - trims `record.old_text`; `if (trimmed != trimmed_source) continue;`
  - `if (validator.validate(type, new_text).level == validation_level_t::error) continue;` ← **silent skip**
  - else write `new_text`, status `propagated`, `m_modified_records.insert({type, i})`, `++count`.
  Returns `count` (plain int).
- **result carrier** — `commit_result_t { std::string new_text; status_t status = untranslated; int propagated_count = 0; bool success = false; }` (document.hpp). No skip info.
- **validator** — `validate(rec_type_t, utf8) -> validation_result_t { validation_level_t level; size_t byte_count; size_t limit; std::string reason; }`. Errors carry `byte_count`, `limit`, `reason` (e.g. "exceeds 31 byte limit") even on error.
- **caller** — `main_window_t::commit_current_edit` (~516-587) → `commit_orchestrator::execute(input, doc, history, validator, glossary)` → `commit_output_t { commit_result_t result; bool glossary_updated; }`. Post-propagation block (~572): `statusBar()->showMessage(tr("Propagated to %1 entries")...)` + `sync_propagated_rows`.
- **dialog pattern** — `dict_selection_dialog_t` (dialog/dict_selection_dialog.*): QDialog, `setWindowTitle(tr(...))`, `setModal(true)`, QVBoxLayout, label + list/tree, `QDialogButtonBox` with `accepted → accept`.

## Design Goals

Capture the already-computed skip reasons during the single propagate loop (R1), carry them out through commit to the main window (R2), and show an informational dialog listing them (R3, R4) — without changing which entries propagate (R5, R6), and unit-testable in-memory (R7). Honor architecture rules: no second data pass (cleanest-approach), dialog formatting on the dialog not the main window (Anti-Gravity), tr() wrapping, ≤50-line functions.

## Decision: `propagate_result_t` returned by `propagate`, folded into `commit_result_t`

Introduce a small struct (yampt.translator/source/model/document.hpp, next to `commit_result_t`):

```cpp
struct propagation_skip_t
{
    rec_type_t type;
    std::string key_text;
    std::string reason;      // validation_result_t::reason
    size_t byte_count = 0;   // encoded length of the would-be text
    size_t limit = 0;        // the type's byte limit
};

struct propagate_result_t
{
    int count = 0;
    std::vector<propagation_skip_t> skipped;
};
```

Change `propagate` to return `propagate_result_t` (was `int`). Add `std::vector<propagation_skip_t> skipped;` to `commit_result_t`. In `commit`, `const auto propagation = propagate(...); result.propagated_count = propagation.count; result.skipped = std::move(propagation.skipped);`.

Rationale (R2.2, cleanest-approach): the skip info is gathered in the same loop that already calls `validate`, so no second pass and no double call. Returning a struct is cleaner than an out-parameter and keeps `propagate` self-describing. `commit_result_t` gains one field; existing consumers of `new_text`/`status`/`propagated_count`/`success` are unaffected (R6.3).

### propagate loop change (R1)

Replace the silent `continue` with a capture-then-continue:

```cpp
const auto validation = validator.validate(type, new_text);
if (validation.level == validation_level_t::error)
{
    result.skipped.push_back({ type, record.key_text, validation.reason, validation.byte_count, validation.limit });
    continue;
}
```

`validate` is called once per candidate (as today) and its result reused for both the branch and the capture (no extra validate call). Caution-level is unchanged and not captured (R1.3). The loop stays flat with early `continue` + blank lines (no-deep-nesting rule); if the loop body grows past limits it is split into a `propagate_to_record` helper returning "wrote / skipped / not-matched".

### Rejected alternative

Keeping `propagate` returning `int` and adding a `std::vector<...> &` out-parameter. Rejected: two-argument limit pressure and a less self-describing signature; a result struct is the cleaner carrier and matches `commit_result_t`'s style.

## Decision: skipped-entries dialog owns its formatting

New `propagation_skip_dialog_t` (yampt.translator/source/dialog/propagation_skip_dialog.hpp/.cpp), modeled on `dict_selection_dialog_t`:

- Constructor takes `const std::vector<propagation_skip_t> & skipped` (and parent).
- `setWindowTitle(tr("Entries Not Updated"))`, modal.
- A `QLabel` explaining that these same-text entries could not receive the translation because it is invalid for their record type.
- A `QTableWidget` (or `QListWidget`) with one row per skip. If a table: columns Type / Entry / Reason; `verticalHeader()->setDefaultSectionSize(24)` (gui-compact-tables rule). Row text: type via `domain_types::type_to_str(skip.type)`, `key_text`, and a reason string combining `reason` (e.g. "exceeds 31 byte limit") with the numbers (`byte_count`/`limit`).
- A `QDialogButtonBox(QDialogButtonBox::Ok)` → `accept`.
- All strings tr()-wrapped. Panel padding per panel-padding rule.

The per-row formatting lives in the dialog (R4.3), so the main window only decides whether to show it.

## Decision: surface from commit_current_edit, thin

In `main_window_t::commit_current_edit`, after the existing propagation handling (~line 572):

```cpp
if (!commit_output.result.skipped.empty())
{
    propagation_skip_dialog_t dialog(commit_output.result.skipped, this);
    dialog.exec();
}
```

This is thin routing (construct + exec), acceptable under the Anti-Gravity rule; all logic is in the dialog. The existing "Propagated to N entries" status message is kept (R4.2). Shown only when non-empty (R3.4).

`commit_orchestrator::execute` needs no logic change — it already returns `commit_output_t` wrapping `commit_result_t`, which now includes `skipped` (R2.3).

Scope (R5.1): the dialog surfaces wherever `commit_current_edit` runs. The direct `commit(..., in_progress)` call sites (inline table edit in `main_window_setup.cpp` ~710, Copy Original in `shortcuts_controller.cpp`) still populate `commit_result_t::skipped`, but do not open the dialog unless the design later routes them through a shared helper. Default: main editor commit path only; the data is available everywhere for a future extension.

## Data Flow

commit_current_edit → commit_orchestrator::execute → dict_document_t::commit → propagate (single loop: writes valid targets, captures error-skips into `propagate_result_t::skipped`) → commit_result_t{ propagated_count, skipped } → back in commit_current_edit: status message for count; if `skipped` non-empty, show `propagation_skip_dialog_t`.

## Error Handling

- No skips → empty vector → no dialog (R6.1).
- Non-dict documents: `commit` is virtual; only `dict_document_t` propagates. YAML/loc/eet return their own results with an empty `skipped`. No dialog for them.

## Testing Strategy (R7)

Pure `[u]` (in-memory `dict_document_t`, no disk/UI):
- Two entries sharing `old_text`: a CELL (limit 63) and an FNAM (limit 31). Commit a translation of, say, 40 encoded bytes on the CELL. Assert: `propagated_count == 1` (the CELL sibling/other valid targets), `skipped` contains exactly the FNAM with `reason` "exceeds 31 byte limit", `byte_count == 40`, `limit == 31`, and the FNAM's `new_text` is unchanged.
- No-skip case: all same-text entries accept the text → `skipped` empty, `propagated_count` matches the number written.

Test the propagate/commit result directly (no dialog needed for `[u]`). Building/running tests is manual (no-build-or-test rule). Names: `owner::member, description`, e.g. `"dict_document_t::propagate, reports byte-limit skips"`, `"dict_document_t::propagate, no skips reports empty list"`.

## Files Touched

| File | Change |
|------|--------|
| `yampt.translator/source/model/document.hpp` | `propagation_skip_t`, `propagate_result_t`; `skipped` field on `commit_result_t` |
| `yampt.translator/source/model/dict_document.hpp/.cpp` | `propagate` returns `propagate_result_t`; capture skips; fold into commit |
| `yampt.translator/source/dialog/propagation_skip_dialog.hpp/.cpp` | new informational dialog |
| `yampt.translator/source/main_window.cpp` | show dialog when `skipped` non-empty (thin) |
| `yampt.translator/yampt.translator.vcxproj` + `.filters` | register the dialog files |
| `yampt.tests/source/tests.dict_document.cpp` (new or extend) + vcxproj/.filters | `[u]` propagate-skip test |

## Documentation

- CHANGELOG `[CHANGE]` (yTranslator): propagation now lists entries it could not update because the translation is too long (or invalid) for their record type, instead of skipping them silently. (`[CHANGE]` because the user now sees new behavior — a dialog — where before there was silence; the skip itself is not a bug.)
- `docs/yTranslator-Manual.md`: in the propagation description, note that when a propagated translation does not fit a shared-text entry's record type, that entry is left unchanged and reported in a dialog listing what was not updated.
- README + README.bbcode in sync if propagation is described. No internal detail in user docs.
