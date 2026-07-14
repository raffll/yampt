# TODO

readme, feature categories

nexus api -> auto upload, auto description
github -> remove obsolete releases

ship model, at least polish

wire up eet convert
shortcuts, leave del
detail button should show better name depends on what is in panel
implement statuses on yaml

settings
panel with all exceptions configurable

yampt <- remove –win1250 


Sorted by effort within each priority tier. Priority is based on: (1) how much infrastructure already exists, (2) practical value for the Morrowind modding workflow, (3) uniqueness vs already-solved-by-other-tools.

## Priority 1 — Low effort, high payoff (infrastructure already exists)

### Plugin Cleaning: Remove ITM Records (yEditor) [S]
yEditor already detects ITM via `plugin_scan_t::itm_count()` and `itm_entries()`. Expose as a one-click "Remove ITM" action that strips identical-to-master records and saves the cleaned plugin. The detection is done — only the write-back path is missing.

### Plugin Cleaning: Remove Evil GMSTs (yEditor) [S]
Known list of 72 GMST IDs that TESCS injects. Compare against master values — if identical, remove. Trivial record-type filter on top of existing ITM logic. Can share the same "Clean Plugin" button.
---

## Priority 2 — Medium effort, directly extends existing code

### Plugin Cleaning: Remove Junk Cells (yEditor) [M]
Detect empty CELL records (no FRMR refs, no LAND/PGRD) that aren't needed. Extends the ITM detection with a content-size check on cell partition. `cell_partition_t` parsing already exists in `sub_record_merge_t`.

### Batch Clean (yEditor) [M]
Combine ITM removal + evil GMST removal + junk cells into a single "Clean All" that processes every loaded plugin in sequence. Log results per-plugin. Fog fix and summon fix already run as part of merged patch — no need for standalone actions.

### Find Text Across All Records (yEditor) [M]
Search all loaded plugins for a text string. Results list: record type, ID, plugin, matching sub-record content. Double-click navigates to the record in the view tree. Uses existing `esm_reader_t` iteration + `sub_record_iter` for content access.

---

## Priority 3 — Significant effort, valuable features

### Dialogue Merging (yEditor) [L]
`merge_dialogue()` already exists in `plugin_scan_t` and `auto_merge_t`. Extend to handle INFO chain ordering conflicts (PNAM/NNAM link repair). `dial_info_align.hpp` already aligns INFO records across plugins — build the merge logic on top.

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

### Lua Handlers Comparison/Conflicts Detector (yEditor) [XL]
Detect conflicts between OpenMW Lua handler registrations across multiple mods (e.g. two mods registering `addSkillLevelUpHandler` that both return `false`). Show conflicts in yEditor's comparison view.

### Linux Build Support [XL]
No Windows-only dependencies identified. Needs CMakeLists.txt for Linux.
