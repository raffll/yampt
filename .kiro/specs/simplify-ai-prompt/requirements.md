# Requirements — Simplify the AI Translation Prompt in yTranslator

## Background — Current Behavior

yTranslator sends a single shared system prompt to every `chat_completion` web provider. The mechanics:

- The canonical default lives in code: `web_translator_config::default_system_prompt()` (`yampt.translator/source/translator/web_translator_config.cpp`). Its text is one blob:
  > "You are a translator for the video game Morrowind. Translate the given text from `{{source_lang_upper}}` to `{{target_lang}}`. Output only the translated text, nothing else. Preserve all HTML tags, line breaks, and formatting exactly as they appear."
- The user override is stored globally under `[Translation]/SystemPrompt` (`settings_store_t::translation_prompt()` / `set_translation_prompt()` in `yampt.qt/source/settings_store.cpp`). An empty override falls back to `default_system_prompt()`.
- Template variables are expanded by `web_translator_t::expand_template(tmpl, text, target_lang)` (`web_translator.cpp`). It supports: `{{text}}`, `{{target_lang}}`, `{{source_lang}}`, `{{target_lang_upper}}`, `{{source_lang_upper}}`, and `{{<setting_key>}}` for every provider setting. The `_upper` variants are computed inline by uppercasing the language codes.
- Source language = `m_source_language` (set via `set_source_language()` from `settings.foreign_language()`); target language = the `target_lang` argument of `translate()` (from `settings.native_language()`). Both already flow automatically; the user never types a language into the prompt.
- At runtime, `translation_suggestion_view_t::apply_provider_settings` reads `settings.translation_prompt()` and calls `web_translator_t::set_system_prompt(shared_prompt)` on every `chat_completion` provider. `set_system_prompt` stores `prompt.empty() ? default_system_prompt() : prompt`. In `send_chat_request`, the stored prompt is passed through `expand_template`, then the glossary and examples are appended automatically.
- The Prompt settings tab (Settings → Auto Translation → Prompt, in `translation_settings_view_t::build_prompt_tab`) shows a `QPlainTextEdit` (`m_prompt_edit`) holding the ENTIRE prompt (technical language line + instruction), a description `QLabel` that lists all four template variables, and a "Reset to Default" button that restores the full `default_system_prompt()`.
- Steering `.kiro/steering/web-translator-architecture.md` documents the four template variables (including both `_upper` variants) and the single-shared-prompt System Prompt model.

## Problem

The prompt box mixes two concerns:

1. A fixed, technical instruction that names the source and target language — which the user should never have to write, must not break, and is already substituted automatically.
2. The actual translation instruction (tone, domain, formatting rules) — the only part a user meaningfully wants to edit.

Exposing the language template variables (and two redundant `_upper` forms of them) invites the user to break the substitution (delete `{{target_lang}}`, mistype `{{source_lang_upper}}`, etc.), and clutters the UI with technical placeholders that are not the user's concern. There are four language variables where one pair suffices.

## Goal

Split the system prompt into two parts:

- A **fixed language prefix** — auto-substituted with the source and target language, NOT shown as editable, NOT exposed as template variables the user must maintain.
- A **user-editable instruction body** — the only text the Prompt settings box holds, pre-filled with a default instruction.

Reduce the language template variables to a **single pair** (`{{source_lang}}`, `{{target_lang}}`), dropping the `{{source_lang_upper}}` / `{{target_lang_upper}}` variants everywhere. The language substitution stays automatic and internal; the user edits only plain instruction text.

## User-Facing Outcomes

- The Prompt tab shows one multi-line box containing only the editable instruction text (e.g. "You are a translator for the video game Morrowind. Output only the translated text, nothing else. Preserve all HTML tags, line breaks, and formatting exactly as they appear."), pre-filled with the default instruction.
- The user no longer sees or edits the "Translate from X to Y" line or any `{{...}}` language placeholders. The source and target languages are inserted automatically from the language settings when the prompt is built.
- "Reset to Default" restores the default instruction text (not a blob containing template variables).
- Translations behave the same as before: the model still receives the language direction plus the user's instruction, glossary, and examples.

## Requirements

### R1 — Split default prompt into fixed prefix + editable instruction

1.1 Replace `web_translator_config::default_system_prompt()` (single blob) with two definitions in `web_translator_config`:
- `language_prefix()` — the fixed, non-editable technical line that states the translation direction using the single language-variable pair, e.g. `"Translate the given text from {{source_lang}} to {{target_lang}}."`.
- `default_instruction()` — the editable default instruction body (the Morrowind translator guidance, output-only rule, tag/formatting preservation), with NO language template variables.

1.2 Add `web_translator_config::compose_system_prompt(const std::string & instruction)` that returns the assembled prompt: `language_prefix()` + a separator (newline) + `(instruction.empty() ? default_instruction() : instruction)`. This is the single place the two parts are joined.

1.3 `language_prefix()` uses only `{{source_lang}}` and `{{target_lang}}` — no `_upper` variants.

### R2 — Reduce template variables to one language pair

2.1 `web_translator_t::expand_template` no longer computes or substitutes `{{source_lang_upper}}` or `{{target_lang_upper}}`. Only `{{text}}`, `{{target_lang}}`, `{{source_lang}}`, and the per-setting-key substitutions remain.

2.2 No provider JSON config, default prompt, or documentation references the `_upper` variants after this change. (Provider configs must still never hardcode source/target language — the single-pair variables remain available in headers/body per the web-translator-architecture rule.)

### R3 — set_system_prompt receives the instruction body, stores the composed prompt

3.1 The stored prompt contract changes: `set_system_prompt` (and the `web_translator_t` constructor fallback) receive the **instruction body**, and store `compose_system_prompt(instruction)` into `m_config.system_prompt`. An empty instruction yields `compose_system_prompt("")` = prefix + `default_instruction()`.

3.2 `send_chat_request` continues to pass `m_config.system_prompt` through `expand_template`, so `{{source_lang}}` / `{{target_lang}}` in the fixed prefix are resolved at send time exactly as today. Glossary and examples are still appended automatically after expansion.

3.3 The empty-override → default behavior is preserved end to end: an empty stored instruction produces the default instruction; a non-empty stored instruction is used verbatim as the body under the fixed prefix.

### R4 — Settings storage holds only the instruction body

4.1 `[Translation]/SystemPrompt` (`settings.translation_prompt()` / `set_translation_prompt()`) now stores only the user-editable instruction body, not the full composed prompt. The fixed prefix is never stored (it is code-owned and composed at build time).

4.2 Pre-1.0, no migration (per the legacy-migration steering rule). A previously stored full-blob value is treated as the instruction body; the fixed prefix is prepended regardless, so behavior remains sensible without a migration path. No version checks or fallback reads for the old format are added.

### R5 — Prompt tab shows only the editable instruction

5.1 `translation_settings_view_t::build_prompt_tab`:
- `m_prompt_edit` binds to the instruction body only. Its placeholder text is `default_instruction()`.
- The description `QLabel` no longer lists template variables. It explains that the source and target languages are inserted automatically and that the glossary and examples are appended automatically. (Localized per the localization rule.)
- The fixed language line is shown to the user as read-only, non-editable context (a greyed, word-wrapped label rendered from `language_prefix()`), so the user understands what precedes their instruction, without being able to edit it or its placeholders.
- "Reset to Default" restores `default_instruction()` into `m_prompt_edit`.

5.2 `load` populates `m_prompt_edit` from `settings.translation_prompt()` (the instruction body). `apply` writes `m_prompt_edit->toPlainText()` back via `set_translation_prompt` (the instruction body). No composition happens in the settings view — composition is owned by `web_translator_config`/`web_translator_t`.

5.3 Panel/tab padding and compact-table rules are unaffected; the read-only prefix label follows the same styling convention as existing greyed description labels.

### R6 — Runtime wiring unchanged in shape

6.1 `translation_suggestion_view_t::apply_provider_settings` continues to read `settings.translation_prompt()` and call `set_system_prompt(...)` per `chat_completion` provider. The only change is the meaning of the passed string (instruction body, not full prompt) — no new call is added, no second parameter.

6.2 Source language (`set_source_language` from `foreign_language()`) and target language (`native_language()` → `translate(text, target)`) continue to flow automatically into `expand_template`; no user-entered language values.

### R7 — No regression

7.1 Simple (non-chat) providers are unaffected: they do not use the system prompt; their headers/body still use `{{text}}`, `{{target_lang}}`, `{{source_lang}}`, and setting-key variables. Only the two `_upper` variables are removed from the engine.

7.2 Glossary and example appending in `send_chat_request` is unchanged.

7.3 The `web_translator_t` constructor fallback (empty stored prompt → default) still yields a working prompt (prefix + default instruction).

7.4 `anthropic` vs `openai` message styling in `send_chat_request` is unchanged.

### R8 — Documentation and steering

8.1 Update `.kiro/steering/web-translator-architecture.md`:
- Template Variables table: remove `{{source_lang_upper}}` and `{{target_lang_upper}}`; keep `{{source_lang}}` and `{{target_lang}}`.
- System Prompt section: describe the split — a fixed, code-owned language prefix (auto-substituted, not user-editable, not stored) plus a user-editable instruction body stored under `[Translation]/SystemPrompt`; `default_instruction()` is the canonical editable default and `language_prefix()` the fixed part; `compose_system_prompt` joins them.

8.2 Update `docs/yTranslator-Manual.md` (Prompt tab / Auto Translation section) to describe editing only the instruction, with the language direction inserted automatically. Prose, no internals (per manual-style).

8.3 `CHANGELOG.md` unreleased `[XXX]`, yTranslator section, `[CHANGE]` (user sees a different Prompt tab and edits only the instruction; behavior of the setting changed). `README.md` / `docs/README.bbcode` only if the feature summary references the prompt; keep the two mirrored if touched.

### R9 — Verification

9.1 Pure `[u]` unit tests (in-memory, no file I/O):
- `web_translator_config::compose_system_prompt`: empty instruction → prefix + `default_instruction()`; non-empty instruction → prefix + that instruction; prefix contains `{{source_lang}}` and `{{target_lang}}` and neither `_upper` token.
- `web_translator_config::language_prefix` / `default_instruction`: prefix has the language pair and no instruction text; instruction has no `{{` language placeholders.
- `web_translator_t::expand_template`: `{{source_lang}}` and `{{target_lang}}` are substituted from source/target; `{{source_lang_upper}}` / `{{target_lang_upper}}` are left untouched (no longer recognized); `{{text}}` and a provider setting key still expand.
- `web_translator_t::set_system_prompt`: passing an instruction body results in `config().system_prompt` equal to `compose_system_prompt(instruction)`; empty body yields the default composition.

9.2 Building and running tests are done manually by the user (project no-build rule); tasks produce tests as artifacts, no "run tests" step.

## Open Decisions

Resolved:
- Keep the single pair `{{source_lang}}` / `{{target_lang}}`; drop both `_upper` variants everywhere (R2).
- The fixed prefix is code-owned and never stored; only the instruction body is persisted under `[Translation]/SystemPrompt` (R4).
- Composition (`prefix + instruction`) lives in `web_translator_config::compose_system_prompt`, invoked from `set_system_prompt` — not in the settings view (R3, R5.2).
- `set_system_prompt` keeps a single-string signature; the passed string's meaning becomes "instruction body" (R3.1, R6.1).
- No migration for old full-blob stored values; the blob becomes the instruction body under the prefix (R4.2).
- The Prompt tab shows the fixed prefix as a read-only greyed label above the editable box (R5.1).

Deferred to design:
- Exact separator between prefix and instruction (single `\n` vs blank line) and whether the prefix ends with a period/newline.
- Exact wording of `default_instruction()` and `language_prefix()` (must preserve the current default's meaning minus the language line).
- Exact wording of the Prompt tab description label and the read-only prefix label.
