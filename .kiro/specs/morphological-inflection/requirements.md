# Requirements — Morphological Dictionary for Inflection Generation (yTranslator, PL-only)

## Background — Current Behavior

yTranslator generates OpenMW `.top` localization files that map inflected forms of dialogue topics back to their canonical topic text, so the in-game topic highlighter recognizes a topic no matter which grammatical case it appears in.

- Generation is driven by `loc_generator::generate(const generation_input_t & input)` (yampt.core/source/creator/loc_generator.cpp). `generation_input_t` carries the dict, output dir, esm name, codepage, and `hunspell_aff_path` / `hunspell_dic_path`.
- `build_top_entries` constructs a local `inflection_t inflector; inflector.load(aff, dic);` and, per DIAL record, emits the lowercased canonical form plus every inflected form from `inflector.phrase_forms(new_text)`, written by `loc_file_writer::write` as tab-separated `key\tvalue` lines.
- `inflection_t` (yampt.core/source/creator/inflection.hpp/.cpp) wraps Hunspell (PIMPL). `word_forms` / `phrase_forms` generate inflected forms via **`Hunspell::stem`** then **`Hunspell::suffix_suggest`** on each stem (fallback: `suffix_suggest` on the word). Multi-word phrases take the cartesian product of per-word candidates (`phrase_form_builder`, capped at 2000).
- The Hunspell dictionary is the **native-language spellcheck** `.aff`/`.dic` (from the deployed `dictionaries/` folder via `resource_paths::dictionaries_dir()`, generated at build from the `external/dictionaries` submodule). Its affix flags are authored for spellchecking, not for full paradigm generation.
- Triggers: GUI sidebar action "Generate Localization Files" → `sidebar_controller_t::on_generate_loc_requested`; CLI `--make-loc` → `user_interface_t::make_loc`. Both resolve the native language dynamically (`languages.json` `dictionary_prefix`, `session.native_language()` / settings) — there is **no PL-only branch** today; generation works for any language that ships a Hunspell dictionary.
- The generated `.top`/`.mrk` files are read at runtime by `inflection_store_t` (yampt.translator) purely for editor highlighting; it does not call Hunspell.

## Problem

Hunspell's `suffix_suggest` only produces forms the specific `.aff`/`.dic` entry is flagged for. Polish spellcheck dictionaries do not flag every inflectional affix on every lemma, so valid inflected forms are never generated. Concrete example: `chorobą` (singular instrumental of `choroba`) is never emitted because `choroba` lacks the affix flag that produces that form. As a result, `.top` files miss legitimate case forms, and the in-game topic highlighter fails to recognize those forms. The spellcheck affix data is structurally the wrong tool for exhaustive paradigm generation.

## Problem Constraints

- A proper solution needs a morphological dictionary that can **generate** the full inflectional paradigm of a lemma. For Polish the standard resource is Morfeusz 2 / SGJP (Institute of Computer Science PAS), which exposes both analysis (form → lemma+tags) and generation (lemma+tags → forms) and is distributed under a 2-clause BSD license with a C++ API and Windows builds.
- This is a **large dependency change** (a new native library + its dictionary data, not from the existing vcpkg set or the `external/dictionaries` submodule) and is **Polish-only** — the current fine-tuning and inflection concerns are PL-centric, and Morfeusz covers Polish specifically.

## Goal

Replace Hunspell-based inflected-form generation with a proper morphological generator (Morfeusz/SGJP) for Polish, producing complete paradigms for `.top` generation, while leaving all other languages on the existing Hunspell path unchanged. The choice of backend is selected by the native language: Polish uses the morphological generator when available; every other language keeps the Hunspell affix-expansion behavior.

## User-Facing Outcomes

- When the native language is Polish and the morphological backend is available, generating localization files produces `.top` entries covering the full inflectional paradigm of each topic — including forms Hunspell misses (e.g. `chorobą`). The in-game highlighter then recognizes topics across all cases.
- For every non-Polish language, `.top` generation is unchanged (Hunspell affix expansion as today).
- If the morphological backend or its data is unavailable at runtime, Polish generation falls back to the current Hunspell behavior with a clear log message, rather than failing.
- No change to how the user triggers generation (same sidebar action, same `--make-loc` CLI command) or to the `.top` file format.

## Requirements

### R1 — Morphological generation backend

1.1 A new component generates the full inflectional paradigm of a word/phrase for Polish, using a morphological dictionary (Morfeusz/SGJP or an equivalent that provides lemma→forms generation). It exposes the same shape the generator needs: given a canonical word/phrase, return the set of inflected surface forms (excluding the canonical form itself, matching current `phrase_forms` semantics).
1.2 It lives in yampt.core (the generation path is core, Qt-free), as its own class with `.hpp`/`.cpp`, behind an interface compatible with how `build_top_entries` consumes forms (see R3).
1.3 Phrase handling mirrors today: for multi-word phrases, per-word forms are combined (reusing `phrase_form_builder` and its cap), so multi-word topics still generate combined forms.

### R2 — Language-selected backend

2.1 The inflection backend is chosen by the resolved native language. Polish selects the morphological generator; all other languages select the existing Hunspell `inflection_t`.
2.2 The selection is explicit and keyed on the language code / `dictionary_prefix` from `languages.json` (the existing dynamic language resolution), not hardcoded elsewhere. Adding a morphological backend for another language later must be a localized change at this selection point plus a new backend, not a rewrite.
2.3 Both the GUI trigger (`sidebar_controller_t::on_generate_loc_requested`) and the CLI trigger (`user_interface_t::make_loc`) resolve and pass the selected backend consistently, so GUI and CLI produce identical `.top` output for the same dict and language.

### R3 — Generator integration without duplicating the pipeline

3.1 `loc_generator`/`build_top_entries` consumes inflected forms through one abstraction so the DIAL loop, collision handling, lowercasing, and `.top` writing are unchanged regardless of backend. The design defines the seam (e.g. a small `inflector_t` interface with `word_forms`/`phrase_forms`, implemented by both the Hunspell-backed and Morfeusz-backed classes, chosen by the caller and passed into `generate`).
3.2 `generation_input_t` carries whatever the selected backend needs (the morphological backend needs its own dictionary data path/handle, not the Hunspell `.aff`/`.dic`). The design specifies whether `generation_input_t` gains a backend selector + data path, or receives a ready-constructed inflector. The pipeline must not call the wrong backend's load with the other backend's paths.
3.3 The `.top` file format (tab-separated `key\tvalue`) and `loc_file_writer` are unchanged.

### R4 — Dependency and data acquisition

4.1 The morphological library is added as a dependency in a way consistent with the project's dependency model: either via vcpkg if a suitable port exists, or as an `external/` submodule / documented build step if not, following the external-dependencies read-only rule (never modify the upstream source in place; wrap via project code).
4.2 The Polish morphological dictionary **data** (Morfeusz/SGJP's own dictionary file) is acquired and deployed next to the executable in a documented location (analogous to how translation models and Hunspell dictionaries are handled), and is excluded from the repo if large / separately licensed, with acquisition documented in the build steering. Its license (BSD 2-clause for Morfeusz; the SGJP dictionary has its own terms) is retained and noted.
4.3 The build integrates the new dependency for the standard `x64-windows` build without breaking the existing CTranslate2/Hunspell/vcpkg setup. The design/tasks specify linking, include paths, and any runtime DLL/data copy target, mirroring the existing `CopyCTranslate2Dll` / `dictionaries.targets` patterns.

### R5 — Runtime availability and fallback

5.1 If the morphological backend cannot load its dictionary at runtime (missing data, load failure), Polish generation falls back to the existing Hunspell `inflection_t` path and logs a clear `[warning]` (consistent with the existing "hunspell dictionary failed to load" warning), rather than aborting or producing empty output.
5.2 Availability is detected before use (an `is_loaded()`-style check), matching the existing `inflection_t::is_loaded` pattern.

### R6 — Scope and non-regression

6.1 Non-Polish languages are byte-for-byte unchanged: same Hunspell path, same `.top` output.
6.2 The runtime `.top` consumer (`inflection_store_t`) and the editor highlighter are unchanged — they read the produced `.top` regardless of how it was generated.
6.3 The spell checker (`spell_checker_t`) is untouched — it uses Hunspell `spell`/`suggest` for a different purpose and is not part of this change.
6.4 The `.cel`/`.mrk` generation status (currently the generator writes `.top`; `.cel`/`.mrk` builders are `[[maybe_unused]]`) is not expanded by this feature.

### R7 — Verification

7.1 Pure `[u]` tests (no disk, no UI) for the pure combinatorial logic that does not need the morphological library: the phrase-combination step still behaves (reusing `phrase_form_builder` tests) and the backend-selection function returns the morphological backend for the Polish language code and the Hunspell backend for others.
7.2 An `[i]`/manual verification (needs the morphological dictionary data present): generating for a Polish dict that contains a topic like `choroba` yields a `.top` containing `chorobą` (the specific form Hunspell misses), demonstrating the paradigm is more complete.
7.3 The fallback path is verifiable: with the morphological data absent, Polish generation logs the warning and produces the same `.top` as the current Hunspell path.
7.4 Non-Polish generation output is unchanged (a non-PL dict produces the same `.top` as before this change).

## Open Decisions

Resolved:
- PL-only; backend selected by native language; all other languages keep Hunspell. (R2)
- Morfeusz 2 / SGJP is the candidate morphological backend (BSD 2-clause, C++ API, Windows, provides generation). (Problem Constraints)
- Runtime fallback to Hunspell on unavailability, with a warning. (R5)
- `.top` format and the generation pipeline (DIAL loop, collisions, phrase combination) are unchanged; only the per-word/phrase form source changes behind a seam. (R3)

Deferred to design:
- Exact seam shape: an `inflector_t` interface implemented by both backends vs. a backend enum inside `inflection_t`. (R3.1)
- How `generation_input_t` carries backend choice + morphological data path vs. receiving a constructed inflector from the caller. (R3.2)
- Dependency delivery: vcpkg port vs. `external/` submodule + build step, and how the SGJP dictionary data is acquired/deployed/gitignored. (R4)
- Whether Morfeusz's generation API (lemma+tags → forms) is used directly, or analysis (form→lemma) followed by generation of the full tag set — i.e. how "all inflected forms of this canonical word" is obtained from the API. (R1.1)
- Whether the morphological backend also improves multi-word phrase coverage beyond the current cartesian product, or keeps the same phrase strategy. (R1.3)
