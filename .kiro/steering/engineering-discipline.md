# Engineering Discipline

## Test Before Fix

When fixing a bug, always write the unit test FIRST — before applying the fix. The test must fail against the current (broken) behavior, then pass after the fix is applied.

This ensures the bug is reproducible and the fix is verified.

This discipline applies to GUI bugs too, but mind where the test belongs. Most "GUI" bugs are really computation bugs — a duplicate or omitted row, a wrong filtered set, a wrong status — and get a FAILING LOGIC test first against the extracted seam, per the "When To Write a GUI Test" section below. Only an irreducibly visual bug (a view that fails to refresh after a model change, a rendering/geometry fault) gets a FAILING GUI test first in `yampt.gui.tests`. Either way: red before the fix, green after, same as any logic bug.

## Don't Delete Tests

Never delete a failing test without explicit user approval. If a test fails, diagnose the root cause and fix either the test data or the code under test. Do not remove the test to make the suite pass.

Never remove a test file or test entry from a vcxproj to fix build or linker errors. If a test doesn't compile or link, fix the underlying issue (add missing source files, fix includes, resolve MOC dependencies). Removing tests from the build is never an acceptable fix.

Every clause here applies equally to GUI tests. In particular, the "never remove a test file or test entry from a vcxproj" clause covers the `yampt.gui.tests` vcxproj just as it covers the logic-suite vcxproj: a GUI test that fails to link is fixed by adding the missing source, moc, or include — never by dropping the test from the GUI project. See the "GUI (Widget) Tests" section below.

If a test approach turns out to be fragile or unreliable, report the finding and propose an alternative — do not silently remove or skip the test.

### Synthetic ESM Tests Are Valid

Synthetic ESM tests (hand-crafted binary records) are the correct approach for testing the converter. They test exactly the case we included — no more, no less. Real ESM files also don't cover all edge cases — we have to figure out edge cases ourselves by testing a lot of real plugins and then encoding each discovery as a synthetic test. When a synthetic test fails, fix the test data to match the actual binary format (check sub-record sizes, key formats like zero-padded INDX, null terminators). Do not dismiss synthetic tests as unreliable.

## GUI (Widget) Tests

This section is the single authoritative home for how GUI tests work. Other steering files point here for the mechanics rather than restating them.

### When To Write a GUI Test

Default, always, to a logic test. A GUI test is the slowest, heaviest, most brittle category — it drags in `QApplication`, moc, and offscreen rendering — so it is a last resort, written ONLY when the behavior genuinely cannot be observed without constructing a widget. Before reaching for one, first try to make the behavior observable without a widget by extracting the computation into a non-widget seam.

**A GUI test is justified ONLY for the irreducibly visual — two cases:**

- **Signal/slot wiring with no observable seam** — a model change must refresh the live view, or a user action must drive a connected slot, and there is genuinely nothing non-visual to assert (e.g. a stale row that only a constructed, event-driven widget reveals). The computation being correct is not enough; the wire itself is under test and cannot be seen except through the widget.
- **True rendering / visual output** — colors, layout geometry, paint output, pixel-level results. These have no non-visual seam by definition.

Everything else is a logic test. If a behavior is computable it belongs in the `yampt.tests` logic suite, UI-free — including:

- all computation: filtering, merge, propagation, validation, status/intent mapping, eligibility rules (which operation is allowed for which row), byte-limit checks, text diffing, row-building from decoded records;
- presentation *decisions* and *values*, which are computable facts, not visuals: the DECISION that a control should be greyed, a convention VALUE such as a 24px settings-table row height, or the VALUE a `load`/`save` round-trip would persist.

If reaching a computable behavior seems to "need" a widget, that is only because the logic was not extracted. Extract it — a non-Q_OBJECT free function, namespace function, or POD-returning static per `code-style.md` Function Design — make the widget a thin caller of that seam, and test the seam logic-side. Seam extraction is a permitted production change but must be behavior-preserving (No Regressions): the widget must call the extracted logic and behave identically.

**We do NOT test that widgets work.** Never write a GUI test to confirm a button fires its slot, a checkbox toggles, or a signal reaches a connected slot — Qt guarantees that; asserting it tests Qt, not our code. A GUI test that merely re-checks a computation already covered logic-side is REDUNDANT — never add it.

**The one question:** can I observe this WITHOUT constructing a widget? If yes — even if it takes extracting a seam — it is a logic test. Only if the answer is genuinely no (irreducible live-view wiring, or actual rendering) does it earn a GUI test. When a feature has both a computation and a wire (propagation computes which siblings change AND the table must refresh their rows), split it: the computation is a logic test, and only the irreducible row-refresh wiring may be a GUI test.

Test-Before-Fix applies either way: a wiring/rendering bug gets a FAILING GUI test first; a computation bug gets a FAILING logic test first.

### Mechanics (Authoritative, Stated Once)

- GUI tests run in a separate executable, `yampt.gui.tests`, with its own `vcxproj` in `yampt.sln`. They do NOT fold into the logic executable `yampt.tests` — folding them in would force the logic suite under a GUI `QApplication` and link `Qt6Widgets`, changing tests that currently pass.
- Catch2 is the test runner and the assertion framework. Qt Test (`QTest::...`) is used ONLY as the widget/event driver inside Catch2 `TEST_CASE`s.
- The process constructs exactly one `QApplication` for the whole session, under the `offscreen` QPA platform. `QT_QPA_PLATFORM=offscreen` is set BEFORE `QApplication` is constructed (via `qputenv` in the GUI test `main`), because Qt reads it during construction.
- `Q_OBJECT` widget classes under test are MOC'd in the GUI project following the existing moc `CustomBuild` pattern (one `moc_*.cpp` per header, each compiled to a distinct `ObjectFileName`).
- CI-safe by construction: offscreen, no real display, no visible windows, and the tests never block on a modal `exec()`.

### Discovery

The Catch2 adapter's `FilenameFilter` in `yampt.runsettings` matches both `yampt.tests` and `yampt.gui.tests`, so both executables are discovered.

### Conventions Are Owned Elsewhere

The GUI conventions themselves (24px settings-table rows, tooltips, enable-vs-hide, panel padding) are defined in `gui-conventions.md`, which remains their single source of truth. Do not restate them here. Note that under "When To Write a GUI Test" the DECISION or VALUE behind a convention (that a row height equals 24, that a control is greyed) is a computable fact tested logic-side; a GUI test is reserved for the irreducibly visual. This section owns only the test mechanics; `gui-conventions.md` owns the conventions themselves.

## No Fallbacks, No Generic Paths

Never add a fallback branch or a generic catch-all path. Every case must be handled explicitly and correctly by name/type/schema. A fallback always hides a real mismatch and breaks a different case that was relying on the exact behavior.

### Rule

- Do NOT add "if the specific lookup fails, use something close" logic (e.g. same-index field, nearest size, first-match, whole-value byte guess, default schema).
- Do NOT add a generic branch that runs when no specific rule matches. If nothing matches, that is a bug in the data/schema — fix the data/schema so the specific path matches.
- When a lookup by name/type/size can miss, the fix is to make the name/type/size correct at the source (schema tables, record definitions), NOT to soften the lookup.
- If a value genuinely cannot be resolved, surface it loudly (log `[error]`, or show an explicit error marker), never silently substitute an approximation.

### Why

Fallbacks and generic paths look harmless but are the top source of regressions here:
- A schema-decode "same-index" fallback fixed one field mismatch but broke the NPC autocalc case (fields legitimately absent at 12-byte NPDT started showing wrong values).
- A whole-value byte merge fallback dropped per-field/per-bit merges for every sub-record not on an allow-list.

Each time, the fallback masked the real problem and corrupted an unrelated case. The correct fix is always the specific, explicit one.

### What To Do Instead

1. Identify the exact case that isn't matching.
2. Fix the schema/definition/table so the specific lookup matches by name/type/size.
3. If the case is genuinely distinct, add an explicit branch for it — named, not generic.
4. If it cannot be resolved, emit an explicit error, never a guess.

## No Regressions

Existing working behavior must keep working. A change may only add new behavior or fix a genuine bug. It must never make something that worked before stop working or silently work differently.

This rule exists because of a concrete failure: a consolidation refactor that folded the five list-merge mechanisms (ENAM, FACT, ARMO, NPCO, NPCS) into one keyed model broke all of them — ENAM merged non-existent fields, FACT and ARMO stopped merging entirely — and had to be reverted. That is the exact class of outcome this policy prevents.

### Forbidden

- Changing a code path that already works in order to clean up, unify, consolidate, or refactor, when that change alters behavior which has passing tests or known-correct output.
- Replacing a working mechanism with a "better" or more general one unless the replacement is proven behavior-identical first.
- Broad rewrites that put working features at risk for a non-functional goal (fewer files, one table, less duplication). The reverted consolidation is the cautionary example — do not reattempt it.

### Allowed

- Adding genuinely new behavior on an isolated path that does not touch existing working code. Example: CELL currently drops the FRMR region, so adding FRMR merge there regresses nothing.
- Fixing an actual bug. Making broken behavior correct is not a regression — it is the point.

### How to comply

- Prefer additive, isolated changes over editing shared working code.
- Before changing a working path, treat its existing tests and output as a contract. If you cannot show the change is behavior-preserving, do not make it.
- If a refactor cannot be proven safe, do not do it — or gate it behind the smallest possible surface and keep every existing mechanism intact.
- When in doubt, ask before altering working behavior. Do not assume a refactor is worth the risk.

## Table-Driven Behavior — No Hardcoding, No Ad Hoc Exclusions

Record and sub-record behavior — how a type is decoded, merged, keyed, excluded, or labeled — is driven by declarative decision tables and schemas, never by inline type checks scattered through the logic. There is one authoritative table per concern; code reads the table and acts on the result.

- **No hardcoded record-type or sub-type string checks in logic.** Never compare a record type or sub-type against a string literal to choose behavior. Read a field on the authoritative behavior table and branch on that. A dispatch decision comes from a table lookup, not a literal.
- **No ad hoc exclusions.** Never skip a type or sub-record with an inline literal. Exclusions belong in a table — a flag on the behavior entry, a schema entry, or the user-facing ignore list — chosen by whichever table already owns that concern. If no table can express the exclusion, add a typed field to the right table, then set it declaratively.
- **No fallback/generic guess paths.** Consistent with the no-fallbacks rule: if a lookup misses, fix the table so the specific case matches — do not soften the lookup or add a catch-all branch.
- **One table per concern.** Do not duplicate the same decision in two places (e.g. a table field AND a parallel `if` chain in the consumer). The code reads the table; the table is the decision.
- **Adding a new type or changing behavior is a table edit.** Introducing a type, changing how one merges, excluding a sub-record, or relabeling a field is done by editing the relevant table/schema — not by adding a branch in the algorithm. If the table cannot express the new behavior, extend its schema (add a typed column/flag) first, then set it.
- **Tables are typed, not stringly.** Use enums and typed flags, not magic strings or ints, to express behavior in a table.
- **UI eligibility (copy / lock / remove / exclude) composes two tables.** Whether the record view offers an operation for a clicked row is `record-type policy AND row-shape capability` — both from tables, never from an inline record-type or row-kind check in a menu builder.

The concrete table inventory — which file owns which concern, the field and accessor names, and the two-layer eligibility mechanics — is in `.kiro/docs/reference/decision-tables.md`. Consult it before touching decode/merge/eligibility. Inline type checks that predate this rule are tracked in `TODO.md`'s "Tech Debt" section — do NOT copy them as a pattern and do NOT add new ones.

## Debug Logging

Do NOT remove `[debug]` diagnostic logs before committing. Keep them in the code, gated behind the global debug flag so they are silent by default and can be turned on when needed.

This overrides any earlier rule that said `[debug]` messages must be removed before commit. Diagnostics are valuable to keep — the fix is to gate them, not delete them.

### How to gate a debug log

The shared logger already supports gating: it has a global debug flag and a silent logging path that emits an entry ONLY when the flag is on (and even then to the in-memory log, not stdout). Every `[debug]` line MUST go through that gated path — silent unless debug is enabled — never through an always-on write.

### Callers without the shared logger

Some classes log through a plain callback that always emits rather than the shared logger. When adding a `[debug]` line in such a class, route it through the shared logger's gated path, or wrap the callback call in a check of the global debug flag. Either keeps the diagnostic in the code but off by default.

### Rules

- Prefix diagnostic messages with `[debug]` (lowercase), per the log-prefix convention.
- Gate every `[debug]` line behind the global debug flag. Never leave an always-on `[debug]` line.
- Never delete a `[debug]` diagnostic to "clean up" before commit. Keep it, gated.
- Debug lines are developer-facing and stay in English (they are not user-visible; the always-update-docs and localization rules do not apply to them).
- The debug flag defaults to off, so gated `[debug]` lines produce no output in normal use and add no noise to the CLI or the GUI log.
- GUI-test diagnostics follow the same gated `[debug]` rule as any other diagnostic — no behavior change for the test harness.
