# Design — Dialogue-Topic Tags in Translations and Correct Phrase Inflection

This design covers three cohesive pieces that all revolve around OpenMW's cel/top/mrk topic-linking contract:

1. A pure core facility for inserting/refreshing `@...#` topic tags in text (`topic_tagger_t`).
2. Using that facility to (a) preserve tags across translation in the workbench, and (b) apply/refresh tags across a whole dictionary — in yTranslator and the CLI.
3. Fixing multi-word phrase inflection so `.top` gets valid agreeing forms instead of one-word-only variants.

All language logic lives in `yampt.core` (pure C++, unit-testable). yTranslator and the CLI are thin callers.

## OpenMW Contract (authoritative, from engine source)

- `.top` maps `inflected phrase form → standard (nominative) form` (`Translation::Storage::topicStandardForm`).
- Explicit link syntax is `@...#` around a phrase; trailing `*` are pseudo-asterisks stripped before the standard-form lookup (`KeywordSearch::parseHyperText`, `removePseudoAsterisks`).
- Implicit keywords are matched by a word-boundary-aware, longest-match trie (`highlightKeywords`); keywords inside an explicit link are not re-detected.

`yampt.core`'s existing `keyword_trie_t::find_matches` already implements the same word-boundary + longest-match behavior, so tagging reuses it rather than reinventing matching.

## Component 1 — `topic_tagger_t` (core, pure)

New class in `yampt.core/source/creator/topic_tagger.hpp/.cpp` (sits beside `loc_generator` and `inflection`, all topic/loc logic).

### Responsibility
Given a set of DIAL topics and a single line of text, produce the same line with `@...#` tags wrapping each topic occurrence, refreshing any pre-existing tags. Pure, no I/O, no Qt.

### Public API
```cpp
struct topic_tag_result_t
{
    std::string text;      // tagged line
    int tags_inserted = 0; // number of links written
};

class topic_tagger_t
{
public:
    void seed_topics(const dict_t & dict);          // builds trie from DIAL standard forms
    topic_tag_result_t tag_line(const std::string & line) const;

    static std::string strip_tags(const std::string & line); // remove existing @...# links
};
```

- `seed_topics` seeds an internal `keyword_trie_t` from the dictionary's DIAL entries (standard forms). Only DIAL is used (R4.2, R4.9). The topic surface used for matching is the topic's `new_text` (translated standard form), consistent with `.top`/`.mrk` generation from the same dict.
- `tag_line`:
  1. `strip_tags` first (refresh — R4.4): remove every `@...#` span, unwrapping the inner phrase back to plain text. This makes tagging idempotent.
  2. Run `keyword_trie_t::find_matches` on the stripped line.
  3. Walk matches left-to-right; skip any match that overlaps an already-emitted link span (R4.5, no nesting/overlap).
  4. Rebuild the line inserting `@` before and `#` after each accepted match span.
  5. Return the tagged line and the count.
- `strip_tags`: scan for `@`...next `#`, replace `@phrase#` with `phrase` (mirrors `parseHyperText`'s span rule: `@` to next `#`). Preserves text outside links verbatim. Handles malformed tags (a `@` with no following `#`) by leaving them unchanged.

### Why strip-then-tag (refresh)
Re-running on already-tagged text must not nest tags. Stripping first guarantees a canonical, tag-free baseline before re-inserting, so the operation is idempotent (R4.4) regardless of prior state.

### Offsets and boundaries
`keyword_trie_t` already reports correct byte offsets and enforces word separators (a substring inside a word does not match — R4.3). No new matching logic is written.

## Component 2a — Preserve tags across translation (yTranslator)

The translate action in `main_window_setup.cpp` currently sends `glossary_t::apply_glossary(old_text)` to the provider and places the raw result. To keep links (R1):

- Split the source line into segments: plain-text runs and explicit-link runs (reuse `parseHyperText`-style splitting; the split logic is a pure core helper so it is testable).
- Translate segments; for a link segment, translate only the inner topic phrase, then re-wrap the translated phrase with `@`...`#`, preserving any pseudo-asterisk suffix (R1.3).
- Reassemble in original order (R1.2).

This "separate-segment" approach (chosen over whole-line re-matching) avoids fragile re-location of the topic phrase in the machine output. The segment split/reassemble helper lives in core (`topic_tagger_t` or a sibling `topic_link_splitter`), with the GUI calling it and feeding each segment to the existing translation request path.

Note: this component depends on the translator's request/response flow; the pure split/reassemble and re-wrap logic is core and unit-tested, while the GUI wiring is a thin caller (per Main Window Anti-Gravity Rule, orchestration goes through a controller/helper, not new logic in `main_window_t`).

## Component 2b — Apply/refresh tags to a dictionary

### Core entry point
A pure function that applies `topic_tagger_t` across a whole dict:
```cpp
struct apply_tags_result_t { int entries_changed = 0; int tags_inserted = 0; };
apply_tags_result_t apply_topic_tags(dict_t & dict); // seeds from dict's own DIAL, tags translatable records
```
- Iterates translatable records; for each, `tag_line(new_text)`, and if the text changed, writes it back and counts it.
- Skips records below the existing minimum-length / non-translated criteria consistent with loc generation (R5.4 alignment).
- Only reads DIAL topics for seeding (R4.2). Records of any type may contain topic references in their text and get tagged, but the topic set is DIAL-only.

### yTranslator wiring (whole-document action — R4.6)
- Add an "Apply Topic Tags" action (menu + optional toolbar button with tooltip per gui-tooltips) routed through a controller method (Anti-Gravity Rule), not inline in `main_window_t`.
- The controller: gets the active `dict_document_t`, builds topics from its `data()`, then for each translatable record computes the refreshed `new_text`. For each changed record it:
  - records the change in `edit_history_t` (revertible — R4.6),
  - writes `entry.new_text` directly on `data_mut()` and calls `modified_records_insert` + `set_dirty(true)`,
  - does NOT call `commit()` (no propagation — matches the revert-no-propagation rule).
- Refresh the table model rows for changed entries and log a summary via the log view (`"apply tags"` header, lowercase).
- Prompts to save unsaved changes first if the existing operation pattern requires it (Auto-Save Before Operations rule) — align with how other batch operations behave.

### CLI wiring (batch mode — R4.7)
- New command `--apply-tags` in `parse_command_line`/`run_command`, requiring `-d <dict>` and `-o <output>` (reuse existing `-d`/`-o` collection).
- Handler `apply_tags()`: load the dict via `dict_reader_t`, call `apply_topic_tags`, write via `dict_writer_t`, log a summary in CLI style (`[info] ...`).
- The `.bat` template docs and CLI manual get an entry (docs updated per changelog/doc rules, user-visible feature).

## Component 3 — Correct multi-word phrase inflection

### Current defect
`inflection_t::phrase_forms` → `build_candidates_for_position` inflects one word position at a time, holding the others in nominative. For agreement languages this both misses the correct joint form and emits many invalid combinations, all written to `.top`.

### New approach
Replace the position-at-a-time generation with a whole-phrase agreeing generation:

- For each word, compute its candidate forms (`generate_forms_for_word`) plus the word itself.
- Build combinations across words (Cartesian product) but bounded to avoid explosion:
  - cap per-word candidates,
  - short-circuit once the per-phrase form cap (`max_phrase_forms`) is reached.
- Accept a combination only if it passes the validity check (R3.3): every word in the combination is accepted by `Hunspell::spell`. This filters out the one-word-only invalid mixes (a word left in nominative that does not agree will typically be rejected only if it is not itself a valid word — so agreement is approximated by requiring all words individually valid AND by preferring combinations where all words are inflected together).
- The standard-form target remains the nominative `new_text` (R3.4).

### Open design point carried from requirements
Pure Hunspell cannot verify grammatical agreement between words. The pragmatic rule: generate the product of per-word valid forms, require every word to be a valid dictionary form, and rely on the fact that OpenMW resolves any listed form to the same standard form. This removes the "only one word varied" artifact (R3.1/R3.2) and the invalid nominative-mix combinations, at the cost of possibly listing forms that are individually valid but not a real agreeing phrase. If stricter agreement is needed, a later iteration can add morphological-tag matching; this is out of scope here and noted as a limitation.

The pure combination + validation logic is extracted so it is unit-testable by injecting a small stub form-provider/validator (no real Hunspell load — R6.5).

## Files

New:
- `yampt.core/source/creator/topic_tagger.hpp/.cpp` — tagging, strip, dict-wide apply, link splitting for translation.

Modified:
- `yampt.core/source/creator/inflection.cpp` — replace one-word-at-a-time generation with bounded product + validation; extract pure combination logic.
- `yampt.cli/source/interface/user_interface.hpp/.cpp` — `--apply-tags` command + `apply_tags()` handler.
- `yampt.translator/source/...` — controller method + menu/toolbar action for whole-document "Apply Topic Tags"; wiring only.
- vcxproj / vcxproj.filters for `yampt.core` (new files) and `yampt.tests` (new test files) per project-paths rules.
- Docs: CLI manual + README/CHANGELOG for the user-visible apply-tags feature (per changelog/doc rules).

## Testing

Pure `[u]` unit tests, no file I/O:
- `topic_tagger_t::tag_line`: single topic wrapped; multiple topics; substring-inside-word not tagged; idempotent re-run; pre-existing tags stripped and reinserted; only DIAL topics seeded.
- `topic_tagger_t::strip_tags`: single/multiple links, malformed `@` with no `#`, text outside links preserved.
- link split/reassemble helper: one link, multiple links, pseudo-asterisks preserved, mixed plain+link.
- `apply_topic_tags(dict)`: counts changed entries and inserted tags; leaves non-matching entries untouched.
- inflection: multi-word phrase yields agreeing forms and omits one-word-only invalid mixes (via stubbed form provider/validator); `.top` target is always nominative; per-phrase cap respected.
