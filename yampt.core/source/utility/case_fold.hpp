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
