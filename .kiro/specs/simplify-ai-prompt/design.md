# Design — Simplify the AI Translation Prompt in yTranslator

The system prompt is split into a fixed, code-owned language prefix (auto-substituted from the source/target language, never editable, never stored) and a user-editable instruction body (the only text the Prompt settings box holds and the only part persisted under `[Translation]/SystemPrompt`). The two are joined by a single composition function. The template engine drops the two `_upper` language variables, keeping one language pair. No runtime call graph changes shape — only the meaning of the stored/passed string changes (full prompt → instruction body), and composition moves into `web_translator_config`.

## Why split in web_translator_config, not the settings view

Composition is a translation-domain concern, not a UI concern. Placing `language_prefix()`, `default_instruction()`, and `compose_system_prompt()` in `web_translator_config` (already the owner of `default_system_prompt()`) keeps the settings view thin: it reads/writes only the instruction body, and never assembles the prompt. `set_system_prompt` composes at store time so `send_chat_request` and `expand_template` see a complete prompt exactly as they do today. This preserves the single-string `set_system_prompt` signature and the existing runtime wiring in `apply_provider_settings`.

## Component 1 — web_translator_config prompt parts (yampt.translator)

`web_translator_config.hpp` (namespace `web_translator_config`): replace the single declaration

```cpp
const std::string & default_system_prompt();
```

with three:

```cpp
const std::string & language_prefix();       // fixed technical line, uses {{source_lang}}/{{target_lang}}
const std::string & default_instruction();    // editable default body, no language placeholders
std::string compose_system_prompt(const std::string & instruction);
```

`web_translator_config.cpp`:

```cpp
const std::string & web_translator_config::language_prefix()
{
    static const std::string prefix =
        "Translate the given text from {{source_lang}} to {{target_lang}}.";
    return prefix;
}

const std::string & web_translator_config::default_instruction()
{
    static const std::string instruction =
        "You are a translator for the video game Morrowind. "
        "Output only the translated text, nothing else. "
        "Preserve all HTML tags, line breaks, and formatting exactly as they appear.";
    return instruction;
}

std::string web_translator_config::compose_system_prompt(const std::string & instruction)
{
    const auto & body = instruction.empty() ? default_instruction() : instruction;
    return language_prefix() + "\n" + body;
}
```

Notes:
- The current default blob's meaning is preserved: the language direction (now the fixed prefix, using the single pair instead of `{{source_lang_upper}}`) plus the instruction body. The only substantive text change is `{{source_lang_upper}}` → `{{source_lang}}` in the direction line (R1.3, R2). Exact wording/period/newline are finalized in implementation (Open Decision) but the separator is a single `\n`.
- `default_system_prompt()` is removed. Every caller migrates to either `default_instruction()` (UI placeholder/reset, empty-body semantics) or `compose_system_prompt()` (assembly). Any other reference is updated.

## Component 2 — expand_template: drop the _upper variants (yampt.translator)

`web_translator_t::expand_template` (web_translator.cpp): delete the two `_upper` computations and their `replace_all` calls (the `upper_lang` / `upper_source` block). The method retains, in order:

```cpp
replace_all("{{text}}", text);
replace_all("{{target_lang}}", target_lang);
replace_all("{{source_lang}}", m_source_language);
for (const auto & [setting_key, setting_value] : m_settings)
    replace_all("{{" + setting_key + "}}", setting_value);
```

Consequence: `{{source_lang_upper}}` / `{{target_lang_upper}}` are no longer recognized and are left verbatim if present. That is acceptable because no default prompt, provider config, or doc references them after this change (R2.2). Simple providers keep working (R7.1) — they never used the `_upper` forms in shipped configs.

## Component 3 — set_system_prompt receives the instruction body (yampt.translator)

`web_translator_t::set_system_prompt` currently:

```cpp
void web_translator_t::set_system_prompt(const std::string & prompt)
{
    m_config.system_prompt = prompt.empty() ? web_translator_config::default_system_prompt() : prompt;
}
```

becomes:

```cpp
void web_translator_t::set_system_prompt(const std::string & instruction)
{
    m_config.system_prompt = web_translator_config::compose_system_prompt(instruction);
}
```

The constructor fallback (`if (m_config.system_prompt.empty()) m_config.system_prompt = ...`) changes to compose from an empty instruction:

```cpp
if (m_config.system_prompt.empty())
    m_config.system_prompt = web_translator_config::compose_system_prompt("");
```

So `m_config.system_prompt` is always the composed prefix+body. `send_chat_request` is unchanged: it calls `expand_template(m_config.system_prompt, text, target_lang)` (resolving `{{source_lang}}`/`{{target_lang}}` in the prefix), then appends glossary and examples (R3.2, R7.2). The empty-override → default path is preserved (R3.3): empty instruction composes prefix + `default_instruction()`.

Parameter name changes from `prompt` to `instruction` in both `.hpp` and `.cpp` (naming rule: identical names in declaration/definition).

## Component 4 — Settings storage semantics (yampt.qt)

No code change in `settings_store_t::translation_prompt()` / `set_translation_prompt()` — they remain a scalar read/write of `[Translation]/SystemPrompt`. Only the *meaning* of the stored value changes: it now holds the instruction body, not the full prompt (R4.1). No migration (R4.2): a stored old blob is treated as the instruction body; `compose_system_prompt` prepends the fixed prefix regardless, so a legacy value with a now-unrecognized `{{source_lang_upper}}` still translates (the direction is additionally provided by the fixed prefix). No version check or fallback read is added (legacy-migration steering rule).

## Component 5 — Prompt tab UI (yampt.translator)

`translation_settings_view_t::build_prompt_tab` (translation_settings_view.cpp):

- Description `QLabel`: replace the variable-listing text with a plain explanation, e.g. tr("The instruction sent to AI translation providers. The source and target languages, glossary, and examples are added automatically."). Greyed, word-wrapped, per existing style. No `{{...}}` tokens shown.
- Read-only prefix label: add a greyed, word-wrapped `QLabel` rendering `language_prefix()` (the raw `{{source_lang}} → {{target_lang}}` line is acceptable to show as the fixed context; wording finalized in implementation) so the user sees the fixed part that precedes their instruction, without editing it. Placed above `m_prompt_edit`.
- `m_prompt_edit`: unchanged widget; `setPlaceholderText(QString::fromStdString(web_translator_config::default_instruction()))` instead of the old full default.
- "Reset to Default" handler: `m_prompt_edit->setPlainText(QString::fromStdString(web_translator_config::default_instruction()))`.

`load(const settings_store_t & settings)`: `m_prompt_edit->setPlainText(QString::fromStdString(settings.translation_prompt()))` — unchanged call, now interpreted as the instruction body (R5.2).

`apply(settings_store_t & settings) const`: `settings.set_translation_prompt(m_prompt_edit->toPlainText().toStdString())` — unchanged call, now the instruction body. No composition in the view.

All new strings wrapped for translation (localization rule). The read-only label follows the same styling convention as the description label; padding rules unaffected (R5.3).

## Component 6 — Runtime wiring (yampt.translator)

`translation_suggestion_view_t::apply_provider_settings` is unchanged in shape: it reads `settings.translation_prompt()` into `shared_prompt` and calls `web_provider->set_system_prompt(shared_prompt)` for each `chat_completion` provider (R6.1). `set_source_language(settings.foreign_language())` and the `target_lang` passed to `translate()` (from `native_language()`) continue to feed `expand_template` (R6.2). The only difference is that `shared_prompt` is now an instruction body that `set_system_prompt` composes under the fixed prefix.

## Files

Modified (yampt.translator):
- `translator/web_translator_config.hpp/.cpp` — remove `default_system_prompt()`; add `language_prefix()`, `default_instruction()`, `compose_system_prompt()`.
- `translator/web_translator.cpp` — `expand_template`: remove `_upper` block; `set_system_prompt` + constructor fallback: compose from instruction. `translator/web_translator.hpp` — rename `set_system_prompt` parameter to `instruction`.
- `dialog/settings/translation_settings_view.cpp` — `build_prompt_tab`: new description text, read-only prefix label, `default_instruction()` placeholder + Reset. (`load`/`apply` calls unchanged.)

Unchanged (meaning-only):
- `yampt.qt/settings_store.cpp` — `translation_prompt` / `set_translation_prompt` untouched (now stores the instruction body).
- `view/translation_suggestion_view.cpp` — `apply_provider_settings` untouched.

No new files → no vcxproj/filters changes. If a new test file is added, register it in `yampt.tests.vcxproj` + flat `.filters` (Component: Testing).

Docs/steering:
- `.kiro/steering/web-translator-architecture.md` — Template Variables table drops both `_upper` rows; System Prompt section rewritten for the prefix/instruction split (R8.1).
- `docs/yTranslator-Manual.md` — Prompt/Auto Translation prose updated (R8.2).
- `CHANGELOG.md` — `[XXX]` yTranslator `[CHANGE]`; `README.md`/`docs/README.bbcode` only if they mention the prompt, kept mirrored (R8.3).

## Testing (pure `[u]`, no file I/O)

In `yampt.tests` (register any new file in vcxproj + flat filters):
- `web_translator_config::compose_system_prompt`: empty instruction → `language_prefix()` + "\n" + `default_instruction()`; non-empty → prefix + "\n" + that text; result contains `{{source_lang}}` and `{{target_lang}}`, and neither `{{source_lang_upper}}` nor `{{target_lang_upper}}`.
- `web_translator_config::language_prefix` / `default_instruction`: prefix contains the language pair and no instruction sentence; instruction contains no `{{` language placeholders.
- `web_translator_t::expand_template` (via a test-visible path or a thin wrapper if the method is private — prefer testing through `set_system_prompt` + `config().system_prompt` then a public expand, or expose a minimal seam without changing behavior): `{{source_lang}}`/`{{target_lang}}` substituted; `{{source_lang_upper}}`/`{{target_lang_upper}}` left verbatim; `{{text}}` and a provider setting key expand. If `expand_template` cannot be reached without infrastructure, cover the removal at the config/compose level and assert the absence of `_upper` tokens in the composed prompt.
- `web_translator_t::set_system_prompt`: after `set_system_prompt(instruction)`, `config().system_prompt == compose_system_prompt(instruction)`; empty instruction yields the default composition.

Building and running tests are done manually by the user (project no-build rule); tasks produce tests as artifacts, no "run tests" step.
