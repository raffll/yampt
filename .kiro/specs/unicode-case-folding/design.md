# Design — Centralized Unicode-Aware Case Folding

## Overview

Add a Unicode-aware lowercasing function to `yampt.core`'s `string_utils`, backed by a small hand-authored fold table covering the code-point ranges used by the six supported languages. Migrate all human-language case-insensitive matching to it, while leaving ASCII-only technical matching on the existing `to_lower`.

The scope split is the core design decision: **language text folds with Unicode rules; technical tokens (paths, extensions, IDs) stay ASCII.** This avoids changing filesystem/identifier semantics while fixing real language matching.

## Where It Lives

`yampt.core/source/utility/string_utils.hpp` already holds `to_lower`, `case_insensitive_equal`, `normalize_path`, `paths_equivalent`. The new functions join it so every project gets them through `yampt.core.lib`. Per the dependency rules, this stays pure C++ (no Qt), which it already is.

The fold table is large enough that it should not be an `inline` header body. Introduce `string_utils.cpp` in `yampt.core/source/utility/` (currently `string_utils` is header-only) to hold the table and the folding function out-of-line. The small existing inline helpers (`trim`, `trim_cr`, etc.) may stay inline in the header; the new `to_lower_utf8`, `case_insensitive_equal_utf8`, and `contains_ci_utf8` are declared in the header and defined in the new `.cpp`.

## Public API (string_utils)

```cpp
// Unicode-aware lowercase for UTF-8 text. Folds Basic Latin, Latin-1
// Supplement, Latin Extended-A, and Cyrillic. Other bytes pass through.
std::string to_lower_utf8(std::string_view input);

// Case-insensitive equality using to_lower_utf8.
bool case_insensitive_equal_utf8(std::string_view lhs, std::string_view rhs);
```

Notes:
- Existing ASCII `to_lower` / `case_insensitive_equal` / `paths_equivalent` are kept unchanged for technical matching (R4).
- No new "contains" helper is strictly required — callers already fold both sides and use `std::string::find`. They switch from `to_lower` to `to_lower_utf8`. This keeps byte offsets valid because folding within the covered ranges preserves the UTF-8 byte length of each code point (see "Byte-length invariant").

## Folding Algorithm

1. Iterate the input as UTF-8 code points. Decode the lead byte to determine sequence length (1–4 bytes). Validate continuation bytes.
2. For a valid code point, look up its lowercase in the fold table. If present, emit the lowercase code point's UTF-8 bytes; otherwise emit the original bytes.
3. For ASCII (`< 0x80`), fold `A`–`Z` directly without a table lookup (fast path, matches existing `to_lower`).
4. For a malformed sequence (bad lead byte or missing/invalid continuation), emit the offending byte unchanged and advance by one byte. Never read past `end`. (R1.4)

### Byte-length invariant

All folds in the covered ranges map a code point to another code point with the **same UTF-8 byte length**:
- Basic Latin (1 byte) → 1 byte.
- Latin-1 Supplement + Latin Extended-A (2 bytes) → 2 bytes.
- Cyrillic (2 bytes) → 2 bytes.

Because length is preserved, a folded string has the same byte length as the original, so match offsets/lengths computed on folded text are valid byte offsets into the original text (R3.2). This is the property the highlight code depends on. Tests assert it.

Exceptions that would break the invariant (e.g. German `ß` → `ss`) are deliberately NOT folded; `ß` passes through unchanged (documented decision, see below).

## Fold Table

A `constexpr`/static table mapping uppercase code point → lowercase code point for:
- **Latin-1 Supplement** U+00C0–U+00DE (À…Þ) → U+00E0–U+00FE, excluding U+00D7 (×). Covers French/German/Italian accented capitals.
- **Latin Extended-A** U+0100–U+017F: paired even/odd upper/lower (Ā/ā …) plus the specific Polish letters (Ł/ł U+0141/U+0142, Ą/ą, Ć/ć, Ę/ę, Ń/ń, Ó/ó U+00D3/U+00F3 lives in Latin-1, Ś/ś, Ź/ź, Ż/ż) and Hungarian (Ő/ő U+0150/U+0151, Ű/ű U+0170/U+0171).
- **Cyrillic** U+0410–U+042F → U+0430–U+044F, plus U+0401 (Ё) → U+0451 (ё).

Representation: a sorted array of `{uint32_t upper, uint32_t lower}` pairs with binary search, or a direct offset rule per contiguous block (e.g. Cyrillic `+0x20`, Latin-1 `+0x20`) with a small exceptions list. The block-offset approach with an exceptions list is smallest and clearest; prefer it.

### `ß` decision

`ß` (U+00DF) has no single lowercase (uppercase is `SS`). For matching purposes it stays unchanged. This is documented in the function comment and covered by a test. Rationale: this is match-folding, not display casing, and preserving the byte-length invariant matters more than perfect German orthography here.

## Migration Map

Switch these language-text call sites from `to_lower` to `to_lower_utf8` (and `case_insensitive_equal` → `case_insensitive_equal_utf8` where used on language text):

- `yampt.translator/source/highlighter/highlight_coordinator.cpp` — term fold.
- `yampt.translator/source/controller/record_display_controller.cpp` — the `original_lower` / `translation_lower` / `current_text` passed into `find_annotation_highlights`.
- `yampt.translator/source/editor/glossary.cpp` — all term/topic/text folds and `apply_glossary`.
- `yampt.translator/source/editor/row_filter.cpp` — query and haystack folds.
- `yampt.core/source/utility/keyword_trie.cpp` — `to_lower_char` becomes code-point aware, or the trie switches to folding whole tokens; decide during implementation (the trie is per-char, so it needs a code-point-aware variant or a pre-fold of inputs).

Leave UNCHANGED (ASCII technical, R4):
- `yampt.core/source/io/file_list.cpp` (extensions, filename classification)
- `yampt.core/source/scanner/batch_cleaner.cpp` (`.esm` check)
- `yampt.core/source/scanner/lua_scanner.cpp`, `omwscripts_parser.cpp` (`.lua`/`.omwscripts`)
- `yampt.core/source/scanner/summon_fixer.cpp` (known summon IDs)
- `yampt.translator/source/session/sidebar_controller.cpp` (`paths_equivalent`)
- `yampt.translator/source/session/plugin_operations_controller.cpp` (path normalize)

### keyword_trie consideration

`keyword_trie_t` folds one `char` at a time (`to_lower_char`). Script keywords are ASCII, but the trie also walks arbitrary text. Since keywords themselves are ASCII, ASCII folding of the trie keys is sufficient for keyword detection; the non-ASCII concern there is only about word boundaries (a separate TODO item about `\x80-\xFF`). Decision: keyword_trie stays ASCII for keyword matching; it is out of scope for this spec except to confirm it is not a language-text matcher. Remove it from R3.1 scope if implementation confirms keywords are strictly ASCII.

## Testing Strategy

New `yampt.tests/source/tests.string_utils_utf8.cpp` (`[u]` tag), plus the existing failing test in `tests.highlight_coordinator.cpp` becomes the regression anchor.

- Per-language fold: Ö→ö, Ä→ä, Ü→ü, É→é, Ł→ł, Ą→ą, Ő→ő, Ű→ű, Cyrillic А→а, Я→я, Ё→ё.
- `ß` passes through unchanged.
- Idempotency: `to_lower_utf8(to_lower_utf8(x)) == to_lower_utf8(x)`.
- Byte-length invariant: folded length == input length for covered ranges.
- Non-letters, digits, punctuation, ASCII unchanged.
- Malformed UTF-8 (lone continuation byte, truncated sequence) does not crash and passes bytes through.
- `case_insensitive_equal_utf8` true/false cases across scripts.
- Regression: the `Ödsee`/`ödsee` highlight test passes with offset 4, length 6.

## Build / Project Wiring

- Add `string_utils.cpp` to `yampt.core.vcxproj` + `.filters`. All consumers get it via `yampt.core.lib` automatically (no other project edits for the core function).
- Add `tests.string_utils_utf8.cpp` to `yampt.tests.vcxproj` + `.filters`.
- No new third-party dependency (no ICU) — table is hand-authored in-repo, satisfying the no-external-tools constraint.

## Risks

- Incomplete fold table: mitigated by per-language tests for the six languages' characteristic letters.
- Byte-length invariant violation if a future entry maps across byte-length boundaries: guard with a test that every table entry's upper and lower have equal UTF-8 length.
- Performance: folding runs on highlight/search paths that already ran `to_lower` over the same text; block-offset + small exceptions keeps it O(n) with tiny constant. No regression expected.
