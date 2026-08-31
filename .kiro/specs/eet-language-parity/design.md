# Design — Language Expansion (EET Parity + Additional Languages)

## Context (current mechanics)

- **Language data** — `languages.json` (repo root, deployed next to the exe) is a flat JSON array; each entry has exactly five fields: `code`, `name`, `nllb`, `dictionary`, `codepage`. Current entries: EN/PL/DE/FR/RU/IT/HU.
- **Loader + helpers** — `language_config` (yampt.core/source/utility/language_config.hpp/.cpp). Struct `language_entry_t { code; display_name; nllb_code; dictionary_prefix; codepage; }`. Functions: `load(json_path)`, `find_by_code(languages, code)`, `resolve_codepage(languages, code)` (falls back to windows_1252), `resolve_dictionary_prefix(languages, code)` (falls back to `""`). Fully data-driven, no hardcoded language list. `load` casts the JSON `codepage` int directly to `codepage_t`.
- **Data-driven consumers** (all iterate the loaded vector, so new codes appear automatically):
  - First-run dialog (`first_run_dialog.cpp`) — fills From/To combos.
  - Language settings (`language_settings_view.cpp`) — fills foreign/native combos, and in `apply()` derives encoding index via `codepage_to_index(native_lang->codepage)`, NLLB target via `set_translation_target(native_lang->nllb_code)`, and the encoding note via `codepage_name(...)`. Spell-check combos are populated by scanning the `dictionaries/` folder for `.aff`/`.dic` stems.
  - Translation settings (`translation_settings_view.cpp`) — builds the all-codes-except-EN label.
  - `main_window.cpp` first-run flow, `translation_suggestion_view.cpp` local-model resolution.
- **Path** — `resource_paths::languages_file()` returns the resolved `languages.json` path; the established pattern is `language_config::load(resource_paths::languages_file())`. `resource_paths::dictionaries_dir()` returns the dictionaries directory with a trailing `/`.
- **Codepage machinery** — `codepage.hpp`: `enum class codepage_t { windows_1250=1250, windows_1251=1251, windows_1252=1252 }`, `supported_codepages`, `codepage_to_index` (1252→2), `index_to_codepage`, `codepage_name`. Exactly three codepages; ES/PT are both 1252, already supported.
- **The one hardcoded bypass** — `sidebar_controller_t::resolve_hunspell_locale` (yampt.translator/source/session/sidebar_controller.cpp):
  ```cpp
  static const std::map<std::string, std::string> locale_map = {
      { "PL", "pl_PL" }, { "DE", "de_DE" }, { "FR", "fr_FR" },
      { "RU", "ru_RU" }, { "IT", "it_IT" }, { "HU", "hu_HU" },
  };
  const auto it_locale = locale_map.find(language_code);
  if (it_locale == locale_map.end()) return {};
  return it_locale->second;
  ```
  Caller `on_generate_loc_requested` reads the code from `m_deps.session.native_language()` (uppercase, e.g. "PL"), and uses the returned locale verbatim as the dictionary filename prefix: `dict_dir + locale + ".aff"/".dic"`. That prefix is exactly `language_entry_t::dictionary_prefix`, and the map values equal the `dictionary` fields in `languages.json`. The unknown→empty contract equals `resolve_dictionary_prefix`'s not-found→`""`. The file already includes `<resource_paths.hpp>` and `<map>` but not `<utility/language_config.hpp>`.
- **Dictionaries** — `dictionaries/{prefix}.aff` + `{prefix}.dic` (en_US, pl_PL, de_DE, fr_FR, ru_RU, it_IT, hu_HU), plus `dictionaries/LICENSE` listing per-dictionary upstream source/license (wooorm/dictionaries, UTF-8).
- **Out of scope (confirmed)** — `file_list::detect_language` matches vanilla ESM byte sizes (EN/DE/PL/FR/RU); no ES/PT vanilla release exists, so no change. The lowercase `"pl"` defaults in `session.cpp` / `sidebar_model.cpp` are default-when-unset fallbacks, not per-language maps; unchanged.

## Design Goals

Add Spanish (ES) and Portuguese (PT, European `pt_PT`) as data (R1) with UTF-8 dictionaries (R2), then add 15 further languages the same way (R8), make the last hardcoded locale path data-driven (R3), touch no codepage machinery (R4), update docs/steering/changelog (R5), regress nothing (R6), and cover the pure logic with `[u]` tests (R7).

## Decision: data + one refactor, zero UI code change

All UI selectors iterate the loaded language vector, so every added language (ES, PT, and the 15) flows through the first-run dialog, both settings pages, NLLB target selection, and codepage resolution with no code change once the JSON entries and dictionary files exist. The only C++ change is the `resolve_hunspell_locale` refactor — and once it lands, it resolves every added language automatically, so the 15 further languages add zero C++ (R8.7).

## Component Changes

### 1. languages.json (R1)

Append two entries in the existing field shape (no schema change):

```json
{ "code": "ES", "name": "Spanish",    "nllb": "spa_Latn", "dictionary": "es_ES", "codepage": 1252 },
{ "code": "PT", "name": "Portuguese", "nllb": "por_Latn", "dictionary": "pt_PT", "codepage": 1252 }
```

Then append the 15 additional entries after ES/PT (R8.1), in the same field shape (no schema change):

```json
{ "code": "CS", "name": "Czech",            "nllb": "ces_Latn", "dictionary": "cs_CZ", "codepage": 1250 },
{ "code": "SK", "name": "Slovak",           "nllb": "slk_Latn", "dictionary": "sk_SK", "codepage": 1250 },
{ "code": "SL", "name": "Slovenian",        "nllb": "slv_Latn", "dictionary": "sl_SI", "codepage": 1250 },
{ "code": "HR", "name": "Croatian",         "nllb": "hrv_Latn", "dictionary": "hr_HR", "codepage": 1250 },
{ "code": "RO", "name": "Romanian",         "nllb": "ron_Latn", "dictionary": "ro_RO", "codepage": 1250 },
{ "code": "UK", "name": "Ukrainian",        "nllb": "ukr_Cyrl", "dictionary": "uk_UA", "codepage": 1251 },
{ "code": "BG", "name": "Bulgarian",        "nllb": "bul_Cyrl", "dictionary": "bg_BG", "codepage": 1251 },
{ "code": "SR", "name": "Serbian",          "nllb": "srp_Cyrl", "dictionary": "sr_RS", "codepage": 1251 },
{ "code": "NL", "name": "Dutch",            "nllb": "nld_Latn", "dictionary": "nl_NL", "codepage": 1252 },
{ "code": "SV", "name": "Swedish",          "nllb": "swe_Latn", "dictionary": "sv_SE", "codepage": 1252 },
{ "code": "DA", "name": "Danish",           "nllb": "dan_Latn", "dictionary": "da_DK", "codepage": 1252 },
{ "code": "NB", "name": "Norwegian Bokmal", "nllb": "nob_Latn", "dictionary": "nb_NO", "codepage": 1252 },
{ "code": "FI", "name": "Finnish",          "nllb": "fin_Latn", "dictionary": "fi_FI", "codepage": 1252 },
{ "code": "CA", "name": "Catalan",          "nllb": "cat_Latn", "dictionary": "ca_ES", "codepage": 1252 },
{ "code": "GL", "name": "Galician",         "nllb": "glg_Latn", "dictionary": "gl_ES", "codepage": 1252 }
```

Repo-root `languages.json` is the source of truth; build/output copies are produced by the build, not hand-edited (R1.3). Placement follows the existing ordering (after HU/PT is fine; the file is not required to be sorted). Finnish carries a `dictionary` value (`fi_FI`) even though no dictionary file ships — the prefix resolves from data, the file's absence is handled downstream (R8.3).

### 2. Dictionaries (R2)

Add UTF-8 Hunspell files sourced from `wooorm/dictionaries`:
- `dictionaries/es_ES.aff` + `dictionaries/es_ES.dic`
- `dictionaries/pt_PT.aff` + `dictionaries/pt_PT.dic`

Prefixes match the `dictionary` fields exactly (R2.2), so `dictionaries/<prefix>.aff/.dic` resolves and the Language settings spell-check combos list them (they scan the folder). Append the two new source/license notices to `dictionaries/LICENSE` (R2.4). Packaging already copies the `dictionaries/` folder, so no packaging-list change is needed beyond the files existing (R2.3).

Portuguese is European `pt_PT` (not EET's `pt_BR`); NLLB code (`por_Latn`) and codepage (1252) are identical either way, only the dictionary differs. Spanish/Portuguese wooorm dictionaries are UTF-8, matching the project's preference over EET's ISO-8859-1 copies.

Then add 14 more UTF-8 Hunspell pairs from `wooorm/dictionaries` for the additional languages (R8.2) — all except Finnish. Each pair's prefix matches its `dictionary` field, sourced from the wooorm folder shown:

| Dictionary files | wooorm folder | codepage |
|------------------|---------------|----------|
| `cs_CZ.aff` / `.dic` | cs | 1250 |
| `sk_SK.aff` / `.dic` | sk | 1250 |
| `sl_SI.aff` / `.dic` | sl | 1250 |
| `hr_HR.aff` / `.dic` | hr | 1250 |
| `ro_RO.aff` / `.dic` | ro | 1250 |
| `uk_UA.aff` / `.dic` | uk | 1251 |
| `bg_BG.aff` / `.dic` | bg | 1251 |
| `sr_RS.aff` / `.dic` | sr (Cyrillic variant, NOT sr-Latn) | 1251 |
| `nl_NL.aff` / `.dic` | nl | 1252 |
| `sv_SE.aff` / `.dic` | sv | 1252 |
| `da_DK.aff` / `.dic` | da | 1252 |
| `nb_NO.aff` / `.dic` | nb | 1252 |
| `ca_ES.aff` / `.dic` | ca | 1252 |
| `gl_ES.aff` / `.dic` | gl | 1252 |

Each `.aff` must be verified to declare `SET UTF-8` (not `SET ISO8859-1`) before inclusion. Append each dictionary's upstream license notice to `dictionaries/LICENSE`.

**Serbian** uses the Cyrillic `sr` folder (`srp_Cyrl`, codepage 1251) to match the Russian Cyrillic path; the Latin variant (`sr-Latn`, 1250) is intentionally not chosen (R8.4).

**Finnish ships no dictionary (R8.3).** `wooorm/dictionaries` has no Finnish entry, so no `fi_FI.aff`/`.dic` is added. Finnish is translation-only: NLLB `fin_Latn` translation works, but the missing-dictionary runtime behavior applies — the Language settings spell-check combo (which scans the `dictionaries/` folder for `.aff`/`.dic` stems) simply does not list `fi_FI`, and localization-file generation for Finnish resolves the prefix `fi_FI` but finds no file, so the hunspell path is skipped (the same empty-locale/missing-file skip described in Error Handling). No error, no crash — Finnish just has no spellcheck until a UTF-8 Finnish dictionary is sourced.

### 3. resolve_hunspell_locale refactor (R3)

Replace the hardcoded map with a data-driven lookup:

```cpp
std::string sidebar_controller_t::resolve_hunspell_locale(const std::string & language_code) const
{
    const auto languages = language_config::load(resource_paths::languages_file());
    return language_config::resolve_dictionary_prefix(languages, language_code);
}
```

Add `#include <utility/language_config.hpp>`. This preserves every current language (PL→pl_PL, DE→de_DE, FR→fr_FR, RU→ru_RU, IT→it_IT, HU→hu_HU — exactly the `dictionary` fields), resolves ES→es_ES / PT→pt_PT and any future entry with no further change (R3.2), and keeps the unknown→empty contract because `resolve_dictionary_prefix` returns `""` when not found (R3.3). The `<map>` include may become unused after removing the map; remove it if nothing else in the file needs it (confirm during implementation).

**This single refactor covers all 15 additional languages with no extra code (R8.7).** Because the lookup is now purely data-driven from `languages.json`, each new entry (CS→cs_CZ, UK→uk_UA, NL→nl_NL, …) resolves its prefix automatically the moment it exists in the JSON. Finnish resolves to `fi_FI` too; the downstream missing-file skip (no `fi_FI.aff`/`.dic` on disk) handles the translation-only case without any special-casing in this function.

Note EN is deliberately absent from the old map (loc generation is for the native/target language, not English). `resolve_dictionary_prefix` would return `en_US` for "EN", but the caller only ever passes the native language code, so behavior is unchanged for the real call path. This is a benign superset, not a regression.

### 4. Codepage machinery (R4) — no change

ES/PT are 1252. `language_config::load` casts the JSON `codepage` int (1252) to `codepage_t::windows_1252` directly, and `codepage_to_index(windows_1252)` → 2 through the existing path. Nothing in `codepage.hpp` changes.

All 15 additional languages map to one of the three already-supported codepages, so none introduces new machinery (R8.6):
- **1250** (win1250, Central/Eastern European): CS, SK, SL, HR, RO — cast to `codepage_t::windows_1250`, `codepage_to_index` → 0.
- **1251** (win1251, Cyrillic): UK, BG, SR — cast to `codepage_t::windows_1251`, `codepage_to_index` → 1. Windows-1251 has imperfect coverage of some Ukrainian letters (e.g. ge-with-upturn), accepted for Morrowind text (R8.5).
- **1252** (win1252, Western European): NL, SV, DA, NB, FI, CA, GL — cast to `codepage_t::windows_1252`, `codepage_to_index` → 2.

Every value flows through the existing `codepage_to_index` mapping with no change to `codepage_t`, `supported_codepages`, `index_to_codepage`, `codepage_name`, or `codepage_iconv_name`.

## Data Flow

`languages.json` (+ ES/PT) → `language_config::load` → `language_entry_t` vector → every selector/derivation (combos, encoding index, NLLB target, codepage note) picks up ES/PT automatically. Native ES/PT selected → encoding index 2 (1252), NLLB `spa_Latn`/`por_Latn`, spell dictionary `es_ES`/`pt_PT`. Loc generation → `resolve_hunspell_locale(native_code)` → `resolve_dictionary_prefix` from `languages.json` → `dictionaries/es_ES.*` / `dictionaries/pt_PT.*`.

## Error Handling

- Missing dictionary files: the spell-check combo simply won't list a stem that has no `.aff`/`.dic` pair; loc generation with an empty locale skips the hunspell path (existing behavior). Shipping the files (R2) avoids this.
- Unknown language code passed to `resolve_hunspell_locale`: returns `""` (unchanged contract).

## Testing Strategy (R7)

Pure `[u]` tests, no disk (the helpers accept an in-memory `std::vector<language_entry_t>`, so `load` is never needed in a unit test). New file `tests.language_config.cpp` mirroring `tests.codepage.cpp` style:

- `find_by_code` returns ES/PT with correct `nllb_code`/`dictionary_prefix`/`codepage`; unknown → `nullptr`.
- `resolve_codepage("ES")`/`("PT")` → `windows_1252`; unknown → `windows_1252`.
- `resolve_dictionary_prefix("ES")`/`("PT")` → `es_ES`/`pt_PT`; unknown → `""`.
- A representative sample across all three codepages (R8.9): `resolve_codepage("CS")` → `windows_1250`, `resolve_codepage("UK")` → `windows_1251`, `resolve_codepage("NL")` → `windows_1252`; and the matching `resolve_dictionary_prefix` values (`CS`→`cs_CZ`, `UK`→`uk_UA`, `NL`→`nl_NL`), plus Finnish `resolve_dictionary_prefix("FI")` → `fi_FI` (prefix resolves from data even though no file ships). Unknown-code fallbacks remain (`resolve_codepage` → `windows_1252`, `resolve_dictionary_prefix` → `""`).

Because `resolve_hunspell_locale` delegates to `resolve_dictionary_prefix`, the refactor is covered by testing that helper directly (the controller has heavy Qt deps and is not unit-testable in isolation). Per test-before-fix, the ES/PT-and-unknown assertions on `resolve_dictionary_prefix` are the failing-first test for the refactor — they pass once the JSON has ES/PT, and the delegation guarantees the controller behaves identically.

Dictionary presence and real-`languages.json` loading are integration-level / manual, not `[u]` (R7.3, R7.4). Building and running tests is done manually by the user (no-build-or-test rule).

Test names follow `owner::member, description` with `[u]`, e.g. `"language_config::find_by_code, ES and PT resolve"`, `"language_config::resolve_dictionary_prefix, new and unknown codes"`.

## Files Touched

| File | Change |
|------|--------|
| `languages.json` | add ES, PT, and the 15 additional entries (17 new rows total) |
| `dictionaries/es_ES.aff` / `.dic`, `dictionaries/pt_PT.aff` / `.dic` (new) | UTF-8 Hunspell dictionaries |
| `dictionaries/{cs_CZ,sk_SK,sl_SI,hr_HR,ro_RO,uk_UA,bg_BG,sr_RS,nl_NL,sv_SE,da_DK,nb_NO,ca_ES,gl_ES}.aff` / `.dic` (new) | 14 UTF-8 Hunspell pairs from wooorm (no Finnish) |
| `dictionaries/LICENSE` | append the ES/PT + 14 additional source/license notices |
| `yampt.translator/source/session/sidebar_controller.cpp` | data-driven `resolve_hunspell_locale`, add `<utility/language_config.hpp>`, drop unused `<map>` if applicable (the ONLY code file, covers all languages) |
| `yampt.tests/source/tests.language_config.cpp` (new) | `[u]` tests for find_by_code / resolve_codepage / resolve_dictionary_prefix incl. ES/PT and a sample across all three codepages |
| `yampt.tests/yampt.tests.vcxproj` + `.filters` | register the new test file |
| `.kiro/steering/supported-languages.md` | add 17 rows + 17 NLLB code lines; note Finnish translation-only |
| `CHANGELOG.md`, `docs/*`, `docs/README.bbcode` | user-visible language-support additions |

Note the code surface stays exactly one production file (`sidebar_controller.cpp`) plus the one test file — the 15 additional languages add only data (JSON rows, dictionary files, license/doc lines), no C++.

## Documentation (R5)

- `supported-languages` steering: add all 17 rows to the language table — Spanish (ES, es_ES), Portuguese (PT, pt_PT), plus the 15 additional (CS/cs_CZ, SK/sk_SK, SL/sl_SI, HR/hr_HR, RO/ro_RO, UK/uk_UA, BG/bg_BG, SR/sr_RS, NL/nl_NL, SV/sv_SE, DA/da_DK, NB/nb_NO, FI/fi_FI, CA/ca_ES, GL/gl_ES), all NLLB-600M. Add the corresponding 17 lines to the NLLB code list (`eng_Latn → spa_Latn`, `eng_Latn → por_Latn`, `eng_Latn → ces_Latn`, `eng_Latn → slk_Latn`, `eng_Latn → slv_Latn`, `eng_Latn → hrv_Latn`, `eng_Latn → ron_Latn`, `eng_Latn → ukr_Cyrl`, `eng_Latn → bul_Cyrl`, `eng_Latn → srp_Cyrl`, `eng_Latn → nld_Latn`, `eng_Latn → swe_Latn`, `eng_Latn → dan_Latn`, `eng_Latn → nob_Latn`, `eng_Latn → fin_Latn`, `eng_Latn → cat_Latn`, `eng_Latn → glg_Latn`). Note in the table that Finnish is translation-only (no Hunspell dictionary shipped). Codepage section unchanged — the additions map to the already-listed win1250 (CS/SK/SL/HR/RO), win1251 (UK/BG/SR), win1252 (NL/SV/DA/NB/FI/CA/GL).
- `docs/yTranslator-Manual.md`: the local-model supported-languages line and any language enumeration add all 17 languages, noting that Finnish translates but has no spell-check dictionary. Only user-visible language support is described — no dictionary-source or refactor detail (manual-style rule).
- README + README.bbcode in sync if they enumerate supported languages.
- CHANGELOG `[NEW]` (yTranslator or Both Apps as appropriate): added Spanish, Portuguese, and the 15 additional languages; note Finnish is translation-only. No dictionary-source, build, or refactor internals (changelog rules).
