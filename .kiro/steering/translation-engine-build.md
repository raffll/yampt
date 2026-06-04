# Translation Engine Build — Findings

## VS 18 (Visual Studio 2026) STL Bug

VS 18 introduced internal STL dispatch symbols that cannot be resolved at link time:
- `__std_rotate`
- `__std_find_first_not_of_trivial_pos_1`

These symbols are called by abseil (a sentencepiece dependency) but do not exist in any linkable `.lib` file — not in `msvcprt.lib` (/MD), not in `libcpmt.lib` (/MT), not in `stl_asan.lib`. This is a confirmed VS 18 toolchain bug that affects any library using `std::rotate` or `std::string::find_first_not_of` through abseil.

## SentencePiece 0.2.x Requires Abseil

SentencePiece v0.2.0+ has a hard dependency on abseil for logging, hashing, and string operations. There is no build flag to disable it. The `SPM_USE_BUILTIN_PROTOBUF=ON` flag only removes the external protobuf dependency — abseil remains required.

## Approaches Attempted (All Failed)

1. **vcpkg sentencepiece** — abseil from vcpkg triggers `__std_find_first_not_of_trivial_pos_1` and `__std_rotate` unresolved externals.
2. **Source build sentencepiece + abseil** — same symbols missing. Even though both are compiled with the same VS 18 compiler, the STL internal symbols don't exist anywhere to link against.
3. **Force-linking libcpmt.lib** — causes MD/MT runtime mismatch errors (libcpmt is static CRT, project uses dynamic CRT).
4. **Switching to /MT (static CRT)** — matches libcpmt but vcpkg's other deps (SDL2, hunspell) are built with /MD, causing the opposite mismatch.
5. **x64-windows-static vcpkg triplet** — would work but abseil still has the missing STL symbols regardless of CRT choice.

## Blocked

The translation engine integration is blocked until one of:
- Microsoft ships a VS 18 update that resolves `__std_rotate` and `__std_find_first_not_of_trivial_pos_1`
- SentencePiece releases a version without abseil dependency
- An alternative tokenizer (not sentencepiece) is used

## What Works

- CTranslate2 compiles and links fine (source build with Ruy backend, no external deps beyond what's in its submodules)
- SentencePiece's own source code compiles fine — only the abseil portion fails at link time
- The `translation_engine.hpp/cpp` code is correct and ready
- The `dict_creator` integration code is correct and ready

## Current State of vcxproj Files

The vcxproj files have been heavily modified during debugging. They need to be reverted to a working state before proceeding with any alternative approach. Key changes that should be reverted:
- `libcpmt.lib` in AdditionalDependencies — causes MT/MD mismatch
- The massive abseil lib list — not needed if sentencepiece is removed
- `sentencepiece` removed from vcpkg.json — correct, don't re-add it
- `external/sentencepiece/` directory — can be deleted if unused
- RuntimeLibrary changes — should match original project settings

## Recommended Next Steps

1. Revert vcxproj files to pre-sentencepiece state (keep CTranslate2 and translation_engine code)
2. Stub out `sentencepiece_processor.h` usage with a compile-time `#ifdef HAS_SENTENCEPIECE` guard
3. Wait for VS 18 fix, or switch to VS 17 2022 for the sentencepiece/abseil build
4. Alternative: use CTranslate2's built-in tokenization if available, bypassing sentencepiece entirely
