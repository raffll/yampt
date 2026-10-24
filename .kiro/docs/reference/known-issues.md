# Known Issues & Accepted Debt

Reference list of things that are deliberately NOT tracked as actionable TODO
items: hard format limitations that won't be fixed, and acknowledged tech debt
that must not be re-reported in audits or expanded. Actionable work lives in
`TODO.md`; this file is the "known, accepted, do not re-flag" record.

## Known Limitations (won't fix)

- **SCDT messagebox segment limited to 254 chars.** The compiled-bytecode segment length is a 1-byte field in the vanilla Morrowind format, so a translated segment longer than 254 chars cannot be patched. Only affects the vanilla engine; OpenMW recompiles from the SCTX source and has no such limit. Not fixable without breaking the vanilla format.

## Table-Driven Migration Debt

Inline type checks that predate the table-driven rule (see `.kiro/steering/engineering-discipline.md`). Do NOT copy them as a pattern and do NOT add new ones. Migrate opportunistically to the behavior table only when already editing that dispatch — not as a standalone sweep.

- `sub_record_merge.cpp`: `merge()` dispatch on `"CELL"`/`"SCPT"`/`"ARMO"`/`"CLOT"`; `is_enam_record_type` (`"ENCH"`/`"SPEL"`/`"ALCH"`); FACT `ANAM`/`INTV` skip in `apply_intermediate`; FACT reaction phase keyed on `"FACT"`; LEVI/CNAM sub-type choice in `build_merged_list_record`
- `auto_merge.cpp`: `is_type_enabled` LAND skip; `dispatch_group` / `should_skip_group` routing on `"LEVI"`/`"LEVC"`/`"DIAL"`/`"INFO"`
- `plugin_index.cpp`: `derive_id` and `derive_display_name` type dispatch (`"CELL"`, `"SKIL"`/`"MGEF"`, `"SCPT"`, `"DIAL"`, `"INFO"`, `"LAND"`, `"PGRD"`, `"GLOB"`, `"TES3"`)

## Grandfathered Style Limits

The size/line/nesting/argument-count/name-length limits and the classes-vs-namespaces rule apply to new and modified code only (see `.kiro/steering/code-style.md`). Existing code that predates them is grandfathered — not tracked as debt, not flagged in audits, fixed only on touch.
