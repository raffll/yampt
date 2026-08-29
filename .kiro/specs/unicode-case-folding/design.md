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

Switch these language-text call sites to `to_lower_utf8` (and `case_insensitive_equal_utf8` where used on language text). Note: many use inline `std::tolower`/`std::transform` or Qt `toLower`, not `string_utils::to_lower`, so a plain swap of the core helper is NOT enough — each must be changed to call the core fold.

**yTranslator — `string_utils::to_lower` sites:**
- `highlighter/highlight_coordinator.cpp` — term fold.
- `editor/glossary.cpp` — all term/topic/text folds and `apply_glossary`.
- `editor/row_filter.cpp` — query and haystack folds.

**yTranslator — inline `std::tolower` / `std::transform` sites (were missed by a swap):**
- `highlighter/topic_highlighter.cpp` — text and per-term folds (currently inline `std::tolower`).
- `editor/spell_checker.cpp` — excluded-word and keyword case-insensitive compares on language words (currently inline `std::tolower`; also consolidate the duplicated compare noted in known-issues).

**yTranslator — Qt `toLower` sites on the match text path (reconcile, see below):**
- `controller/record_display_controller.cpp` — `original_lower` / `translation_lower` / `current_text` (currently Qt `toLower`).
- `highlighter/highlight_applier.cpp` — editor text fold (currently Qt `toLower`).

**yEditor — previously omitted, now in scope:**
- `model/nav_tree_filter.cpp` — `contains_case_insensitive` (inline `std::tolower`), used for nav-tree search.
- `model/nav_tree_model.cpp` — the case-insensitive compare loop (`ca`/`cb`, inline `std::tolower`).

Do NOT change (ASCII technical): `session.cpp` extension check, `plugin_operations_controller.cpp` path normalize, `make_base_dialog.cpp` filename compare, `sidebar_view.cpp` `suffix().toLower()`, `nav_tree_model` path-compare used for file identity (verify each is ASCII-token, not language text, during implementation).

### Qt `toLower` reconciliation (root of the failing test)

The failing `highlight_coordinator` test folds the **term** with ASCII `string_utils::to_lower` while the **text** side is produced by Qt `QString::toLower` in `record_display_controller`. When the term has a multi-byte letter, the two folds disagree and the match length is off by the multibyte delta (the observed 8-vs-9). Fix: fold BOTH sides with the same core `to_lower_utf8` over the identical UTF-8 byte string. Concretely, `record_display_controller` and `highlight_applier` stop using `QString::toLower().toStdString()` for the match text and instead take the UTF-8 std::string and fold it with `to_lower_utf8`, matching the term side. This preserves the byte-length invariant end-to-end so offsets/lengths are correct.

Leave UNCHANGED (ASCII technical, R4):
- `yampt.core/source/io/file_list.cpp` (extensions, filename classification)
- `yampt.core/source/scanner/batch_cleaner.cpp` (`.esm` check)
- `yampt.core/source/scanner/lua_scanner.cpp`, `omwscripts_parser.cpp` (`.lua`/`.omwscripts`)
- `yampt.core/source/scanner/summon_fixer.cpp` (known summon IDs)
- `yampt.translator/source/session/sidebar_controller.cpp` (`paths_equivalent`)
- `yampt.translator/source/session/plugin_operations_controller.cpp` (path normalize)

### keyword_trie consideration

`keyword_trie_t` folds one `char` at a time (`to_lower_char`) and is used for BOTH ASCII script keywords AND dialogue-topic matching. Topic names can be accented/Cyrillic (Polish, Russian), so ASCII-only folding means accented topics whose case differs from the source text will not match through the trie.

Decision (resolved): the trie's `to_lower_char` is replaced by a code-point-aware fold path so the trie folds by code point using the same fold table as `to_lower_utf8`. Because the fold is byte-length-preserving, the trie's byte-offset match results stay valid. This brings accented topic matching in line with the rest of the language-text matching (closing the "all cases" gap). If, during implementation, topic matching is confirmed to never reach the trie with non-ASCII text (it does, via dial topics), this could be reduced — but the default is: make the trie code-point aware. The word-boundary handling (`\x80-\xFF`) is a separate concern and out of scope.

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
- keyword_trie: an accented dial-topic keyword matches accented text of differing case, with correct byte offset/length.
- yEditor `nav_tree_filter_t::contains_case_insensitive`: an accented needle matches accented haystack case-insensitively (and a negative case).

## Build / Project Wiring

- Add `string_utils.cpp` to `yampt.core.vcxproj` + `.filters`. All consumers get it via `yampt.core.lib` automatically (no other project edits for the core function).
- Add `tests.string_utils_utf8.cpp` to `yampt.tests.vcxproj` + `.filters`.
- No new third-party dependency (no ICU) — table is hand-authored in-repo, satisfying the no-external-tools constraint.

## Risks

- Incomplete fold table: mitigated by per-language tests for the six languages' characteristic letters.
- Byte-length invariant violation if a future entry maps across byte-length boundaries: guard with a test that every table entry's upper and lower have equal UTF-8 length.
- Performance: folding runs on highlight/search paths that already ran `to_lower` over the same text; block-offset + small exceptions keeps it O(n) with tiny constant. No regression expected.
