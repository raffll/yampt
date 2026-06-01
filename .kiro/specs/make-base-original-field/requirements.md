# Requirements Document

## Introduction

The `--make-base` command produces a JSON dictionary by pairing records from a native ESM (e.g. Polish) with a foreign ESM (e.g. English). Currently, `insertRecordToDict` sets the `original` field to the same value as `id` (the composite lookup key). For record types where the key is a composite (FNAM, DESC, INDX, RNAM, INFO, BNAM, SCTX), this is incorrect — the `original` field should contain the actual foreign text, not the composite key. For CELL and DIAL, the key already equals the foreign text, so the current behavior happens to be correct.

This feature adds a third parameter to `insertRecordToDict` so that callers can pass the actual foreign text separately from the composite key.

## Glossary

- **DictCreator**: The class responsible for building translation dictionaries from ESM files
- **insertRecordToDict**: The method that inserts a single record into the dictionary during `--make-base` operation
- **esm**: The EsmReader instance holding the native (first argument) ESM file (e.g. Polish)
- **esm_ext**: The EsmReader instance holding the foreign (second argument) ESM file (e.g. English). In `--make-base` mode, `esm_ref` is a reference to `esm_ext`
- **esm_ref**: A reference that points to `esm_ext` in base mode. Used by same-order callers to access the foreign ESM
- **Composite_Key**: A lookup key formed by concatenating record type, record name, and/or subrecord identifiers with `^` separators (e.g. `"ACTI^A_Ex_De_Oar"` for FNAM, `"T^topic^INFO_ID"` for INFO)
- **Foreign_Text**: The actual text content read from the foreign ESM's subrecord (e.g. `"Oar"` for an FNAM, `"Some dialog line"` for INFO)
- **RecordEntry**: The struct with fields `id`, `original`, `translation`, `status` that represents one dictionary entry

## Requirements

### Requirement 1: Accept foreign text parameter

**User Story:** As a dictionary creator, I want `insertRecordToDict` to accept the foreign text as a separate parameter, so that the `original` field can be populated independently from the `id` field.

#### Acceptance Criteria

1. WHEN `insertRecordToDict` is called, THE DictCreator SHALL accept a fourth parameter representing the foreign text
2. WHEN a record is inserted into the dictionary, THE DictCreator SHALL set `entry.original` to the foreign text parameter value
3. WHEN a doubled record is created, THE DictCreator SHALL set `doubled_entry.original` to the foreign text parameter value

### Requirement 2: Pass foreign text from FNAM callers

**User Story:** As a translator, I want the FNAM dictionary entries to contain the actual foreign name in the `original` field, so that I can see what the English name is without parsing the composite key.

#### Acceptance Criteria

1. WHEN `makeDictFNAM` runs in base mode, THE DictCreator SHALL look up the matching record in esm_ext by record type and NAME key, read its FNAM subrecord value, and pass it as the foreign text to `insertRecordToDict`
2. WHEN a FNAM entry is written to JSON, THE DictCreator SHALL output `original` as the foreign FNAM value (e.g. `"Oar"`) rather than the composite key (e.g. `"ACTI^A_Ex_De_Oar"`)

### Requirement 3: Pass foreign text from DESC callers

**User Story:** As a translator, I want the DESC dictionary entries to contain the actual foreign description in the `original` field, so that I can see the English description text.

#### Acceptance Criteria

1. WHEN `makeDictDESC` runs in base mode, THE DictCreator SHALL look up the matching record in esm_ext by record type and NAME key, read its DESC subrecord value, and pass it as the foreign text to `insertRecordToDict`
2. WHEN a DESC entry is written to JSON, THE DictCreator SHALL output `original` as the foreign DESC value rather than the composite key

### Requirement 4: Pass foreign text from RNAM callers

**User Story:** As a translator, I want the RNAM dictionary entries to contain the actual foreign rank name in the `original` field, so that I can see the English rank name.

#### Acceptance Criteria

1. WHEN `makeDictRNAM` runs in base mode, THE DictCreator SHALL look up the matching FACT record in esm_ext by NAME key, read its RNAM subrecord values, and pass each as the foreign text to `insertRecordToDict`
2. WHEN a RNAM entry is written to JSON, THE DictCreator SHALL output `original` as the foreign RNAM value rather than the composite key

### Requirement 5: Pass foreign text from INDX callers

**User Story:** As a translator, I want the INDX dictionary entries to contain the actual foreign description in the `original` field, so that I can see the English skill/magic effect description.

#### Acceptance Criteria

1. WHEN `makeDictINDX` runs in base mode, THE DictCreator SHALL look up the matching SKIL or MGEF record in esm_ext by INDX key, read its DESC subrecord value, and pass it as the foreign text to `insertRecordToDict`
2. WHEN an INDX entry is written to JSON, THE DictCreator SHALL output `original` as the foreign DESC value rather than the composite key

### Requirement 6: Pass foreign text from INFO callers

**User Story:** As a translator, I want the INFO dictionary entries to contain the actual foreign dialog text in the `original` field, so that I can see the English dialog response.

#### Acceptance Criteria

1. WHEN `makeDictINFO` runs in base mode, THE DictCreator SHALL look up the matching INFO record in esm_ext by INAM key, read its NAME subrecord value, and pass it as the foreign text to `insertRecordToDict`
2. WHEN an INFO entry is written to JSON, THE DictCreator SHALL output `original` as the foreign dialog text rather than the composite key

### Requirement 7: Pass foreign text from script callers (BNAM, SCTX)

**User Story:** As a translator, I want the BNAM and SCTX dictionary entries to contain the actual foreign script line in the `original` field, so that I can see the English script message.

#### Acceptance Criteria

1. WHEN `makeDictScript` runs in base mode (same order), THE DictCreator SHALL construct the foreign text from esm_ref's key and message content and pass it as the foreign text to `insertRecordToDict`
2. WHEN a BNAM or SCTX entry is written to JSON, THE DictCreator SHALL output `original` as the foreign script key and message (e.g. `"ScriptName^MessageBox \"English text\""`) rather than duplicating the composite key from the id field

### Requirement 8: Preserve correct behavior for CELL and DIAL

**User Story:** As a translator, I want CELL and DIAL entries to continue working correctly, so that existing behavior is not broken.

#### Acceptance Criteria

1. WHEN `makeDictCELL` runs in base mode, THE DictCreator SHALL pass the foreign cell name (which equals the id) as the foreign text to `insertRecordToDict`
2. WHEN `makeDictDIAL` runs in base mode, THE DictCreator SHALL pass the foreign topic name (which equals the id) as the foreign text to `insertRecordToDict`
3. WHEN CELL or DIAL entries are written to JSON, THE DictCreator SHALL output `original` equal to `id` (preserving current behavior)

### Requirement 9: Pass foreign text from GMST callers

**User Story:** As a translator, I want the GMST dictionary entries to contain the actual foreign game setting string in the `original` field, so that I can see the English game setting text.

#### Acceptance Criteria

1. WHEN `makeDictGMST` runs in base mode, THE DictCreator SHALL look up the matching GMST record in esm_ext by NAME key, read its STRV subrecord value, and pass it as the foreign text to `insertRecordToDict`
2. WHEN a GMST entry is written to JSON, THE DictCreator SHALL output `original` as the foreign STRV value rather than the GMST key name (e.g. `"sDefaultCellname"`)

### Requirement 10: Pass foreign text from unordered callers

**User Story:** As a translator, I want the unordered CELL, DIAL, and script dictionary entries to also contain the correct foreign text in the `original` field, so that behavior is consistent regardless of record ordering.

#### Acceptance Criteria

1. WHEN `makeDictCELL_Unordered` runs, THE DictCreator SHALL pass the foreign cell name (read from esm_ext at the matched position) as the foreign text to `insertRecordToDict`
2. WHEN `makeDictDIAL_Unordered` runs, THE DictCreator SHALL pass the foreign topic name (read from esm_ext at the matched position) as the foreign text to `insertRecordToDict`
3. WHEN `makeDictScript_Unordered` runs, THE DictCreator SHALL pass the foreign script key and message (read from esm_ext at the matched position) as the foreign text to `insertRecordToDict`
4. WHEN `makeDictCELL_Unordered_Default` runs, THE DictCreator SHALL look up the matching GMST record in esm_ext and pass the foreign default cell name as the foreign text to `insertRecordToDict`
5. WHEN `makeDictCELL_Unordered_REGN` runs, THE DictCreator SHALL look up the matching REGN record in esm_ext by NAME key and pass the foreign region FNAM as the foreign text to `insertRecordToDict`

### Requirement 11: Pass foreign text from TEXT and NPC_FLAG callers

**User Story:** As a translator, I want TEXT and NPC_FLAG entries to also have the correct `original` field, so that all record types are handled consistently.

#### Acceptance Criteria

1. WHEN `makeDictTEXT` runs in base mode, THE DictCreator SHALL look up the matching BOOK record in esm_ext by NAME key, read its TEXT subrecord value, and pass it as the foreign text to `insertRecordToDict`
2. WHEN `makeDictNPC_FLAG` runs in base mode, THE DictCreator SHALL look up the matching NPC_ record in esm_ext by NAME key, read its FLAG subrecord value, and pass the derived gender flag as the foreign text to `insertRecordToDict`

### Requirement 12: Pass foreign text from Glossary caller

**User Story:** As a translator, I want the Glossary entries to retain the foreign FNAM value as the `original` field, so that the glossary lookup key matches the foreign display name.

#### Acceptance Criteria

1. WHEN `makeDictFNAM_Glossary` runs, THE DictCreator SHALL pass the foreign FNAM value as the foreign text to `insertRecordToDict`
2. WHEN a Glossary entry is written to JSON, THE DictCreator SHALL output `original` as the foreign FNAM value (which already equals the id for Glossary entries)

### Requirement 13: Rename JSON keys to `old` and `new`

**User Story:** As a translator, I want the JSON dictionary to use shorter key names (`old` and `new`), so that the file is more concise and readable.

#### Acceptance Criteria

1. THE JSON dictionary output SHALL use `"old"` as the key name instead of `"original"`
2. THE JSON dictionary output SHALL use `"new"` as the key name instead of `"translation"`
3. THE DictReader SHALL parse `"old"` and `"new"` JSON keys when loading dictionary files
4. THE `RecordEntry` struct fields in C++ code SHALL be renamed: `id` becomes `key_text`, `original` becomes `old_text`, `translation` becomes `new_text`, `status` stays as `status`
5. THE mapping between C++ fields and JSON keys SHALL be:

| C++ struct field | JSON key | Meaning |
|---|---|---|
| `key_text` | `"id"` | composite lookup key |
| `old_text` | `"old"` | foreign/source text |
| `new_text` | `"new"` | native/translated text |
| `status` | `"status"` | translation state |

### Requirement 14: Unify insertion functions

**User Story:** As a developer, I want a single insertion function that handles both make and base modes, so that the code has no duplicated logic.

#### Acceptance Criteria

1. THE DictCreator SHALL replace `insertRecord` and `insertRecordToDict` with a single `insertEntry(id, old_text, new_text, type)` function
2. WHEN called in make mode (`is_make_mode == true`), `insertEntry` SHALL ignore the `new_text` parameter and look up `base_dict` to determine `new_text` and `status` (same behavior as current `insertRecord`)
3. WHEN called in base mode (`is_make_mode == false`), `insertEntry` SHALL use `new_text` directly, set `status = ""`, and handle DOUBLED duplicates (same behavior as current `insertRecordToDict`)

### Requirement 15: Eliminate `is_make_mode` branches inside extractors

**User Story:** As a developer, I want each extractor function to have a single code path instead of `if (is_make_mode)` branches, so that the code is simpler and easier to maintain.

#### Acceptance Criteria

1. EACH extractor function (makeDictCELL, makeDictCELL_Default, makeDictCELL_REGN, makeDictDIAL, makeDictGMST, makeDictFNAM, makeDictDESC, makeDictTEXT, makeDictRNAM, makeDictINDX, makeDictNPC_FLAG, makeDictINFO, makeDictScript) SHALL have a single loop that calls `insertEntry` with the appropriate parameters for both modes
2. FOR all extractors: `old_text` SHALL come from `esm_ref` — in make mode this reads from `esm` itself (native text), in base mode this reads from `esm_ext` (foreign text)
3. Pattern 1 extractors (CELL, CELL_Default, CELL_REGN, DIAL, Script): use `esm_ref` at the same record index for same-index pairing
4. Pattern 2 extractors (GMST, FNAM, DESC, TEXT, RNAM, INDX, NPC_FLAG, INFO): use pre-built index to find the matching record position in `esm_ref`, then read its value subrecord

### Requirement 16: Build esm_ext lookup indexes

**User Story:** As a developer, I want pre-built indexes for esm_ext records, so that Category B extractors can efficiently look up foreign text without nested loops.

#### Acceptance Criteria

1. WHEN `makeDictForBase` is called, THE DictCreator SHALL build `unordered_map<string, size_t>` indexes mapping internal keys to record positions in `esm_ref` for each record type that needs it (GMST, FNAM, DESC, TEXT, RNAM, INDX, NPC_FLAG, INFO)
2. THE indexes SHALL be built once before any extractor runs
3. THE indexes SHALL use the same composite key format as the extractors use for `id` (e.g. `"NPC_^Caius Cosades"` for FNAM, `"SKIL^000"` for INDX)
4. EACH Pattern 2 extractor SHALL use the pre-built index to find the matching record in `esm_ref` and read its value subrecord to obtain `old_text`
5. IN make mode, the indexes SHALL also be built from `esm_ref` (which points to `esm`) — the lookup will return the same text as `esm`, making `old_text` = native text (correct for make mode)

### Requirement 17: Keep unordered variants unchanged

**User Story:** As a developer, I want the unordered matching variants to remain separate functions, so that the complex fingerprint-matching logic is not mixed with the simpler ordered/indexed logic.

#### Acceptance Criteria

1. THE unordered variants (makeDictCELL_Unordered, makeDictDIAL_Unordered, makeDictScript_Unordered, and their helpers) SHALL remain as separate base-mode-only functions
2. THE unordered variants SHALL call `insertEntry` with the correct `old_text` parameter (foreign text from the matched esm_ext record)
3. THE unordered variants SHALL NOT be merged with the ordered extractors

### Requirement 18: Unify makeDictForMake and makeDictForBase into single orchestrator

**User Story:** As a developer, I want a single orchestrator function that handles both make and base modes, so that the extraction sequence is defined in one place.

#### Acceptance Criteria

1. THE DictCreator SHALL replace `makeDictForMake()` and `makeDictForBase(same_order)` with a single `makeDict()` function
2. THE unified `makeDict()` SHALL call all extractors in the same order for both modes
3. WHEN in base mode with `!same_order`, THE unified `makeDict()` SHALL call the unordered variants (CELL_Unordered, DIAL_Unordered, Script_Unordered) instead of the ordered ones for CELL, DIAL, and Script
4. THE unified `makeDict()` SHALL call `buildExtIndexes()` at the start when in base mode
5. THE `makeDictFNAM_Glossary()` function SHALL only be called in base mode (it produces glossary entries from paired ESMs)

### Requirement 19: Keep esm_ref as unified access point

**User Story:** As a developer, I want `esm_ref` to remain the single reference for Pattern 1 extractors, so that they work transparently in both modes without branching.

#### Acceptance Criteria

1. THE DictCreator SHALL keep `esm_ref` as a reference member: bound to `esm` in make mode, bound to `esm_ext` in base mode
2. Pattern 1 extractors (CELL, DIAL, Script) SHALL use `esm_ref` for same-index pairing — this gives native text in make mode and foreign text in base mode
3. THE `esm_ext` member SHALL only be used directly by: `buildExtIndexes()`, `makeDictFNAM_Glossary()`, and the unordered variants
4. THE `is_make_mode` flag SHALL remain to distinguish behavior inside `insertEntry` (base_dict lookup vs direct assignment)

### Requirement 20: Make mode continues to work without esm_ext

**User Story:** As a translator, I want `--make` to continue working with a single plugin file, so that I can extract records without needing a second ESM.

#### Acceptance Criteria

1. WHEN `--make` is called with one plugin, `esm_ref` SHALL be bound to `esm` (same ESM)
2. Pattern 1 extractors using `esm_ref[i]` SHALL read from `esm` itself — producing `old_text` = native text (same as the plugin text)
3. Pattern 2 extractors SHALL skip the `esm_ext` index lookup in make mode (no foreign ESM loaded) and set `old_text` = text from `esm`
4. `insertEntry` in make mode SHALL use `base_dict` lookup for `new_text` and `status` determination

### Requirement 21: Replace all esm_ext usage with esm_ref in DictCreator functions

**User Story:** As a developer, I want all DictCreator functions to use `esm_ref` instead of `esm_ext` directly, so that the code works transparently in both modes through a single reference.

#### Acceptance Criteria

1. ALL functions in `dictcreator.cpp` that currently access `esm_ext` directly SHALL be rewritten to use `esm_ref` instead
2. THE `esm_ext` member SHALL only be used for storage — it SHALL NOT be accessed directly by any extractor or helper function
3. THE unordered variants (makeDictCELL_Unordered, makeDictDIAL_Unordered, makeDictScript_Unordered, makeDictFNAM_Glossary, and all pattern/missing helpers) SHALL use `esm_ref` instead of `esm_ext`
4. THE `buildExtIndexes()` function SHALL iterate `esm_ref` instead of `esm_ext`
5. THIS ensures that in make mode (`esm_ref` = `esm`), all functions read from the native ESM; in base mode (`esm_ref` = `esm_ext`), all functions read from the foreign ESM

### Requirement 22: Always treat records as unordered

**User Story:** As a developer, I want to always use the unordered matching approach (fingerprint/key lookup), so that the code works correctly regardless of record ordering and there's no branching on `same_order`.

#### Acceptance Criteria

1. THE `isSameOrder()` function SHALL be removed
2. THE `same_order` parameter and branching logic SHALL be removed from the orchestrator
3. THE unordered variants (makeDictCELL_Unordered, makeDictDIAL_Unordered, makeDictScript_Unordered and their helpers) SHALL be kept and become the only code path for CELL, DIAL, and Script in base mode
4. THE same-index pairing code inside `makeDictCELL`, `makeDictDIAL`, and `makeDictScript` (the `else` branch that uses `esm_ref[i]`) SHALL be removed — these functions only handle make mode now
5. Pattern 2 extractors (GMST, FNAM, DESC, TEXT, RNAM, INDX, NPC_FLAG, INFO) SHALL use pre-built index lookups to find matching records in `esm_ref`
