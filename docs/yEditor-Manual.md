# yEditor — User Manual

Plugin conflict viewer and merged patch creator for Morrowind.

## Loading Plugins

Open File menu and choose one of three loading methods:

- **Open Folder** (Ctrl+O) — select a folder containing ESM/ESP files. A dialog appears listing all plugins found. Check the ones you want to load. They are sorted by modification date (simulating load order).
- **Open MO2 Profile** — select a Mod Organizer 2 profile directory. The application reads loadorder.txt and modlist.txt to determine which plugins to load and in what order. Plugins are resolved through MO2's virtual filesystem (overwrite folder, mod folders).
- **Open OpenMW Config** — select an openmw.cfg file. The application reads all `data=` and `content=` lines to build the load order.

Use **Unload All** to close everything and start fresh.

If any loaded plugin has unsaved field edits, you are prompted before those edits would be lost. Loading a new set of plugins, unloading everything, cleaning, and closing the application each offer to save the pending changes, discard them, or cancel the action and keep everything as it is.

## Main Layout

- **Left** — Navigation panel with two tabs. The Plugins tab shows the ESP/ESM record tree: plugins at the top level, records grouped by type under each plugin. The Lua tab shows OpenMW Lua handler registrations grouped by mod name.
- **Right** — Record view. When you click a record in the nav tree, this area shows all sub-records in a multi-column tree. Each column represents one plugin's version of that record. The leftmost column is the master definition, subsequent columns are overrides in load order.
- **Bottom** — Edit tab (field comparison and editing) and Log tab (operation output).

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

Each plugin in the tree is prefixed with an icon indicating its role:

- 📜 — a master file that other plugins depend on.
- 📄 — a regular plugin loaded from a mod folder or game data directory.
- ⚡ — an overridden plugin loaded from MO2's overwrite folder, meaning a cleaned or patched copy is being used instead of the original mod version.
- ⚙ — the merged patch produced by the auto-merge operation.
- 🛡 — a guard patch that acts as a priority barrier during auto-merge.
- 🔒 — a plugin excluded from the merged patch. Its records are ignored during merge.

When a plugin has field edits that have not yet been written to disk, an asterisk appears next to its name, after the icon and before the filename. The asterisk disappears once the plugin is saved.

## Record View

Clicking a record in the nav tree displays its full content in the record view. Sub-records are decoded into readable fields where the format is known (names, positions, flags, stats). Unknown or binary sub-records display as raw byte counts.

Each column represents one plugin's version. Column headers show the plugin filename, colored by that plugin's conflict status for this record. Cells with differing values across plugins are highlighted to make conflicts visible.

Empty cells mean that plugin does not include the sub-record. This happens when a plugin only modifies some fields of a record.

## Dialogue Responses

When you select a DIAL (dialogue topic) record, the record view shows an additional collapsible group below the sub-records: Dialogue Responses. Expand it to see all INFO records belonging to the topic in their final merged order, resolved using the same algorithm as OpenMW; collapse it to hide the list.

Each INFO is positioned according to its PNAM (Previous Info) sub-record. When a plugin adds a new INFO with PNAM pointing to an existing INFO, the new one is inserted immediately after it. When a plugin redefines an existing INFO with a different PNAM, the INFO is moved to its new position in the chain.

Each row is labeled with the INFO's display name (typically the speaker NPC ID), and each plugin column shows that plugin's response text for the INFO. Selecting a response row shows its full text in the Edit panel comparison, so long responses can be read in full. A column is left empty when the plugin does not contain the INFO. When two or more plugins define the same INFO with different response text, the row is highlighted as a conflict in the same way as any other differing sub-record, so you can see not only which plugin contributes each response and in what order the player encounters them, but also where plugins disagree on the wording.

## Context Menus

Right-click in the record view to access merge operations:

- **Copy Record to Merged Patch** — copies the entire record from the selected plugin column into the merged patch.
- **Copy Sub-Record to Merged Patch** — copies a single sub-record (one row) from a plugin column.
- **Copy Field to Merged Patch** — copies a single decoded field within a sub-record from a plugin column.
- **Copy Bit to Merged Patch** — copies a single flag bit (a row under a Flags field, such as Female) from a plugin column, changing only that bit in the merged patch and leaving the record's other flags as they are.
- **Copy Group to Merged Patch** — copies a group of related sub-records (e.g. all fields of a referenced object in a cell).
- **Remove Sub-Record from Merged Patch** / **Remove Group from Merged Patch** — removes content from the merged patch column.
- **Exclude Sub-Record** / **Include Sub-Record** — toggles the sub-record type in the exclusion list. When the type is already excluded, the action reads "Include Sub-Record" and removes the rule; when the whole record type is excluded by a wildcard rule, the include option is greyed out. Excluding adds the sub-record type to the exclusion list in settings. The sub-record will be hidden from conflict detection and excluded from the merged patch. The rule is stored as `RECORD:SUB` (e.g. `CELL:NAM0`) and can be reviewed in Settings. Exclusion applies only to top-level sub-records; sub-records nested inside a cell's referenced objects are never excluded, so a rule such as `CELL:DATA` affects the cell's own data and leaves the referenced objects intact. This option is offered only on top-level sub-record rows.

Right-click a record node belonging to the merged patch in the navigation tree to see the **Remove Record from Merged Patch** option, which deletes that record from the merged patch entirely.

Right-click a record node belonging to a loaded plugin to see the **Copy Record to Merged Patch** option, which copies the whole record from that plugin into the merged patch. It is greyed out unless a merged patch exists and the record is not already in it.

The same menu offers **Remove Record from Plugin**. After a confirmation prompt, the record is dropped from that plugin in memory and the plugin is marked as having unsaved changes. The record disappears from the file the next time you save the plugin. This removal cannot be undone; the only way to recover the record is to close the plugin without saving. The option is greyed out unless editing is enabled.

Right-click a plugin node in the navigation tree for plugin-level options:

- **Save** — writes the plugin's pending field edits to disk and removes its asterisk. This option is enabled only while the plugin has unsaved changes; when the plugin is already saved it appears greyed out.
- **Exclude from Merged Patch** / **Include in Merged Patch** — excluded plugins are completely ignored during auto-merge. Their records will not appear in the merged patch regardless of conflicts.
- **Mark as Guard Patch** — the guard patch acts as a priority barrier during auto-merge. Plugins loaded before the guard that modify the same records are ignored. Only the guard's version and later plugins are considered. If the final plugin's version matches master (reverting a change), the guard's version is used instead of letting the revert through.

## View Menu

The View menu provides display options:

- **Toggle Sidebar** — shows or hides the left navigation panel, giving the record view the full width of the window.
- **Toggle Bottom Panel** — shows or hides the bottom edit and log panel, giving the record view more vertical space. Both toggles remember their state between sessions.
- **Sync Scrolling** — locks the scroll position between the two comparison panes in the Edit panel so they stay aligned as you scroll either one. The setting is remembered between sessions.
- **Show Only One Column Per Plugin** — when a plugin defines the same record more than once, collapses those versions into a single column showing only that plugin's last (winning) version, instead of one column per occurrence.
- **Strike Out Deleted Records** — renders deleted records and cell references with strikethrough text, making them visually distinct from active content.

## Toolbar Search

The toolbar provides three filter controls that compose together: a record is shown only if it satisfies every active control at once. Enabling Conflicts Only while an advanced filter and a search are both active narrows the tree further rather than replacing either of them.

**Conflicts Only** is a quick preset that restricts the navigation tree to records touched by multiple plugins — those with a conflict or a benign override. It controls only the conflict dimension and combines with any active advanced filter or search.

The toolbar search field filters the navigation tree by record ID or display name. Type a query, select which fields to search with the toggle buttons, and press Enter to apply. The search combines with Conflicts Only and any advanced filter criteria already in effect.

- **Aa** — case-sensitive matching. When off, the search ignores letter case.
- **.\*** — interpret the query as a regular expression.
- **ID** — search in the record's internal ID (e.g. "iron_dagger", "balmora_guild").
- **Name** — search in the record's display name (e.g. "Iron Dagger", "Balmora Mages Guild").
- **Advanced Filters...** — opens the advanced filter dialog for filtering by conflict severity, per-plugin conflict status, record type, deleted status, and Lua handler criteria. The dialog opens pre-populated with the advanced criteria currently in effect, so adjustments build on the existing selection rather than starting from scratch.
- **No Filters** — a checkable toggle that shows whether any filters are active. When unchecked (filters are active), clicking it clears Conflicts Only, the search field, and the advanced filter in one action, returning the navigation tree to showing every record. When already checked, clicking it does nothing.

Press Escape to clear the search field and remove the text filter.

## Edit Panel

The Edit panel at the bottom of the window serves two purposes: text comparison and field editing.

When you click a cell in the record view that has a conflict with a previous column, the Edit panel shows both values side by side with character-level diff highlighting. Deleted text appears with a red background on the left, inserted text with a green background on the right.

Editing is off whenever the application starts. Turn it on with the Enable Editing button on the toolbar; the choice is not remembered, so each new session begins with editing disabled to guard against accidental changes.

When editing is enabled (via the Enable Editing button on the toolbar), clicking a decoded field in any plugin's column activates the Edit panel as an editor. The right pane becomes editable and an Apply button appears. For enum fields (race, class, type), a dropdown selector shows all valid values. For flag fields (NPC flags, cell flags), the dropdown presents checkboxes for each flag bit. Free-text fields such as names and IDs accept direct text input. The panel validates the input against the field's constraints — numeric range, string length, and codepage encoding limits. When the value is invalid the field is marked red and the reason is shown next to the Apply button. The Apply button stays disabled until the value is both valid and different from the original. Clicking Apply updates the loaded plugin held in memory and refreshes the record view to reflect the new state. It does not write the plugin file at this point; the change is kept until you choose to save it.

A plugin with changes that have not yet been written to disk is marked with an asterisk next to its name in the navigation panel, and the window title also shows an asterisk while any loaded plugin has unsaved changes. This gives you a clear view of which plugins have pending edits, so you can make several changes and decide when to commit them.

To write a plugin's pending changes to disk, right-click that plugin in the navigation panel and choose **Save**. This option is available only while the plugin has unsaved changes. Saving writes the plugin file and removes its asterisk. The File menu offers **Save** to write the currently selected plugin and **Save All** to write every plugin with unsaved changes at once. Text you have typed into a field but not yet applied is not saved; only changes you confirmed with Apply are written.

## History

The History tab sits beside the Edit and Log tabs at the bottom of the window. It lists the changes you have made during the current session, most recent first: each field edit you applied and each record you removed from a plugin. Every line shows the time of the change, the plugin it affected, and a short description of what changed.

The history is a record of what you did this session only. It is not written to disk, it is not restored the next time you open the application, and it clears when you unload all plugins. It is a reference for reviewing your recent edits, not an undo mechanism.

## Creating a Merged Patch

Click **Create Merged Patch** in the toolbar to run the automatic merge. If any loaded plugin has unsaved field edits, you are first offered to save those plugins or cancel the merge. The merge uses the current on-screen state of each plugin, so saving first keeps the files on disk consistent with what goes into the merged patch. Cancelling stops the merge and changes nothing.

The auto-merge performs several operations:

- **Leveled list merge** — combines entries from all plugins that modify leveled item or creature lists. No entries are lost; duplicates are removed.
- **Three-way record merge** — for object records modified by multiple plugins, compares each plugin's changes against the master. Non-conflicting field changes from different plugins are combined into one record.
- **Bug fixes** — optionally corrects known engine bugs: fog density values outside valid range, summon persistence flags, and cell name reverts.

After auto-merge completes, the merged patch is saved automatically. The output location depends on how you loaded plugins: same folder for Open Folder, MO2 overwrite directory for Open MO2 Profile, or the OpenMW data directory for Open OpenMW Config.

You can refine the auto-merge result manually. Use the record view context menu to copy individual sub-records from any plugin column into the merged patch, or remove sub-records that shouldn't be there. Changes are saved immediately.

A merged patch can be created even with a single plugin loaded. With one plugin there is nothing to merge automatically, so the patch starts empty; it still gives you a merged-patch column to copy records into by hand, which is a convenient way to build a small patch from one mod. The empty patch is written to disk like any other, and it gains its master references as you copy records into it.

## Settings

Open Settings via Ctrl+, or the Tools menu. Four pages are available:

- **Appearance** — choose between light and dark theme, and set the text codepage used to display plugin text. Choose Windows-1250 for Polish and Central European plugins, Windows-1251 for Russian, or Windows-1252 for English and other Western languages. The codepage applies to the navigation tree, the record view, and the Edit panel. Changing it updates the navigation tree and record view right away; the Edit panel refreshes the next time you select a record cell. Plugin files carry no encoding marker, so pick the codepage that matches the language of the plugins you are inspecting; choosing the wrong one makes accented or non-English characters appear as replacement symbols.
- **Output Paths** — configure the merged patch output path for each loading mode (folder, MO2, OpenMW). Normally these are automatic and don't need changing.
- **Merged Patch** — three sub-tabs control how auto-merge behaves:
  - **Exclude Sub-Records** — a list of sub-records excluded from conflict detection and the merged patch. Each entry uses `RECORD:SUB` format (e.g. `CELL:NAM0`). Use `TYPE:*` to exclude an entire record type. Add entries via the input field or right-click a sub-record row in the record view and choose "Exclude Sub-Record."
  - **Exclude by ID** — a list of regular expression patterns matched against record IDs. Records matching any pattern are skipped entirely during auto-merge.
  - **Fixes** — toggle individual bug fixes applied during merge: fog density correction, summon persistence flag, and cell name reversion prevention.
- **Cleaning** — toggle which cleaning operations the Clean All button performs. Evil GMSTs are Construction Set artifacts from Tribunal/Bloodmoon that can cause issues in mods that don't require those expansions. Junk cells are empty exterior cell records that only contain position data and serve no purpose. The Header Repair group provides additional fixes applied during cleaning: updating master file sizes in the plugin header to match the actual file sizes on disk, and updating the plugin version field to 1.3 (required by some engines).

## Cleaning Plugins

Click **Clean All** in the toolbar to remove known problematic records from all loaded plugins (except masters). Cleaned plugins are written to the output directory alongside the merged patch. The original plugin files are never modified.

Two types of records are removed:

- **Evil GMSTs** — game settings injected by the Construction Set when editing plugins with Tribunal or Bloodmoon loaded. These settings override expansion-specific values and can cause problems for players without the expansions.
- **Junk cells** — exterior cell records that contain only a NAME and DATA sub-record with no references, no region assignment, and no meaningful content. These are Construction Set artifacts from brief edits near cell borders.

When loading via Open MO2 Profile, cleaned plugins are written to the MO2 overwrite folder. The overwrite folder has the highest priority in MO2's virtual filesystem, so reloading the same profile after cleaning will automatically use the cleaned copies instead of the originals. Plugins loaded from overwrite are marked with the ⚡ icon in the navigation tree. Running Clean All again on an already-cleaned profile will report "no records to clean" because the loaded files are already the cleaned versions.

## Lua Handler Conflicts

After plugins are loaded, the application scans all `.omwscripts` files in the data paths for OpenMW Lua handler registrations. It identifies cases where multiple mods register handlers on the same interface method (e.g. two mods both adding an `ItemUsage.addHandlerForType` for the same item type).

The navigation panel has two tabs: **Plugins** (the ESP/ESM record tree) and **Lua** (handler registrations). After a scan completes, the Lua tab shows registrations grouped by mod name. Each registration shows the interface, method, and type argument. Registrations involved in a conflict are colored by severity:

- **Red** — blocking conflict. One handler returns false (cancels the action) and another mod expects the action to proceed.
- **Orange** — mutating conflict. Multiple handlers modify the same data in potentially incompatible ways.
- **Green** — overlapping registration. Multiple mods register on the same hook but their behaviors are compatible.

Clicking a conflicting registration displays all participating mods side by side in the record view, with cell-level highlighting on fields that differ between mods (same coloring style as ESP record conflicts).
