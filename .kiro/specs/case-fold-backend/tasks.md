# Tasks — Swappable Case-Fold Backend

Pure refactor + enforcement. No behavior change for the six current languages, no new dependency.

## 1. Extract the fold seam

- [ ] 1.1 Create `yampt.core/source/utility/case_fold.hpp` declaring `case_fold::to_lower(std::uint32_t) -> std::uint32_t` with the documented contract comment (simple 1:1 lowercase, byte-length preserving, hazard list). (R1.1, R1.2)
- [ ] 1.2 Create `case_fold.cpp` with the current `fold_code_point` body verbatim, renamed to `case_fold::to_lower`. (R1.1, R5.1)
- [ ] 1.3 Register both files in `yampt.core.vcxproj` + `.vcxproj.filters` (flat). (project-paths)

## 2. Rewire string_utils to the seam

- [ ] 2.1 In `string_utils.cpp`, include `case_fold.hpp`, remove the local `fold_code_point`, and call `case_fold::to_lower(code_point)` in the `to_lower_utf8` loop. Keep `decode_utf8` / `append_code_point` / the loop unchanged. (R1.3, R2.1, R2.2)

## 3. Enforce the byte-length invariant

- [ ] 3.1 Add a small helper (test-local) that returns a code point's UTF-8 byte length.
- [ ] 3.2 Add a `[u]` test asserting `utf8_length(case_fold::to_lower(cp)) == utf8_length(cp)` for a representative sample across every supported range (Basic Latin, Latin-1, Latin Extended-A incl. the parity ranges, Cyrillic + Ё). (R3.1, R6.2)

## 4. Regression guard for the six languages

- [ ] 4.1 Add/extend `[u]` tests asserting fold output is byte-identical for characteristic letters of PL, DE, FR, RU, IT, HU (proves the extract changed nothing). (R5.1, R6.1)
- [ ] 4.2 Confirm existing `tests.string_utils_utf8.cpp` cases still pass unchanged. (R5.3)

## 5. Developer note

- [ ] 5.1 Ensure the "how to add a language later" note and the hazard-character list live at the seam (contract comment) and/or the design doc. No user-facing docs, no CHANGELOG (internal refactor, not a user feature). (R4.1, R4.2)

## Notes

- Deferred by design (not tasks here): adopting utf8proc/ICU, and fold-aware offset mapping for length-changing folds. These are only needed when broad coverage is actually added; R3.2 records the trigger.
- No call sites change; the public API is untouched.
- Building/running tests are manual (project no-build rule); tasks produce tests as artifacts, no "run tests" step.
- Not in CHANGELOG/README: this is an internal refactor with no user-visible change.
