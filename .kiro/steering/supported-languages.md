# Supported Languages

yampt must support all languages that were available in the legacy yampt-0.21 release (`c:\OMEN\Morrowind\Tools\yampt-0.21\`):

| Code | Language | Spellcheck dict | Translation model |
|------|----------|-----------------|-------------------|
| PL | Polish | pl_PL (SJP) | nllb-600M |
| DE | German | de_DE | nllb-600M |
| FR | French | fr_FR | nllb-600M |
| RU | Russian | ru_RU | nllb-600M |
| IT | Italian | it_IT | nllb-600M |
| HU | Hungarian | hu_HU | nllb-600M |

## Spellcheck Dictionaries

All spellcheck dictionaries live in `external/dictionaries/` and are copied to the release package. Source for missing dictionaries: `c:\OMEN\Morrowind\Tools\ESP-ESM Translator - Application only Date up-921-4-35-1734639838\Dictionaries\` or `wooorm/dictionaries` on GitHub.

Required files per language: `{locale}.aff` + `{locale}.dic` (Hunspell format, UTF-8 encoding preferred).

The `en_US` dictionary is always included (used for partial-mode English word detection in make-base).

## Translation Engine

The NLLB-600M model supports all 6 languages natively. Language codes for CTranslate2:
- `eng_Latn` → `pol_Latn` (EN→PL)
- `eng_Latn` → `deu_Latn` (EN→DE)
- `eng_Latn` → `fra_Latn` (EN→FR)
- `eng_Latn` → `rus_Cyrl` (EN→RU)
- `eng_Latn` → `ita_Latn` (EN→IT)
- `eng_Latn` → `hun_Latn` (EN→HU)

## Codepages (from OpenMW `components/toutf8/`)

OpenMW supports exactly 3 codepages. All 6 yampt languages map to one of these:

| Codepage | OpenMW name | Languages |
|----------|-------------|-----------|
| Windows-1250 | `win1250` | PL, HU (Central/Eastern European) |
| Windows-1251 | `win1251` | RU (Cyrillic) |
| Windows-1252 | `win1252` | DE, FR, IT (Western European, also EN) |

yampt must support all 3 codepages for reading/writing ESM/ESP files. The codepage determines how raw bytes in plugin files are decoded to Unicode and re-encoded on save.

## Rules

- Never drop support for any of these 6 languages.
- When adding a new language-dependent feature (spell check, translation suggestions, codepage detection), ensure it works for all 6.
- The GUI language selector must offer all 6 target languages.
- Fine-tuning data is currently PL-only, but the base NLLB model handles all 6 without fine-tuning.
- All 3 codepages (1250, 1251, 1252) must be supported for ESM/ESP reading and writing.
