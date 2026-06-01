# Implementation Plan: Make-Base Original Field

## Overview

Refactor DictCreator to: rename RecordEntry fields and JSON keys, unify insertion functions into `insertEntry`, build lookup indexes from `esm_ref` for Pattern 2 extractors, eliminate `is_make_mode` branches in extractors, update unordered variants to use `esm_ref` and pass correct `old_text`, unify orchestrators into single `makeDict()`, and remove dead code (`isSameOrder`, same-index pairing in Pattern 1 extractors).

## Tasks

- [x] 1. Rename RecordEntry fields and update JSON keys
  - [x] 1.1 Rename RecordEntry struct fields in tools.hpp
    - Rename `id` → `key_text`, `original` → `old_text`, `translation` → `new_text`
    - Update Chapter::insert and Chapter::find to use new field names
    - _Requirements: 13.4_

  - [x] 1.2 Update DictWriter JSON keys
    - Change `"original"` → `"old"`, `"translation"` → `"new"` in output
    - Keep `"id"` and `"status"` unchanged
    - _Requirements: 13.1, 13.2, 13.5_

  - [x] 1.3 Update DictReader JSON keys
    - Parse `"old"` and `"new"` instead of `"original"` and `"translation"`
    - _Requirements: 13.3_

  - [x] 1.4 Update all other files for renamed fields
    - DictMerger, EsmConverter, ScriptParser, DictCreator, all test files
    - Replace `.id` → `.key_text`, `.original` → `.old_text`, `.translation` → `.new_text`
    - _Requirements: 13.4_

- [x] 2. Checkpoint
  - Ensure all tests pass, ask the user if questions arise.

- [x] 3. Create unified `insertEntry` function
  - [x] 3.1 Implement `insertEntry(id, old_text, new_text, type)` in DictCreator
    - Make mode: set `entry.key_text = id`, `entry.old_text = old_text`, look up `base_dict[type].find(id)` for `new_text` and `status` (translated/auto_identical/changed/untranslated)
    - Base mode: set `entry.key_text = id`, `entry.old_text = old_text`, `entry.new_text = new_text`, `entry.status = ""`, handle DOUBLED duplicates (same id, different new_text → create doubled entry with same old_text)
    - Use early returns to avoid deep nesting
    - _Requirements: 14.1, 14.2, 14.3, 1.1, 1.2, 1.3_

  - [x] 3.2 Update all callers to use `insertEntry`
    - Replace calls to `insertRecord` and `insertRecordToDict` with `insertEntry`
    - Temporarily pass current values to preserve existing behavior
    - Remove old `insertRecord` and `insertRecordToDict` functions
    - _Requirements: 14.1_

- [ ] 4. Checkpoint
  - Ensure all tests pass, ask the user if questions arise.

- [x] 5. Build lookup indexes from `esm_ref`
  - [x] 5.1 Add `buildIndexes()` method and index member variables
    - Add `unordered_map<string, size_t>` members: `gmst_index`, `fnam_index`, `desc_index`, `text_index`, `rnam_index`, `indx_index`, `npc_flag_index`, `info_index`
    - Implement `buildIndexes()`: iterate `esm_ref` once, populate all indexes using composite key formats from design (GMST by NAME, FNAM by `rec_id^NAME`, DESC by `rec_id^NAME`, TEXT by NAME, RNAM by `NAME^counter`, INDX by `rec_id^INDX`, NPC_FLAG by NAME, INFO by `prefix^INAM`)
    - In make mode `esm_ref` = `esm`, so indexes point into native ESM; in base mode `esm_ref` = `esm_ext`, so indexes point into foreign ESM
    - _Requirements: 16.1, 16.2, 16.3, 16.5_

- [x] 6. Refactor Pattern 2 extractors — single loop, no `is_make_mode` branch
  - [x] 6.1 Refactor `makeDictGMST`
    - Single loop over `esm`: for each string GMST, look up `gmst_index` in `esm_ref`, read STRV for `old_text`
    - If index miss: fall back to `old_text = new_text`
    - Call `insertEntry(id, old_text, new_text, type)`
    - _Requirements: 9.1, 9.2, 15.1, 16.4_

  - [x] 6.2 Refactor `makeDictFNAM`
    - Same pattern: look up `fnam_index`, read FNAM from `esm_ref` for `old_text`
    - _Requirements: 2.1, 2.2, 15.1, 16.4_

  - [x] 6.3 Refactor `makeDictDESC`
    - Same pattern: look up `desc_index`, read DESC from `esm_ref` for `old_text`
    - _Requirements: 3.1, 3.2, 15.1, 16.4_

  - [x] 6.4 Refactor `makeDictTEXT`
    - Same pattern: look up `text_index`, read TEXT from `esm_ref` for `old_text`
    - _Requirements: 11.1, 15.1, 16.4_

  - [x] 6.5 Refactor `makeDictRNAM`
    - Same pattern: look up `rnam_index`, read RNAM from `esm_ref` for `old_text`
    - _Requirements: 4.1, 4.2, 15.1, 16.4_

  - [x] 6.6 Refactor `makeDictINDX`
    - Same pattern: look up `indx_index`, read DESC from `esm_ref` for `old_text`
    - _Requirements: 5.1, 5.2, 15.1, 16.4_

  - [x] 6.7 Refactor `makeDictNPC_FLAG`
    - Same pattern: look up `npc_flag_index`, read FLAG from `esm_ref`, derive gender for `old_text`
    - _Requirements: 11.2, 15.1, 16.4_

  - [x] 6.8 Refactor `makeDictINFO`
    - Same pattern: look up `info_index`, read NAME from `esm_ref` for `old_text`
    - _Requirements: 6.1, 6.2, 15.1, 16.4_

- [x] 7. Checkpoint
  - Ensure all tests pass, ask the user if questions arise.

- [x] 8. Refactor Pattern 1 extractors — make-mode only, remove base-mode branch
  - [x] 8.1 Simplify `makeDictCELL` to make-mode only
    - Remove the `else` branch (base-mode same-index pairing)
    - Keep only the make-mode loop: iterate `esm`, read NAME for `old_text`, call `insertEntry(id, old_text, "", type)`
    - Base mode handled entirely by `makeDictCELL_Unordered`
    - _Requirements: 8.1, 15.1, 22.4_

  - [x] 8.2 Simplify `makeDictCELL_Default` to make-mode only
    - Remove base-mode branch, keep make-mode loop
    - Base mode handled by `makeDictCELL_Unordered_Default`
    - _Requirements: 15.1, 22.4_

  - [x] 8.3 Simplify `makeDictCELL_REGN` to make-mode only
    - Remove base-mode branch, keep make-mode loop
    - Base mode handled by `makeDictCELL_Unordered_REGN`
    - _Requirements: 15.1, 22.4_

  - [x] 8.4 Simplify `makeDictDIAL` to make-mode only
    - Remove base-mode branch, keep make-mode loop
    - Base mode handled by `makeDictDIAL_Unordered`
    - _Requirements: 8.2, 15.1, 22.4_

  - [x] 8.5 Simplify `makeDictScript` to make-mode only
    - Remove base-mode branch, keep make-mode loop
    - Base mode handled by `makeDictScript_Unordered`
    - _Requirements: 7.1, 15.1, 22.4_

- [x] 9. Update unordered variants to use `esm_ref` and pass `old_text`
  - [x] 9.1 Update `makeDictCELL_Unordered` — replace `esm_ext` with `esm_ref`, pass foreign cell name as `old_text` to `insertEntry`
    - _Requirements: 10.1, 21.3_

  - [x] 9.2 Update `makeDictCELL_Unordered_Default` — replace `esm_ext` with `esm_ref`, pass foreign default cell name as `old_text`
    - _Requirements: 10.4, 21.3_

  - [x] 9.3 Update `makeDictCELL_Unordered_REGN` — replace `esm_ext` with `esm_ref`, pass foreign region FNAM as `old_text`
    - _Requirements: 10.5, 21.3_

  - [x] 9.4 Update `makeDictDIAL_Unordered` — replace `esm_ext` with `esm_ref`, pass foreign topic name as `old_text`
    - _Requirements: 10.2, 21.3_

  - [x] 9.5 Update `makeDictScript_Unordered` — replace `esm_ext` with `esm_ref`, pass foreign script key+message as `old_text`
    - _Requirements: 10.3, 21.3_

  - [x] 9.6 Update `makeDictCELL_Unordered_AddMissing` and `makeDictDIAL_Unordered_AddMissing` — replace `esm_ext` with `esm_ref`
    - _Requirements: 10.1, 10.2, 21.3_

  - [x] 9.7 Update all pattern/helper functions — replace `esm_ext` with `esm_ref`
    - `makeDictCELL_Unordered_PatternsExt`, `makeDictDIAL_Unordered_PatternsExt`, `makeDict_Unordered_PatternsExt`
    - _Requirements: 21.1, 21.2, 21.4_

  - [x] 9.8 Update `makeDictFNAM_Glossary` — replace `esm_ext` with `esm_ref`, pass foreign FNAM as `old_text`
    - _Requirements: 12.1, 12.2, 21.3_

- [x] 10. Checkpoint
  - Ensure all tests pass, ask the user if questions arise.

- [x] 11. Unify orchestrators into single `makeDict()`
  - [x] 11.1 Replace `makeDictForMake()` and `makeDictForBase()` with `makeDict()`
    - Call `buildIndexes()` first
    - Call Pattern 2 extractors (work for both modes via index + `esm_ref`)
    - In make mode: call Pattern 1 extractors (CELL, CELL_Default, CELL_REGN, DIAL, Script)
    - In base mode: call unordered variants (CELL_Unordered, CELL_Unordered_Default, CELL_Unordered_REGN, DIAL_Unordered, Script_Unordered)
    - In base mode: call `makeDictFNAM_Glossary()`
    - Remove `isSameOrder()` — always treat as unordered in base mode
    - _Requirements: 18.1, 18.2, 18.3, 18.4, 18.5, 22.1, 22.2, 22.3_

- [x] 12. Remove dead code and update declarations
  - [x] 12.1 Remove `isSameOrder()`, `makeDictForMake()`, `makeDictForBase()` declarations from header
    - Remove `same_order` member variable if present
    - _Requirements: 22.1, 18.1_

  - [x] 12.2 Remove any remaining old function bodies (`insertRecord`, `insertRecordToDict`) if not already removed in task 3.2
    - _Requirements: 14.1_

- [x] 13. Checkpoint
  - Ensure all tests pass, ask the user if questions arise.

- [x] 14. Update tests
  - [x] 14.1 Update tests.dictcreator.cpp for new `insertEntry` behavior
    - Verify `old_text` = foreign text in base mode entries
    - Verify `old_text` = native text in make mode entries
    - Verify DOUBLED entries preserve `old_text`
    - _Requirements: 14.2, 14.3, 1.2, 1.3, 20.2_

  - [x] 14.2 Update remaining test files for renamed fields and JSON keys
    - Ensure tests use `key_text`, `old_text`, `new_text` field names
    - Ensure tests expect `"old"` and `"new"` JSON keys
    - _Requirements: 13.1, 13.2, 13.3, 13.4_

  - [ ]* 14.3 Write property test for JSON round-trip (Property 1)
    - **Property 1: JSON serialization round-trip**
    - Generate arbitrary RecordEntry values, write via DictWriter, read via DictReader, verify all fields preserved
    - **Validates: Requirements 13.1, 13.2, 13.3, 13.5**

  - [ ]* 14.4 Write property test for insertEntry make mode (Property 2)
    - **Property 2: insertEntry make mode uses base_dict**
    - Generate arbitrary (id, old_text, new_text, type) inputs with a base_dict, verify new_text and status come from base_dict lookup
    - **Validates: Requirements 14.2, 20.4**

  - [ ]* 14.5 Write property test for insertEntry base mode (Property 3)
    - **Property 3: insertEntry base mode uses parameters directly**
    - Generate arbitrary inputs in base mode, verify entry fields match parameters exactly with empty status
    - **Validates: Requirements 1.1, 1.2, 14.3**

  - [ ]* 14.6 Write property test for DOUBLED entries (Property 5)
    - **Property 5: DOUBLED entries preserve old_text**
    - Generate duplicate insertions in base mode with different new_text, verify DOUBLED entry has correct old_text
    - **Validates: Requirements 1.3**

- [x] 15. Final checkpoint
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Pattern 1 (CELL, DIAL, Script): can't use key lookup (NAME is the translatable text). Make mode uses simple iteration. Base mode uses fingerprint/key matching via unordered variants.
- Pattern 2 (GMST, FNAM, DESC, TEXT, RNAM, INDX, NPC_FLAG, INFO): have stable internal keys. Single loop with index lookup works for both modes via `esm_ref`.
- `esm_ref` points to `esm` in make mode, `esm_ext` in base mode — all functions use `esm_ref`, never `esm_ext` directly.
- `is_make_mode` remains only for: `insertEntry` (base_dict lookup vs direct assignment) and orchestrator (which extractors to call for CELL/DIAL/Script).
- Tasks marked with `*` are optional and can be skipped for faster MVP.
- Per steering: never build or run tests, no std::regex, no deep nesting.

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1"] },
    { "id": 1, "tasks": ["1.2", "1.3", "1.4"] },
    { "id": 2, "tasks": ["3.1"] },
    { "id": 3, "tasks": ["3.2"] },
    { "id": 4, "tasks": ["5.1"] },
    { "id": 5, "tasks": ["6.1", "6.2", "6.3", "6.4", "6.5", "6.6", "6.7", "6.8"] },
    { "id": 6, "tasks": ["8.1", "8.2", "8.3", "8.4", "8.5"] },
    { "id": 7, "tasks": ["9.1", "9.2", "9.3", "9.4", "9.5", "9.6", "9.7", "9.8"] },
    { "id": 8, "tasks": ["11.1"] },
    { "id": 9, "tasks": ["12.1", "12.2"] },
    { "id": 10, "tasks": ["14.1", "14.2"] },
    { "id": 11, "tasks": ["14.3", "14.4", "14.5", "14.6"] }
  ]
}
```
