# Design — Whole-Dictionary Spell Check in yTranslator

## Context (current mechanics)

- **spell_checker_t** (yampt.translator/source/editor/spell_checker.cpp) — PIMPL over Hunspell. `check_word(word)`: empty → true; `is_excluded(word)` → true; `is_mwscript_keyword(word)` → true; `!hunspell` → true; else `hunspell->spell(word)`. `find_misspelled(text)`: byte tokenizer, word char = `isalnum || '\'' || >=0x80`, trims trailing apostrophes, returns `spell_match_t{start,end,word}` for tokens failing `check_word`. No all-caps skip. Loaded in `main_window_t::on_spell_lang_changed()` from `settings.spell_aff_path()`/`spell_dic_path()` (native dictionary). Consumed only by `editor_highlighter.cpp` and `spell_context_menu_t`.
- **status_types.hpp** (yampt.core) — `enum class status_t` (17 values) + `constexpr std::array<std::pair<status_t,std::string_view>,17> status_entries`. `status_to_string`/`string_to_status` iterate the array (JSON boundary: `dict_writer.cpp` line ~59, `dict_reader.cpp` line ~31). `is_approved_status(s) = (s == translated)`; checked in `esm_converter.cpp` (lines 74, 143) and `script_parser.cpp` (388, 396, 410, 520).
- **Presentation** — `status_display.hpp::status_display_name(status_t)` switch (no real default → "Error"); `theme_system.cpp::get_status_color(status_t, theme_t)` exhaustive switch → `color_name_t` (+ light/dark table); `status_filter_view.cpp` hardcoded `status_order` vector + `get_status_tooltip` switch + `build_rows` iterating `status_order`; `dict_document_t::supported_statuses()` set gates which show.
- **Batch precedent** — `batch_status_change_requested(rows, new_status)` handled in `main_window_setup.cpp` (~755): loop rows → `m_active_doc->commit_status(*row_data, new_status)` → `update_row` → `set_unsaved_changes` + `update_status_counts`. `commit_status` sets status, marks dirty, inserts modified record, does NOT propagate. `dict_document_t` also exposes `data_mut()`, `modified_records_insert`, `set_dirty`.

## Design Goals

Add a `misspelled` status (R1, R2), an all-uppercase skip (R3), a whole-dictionary scan that flags entries whose `new_text` misspells (R4, R5), orchestrated on a controller with an extracted testable predicate (R6), regressing nothing (R7), verifiable in-memory (R8) — honoring architecture rules (yampt.core purity for the status; ≤50-line functions; ≤3 nesting; controller for orchestration; tr() wrapping).

## Decision: add `misspelled` as a first-class status_t

Edit `status_types.hpp`:
- Add `misspelled` to the enum (append at the end, after `error`, to keep existing serialized ints/order stable — status is serialized as a string so ordinal does not matter, but appending is least disruptive).
- Add `{ status_t::misspelled, "misspelled" }` to `status_entries` and bump the array size `17 → 18`.

`is_approved_status` is unchanged, so `misspelled` is skipped by convert/create automatically (R1.2). JSON round-trips automatically (R7.2, R8.2).

Then wire presentation:
- `status_display.hpp`: `case status_t::misspelled: return QCoreApplication::translate("yTranslator", "Misspelled");`
- `theme_system.hpp`: add `color_name_t::status_misspelled`; `theme_system.cpp`: `case status_t::misspelled: return ...status_misspelled;` + light/dark entries in the color table (a distinct hue from `status_error`; e.g. an amber/orange, design picks the exact values consistent with the palette).
- `status_filter_view.cpp`: add `status_t::misspelled` to `status_order`; add a `get_status_tooltip` case: "Translation contains a misspelled word".
- `dict_document_t::supported_statuses()`: add `misspelled`.

## Decision: all-caps skip in a shared helper, applied by the scan (not silently in the live path)

Add a small pure predicate to `string_utils` (per no-duplicated-utility rule):

```cpp
// string_utils
bool is_all_uppercase_utf8(std::string_view token); // true if token has no lowercase letters
```

Implemented via the existing UTF-8 lowering: a token is "all uppercase" when `to_lower_utf8(token) != token` is false in the direction that matters — concretely, when lowering changes nothing AND the token contains at least one cased letter. The design uses: `is_all_uppercase_utf8(t)` = there exists a cased letter in `t` and `to_lower_utf8(t) != t`. This correctly treats `NPC`, `GMST`, Cyrillic all-caps, etc. as all-caps, and does not treat digit/punct-only tokens as all-caps.

The scan applies the skip; the live `check_word` is left as-is to avoid changing highlighter behavior unless desired. Decision: **place the skip in the scan predicate, not in `check_word`** — this keeps the live highlighter's behavior identical (R7.1) and scopes the feature. (If product later wants all-caps skipped live too, the same helper is reused in `check_word`.)

### Rejected alternative

Putting the all-caps guard inside `spell_checker_t::check_word`. Rejected for now: it silently changes the live editor highlighter (would stop underlining all-caps typos everywhere), a scope creep not asked for. The helper is shared so this remains a one-line future change.

## Decision: extract a pure flagging predicate

Add a stateless function usable from a controller and from `[u]` tests:

```cpp
// e.g. spell_scan (namespace) in yampt.translator/source/editor/spell_scan.hpp/.cpp
bool translation_has_misspelling(const std::string & new_text, const spell_checker_t & checker);
```

Logic: tokenize `new_text` the same way `find_misspelled` does (reuse `spell_checker_t::find_misspelled` to get candidate misspellings, then filter out all-caps tokens); return true if any surviving token remains. Concretely:

```cpp
bool spell_scan::translation_has_misspelling(const std::string & new_text, const spell_checker_t & checker)
{
    const auto matches = checker.find_misspelled(new_text);
    for (const auto & match : matches)
    {
        if (string_utils::is_all_uppercase_utf8(match.word))
            continue;

        return true;
    }

    return false;
}
```

This reuses the existing tokenizer and exclusion/keyword logic (they are inside `check_word`, which `find_misspelled` already calls), and layers the all-caps skip on top (R3.3, R4.2). It is pure (checker + string in, bool out), so `[u]`-testable (R6.2, R8.1) — the checker can be loaded from a tiny fixture dictionary in an `[i]` test, or the all-caps/empty branches tested with an unloaded checker (`!hunspell` → `find_misspelled` returns empty → clean) for `[u]`.

## Decision: orchestration on a controller

Per the Anti-Gravity rule, add the scan orchestration to a controller (the existing spell-related controller if one exists, else a new `spell_scan_controller_t` in `yampt.translator/source/controller/` or the editor folder). The controller:

1. Guards: active document is a `dict_document_t`; the native `spell_checker_t` `is_loaded()` — if not, report "native dictionary required" and stop (R5.2).
2. Iterates `dict_doc->data_mut()`; for each record, skip `untranslated` (and empty `new_text`); run `spell_scan::translation_has_misspelling(rec.new_text, checker)`.
3. For flagged records: set `rec.status = status_t::misspelled`, `dict_doc->modified_records_insert(type, i)`; no propagation (R4.4).
4. After the loop: `dict_doc->set_dirty(true)`, refresh the table model + status counts, report a summary (`statusBar` / log): "Flagged N entries as misspelled" (R4.5).

The main window wires an action to `controller.scan_active_document()` and applies the result to the table/counters — one call, no logic inline (R6.1). The per-entry loop uses early `continue` with blank lines (no-deep-nesting rule) and stays under the function-size limits (split into a `flag_document` helper + a `report` helper if needed).

The controller owns a reference to the shared `spell_checker_t` (the one `main_window_t` loads in `on_spell_lang_changed`) so it uses the already-loaded native dictionary; it does not load its own.

## Component Changes

| Area | Change |
|------|--------|
| `status_types.hpp` | add `misspelled` enum value + `status_entries` entry + bump size |
| `status_display.hpp` | `status_display_name` case → "Misspelled" |
| `theme_system.hpp/.cpp` | `color_name_t::status_misspelled` + `get_status_color` case + color table entries |
| `status_filter_view.cpp` | add to `status_order`; `get_status_tooltip` case |
| `dict_document.cpp` | `supported_statuses()` add `misspelled` |
| `string_utils.hpp/.cpp` | `is_all_uppercase_utf8` |
| `spell_scan.hpp/.cpp` (new) | `translation_has_misspelling` pure predicate |
| controller (new or existing) | scan orchestration over the active dict document |
| `main_window` + setup | action wiring to the controller (thin) |

## Data Flow

Action → controller.scan_active_document → guard (dict doc + loaded native checker) → loop entries, skip untranslated/empty → `translation_has_misspelling` → flag `misspelled` + record modified (no propagation) → set dirty, refresh table/counts, report summary. Filter shows `misspelled`; convert/create skip it (`is_approved_status` false). Editing a flagged entry commits through the normal path, replacing `misspelled`.

## Error Handling

- No native dictionary loaded → controller reports "native dictionary required", tags nothing (R5.2).
- Non-dict active document (yaml/loc/eet) → operation is a no-op (dict-only per Open Decisions default), reported.
- Empty document / no flags → summary reports zero flagged; document not marked dirty if nothing changed.

## Testing Strategy (R8)

`[u]` (in-memory):
- `string_utils::is_all_uppercase_utf8` — true for `"NPC"`, `"GMST"`, an all-caps native token; false for `"Npc"`, `"npc"`, `"123"`, `""`.
- `spell_scan::translation_has_misspelling` with an **unloaded** checker → clean (find_misspelled returns empty); with the all-caps-only input → clean; the "flagged" path (a real misspelling) is validated in an `[i]` test that loads a tiny fixture `.aff/.dic` from the temp dir, or via a seam.
- `status_types`: `status_to_string(misspelled)=="misspelled"`, `string_to_status("misspelled")==misspelled`, `is_approved_status(misspelled)==false`.

Building/running tests is manual (no-build-or-test rule). Test names follow `owner::member, description`, e.g. `"spell_scan::translation_has_misspelling, all caps token is clean"`, `"status_types::is_approved_status, misspelled not approved"`.

## Files Touched

| File | Change |
|------|--------|
| `yampt.core/source/utility/status_types.hpp` | new status + array + size |
| `yampt.core/source/utility/string_utils.hpp/.cpp` | `is_all_uppercase_utf8` |
| `yampt.translator/source/view/status_display.hpp` | display name case |
| `yampt.qt/source/theme_system.hpp/.cpp` | color name + case + table |
| `yampt.translator/source/view/status_filter_view.cpp` | status_order + tooltip |
| `yampt.translator/source/model/dict_document.cpp` | supported_statuses |
| `yampt.translator/source/editor/spell_scan.hpp/.cpp` | new predicate |
| `yampt.translator/source/controller/spell_scan_controller.hpp/.cpp` (or existing controller) | orchestration |
| `yampt.translator/source/main_window*.cpp` | action wiring (thin) |
| `yampt.translator/yampt.translator.vcxproj` + `.filters` | register new files |
| `yampt.tests/*` + vcxproj/.filters | `[u]`/`[i]` tests |

## Documentation

- CHANGELOG `[NEW]` (yTranslator): a whole-dictionary spell check that flags entries whose translation contains a misspelled word (all-uppercase acronyms ignored) with a new "Misspelled" status; flagged entries are filterable and are not applied when converting/creating plugins.
- `docs/yTranslator-Manual.md`: describe the operation, the "Misspelled" status, that acronyms in all caps are ignored, and that misspelled entries are excluded from conversion until fixed.
- README + README.bbcode in sync if spell checking is described. No internal detail (Hunspell, enum) in user docs.
