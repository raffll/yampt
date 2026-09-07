# Design — Derived Field Protection

## Overview

Introduce a **system-derived exclusion tier**: a fixed, code-defined set of derived fields
that are non-editable, excluded from merge, non-lockable, shown greyed and non-toggleable in
the exclusion UIs, and recalculated when the merged patch is built. The tier reuses the
existing user-exclusion presentation (`is_ignored` → `conflict_this_t::ignored` greying) and
the existing merge-skip (`filter_sub_records_by_rules`), and adds a new per-record recompute
pass.

## Current State (verified in code)

- **User exclusion** is stored in `settings_store_t::sub_record_ignore_conflict()`
  (`SubRecordRules/IgnoreConflict`, default `CELL:NAM0`), parsed as comma-separated
  `REC:SUB` / `REC:*` rules.
- **Greying**: `view_tree_decode.cpp` sets `row.is_ignored = true` and
  `cell_conflict_this = conflict_this_t::ignored` for user-ignored rows; the theme maps that
  to a grey foreground.
- **Merge skip**: `merge_controller_t::create_merge_records` builds
  `merge_config_t.ignored_sub_records` from the user rules; `sub_record_merge_t::filter_sub_records_by_rules`
  drops them before merging (preserving FRMR groups).
- **Context menu**: `view_context_menu_t::build_sub_record_ignore_menu` adds an
  Include/Exclude action and `toggle_ignore_rule` writes the setting; a wildcard-covered rule
  is already shown disabled.
- **Settings list**: `merge_settings_view.cpp` lists the rules and lets the user edit them.
- **SCHD schema**: `sub_record_schema.cpp` `scpt_schd_fields` exposes Num Shorts, Num Longs,
  Num Floats, Script Data Size, Local Var Size as editable `u32` fields.
- **SCVR (script)**: SCPT `SCVR` is decoded as a single binary blob (`land_binary_fields`),
  NOT parsed into typed short/long/float variable blocks. This is why the three Num* counts
  cannot be reliably recomputed and are carried unchanged.
- **SCDT patching** (converter only): `scdt_patcher_t` recomputes inline text length,
  getpccell expr size, and messagebox segment lengths; `esm_converter_t` rewrites SCHD Script
  Data Size after patching. The editor never edits SCDT.
- **Record body Size / sub-record Size** are recomputed on every `reconstruct_record` /
  `reconstruct_cell` / `reconstruct_armor` and in `convert_record_content` — out of scope,
  must not regress.

## Architecture

### Component 1 — `derived_fields` (new, yampt.core)

`yampt.core/source/utility/derived_fields.hpp/.cpp` — a namespace (no mutable state) holding
the fixed table and queries.

```cpp
namespace derived_fields
{
    struct derived_entry_t
    {
        std::string_view record_type;
        std::string_view sub_type;
        int field_index; // -1 = whole sub-record
    };

    bool is_derived_sub_record(const std::string & record_type, const std::string & sub_type);
    bool is_derived_field(const std::string & record_type, const std::string & sub_type, int field_index);
    std::vector<derived_entry_t> all_entries();
}
```

Fixed table:

| record | sub | field_index | meaning |
|--------|-----|-------------|---------|
| SCPT | SCDT | -1 | compiled bytecode (whole) |
| SCPT | SCHD | 1 | Num Shorts |
| SCPT | SCHD | 2 | Num Longs |
| SCPT | SCHD | 3 | Num Floats |
| SCPT | SCHD | 4 | Script Data Size |
| SCPT | SCHD | 5 | Local Var Size |
| LEVI | INDX | -1 | item count (whole) |
| LEVC | INDX | -1 | item count (whole) |

(SCHD field indices match `scpt_schd_fields`: 0 = Name, 1..5 = the five counts/sizes.)

### Component 2 — `derived_recompute` (new, yampt.core)

`yampt.core/source/scanner/derived_recompute.hpp/.cpp` — a class with a pure static entry
point so it is unit-testable without infrastructure:

```cpp
class derived_recompute_t
{
public:
    // Returns the record with all derivable derived fields recomputed from its own data.
    // Idempotent. Unknown/uncomputable fields are left unchanged.
    static std::string recompute(const std::string & record_type, const std::string & content);
};
```

Recompute rules:
- SCPT: parse sub-records; set SCHD Script Data Size (offset 44, u32) = SCDT data length; set
  SCHD Local Var Size (offset 48, u32) = SCVR data length. Num Shorts/Longs/Floats left as-is
  (not derivable — see below). Rebuild via `reconstruct_record` (recomputes body/sub sizes).
- LEVI/LEVC: set INDX (first 4 bytes, u32) = number of `INAM`/`CNAM` list items. (This mirrors
  the count `leveled_list_merge_t` already produces; the pass normalizes copied records.)
- Any other record type: returned unchanged.

Idempotency: recompute reads only the authoritative sub-records and writes the counts that
match them; running twice yields identical bytes.

**Num Shorts/Longs/Floats:** the editor does not parse the SCVR typed variable table, and
yampt never adds or removes script variables, so these three are carried unchanged (still
non-editable, excluded, non-lockable). Documented as a deliberate limitation.

### Component 3 — Record view greying + non-editable

`view_tree_decode*.cpp`: when building a sub-record row or a schema field row, OR the
existing user-ignore condition with
`derived_fields::is_derived_sub_record(...)` / `is_derived_field(...)`. A derived row/field
is marked `is_ignored` (greyed) exactly like a user-excluded one.

Editability: the record view already computes `Qt::ItemIsEditable` per cell. Add a derived
check so a derived field/sub-record never gets the editable flag.
`field_edit_controller_t::commit_field_edit` adds a defensive early return rejecting an edit
to a derived field, with a status-bar reason.

### Component 4 — Merge exclusion

`merge_controller_t::create_merge_records`: after building `ignored_sub_records` from user
rules, union in the system-derived whole-sub-record entries (`SCPT:SCDT`, `LEVI:INDX`,
`LEVC:INDX`). Field-level SCHD entries are not sub-record rules; they are protected by the
recompute pass and non-editability, not by merge-skip (SCHD as a whole still merges normally,
then the recompute pass fixes the derived fields).

### Component 5 — Lock refusal

`view_context_menu_t::build_lock_for` / `build_lock_menu`: if the lock target IS a derived
sub-record or derived field (not a whole-record lock), mark it unlockable so the action is
greyed (reusing the existing greyed-when-invalid path). Whole-record locks remain allowed for
records that merely contain derived fields.

### Component 6 — Exclusion UIs greyed/non-toggleable

- `build_sub_record_ignore_menu`: if `is_derived_sub_record`/`is_derived_field`, add the
  Include/Exclude action `setEnabled(false)` and do not wire the toggle.
- `merge_settings_view`: append system-derived entries as disabled list items (not selectable
  for removal), separated from the editable user rules; never serialize them into the setting.

### Component 7 — Recompute pass invocation

`merge_controller_t::create_merge_records`: after `reapply_locks()`, iterate the merged
records and replace each with `derived_recompute_t::recompute(rec_type, content)` via
`copy_record_to_merge_raw`. This runs once per merge build, after both auto-merge and lock
re-apply, satisfying idempotency for locked records.

## Data Flow

```
build merged patch
  ├─ auto_merge (skips user + system-derived sub-records)
  ├─ reapply_locks (whole/partial locks restored)
  └─ derived_recompute pass  ← NEW: fix SCHD sizes, LEVI/LEVC INDX
        └─ write back via copy_record_to_merge_raw
```

## Testing Strategy

Unit tests (in-memory, `[u]`):
- `derived_fields::is_derived_sub_record` / `is_derived_field` — true/false cases for each
  entry and negatives.
- `derived_recompute_t::recompute`:
  - SCPT with wrong SCHD Script Data Size / Local Var Size → corrected to SCDT / SCVR length.
  - SCPT already correct → unchanged (idempotency).
  - LEVI/LEVC with wrong INDX → corrected to item count.
  - Non-derived record → returned byte-for-byte identical.
  - Running recompute twice → identical to running once.

Use the synthetic-ESM record builders already present in `tests.sub_record_merge.cpp`
(`make_record`, `make_sub`, `make_uint32`).

## Non-Goals

- Parsing the SCPT SCVR typed variable table to recompute Num Shorts/Longs/Floats.
- Editing or recompiling MWScript source (SCTX) / bytecode (SCDT).
- Recomputing CELL NAM0, FRMR indices, or INFO PNAM/NNAM links (deliberately verbatim).

## Affected Files

New:
- `yampt.core/source/utility/derived_fields.hpp/.cpp`
- `yampt.core/source/scanner/derived_recompute.hpp/.cpp`
- `yampt.tests/source/tests.derived_fields.cpp`
- `yampt.tests/source/tests.derived_recompute.cpp`

Modified:
- `yampt.editor/source/model/view_tree_decode.cpp`, `view_tree_decode_cell.cpp` — greying +
  non-editable for derived.
- `yampt.editor/source/controller/field_edit_controller.cpp` — reject derived edits.
- `yampt.editor/source/controller/view_context_menu.cpp` — greyed non-toggleable exclude;
  refuse lock on derived target.
- `yampt.editor/source/dialog/settings/merge_settings_view.cpp` — show derived entries
  disabled.
- `yampt.editor/source/controller/merge_controller.cpp` — union system-derived into
  ignored_sub_records; invoke recompute pass.
- vcxproj/filters for the new core + test files.
- `docs/yEditor-Manual.md`, `CHANGELOG.md`.
