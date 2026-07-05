# TODO

Sorted by effort within each priority tier. Priority is based on: (1) how much infrastructure already exists, (2) practical value for the Morrowind modding workflow, (3) uniqueness vs already-solved-by-other-tools.

## Priority 1 — Low effort, high payoff (infrastructure already exists)

### Plugin Cleaning: Remove ITM Records (yEditor) [S]
yEditor already detects ITM via `plugin_scan_t::itm_count()` and `itm_entries()`. Expose as a one-click "Remove ITM" action that strips identical-to-master records and saves the cleaned plugin. The detection is done — only the write-back path is missing.

### Plugin Cleaning: Remove Evil GMSTs (yEditor) [S]
Known list of 72 GMST IDs that TESCS injects. Compare against master values — if identical, remove. Trivial record-type filter on top of existing ITM logic. Can share the same "Clean Plugin" button.

### Conflict Report Export (yEditor) [S]
yEditor already shows conflicts visually with full coloring. Add "Export to Text" that dumps the nav tree + per-record conflict summary to a plain-text or markdown file. No new analysis needed.

### Leveled List Merge Polish (yEditor) [S]
`auto_merge_t` already merges leveled lists, dialogues, and three-way objects. The "Create Merged Patch" button exists. Polish: add progress feedback, a summary dialog showing what was merged, and make the one-click experience smoother.

### YAML Translation: Save Directly as Target Language File (yTranslator) [S]
Stop using a tmp file. Since we know the native language from settings, save the YAML translation directly as `pl.yaml` (or `de.yaml`, etc.) in the workspace folder. When translating a YAML opened from outside the workspace, store a metadata association (source mod path → output file path) so yTranslator knows where the translation belongs on subsequent opens. The `yaml_document_t` already handles save — just needs proper naming and path resolution.

---

## Priority 2 — Medium effort, directly extends existing code

### Plugin Cleaning: Remove Junk Cells (yEditor) [M]
Detect empty CELL records (no FRMR refs, no LAND/PGRD) that aren't needed. Extends the ITM detection with a content-size check on cell partition. `cell_partition_t` parsing already exists in `sub_record_merge_t`.

### Object Three-Way Merge Expansion (yEditor) [M]
`sub_record_merge_t::merge()` handles generic three-way merge + cell refs + leveled lists + ENAM slots. Extend coverage to more packed sub-records (WPDT, NPDT, AODT) where two mods change different fields of the same record. The framework is there — needs per-type field layout tables.

### Batch Clean (yEditor) [M]
Combine ITM removal + evil GMST removal + junk cells into a single "Clean All" that processes every loaded plugin in sequence. Log results per-plugin. Fog fix and summon fix already run as part of merged patch — no need for standalone actions.

### Find Text Across All Records (yEditor) [M]
Search all loaded plugins for a text string. Results list: record type, ID, plugin, matching sub-record content. Double-click navigates to the record in the view tree. Uses existing `esm_reader_t` iteration + `sub_record_iter` for content access.

---

## Priority 3 — Significant effort, valuable features

### Dialogue Merging (yEditor) [L]
`merge_dialogue()` already exists in `plugin_scan_t` and `auto_merge_t`. Extend to handle INFO chain ordering conflicts (PNAM/NNAM link repair). `dial_info_align.hpp` already aligns INFO records across plugins — build the merge logic on top.

### CSV Export/Import for Bulk Editing (yampt.core + yTranslator) [L]
Export dictionary entries (or raw ESM records) to CSV for spreadsheet-based mass edits (NPC stats, item values, translation batches). Import back with validation. Useful for rebalancing mods and translation teams.

### Regex Find/Replace in Translations (yTranslator) [L]
`find_replace_t` already handles literal find/replace across all entries. Extend with `std::regex` support. Batch mode: apply to all matching entries with preview. Useful for systematic text corrections across thousands of entries.

### Diff Viewer Between Plugin Versions (yTranslator) [L]
When a mod updates, show which source strings changed between old and new ESM. Uses existing `dict_creator_t` to extract both versions, then diff. Highlights entries needing re-translation. `char_diff_t` already provides character-level diffing.

### Plugin Header Management (yEditor or yampt.cli) [L]
- Update master file sizes in headers (Wrye Mash feature — needed after cleaning)
- Update plugin version to 1.3
- Reassign masters (swap one master for another)
Simple binary surgery on the TES3 header record. `esm_reader_t` already parses it.

---

## Priority 4 — Large effort, nice-to-have

### Scan Lua Files for Translatable Strings [L]
Parse OpenMW Lua scripts (l10n YAML, or string literals) to extract translatable text for dictionary creation.

### Generate TOP/CEL/MRK Files from Dictionaries [XL]
Create `.top`, `.cel`, `.mrk` files based on DIAL/CELL dictionaries and the language spell checker dictionary. These are OpenMW cell/topic name files used by the engine for localized map markers and journal topics. Include generated files as annotation sources in yTranslator.

### Script Compiler with Diagnostics [XL]
Full MWScript parser with better error messages than TESCS. Color-coded script display (already partially in yTranslator's `script_tokenizer`). Function parameter type checking, length checks, deprecated function warnings. Builds on existing `script_parser_t`.

### Lua Handlers Comparison/Conflicts Detector (yEditor) [XL]
Detect conflicts between OpenMW Lua handler registrations across multiple mods (e.g. two mods registering `addSkillLevelUpHandler` that both return `false`). Show conflicts in yEditor's comparison view.

### Linux Build Support [XL]
No Windows-only dependencies identified. Needs CMakeLists.txt for Linux.

---

## Excluded — Not a good fit

These were considered but don't make sense for yampt's apps:

- **BSA Archive Extraction** — unrelated to translation or conflict resolution; dedicated tools (BSA Browser) do this better
- **Plugin Active List Management** — MO2 and OpenMW launcher already handle this; duplicating would create confusion
- **Automatic Backup on Save** — OS-level or VCS-level concern, not an app responsibility
- **PEX Script Decompilation** — Skyrim/Fallout only, yampt is Morrowind-focused
- **Fuz Voice File Mapping** — Morrowind uses simple .wav files referenced in Say commands; already handled by script parser displaying the filename
- **Record Copying Between Plugins** — yEditor's drag-and-drop to merge patch already covers this use case
- **Alias/Variable Integrity Check** — Morrowind doesn't use format variables in dialogue; irrelevant for TES3
- **Encoding Detection and Conversion** — already solved (Windows-1250/1251/1252 + UTF-8 detection); adding Asian encodings has no Morrowind use case
