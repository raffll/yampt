# Requirements — Centralized Unicode-Aware Case Folding

## Problem

Case-insensitive text matching across both applications uses `string_utils::to_lower`, which lowercases byte-by-byte with `std::tolower`. This only handles ASCII (`A`–`Z`). Any letter outside ASCII — German `Ö/Ä/Ü`, Polish `Ł/Ą/Ę`, Cyrillic, French accents, Hungarian, Italian — is left unchanged. As a result, case-insensitive matching silently fails whenever the text contains a non-ASCII letter with differing case between the two sides.

This breaks core functionality in the exact languages the tool targets. yampt supports six target languages (PL, DE, FR, RU, IT, HU), all of which use non-ASCII letters. A glossary term `Ödsee`, a cell name, an NPC name, or a search query containing an accented letter cannot be matched case-insensitively against text that spells the same word in a different case.

Confirmed failing example (`highlight_coordinator_t::find_annotation_highlights`): term `Ödsee` (uppercase `Ö` = `0xC3 0x96`) does not match text `ödsee` (lowercase `ö` = `0xC3 0xB6`), because `to_lower` leaves both bytes untouched.

## Goal

One centralized, Unicode-aware case-folding facility in `yampt.core` used by both `yTranslator` and `yEditor` (and the CLI, via core) for all case-insensitive matching of human-language text. It must correctly fold letters for all six supported languages.

## User-Facing Outcomes

- Glossary terms and dialogue hyperlinks highlight regardless of the case of accented letters.
- The record table search matches accented text case-insensitively.
- Glossary term substitution before AI translation matches accented terms.
- Any future case-insensitive matching of language text works for all six languages by default.

## Requirements

### R1 — Unicode-aware lowercasing helper

1.1 A single function in `yampt.core` (`string_utils`) lowercases a UTF-8 string, folding letters across the character ranges used by the six supported languages: Basic Latin, Latin-1 Supplement, Latin Extended-A, and Cyrillic.
1.2 The function accepts and returns UTF-8 encoded `std::string`. Input that is already lowercase, or contains no letters, is returned unchanged.
1.3 Bytes that are not part of a recognized letter code point (digits, punctuation, whitespace, control bytes, and code points outside the covered ranges) pass through unchanged.
1.4 Malformed UTF-8 sequences are passed through without crashing — the function never throws and never reads out of bounds.

### R2 — Unicode-aware equality and matching helpers

2.1 A case-insensitive equality helper compares two UTF-8 strings using the Unicode-aware fold from R1.
2.2 The substring/contains matching used by highlight and glossary code folds both the haystack and the needle with the R1 fold before comparing.
2.3 Folding a string twice yields the same result as folding it once (idempotent), so callers may fold at any layer without double-folding hazards.

### R3 — Language-text matching migrated to the Unicode fold

3.1 All case-insensitive matching of human-language text uses the Unicode-aware helpers. This covers, at minimum:
- annotation/hyperlink highlighting (`highlight_coordinator`)
- glossary term and dial-topic matching and substitution (`glossary`)
- record table search (`row_filter`)
- annotation/topic highlighting that folds with inline `std::tolower` (`topic_highlighter`)
- spell-checker excluded-word / keyword matching on language words (`spell_checker`)
- **yEditor** nav-tree search and comparison (`nav_tree_filter::contains_case_insensitive`, `nav_tree_model` case-insensitive compare)
3.2 A term with a non-ASCII letter matches text spelling the same word in a different case, and the reported match offsets and lengths are correct byte offsets into the original text.
3.3 Both sides of a case-insensitive comparison use the SAME byte-length-preserving fold. Where one side currently folds with Qt `QString::toLower` (`highlight_applier`, `record_display_controller`) and the other with a core fold, the mismatch is reconciled by folding both sides with the core `to_lower_utf8` on the identical byte string, so offsets/lengths stay consistent. Qt `toLower` is not mixed with `to_lower_utf8` on the two sides of the same match.
3.4 Inline `std::tolower` / `std::transform` lowercasing of language text is replaced by the shared fold; a swap of `string_utils::to_lower` alone is not sufficient because several sites do not call it.

### R4 — ASCII-only technical matching preserved

4.1 Matching that operates exclusively on ASCII technical tokens is NOT changed to the Unicode fold. This includes: file extensions (`.esm`, `.esp`, `.lua`, `.json`, `.omwscripts`), filesystem paths, record identifiers, and known-ID lookups (summon IDs, GMST keys).
4.2 `paths_equivalent` and path/extension classification keep ASCII lowercasing — filesystem case rules are ASCII for these tokens and Unicode folding could change behavior.
4.3 The existing ASCII `to_lower` remains available for these technical uses; it is not removed.

### R5 — Consistency across both apps

5.1 The Unicode-aware helpers live in `yampt.core` so `yTranslator`, `yEditor`, and the CLI share one implementation.
5.2 No app defines its own lowercasing or case-fold logic for language text. Any per-file `static` or inline `std::tolower` lowercasing of language text is replaced by the shared function. This explicitly includes yEditor: `nav_tree_filter_t::contains_case_insensitive` and the `nav_tree_model` case-insensitive compare are migrated so accented record ids/names search correctly in the nav tree.
5.3 yEditor's language-text matching behaves identically to yTranslator's for the same input (same fold, same results), satisfying the Consistent-Across-Apps rule in fact, not just in claim.

### R6 — Correctness is verified by unit tests

6.1 Unit tests cover the fold for each supported language's characteristic letters (German umlauts + ß handling decision, Polish, French, Russian Cyrillic, Hungarian, Italian).
6.2 A regression test reproduces the `Ödsee` / `ödsee` annotation match and asserts the correct offset and length.
6.3 Tests assert idempotency (R2.3), pass-through of non-letters and malformed UTF-8 (R1.3, R1.4), and that ASCII-only paths are unaffected.
6.4 A yEditor test covers `nav_tree_filter_t::contains_case_insensitive` matching an accented needle against accented text case-insensitively.
6.5 Tests are pure in-memory unit tests (`[u]` tag), no file I/O.

## Open Decisions (resolve during design)

- **German `ß`**: `ß` has no single lowercase; uppercase is `SS`. Decide whether to leave `ß` unchanged (recommended — matching, not display) and document it.
- **Cyrillic `Ё`/`ё` and range coverage**: confirm the exact Cyrillic block range to fold (U+0400–U+04FF).
- **Data source**: hand-authored fold table for the covered ranges vs. a generated table. Must not add a third-party dependency (no ICU) per project constraints.
- **codepoint vs codepage**: fold by Unicode code point (text is UTF-8 in memory) rather than by source codepage — confirm this matches how highlight/glossary text is stored at the matching layer.
- **keyword_trie for accented topics**: the trie folds one ASCII `char` at a time and is used both for ASCII script keywords AND for dialogue-topic matching. Topic names can be accented/Cyrillic (Polish, Russian). Decide whether the trie needs a code-point-aware fold for topic matching, or whether topic matching is moved off the per-char trie. If left ASCII, accented topics will not match — this must be an explicit, justified decision, not an omission.
- **Qt `toLower` on the highlight text path**: `highlight_applier` and `record_display_controller` currently fold the editor text with Qt `QString::toLower` (Unicode-aware but not guaranteed byte-length-preserving for our purposes), while the term side folds with the core fold. Decide the single fold used on both sides so offsets/lengths agree (design leans: fold both sides with core `to_lower_utf8` on the same UTF-8 byte string).
