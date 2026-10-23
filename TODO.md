# TODO

## Medium

- advanced filters: locked only, excluded only — yEditor
- PNAM etc should not be taken from last?
- colors blend together / hard to distinguish — adjust the palette so states are easier to tell apart (user reported with screenshot) — yTranslator
- let the user customize the colors in settings (bigger task than just adjusting defaults) — yTranslator

## Hard

- Cross-workspace search window: search a phrase across all open dictionaries, results in a separate window with dictionary name column and highlighted match — yTranslator
- MO2 mode: resolve Kezyma OpenMW Player stub plugins — a `.omwaddon.esp` / `.omwscript.esp` double-extension file is an empty placeholder MO2 uses to put the real `.omwaddon`/`.omwscript` into the load order; yEditor loads the empty stub instead of the real file. In MO2 mode, detect the double extension and load the real file (strip the trailing `.esp`). — yEditor
- value changed on locked field doesn't survive merged patch — yEditor

## Hard to Fix

- editor diff off by couple characters
- NPDT 12 vs 52 merged patch

## Tech Debt (acknowledged — do not re-report in audits)

Size/line/nesting/argument-count/name-length limits are grandfathered for existing code (see code-style.md), so pure "over the limit" cases are not tracked here. These are genuine structural/behavioral debts:

- `translate_all_requested` handler in `main_window` holds orchestration logic — extract to a translation controller (Anti-Gravity Rule) — yTranslator
- Validation feedback diverges between yTranslator (informs, three-tier, own validator in yampt.translator) and yEditor (blocks, binary, validator in yampt.core) — contradicts Consistent Across Apps; needs a design decision (shared validation core, unified message wording, deliberate block-vs-inform per app) before implementation

### Table-driven migration debt (inline type checks predating the table-driven rule — migrate to the behavior table, do not copy or add more)

- `sub_record_merge.cpp`: `merge()` dispatch on `"CELL"`/`"SCPT"`/`"ARMO"`/`"CLOT"`; `is_enam_record_type` (`"ENCH"`/`"SPEL"`/`"ALCH"`); FACT `ANAM`/`INTV` skip in `apply_intermediate`; FACT reaction phase keyed on `"FACT"`; LEVI/CNAM sub-type choice in `build_merged_list_record`
- `auto_merge.cpp`: `is_type_enabled` LAND skip; `dispatch_group` / `should_skip_group` routing on `"LEVI"`/`"LEVC"`/`"DIAL"`/`"INFO"`
- `plugin_index.cpp`: `derive_id` and `derive_display_name` type dispatch (`"CELL"`, `"SKIL"`/`"MGEF"`, `"SCPT"`, `"DIAL"`, `"INFO"`, `"LAND"`, `"PGRD"`, `"GLOB"`, `"TES3"`)

## Known Limitations (not debt — won't fix)

- SCDT messagebox segment limited to 254 chars: the compiled-bytecode segment length is a 1-byte field in the vanilla Morrowind format, so a translated segment longer than 254 chars cannot be patched. Only affects the vanilla engine; OpenMW recompiles from the SCTX source and has no such limit. Not fixable without breaking the vanilla format.
