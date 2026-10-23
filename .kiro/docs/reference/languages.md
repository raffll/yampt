# Supported Languages

yampt's supported languages are defined in `languages.json` (deployed next to the executable). Adding a new language requires only editing this file — no recompilation needed.

Current languages:

| Code | Language | Spellcheck dict | Translation model |
|------|----------|-----------------|-------------------|
| PL | Polish | pl_PL (SJP) | nllb-600M |
| DE | German | de_DE | nllb-600M |
| FR | French | fr_FR | nllb-600M |
| RU | Russian | ru_RU | nllb-600M |
| IT | Italian | it_IT | nllb-600M |
| HU | Hungarian | hu_HU | nllb-600M |
| ES | Spanish | es_ES | nllb-600M |
| PT | Portuguese | pt_PT | nllb-600M |
| CS | Czech | cs_CZ | nllb-600M |
| SK | Slovak | sk_SK | nllb-600M |
| SL | Slovenian | sl_SI | nllb-600M |
| HR | Croatian | hr_HR | nllb-600M |
| RO | Romanian | ro_RO | nllb-600M |
| UK | Ukrainian | uk_UA | nllb-600M |
| BG | Bulgarian | bg_BG | nllb-600M |
| SR | Serbian | sr_RS | nllb-600M |
| NL | Dutch | nl_NL | nllb-600M |
| SV | Swedish | sv_SE | nllb-600M |
| DA | Danish | da_DK | nllb-600M |
| NB | Norwegian Bokmal | nb_NO | nllb-600M |
| FI | Finnish | (none — translation-only) | nllb-600M |
| CA | Catalan | ca_ES | nllb-600M |
| GL | Galician | gl_ES | nllb-600M |

Hunspell Spell-Check Dictionaries
Source: https://github.com/wooorm/dictionaries (UTF-8 normalized versions)

These dictionary files are NOT stored in this repository. They are provided by
the wooorm/dictionaries git submodule at external/dictionaries (pinned commit
8cfea40) and copied into the application's dictionaries/ folder at build time,
renaming each upstream index.aff / index.dic to the locale prefix used by yampt.

Each dictionary retains its original license from the upstream project.

## Translation Engine

The NLLB-600M model supports all languages natively. Language codes for CTranslate2:
- `eng_Latn` → `pol_Latn` (EN→PL)
- `eng_Latn` → `deu_Latn` (EN→DE)
- `eng_Latn` → `fra_Latn` (EN→FR)
- `eng_Latn` → `rus_Cyrl` (EN→RU)
- `eng_Latn` → `ita_Latn` (EN→IT)
- `eng_Latn` → `hun_Latn` (EN→HU)
- `eng_Latn` → `spa_Latn` (EN→ES)
- `eng_Latn` → `por_Latn` (EN→PT)
- `eng_Latn` → `ces_Latn` (EN→CS)
- `eng_Latn` → `slk_Latn` (EN→SK)
- `eng_Latn` → `slv_Latn` (EN→SL)
- `eng_Latn` → `hrv_Latn` (EN→HR)
- `eng_Latn` → `ron_Latn` (EN→RO)
- `eng_Latn` → `ukr_Cyrl` (EN→UK)
- `eng_Latn` → `bul_Cyrl` (EN→BG)
- `eng_Latn` → `srp_Cyrl` (EN→SR)
- `eng_Latn` → `nld_Latn` (EN→NL)
- `eng_Latn` → `swe_Latn` (EN→SV)
- `eng_Latn` → `dan_Latn` (EN→DA)
- `eng_Latn` → `nob_Latn` (EN→NB)
- `eng_Latn` → `fin_Latn` (EN→FI, translation-only — no Hunspell dictionary shipped)
- `eng_Latn` → `cat_Latn` (EN→CA)
- `eng_Latn` → `glg_Latn` (EN→GL)

## Codepages (from OpenMW `components/toutf8/`)

OpenMW supports exactly 3 codepages. All yampt languages map to one of these:

| Codepage | OpenMW name | Languages |
|----------|-------------|-----------|
| Windows-1250 | `win1250` | PL, HU, CS, SK, SL, HR, RO (Central/Eastern European) |
| Windows-1251 | `win1251` | RU, UK, BG, SR (Cyrillic) |
| Windows-1252 | `win1252` | DE, FR, IT, ES, PT, NL, SV, DA, NB, FI, CA, GL (Western European, also EN) |

yampt must support all 3 codepages for reading/writing ESM/ESP files. The codepage determines how raw bytes in plugin files are decoded to Unicode and re-encoded on save.

## Rules

- Never drop support for any of these languages.
- When adding a new language-dependent feature (spell check, translation suggestions, codepage detection), ensure it works for all of them.
- The GUI language selector must offer all target languages.
- Fine-tuning data is currently PL-only, but the base NLLB model handles all of them without fine-tuning.
- All 3 codepages (1250, 1251, 1252) must be supported for ESM/ESP reading and writing.
- The Translate button only works on entries with status `untranslated`. For all other statuses, it shows an info message and does nothing. Do NOT remove or bypass this restriction.
