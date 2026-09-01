# Requirements — Dialogue-Topic Tags in Translations and Correct Phrase Inflection

## Background — How OpenMW Consumes cel/top/mrk

OpenMW loads three tab-separated localization files next to a plugin's ESM/ESP, keyed by the plugin stem (`components/translation/translation.cpp`, `Storage::loadTranslationData`):

- `<esm>.cel` — cell name → translated cell name (`translateCellName`).
- `<esm>.top` — an inflected phrase form → the topic's **standard form** (nominative). Consulted by `topicStandardForm` when resolving explicit hyperlinks.
- `<esm>.mrk` — keyword → topic. The "mark" file; it can override which topic a keyword resolves to (the Bloodmoon `убит` case in `testkeywordsearch.cpp`).

Each file is `key\tvalue`, one entry per line, decoded from the plugin codepage to UTF-8.

Dialogue hypertext (`apps/openmw/mwdialogue/keywordsearch.cpp`):

- An **explicit link** is the substring from `@` to the next `#` (`parseHyperText`). The tags `@` and `#` wrap the linked phrase in the response text.
- Inside a link, trailing `*` characters are "pseudo-asterisks": `removePseudoAsterisks` strips them, the id is padded back with `*`, then the standard form is looked up in `.top`.
- **Implicit keywords** are seeded dialogue topics matched by a trie (`highlightKeywords`) in the gaps between explicit links. Keywords inside an explicit link are not re-detected.
- Word-boundary rules apply: substrings inside a word do not match (`orc` does not match `Force`); the longest keyword in a conflict wins.

This is the authoritative contract. yampt must produce cel/top/mrk that OpenMW reads correctly, and must not emit forms that map an inflected phrase to the wrong standard form.

## How yampt Produces These Files Today

`yampt.core/source/creator/loc_generator.cpp` builds all three from a translated dictionary:

- `build_cel_entries` — CELL records → `old_text \t new_text`.
- `build_mrk_entries` — DIAL records → `new_text \t old_text` (translated keyword → original topic).
- `build_top_entries` → `process_top_record` — for each DIAL record it inserts the lowercase `new_text` → `new_text`, then adds inflected forms from `inflection_t::phrase_forms(new_text)`, each mapping `form → new_text`.

Inflection (`yampt.core/source/creator/inflection.cpp`, `phrase_forms`):

- Single-word phrase → `word_forms` (Hunspell stem + suffix_suggest).
- Multi-word phrase → `build_candidates_for_position`: it varies **one word position at a time**, inflecting that single word and leaving all other words unchanged, then joins back. Results are capped at 50 (`max_phrase_forms`).

## Problems

### Problem 1 — Translations lose dialogue-topic tags (`top/mrk etc require also tags in translated new text`)

When a source dialogue response contains explicit `@...#` topic tags (or relies on topic keywords), the translated `new_text` produced in the workbench does not carry those tags. The `.top`/`.mrk` machinery can only resolve a link if the tag markers survive translation and the tagged phrase corresponds to a known topic form. Today the translate flow (`main_window_setup.cpp` translate action → `glossary_t::apply_glossary`) substitutes glossary terms but does not preserve or re-apply the `@...#` tags around the translated topic phrase. The result is translated text where topics are no longer linkable in-game.

### Problem 2 — Multi-word inflection only varies one word and emits invalid phrases (`top/mrk do only variant of one word in sentence, and create lots of not valid sentences`)

For a multi-word topic phrase, `phrase_forms` inflects a single word at a time while holding the rest fixed. For inflected languages (Polish, Russian, etc.) the words in a phrase must agree in case/number, so:

- The correct fully-agreeing form is often never produced (no position-combination varies all words together).
- Many produced combinations are grammatically invalid (one word inflected, the others left in nominative), and every such combination is written to `.top` as a valid inflected-form → standard-form mapping.

This floods `.top` with wrong forms, wastes the 50-form budget on invalid variants, and can cause the in-game link resolver to match a bad form or miss the real one.

## Goal

1. Preserve dialogue-topic tags through translation so that a translated response keeps its `@...#` links (and the tagged phrase resolves to the correct topic via `.top`/`.mrk`).
2. Make phrase-form generation produce only plausible, grammatically-valid inflected forms — not one-word-only variants — so `.top` maps correct inflected phrases to the right standard form.
3. Provide a batch operation that applies (refreshes) dialogue-topic `@...#` tags directly into a dictionary's translated text, available both in yTranslator (whole-document action) and in the CLI.

## User-Facing Outcomes

- After translating a dialogue response that had topic tags, the committed `new_text` still contains the topic tags around the translated topic phrase.
- Generated `.top` files contain inflected forms that a native speaker would accept as real inflections of the phrase, and omit the one-word-only garbage combinations.
- In-game, translated dialogue topics remain clickable/linkable where the original had them.
- A single action (in yTranslator or the CLI) tags every translated entry in a dictionary with dialogue-topic links, and re-running it refreshes the tags rather than nesting or duplicating them.

## Requirements

### R1 — Preserve explicit topic tags across translation

1.1 When source text contains explicit `@...#` links, the corresponding translated `new_text` retains a matching `@...#` link around the translated topic phrase.
1.2 The number and order of explicit links is preserved between source and translation; no link is dropped, duplicated, or reordered.
1.3 Pseudo-asterisk suffixes inside a link are preserved so the in-game standard-form lookup still works.
1.4 Text outside links is translated normally; text inside a link is treated as the topic phrase to be tagged in the translation.

### R2 — Topic phrase mapping stays consistent with `.top`/`.mrk`

2.1 The tagged translated phrase corresponds to an entry that `.top` maps to the correct standard form, or is itself the standard form.
2.2 Tag insertion never produces a phrase that maps to a different topic than the source link's topic.
2.3 The mrk keyword file continues to map the translated keyword to the original topic without regression.

### R3 — Phrase inflection produces valid multi-word forms

3.1 For a multi-word phrase, generated forms reflect agreement across the words (the words are inflected together), not a single word varied in isolation.
3.2 One-word-only variants that leave the remaining words in nominative are not emitted when they do not form a valid phrase.
3.3 Every emitted form is validated (e.g. each word accepted by the spell dictionary, or the whole form accepted) before being written to `.top`.
3.4 The standard-form target of every generated form remains the phrase's `new_text` (nominative), matching OpenMW's `topicStandardForm` contract.
3.5 The per-phrase form cap still applies; the budget is spent on valid forms, not invalid combinations.

### R4 — Apply (refresh) dialogue-topic tags to a dictionary

4.1 A batch operation walks every translatable record in a dictionary and rewrites its `new_text` so that occurrences of known DIAL topics are wrapped in `@...#` links.
4.2 Only DIAL topics are tagged. Cell names, FNAM/RNAM/INDX glossary terms, and other record kinds are not tagged by this operation.
4.3 Topic detection uses word-boundary-aware, longest-match-wins matching consistent with OpenMW's `highlightKeywords` (a substring inside a word does not match; the longest conflicting topic wins).
4.4 The operation is a **refresh**: any pre-existing `@...#` links in `new_text` are removed first, then tags are re-inserted from the current topic set. Re-running the operation is idempotent — it never nests, duplicates, or accumulates tags.
4.5 A phrase span already covered by a freshly inserted link is not tagged again within the same pass (no overlapping or nested links).
4.6 In yTranslator the operation is a single whole-document action that: tags all translatable records in the active dictionary, marks changed records dirty, records the change in edit history so it is revertible, and does not trigger propagation. It logs a summary of entries changed and tags inserted.
4.7 In the CLI the operation is a batch mode that reads a dictionary, applies the refresh, and writes the result. It logs a summary consistent with the existing CLI log style.
4.8 Both entry points share one pure core implementation; neither reimplements tagging.
4.9 The set of topics used for tagging is the dictionary's own DIAL entries (standard forms), consistent with how `.top`/`.mrk` are generated from the same dictionary.

### R5 — No regression to cel/mrk generation or the OpenMW contract

5.1 `.cel` generation is unchanged.
5.2 `.mrk` generation (translated keyword → original topic) is unchanged except as required by R2.3.
5.3 All three files remain tab-separated `key\tvalue`, codepage-encoded, with the existing skip/encode/dedup and collision-warning behavior.
5.4 Entries shorter than the existing minimum length threshold continue to be skipped.

### R7 — Apply Topic Tags lives on the dictionary right-click menu

7.1 In yTranslator the whole-document "Apply Topic Tags" action SHALL be invoked from the dictionary's right-click (context) menu in the sidebar, targeting the right-clicked dictionary.
7.2 The action SHALL be removed from the Tools menu; it is no longer a global menu action.
7.3 The action SHALL operate on the dictionary that was right-clicked, not on whichever document happens to be active.
7.4 The action SHALL only appear for dictionary documents, not for localization files (see R8).

### R8 — Apply Topic Tags only for dictionaries

8.1 The Apply Topic Tags operation SHALL apply only to dictionary documents; it SHALL NOT apply to localization documents (`.top`, `.mrk`, `.yaml`).
8.2 The right-click "Apply Topic Tags" action SHALL be offered only for dictionaries, never for a localization document.
8.3 Loc-file generation (`.cel`/`.top`/`.mrk`) is unaffected — R8 concerns the in-workbench tagging action only, not the OpenMW files yampt produces.

### R9 — Localization inflection entries appear in the Annotations tab

The `.top` and `.mrk` files hold inflected forms mapping to a standard form. These entries belong alongside the existing Hyperlinks and Glossary contextual data.

9.1 Localization-file entries (the inflected-form → standard-form pairs from a loaded `.top`/`.mrk`/`.yaml` document) SHALL be shown in the Annotations tab under a new section titled **"— Inflection —"**.
9.2 The new Inflection section SHALL sit alongside the existing Hyperlinks and Glossary sections in the same list, using the same section-header style (its own distinct header color).
9.3 Each inflection entry SHALL display as `form → standard form` consistent with the other annotation rows, and clicking it SHALL copy the standard form like other annotation rows.
9.4 When no localization inflection data is loaded, the Inflection section SHALL NOT appear (empty sections are omitted, matching the existing Hyperlinks/Glossary behavior).

### R6 — Correctness is verified by unit tests

6.1 Unit tests cover explicit-link preservation: source with one link, multiple links, pseudo-asterisks, and text mixing links with plain text, asserting the translated output keeps the tags in order.
6.2 Unit tests cover multi-word inflection: a phrase whose words must agree produces the agreeing form and does not produce the one-word-only invalid combinations.
6.3 Tests assert the `.top` target is always the nominative `new_text`.
6.4 Unit tests cover the apply-tags refresh: tagging a line with a known topic wraps it correctly; re-running is idempotent; a substring inside a word is not tagged; pre-existing tags are stripped and reinserted; only DIAL topics are tagged.
6.5 Tests are pure in-memory unit tests (`[u]` tag), no file I/O; inflection tests that require a Hunspell dictionary are structured so the pure combination/validation logic is testable without loading a real dictionary.

## Open Decisions (resolve during design)

- **Tag re-insertion strategy**: how the translated topic phrase is located in the machine-translated output to wrap it with `@...#` — translate the inside of each link separately and reassemble, vs. translate the whole line then re-tag by matching the translated topic phrase. Separate-segment translation is the more reliable option and avoids fragile re-matching.
- **Which layer owns tag preservation**: `glossary_t::apply_glossary` / the translate action in `main_window_setup.cpp`, vs. a dedicated helper in `yampt.core` reused by CLI and GUI. Prefer a pure, testable core helper with the GUI as a thin caller.
- **Multi-word inflection algorithm**: generate the Cartesian product of per-word forms and filter by whole-phrase validity, vs. inflect the head word and propagate agreement, vs. rely on phrase-level dictionary entries. Must stay within the form cap and avoid a combinatorial explosion.
- **Validity check for a multi-word form**: per-word `spell()` on every word vs. a phrase-level acceptance heuristic. Decide what "valid" means precisely and how agreement is approximated with Hunspell only.
- **Interaction with implicit keywords**: whether tag preservation should also emit/annotate implicit topic keywords, or only handle explicit `@...#` links in this change.
