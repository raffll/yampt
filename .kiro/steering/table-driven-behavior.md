# Table-Driven Behavior — No Hardcoding, No Ad Hoc Exclusions

Record and sub-record behavior — how a type is decoded, merged, keyed, excluded, or labeled — is driven by declarative **decision tables and schemas**, never by inline type checks scattered through the logic. There is one authoritative table per concern; code reads the table and acts on the result.

## Authoritative Tables

- **`record_behavior.cpp` / `record_behavior_t`** — per record type, the single source of truth for how a record merges, decodes, and what the UI permits. Fields (all with defaults, set via designated initializers): `decode_mode_t` (generic, cell, leveled, faction, container, armor, info, dial), `copy_strategy_t`, `merge_strategy_t` (generic, armor_parts), `enam_effect_list`, `merge_excluded`, `leveled_item_sub_type`, `keyed_list_sub_types`, `record_id_sub_type`, `read_only_reason_t` (editable, landscape_data), `allows_copy` / `allows_lock` / `allows_exclude`, the `sub_record_rule_t` list (with `element_wise_merge` / `skip_merge` / `skip_*` flags), the wildcard rule, and `paired_merge_rule_t` pairs. Accessors: `decode_mode_for`, `merge_strategy_for`, `is_enam_effect_list`, `is_merge_excluded`, `leveled_item_sub_type_for`, `is_keyed_list_sub_type`, `record_id_sub_type_for`, `read_only_reason_for`, `record_allows_copy` / `record_allows_lock` / `record_allows_exclude`. Rows use designated initializers — a rule change is one named field on one `.record_type = "..."`-anchored row.
- **`sub_record_schema.cpp` / `field_def_t` + `sub_record_schema_t`** — per (record type, sub-type, size): the field layout used to decode and to merge field-by-field or bit-by-bit. `find_schema` / `find_largest_schema` / `find_cell_data_schema` are the only lookups.
- **`record_composition` / `record_sub_record_t`** — per record type: which sub-records exist and their kind (single / multi / repeatable).
- Name/index lookups (`effect_name_by_index`, `skill_name_by_index`, `global_type_name`, and similar) are tables too — extend the table, never inline a switch at the call site.

## Rules

- **No hardcoded record-type or sub-type string checks in logic.** Never write `if (rec_type == "CELL")` / `if (sub_type == "FLAG")` to choose behavior. Add or read a field on the behavior table (a `decode_mode_t`, a `merge_strategy_t`, or a `sub_record_rule_t` flag such as `skip_merge`) and branch on that. A dispatch decision must come from a table lookup, not a literal.
- **No ad hoc exclusions.** Never skip a type/sub-record with an inline literal (e.g. `if (rec_type == "LAND") return false;`, `if (rec_type == "FACT" && type == "ANAM") continue;`). Exclusions belong in a table: a rule flag on the behavior entry, an entry in the schema, or the user-facing `RECORD:SUB` ignore list — chosen by whichever table already owns that concern. If the right table has no place for the exclusion, add a typed field to that table, then set it declaratively.
- **No fallback/generic guess paths.** Consistent with the no-fallbacks rule: if a lookup misses, fix the table so the specific case matches — do not soften the lookup or add a catch-all branch. A genuinely unresolved case surfaces loudly, it is not silently substituted.
- **One table per concern.** Do not duplicate the same decision in two places (e.g. a decode_mode in the behavior table AND a parallel `if` chain in the merger). The code reads the table; the table is the decision.
- **Adding a new type or changing behavior is a table edit.** Introducing a record type, changing how one merges, excluding a sub-record, or relabeling a field is done by editing the relevant table/schema — not by adding a branch in the algorithm. If the table cannot express the new behavior, extend the table's schema (add a typed column/flag) first, then set it.
- **Tables are typed, not stringly.** Use enums and typed flags (`decode_mode_t`, `sub_rule_flag_t`, `field_type_t`), not magic strings or ints, to express behavior in a table.

## Two-Layer Eligibility (copy / lock / remove / exclude)

Whether the record view / context menu offers copy, lock, remove, or exclude for a clicked row is decided by **composing two tables**, never by inline record/row-shape checks:

1. **Record-type policy** — `record_allows_copy` / `record_allows_lock` / `record_allows_exclude` on `record_behavior_t`. This says whether a record TYPE permits the operation at all (e.g. LAND sets `allows_copy = false`, `allows_lock = false`). Default is allowed.
2. **Row-kind capability** — `view_context_menu_t::caps_for(row_kind_t)` returns `{ can_copy, can_remove, can_lock }` for the SHAPE of the clicked row (sub_record / schema_record / group / field_of_schema / field_of_group / other). This says whether the operation is structurally possible for that row shape.

An action is offered only when **both** agree: `record_allows_X(rec_type) && caps_for(kind).can_X`. The menu builders (`build_lock_menu`, `build_merge_remove_menu`, `build_copy_to_merge_menu` / `build_source_copy_menu`, `add_exclude_record_action`, and the nav-menu whole-record lock/copy) read these accessors — they do NOT re-derive eligibility from `rec_type` literals or from `context.kind` switches. The lock SCOPE for a row still comes from `build_lock_for` (structural, by `row_kind_t`), and copy/remove PAYLOAD extraction (`resolve_schema_field` etc.) stays per-kind code — only the yes/no eligibility is table-driven.

To change what a record type permits, edit its `allows_*` fields. To change what a row shape permits, edit `caps_for`. Never gate an operation with a new `rec_type == "..."` or bare `context.kind ==` check in a menu builder.

## Why

Scattered type checks are the top source of regressions here: a new record type silently falls through the generic path, an exclusion added in one place is missed in another, and two copies of the same decision drift apart. A single typed table makes the behavior auditable in one file, keeps decode and merge in agreement, and makes adding a type a localized, reviewable change.

## Known Migration Debt

The dispatch is not yet fully table-driven. The following inline type checks predate this rule and are tracked for migration to the behavior table — do NOT copy them as a pattern, and do NOT add new ones:

- `sub_record_merge.cpp`: `merge()` dispatch on `"CELL"`/`"SCPT"`/`"ARMO"`/`"CLOT"`; `is_enam_record_type` (`"ENCH"`/`"SPEL"`/`"ALCH"`); the FACT `ANAM`/`INTV` skip in `apply_intermediate`; the FACT reaction phase keyed on `"FACT"`; the LEVI/CNAM sub-type choice in `build_merged_list_record`.
- `auto_merge.cpp`: `is_type_enabled` LAND skip; `dispatch_group` / `should_skip_group` routing on `"LEVI"`/`"LEVC"`/`"DIAL"`/`"INFO"`.
- `plugin_index.cpp`: `derive_id` and `derive_display_name` type dispatch (`"CELL"`, `"SKIL"`/`"MGEF"`, `"SCPT"`, `"DIAL"`, `"INFO"`, `"LAND"`, `"PGRD"`, `"GLOB"`, `"TES3"`).

Each should become a `decode_mode_t` route, a behavior-table flag, or a schema entry. New work in these files must move the decision into the table, not add another literal.
