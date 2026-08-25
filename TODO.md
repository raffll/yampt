# TODO

$(SolutionDir) still created in release folder

make dictionary dont need base dict, create/convert need

Translator settings: propose better name: and also populate dynamically with providers file

## Audit Findings

- highlight_coordinator_t::to_lower_ascii only handles ASCII (A-Z), breaks annotation matching for non-English characters (Ö, Ą, É). Replace with string_utils::to_lower.
- web_translator_t::send_chat_request glossary_fn returns substituted text instead of term pairs for AI system prompt. Should use find_glossary_matches and format as reference list.
- script_parser_t::find_keyword lacks word boundary checks (unlike creator_helpers). Use find_whole_word and skip npos entries.
- script_parser_t member variables missing m_prefix (18 members: type, merger, record_key, source_path, old_script, old_scdt, new_script, is_done, line, line_lc, old_text, new_line, new_text, pos, keyword_pos, keyword, error).
- includes.hpp is a monolithic catch-all header pulling 17 std headers into 15 TUs. Eliminate and replace with per-file includes.
- README.md: Lua handler feature misplaced under "Viewing" section — should describe it as a navigation tab.
- Stale error status on BNAM entries: error status persists in saved dict after the offending content is fixed. Re-validate on load or on display and auto-clear error when text passes validation.
- Translation settings page: add Local Models tab alongside Web Providers tab. JSON configs in models/ describing available CTranslate2 models (name, path, supported languages). Similar discovery pattern to web providers — data-driven, no hardcoding.