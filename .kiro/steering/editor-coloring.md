# Editor Panel Coloring — xEdit Specification

Exact reproduction of xEdit's (TES3Edit/TES5Edit) coloring behavior in both panels.

## Rule: Color Changes Apply Everywhere

When changing a color for a conflict status, apply it consistently to ALL places that status appears: left panel (nav tree — record, group, file nodes) and right panel (view tree). Never change a color in one panel without updating the other. Parent nodes always inherit the worst-case color from their children — same rules at every level.

## Rule: No Broad Color Changes

When fixing a coloring issue, only change the color/background for the specific case that is wrong. Never change the logic for all rows or all columns to fix one scenario. Guard every color change with a condition that targets only the affected case (e.g. single-column, specific conflict_this value, specific row type). If a fix would affect other cases, add a narrower condition rather than broadening an existing one.

## Color Palette

### ConflictAll Colors (Background — lightened 85%)

Raw colors before lightening:

| Enum | Raw Color | RGB |
|------|-----------|-----|
| unknown | (no paint) | — |
| only_one | (no paint) | — |
| no_conflict | lime | (0, 255, 0) |
| override_benign | yellow | (255, 255, 0) |
| conflict | red | (255, 0, 0) |

Background is ONLY painted when `conflict_all >= no_conflict`. For `unknown` and `only_one`, no background is painted (stays default white/system color).

### ConflictThis Colors (Foreground — full intensity text)

| Enum | Color | RGB |
|------|-------|-----|
| unknown | black | (0, 0, 0) |
| master | purple | (128, 0, 128) |
| identical_to_master | dark gray | (128, 128, 128) |
| override_wins | green | (0, 128, 0) |
| conflict_wins | orange | (255, 165, 0) |
| conflict_loses | red | (255, 0, 0) |
| deleted | dark gray | (128, 128, 128) |

### Lighter Function

`lighter(color, amount)` moves lightness in HSL toward white:
```
l = l + (1.0 - l) * amount
```
Standard factor is 0.85. Empty cells use 0.93. Focused column uses 0.81.

## Navigation Tree (Left Panel)

### Background

- Scope: **entire row** (all columns same background)
- Source: `conflict_all` per node (single value, not per-column)
- Inheritance: parent nodes (file, type group) take the **maximum** `conflict_all` from all their children
- Paint condition: ONLY when `conflict_all >= no_conflict`. For `unknown`/`only_one` → no paint (white)
- Color: `lighter(conflict_all_raw_color, 0.85)`

### Foreground (Text)

- Scope: **entire row** (all columns same text color)
- Source: `conflict_this` per node (single value)
- Inheritance: parent nodes take the **maximum** `conflict_this` from all their children
- Color: `conflict_this_foreground(conflict_this)` — full intensity
- For nodes with `conflict_this == unknown` → black (clWindowText)

### Key Behaviors

- File nodes and type group nodes DO get colored if their children have conflicts (via inheritance)
- Nodes with `only_one` (single plugin defines it) stay white background + black text
- Font style: bold if modified, italic if injected, strikeout if not reachable

## View Tree (Right Panel)

### Background — Two-Layer System

**Layer 1: Row background** (`BeforeItemErase`):
- Scope: entire row
- Source: `conflict_all` of the row (same value across all columns — it's the max of all per-column values, then assigned back to all)
- Paint condition: only when `conflict_all >= no_conflict`
- Color: `lighter(conflict_all_raw_color, 0.85)`
- This is the BASE background for the entire row

**Layer 2: Per-cell override** (`BeforeCellPaint`):
- ONLY triggers for special cells, not normal data cells
- Trigger conditions (OR):
  - Cell has NO element/value (empty cell)
  - Cell is in the actively focused column
- Factor adjustments from base 0.85:
  - Empty cell: +0.08 (becomes 0.93 → even lighter/more washed out)
  - Focused column: -0.04 (becomes 0.81 → slightly darker for highlight)
  - Empty + focused: +0.08 -0.04 + 0.015 = net +0.055 (becomes 0.905)
- Color: `lighter(conflict_all_raw_color, adjusted_factor)`
- Normal data cells with values get ONLY the row-level background (layer 1)

### Foreground (Text)

- Scope: **per-cell** (each column gets its own color)
- Source for plugin columns (col >= 1): `conflict_this` of that specific column's version
- Source for label column (col 0, the tree column): **maximum** `conflict_this` across ALL plugin columns
- Color: `conflict_this_foreground(conflict_this)`

### ConflictAll Is Per-Row, Not Per-Cell

Critical: `conflict_all` in the view tree is the SAME for all columns in a row. The computation:
1. Each leaf node computes ConflictAll from comparing element SortKeys across columns
2. Parent nodes inherit the max ConflictAll from children
3. After inheritance, the max is assigned back to ALL columns: `aNodeDatas[i].ConflictAll := ConflictAll`

So the background is uniform across the row. Cell differentiation is done through TEXT COLOR only.

### ConflictThis Per-Column Computation (Leaf Nodes)

For each sub-record/element row with N plugin columns:
- Column 0 (master/first plugin): `ctMaster` (or `ctUnknown` if element not assigned)
- Other columns where element's SortKey matches first element's SortKey: `ctIdenticalToMaster`
- Last column where SortKey matches last element (wins by load order): `ctConflictWins`
- Other columns with different SortKey: `ctConflictLoses`
- Element not assigned in a column: `ctNotDefined` (medium gray text)
- Conflict priority is `cpIgnore`: `ctIgnored`
- If only the last column differs and overall is `caOverride`: last column → `ctOverride` (green)

### ConflictAll Computation (Leaf Nodes)

Based on unique SortKey count across all columns (ignoring cpIgnore elements):
- 0 or 1 unique values: `caNoConflict`
- 2 unique values: `caOverride` if master differs from last (simple override), else `caConflict`
- 3+ unique values: `caConflict`

Modified by conflict priority:
- `cpBenign` priority: caps result at `caConflictBenign`
- `cpCritical` priority (injected records): escalates to `caConflictCritical`

## Implementation Notes

- `conflict_all` for our simplified implementation (binary comparison): use the existing `conflict_all_t` from `plugin_scan_t` for the whole record. Sub-record rows can refine it by checking if their individual values differ.
- For sub-record rows: if all columns have the same formatted value → `no_conflict`. If only the last differs → `override_benign`. If multiple differ → `conflict`.
- `conflict_this` per column: use the version's `conflict_this_t` from `plugin_scan_t` for all rows of that record. xEdit computes per-sub-record, but for our simplified model, using the record-level conflict_this per column is acceptable.
- The label column (col 0) never gets the per-cell background override — it always gets the row-level background.
