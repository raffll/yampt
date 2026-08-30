# Requirements — EET Language Parity (Spanish + Portuguese)

## Background — Current Behavior

yampt's supported languages are defined in `languages.json`, deployed next to the executable. Each entry has five fields consumed across the app:

```json
{ "code": "PL", "name": "Polish", "nllb": "pol_Latn", "dictionary": "pl_PL", "codepage": 1250 }
```

- `code` — identity key used everywhere.
- `name` — display name in combo boxes.
- `nllb` — CTranslate2 NLLB translation-target code (e.g. `pol_Latn`).
- `dictionary` — Hunspell prefix / locale stem (e.g. `pl_PL`) → `dictionaries/<prefix>.aff` + `.dic`.
- `codepage` — Windows codepage for ESM/ESP byte encoding (1250 / 1251 / 1252).

Current entries: EN (eng_Latn / en_US / 1252), PL (pol_Latn / pl_PL / 1250), DE (deu_Latn / de_DE / 1252), FR (fra_Latn / fr_FR / 1252), RU (rus_Cyrl / ru_RU / 1251), IT (ita_Latn / it_IT / 1252), HU (hun_Latn / hu_HU / 1250).

The language set is data-driven from `languages.json` for: the first-run dialog, the yTranslator Language settings page combos, the Translation settings page, NLLB target selection, per-language codepage resolution, and spellcheck-prefix resolution — via `language_config::load` and the helpers `find_by_code` / `resolve_codepage` / `resolve_dictionary_prefix` (yampt.core/source/utility/language_config.{hpp,cpp}).

One production spot bypasses `languages.json`: `sidebar_controller_t::resolve_hunspell_locale` (yampt.translator/source/session/sidebar_controller.cpp) hardcodes a `std::map<std::string, std::string>` of `PL/DE/FR/RU/IT/HU` → locale. Any new language must be added there too, or that path returns an empty locale and localization-file generation silently loses spellcheck.

The codepage machinery (`codepage_t` enum, `supported_codepages` / `codepage_to_index` / `index_to_codepage`, `codepage_name`, Linux `codepage_iconv_name`, yEditor's codepage combo) supports exactly Windows-1250 / 1251 / 1252.

## What "Same Languages as EET" Means

EET (ESP-ESM Translator) ships Hunspell dictionaries for its translation targets: `de_DE, en_US, es_ES, fr_FR, it_IT, pt_BR, ru_RU` (its `Dictionaries/` folder). Its UI additionally offers Hungarian and Japanese as interface languages, but the translation-target set relevant to Morrowind is the seven dictionary languages above.

Comparing to yampt's current set:
- Already present in yampt: English, German, French, Italian, Russian (and yampt adds Polish + Hungarian, which EET does not have as dictionaries).
- **Missing in yampt: Spanish and Portuguese.**

So reaching EET parity for Morrowind means adding **Spanish** and **Portuguese**. Both use Windows-1252, which yampt already supports — no new codepage is required.

## Decisions

- **Portuguese variant → `pt_PT`** (European Portuguese), not EET's `pt_BR`. The NLLB code (`por_Latn`) and codepage (1252) are identical either way; only the Hunspell dictionary differs. `pt_PT` UTF-8 dictionaries are available from `wooorm/dictionaries`.
- **Dictionary source → `wooorm/dictionaries` (UTF-8)**, not EET's copies. EET's `es_ES`/`pt_BR` dictionaries are ISO-8859-1 (`SET ISO8859-1`); the project's `supported-languages` rule prefers UTF-8 Hunspell dictionaries.
- **Refactor `resolve_hunspell_locale`** to be data-driven from `languages.json` (via `language_config::resolve_dictionary_prefix`) instead of extending the hardcoded map. This removes the last hardcoded language list on this path so future languages need no C++ change.

## Goal

Add Spanish and Portuguese (pt_PT) to yampt's supported languages so the app reaches EET parity for Morrowind translation, driven by data (`languages.json` + dictionaries) with no changes to the codepage machinery, and eliminate the hardcoded locale map that would otherwise need manual updates.

## User-Facing Outcomes

- Spanish and Portuguese appear in every yTranslator language selector (first-run dialog, Language settings foreign/native combos, Translation settings) exactly like the existing languages, because those lists are built from `languages.json`.
- Selecting Spanish or Portuguese as the native language sets the NLLB translation target (`spa_Latn` / `por_Latn`), the codepage (1252), and the spellcheck dictionary (`es_ES` / `pt_PT`) through the same data-driven paths the existing languages use.
- Spell checking and localization-file generation work for the two new languages (their dictionaries are present and resolved from `languages.json`).
- No existing language regresses; the 6 target languages in the steering rule remain, plus the two new ones.

## Requirements

### R1 — Add Spanish and Portuguese to languages.json

1.1 Add two entries to `languages.json`:
   - Spanish: `code` `ES`, `name` `Spanish`, `nllb` `spa_Latn`, `dictionary` `es_ES`, `codepage` `1252`.
   - Portuguese: `code` `PT`, `name` `Portuguese`, `nllb` `por_Latn`, `dictionary` `pt_PT`, `codepage` `1252`.
1.2 The entries follow the exact field shape of existing entries; no schema change.
1.3 All deployed copies of `languages.json` that ship with the app are updated consistently (the repo-root source of truth; build/output copies are produced by the build, not hand-edited).

### R2 — Provide UTF-8 Hunspell dictionaries

2.1 `dictionaries/es_ES.aff` + `dictionaries/es_ES.dic` and `dictionaries/pt_PT.aff` + `dictionaries/pt_PT.dic` are added, UTF-8 encoded, sourced from `wooorm/dictionaries` (or an equivalent UTF-8 Hunspell source).
2.2 The dictionary prefixes match the `dictionary` field in `languages.json` exactly (`es_ES`, `pt_PT`), so `dictionaries/<prefix>.aff/.dic` resolves.
2.3 The new dictionaries are included in the release package alongside the existing ones (the packaging already copies the `dictionaries/` folder; no packaging-list change needed beyond the files existing).
2.4 Licensing: the source dictionaries' license permits redistribution; the license notice is preserved as the existing dictionaries do (`dictionaries/LICENSE`).

### R3 — Make hunspell-locale resolution data-driven

3.1 `sidebar_controller_t::resolve_hunspell_locale` no longer uses a hardcoded language→locale map. It resolves the Hunspell prefix from `languages.json` via `language_config::resolve_dictionary_prefix` (loading the languages through the same path other yTranslator code uses, e.g. `resource_paths::languages_file()`).
3.2 The refactor preserves existing behavior for all current languages (PL/DE/FR/RU/IT/HU return their existing locale strings) and additionally resolves ES/PT, plus any future `languages.json` entry, with no further code change.
3.3 If a code is not found in `languages.json`, the function returns an empty string, matching the current "unknown language → empty" contract so callers behave identically for unknown codes.

### R4 — No codepage machinery change

4.1 No changes to `codepage_t`, `supported_codepages`, `codepage_to_index`, `index_to_codepage`, `codepage_name`, the Linux `codepage_iconv_name`, or yEditor's codepage combo. Both new languages are 1252, already supported.
4.2 Selecting ES/PT as native language derives encoding index 1252 through the existing `codepage_to_index(entry.codepage)` path, unchanged.

### R5 — Documentation and steering

5.1 The `supported-languages` steering rule's language table adds Spanish (ES, es_ES, nllb via NLLB-600M) and Portuguese (PT, pt_PT), and the NLLB language-code list adds `eng_Latn → spa_Latn` and `eng_Latn → por_Latn`. The codepage section is unchanged (both map to Windows-1252, already listed under `win1252`).
5.2 User-facing docs that enumerate supported languages (manuals / README as applicable) list the two additions. Per the manual-style and changelog rules, only user-visible language support is described; no dictionary-source or refactor detail appears in user docs.
5.3 `CHANGELOG.md` gets a `[NEW]` entry (Both Apps or yTranslator as appropriate) for added Spanish and Portuguese language support. No dictionary-source, build, or refactor internals in the changelog.

### R6 — No regression

6.1 All existing languages continue to resolve their NLLB code, codepage, and dictionary exactly as before.
6.2 The first-run dialog, Language settings combos, and Translation settings list now include ES/PT with no other behavioral change, because they iterate `languages.json`.
6.3 The NLLB-600M model already covers `spa_Latn` and `por_Latn`; no model change or download-script change is required for base translation.

### R7 — Verification

7.1 Data-driven behavior is verifiable: after adding the entries, `language_config::find_by_code` returns ES/PT with the correct `nllb_code`, `dictionary_prefix`, and `codepage`; `resolve_codepage("ES")` / `resolve_codepage("PT")` return Windows-1252; `resolve_dictionary_prefix("ES")` / `("PT")` return `es_ES` / `pt_PT`.
7.2 The refactored `resolve_hunspell_locale` is verifiable against `languages.json`: it returns the existing locale for each current code and `es_ES` / `pt_PT` for the new codes, and empty for an unknown code.
7.3 Dictionary presence is verifiable: `dictionaries/es_ES.aff/.dic` and `dictionaries/pt_PT.aff/.dic` exist and load in the spell checker.
7.4 Pure logic covered by `[u]` unit tests without file I/O where possible (e.g. `resolve_dictionary_prefix` / `resolve_codepage` given an in-memory `languages` vector). Tests that need the real `languages.json` or dictionary files on disk are integration-level, not `[u]`.

## Open Decisions

Resolved:
- Which languages → Spanish + Portuguese (the only Morrowind-relevant EET languages yampt lacks). (R1)
- Portuguese variant → `pt_PT` (European), not EET's `pt_BR`. NLLB/codepage identical; only the dictionary differs. (Decisions)
- Dictionary source → `wooorm/dictionaries` UTF-8, not EET's ISO-8859-1 copies. (R2)
- `resolve_hunspell_locale` → refactor to data-driven via `resolve_dictionary_prefix`, not extend the hardcoded map. (R3)
- Codepage → no machinery change; both are 1252. (R4)

To confirm during design:
- Whether any other hardcoded language reference (`file_list::detect_language` file-size heuristic, `session`/`sidebar_model` `"pl"` default) needs touching. `detect_language` matches vanilla Morrowind/Tribunal/Bloodmoon file sizes by language and does not affect ES/PT (no official Spanish/Portuguese vanilla release), so it is expected to be out of scope; the design will confirm.
