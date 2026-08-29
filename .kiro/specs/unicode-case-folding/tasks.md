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

- [ ] 3. Fix the annotation-highlight regression (reconcile Qt toLower vs core fold)
  - Switch `highlight_coordinator.cpp` term fold to `to_lower_utf8`.
  - Change `record_display_controller.cpp` and `highlight_applier.cpp` to fold the match text with `to_lower_utf8` over the UTF-8 std::string instead of Qt `QString::toLower`, so both sides use the same byte-length-preserving fold.
  - Confirm the existing `tests.highlight_coordinator.cpp` `Ödsee`/`ödsee` test passes with offset 4, length 6 (the failing `8 == 9` case).
  - _Requirements: R3.1, R3.2, R3.3, R6.2_

- [ ] 4. Migrate glossary matching to the Unicode fold
  - Switch all `to_lower` calls in `glossary.cpp` (term/topic build, find, remove, `apply_glossary`, text folds) to `to_lower_utf8`.
  - Verify substitution offsets remain correct via the byte-length invariant.
  - Add/extend glossary unit tests with an accented term.
  - _Requirements: R3.1, R3.2_

- [ ] 5. Migrate record-table search to the Unicode fold
  - Switch `row_filter.cpp` query and haystack folds to `to_lower_utf8`.
  - Add a `row_filter` unit test matching an accented query against accented text case-insensitively.
  - _Requirements: R3.1_

- [ ] 6. Migrate remaining inline-std::tolower language sites (yTranslator)
  - `topic_highlighter.cpp`: replace inline `std::tolower` text/term folds with `to_lower_utf8`.
  - `spell_checker.cpp`: replace inline `std::tolower` excluded-word/keyword compares with the shared fold (and consolidate the duplicated compare per known-issues).
  - _Requirements: R3.1, R3.4, R5.2_

- [ ] 7. Migrate yEditor nav-tree matching
  - `nav_tree_filter.cpp` `contains_case_insensitive`: fold both sides with `to_lower_utf8` (or use `case_insensitive_equal_utf8`/substring on folded strings) so accented record ids/names search correctly.
  - `nav_tree_model.cpp` case-insensitive compare loop: use the shared code-point fold.
  - Add a `[u]` test for `contains_case_insensitive` with an accented needle/haystack (positive + negative).
  - _Requirements: R3.1, R5.2, R5.3, R6.4_

- [ ] 8. Make keyword_trie code-point aware for accented topics
  - Replace `keyword_trie_t::to_lower_char` per-char ASCII fold with a code-point-aware fold using the same table as `to_lower_utf8`, keeping byte offsets valid.
  - Add a test: an accented dial-topic keyword matches accented text of differing case with correct offset/length.
  - _Requirements: R3.1, R3.2_

- [ ] 9. Confirm ASCII-only sites are untouched
  - Verify no change to file_list, batch_cleaner, lua_scanner, omwscripts_parser, summon_fixer, sidebar_controller/`paths_equivalent`, plugin_operations_controller path normalize, session extension check, make_base_dialog filename compare, sidebar_view `suffix().toLower()`, and any nav_tree_model path-identity compare.
  - _Requirements: R4_

- [ ] 10. Documentation and steering
  - Add a CHANGELOG `[FIX]` under "Both Apps": case-insensitive matching (search, glossary, highlights, topic links) now works for accented and Cyrillic letters, in both yTranslator and yEditor.
  - Update the `known-issues` steering: remove the spell_checker duplicated-compare entry if task 6 resolves it.
  - Do NOT add tests/scripts/build changes to the CHANGELOG per steering.
  - _Requirements: R3, R5.3, R6.2_

## Notes

- No build/test execution is performed by the agent (no-build-or-test rule); the user builds and runs `yampt.tests.exe`.
- No third-party dependency may be added; the fold table is hand-authored in-repo.
- Each task should be committed only when the user asks; keep changes scoped per task.
