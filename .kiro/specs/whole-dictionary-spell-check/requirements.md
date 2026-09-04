# Requirements — Whole-Dictionary Spell Check in yTranslator

## Background — Current Behavior

yTranslator has a spell checker used only for live, in-editor highlighting of the translation being typed.

- `spell_checker_t` (yampt.translator/source/editor/spell_checker.hpp/.cpp) wraps Hunspell (PIMPL). API: `load(aff, dic)`, `is_loaded()`, `check_word(word)`, `suggest(word)`, `add_to_user_dict(word)`, `find_misspelled(text) -> std::vector<spell_match_t>` where `spell_match_t { size_t start; size_t end; std::string word; }`, and `set_excluded_words(...)`.
- `check_word` returns true (spelled OK) for: empty words, excluded words (`is_excluded`), MWScript keywords (`is_mwscript_keyword`), and always true when no dictionary is loaded; otherwise it returns `hunspell->spell(word)`.
- `find_misspelled` tokenizes a string treating a "word char" as `isalnum(c) || c == '\'' || c >= 0x80` (so UTF-8 multibyte letters and apostrophes are part of words), trims trailing apostrophes, and collects tokens that fail `check_word`. **There is no all-uppercase skip today.**
- The checker is loaded in `main_window_t::on_spell_lang_changed()` from `settings.spell_aff_path()` / `settings.spell_dic_path()` — the **native-language** Hunspell dictionary (the language the translation `new_text` is written in). It is used only by the editor highlighter (`editor_highlighter.cpp::highlight_spelling`) and the spell context menu. There is no batch pass over the whole document today.

- Statuses are a real `enum class status_t` in yampt.core/source/utility/status_types.hpp, paired with a `status_entries` array (17 entries) that drives `status_to_string` / `string_to_status` (used at the JSON boundary in `dict_writer.cpp` / `dict_reader.cpp`). `is_approved_status(status)` returns `status == status_t::translated` and is the single gate the CLI convert/create path checks (`esm_converter.cpp`, `script_parser.cpp`) — every non-`translated` status is skipped during `--convert` / `--create`.
- GUI status filtering is driven by a hardcoded `status_order` vector in `status_filter_view.cpp`, per-status tooltips in `get_status_tooltip`, display names in `status_display.hpp::status_display_name` (e.g. `model` → "Generated"; a status with no case renders as "Error"), and colors in `theme_system.cpp::get_status_color` (+ `color_name_t` in theme_system.hpp). A dict document's filterable statuses are gated by `dict_document_t::supported_statuses()`.
- Batch-over-entries precedent: the table context menu emits `batch_status_change_requested(rows, new_status)`, handled in `main_window_setup.cpp` by looping rows and calling `m_active_doc->commit_status(*row_data, new_status)`, then `set_unsaved_changes` + `update_status_counts`. `dict_document_t` exposes `data()`/`data_mut()`, `commit_status`, `modified_records_insert`, `set_dirty`, and `is_dirty`.

## Problem

A translator can finish a dictionary with typos in the native-language translations and never notice, because spelling is only surfaced for the one entry currently being edited. There is no way to sweep the whole dictionary for misspellings, flag the offending entries, and review them as a group. Acronyms and tag-like tokens written in all caps (e.g. `NPC`, `GMST`, `MCP`) are legitimate and must not be flagged as typos.

## Goal

Add a whole-dictionary spell-check operation that scans every entry's translation (`new_text`) with the native-language Hunspell dictionary, ignoring all-uppercase words, and tags entries that contain at least one misspelling with a new `misspelled` status. The `misspelled` status is filterable and reviewable, and is NOT approved — it is skipped during `--convert`/`--create` like every non-`translated` status.

## User-Facing Outcomes

- A new action (menu/toolbar) runs "spell check whole dictionary" over the active dict document.
- Every entry whose `new_text` contains at least one misspelled word (per the native dictionary, ignoring all-uppercase tokens, exclusions, and MWScript keywords) is set to status `misspelled`. Entries with clean translations are left untouched.
- `misspelled` appears in the status filter list (with its own color, display name "Misspelled", and tooltip) so the user can isolate and review flagged entries.
- `misspelled` is never applied during `--convert`/`--create` — only `translated` is. Fixing a flagged entry (editing its translation) moves it off `misspelled` through the normal commit path.
- If no native Hunspell dictionary is loaded/configured, the operation reports that it cannot run rather than silently tagging nothing (or tagging everything).

## Requirements

### R1 — New `misspelled` status

1.1 Add `misspelled` to `enum class status_t` and to the `status_entries` array in yampt.core/source/utility/status_types.hpp, bumping the array size. `status_to_string`/`string_to_status` then round-trip it automatically at the JSON boundary.
1.2 `is_approved_status` remains `== status_t::translated` only, so `misspelled` is automatically excluded from `--convert`/`--create` with no converter/parser changes (this is the desired behavior, confirmed for the status-definitions rule).
1.3 `misspelled` is a user/GUI status (assigned by the spell-check operation), consistent with `model`, `propagated`, `replaced` in the status-definitions taxonomy.

### R2 — Status presentation

2.1 `status_display.hpp::status_display_name` gains a `case status_t::misspelled` returning the translated display name "Misspelled" (`QCoreApplication::translate("yTranslator", "Misspelled")`), so it does not render as "Error".
2.2 `theme_system.cpp::get_status_color` gains a `case status_t::misspelled`, backed by a new `color_name_t::status_misspelled` (theme_system.hpp) with light/dark color definitions in the color table.
2.3 `status_filter_view.cpp`: `misspelled` is added to `status_order` so it appears in the filter list, and `get_status_tooltip` gains a `case` describing it (e.g. "Translation contains a misspelled word").
2.4 `dict_document_t::supported_statuses()` includes `misspelled` so the filter shows it for dictionaries. (YAML documents keep their existing `{translated, untranslated}` set unless the design opts to spell-check YAML too — see Open Decisions.)

### R3 — All-uppercase skip

3.1 The spell scan ignores all-uppercase tokens (acronyms/tags), treating them as correctly spelled. "All uppercase" means a token whose cased letters are all uppercase (a token with no lowercase letters), evaluated correctly for UTF-8 (native-language letters, not just ASCII).
3.2 The skip is applied consistently so both a future in-editor use and this batch pass agree. The design will decide whether the all-caps guard lives inside `spell_checker_t::check_word` (affecting the live highlighter too) or in a dedicated batch routine; the requirement is only that the whole-dictionary scan skips all-caps tokens.
3.3 Existing skips are preserved: excluded words (`set_excluded_words`) and MWScript keywords are still treated as correct.

### R4 — Whole-dictionary scan operation

4.1 The operation iterates every entry of the active dict document and runs the native-language spell check over each entry's `new_text`.
4.2 An entry is flagged `misspelled` iff its `new_text` contains at least one token that (a) is not all-uppercase, (b) is not excluded, (c) is not an MWScript keyword, and (d) fails Hunspell `spell`.
4.3 Entries whose translation is clean are left unchanged (status not modified). Entries that are `untranslated` (no real translation) are not flagged — the design will specify the exact skip set (e.g. skip `untranslated`, and optionally skip already-`misspelled` re-tagging is a no-op).
4.4 Flagging sets status `misspelled` via the document, marks it dirty and records it in `modified_records`, and does NOT propagate (spell-tagging is not a translation change). Follow the batch pattern (direct `data_mut()` write + `modified_records_insert` + `set_dirty`, or `commit_status`, whichever the design selects — both avoid propagation).
4.5 The operation reports a summary (e.g. how many entries were flagged) via the status bar / log, consistent with existing batch feedback.

### R5 — Native dictionary requirement

5.1 The scan uses the native-language Hunspell dictionary (the same `.aff`/`.dic` the live checker loads from `settings.spell_aff_path()`/`settings.spell_dic_path()`), since `new_text` is in the native language.
5.2 If no native dictionary is loaded, the operation does not run a meaningless pass (which would treat every word as correct); it reports that a native dictionary is required and tags nothing.

### R6 — Orchestration placement

6.1 Per the Main Window Anti-Gravity rule, the scan orchestration (iterate entries, run checker, apply status, report) lives on a controller (existing editor/spell controller or a new one), not inline in `main_window_t`. The main window only wires the action to the controller method.
6.2 The pure scanning logic (given a translation string and a checker, decide flagged/clean) is extracted so it is unit-testable without a document or UI.

### R7 — No regression

7.1 Existing statuses, the status filter, convert/create behavior, and the live editor spell highlighter are unchanged except for the added `misspelled` status and the all-caps skip (if the skip is placed in `check_word`, the live highlighter also stops flagging all-caps tokens — an intended, benign improvement; the design notes this).
7.2 The JSON dictionary format is unchanged structurally; `misspelled` serializes as the string `"misspelled"` via the existing status mechanism.

### R8 — Verification

8.1 Pure `[u]` tests (no disk, no UI): the flagging predicate returns clean for an all-uppercase token, clean for an empty/whitespace translation, and flagged for a translation containing a genuinely misspelled lowercase word; the all-caps detection is correct for a UTF-8 native-language token. The Hunspell-backed check itself is exercised through a small stub/seam if needed so `[u]` tests stay in-memory.
8.2 Status round-trip: `status_to_string(status_t::misspelled) == "misspelled"` and `string_to_status("misspelled") == status_t::misspelled`.
8.3 `is_approved_status(status_t::misspelled) == false`.
8.4 Manual verification: running the operation on a dictionary with a deliberate typo flags exactly that entry as `misspelled`, the filter isolates it, and `--convert`/`--create` skip it.

## Open Decisions

Resolved:
- New `misspelled` status, added to `status_types.hpp`; not approved (auto-skipped by convert/create). (R1)
- Scan uses the native-language dictionary over `new_text`, ignores all-uppercase tokens, keeps existing exclusions/keyword skips. (R3, R5)
- Flagging does not propagate. (R4.4)
- Orchestration on a controller, pure predicate extracted for testing. (R6)

Deferred to design:
- Whether the all-caps skip lives in `spell_checker_t::check_word` (affects the live highlighter too) or only in the batch routine (R3.2).
- Exact skip set for the scan (definitely `untranslated`; whether other non-translated statuses are skipped or also scanned) (R4.3).
- Whether the operation also scans YAML (l10n) documents or dict documents only (affects `supported_statuses` for yaml) — default: dict documents only.
- The action's placement (menu vs toolbar) and label wording.
- Whether a `[u]` seam is added to `spell_checker_t` to test the predicate without a real Hunspell dictionary, or the predicate is tested purely on the tokenization/all-caps logic with the Hunspell call mocked.
