# Table-Driven Behavior — No Hardcoding, No Ad Hoc Exclusions

Record and sub-record behavior — how a type is decoded, merged, keyed, excluded, or labeled — is driven by declarative **decision tables and schemas**, never by inline type checks scattered through the logic. There is one authoritative table per concern; code reads the table and acts on the result.

## Authoritative Tables

- **`record_behavior.cpp` / `record_behavior_t`** — per record type: `decode_mode_t` (generic, cell, leveled, faction, container, armor, info, dial), `copy_strategy_t`, `atomic_groups`, the `sub_record_rule_t` list (with `element_wise_merge` / `skip_*` flags), the wildcard rule, and `paired_merge_rule_t` pairs. This is the single source of truth for how a record merges and how its sub-records are treated.
- **`sub_record_schema.cpp` / `field_def_t` + `sub_record_schema_t`** — per (record type, sub-type, size): the field layout used to decode and to merge field-by-field or bit-by-bit. `find_schema` / `find_largest_schema` / `find_cell_data_schema` are the only lookups.
- **`record_composition` / `record_sub_record_t`** — per record type: which sub-records exist and their kind (single / multi / repeatable).
- Name/index lookups (`effect_name_by_index`, `skill_name_by_index`, `global_type_name`, and similar) are tables too — extend the table, never inline a switch at the call site.

## Rules

- **No hardcoded record-type or sub-type string checks in logic.** Never write `if (rec_type == "CELL")` / `if (sub_type == "FLAG")` to choose behavior. Add or read a field on the behavior table (a `decode_mode_t`, a flag like `atomic_groups`, or a `sub_record_rule_t`) and branch on that. A dispatch decision must come from a table lookup, not a literal.
- **No ad hoc exclusions.** Never skip a type/sub-record with an inline literal (e.g. `if (rec_type == "LAND") return false;`, `if (rec_type == "FACT" && type == "ANAM") continue;`). Exclusions belong in a table: a rule flag on the behavior entry, an entry in the schema, or the user-facing `RECORD:SUB` ignore list — chosen by whichever table already owns that concern. If the right table has no place for the exclusion, add a typed field to that table, then set it declaratively.
- **No fallback/generic guess paths.** Consistent with the no-fallbacks rule: if a lookup misses, fix the table so the specific case matches — do not soften the lookup or add a catch-all branch. A genuinely unresolved case surfaces loudly, it is not silently substituted.
- **One table per concern.** Do not duplicate the same decision in two places (e.g. a decode_mode in the behavior table AND a parallel `if` chain in the merger). The code reads the table; the table is the decision.
- **Adding a new type or changing behavior is a table edit.** Introducing a record type, changing how one merges, excluding a sub-record, or relabeling a field is done by editing the relevant table/schema — not by adding a branch in the algorithm. If the table cannot express the new behavior, extend the table's schema (add a typed column/flag) first, then set it.
- **Tables are typed, not stringly.** Use enums and typed flags (`decode_mode_t`, `sub_rule_flag_t`, `field_type_t`), not magic strings or ints, to express behavior in a table.

## Why

Scattered type checks are the top source of regressions here: a new record type silently falls through the generic path, an exclusion added in one place is missed in another, and two copies of the same decision drift apart. A single typed table makes the behavior auditable in one file, keeps decode and merge in agreement, and makes adding a type a localized, reviewable change.

## Known Migration Debt

The dispatch is not yet fully table-driven. The following inline type checks predate this rule and are tracked for migration to the behavior table — do NOT copy them as a pattern, and do NOT add new ones:

- `sub_record_merge.cpp`: `merge()` dispatch on `"CELL"`/`"SCPT"`/`"ARMO"`/`"CLOT"`; `is_enam_record_type` (`"ENCH"`/`"SPEL"`/`"ALCH"`); the FACT `ANAM`/`INTV` skip in `apply_intermediate`; the FACT reaction phase keyed on `"FACT"`; the LEVI/CNAM sub-type choice in `build_merged_list_record`.
- `auto_merge.cpp`: `is_type_enabled` LAND skip; `dispatch_group` / `should_skip_group` routing on `"LEVI"`/`"LEVC"`/`"DIAL"`/`"INFO"`.
- `plugin_index.cpp`: `derive_id` and `derive_display_name` type dispatch (`"CELL"`, `"SKIL"`/`"MGEF"`, `"SCPT"`, `"DIAL"`, `"INFO"`, `"LAND"`, `"PGRD"`, `"GLOB"`, `"TES3"`).

Each should become a `decode_mode_t` route, a behavior-table flag, or a schema entry. New work in these files must move the decision into the table, not add another literal.
