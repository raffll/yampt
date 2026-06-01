# Requirements Document

## Introduction

When creating base dictionaries from two language versions of Morrowind (e.g. English and German), yampt must match cells between the two ESM files to create translation pairs. Currently, yampt uses record order (same-order mode) or a fuzzy pattern-matching approach (unordered mode) based on cell DATA + object NAME concatenation. This is fragile — it produces MISSING entries when the pattern doesn't match due to minor differences between versions.

This feature replaces the fuzzy matching with deterministic matching based on stable identifiers: grid coordinates for exterior cells, and door marker destinations (coordinates + target cell references) for interior cells. These identifiers are language-independent and identical across all localized versions of the same ESM.

## Background: Cell Structure in TES3

From analysis of `Morrowind.json` (tes3conv output):

### Exterior Cells
```json
{
  "type": "Cell",
  "name": "",
  "data": { "flags": "HAS_WATER", "grid": [23, 7] },
  "region": "Azura's Coast Region",
  "references": []
}
```
- Identified by `grid` coordinates `[x, y]` — unique per exterior cell
- `name` is empty for exterior cells (display name comes from region)
- Grid coordinates are identical across all language versions

### Interior Cells
```json
{
  "type": "Cell",
  "name": "Abaelun Mine",
  "data": { "flags": "IS_INTERIOR", "grid": [0, 1065353216] },
  "references": [
    {
      "id": "door_cavern_doors10",
      "translation": [-1473.151, 2880.983, -416.082],
      "destination": {
        "translation": [-576.0, 29024.0, 1472.0],
        "rotation": [0.0, 0.0, 1.399754],
        "cell": ""
      }
    }
  ]
}
```
- `name` is the localized cell name (different per language — this is what we're trying to match)
- `grid` for interiors is meaningless (always `[0, garbage]`)
- **Door references with `destination`** are the stable identifier:
  - `destination.translation` — coordinates in the target cell (language-independent)
  - `destination.cell` — target cell name (empty = exterior, otherwise another interior)
  - The door object `id` is also language-independent (e.g. "door_cavern_doors10")
- **NPC/creature `id` fields** are also language-independent and can serve as secondary identifiers

### Why Current Matching Fails
The current unordered matching (`makeDictCELL_Unordered`) builds a "pattern" by concatenating DATA + all NAME subrecords from references. If any reference differs (added/removed by a patch, different order), the pattern doesn't match and the cell gets marked MISSING. The German Morrowind ESM has records in a different order than English, triggering this path.

## Glossary

- **Cell_Matcher**: The yampt component responsible for matching cells between two language versions of an ESM
- **Exterior_Cell**: A cell with grid coordinates and no interior flag — identified uniquely by its `[x, y]` grid position
- **Interior_Cell**: A cell with the IS_INTERIOR flag — identified by its door markers and reference fingerprint
- **Door_Marker**: A reference in a cell that has a `destination` field, indicating a teleport link to another cell
- **Reference_Fingerprint**: A set of language-independent identifiers (object IDs, coordinates) that uniquely identify an interior cell
- **Grid_Coordinates**: The `[x, y]` integer pair identifying an exterior cell's position in the world grid
- **Stable_Identifier**: Any data field that remains identical across all localized versions of the same ESM (coordinates, object IDs, grid positions)

## Requirements

### Requirement 1: Match Exterior Cells by Grid Coordinates

**User Story:** As a translator, I want exterior cells to be matched between language versions using their grid coordinates, so that matching is deterministic and never produces false MISSING entries for exterior cells.

#### Acceptance Criteria

1. WHEN creating a base dictionary from two ESM files with different record orders, THE Cell_Matcher SHALL match exterior cells by their `grid [x, y]` coordinates rather than by record position or pattern matching
2. WHEN two exterior cells from different ESM files share the same grid coordinates, THE Cell_Matcher SHALL pair them as a translation match
3. WHEN an exterior cell exists in one ESM but not the other (different grid coordinate sets), THE Cell_Matcher SHALL mark it as MISSING with a log message including the grid coordinates
4. THE Cell_Matcher SHALL complete exterior cell matching in O(n) time using a coordinate-indexed lookup rather than O(n²) pattern comparison

### Requirement 2: Match Interior Cells by Door Marker Fingerprint

**User Story:** As a translator, I want interior cells to be matched using their door marker destinations, so that cells with translated names are correctly paired even when record order differs.

#### Acceptance Criteria

1. WHEN creating a base dictionary from two ESM files, THE Cell_Matcher SHALL build a fingerprint for each interior cell based on its door marker destinations
2. THE Cell_Matcher SHALL construct the fingerprint from the set of `destination.translation` coordinates (rounded to integer) of all door references in the cell
3. WHEN two interior cells from different ESM files have matching door marker fingerprints, THE Cell_Matcher SHALL pair them as a translation match
4. WHEN an interior cell has no door markers, THE Cell_Matcher SHALL fall back to matching by reference object IDs (the `id` fields of non-temporary references)

### Requirement 3: Fallback Matching for Cells Without Doors or References

**User Story:** As a translator, I want cells that have no door markers and no unique references to still be matched using available stable data, so that no cells are unnecessarily marked as MISSING.

#### Acceptance Criteria

1. WHEN an interior cell has no door markers and no non-temporary references, THE Cell_Matcher SHALL attempt matching by the cell's atmosphere data (ambient color, sunlight color, fog density) as a combined fingerprint
2. IF atmosphere-based matching produces multiple candidates, THEN THE Cell_Matcher SHALL mark the cell as MISSING and log a warning listing the ambiguous candidates
3. THE Cell_Matcher SHALL report the total number of cells matched by each method (grid, door markers, reference IDs, atmosphere, unmatched) in the summary log

### Requirement 4: Reference ID-Based Secondary Matching

**User Story:** As a translator, I want NPC and creature IDs within a cell to serve as secondary matching criteria, so that cells with unique inhabitants can be matched even without door markers.

#### Acceptance Criteria

1. WHEN door marker fingerprint matching fails for an interior cell, THE Cell_Matcher SHALL build a secondary fingerprint from the sorted set of non-temporary reference `id` fields
2. WHEN two interior cells share the same set of reference IDs, THE Cell_Matcher SHALL pair them as a translation match
3. WHEN reference ID matching produces multiple candidates (common objects like "Imperial Guard" appearing in many cells), THE Cell_Matcher SHALL not use that match and fall back to the next matching strategy

### Requirement 5: Backward Compatibility

**User Story:** As a translator, I want the new matching to produce the same or better results than the current pattern-based matching, so that existing workflows are not broken.

#### Acceptance Criteria

1. FOR ALL cell pairs that the current pattern-based matcher correctly identifies, THE Cell_Matcher SHALL also correctly identify them using coordinate/fingerprint matching
2. THE Cell_Matcher SHALL reduce the number of MISSING entries compared to the current implementation when processing ESM files with different record orders
3. WHEN the `--make-base` command is used with two same-order ESM files, THE Cell_Matcher SHALL still use direct positional matching (no change to the fast path)

### Requirement 6: Same-Order Optimization

**User Story:** As a developer, I want the coordinate-based matching to only activate when record orders differ, so that the common case (same-order ESMs) remains fast.

#### Acceptance Criteria

1. WHEN `isSameOrder()` returns true, THE Cell_Matcher SHALL use direct positional matching (existing behavior, no fingerprinting overhead)
2. WHEN `isSameOrder()` returns false, THE Cell_Matcher SHALL use the coordinate/fingerprint-based matching strategy
3. THE Cell_Matcher SHALL log which matching strategy was selected

### Requirement 7: Coordinate Precision Handling

**User Story:** As a developer, I want coordinate comparison to handle floating-point precision correctly, so that minor rounding differences between versions do not prevent matching.

#### Acceptance Criteria

1. WHEN comparing door marker destination coordinates, THE Cell_Matcher SHALL round coordinates to the nearest integer before comparison
2. WHEN comparing reference translation coordinates for fingerprinting, THE Cell_Matcher SHALL round coordinates to the nearest integer
3. THE Cell_Matcher SHALL treat coordinates that differ by less than 1.0 unit as equal for matching purposes
