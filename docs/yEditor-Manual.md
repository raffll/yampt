# yEditor — User Manual

Plugin conflict viewer and merged patch creator for Morrowind ESM/ESP files. Loads multiple plugins, detects conflicts between them, and produces a merged patch that resolves those conflicts automatically or manually.

## Getting Started

1. Launch `yEditor.exe`
2. Load plugins via File → Open Folder, Open MO2 Profile, or Open OpenMW Config
3. The navigation tree shows all records organized by plugin and type
4. Click a record to see all versions side-by-side in the record view
5. Click "Create Merged Patch" in the toolbar to auto-generate a merged patch

## Window Layout

- **Left panel**: Navigation tree — plugins → record types → individual records
- **Right panel**: Record view — sub-records displayed as a multi-column tree (one column per plugin)
- **Bottom panel**: Log tab (operation messages) and Preview tab (text comparison)
- **Toolbar**: Create Merged Patch button
- **Status bar**: Load source info (left), record/conflict counts (right)

## Loading Plugins

Three loading modes:

### Open Folder
Select a folder containing ESM/ESP files. A selection dialog lets you pick which plugins to load and their order.

### Open MO2 Profile
Select a Mod Organizer 2 profile directory. yEditor reads `loadorder.txt` and `modlist.txt`, resolves plugin paths through the MO2 mod folder structure automatically.

### Open OpenMW Config
Select an `openmw.cfg` file. yEditor parses `data=` directories and `content=` entries to resolve and load all plugins in the correct order.

## Navigation Tree

Records are organized as: **Plugin → Record Type → Record**

Each node is colored by conflict status:

- **Background** (conflict severity): green = no conflict, yellow = benign override, red = conflict
- **Text color** (per-plugin status): purple = master, gray = identical to master, green = override wins, orange = conflict wins, red = conflict loses

### Filtering

- **View → Conflicts Only**: Shows only records where multiple plugins define different values
- **View → Hide Duplicate Columns**: Collapses identical versions
- **View → Show Deleted Strikeout**: Strikethrough for deleted records
- **View → Filter**: Advanced filter dialog with options for conflict level, conflict status, record type, ID/name text search, and deleted flag

## Record View

The right panel shows all sub-records of the selected record, with one column per plugin that defines it. Sub-records are decoded into human-readable fields where schemas are available.

Display modes vary by record type:

- **CELL**: Groups sub-records by referenced object (FRMR index)
- **LEVI/LEVC**: Aligns leveled list entries by item/creature ID across plugins
- **FACT**: Aligns faction rank entries by name
- **CONT/NPC_/CREA**: Aligns inventory items and spells by ID
- **Generic**: Shows sub-records in file order

### Context Menu (right-click on sub-records)

Actions depend on whether the record is already in the merged patch:

**When no merge record exists:**
- Copy Record to Merged Patch — copies the entire record from the clicked plugin

**When merge record exists (clicking a source column):**
- Copy Sub-Record to Merged Patch
- Copy Group to Merged Patch
- Copy Field to Merged Patch

**When clicking the merge column:**
- Remove Sub-Record
- Remove Group

### Navigation Tree Context Menu

- **On source plugin files**: Include/Exclude from Merge, Mark/Unmark as Guard Patch
- **On merged patch records**: Remove from merge

## Merged Patch

### Automatic Merge

Click "Create Merged Patch" in the toolbar. The auto-merge handles:

- **Leveled lists** (LEVI/LEVC): Combines entries from all plugins, removes duplicates, takes header fields from the winning plugin
- **Dialogues** (DIAL): Merges INFO chains from all plugins (first-seen ordering, last-wins for content)
- **Three-way object merge**: For records with 3+ versions, performs byte-level merge of packed sub-records (NPDT, AIDT, WPDT, AODT, AI_W) to combine non-conflicting field changes
- **Bug fixes**: Fog density fix, summon persistence fix, cell name reversion fix (configurable in Settings)

The merged patch is auto-saved to the configured output path.

### Manual Editing

After auto-merge, you can refine the result by copying individual sub-records or fields from any plugin version into the merge column, or removing unwanted entries.

### Guard Patches

Right-click a plugin in the nav tree and select "Mark as Guard Patch". Guard patch plugins are given priority in the auto-merge — their records take precedence over automatic merging logic.

### Excluded Plugins

Right-click a plugin and select "Exclude from Merge" to skip it entirely during auto-merge. Useful for plugins you don't want contributing to the merged patch.

### Save Plugin

File → Save Plugin prompts for author and description, then writes the merged patch as an ESP file. Master entries are computed automatically from contributing plugins.

## Settings

Preferences (Ctrl+,) has three pages:

### Appearance
Theme selection (light/dark).

### Paths
Merged patch output path relative to the load base directory:

- **Folder mode**: `Merged Patch.esp` (same folder as loaded plugins)
- **MO2 mode**: `../../overwrite/Merged Patch.esp` (MO2 overwrite folder)
- **OpenMW mode**: `data/Merged Patch.esp` (OpenMW data directory)

### Merge
- **Record Types**: Enable/disable which types participate in the merge (36 checkboxes)
- **Exclusion Pattern**: Regex to exclude records by ID
- **Bug Fixes**: Toggle fog density fix, summon persistence fix, cell name reversion fix

## Conflict Colors Reference

### Background (row-level conflict severity)
| Level | Color | Meaning |
|-------|-------|---------|
| Only One | white | Single plugin defines this record |
| No Conflict | green | Multiple plugins, all identical |
| Override | yellow | Simple override (master differs from winner) |
| Conflict | red | Multiple plugins with different values |

### Text (per-column plugin status)
| Status | Color | Meaning |
|--------|-------|---------|
| Master | purple | First/master definition |
| Identical to Master | gray | Same as master (ITM candidate) |
| Override Wins | green | Override that wins by load order |
| Conflict Wins | orange | Conflicting version that wins |
| Conflict Loses | red | Conflicting version that loses |

## Tips

- Load all your plugins in the correct order (matching your actual load order) for accurate conflict detection
- Use "Conflicts Only" mode to focus on records that need attention
- After creating a merged patch, check red-highlighted records for unresolved conflicts that may need manual intervention
- Guard patches are useful for patches that are specifically designed to override certain records (like Patch for Purists)
- The merged patch auto-saves on every change — no need to manually save after each copy operation
