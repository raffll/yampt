# yEditor — User Manual

Plugin conflict viewer and merged patch creator for Morrowind.

## Loading Plugins

File menu:

- **Open Folder** (Ctrl+O) — select a folder with ESM/ESP files, then pick which plugins to load
- **Open MO2 Profile** — select a Mod Organizer 2 profile directory (reads loadorder.txt automatically)
- **Open OpenMW Config** — select an openmw.cfg file
- **Unload All** — close everything

## Main Layout

- **Left**: Navigation tree (plugins → record types → records)
- **Right**: Record view (sub-records in columns, one column per plugin)
- **Bottom**: Log and Preview tabs

## Navigation Tree

Records are grouped under their owning plugin, then by record type. Colors indicate conflict status:

- Green background — no conflict (all plugins agree)
- Yellow background — benign override
- Red background — real conflict between plugins
- Purple text — master record
- Gray text — identical to master (ITM)
- Green text — winning override
- Red text — losing version

## View Menu

- **Conflicts Only** — hide records where all plugins agree
- **Hide Duplicate Columns** — collapse identical plugin versions
- **Show Deleted Strikeout** — strikethrough deleted records
- **Filter** — advanced filter by conflict level, record type, ID, or name

## Record View

Clicking a record in the nav tree shows all its sub-records in a multi-column tree. Each column represents one plugin's version. Sub-records are decoded into readable fields where possible.

Right-click in the record view to:

- **Copy Record/Sub-Record/Group/Field to Merged Patch** — from a source plugin column
- **Remove Sub-Record/Group** — from the merge column

Right-click on a plugin node in the nav tree to:

- **Exclude from Merge** / **Include in Merge**
- **Mark as Guard Patch** (priority plugin in auto-merge)

## Creating a Merged Patch

Click **Create Merged Patch** in the toolbar. The auto-merge:

- Merges leveled lists (combines entries from all plugins)
- Merges dialogues (collects INFO records from all plugins)
- Three-way merges object records (combines non-conflicting field changes)
- Applies bug fixes (fog density, summon persistence, cell name reversion)

The merged patch is saved automatically. Output location depends on how you loaded plugins (same folder, MO2 overwrite, or OpenMW data dir).

After auto-merge, you can manually refine by copying or removing individual sub-records via the context menu.

## Settings (Ctrl+,)

- **Appearance**: Light/dark theme
- **Paths**: Merged patch output path for each load mode
- **Merge**: Record type toggles, exclusion regex, bug fix toggles
