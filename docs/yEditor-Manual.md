# yEditor — User Manual

Plugin conflict viewer and merged patch creator for Morrowind.

## Loading Plugins

Open File menu and choose one of three loading methods:

- **Open Folder** (Ctrl+O) — select a folder containing ESM/ESP files. A dialog appears listing all plugins found. Check the ones you want to load. They are sorted by modification date (simulating load order).
- **Open MO2 Profile** — select a Mod Organizer 2 profile directory. The application reads loadorder.txt and modlist.txt to determine which plugins to load and in what order. Plugins are resolved through MO2's virtual filesystem (overwrite folder, mod folders).
- **Open OpenMW Config** — select an openmw.cfg file. The application reads all `data=` and `content=` lines to build the load order.

Use **Unload All** to close everything and start fresh.

## Main Layout

- **Left** — Navigation tree. Plugins are listed at the top level. Under each plugin, records are grouped by type (ACTI, ARMO, CELL, NPC_, etc.). Under each type, individual records are listed by ID.
- **Right** — Record view. When you click a record in the nav tree, this area shows all sub-records in a multi-column tree. Each column represents one plugin's version of that record. The leftmost column is the master definition, subsequent columns are overrides in load order.
- **Bottom** — Log tab (operation output) and Preview tab.

## Navigation Tree

The tree uses colors to communicate conflict information at a glance.

Background colors indicate the overall conflict severity for a record (worst-case across all sub-records):

- **Green** — no conflict. All plugins that touch this record agree on its content.
- **Yellow** — benign override. A later plugin changes the record but the change is non-destructive (e.g. moving a reference slightly).
- **Red** — real conflict. Multiple plugins make incompatible changes to the same record.

Text colors indicate how each specific plugin version relates to others:

- **Purple** — this is the master definition (first plugin to define the record).
- **Gray** — this plugin's version is identical to the master. No effective change.
- **Green** — this plugin overrides the master and wins (it's the last loaded).
- **Orange** — this plugin has a conflicting change and wins by load order.
- **Red** — this plugin has a conflicting change but loses (a later plugin overwrites it).

Records with no conflict (only one plugin defines them) show with no background color and black text.

## Record View

Clicking a record in the nav tree displays its full content in the record view. Sub-records are decoded into readable fields where the format is known (names, positions, flags, stats). Unknown or binary sub-records display as raw byte counts.

Each column represents one plugin's version. Column headers show the plugin filename, colored by that plugin's conflict status for this record. Cells with differing values across plugins are highlighted to make conflicts visible.

Empty cells mean that plugin does not include the sub-record. This happens when a plugin only modifies some fields of a record.

## Dialogue INFO Chain

When you select a DIAL (dialogue topic) record, the record view shows an additional section below the sub-records: the INFO chain. This displays all INFO records belonging to the topic in their final merged order, resolved using the same algorithm as OpenMW.

Each INFO is positioned according to its PNAM (Previous Info) sub-record. When a plugin adds a new INFO with PNAM pointing to an existing INFO, the new one is inserted immediately after it. When a plugin redefines an existing INFO with a different PNAM, the INFO is moved to its new position in the chain.

Each row in the INFO chain shows the INFO's display name (typically the speaker NPC ID) and a checkmark in each plugin column that contains that INFO. This lets you see which plugin contributes each dialogue response and in what order the player will encounter them in-game.

## Context Menus

Right-click in the record view to access merge operations:

- **Copy Record to Merged Patch** — copies the entire record from the selected plugin column into the merged patch.
- **Copy Sub-Record to Merged Patch** — copies a single sub-record (one row) from a plugin column.
- **Copy Group to Merged Patch** — copies a group of related sub-records (e.g. all fields of a referenced object in a cell).
- **Remove Sub-Record** / **Remove Group** — removes content from the merged patch column.

Right-click a plugin node in the navigation tree for plugin-level options:

- **Exclude from Merged Patch** / **Include in Merged Patch** — excluded plugins are completely ignored during auto-merge. Their records will not appear in the merged patch regardless of conflicts.
- **Mark as Guard Patch** — the guard patch acts as a priority barrier during auto-merge. Plugins loaded before the guard that modify the same records are ignored. Only the guard's version and later plugins are considered. If the final plugin's version matches master (reverting a change), the guard's version is used instead of letting the revert through.

## Creating a Merged Patch

Click **Create Merged Patch** in the toolbar to run the automatic merge. The auto-merge performs several operations:

- **Leveled list merge** — combines entries from all plugins that modify leveled item or creature lists. No entries are lost; duplicates are removed.
- **Three-way record merge** — for object records modified by multiple plugins, compares each plugin's changes against the master. Non-conflicting field changes from different plugins are combined into one record.
- **Bug fixes** — optionally corrects known engine bugs: fog density values outside valid range, summon persistence flags, and cell name reverts.

After auto-merge completes, the merged patch is saved automatically. The output location depends on how you loaded plugins: same folder for Open Folder, MO2 overwrite directory for Open MO2 Profile, or the OpenMW data directory for Open OpenMW Config.

You can refine the auto-merge result manually. Use the record view context menu to copy individual sub-records from any plugin column into the merged patch, or remove sub-records that shouldn't be there. Changes are saved immediately.

## Settings

Open Settings via Ctrl+, or the Edit menu. Five pages are available:

- **Appearance** — choose between light and dark theme.
- **Paths** — configure the merged patch output path for each loading mode (folder, MO2, OpenMW). Normally these are automatic and don't need changing.
- **Merged Patch** — toggle which record types participate in auto-merge. Set an exclusion regex to skip specific record IDs. Enable or disable individual bug fixes (fog density fix, summon persistence fix, cell name reversion fix).
- **Cleaning** — toggle which cleaning operations the Clean All button performs. Evil GMSTs are Construction Set artifacts from Tribunal/Bloodmoon that can cause issues in mods that don't require those expansions. Junk cells are empty exterior cell records that only contain position data and serve no purpose.
- **Sub-Record Rules** — configure which sub-records are ignored during conflict detection, excluded from the merged patch output, or skipped when not present in a plugin. Each field takes a comma-separated list of entries in `RECORD:SUB` format (e.g. `CELL:NAM0, CELL:NAM9`). Use `*` as a wildcard for the sub-record name to match all sub-records of a record type.

## Cleaning Plugins

Click **Clean All** in the toolbar to remove known problematic records from all loaded plugins (except masters). Cleaned plugins are written to the output directory alongside the merged patch. The original plugin files are never modified.

Two types of records are removed:

- **Evil GMSTs** — game settings injected by the Construction Set when editing plugins with Tribunal or Bloodmoon loaded. These settings override expansion-specific values and can cause problems for players without the expansions.
- **Junk cells** — exterior cell records that contain only a NAME and DATA sub-record with no references, no region assignment, and no meaningful content. These are Construction Set artifacts from brief edits near cell borders.
