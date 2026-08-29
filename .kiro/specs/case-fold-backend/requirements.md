# Requirements — Swappable Case-Fold Backend for More Languages

## Background — Current State

Case-insensitive matching of human-language text in both apps goes through two functions in `yampt.core/source/utility/string_utils`:

- `to_lower_utf8(std::string_view) -> std::string`
- `case_insensitive_equal_utf8(std::string_view, std::string_view) -> bool`

`to_lower_utf8` is a UTF-8 decode → per-code-point fold → UTF-8 re-encode loop. The fold decision is a single file-local function `fold_code_point(std::uint32_t) -> std::uint32_t` with a hand-authored set of range rules covering exactly the letters used by the six currently supported languages: Basic Latin (A–Z), Latin-1 Supplement (À–Þ), Latin Extended-A (Ą, Ć, Ę, Ł, Ń, Ó, Ś, Ź, Ż, Ő, Ű, etc., including the odd/even parity inversions), and Cyrillic (А–Я + Ё).

Every language-text call site (highlight coordinator, glossary, row_filter, topic_highlighter, spell_checker, yEditor nav_tree_filter) depends only on those two public functions. Fifteen-plus call sites already treat `to_lower_utf8` as the single interchange point.

## Invariant the current code relies on

`to_lower_utf8` is **byte-length preserving**: every folded code point re-encodes to the same number of UTF-8 bytes as the input code point. This is required because match offsets and lengths are computed on the folded string and then reused against the original string (highlight cursor placement via `utf8_byte_to_char_offset`, glossary `replace(pos, len, ...)`). If folding changed byte length, all offsets after a length-changing character would be wrong.

The current fold satisfies this by construction: it only maps letters whose lowercase has the same UTF-8 byte length (1→1 or 2→2 for the six languages), and deliberately does NOT fold `ß` (U+00DF → "ss") because that would change length.

## Problem

The project intends to support additional languages in the future. Each new language whose letters fall outside the four hand-coded ranges (e.g. Greek, Turkish, Baltic, Vietnamese, additional Latin Extended-B/Additional letters) requires a manual edit to `fold_code_point`, with real risk of range/parity mistakes (a parity-inversion bug already occurred in Latin Extended-A). The hand table does not scale to broad Unicode coverage, and there is no clean seam to swap in a maintained Unicode fold source (e.g. `utf8proc`) when coverage needs to grow.

## Goal

Make the case-fold implementation cleanly swappable and extensible so that:

1. Adding languages does not require editing scattered logic — the fold source is one well-defined, documented seam.
2. A future backend (a maintained Unicode library such as `utf8proc`, or an expanded table) can replace the fold source without touching any caller or the public API.
3. The byte-length-preservation contract is explicit and enforced, so no backend silently breaks match offsets.

This spec is design/enablement only — it does NOT add a third-party dependency now and does NOT change matching behavior for the six current languages.

## Non-Goals

- Not adding ICU/utf8proc in this change.
- Not changing the public API (`to_lower_utf8`, `case_insensitive_equal_utf8`) or any call site.
- Not implementing locale-aware collation (sorting) — that is a separate concern.
- Not changing folding results for the six currently supported languages.

## Requirements

### R1 — Single documented fold seam

1.1 The per-code-point fold decision is isolated behind one clearly named seam (the `fold_code_point` function or an equivalent single entry point) that takes a code point and returns its lowercase code point.
1.2 The seam carries a documented contract: input is a Unicode code point, output is its simple (1:1) lowercase code point, and the mapping MUST be byte-length preserving in UTF-8 (the output code point re-encodes to the same number of bytes as the input).
1.3 The UTF-8 decode/encode loop in `to_lower_utf8` is independent of the fold source, so replacing the seam requires no change to the loop.

### R2 — Public API and call sites unchanged

2.1 `to_lower_utf8` and `case_insensitive_equal_utf8` keep their exact signatures and behavior.
2.2 No call site changes. Swapping the backend is invisible above the seam.

### R3 — Byte-length invariant is enforced, not assumed

3.1 The byte-length-preservation contract is stated in code near the seam and covered by a test that asserts, for a representative set across all supported ranges, that folding does not change byte length.
3.2 If a future backend is introduced that can produce length-changing folds (e.g. a full Unicode case fold, or a simple fold that crosses a UTF-8 length band such as Turkish `İ` U+0130 → `i`), the offset-reuse assumption must be addressed rather than silently broken. This spec records that as the required follow-up: either restrict the backend to a documented byte-length-safe set, or make the offset-consuming code fold-aware (map folded offsets back to original offsets) so any fold is safe.

### R4 — Extensibility path documented

4.1 A short design note (in the design doc, not a doc/README shipped to users) describes exactly what a future maintainer changes to add a language: extend the seam's coverage (add ranges) or swap the seam body to a library call.
4.2 The note identifies the byte-length hazard characters to watch for when widening coverage (dotless/dotted I, ligatures, and any 1:many or cross-band folds) so a future change does not regress offsets.

### R5 — No regression

5.1 The six current languages fold identically before and after the refactor (same output bytes for the same input).
5.2 yampt.core stays pure C++ with no new external dependency and no Qt.
5.3 Existing unit tests for the fold continue to pass unchanged.

### R6 — Verification

6.1 A test asserts fold output is unchanged for the six languages' characteristic letters (regression guard for the refactor).
6.2 A test asserts the byte-length invariant across the covered ranges (R3.1).
6.3 Tests are pure in-memory `[u]` unit tests, no file I/O.

## Open Decisions (resolve during design)

- **Seam shape**: keep `fold_code_point` as the single file-local function with a documented contract (minimal), vs. expose a small internal interface/policy so multiple backends can coexist behind a build flag. Given the non-goal of adding a dependency now, the minimal single-function seam is the likely choice; the interface/policy is only worth it if selectable backends are actually planned.
- **Where the seam lives**: inside `string_utils.cpp` (current) vs. a dedicated `case_fold` translation unit in `yampt.core/utility` so the fold data/logic is physically separable from the rest of `string_utils`.
- **Future backend choice**: `utf8proc` (small, pure C, vcpkg) vs. an expanded hand table. Decide only when coverage is actually needed; this spec just ensures the swap is a one-place change.
- **Offset strategy for length-changing folds**: whether to commit now to a fold-aware offset mapping in the highlight/glossary consumers (future-proof but more work) or defer until a length-changing backend is actually adopted.
