# Tasks — Simplify the AI Translation Prompt in yTranslator

Order: prompt parts in `web_translator_config` first (with unit tests), then the template-engine `_upper` removal and `set_system_prompt` composition, then the Prompt tab UI, then docs/steering. yTranslator + yampt.qt only; no core (yampt.core) changes.

## 1. Prompt parts in web_translator_config

- [ ] 1.1 In `web_translator_config.hpp`, remove `default_system_prompt()` and add `language_prefix()`, `default_instruction()`, and `compose_system_prompt(const std::string & instruction)`. (R1.1, R1.2)
- [ ] 1.2 In `web_translator_config.cpp`, implement the three: `language_prefix()` = fixed direction line using `{{source_lang}}`/`{{target_lang}}` (no `_upper`); `default_instruction()` = the editable default body (Morrowind guidance, output-only, tag/formatting preservation, no language placeholders); `compose_system_prompt` = `language_prefix() + "\n" + (instruction.empty() ? default_instruction() : instruction)`. (R1.1–R1.3)
- [ ] 1.3 Unit tests for `compose_system_prompt` / `language_prefix` / `default_instruction` (register any new file in `yampt.tests.vcxproj` + flat `.filters`): empty vs non-empty instruction composition; prefix has the language pair and neither `_upper` token; instruction has no `{{` language placeholders. (R9.1)

## 2. Template engine: drop _upper variants

- [ ] 2.1 In `web_translator_t::expand_template` (web_translator.cpp), remove the `{{source_lang_upper}}` / `{{target_lang_upper}}` computations and their `replace_all` calls. Keep `{{text}}`, `{{target_lang}}`, `{{source_lang}}`, and the per-setting-key loop. (R2.1)
- [ ] 2.2 Verify no remaining reference to `_upper` variables in provider JSON configs (`providers/*.json`), default prompt text, or code; none expected. (R2.2)

## 3. set_system_prompt composes from instruction body

- [ ] 3.1 Change `web_translator_t::set_system_prompt` to `m_config.system_prompt = web_translator_config::compose_system_prompt(instruction);` and rename the parameter to `instruction` in both `.hpp` and `.cpp`. (R3.1, naming rule)
- [ ] 3.2 Change the constructor fallback so an empty `m_config.system_prompt` becomes `compose_system_prompt("")`. Confirm `send_chat_request` still passes `m_config.system_prompt` through `expand_template` and appends glossary/examples unchanged. (R3.2, R3.3, R7.2, R7.4)
- [ ] 3.3 Unit test `set_system_prompt`: after `set_system_prompt(instruction)`, `config().system_prompt == compose_system_prompt(instruction)`; empty instruction yields the default composition. If reachable, also assert `expand_template` substitutes `{{source_lang}}`/`{{target_lang}}` and leaves `_upper` tokens verbatim; otherwise assert absence of `_upper` in the composed prompt (design Testing note). (R9.1)

## 4. Prompt tab UI

- [ ] 4.1 In `translation_settings_view_t::build_prompt_tab`: replace the variable-listing description label with a plain explanation (languages/glossary/examples added automatically, no `{{...}}`); add a greyed read-only word-wrapped label rendering `language_prefix()` above `m_prompt_edit`; set `m_prompt_edit` placeholder to `default_instruction()`; Reset button restores `default_instruction()`. All new strings `tr(...)`. (R5.1)
- [ ] 4.2 Confirm `load` populates `m_prompt_edit` from `settings.translation_prompt()` and `apply` writes it back via `set_translation_prompt` — calls unchanged, now the instruction body; no composition in the view. (R5.2)
- [ ] 4.3 Confirm padding/label styling conventions are followed for the new labels. (R5.3)

## 5. Documentation and steering

- [ ] 5.1 `.kiro/steering/web-translator-architecture.md`: remove `{{source_lang_upper}}` / `{{target_lang_upper}}` rows from the Template Variables table; rewrite the System Prompt section for the fixed prefix (code-owned, auto-substituted, not stored) + editable instruction body (`[Translation]/SystemPrompt`), naming `language_prefix()`, `default_instruction()`, `compose_system_prompt`. (R8.1)
- [ ] 5.2 `docs/yTranslator-Manual.md`: update the Auto Translation / Prompt prose — user edits only the instruction; language direction inserted automatically; glossary/examples appended automatically. No internals. (R8.2, manual-style)
- [ ] 5.3 `CHANGELOG.md` `[XXX]` → yTranslator → `[CHANGE]` describing the Prompt tab now holding only the editable instruction with the language direction added automatically. Update `README.md` + `docs/README.bbcode` (mirrored) only if they reference the prompt. (R8.3, changelog-categories, bbcode-sync)

## Notes

- yTranslator + yampt.qt only; no yampt.core changes.
- Composition lives in `web_translator_config::compose_system_prompt`, invoked from `set_system_prompt` — never in the settings view.
- The fixed prefix is code-owned and never persisted; only the instruction body is stored under `[Translation]/SystemPrompt`.
- No migration for old full-blob stored values (pre-1.0, legacy-migration rule): the blob becomes the instruction body under the fixed prefix.
- Simple (non-chat) providers are unaffected beyond losing the two unused `_upper` variables.
- Building and running tests are done manually by the user (project no-build rule); tasks produce tests as artifacts, no "run tests" step.
