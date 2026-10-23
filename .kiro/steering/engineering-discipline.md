# Engineering Discipline

## Test Before Fix

When fixing a bug, always write the unit test FIRST — before applying the fix. The test must fail against the current (broken) behavior, then pass after the fix is applied.

This ensures the bug is reproducible and the fix is verified.

## Don't Delete Tests

Never delete a failing test without explicit user approval. If a test fails, diagnose the root cause and fix either the test data or the code under test. Do not remove the test to make the suite pass.

Never remove a test file or test entry from a vcxproj to fix build or linker errors. If a test doesn't compile or link, fix the underlying issue (add missing source files, fix includes, resolve MOC dependencies). Removing tests from the build is never an acceptable fix.

If a test approach turns out to be fragile or unreliable, report the finding and propose an alternative — do not silently remove or skip the test.

### Synthetic ESM Tests Are Valid

Synthetic ESM tests (hand-crafted binary records) are the correct approach for testing the converter. They test exactly the case we included — no more, no less. Real ESM files also don't cover all edge cases — we have to figure out edge cases ourselves by testing a lot of real plugins and then encoding each discovery as a synthetic test. When a synthetic test fails, fix the test data to match the actual binary format (check sub-record sizes, key formats like zero-padded INDX, null terminators). Do not dismiss synthetic tests as unreliable.

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

Record and sub-record behavior — how a type is decoded, merged, keyed, excluded, or labeled — is driven by declarative **decision tables and schemas**, never by inline type checks scattered through the logic. There is one authoritative table per concern; code reads the table and acts on the result.

### Authoritative Tables

- **`record_behavior.cpp` / `record_behavior_t`** — per record type, the single source of truth for how a record merges, decodes, and what the UI permits. Fields (all with defaults, set via designated initializers): `decode_mode_t` (generic, cell, leveled, faction, container, armor, info, dial), `copy_strategy_t`, `merge_strategy_t` (generic, armor_parts), `enam_effect_list`, `merge_excluded`, `leveled_item_sub_type`, `keyed_list_sub_types`, `record_id_sub_type`, `read_only_reason_t` (editable, landscape_data), `allows_copy` / `allows_lock` / `allows_exclude`, the `sub_record_rule_t` list (with `element_wise_merge` / `skip_merge` / `skip_*` flags), the wildcard rule, and `paired_merge_rule_t` pairs. Accessors: `decode_mode_for`, `merge_strategy_for`, `is_enam_effect_list`, `is_merge_excluded`, `leveled_item_sub_type_for`, `is_keyed_list_sub_type`, `record_id_sub_type_for`, `read_only_reason_for`, `record_allows_copy` / `record_allows_lock` / `record_allows_exclude`. Rows use designated initializers — a rule change is one named field on one `.record_type = "..."`-anchored row.
- **`sub_record_schema.cpp` / `field_def_t` + `sub_record_schema_t`** — per (record type, sub-type, size): the field layout used to decode and to merge field-by-field or bit-by-bit. `find_schema` / `find_largest_schema` / `find_cell_data_schema` are the only lookups.
- **`record_composition` / `record_sub_record_t`** — per record type: which sub-records exist and their kind (single / multi / repeatable).
- Name/index lookups (`effect_name_by_index`, `skill_name_by_index`, `global_type_name`, and similar) are tables too — extend the table, never inline a switch at the call site.

### Rules

- **No hardcoded record-type or sub-type string checks in logic.** Never write `if (rec_type == "CELL")` / `if (sub_type == "FLAG")` to choose behavior. Add or read a field on the behavior table (a `decode_mode_t`, a `merge_strategy_t`, or a `sub_record_rule_t` flag such as `skip_merge`) and branch on that. A dispatch decision must come from a table lookup, not a literal.
- **No ad hoc exclusions.** Never skip a type/sub-record with an inline literal (e.g. `if (rec_type == "LAND") return false;`, `if (rec_type == "FACT" && type == "ANAM") continue;`). Exclusions belong in a table: a rule flag on the behavior entry, an entry in the schema, or the user-facing `RECORD:SUB` ignore list — chosen by whichever table already owns that concern. If the right table has no place for the exclusion, add a typed field to that table, then set it declaratively.
- **No fallback/generic guess paths.** Consistent with the no-fallbacks rule: if a lookup misses, fix the table so the specific case matches — do not soften the lookup or add a catch-all branch. A genuinely unresolved case surfaces loudly, it is not silently substituted.
- **One table per concern.** Do not duplicate the same decision in two places (e.g. a decode_mode in the behavior table AND a parallel `if` chain in the merger). The code reads the table; the table is the decision.
- **Adding a new type or changing behavior is a table edit.** Introducing a record type, changing how one merges, excluding a sub-record, or relabeling a field is done by editing the relevant table/schema — not by adding a branch in the algorithm. If the table cannot express the new behavior, extend the table's schema (add a typed column/flag) first, then set it.
- **Tables are typed, not stringly.** Use enums and typed flags (`decode_mode_t`, `sub_rule_flag_t`, `field_type_t`), not magic strings or ints, to express behavior in a table.

### Two-Layer Eligibility (copy / lock / remove / exclude)

Whether the record view / context menu offers copy, lock, remove, or exclude for a clicked row is decided by **composing two tables**, never by inline record/row-shape checks:

1. **Record-type policy** — `record_allows_copy` / `record_allows_lock` / `record_allows_exclude` on `record_behavior_t`. This says whether a record TYPE permits the operation at all (e.g. LAND sets `allows_copy = false`, `allows_lock = false`). Default is allowed.
2. **Row-kind capability** — `view_context_menu_t::caps_for(row_kind_t)` returns `{ can_copy, can_remove, can_lock }` for the SHAPE of the clicked row (sub_record / schema_record / group / field_of_schema / field_of_group / other). This says whether the operation is structurally possible for that row shape.

An action is offered only when **both** agree: `record_allows_X(rec_type) && caps_for(kind).can_X`. The menu builders (`build_lock_menu`, `build_merge_remove_menu`, `build_copy_to_merge_menu` / `build_source_copy_menu`, `add_exclude_record_action`, and the nav-menu whole-record lock/copy) read these accessors — they do NOT re-derive eligibility from `rec_type` literals or from `context.kind` switches. The lock SCOPE for a row still comes from `build_lock_for` (structural, by `row_kind_t`), and copy/remove PAYLOAD extraction (`resolve_schema_field` etc.) stays per-kind code — only the yes/no eligibility is table-driven.

To change what a record type permits, edit its `allows_*` fields. To change what a row shape permits, edit `caps_for`. Never gate an operation with a new `rec_type == "..."` or bare `context.kind ==` check in a menu builder.

### Why

Scattered type checks are the top source of regressions here: a new record type silently falls through the generic path, an exclusion added in one place is missed in another, and two copies of the same decision drift apart. A single typed table makes the behavior auditable in one file, keeps decode and merge in agreement, and makes adding a type a localized, reviewable change.

### Known Migration Debt

The dispatch is not yet fully table-driven. The following inline type checks predate this rule and are tracked for migration to the behavior table — do NOT copy them as a pattern, and do NOT add new ones:

- `sub_record_merge.cpp`: `merge()` dispatch on `"CELL"`/`"SCPT"`/`"ARMO"`/`"CLOT"`; `is_enam_record_type` (`"ENCH"`/`"SPEL"`/`"ALCH"`); the FACT `ANAM`/`INTV` skip in `apply_intermediate`; the FACT reaction phase keyed on `"FACT"`; the LEVI/CNAM sub-type choice in `build_merged_list_record`.
- `auto_merge.cpp`: `is_type_enabled` LAND skip; `dispatch_group` / `should_skip_group` routing on `"LEVI"`/`"LEVC"`/`"DIAL"`/`"INFO"`.
- `plugin_index.cpp`: `derive_id` and `derive_display_name` type dispatch (`"CELL"`, `"SKIL"`/`"MGEF"`, `"SCPT"`, `"DIAL"`, `"INFO"`, `"LAND"`, `"PGRD"`, `"GLOB"`, `"TES3"`).

Each should become a `decode_mode_t` route, a behavior-table flag, or a schema entry. New work in these files must move the decision into the table, not add another literal.

## Debug Logging

Do NOT remove `[debug]` diagnostic logs before committing. Keep them in the code, gated behind the global debug flag so they are silent by default and can be turned on when needed.

This overrides any earlier rule that said `[debug]` messages must be removed before commit. Diagnostics are valuable to keep — the fix is to gate them, not delete them.

### How to gate a debug log

`app_logger_t::add_log` already supports this via its `silent` parameter and the global `m_debug_flag`:

- `add_log(entry, /*silent=*/true)` — the entry is emitted ONLY when the debug flag is on (`app_logger_t::set_debug(true)`), and even then it goes to the in-memory log, not stdout.
- The global switch is `app_logger_t::set_debug(bool)` / `app_logger_t::is_debug()`.

So every `[debug]` line MUST be written as a debug-gated call:

```cpp
// Good — kept in the code, silent unless debug is enabled
app_logger_t::add_log("[debug] copy_field: patch_field failed for " + sub_type + "\r\n", true);
```

```cpp
// Bad — always prints, noise in normal runs
app_logger_t::add_log("[debug] copy_field: patch_field failed\r\n");
```

### Callers without app_logger

Some classes log through a `m_log` callback (e.g. `merge_controller_t`) rather than `app_logger_t` directly. Those callbacks always emit. When adding a `[debug]` line in such a class, route it through `app_logger_t::add_log(..., true)` (which the debug flag gates) rather than the always-on `m_log` callback, OR gate the `m_log` call behind `if (app_logger_t::is_debug())`. Either keeps the diagnostic in the code but off by default.

### Rules

- Prefix diagnostic messages with `[debug]` (lowercase), per the log-prefix convention.
- Gate every `[debug]` line behind the debug flag (`silent=true` on `app_logger_t::add_log`, or an `is_debug()` guard for callback loggers). Never leave an always-on `[debug]` line.
- Never delete a `[debug]` diagnostic to "clean up" before commit. Keep it, gated.
- Debug lines are developer-facing and stay in English (they are not user-visible; the always-update-docs and localization rules do not apply to them).
- The debug flag defaults to off, so gated `[debug]` lines produce no output in normal use and add no noise to the CLI or the GUI log.
