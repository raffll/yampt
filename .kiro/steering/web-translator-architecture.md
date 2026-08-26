# Web Translator Architecture

## Provider Config Files

Translation providers are defined as JSON files in `providers/` next to the executable. Each file describes one provider. The application discovers them on startup — no code changes needed to add a provider.

Config schema:
- `name` — display name in the UI
- `kind` — `"simple"` (direct translation API) or `"chat_completion"` (LLM with system prompt + messages)
- `message_style` — only for `chat_completion` kind: `"openai"` (default, system role in messages array) or `"anthropic"` (top-level system field, user-only messages)
- `endpoint` — API URL
- `body_format` — `"json"`, `"form"` (URL-encoded), or `"query"` (URL query parameters, uses GET)
- `headers` — key/value map, supports template variables
- `body` — key/value map of request body fields, supports template variables
- `response_path` — dot-separated JSON path with array indexing (e.g. `translations[0].text`, `content[0].text`)
- `system_prompt` — only used for `chat_completion` kind
- `quota_limit` — optional character limit (0 = unlimited)
- `settings` — array of provider-specific settings (see below)

### Settings Array

Each provider can define user-configurable settings via the `settings` array. Each entry:
- `key` — internal identifier, used as INI key and template variable name `{{key}}`
- `label` — display name shown in the settings UI
- `type` — `"text"`, `"password"`, or `"choice"`
- `choices` — array of strings (only for `type: "choice"`)
- `default` — default value if user hasn't configured one
- `required` — boolean, whether the provider needs this to function (default: true)

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
| `{{<setting_key>}}` | Value of any provider setting by its `key` (e.g. `{{model}}`) |

## Source Language

The source language is read from `settings.foreign_language()` and set on each `web_translator_t` instance via `set_source_language()`. It is NOT hardcoded in provider configs — configs use `{{source_lang}}` placeholders.

## Settings Storage

Provider settings are stored per provider identifier and setting key under `[WebProviders]` in the INI file:
- `settings.web_provider_setting("claude", "api_key")` reads `[WebProviders]/claude/api_key`
- `settings.set_web_provider_setting("claude", "model", "claude-sonnet-4-20250514")` writes `[WebProviders]/claude/model`

The identifier is the JSON filename stem (e.g. `deepl.json` → identifier `deepl`).

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
