# Translation Providers

Each JSON file in this folder defines a web translation provider. The application discovers them on startup — drop a new file here and restart to add a provider.

## Included Providers

### Google Free (`google_free.json`)

Uses Google Translate's unofficial web endpoint. No API key required — leave the API key field empty in settings. No signup, no billing, no project creation.

Limitations: unofficial endpoint with no guarantees. May be rate-limited under heavy use. Best for occasional single-entry translations.

### Google Cloud (`google.json`)

Uses the official Google Cloud Translation API v2. Free tier: 500,000 characters per month.

Setup:
1. Go to https://console.cloud.google.com/
2. Create a project (or select an existing one)
3. Enable "Cloud Translation API" in the API library
4. Go to Credentials → Create Credentials → API Key
5. Copy the API key into yTranslator settings

### DeepL (`deepl.json`)

Uses the DeepL API Free plan. Free tier: 500,000 characters per month.

Setup:
1. Go to https://www.deepl.com/pro-api
2. Sign up for the "DeepL API Free" plan
3. In your account settings, find your Authentication Key
4. Copy the key into yTranslator settings

Note: The free plan uses `api-free.deepl.com`. If you have a Pro plan, duplicate this file and change the endpoint to `api.deepl.com`.

### Claude (`claude.json`)

Uses Anthropic's Claude API for AI-powered translation with context awareness. No free tier — pay per token.

Setup:
1. Go to https://console.anthropic.com/
2. Create an account and add billing
3. Go to API Keys → Create Key
4. Copy the key into yTranslator settings

### ChatGPT (`chatgpt.json`)

Uses OpenAI's GPT-4o-mini model for AI-powered translation. No free tier — pay per token (very cheap for gpt-4o-mini).

Setup:
1. Go to https://platform.openai.com/
2. Create an account and add billing
3. Go to API Keys → Create new secret key
4. Copy the key into yTranslator settings

To use a different model (gpt-4o, gpt-4-turbo, etc.), duplicate the file and change the `"model"` field.

## Creating a Custom Provider

Create a new JSON file in this folder. The schema:

```json
{
    "name": "Display Name",
    "kind": "simple",
    "endpoint": "https://api.example.com/translate",
    "body_format": "json",
    "headers": {
        "Authorization": "Bearer {{api_key}}",
        "Content-Type": "application/json"
    },
    "body": {
        "text": "{{text}}",
        "target": "{{target_lang}}",
        "source": "{{source_lang}}"
    },
    "response_path": "translation",
    "quota_limit": 0
}
```

### Fields

| Field | Description |
|-------|-------------|
| `name` | Display name shown in the provider dropdown |
| `kind` | `"simple"` for direct translation APIs, `"chat_completion"` for LLM APIs with system prompt |
| `endpoint` | API URL (supports template variables) |
| `body_format` | `"json"` (POST with JSON body), `"form"` (POST with URL-encoded body), or `"query"` (GET with URL query parameters) |
| `headers` | HTTP headers as key/value pairs |
| `body` | Request body fields (or query parameters when `body_format` is `"query"`) |
| `response_path` | Dot-separated path to the translated text in the JSON response |
| `quota_limit` | Character limit for status display (0 = unlimited/not tracked) |
| `models_endpoint` | Optional GET URL that returns the provider's available models. When set, a Refresh control appears in the Auto Translate panel to fetch the current model list. Supports the same template variables and `headers` as translation requests, so the API key header applies. Omit for static model lists only. |
| `models_path` | Dot-separated path to the array of model entries in the `models_endpoint` response (e.g. `data` for `{"data": [ ... ]}`). Only used when `models_endpoint` is set. |
| `models_id_key` | The field within each array element that holds the model ID string. Defaults to `"id"` when omitted. Only used when `models_endpoint` is set. |

### Template Variables

Available in `endpoint`, `headers`, `body`, and the system prompt:

| Variable | Value |
|----------|-------|
| `{{api_key}}` | API key from settings |
| `{{text}}` | Source text to translate |
| `{{target_lang}}` | Target language code |
| `{{target_lang_upper}}` | Target language code (uppercase) |
| `{{source_lang}}` | Source language code |
| `{{source_lang_upper}}` | Source language code (uppercase) |

### Response Path

Use dot notation to navigate JSON responses. Array indexing uses `[N]` notation. Separate each level with a dot.

Examples:
- `translations[0].text` — for `{"translations": [{"text": "..."}]}`
- `data.translations[0].translatedText` — for `{"data": {"translations": [{"translatedText": "..."}]}}`
- `[0].[0].[0]` — for `[[["translated text", ...]]]`
- `content[0].text` — for `{"content": [{"text": "..."}]}`

### Chat Completion Kind

For LLM providers (Claude, GPT), set `kind` to `"chat_completion"`. The provider builds the request with:
- A system prompt (set in Settings → Auto Translation → Prompt, shared across all chat providers) expanded with template variables, with glossary terms and examples appended
- A user message containing the source text
- Body fields from `body` merged into the request

The response is extracted using `response_path` as usual.

### Dynamic Model Discovery

Providers that expose a models endpoint can offer an up-to-date model list instead of a static one. Add `models_endpoint`, `models_path`, and (optionally) `models_id_key`:

```json
{
    "models_endpoint": "https://api.openai.com/v1/models",
    "models_path": "data",
    "models_id_key": "id"
}
```

With these fields set, the Auto Translate panel shows a Refresh control. Clicking it sends a GET request to `models_endpoint` using the provider's `headers` (so the API key is applied), reads the array at `models_path`, and collects each element's `models_id_key` value as a selectable model ID. Both OpenAI and Anthropic return `{ "data": [ { "id": "..." } ] }`, so `models_path: data` with the default `models_id_key: id` works for both.

When these fields are absent, the provider falls back to the static model list from its `model` setting's `choices` and no Refresh control is shown.
