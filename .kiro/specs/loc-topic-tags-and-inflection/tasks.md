# Tasks — Dialogue-Topic Tags in Translations and Correct Phrase Inflection

Order: build the pure core first (tagging, strip, dict-wide apply), then the inflection fix, then wire CLI and yTranslator, then docs. Each core piece gets unit tests written alongside it.

## 1. Core: topic tagging facility

- [x] 1.1 Create `yampt.core/source/creator/topic_tagger.hpp/.cpp` with `topic_tag_result_t`, `apply_tags_result_t`, and the `topic_tagger_t` class (`seed_topics`, `tag_line`, static `strip_tags`). Add files to `yampt.core.vcxproj` and `.vcxproj.filters` (flat, per project-paths rules). (R4.1–R4.5)
- [x] 1.2 Implement `strip_tags`: remove every `@...#` span, unwrapping the inner phrase; preserve text outside links; leave a lone `@` with no following `#` unchanged. (R4.4)
- [x] 1.3 Implement `seed_topics(const dict_t &)`: seed an internal `keyword_trie_t` from DIAL standard forms only (use `new_text` as the surface). (R4.2, R4.9)
- [x] 1.4 Implement `tag_line`: strip first, run `keyword_trie_t::find_matches`, skip overlapping spans, insert `@`/`#` around accepted spans, return tagged text + count. (R4.3–R4.5)
- [x] 1.5 Implement free function `apply_topic_tags(dict_t &) -> apply_tags_result_t`: seed from the dict's DIAL, tag translatable records' `new_text`, write back changed entries, count entries changed and tags inserted; skip entries below the existing min-length / non-translated criteria. (R4.1, R5.4)

## 2. Core: unit tests for tagging

- [x] 2.1 Add `yampt.tests/source/tests.topic_tagger.cpp` (register in `yampt.tests.vcxproj` + filters). (R6.4, R6.5)
- [x] 2.2 `topic_tagger_t::strip_tags` tests: single link, multiple links, malformed lone `@`, text outside links preserved.
- [x] 2.3 `topic_tagger_t::tag_line` tests: single topic wrapped; multiple topics; substring inside a word not tagged; longest-match wins; idempotent re-run; pre-existing tags stripped and reinserted; only DIAL topics seeded.
- [x] 2.4 `topic_tagger_t::apply_topic_tags` tests: counts changed entries and inserted tags; non-matching entries untouched; entries below threshold skipped.

## 3. Core: preserve tags across translation (split/reassemble helper)

- [x] 3.1 Add a pure link-split/reassemble helper (in `topic_tagger` or sibling `topic_link_splitter`): split a line into ordered plain-text and explicit-link segments using the `@`-to-next-`#` rule; expose reassembly that re-wraps a translated inner phrase with `@...#` preserving pseudo-asterisk suffixes. (R1.1–R1.4)
- [x] 3.2 Unit tests: one link, multiple links, pseudo-asterisks preserved, mixed plain+link; assert order and count preserved. (R6.1)

## 4. Core: fix multi-word phrase inflection

- [x] 4.1 Write failing unit tests first (test-before-fix): a multi-word phrase must produce the agreeing form and must NOT produce one-word-only invalid mixes; `.top` target is always the nominative phrase; per-phrase cap respected. Use an injected stub form-provider/validator so no real Hunspell load is needed. (R6.2, R6.3, R6.5)
- [x] 4.2 Extract the pure combination + validation logic from `inflection.cpp` into a testable unit that takes per-word candidate forms and a validity predicate and returns valid whole-phrase forms, bounded by the per-phrase cap. (R3.1–R3.3, R3.5)
- [x] 4.3 Replace `build_candidates_for_position`/`phrase_forms` multi-word path to use the product-and-validate approach; keep single-word path via `word_forms`; keep the nominative standard-form target. (R3.1–R3.4)
- [x] 4.4 Confirm the failing tests from 4.1 now pass.

## 5. CLI: apply-tags batch mode

- [x] 5.1 Add `--apply-tags` to `parse_command_line` (accept `-d` dict input and `-o` output) and to `run_command` dispatch. (R4.7)
- [x] 5.2 Add `user_interface_t::apply_tags()`: load dict via `dict_reader_t`, call `apply_topic_tags`, write via `dict_writer_t`, log an `[info]` summary in CLI style. (R4.7, R4.8)

## 6. yTranslator: whole-document Apply Topic Tags action

- [x] 6.1 Add a controller method (existing operations controller) that builds topics from the active `dict_document_t::data()`, refreshes `new_text` per translatable record, and for each changed record: records edit history, writes via `data_mut()` + `modified_records_insert` + `set_dirty(true)`, and does NOT call `commit()` (no propagation). (R4.6)
- [x] 6.2 Refresh changed table rows and append an `"apply tags"` log summary. (R4.6)
- [x] 6.3 Add the menu action (and optional toolbar button with `setToolTip`) wired to the controller method — no logic in `main_window_t`. Follow Auto-Save-Before-Operations if other batch ops prompt to save. (R4.6, gui-tooltips, Anti-Gravity Rule)

## 7. Component 2a wiring: tags in the translate flow

- [x] 7.1 In the translate action path, use the split/reassemble helper: translate link inner phrases as separate segments and re-wrap, reassemble in order; place the tagged result into the editor. Keep orchestration in a helper/controller, not `main_window_t`. (R1.1–R1.4)

## 9. Move Apply Topic Tags to the dictionary right-click menu

- [x] 9.1 Remove the Apply Topic Tags action from the Tools menu in `main_window_setup.cpp`. (R7.2)
- [x] 9.2 Add an "Apply Topic Tags" entry to the sidebar's document right-click menu, shown only for dictionary documents (not `.top`/`.mrk`/`.yaml`). Wire it (no logic in `main_window_t`) to the controller, passing the right-clicked document. (R7.1, R7.3, R7.4, R8.2, gui-tooltips, Anti-Gravity)
- [x] 9.3 Adjust `dict_operations_controller_t::on_apply_tags` to operate on the supplied dictionary rather than the active document; guard against loc documents. (R7.3, R8.1)

## 10. Loc inflection entries in the Annotations tab

- [x] 10.1 Add `annotation_t::inflection_form` kind. (R9.1)
- [x] 10.2 In the annotation/glossary rebuild path, contribute inflection entries (form → standard form) from a loaded `.top`/`.mrk`/`.yaml` document, with `source` = loc filename. (R9.1, R9.3)
- [x] 10.3 In `annotations_view_t::update_annotations`, add a `--- Inflection ---` section after Glossary with a distinct header color; empty section omitted; click copies the standard form. (R9.1–R9.4)
- [x] 10.4 Unit-test the inflection-entry contribution/dedup where pure logic allows (`[u]`). (R6.5)

## 8. Documentation

- [x] 8.1 Update `docs/yampt-CLI-Manual.md` with the `--apply-tags` mode (explanatory prose, manual-style). (R4.7)
- [x] 8.2 Update `README.md`, `docs/README.bbcode` (mirror), and `CHANGELOG.md` under `2.0beta` for the user-visible Apply Topic Tags feature (`[NEW]`). Do not mention tests, scripts, or build changes. (changelog-categories, readme rules)
- [x] 8.3 If any user-visible label is added (menu/toolbar text), verify README/manual describe it (sync-docs-with-code).
- [x] 8.4 Document that Apply Topic Tags moved from the Tools menu to the dictionary right-click menu (CHANGELOG `[CHANGE]`, yTranslator manual). (R7)
- [x] 8.5 Document that localization files' inflection entries now show in the Annotations tab under an Inflection section (CHANGELOG `[NEW]`/`[CHANGE]`, yTranslator manual). (R9)

## Notes

- All matching reuses `keyword_trie_t` (word-boundary, longest-match) — do not reimplement matching.
- Tagging is DIAL-only; cel/mrk generation is untouched (R5.1–R5.4).
- Building and running tests are done manually by the user (project no-build rule); tasks produce the tests as artifacts but do not include a "run tests" step.
