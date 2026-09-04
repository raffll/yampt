# Validation Error Feedback — yTranslator vs yEditor

Internal engineering analysis of how "validation error" feedback differs in the GUI between the two yampt apps. Both apps edit Morrowind plugin/dictionary text under byte-limit and character constraints, but they validate and surface errors in completely different ways. This document maps the divergence and its root causes. It is an analysis, not a spec — no behavior is changed by this file.

## Summary

The two apps validate edited text with **separate code paths, different validation models, and different UI feedback**. Nothing is shared: even though yEditor's validator lives in `yampt.core`, yTranslator uses its own validator in `yampt.translator`. This contradicts the `consistent-across-apps` steering rule.

- **yTranslator** — informs but never blocks. Invalid text commits with `status_t::error`; feedback is a status-bar counter, an in-editor overflow highlight, and the row's error status. Three-tier model (`ok / caution / error`).
- **yEditor** — blocks. The Apply button disables on invalid input and the controller re-rejects the commit. Feedback is a red edit box, an inline "Error:" label, and a tooltip. Binary model (`valid / invalid`).

## Where the logic lives

| | yTranslator | yEditor |
|---|---|---|
| Validator | `byte_limit_validator_t` | `field_validator::validate_field` (namespace) |
| File | `yampt.translator/source/editor/byte_limit_validator.hpp/.cpp` | `yampt.core/source/decoder/field_validator.hpp/.cpp` |
| Consumed by | `commit_orchestrator`, `record_display_controller_t::update_validation` | `preview_view_t`, `field_edit_controller_t::commit_field_edit` |
| Shared? | No — independent implementation | Lives in core but only yEditor consumes it |

## Validation model

| Aspect | yTranslator | yEditor |
|---|---|---|
| Result type | `validation_result_t { level, byte_count, limit, reason }` | `validate_result_t { valid, error_message }` |
| Levels | `enum class validation_level_t { ok, caution, error }` (three-tier) | binary `valid` / `invalid` (no caution) |
| Keyed on | record type (`rec_type_t`) | field schema type (`field_type_t`) + on-disk size |
| Byte limits | hardcoded magic numbers by record type | from schema `field.size` / `existing_sub_size` |
| Forbidden chars | rejects `\|`, `~`, `{`, `}` and control chars (≤0x1F except tab/CR/LF) | no forbidden-character scan |
| Unmappable chars | explicit error `"contains characters not representable in <codepage>"` | silent lossy encode, then size check only |

### yTranslator hardcoded limits (`byte_limit_validator.cpp`)

- CELL / DIAL: > 63 → error `"exceeds 63 byte limit"`
- FNAM: > 31 → error `"exceeds 31 byte limit"`
- RNAM: > 32 → error `"exceeds 32 byte limit"`
- INFO: > 1024 → error; > 512 → **caution** (the only place caution is produced)
- everything else: `ok`, limit 0 (unlimited)

### yEditor limits (`field_validator.cpp`)

- No per-record-type constants at all. String limits come from the decode schema: `string_fixed`/`scvr_subject` → `validate_string_fixed(input, codepage, field.size)`; `string_var` → capped at `max_string_var_bytes = 65535`; `raw` → `existing_sub_size`.
- Numeric/enum/flags/bool validated by field type (per-type min/max, enum name lists, `" | "`-separated flags or `0x` hex).

The record-type byte caps (FNAM=31, CELL/DIAL=63, RNAM=32) exist **only** in yTranslator. yEditor never encodes those numbers — it trusts the field schema/on-disk size. The two apps therefore do not share a single source of truth for limits and can disagree.

## UI feedback (concrete differences)

| Feedback | yTranslator | yEditor |
|---|---|---|
| Red background on the edit box | No | **Yes** — `setStyleSheet("background-color: #ffcccc;")` |
| Inline "Error: …" label | No | **Yes** — `m_validation_label`, red text `rgb(200,60,60)` |
| Live character/byte counter | **Yes** — `chars: N / limit` status-bar widget | No |
| Where the reason string shows | appended to status-bar counter as red text (`\| <reason>`) | inline label + failure tooltip |
| In-editor overflow highlight | **Yes** — paints overflow chars with `syntax_forbidden_background` | No |
| Apply button disabled on invalid | No | **Yes** — enabled only when `valid && changed` |
| Tooltip on commit failure | No | **Yes** — `QToolTip::showText` on the Apply button |
| Caution tier (yellow) | **Yes** — INFO > 512, yellow `rgb(200,180,0)` | No |
| Blocks the write? | **No** — commits with `status_t::error`, logs `[warning]` | **Yes** — button disabled + controller returns failure |
| Text-color feedback by level | ok green / caution yellow / error red (status-bar label) | red only (invalid) |

### yTranslator flow

`editor_view_t::text_changed` → `main_window_t::on_translation_changed()` → `record_display_controller_t::update_validation()`:
1. `byte_limit_validator.validate(type, text)`.
2. `validation_view_t::update_validation(result)` — status-bar counter, text color by level, reason appended on error.
3. If `byte_count > limit`, a `QTextEdit::ExtraSelection` highlights the overflow characters inside the editor.

On commit, `commit_orchestrator::execute` validates; if `error`, it sets `effective_intent = status_t::error`, logs a developer `[warning]`, and **commits anyway**. The edit is never blocked. The status filter maps `status_t::error` to "Translation has a validation error". (`dict_document_t` also re-validates on load to downgrade a stale `error` to `in_progress` when the text now validates.)

### yEditor flow

`preview_view_t::on_text_changed` (after `m_user_has_typed`):
1. `field_validator::validate_field(field, text, codepage, existing_sub_size)`.
2. Invalid → red background on `m_right_edit` + inline label `"Error: <message>"`; valid → clears both.
3. `m_apply_button->setEnabled(is_valid && value_changed)`.

`preview_view_t::on_apply_clicked` → `field_edit_controller_t::commit_field_edit`, which **re-validates** server-side and returns `{false, error_message}` on failure; the view then shows `QToolTip::showText` at the Apply button. yEditor thus validates twice (view + controller) and blocks on failure.

## Message wording is inconsistent

- yTranslator (byte/char phrased): `"exceeds 63 byte limit"`, `"forbidden character: |"`, `"control character: 0xNN"`, `"contains characters not representable in <codepage>"`.
- yEditor (schema/type phrased): `"encoded string exceeds field size"`, `"value out of range"`, `"unknown flag name"`, `"not a valid unsigned integer"`, `"unknown enum value"`.

No shared vocabulary or shared source of truth.

## Root-cause inconsistencies (contradict `consistent-across-apps`)

1. **Two validators, two models** — three-tier record-type byte limits (translator) vs. binary field-type/schema-size (editor); no shared code.
2. **Duplicated, divergent limits** — FNAM=31 / CELL=63 / RNAM=32 / INFO 512/1024 live only in the translator; the editor uses schema sizes. They can disagree for the same field.
3. **Opposite philosophies** — translator informs and commits an error status; editor blocks the write.
4. **Different feedback widgets** — red box + inline label + disabled button + tooltip (editor) vs. status-bar counter + text color + in-editor overflow highlight (translator).
5. **Different message wording** — no unified phrasing.
6. **Forbidden/control-char rules** — enforced only in the translator.

## Possible reconciliation directions (not decided)

Candidate approaches, to be turned into a spec if pursued:

- Extract a **shared validation core** in `yampt.core` with a single result model (three-tier, with `reason`, `byte_count`, `limit`) and one source of truth for record-type byte limits and forbidden/control-char rules.
- Unify **message wording** so the same violation reads the same in both apps.
- Decide **block-vs-inform** deliberately per app (the divergence may be intentional given each app's workflow — yTranslator tolerates in-progress invalid entries filtered by status; yEditor patches binary records where an invalid write is unacceptable), and document the decision rather than accidentally diverging.
- Align **feedback surface** where it makes sense (e.g. add a live byte counter to yEditor, or a consistent inline error presentation).

Any of these is a behavior change and must go through the spec/approval workflow before implementation.
