# Implementation Plan: Scope DIAL/INFO Hyperlink Highlighting to DIAL/INFO Records

## Goal

In yTranslator's Original and Translation editor text boxes, the blue DIAL/INFO
hyperlink highlight must appear ONLY when the currently-edited entry is a DIAL or
INFO record. Green glossary terms (FNAM/CELL/RNAM/INDX) and purple inflection
forms must keep showing for ALL record types — do not touch them.

## Investigation Findings (authoritative — the coder should trust these)

### Where hyperlink highlighting actually lives

The live highlighting of the editor text boxes is the **ExtraSelections path**, NOT
a `QSyntaxHighlighter`. The flow is:

1. `record_display_controller_t` (`yampt.translator/source/controller/record_display_controller.cpp`)
   gathers annotations and an `enabled_kinds` set, builds a `highlight_request_t`,
   and calls `highlight_coordinator_t::find_annotation_highlights(...)`.
2. `highlight_coordinator_t::find_annotation_highlights`
   (`yampt.translator/source/highlighter/highlight_coordinator.cpp`) maps each
   annotation to a `highlight_kind_t` via `kind_for_annotation` (a
   `dial_topic` annotation → `highlight_kind_t::hyperlink`) and keeps a candidate
   only if its kind is present in `request.enabled_kinds`.
3. `highlight_applier_t::build_selections` / `apply`
   (`yampt.translator/source/highlighter/highlight_applier.cpp`) paints the blue
   background for `highlight_kind_t::hyperlink` and applies the selections.

So the single decision point for "mark this span as a hyperlink or not" is the
`enabled_kinds` set that `record_display_controller_t` passes into the request. If
`highlight_kind_t::hyperlink` is absent from that set, no blue hyperlink span is
produced — this is exactly the gate we need, with no change to the coordinator or
applier.

The `enabled_kinds` set originates from the toggle buttons via
`editor_view_t::enabled_highlight_kinds()`
(`yampt.translator/source/view/editor_view.cpp`). It currently reflects only the
H/I/G toggle state and knows nothing about record type. We must NOT change
`enabled_highlight_kinds()` itself — the toggle set is a user preference persisted
across sessions and shared by all record types. The record-type gate is applied on
top of it, inside the controller, where `row_data->type` is in scope.

There are three call sites in `record_display_controller.cpp` that build a
`highlight_request_t` from `enabled_kinds`:
- `apply_initial_highlights(const table_row_t * row_data, ...)` — builds BOTH the
  original-view request and the translation-view request (two requests).
- `refresh_highlight_filter(const table_row_t * row_data)` — builds the
  original-view request, then delegates the translation to
  `apply_translation_highlights`.
- `apply_translation_highlights(const table_row_t * row_data)` — builds the
  translation-view request.

All three already receive `row_data` (type `const table_row_t *`) and read
`row_data->type`, so the record type is already available at every gate point.

### Dead highlighter classes — DO NOT touch, DO NOT use

`glossary_highlighter_t` (`highlighter/glossary_highlighter.*`) and
`topic_highlighter_t` (`highlighter/topic_highlighter.*`) are both
`QSyntaxHighlighter`s that are **never instantiated** anywhere in the app (verified:
no `new glossary_highlighter_t` / `new topic_highlighter_t` outside MOC output).
`glossary_highlighter_t` even has an unused `m_record_type` member. They are legacy
dead code. They are NOT the path that colors the text boxes. Do not modify them and
do not route the gate through them. The highlighters actually attached to the three
editor documents are `editor_highlighter_t` instances (`m_hl_original`,
`m_hl_adapted`, `m_hl_translation` in `main_window`), and `editor_highlighter_t`
does MWScript/HTML syntax, forbidden chars, and spell check only — it never draws
hyperlink/glossary backgrounds. Leave it alone.

### Record-type classification helpers

The typed enum is `rec_type_t` in
`yampt.core/source/utility/domain_types.hpp`, with members `rec_type_t::dial` and
`rec_type_t::info`. There are **no** boolean helpers such as `is_dial` / `is_info`;
the codebase classifies by direct typed comparison against the enum (e.g.
`record_display_controller.cpp` already does
`row_data->type == rec_type_t::dial`, `== rec_type_t::sctx`, etc.). The enum
comparison IS the typed classification — it satisfies the table-driven / no-raw-string
rule because no string literal like `"DIAL"` is involved. The gate MUST use
`rec_type_t::dial` / `rec_type_t::info`, never a raw string compare.

### Gate design (explicit, no fallback)

Highlight hyperlinks IF AND ONLY IF the current entry's record type is
`rec_type_t::dial` OR `rec_type_t::info`; otherwise the hyperlink kind is removed
from the enabled set for that entry. There is no "default to highlighting" branch
and no fallback: for every record type that is not DIAL or INFO, hyperlink spans are
unconditionally suppressed. Glossary and inflection kinds are passed through
unchanged for all record types.

### New source file?

No new source file is needed. The gate is a small private helper on the existing
`record_display_controller_t` class, implemented in the existing
`record_display_controller.hpp/.cpp`. Therefore **no `.vcxproj` / `.vcxproj.filters`
edits** are required for `yampt.translator` or `yampt.tests`.

## Plan

- [ ] 1. Add a record-type hyperlink gate helper to `record_display_controller_t`.
      Declare a private helper
      `std::set<highlight_kind_t> hyperlink_scoped_kinds(rec_type_t type) const;`
      in `record_display_controller.hpp` (ensure `#include <set>` and the
      `highlight_coordinator.hpp` include that defines `highlight_kind_t` are present
      — the header already includes `../highlighter/highlight_coordinator.hpp`).
      Implement it in `record_display_controller.cpp`: start from
      `m_deps.editor_view.enabled_highlight_kinds()`; if `type` is neither
      `rec_type_t::dial` nor `rec_type_t::info`, `erase(highlight_kind_t::hyperlink)`
      from the set; return the set. Keep it under 50 lines, max 3 nesting levels,
      use early structure (single `if` guard), and put a blank line after any
      `return`/`continue`/`break` that is not the last statement in its block. No raw
      string comparisons — use the `rec_type_t` enum members only.
      Files: `yampt.translator/source/controller/record_display_controller.hpp`,
      `yampt.translator/source/controller/record_display_controller.cpp`
      Verify: user builds `yampt.translator`; load a DIAL or INFO entry and confirm
      blue topic highlights appear in the Original and Translation boxes.

- [ ] 2. Route all three highlight-request build sites through the gate.
      In `record_display_controller.cpp`, replace the local
      `const auto enabled_kinds = m_deps.editor_view.enabled_highlight_kinds();`
      (and the inline `m_deps.editor_view.enabled_highlight_kinds()` argument in
      `apply_translation_highlights`'s `highlight_request_t`) with the gated set from
      `hyperlink_scoped_kinds(row_data->type)` at these three functions:
      `apply_initial_highlights` (used for both the orig_request and trans_request —
      compute the gated set once at the top and reuse it for both requests),
      `refresh_highlight_filter`, and `apply_translation_highlights`. Do not change
      the glossary/inflection handling, the sort policies, or the `use_old_text`
      flags. Each function must stay ≤ 50 lines and ≤ 3 nesting levels after the edit.
      Files: `yampt.translator/source/controller/record_display_controller.cpp`
      Verify: user builds `yampt.translator`; load a non-DIAL/INFO entry (e.g. CELL,
      FNAM, GMST) and confirm NO blue hyperlink highlight appears, while green
      glossary terms still highlight; load a DIAL/INFO entry and confirm blue
      hyperlinks DO appear. Toggle the H button off on a DIAL entry and confirm the
      blue highlight disappears (toggle still works), on top of the record-type gate.

- [ ] 3. Add a unit test for the gate logic.
      The gate helper is a method on `record_display_controller_t`, which has heavy
      Qt dependencies, so testing the method directly requires constructing the
      controller. To keep the test pure (`[u]` tag, in-memory, no Qt widgets, no file
      I/O) per the unit-test rules, extract the pure decision into a free function in
      a namespace that both the controller and the test call. Add
      `std::set<highlight_kind_t> scope_kinds_for_record(const std::set<highlight_kind_t> & enabled_kinds, rec_type_t type);`
      to `highlight_coordinator.hpp` (declaration) and implement it in
      `highlight_coordinator.cpp`: copy `enabled_kinds`, erase
      `highlight_kind_t::hyperlink` unless `type` is `rec_type_t::dial` or
      `rec_type_t::info`, return it. Then have
      `record_display_controller_t::hyperlink_scoped_kinds` delegate to it
      (`return scope_kinds_for_record(m_deps.editor_view.enabled_highlight_kinds(), type);`).
      `highlight_coordinator.cpp` is in `yampt.translator` and is already compiled
      into `yampt.tests` (translator `.cpp` files are compiled directly by the test
      project), so no `.vcxproj`/`.filters` edits are needed. Add a Catch2 test file
      `yampt.tests/source/test_highlight_scope.cpp` (follow the existing test-file
      pattern in `yampt.tests/source/`) with cases named per the convention
      `"highlight_coordinator::scope_kinds_for_record, <desc>"`, covering: DIAL keeps
      hyperlink, INFO keeps hyperlink, CELL/FNAM/GMST drop hyperlink, glossary and
      inflection kinds are preserved for every type, and hyperlink stays absent when
      it was not enabled to begin with. If a new test `.cpp` is added, update
      `yampt.tests/yampt.tests.vcxproj` AND its flat `.vcxproj.filters` in the same
      edit (per project-paths rules).
      Files: `yampt.translator/source/highlighter/highlight_coordinator.hpp`,
      `yampt.translator/source/highlighter/highlight_coordinator.cpp`,
      `yampt.translator/source/controller/record_display_controller.hpp`,
      `yampt.translator/source/controller/record_display_controller.cpp`,
      `yampt.tests/source/test_highlight_scope.cpp`,
      `yampt.tests/yampt.tests.vcxproj`, `yampt.tests/yampt.tests.vcxproj.filters`
      Verify: user builds `yampt.tests` and runs `x64\Debug\yampt.tests.exe`;
      the new `highlight_coordinator::scope_kinds_for_record` cases pass.

      NOTE on ordering vs. steps 1–2: if step 3's extraction is implemented, step 1's
      helper simply delegates to `scope_kinds_for_record` instead of duplicating the
      erase logic. The coder should implement step 3's free function first, then make
      step 1's helper a one-line delegate, to avoid two copies of the decision (DRY /
      one-table-per-concern). Steps 1 and 2 remain the integration into the live path.

## Required non-code edits (part of this task — the coder MUST do these)

- [ ] 4. Update steering: `c:\S\yampt\.kiro\steering\design-decisions.md`,
      "Glossary Sources" section. The current line reads:
      `Glossary terms show for all record types. DIAL hyperlinks show for all record types.`
      Change ONLY the DIAL sentence so it states that DIAL hyperlinks now show only
      for DIAL and INFO record types. Keep the glossary-terms sentence exactly as is.
      Suggested result:
      `Glossary terms show for all record types. DIAL hyperlinks show only for DIAL and INFO record types.`
      Files: `c:\S\yampt\.kiro\steering\design-decisions.md`
      Verify: visual read — the glossary sentence is unchanged and the DIAL sentence
      now limits hyperlinks to DIAL/INFO.

- [ ] 5. Add a CHANGELOG entry: `c:\S\yampt\CHANGELOG.md`, under the current
      unreleased version `## [XXX]`, `### yTranslator` section, tag `[CHANGE]`,
      inserted after the last existing `[CHANGE]` in that yTranslator section (there
      is currently a `[NEW]` and a `[FIX]` but no `[CHANGE]` yet — per the
      changelog-categories ordering `[NEW] → [CHANGE] → [FIX] → [REMOVE]`, insert the
      new `[CHANGE]` after the `[NEW]` line and before the `[FIX]` line). This is a
      `[CHANGE]` because the user sees hyperlight highlighting behave differently
      (it no longer appears on non-dialogue records). Suggested text:
      `- [CHANGE] The blue dialogue-topic highlighting in the Original and Translation boxes now appears only while editing a dialogue topic or dialogue response entry; on all other record types it is no longer shown. The green glossary highlighting is unaffected and still appears for every record type`
      Files: `c:\S\yampt\CHANGELOG.md`
      Verify: visual read — entry is tagged `[CHANGE]`, sits in the yTranslator
      section of `## [XXX]`, in the correct tag order, and does not mention tests,
      scripts, or build changes.

- [ ] 6. Update the yTranslator manual prose:
      `c:\S\yampt\docs\yTranslator-Manual.md`. The paragraph describing the Original
      and Translation panel inline highlighting currently begins:
      `The Original and Translation panels also highlight recognized terms inline: dialog topic names appear in blue (matching known DIAL entries), glossary terms ... appear in green, and inflected topic forms ...`.
      Revise the prose so it states that the blue dialogue-topic highlighting appears
      only when editing a dialogue topic or dialogue response entry, while the green
      glossary highlighting and the inflected-form highlighting still appear for every
      record type. Keep manual-style prose (full sentences, no internal technology or
      enum/class names — say "dialogue topic or dialogue response entries", not
      "DIAL/INFO records"; "DIAL entries" already appears parenthetically in the
      existing sentence, so rephrase that clause to user-facing language). Do not
      repeat facts stated elsewhere.
      Files: `c:\S\yampt\docs\yTranslator-Manual.md`
      Verify: visual read — the paragraph now limits blue hyperlinks to
      dialogue-related entries and leaves green glossary/inflection described as
      all-record-type.

- [ ] 7. README / BBCode: NO feature-level change required.
      `c:\S\yampt\README.md` and `c:\S\yampt\docs\README.bbcode` describe hyperlink
      highlighting only as a feature bullet
      ("Multi-layer syntax highlighting: ... hyperlinks ...") without stating the
      record-type scope. Per the task's "mirror ONLY if they describe this at feature
      level" instruction, and because they do not describe the scope, these two files
      are left unchanged. (Recorded here so the coder does not add anything to them.)
      Files: none
      Verify: confirm neither README nor BBCode mentions the scope, so no edit is
      warranted.

## Constraints carried into implementation

- snake_case for all new identifiers; Qt overrides stay camelCase (no new Qt
  overrides are added here).
- Max 50 lines per function, max 3 nesting levels, early returns, blank line after
  `return`/`continue`/`break` unless it is the last statement in the block.
- No raw record-type string comparisons — use `rec_type_t::dial` / `rec_type_t::info`.
- No fallback / "default to highlighting" branch.
- Any new user-visible string wrapped in `tr(...)` — none are introduced by this
  change (the gate adds no UI text).
- Do NOT build or run tests automatically — the user builds manually. Verify steps
  describe what the user runs.

## Loop stop contract

The reviewer writes the verdict to
`c:\S\yampt\.agents\tasks\hyperlink-scope-review.json` with JSON path `verdict`
equal to `APPROVED` as the loop's final step.

## Implementation Record (iteration 1)

### Files changed

Code:
- `yampt.translator/source/highlighter/highlight_coordinator.hpp` — added
  `#include <utility/domain_types.hpp>` and declared the free function
  `scope_kinds_for_record(const std::set<highlight_kind_t> &, rec_type_t)`.
- `yampt.translator/source/highlighter/highlight_coordinator.cpp` — implemented
  `scope_kinds_for_record` (the pure decision; erases hyperlink unless DIAL/INFO).
- `yampt.translator/source/controller/record_display_controller.hpp` — declared
  private helper `std::set<highlight_kind_t> hyperlink_scoped_kinds(rec_type_t) const`.
- `yampt.translator/source/controller/record_display_controller.cpp` — implemented
  the helper (delegates to `scope_kinds_for_record`) and routed all three
  highlight-request build sites through it: `apply_initial_highlights` (orig +
  trans share one gated set), `refresh_highlight_filter`, `apply_translation_highlights`.
- `yampt.tests/source/tests.highlight_coordinator.cpp` — added four `[u]` unit tests
  for `scope_kinds_for_record` (DIAL keeps, INFO keeps, CELL/FNAM/GMST drop while
  keeping inflection+glossary, absent hyperlink stays absent). No vcxproj/filters
  edits needed: `highlight_coordinator.cpp` is already compiled by `yampt.tests`
  and `tests.highlight_coordinator.cpp` is already listed.

Non-code:
- `.kiro/steering/design-decisions.md` — "Glossary Sources" DIAL sentence changed to
  "DIAL hyperlinks show only for DIAL and INFO record types." Glossary sentence unchanged.
- `CHANGELOG.md` — `[CHANGE]` entry added in `## [0.1135]` yTranslator section after
  the last existing `[CHANGE]`.
- `docs/yTranslator-Manual.md` — Annotations prose revised: blue dialogue-topic
  highlighting only for dialogue topic/response entries; green glossary and inflected
  forms still for every record type.
- `README.md` / `docs/README.bbcode` — not changed; they do not describe the
  hyperlink scope at feature level.

### Gating code snippet

`highlight_coordinator.cpp`:
```cpp
std::set<highlight_kind_t> scope_kinds_for_record(
    const std::set<highlight_kind_t> & enabled_kinds,
    rec_type_t type)
{
	auto scoped = enabled_kinds;

	if (type != rec_type_t::dial && type != rec_type_t::info)
		scoped.erase(highlight_kind_t::hyperlink);

	return scoped;
}
```

`record_display_controller.cpp`:
```cpp
std::set<highlight_kind_t> record_display_controller_t::hyperlink_scoped_kinds(rec_type_t type) const
{
	return scope_kinds_for_record(m_deps.editor_view.enabled_highlight_kinds(), type);
}
```

All three request sites now pass `hyperlink_scoped_kinds(row_data->type)` as the
`enabled_kinds`. The gate is explicit (DIAL/INFO only, no fallback), uses the typed
`rec_type_t` enum (no raw string compares), and leaves glossary/inflection kinds
untouched.
