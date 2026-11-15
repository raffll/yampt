# Decision Tables

Concrete map of the authoritative decision tables that drive record/sub-record
behavior. The *rule* ("behavior is table-driven, never inline type checks") lives
in `.kiro/steering/engineering-discipline.md`; this doc is the knowledge of which
table owns which concern and what each exposes. Consult it before adding a record
type or changing how one decodes, merges, or is gated in the UI.

## Authoritative Tables

- **`record_behavior.cpp` / `record_behavior_t`** (in `yampt.core/source/utility/`) — per record type, the single source of truth for how a record merges, decodes, and what the UI permits. Fields (all with defaults, set via designated initializers): `decode_mode_t` (generic, cell, leveled, faction, container, armor, info, dial), `copy_strategy_t`, `merge_strategy_t` (generic, armor_parts), `enam_effect_list`, `merge_excluded`, `leveled_item_sub_type`, `keyed_list_sub_types`, `record_id_sub_type`, `read_only_reason_t` (editable, landscape_data), `allows_copy` / `allows_lock` / `allows_exclude`, the `sub_record_rule_t` list (with `element_wise_merge` / `skip_merge` / `skip_*` flags), the wildcard rule, and `paired_merge_rule_t` pairs. Accessors: `decode_mode_for`, `merge_strategy_for`, `is_enam_effect_list`, `is_merge_excluded`, `leveled_item_sub_type_for`, `is_keyed_list_sub_type`, `record_id_sub_type_for`, `read_only_reason_for`, `record_allows_copy` / `record_allows_lock` / `record_allows_exclude`. Rows use designated initializers — a rule change is one named field on one `.record_type = "..."`-anchored row.
- **`sub_record_schema.cpp` / `field_def_t` + `sub_record_schema_t`** — per (record type, sub-type, size): the field layout used to decode and to merge field-by-field or bit-by-bit. `find_schema` / `find_largest_schema` / `find_cell_data_schema` are the only lookups.
- **`record_composition` / `record_sub_record_t`** — per record type: which sub-records exist and their kind (single / multi / repeatable).
- Name/index lookups (`effect_name_by_index`, `skill_name_by_index`, `global_type_name`, and similar) are tables too — extend the table, never inline a switch at the call site.

## Two-Layer Eligibility (copy / lock / remove / exclude)

Whether the record view / context menu offers copy, lock, remove, or exclude for a clicked row is decided by **composing two tables**, never by inline record/row-shape checks:

1. **Record-type policy** — `record_allows_copy` / `record_allows_lock` / `record_allows_exclude` on `record_behavior_t`. This says whether a record TYPE permits the operation at all (e.g. LAND sets `allows_copy = false`, `allows_lock = false`). Default is allowed.
2. **Row-kind capability** — `view_context_menu_t::caps_for(row_kind_t)` returns `{ can_copy, can_remove, can_lock }` for the SHAPE of the clicked row (sub_record / schema_record / group / field_of_schema / field_of_group / other). This says whether the operation is structurally possible for that row shape.

An action is offered only when **both** agree: `record_allows_X(rec_type) && caps_for(kind).can_X`. The menu builders (`build_lock_menu`, `build_merge_remove_menu`, `build_copy_to_merge_menu` / `build_source_copy_menu`, `add_exclude_record_action`, and the nav-menu whole-record lock/copy) read these accessors — they do NOT re-derive eligibility from `rec_type` literals or from `context.kind` switches. The lock SCOPE for a row still comes from `build_lock_for` (structural, by `row_kind_t`), and copy/remove PAYLOAD extraction (`resolve_schema_field` etc.) stays per-kind code — only the yes/no eligibility is table-driven.

To change what a record type permits, edit its `allows_*` fields. To change what a row shape permits, edit `caps_for`. Never gate an operation with a new `rec_type == "..."` or bare `context.kind ==` check in a menu builder.

## Typed, Not Stringly

Use enums and typed flags (`decode_mode_t`, `sub_rule_flag_t`, `field_type_t`), not magic strings or ints, to express behavior in a table.

## Why

Scattered type checks are the top source of regressions here: a new record type silently falls through the generic path, an exclusion added in one place is missed in another, and two copies of the same decision drift apart. A single typed table makes the behavior auditable in one file, keeps decode and merge in agreement, and makes adding a type a localized, reviewable change.
