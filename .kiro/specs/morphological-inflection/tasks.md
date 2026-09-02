# Implementation Plan

## Overview

Replace Hunspell affix-expansion with a proper morphological generator (Morfeusz 2 / SGJP, BSD-2-clause, C++ API, Windows) for Polish `.top` inflection generation, while keeping every other language on the existing Hunspell path. The core move is a backend seam: an `inflector_t` interface consumed by `loc_generator`, implemented by the existing Hunspell code and by a new Morfeusz-backed class, chosen by a language-keyed factory that falls back to Hunspell when the Polish morphological data is unavailable. The `.top` format and the generation pipeline are unchanged. Work order: define the seam and adapt the Hunspell backend, switch `loc_generator` to consume the interface, add the factory + callers, then integrate the Morfeusz dependency and implement the backend, then tests and docs. This is a large dependency change and Polish-only.

## Tasks

- [ ] 1. Define the inflector interface
  - `inflector_t` in `yampt.core/source/creator/inflector.hpp`: `is_loaded()`, `word_forms(word)`, `phrase_forms(phrase)`.
  - _Requirements: R1.2, R3.1_

- [ ] 2. Adapt the Hunspell backend to the interface
  - Make `inflection_t` implement `inflector_t` (rename to `hunspell_inflector_t` or keep the name and derive). Behavior unchanged (`stem` + `suffix_suggest`, phrase combination).
  - Update `yampt.core.vcxproj` + `.filters` if files are renamed.
  - _Requirements: R6.1, R6.3_

- [ ] 3. Switch loc_generator to consume the interface
  - `generation_input_t` takes `const inflector_t & inflector` instead of `hunspell_aff_path`/`hunspell_dic_path`.
  - `build_top_entries` uses the passed inflector; DIAL loop, lowercasing, dedup, collisions, `.top` writing unchanged.
  - _Requirements: R3.1, R3.2, R3.3_

- [ ] 4. Add the language-selected inflector factory
  - `inflector_factory::make_inflector(const inflector_request_t &)` in `yampt.core/source/creator/`: Polish + morphological data present → Morfeusz backend; else → Hunspell backend (with a `[warning]` when Polish falls back). Request carries language code/dictionary_prefix, hunspell aff/dic paths, morphological data dir.
  - Extract a pure selection predicate (language → backend kind) for testing.
  - _Requirements: R2.1, R2.2, R5.1, R5.2_

- [ ] 5. Wire GUI and CLI callers to the factory
  - `sidebar_controller_t::on_generate_loc_requested` and `user_interface_t::make_loc`: build the inflector via `make_inflector`, pass into `generate`. Identical output for GUI and CLI.
  - Add a `resource_paths` accessor for the Polish morphological data directory.
  - _Requirements: R2.3, R3.2_

- [ ] 6. Integrate the Morfeusz dependency
  - Add Morfeusz for `x64-windows` (vcpkg port if available, else `external/` submodule / documented prebuilt) with include paths, link, and a DLL-copy post-build target mirroring `CopyCTranslate2Dll`.
  - Deploy the SGJP dictionary data next to the executable (documented folder, gitignored if large), with acquisition steps in the build steering. Retain license notices. Upstream source stays read-only.
  - _Requirements: R4.1, R4.2, R4.3_

- [ ] 7. Implement the Morfeusz-backed inflector
  - `morfeusz_inflector_t` (PIMPL) implementing `inflector_t`: load the SGJP data; `word_forms` obtains the full paradigm (analyze→generate or direct generate per the Morfeusz API); `phrase_forms` reuses `phrase_form_builder`. `is_loaded()` reflects data availability.
  - Add files to `yampt.core.vcxproj` + `.filters`.
  - _Requirements: R1.1, R1.3, R5.2_

- [ ] 8. Tests
  - `[u]`: the factory's selection predicate returns the morphological backend for the Polish code and Hunspell for others (backend construction stubbed so no data needed); `phrase_form_builder` behavior retained.
  - `[i]`/manual (data present): Polish dict with `choroba` yields `.top` containing `chorobą`; data absent → fallback warning + Hunspell-equivalent `.top`; non-PL `.top` unchanged.
  - Register new test files in `yampt.tests.vcxproj` + `.filters`.
  - _Requirements: R7.1, R7.2, R7.3, R7.4_

- [ ] 9. Update documentation
  - CHANGELOG `[CHANGE]` (yTranslator): Polish localization generation now produces full inflectional paradigms for topics; other languages unchanged.
  - `docs/yTranslator-Manual.md`: describe the Polish morphological generation and the per-language behavior.
  - Build steering: document Morfeusz + SGJP data acquisition, deployment, and fallback.
  - README + README.bbcode in sync only if localization generation is user-described (no library names).
  - _Requirements: R2, R4.2, R5_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1], "depends_on": [] },
    { "wave": 2, "tasks": [2, 3], "depends_on": [1] },
    { "wave": 3, "tasks": [4, 5], "depends_on": [2, 3] },
    { "wave": 4, "tasks": [6], "depends_on": [4] },
    { "wave": 5, "tasks": [7], "depends_on": [6] },
    { "wave": 6, "tasks": [8, 9], "depends_on": [5, 7] }
  ]
}
```

The interface (1) unblocks adapting Hunspell (2) and switching the generator (3). The factory (4) and callers (5) come next — at this point the feature works end-to-end on Hunspell for all languages, including Polish (the Morfeusz path just isn't available yet). Dependency integration (6) precedes the Morfeusz backend implementation (7). Tests and docs (8, 9) last. The seam-first order means the pipeline is decoupled and shippable before the heavy dependency lands.

## Notes

- Polish-only and a large dependency change: Morfeusz (BSD-2-clause) + its SGJP dictionary data. The library source in `external/` stays read-only; wrapping is done in `morfeusz_inflector_t`.
- Selection is keyed on the resolved native language in one factory; adding another language's morphological backend later is a localized change (new backend + one factory branch), not a rewrite.
- Runtime fallback: if the Polish morphological data is missing or fails to load, generation falls back to the existing Hunspell path with a `[warning]`, never aborting or emitting empty output.
- The `.top` format, the generation pipeline (DIAL loop, collisions, phrase combination), the runtime `.top` consumer, and the spell checker are all unchanged.
- The seam-first task order lets everything but the Morfeusz dependency be implemented and verified with the existing Hunspell backend before the dependency is integrated.
- Building/running tests and acquiring the Morfeusz data are manual (no-build-or-test rule).
