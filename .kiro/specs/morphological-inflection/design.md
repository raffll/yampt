# Design — Morphological Dictionary for Inflection Generation (yTranslator, PL-only)

## Context (current mechanics)

- **Pipeline** — `loc_generator::generate(input)` → `build_top_entries(context, top_input)` constructs a local `inflection_t inflector; inflector.load(top_input.aff_path, top_input.dic_path);` (falls back to identity mappings on failure with a `[warning]`), then per DIAL record calls `process_top_record` → `inflector.phrase_forms(new_text)` and inserts `{form_lowercased, canonical}` into a dedup/collision context. `loc_file_writer::write` emits tab-separated `key\tvalue`.
- **inflection_t** (yampt.core/source/creator/inflection.hpp/.cpp) — PIMPL over Hunspell. `word_forms` → `stem` + `suffix_suggest`; `phrase_forms` → split, per-word candidates, `phrase_form_builder::build_phrase_forms(..., max_phrase_forms=2000)`, removes the nominative. `load(aff, dic)`, `is_loaded()`.
- **generation_input_t** (loc_generator.hpp) — `{ const dict_t & dict; std::string output_directory; std::string esm_name; codepage_t codepage; std::string hunspell_aff_path; std::string hunspell_dic_path; }`.
- **Triggers** — GUI `sidebar_controller_t::on_generate_loc_requested`: resolves `session.native_language()` → `resolve_hunspell_locale` → `dictionaries_dir()/<locale>.aff|.dic`, builds `generation_input_t`, calls `generate`. CLI `user_interface_t::make_loc`: resolves locale/codepage from `languages.json`, builds `generation_input_t`, calls `generate`.
- **Language resolution** — `language_config` over `languages.json` (`dictionary_prefix`, `codepage`, `nllb_code`). No PL-only branch anywhere in generation today.
- **Consumer** — `inflection_store_t` reads `.top`/`.mrk` for editor highlighting only.

## Design Goals

Add a Polish morphological generator (Morfeusz/SGJP) behind a seam so `loc_generator` is backend-agnostic (R3), select it by native language (R2), leave every other language on Hunspell (R6.1), integrate the dependency cleanly for `x64-windows` (R4), fall back to Hunspell on unavailability (R5), and keep the `.top` format and pipeline intact (R3.3, R6). Honor architecture rules (yampt.core purity, one class per file, `_t` suffix, external read-only).

## Decision: an `inflector_t` interface implemented by both backends

Introduce a pure interface in yampt.core (the seam the pipeline consumes, R3.1):

```cpp
// yampt.core/source/creator/inflector.hpp
class inflector_t
{
public:
    virtual ~inflector_t() = default;
    virtual bool is_loaded() const = 0;
    virtual std::vector<std::string> word_forms(const std::string & word) const = 0;
    virtual std::vector<std::string> phrase_forms(const std::string & phrase) const = 0;
};
```

Two implementations:
- `hunspell_inflector_t` — the current `inflection_t` renamed/adapted to implement `inflector_t` (its behavior is unchanged; it keeps `stem`+`suffix_suggest`). Minimal churn: either `inflection_t` gains the interface or is wrapped.
- `morfeusz_inflector_t` — new, wraps the Morfeusz C++ API (PIMPL, same as `inflection_t` hides Hunspell), producing full paradigms for Polish. `phrase_forms` reuses `phrase_form_builder` for multi-word combination (R1.3).

`build_top_entries` takes an `const inflector_t *` (or reference) instead of constructing `inflection_t` itself. The DIAL loop, lowercasing, dedup, collision handling, and `.top` writing are untouched (R3.1, R3.3).

### Rejected alternative

A backend enum inside `inflection_t` that switches between Hunspell and Morfeusz internally. Rejected: mixes two unrelated native libraries and two data models in one class (violates one-responsibility), and forces `generation_input_t` to carry both backends' paths regardless. An interface + two classes is the clean fit and matches the naming/one-class rules.

## Decision: caller constructs the inflector, passes it into generate

Change `generation_input_t` to receive a ready inflector rather than raw Hunspell paths, so the pipeline never picks a backend or loads the wrong data (R3.2):

```cpp
struct generation_input_t
{
    const dict_t & dict;
    std::string output_directory;
    std::string esm_name;
    codepage_t codepage;
    const inflector_t & inflector;   // constructed and loaded by the caller
};
```

A small factory selects and builds the backend by language (R2.1, R2.2):

```cpp
// yampt.core/source/creator/inflector_factory.hpp (namespace, no mutable state)
std::unique_ptr<inflector_t> make_inflector(const inflector_request_t & request);
// request carries: language code / dictionary_prefix, hunspell aff/dic paths, morfeusz data dir
```

`make_inflector` returns a `morfeusz_inflector_t` (loaded from the Polish morphological data) when the language is Polish and the data loads, otherwise a `hunspell_inflector_t` loaded from the aff/dic paths. The fallback (Polish but morphological data missing) also returns the Hunspell backend with a `[warning]` (R5.1). The GUI and CLI callers both use `make_inflector` and pass the result into `generate`, guaranteeing identical output (R2.3).

Rationale: keeps backend selection in one place keyed on the resolved language (R2.2), keeps `loc_generator` a pure consumer of `inflector_t`, and prevents the "load Hunspell paths into the Morfeusz backend" mistake because each backend is constructed with only its own inputs inside the factory.

## Decision: Morfeusz as the Polish morphological backend

Morfeusz 2 / SGJP (Institute of Computer Science PAS) is BSD-2-clause, has a C++ API and Windows builds, and provides both analysis (form → lemma+tags) and generation (lemma+tags → forms). `morfeusz_inflector_t::word_forms(word)` obtains the full paradigm by: analyzing `word` to its lemma(s) + tagset, then generating all surface forms for those lemmas (the exact API call sequence — `generate` over the lemma, or analyze-then-generate — is finalized against the Morfeusz API during implementation; see Open Decisions). This yields forms Hunspell's affix flags miss, e.g. `chorobą` from `choroba` (R1.1, the motivating case).

## Dependency & Data (R4)

- **Library**: add Morfeusz. Preferred: a vcpkg port if available for `x64-windows`; otherwise an `external/morfeusz` submodule (or a documented prebuilt) integrated like CTranslate2 (include path + link + a DLL-copy post-build target mirroring `CopyCTranslate2Dll`). The upstream source stays read-only; wrapping is done in `morfeusz_inflector_t` (external-dependencies rule).
- **Dictionary data**: the SGJP dictionary file is deployed next to the executable (a documented folder, analogous to `models/` / `dictionaries/`), gitignored if large / separately licensed, with acquisition steps added to the build steering (a download script or documented manual step). Its license notice is retained.
- **Runtime resolution**: the Polish morphological data path is resolved via `resource_paths` (a new accessor, consistent with `dictionaries_dir()`/`models_dir()`), joined with `string_utils::join_path`.

The design/tasks document the include dirs, link, and copy target so the standard `x64-windows` build is unaffected for everyone not on Polish, and Morfeusz's absence at build time is handled (the feature is additive; if the dependency is not yet integrated the Hunspell path remains the only backend).

## Component Changes

| Area | Change |
|------|--------|
| `yampt.core/source/creator/inflector.hpp` (new) | `inflector_t` interface |
| `yampt.core/source/creator/inflection.hpp/.cpp` | make `inflection_t` implement `inflector_t` (renamed to `hunspell_inflector_t` or kept and adapted) |
| `yampt.core/source/creator/morfeusz_inflector.hpp/.cpp` (new) | Morfeusz-backed `inflector_t` (PIMPL) |
| `yampt.core/source/creator/inflector_factory.hpp/.cpp` (new) | `make_inflector` language-selected factory + fallback |
| `yampt.core/source/creator/loc_generator.hpp/.cpp` | `generation_input_t` takes `const inflector_t &`; `build_top_entries` consumes the interface |
| `yampt.translator/source/session/sidebar_controller.cpp` | build the inflector via factory, pass into `generate` |
| `yampt.cli/source/interface/user_interface.cpp` | same, for `--make-loc` |
| `yampt.qt/source/resource_paths.*` | Polish morphological data dir accessor |
| build files (vcxproj / targets / vcpkg or external submodule) | Morfeusz include/link/copy; data deploy target |

## Data Flow

Trigger (GUI/CLI) → resolve native language → `make_inflector(request)`: Polish + data present → `morfeusz_inflector_t` (full paradigms); else → `hunspell_inflector_t` (with `[warning]` if Polish fell back) → `generate(input{..., inflector})` → `build_top_entries` calls `inflector.phrase_forms` per DIAL topic → `loc_file_writer::write` `.top`. Runtime `.top` consumption by `inflection_store_t` is unchanged (R6.2).

## Error Handling

- Morfeusz data missing / load failure → factory returns Hunspell backend, logs `[warning]` (R5.1), generation proceeds.
- Both backends unavailable (no Hunspell dict either) → existing identity-mapping fallback in `build_top_entries` (unchanged) still applies.
- Non-Polish language → factory returns Hunspell backend directly, no warning (R6.1).

## Testing Strategy (R7)

`[u]` (in-memory, no library/data):
- `inflector_factory::make_inflector` (or the selection predicate extracted from it) returns the morphological backend type for the Polish code and the Hunspell type for others — tested via the selection logic with backend construction stubbed/guarded so the test needs no dictionary data.
- `phrase_form_builder` combination behavior is retained (existing tests).

`[i]` / manual (needs Morfeusz + SGJP data present):
- Generate for a Polish dict with topic `choroba`; assert the produced `.top` contains `chorobą` (the Hunspell-missed form) (R7.2).
- Remove the morphological data; assert Polish generation logs the fallback warning and produces the same `.top` as the Hunspell path (R7.3).
- Non-PL dict produces identical `.top` before/after (R7.4).

Building/running tests and acquiring the Morfeusz data are manual (no-build-or-test rule). Test names: `owner::member, description`, e.g. `"inflector_factory::make_inflector, polish selects morphological backend"`.

## Files Touched

See Component Changes; plus `yampt.core.vcxproj` + `.filters` (new creator files), the tests project + `.filters` for the `[u]` selection test, and build/dependency files for Morfeusz.

## Documentation

- CHANGELOG `[CHANGE]` (yTranslator): for Polish, localization-file generation now produces complete inflectional paradigms for dialogue topics (recognizing case forms the previous method missed); other languages are unchanged. (`[CHANGE]` — visible improvement in generated `.top` coverage for PL users.)
- `docs/yTranslator-Manual.md`: in the localization-generation description, note that Polish uses a full morphological dictionary so topics are recognized in all grammatical cases, and that other languages use the spellcheck-dictionary method.
- Build steering (`.kiro/steering` translation-engine-build / project-paths): document acquiring and deploying the Morfeusz library and SGJP dictionary data, and the fallback behavior. (Build/dependency details stay out of the user-facing README/CHANGELOG per the changelog rules.)
- README + README.bbcode in sync only if localization generation is described there; keep it user-facing (no library names).
