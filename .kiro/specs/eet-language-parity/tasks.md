# Implementation Plan

## Overview

Expand yampt's supported languages, driven entirely by data. Stage one reaches EET parity for Morrowind by adding Spanish (ES) and Portuguese (European `pt_PT`); stage two adds 15 further languages (CS, SK, SL, HR, RO, UK, BG, SR, NL, SV, DA, NB, FI, CA, GL). Because every language selector (first-run dialog, both settings pages, NLLB target, codepage resolution) already iterates `languages.json`, adding a JSON entry plus a UTF-8 Hunspell dictionary makes a language appear across the app with no UI code change. The one required code change is refactoring `sidebar_controller_t::resolve_hunspell_locale` from a hardcoded PL/DE/FR/RU/IT/HU→locale map to a data-driven lookup via `language_config::resolve_dictionary_prefix` — once it lands, it resolves every added language automatically, so the 15 further languages add zero C++.

Two caveats: **Finnish** has no wooorm dictionary, so it gets a `languages.json` entry (translation-only via NLLB `fin_Latn`) but ships no `.aff`/`.dic`; the spell-check combo omits it and loc-generation skips the hunspell path — Finnish is kept, not dropped. **Serbian** uses the Cyrillic wooorm variant (`sr` folder, `srp_Cyrl`, codepage 1251) to match the Russian path, not Latin `sr-Latn`. No codepage machinery changes: all 15 map to the existing 1250 / 1251 / 1252 codepages, and no UI code changes. Work order: unit test the data helpers first (test-before-fix for the refactor), add the JSON entries and dictionaries, refactor the locale resolver, then docs.

## Tasks

- [x] 1. Unit-test language_config helpers across all codepages and unknown codes
  - New `yampt.tests/source/tests.language_config.cpp`, purely in-memory (build a `std::vector<language_entry_t>` literal, no `load`, no disk).
  - `[u]`: `find_by_code` returns ES/PT with correct nllb_code/dictionary_prefix/codepage; unknown → nullptr.
  - `[u]`: `resolve_codepage` across the three codepages using a representative sample — `("ES")`/`("PT")`/`("NL")` → windows_1252, `("CS")` → windows_1250, `("UK")` → windows_1251; unknown → windows_1252.
  - `[u]`: `resolve_dictionary_prefix` for a representative sample — `("ES")`→es_ES, `("PT")`→pt_PT, `("CS")`→cs_CZ, `("UK")`→uk_UA, `("NL")`→nl_NL, `("FI")`→fi_FI (prefix resolves from data even though no dictionary file ships); unknown → "".
  - These are the failing-first tests backing the resolve_hunspell_locale refactor (which delegates to resolve_dictionary_prefix).
  - _Requirements: R7.1, R7.2, R7.4, R8.9_

- [x] 2. Register the new test file in the tests project
  - Add `tests.language_config.cpp` to `yampt.tests.vcxproj` + `.vcxproj.filters` (flat, disk-mirroring).
  - _Requirements: R7_

- [x] 3. Add all new languages to languages.json
  - Append the ES/PT entries — `{ "code": "ES", "name": "Spanish", "nllb": "spa_Latn", "dictionary": "es_ES", "codepage": 1252 }` and `{ "code": "PT", "name": "Portuguese", "nllb": "por_Latn", "dictionary": "pt_PT", "codepage": 1252 }` — to the repo-root source of truth, matching the existing field shape.
  - Then append the 15 additional entries after ES/PT, same shape: CS/ces_Latn/cs_CZ/1250, SK/slk_Latn/sk_SK/1250, SL/slv_Latn/sl_SI/1250, HR/hrv_Latn/hr_HR/1250, RO/ron_Latn/ro_RO/1250, UK/ukr_Cyrl/uk_UA/1251, BG/bul_Cyrl/bg_BG/1251, SR/srp_Cyrl/sr_RS/1251, NL/nld_Latn/nl_NL/1252, SV/swe_Latn/sv_SE/1252, DA/dan_Latn/da_DK/1252, NB/nob_Latn/nb_NO/1252, FI/fin_Latn/fi_FI/1252, CA/cat_Latn/ca_ES/1252, GL/glg_Latn/gl_ES/1252.
  - Finnish carries `dictionary` `fi_FI` even though no dictionary file ships (translation-only). Serbian uses the Cyrillic choice (`srp_Cyrl`, codepage 1251).
  - _Requirements: R1.1, R1.2, R1.3, R8.1, R8.3, R8.4_

- [x] 4. Add UTF-8 Hunspell dictionaries
  - Add `dictionaries/es_ES.aff` + `.dic` and `dictionaries/pt_PT.aff` + `.dic`, UTF-8, from wooorm/dictionaries.
  - Add the 14 additional UTF-8 Hunspell pairs from wooorm/dictionaries (all except Finnish): cs_CZ (folder cs), sk_SK (sk), sl_SI (sl), hr_HR (hr), ro_RO (ro), uk_UA (uk), bg_BG (bg), sr_RS (folder sr, Cyrillic variant — NOT sr-Latn), nl_NL (nl), sv_SE (sv), da_DK (da), nb_NO (nb), ca_ES (ca), gl_ES (gl).
  - Verify each `.aff` declares `SET UTF-8` (not `SET ISO8859-1`) before including it.
  - **Finnish ships no dictionary** — wooorm has no Finnish entry, so no `fi_FI.aff`/`.dic` is added; Finnish is translation-only and its spell-check/loc-hunspell path is skipped at runtime.
  - Append every added dictionary's source/license notice to `dictionaries/LICENSE`.
  - _Requirements: R2.1, R2.2, R2.3, R2.4, R8.2, R8.3, R8.4_

- [x] 5. Make resolve_hunspell_locale data-driven
  - Replace the hardcoded `locale_map` in `sidebar_controller_t::resolve_hunspell_locale` with `language_config::resolve_dictionary_prefix(language_config::load(resource_paths::languages_file()), language_code)`.
  - Add `#include <utility/language_config.hpp>`; remove `#include <map>` if it becomes unused.
  - Preserves existing locales for PL/DE/FR/RU/IT/HU, resolves ES/PT and all 15 additional languages automatically from the JSON, and returns "" for unknown codes. This single refactor is the only C++ change needed for the entire expansion.
  - _Requirements: R3.1, R3.2, R3.3, R8.7_

- [x] 6. Update steering and user documentation
  - `.kiro/steering/supported-languages.md`: add all 17 rows to the language table (ES/es_ES, PT/pt_PT, CS/cs_CZ, SK/sk_SK, SL/sl_SI, HR/hr_HR, RO/ro_RO, UK/uk_UA, BG/bg_BG, SR/sr_RS, NL/nl_NL, SV/sv_SE, DA/da_DK, NB/nb_NO, FI/fi_FI, CA/ca_ES, GL/gl_ES); add the matching 17 `eng_Latn → <nllb>` lines to the NLLB code list. Note that Finnish is translation-only (no Hunspell dictionary shipped). Codepage section unchanged — additions map to existing win1250 (CS/SK/SL/HR/RO), win1251 (UK/BG/SR), win1252 (NL/SV/DA/NB/FI/CA/GL).
  - `docs/yTranslator-Manual.md`: add all 17 languages to the local-model supported-languages line and any language enumeration; note Finnish translates but has no spell-check dictionary.
  - README + README.bbcode in sync if they list supported languages.
  - CHANGELOG `[NEW]`: added Spanish, Portuguese, and the 15 additional languages; note Finnish is translation-only (no dictionary-source/build/refactor internals).
  - _Requirements: R5.1, R5.2, R5.3, R8.3_

## Task Dependency Graph

```json
{
  "waves": [
    { "wave": 1, "tasks": [1, 3, 4], "depends_on": [] },
    { "wave": 2, "tasks": [2, 5], "depends_on": [1, 3] },
    { "wave": 3, "tasks": [6], "depends_on": [3, 4, 5] }
  ]
}
```

The data helpers test (1) and the JSON/dictionary data (3, 4) are independent and can start together. The refactor (5) is behavior-equivalent once the new codes exist in the JSON; the test (1) validates the helper it delegates to. Docs (6) come last, after the data and refactor are in place. The wave structure is unchanged by the expansion — the additional languages only enlarge the data added within tasks 3, 4, and 6, not the task dependencies.

## Notes

- No codepage machinery changes: every added language maps to an existing codepage, cast directly from the JSON `codepage` field by `language_config::load` — 1250 (CS/SK/SL/HR/RO), 1251 (UK/BG/SR), 1252 (ES/PT/NL/SV/DA/NB/FI/CA/GL); `codepage_to_index` already handles all three (R4, R8.6).
- Zero UI code change: first-run dialog, Language and Translation settings pages, and NLLB target selection all iterate the loaded language vector, so every added language appears automatically once the JSON entries and dictionaries exist (R6.2). The `resolve_hunspell_locale` refactor (task 5) is the only C++ change and covers all languages (R8.7).
- Finnish caveat: no wooorm Finnish dictionary exists, so Finnish is translation-only (NLLB `fin_Latn`) with a `languages.json` entry but no `.aff`/`.dic`; the spell-check combo omits `fi_FI` and loc-generation skips the hunspell path. Finnish is kept, not dropped (R8.3).
- Serbian is the Cyrillic variant (`srp_Cyrl`, codepage 1251, wooorm `sr` folder) to match the Russian path; Latin `sr-Latn` (1250) is intentionally not chosen (R8.4). Ukrainian/Bulgarian/Serbian share codepage 1251, which has imperfect Ukrainian-letter coverage — accepted for Morrowind text (R8.5).
- `file_list::detect_language` (vanilla ESM byte-size lookup) and the lowercase `"pl"` default fallbacks are out of scope — no vanilla release for the added languages, and the defaults are not per-language maps.
- Portuguese is European `pt_PT`, not EET's `pt_BR`; only the dictionary differs, NLLB and codepage are identical.
- The NLLB-600M base model already covers every added language code natively; no model or download-script change (R6.3, R8.8).
- Pure logic is unit-tested in memory; dictionary presence and real-file loading are verified manually per the integration-test rules. Building and running tests is done manually by the user (no-build-or-test rule).
