# Tasks — Centralized Unicode-Aware Case Folding

- [ ] 1. Add the fold table and `to_lower_utf8` to `string_utils`
  - Create `yampt.core/source/utility/string_utils.cpp`; declare `to_lower_utf8` and `case_insensitive_equal_utf8` in `string_utils.hpp`.
  - Implement UTF-8 decode + block-offset fold (Latin-1 Supplement, Latin Extended-A, Cyrillic) with a small exceptions list; ASCII fast path; malformed-sequence pass-through.
  - Leave `ß` unchanged; document the decision inline (one short line).
  - Register `string_utils.cpp` in `yampt.core.vcxproj` and `.filters`.
  - _Requirements: R1, R2.1, R5.1_

- [ ] 2. Write unit tests for the fold (test-first anchor)
  - New `yampt.tests/source/tests.string_utils_utf8.cpp` (`[u]`): per-language folds, `ß` pass-through, idempotency, byte-length invariant, non-letter pass-through, malformed UTF-8, `case_insensitive_equal_utf8`.
  - Add a table-integrity test: every fold pair has equal UTF-8 byte length.
  - Register the test file in `yampt.tests.vcxproj` and `.filters`.
  - _Requirements: R2.3, R6.1, R6.3, R6.4_

- [ ] 3. Fix the annotation-highlight regression
  - Switch `highlight_coordinator.cpp` term fold and `record_display_controller.cpp` text folds to `to_lower_utf8`.
  - Confirm the existing `tests.highlight_coordinator.cpp` `Ödsee`/`ödsee` test passes with offset 4, length 6.
  - _Requirements: R3.1, R3.2, R6.2_

- [ ] 4. Migrate glossary matching to the Unicode fold
  - Switch all `to_lower` calls in `glossary.cpp` (term/topic build, find, remove, `apply_glossary`, text folds) to `to_lower_utf8`.
  - Verify substitution offsets remain correct via the byte-length invariant.
  - Add/extend glossary unit tests with an accented term.
  - _Requirements: R3.1, R3.2_

- [ ] 5. Migrate record-table search to the Unicode fold
  - Switch `row_filter.cpp` query and haystack folds to `to_lower_utf8`.
  - Add a `row_filter` unit test matching an accented query against accented text case-insensitively.
  - _Requirements: R3.1_

- [ ] 6. Confirm ASCII-only sites are untouched and remove stray local lowercasing
  - Verify no change to file_list, batch_cleaner, lua_scanner, omwscripts_parser, summon_fixer, sidebar_controller, plugin_operations_controller.
  - Confirm keyword_trie stays ASCII (keywords are ASCII); document that decision in the design if confirmed.
  - Replace any per-file `static` language-text lowercasing helper with the shared function (none expected beyond keyword_trie).
  - _Requirements: R4, R5.2_

- [ ] 7. Documentation and steering
  - Add a CHANGELOG `[FIX]` under "Both Apps": case-insensitive matching (search, glossary, highlights) now works for accented and Cyrillic letters.
  - Update the `known-issues` steering: remove the entry if this resolves a tracked item; otherwise no change.
  - Do NOT add tests/scripts/build changes to the CHANGELOG per steering.
  - _Requirements: R3, R6.2_

## Notes

- No build/test execution is performed by the agent (no-build-or-test rule); the user builds and runs `yampt.tests.exe`.
- No third-party dependency may be added; the fold table is hand-authored in-repo.
- Each task should be committed only when the user asks; keep changes scoped per task.
