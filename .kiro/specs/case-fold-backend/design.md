# Design — Swappable Case-Fold Backend

Make the fold source a single documented seam so future language coverage is a one-place change, without adding a dependency or changing behavior today.

## Current structure (recap)

`string_utils.cpp` contains, in an anonymous namespace:
- `fold_code_point(uint32_t) -> uint32_t` — the range rules (the seam, already de-facto).
- `decode_utf8(...)`, `append_code_point(...)` — the UTF-8 codec.
- `to_lower_utf8` — the loop: decode → `fold_code_point` → encode.
- `case_insensitive_equal_utf8` — folds both sides and compares.

The seam already exists; this spec makes it explicit, contract-bound, physically separable, and enforced.

## Chosen approach — minimal seam (Open Decision resolved)

Keep a single free function as the fold seam (no interface/policy, no build flags — those are speculative abstraction the project rules discourage, and there is no plan to ship multiple backends simultaneously). The swap point is the seam's body. Move it to its own translation unit so the fold data/logic is separable from the rest of `string_utils`.

### New file: `yampt.core/source/utility/case_fold.hpp/.cpp`

```cpp
// case_fold.hpp
#pragma once
#include <cstdint>

namespace case_fold {

// Contract:
//  - input: a Unicode code point
//  - output: its SIMPLE (1 code point -> 1 code point) lowercase code point,
//            or the input unchanged if it has no simple lowercase in the
//            supported set.
//  - INVARIANT: the returned code point MUST encode to the same number of
//    UTF-8 bytes as the input code point (byte-length preserving). Callers
//    reuse folded-string byte offsets against the original string; a
//    length-changing fold would corrupt those offsets.
//  - Do NOT add mappings that change UTF-8 byte length (e.g. U+0130 -> U+0069,
//    or any 1:many fold such as U+00DF -> "ss"). See design note.
std::uint32_t to_lower(std::uint32_t code_point);

} // namespace case_fold
```

```cpp
// case_fold.cpp — the current fold_code_point body, verbatim, renamed.
```

`string_utils.cpp` then:
- includes `case_fold.hpp`,
- keeps `decode_utf8` / `append_code_point` / the `to_lower_utf8` loop,
- calls `case_fold::to_lower(code_point)` where it currently calls `fold_code_point`.

This is the entire seam: to widen coverage later, edit only `case_fold.cpp` (add ranges) or replace its body with a library call. Nothing else moves.

### Why a separate TU, not just a documented function in place

- Physical separation makes the fold source obviously the one place to change.
- Keeps `string_utils` focused on generic string helpers; the Unicode fold data is its own concern (aligns with one-file-one-responsibility).
- A future `utf8proc` swap becomes: `case_fold.cpp` includes `<utf8proc.h>` and returns `utf8proc_tolower(cp)` (guarded per the invariant), touching one file.

## Byte-length invariant — made explicit and enforced

- The contract comment above lives at the seam (R1.2, R3.1).
- A unit test (`tests.case_fold.cpp` or extend `tests.string_utils_utf8.cpp`) asserts that for a representative sample spanning every supported range, `utf8_length(to_lower(cp)) == utf8_length(cp)`. A small helper computes a code point's UTF-8 length.
- This test is the guard: if a future maintainer adds a cross-band mapping, it fails immediately, forcing the offset decision in R3.2 instead of a silent offset bug.

## Design note — how to add a language later (R4)

Recorded in this design doc (developer-facing), not shipped docs:

To add a language:
1. Identify the code-point ranges its letters occupy (upper/lower pairs).
2. Add range rules to `case_fold::to_lower`, OR swap the body to `utf8proc_tolower` for full coverage.
3. Run the byte-length invariant test.

Hazard characters that break the invariant (must be excluded or handled):
- `ß` U+00DF → "ss" (1:many) — already excluded.
- Turkish dotted/dotless I: `İ` U+0130 (2 bytes) → `i` U+0069 (1 byte) — cross-band.
- Ligatures and other 1:many folds.

If broad coverage is needed and these characters must fold, the alternative is to stop reusing folded offsets on the original string: fold-aware matching that maps folded offsets back to original offsets (in `highlight_coordinator` / `highlight_applier` / `glossary`). That is a larger change and is deferred until a length-changing backend is actually adopted (R3.2 follow-up).

## Files

New:
- `yampt.core/source/utility/case_fold.hpp/.cpp` — the seam (moved `fold_code_point`, renamed `case_fold::to_lower`, with the contract comment). Register in `yampt.core.vcxproj` + filters.

Modified:
- `yampt.core/source/utility/string_utils.cpp` — include `case_fold.hpp`, call `case_fold::to_lower`, drop the local `fold_code_point`.

Tests:
- Extend `tests.string_utils_utf8.cpp` (or add `tests.case_fold.cpp`) with the byte-length invariant test and a regression sample for the six languages.

No call sites change. No public API change. No dependency added.

## Testing

- Regression: fold output for characteristic letters of PL/DE/FR/RU/IT/HU is byte-identical to before the refactor.
- Invariant: `to_lower(cp)` UTF-8 length equals `cp` UTF-8 length across all covered ranges.
- Existing `tests.string_utils_utf8.cpp` cases keep passing unchanged.
- Pure `[u]`, no file I/O.
