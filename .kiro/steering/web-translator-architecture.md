# Web Translator Architecture

## Provider Config Files

Translation providers are defined as JSON files in `providers/` next to the executable. Each file describes one provider. The application discovers them on startup — no code changes needed to add a provider.

Config schema:
- `name` — display name in the UI
- `kind` — `"simple"` (direct translation API) or `"chat_completion"` (LLM with system prompt + messages)
- `endpoint` — API URL
- `method` — HTTP method (default: POST)
- `body_format` — `"json"` or `"form"` (URL-encoded)
- `headers` — key/value map, supports template variables
- `body` — key/value map of request body fields, supports template variables
- `response_path` — dot-separated JSON path with array indexing (e.g. `translations[0].text`, `content[0].text`)
- `system_prompt` — only used for `chat_completion` kind
- `quota_limit` — optional character limit (0 = unlimited)

## Template Variables

Available in `headers`, `body`, and `system_prompt` fields:

| Variable | Expands to |
|----------|-----------|
| `{{api_key}}` | User's API key from settings |
| `{{text}}` | Source text to translate |
| `{{target_lang}}` | Target language code (as-is from settings) |
| `{{target_lang_upper}}` | Target language code uppercased |
| `{{source_lang}}` | Source language from settings (foreign_language) |
| `{{source_lang_upper}}` | Source language uppercased |

## Source Language

The source language is read from `settings.foreign_language()` and set on each `web_translator_t` instance via `set_source_language()`. It is NOT hardcoded in provider configs — configs use `{{source_lang}}` placeholders.

## Settings Storage

API keys are stored per provider identifier under `[WebTranslators]` in the INI file:
- `settings.web_api_key("deepl")` reads `[WebTranslators]/deepl`
- `settings.set_web_api_key("claude", "sk-ant-...")` writes `[WebTranslators]/claude`

The identifier is the JSON filename stem (e.g. `deepl.json` → identifier `deepl`).

Legacy per-provider methods (`deepl_api_key()`, `google_api_key()`, `claude_api_key()`) still exist for backward compatibility but new code uses `web_api_key(id)`.

## Translation Settings Page

The Translation page in Settings shows a table auto-populated from discovered provider configs:
- Column 0: Provider name (read-only)
- Column 1: API key (password field)
- Column 2: Status (Configured / Not configured)

## Provider Combo Box

The Auto Translate tab's combo box shows:
- Index 0: CTranslate2 (always present, local model)
- Index 1+: Web providers discovered from `providers/` directory

## Status Name

The status assigned after auto-translation is `model` internally, displayed as **"Generated"** in the UI.

## Tab Name

The translation tab is labeled **"Auto Translate"**.

## Files

| File | Purpose |
|------|---------|
| `translator/web_translator_config.hpp/.cpp` | Config struct + JSON loader |
| `translator/web_translator.hpp/.cpp` | Generic HTTP translator (QObject with signals) |
| `translator/ctranslate2_translator.hpp/.cpp` | Local CTranslate2 model wrapper (unchanged) |
| `translator/translator.hpp` | Pure virtual interface (unchanged) |
| `providers/*.json` | Provider config files (deployed next to exe) |

## Rules

- Never hardcode source or target language in provider JSON configs — always use template variables.
- Never add a new translator class for a web API — create a JSON config file instead.
- The `web_translator_t` class handles both simple (DeepL, Google) and chat_completion (Claude) providers via the `kind` field.
- Provider configs are NOT in `yampt.translator/` project folder — they live in solution root `providers/` and are copied to the output directory by MSBuild.
- The `CopyProviders` MSBuild target copies `providers/*.json` to `$(OutDir)providers/` on build.
