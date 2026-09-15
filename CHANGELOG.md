# Changelog

## [XXX]

### yTranslator
- [CHANGE] Copy Original (F8), Set In Progress (F9), Set Translated (F10), and Set Untranslated (Del) now apply to every selected row instead of only the active one, and all four appear in the Records context menu with their shortcut keys shown; Set Untranslated clears the translation and sets the status to Untranslated, the same as pressing Del
- [FIX] Converting a plugin now rewrites compiled script data correctly, so translated scripts run in the original Morrowind engine and open in the Construction Set without errors. Message boxes, say subtitles, and choice options keep their exact translated text (including multi-line messages that use the vertical bar), message-box buttons are translated, and a topic reference is no longer confused with an identically named creature or object elsewhere in the same script
- [CHANGE] The Statuses tab counts now reflect the active type filter, showing the tally within that subset (for example the counts for a soloed CELL type) instead of always counting the whole dictionary; the counts are not narrowed by the status selection or the search field

### yEditor
- [NEW] Build a plugin by hand into any target, not only the merged patch: one loaded plugin is the active plugin (marked with an [Active] label in the navigation tree and record view) and receives copied records. New Plugin in the toolbar creates an empty plugin and makes it active; Set as Active Plugin in a plugin's right-click menu switches the target. The merged patch is the active plugin by default, so existing workflows are unchanged. Only one plugin is active at a time, and switching first offers to save the current one.
- [NEW] Lock an entire record directly from the navigation tree: right-click a record under the merged patch and choose Lock in Merged Patch to freeze the whole record, or Unlock in Merged Patch to release it
- [NEW] Fields that must merge together as a pair are marked in the record view with a 🔗 link icon before both of their names, showing that the two values always come from the same plugin during a merge and are never split. This covers a creature's attack damage minimum and maximum, a weapon's chop, slash, and thrust damage minimums and maximums, and the magnitude minimum and maximum of enchantment, spell, potion, and ingredient effects
- [NEW] Show Optional Fields (View menu): adds a placeholder row for each single-occurrence sub-record a record can have but currently does not, drawn on a white background with light grey text, so you can see which fields are available to add. The choice is remembered between sessions
- [NEW] While editing a field, the Edit panel shows its accepted range next to the Apply button (for example "Range: 0 to 255" or "Range: up to 32 bytes"), so you know the limits before you type
- [CHANGE] The navigation tree now has two columns — ID and Name. Plugin role icons appear before the plugin filename in the ID column
- [CHANGE] A plugin now shows a single role icon instead of a stack: the highest-priority role wins, in the order guard, merged patch, overwrite copy, then file type (so the merged patch no longer shows the ⚡ overwrite marker). The active-plugin [Active] label is still shown after the role icon
- [CHANGE] Copy and remove record-view and navigation actions now read "Active Plugin" instead of "Merged Patch", since they target whichever plugin is active (the merged patch by default); locking remains available only on the merged patch, because a lock protects a value from the next auto-merge
- [CHANGE] Locked content is now shown by color instead of an icon: locked cells and locked records use a light blue background with blue text, and the 🔒 lock glyph has been removed from both panels. Content excluded from the merged patch is no longer marked at all — excluded plugins, records, and sub-records keep their normal conflict coloring, since exclusion only affects what the merge writes, not how content is shown
- [CHANGE] Record and navigation context menus are now ordered consistently in three bands: setters and copy actions first, then the reversible toggles (lock/unlock and exclude/include), then destructive actions last — Save and record removal — each band separated by a divider
- [CHANGE] The window title now shows the active plugin's file name (for example "yEditor - New Plugin.esp"), updating whenever you switch, create, load, or unload the active plugin; the unsaved-changes asterisk still appears after the name
- [CHANGE] The record header's flags are now shown as separate Persistent and Blocked rows, each reading Yes or No like other flags, instead of a single combined text line, so each flag can be compared per plugin
- [CHANGE] Inventory and spell lists now merge as a true three-way merge: entries added by any plugin are kept, changes follow load order (later plugins win), and an entry removed from the master by a plugin stays removed in the merged patch. Previously all entries were simply combined and removals were ignored
- [CHANGE] The record view now shows sub-records in a fixed order shared across every record type: the record header, then simple single-value fields (with any optional placeholder rows in their usual position), then data blocks that expand into sub-values, then repeating content such as inventory items, spells, and body-part slots. Because the order follows one shared sequence, a sub-record common to several record types (such as the display name or model path) always appears in the same relative position
- [CHANGE] The Advanced Filters dialog now lists record types by their readable names (Cell, Creature, Dialogue Response) matching the navigation tree, instead of the raw four-letter codes
- [CHANGE] The toolbar button that creates an empty active plugin is now labelled "Create New Plugin" (previously "New Plugin")
- [CHANGE] A field's validation error now appears in the Edit panel next to the Apply button, prefixed "Error:", instead of in the status bar at the bottom of the window
- [CHANGE] A leveled list's entry Count is no longer editable, since the game recomputes it automatically; selecting it shows "Auto-calculated, not editable" next to the Apply button
- [CHANGE] A record's ID field is no longer editable, since it identifies the record and renaming it in place would break references; selecting it shows "Record ID, not editable" next to the Apply button
- [CHANGE] Landscape (LAND) records are never written to the merged patch and their fields cannot be edited, since landscape is bulk terrain data that the merge cannot combine meaningfully; selecting a landscape field shows "Landscape data, not editable". Landscape records still appear in the navigation tree so conflicts remain visible
- [CHANGE] Pathgrids, regions, scripts, and dialogue topics with their responses are no longer combined by the automatic merge, matching landscape: their content cannot be merged field by field, so they are left to load order. These records still appear in the navigation tree and can be copied into the active plugin by hand
- [CHANGE] A global variable's Name column in the navigation tree now reads its type as Short, Long, or Float instead of the raw single-letter code
- [CHANGE] Everything left out of the merged patch is now managed in one Excludes table under Merged Patch settings, replacing the separate sub-record and record-ID lists. Each row targets a plugin file, a record ID (regular expression), a whole record type, or a single sub-record. Right-clicking a record or sub-record in the record view still adds the matching rule
- [CHANGE] Excluding a sub-record or record type no longer hides it from conflict detection — conflicts are always shown for what the plugins actually contain, and exclusion only keeps the content out of the merged patch. The previous defaults that suppressed a cell's object count and a land texture's index as conflicts have been removed, so those differences now appear as normal conflicts
- [FIX] The navigation tree no longer jumps or scrolls away from where you were working when you exclude, include, guard, save, lock, or unlock — the scroll position and expanded state stay put
- [FIX] An unused skill or attribute slot in a record (such as a race's empty bonus-skill slots) now reads "None" in the record view instead of the raw number 4294967295
- [FIX] When one plugin defines an NPC with auto-calculated stats and another with full stats, the record view now lines the two up field by field: shared fields (level, disposition, reputation, rank, gold) compare correctly and the auto-calculated fields read "Auto" for the auto-calc version, instead of showing mismatched or garbage values
- [FIX] An NPC or creature's travel destination now reads "Travel Destination" in the record view instead of "Door Destination", and its destination cell is no longer mislabelled "Hair Model"
- [FIX] Locking a group now covers exactly the group members you selected, following the same rules as copying a group; the Lock and Unlock options are greyed out when the right-clicked cell has nothing that can be locked
- [FIX] Locking a text field — such as a name, model path, script, book text, or dialogue value — now actually holds its value through a re-merge; previously locking these variable-length fields appeared to work but the value was not preserved when the merged patch was regenerated
- [FIX] The Edit panel's left comparison pane now shows the value from the nearest earlier plugin that actually defines the clicked field, skipping columns that leave it empty, instead of only the immediately preceding column which could be blank
- [FIX] A deleted-record marker in the record view now reads "DELETED" instead of showing 0 or a raw value
- [FIX] Cell records now merge into the merged patch as a true three-way merge: cell settings such as water, sleep, and lighting merge per setting, and object references are combined so a reference added by any plugin is kept. Each placed object is treated as a whole — when plugins change the same reference, the last plugin's version of that object wins in full, so a reference is never left combining position from one plugin and ownership or lock state from another. Previously the merged patch simply took the last plugin's version of a cell, dropping every other plugin's cell changes
- [FIX] The plugin-loading progress bar now starts empty and fills as each plugin loads, instead of briefly showing as full at the start
- [FIX] The difference highlighting in the Edit panel comparison now lines up with the text it marks, instead of being shifted onto the wrong characters
- [FIX] Flag bits on a cell's placed objects (such as a door or lock flag) now read Yes or No in the record view, matching every other flag, instead of showing 1 or 0
- [FIX] When merging a spell, enchantment, or potion effect whose magnitude was changed by more than one plugin, the effect's minimum and maximum magnitude are now taken together from the same plugin. Previously the merge could pair the magnitude minimum with the effect's duration instead, producing a wrong magnitude range in the merged patch
- [FIX] Race records now merge field by field, so a race's skill bonuses, attributes, height, weight, and flags from different plugins are combined instead of the last plugin's entire race data replacing the rest
- [FIX] Faction reactions now merge by faction rather than by position, so reactions listed in a different order across plugins no longer drop or get mismatched; a changed reaction value follows load order and a reaction removed by a plugin stays removed
- [FIX] An NPC whose stats a plugin changed now carries that change into the merged patch, including when the change switches the NPC between auto-calculated and full hand-set stats. The stat block is resolved like any other field, so the last plugin in load order to change it wins instead of the change being dropped
- [FIX] The Edit panel's difference highlighting now shows on editable fields as well as read-only ones, and updates as you type. Differences are marked character by character — the removed characters on the left pane and the added characters on the right — instead of highlighting whole lines
- [FIX] Leveled list entries now merge per item instead of treating each item-and-level pair as separate: when a plugin changes an entry's PC level, or changes how many times an item appears, that change is applied with the last plugin in load order winning, instead of the old value being kept or the entry appearing twice. Items intentionally repeated for spawn weighting are preserved
- [FIX] Flag fields now merge one bit at a time, so changes to different flags of the same field made by different plugins are combined instead of the last plugin's whole set of flags overwriting the rest. This includes a leveled list's calculation flags (Calc for Each Item, Calc from All Levels) and the flag fields of NPCs, creatures, and containers
- [FIX] A book or scroll's skill is now read correctly in the record view (it was previously misread from the wrong bytes)
- [FIX] Start Script and Land Texture records now show their fields (script data, texture index and path) in the record view instead of raw bytes
- [FIX] Interior cells no longer show Grid X and Grid Y in the record view, since those coordinates only apply to exterior cells; the interior cell's data previously displayed meaningless numbers in those rows
- [FIX] Conflicting values are now colored in the record view even when only one plugin and the merged patch define a record; previously a differing merged-patch value showed in the plain no-conflict color instead of the conflict colors

### Both Apps
- [CHANGE] The log panel no longer wraps long lines; instead it scrolls horizontally, so each log entry stays on one line

## [0.1135] - 2026-09-04

### yTranslator
- [NEW] Added Spanish and Portuguese, plus Czech, Slovak, Slovenian, Croatian, Romanian, Ukrainian, Bulgarian, Serbian, Dutch, Swedish, Danish, Norwegian Bokmal, Finnish, Catalan, and Galician as target languages; Finnish can be translated but has no spell-check dictionary
- [NEW] Apply Topic Tags in a dictionary's right-click menu: wraps dialogue topics found in the text of dialogue responses in hyperlink tags so they appear as clickable links in-game. It also tags inflected forms of topics (read from the dictionary's generated .top file) where they do not collide with a direct topic link. All dialogue responses are tagged regardless of their status, excluding voice lines. Changes are recorded in history and can be reverted, and re-running refreshes the tags rather than duplicating them
- [NEW] Remove Topic Tags in a dictionary's right-click menu: strips all topic hyperlink tags from dialogue responses; the change is recorded in history and can be reverted
- [NEW] While editing a dictionary, the Annotations tab lists the inflected topic forms (from loaded .top and .mrk files) related to the current entry's translation, under an Inflection section. When any form of a topic appears in the text, every known form of that topic is listed, not only the one present. The tab is now a two-column table showing each annotation and its source file, with resizable columns
- [NEW] Editable AI translation prompt: a Prompt tab in the Auto Translation settings holds a single system prompt shared by all AI providers, with a Reset to Default button to restore the built-in prompt. The prompt is a template with placeholders for the source and target languages, the marked examples, and the known dialogue topics, so you control where each part appears. A read-only Preview tab shows the full prompt as it will be sent, with the languages and examples filled in
- [NEW] Toggle buttons beside the Next button control what is marked in the translation editor: H, I, and G turn hyperlink, inflection, and glossary highlights on or off, and S, Gr, and W toggle spell check, grammar check, and whitespace markers. Turning a highlight off lets a lower-priority one take over the same word — for example disabling hyperlinks reveals the inflection highlight underneath. The S, Gr, and W buttons stay in sync with the matching View menu items, and the highlight choices are remembered between sessions
- [NEW] Language settings show whether the native spell-check dictionary loads and which one, so a missing or broken dictionary is visible at a glance
- [CHANGE] Inflected topic forms are highlighted in the editor with their own color, distinct from dialogue topics and glossary terms
- [CHANGE] Localization files (.cel, .top, .mrk) no longer show the Status column or the Statuses filter, since their entries have no status
- [CHANGE] Language settings now list the foreign and native languages in alphabetical order
- [CHANGE] The Encoding line in Language settings is now shown in the normal text color instead of a greyed-out note
- [CHANGE] Sync Scrolling now remembers its on/off state between sessions instead of always starting enabled
- [CHANGE] The History panel now lists the selected entry's changes as a plain list; right-click a change and choose Revert to restore it, instead of a Revert button on every row
- [CHANGE] Grammar issues in the translation editor are now marked with a wavy underline, like spell check (amber for grammar, red for spelling), instead of a background highlight
- [CHANGE] The over-the-byte-limit part of a translation now uses the same highlight color as forbidden characters, so all validation problems share one color
- [CHANGE] The @ character is no longer treated as a forbidden character, so translations containing topic tags no longer show a validation error
- [CHANGE] The Tools menu entry and its dialog title "Merge Dictionaries" are now named "Dictionary Merger"
- [CHANGE] Hyperlink and glossary annotations are now built only from entries with the Translated status, so unverified translations no longer contribute topic links or glossary terms
- [CHANGE] When editing a localization file, the two columns are now labelled for the file type: Cell and Translation for .cel, Topic Form and Topic for .top, and Translation and Topic for .mrk
- [CHANGE] History entries now show the status in square brackets to match the timestamp style
- [FIX] The validation status now states the reason when a translation is invalid (for example the byte limit it exceeds, or the forbidden character it contains), instead of only the character count
- [FIX] A translation is now propagated to other entries with the same source text even when the translation is identical to the original (a proper noun), while entries that already hold that text are left untouched
- [FIX] An invalid translation (containing a forbidden character or exceeding the byte limit) is marked with the Error status and is no longer propagated to other entries; propagation also skips any entry whose record type cannot hold the text within its own byte limit
- [FIX] Topic tag markers in the editor are no longer shown as underlined colored web-style links; the tagged text now renders normally while recognized topics keep their background highlight
- [FIX] Grammar and topic highlights in the translation editor now refresh correctly when text is edited back to its original value — removing a double space now clears the grammar mark, and reverting a broken topic tag to a valid form restores its highlight

### yEditor
- [NEW] Sync Scrolling: a View menu toggle binds scrolling between the two comparison panes in the Edit panel, so both sides move together; the state persists between sessions
- [NEW] When a field edit is invalid, the reason is shown in the status bar instead of only marking the field red
- [NEW] Remove Record from Plugin: right-click a record in a loaded plugin to delete it from that plugin; the record is dropped when the plugin is saved. This cannot be undone. The option is greyed out unless editing is enabled
- [NEW] Copy Record to Merged Patch is now also available by right-clicking a record in the navigation tree, copying the whole record into the merged patch
- [NEW] Copy Bit to Merged Patch: right-click an individual flag (such as a body part's Female flag) to copy just that one bit into the merged patch, leaving the record's other flags untouched
- [NEW] Selecting a response under the Dialogue Responses group now shows that response text in the Edit panel comparison, instead of clearing the panel
- [NEW] Each dialogue condition now reads as a plain sentence (for example "Function Choice == 1"), with the raw condition text shown and editable underneath
- [NEW] History tab: a tab beside the Edit and Log panels lists the field edits and record removals made during the current session, newest first. The history is not saved and resets when plugins are unloaded
- [NEW] Diff button in the Edit panel toggles the difference highlighting between the two comparison panes. When on, lines that changed between the plugins are highlighted (removed text on the left, added text on the right); when off, both panes show plain text. Differences are compared ignoring leading indentation, so a script that differs only in spacing shows as unchanged while each pane keeps its original layout
- [NEW] The main window now opens immediately at startup, with a progress dialog shown while the previous session is restored and while plugins are loaded or a merged patch is created, instead of a blank wait until everything is in memory
- [NEW] Lock in Merged Patch: right-click a cell in the merged patch column to lock a whole record, a sub-record, a decoded field, or a single flag bit. A locked cell shows a lock icon, keeps its current value, and is re-applied unchanged after you regenerate the merged patch, so the auto-merge cannot overwrite it. Locks are remembered between sessions. Right-click again to unlock
- [CHANGE] Direct editing of loaded plugins is now turned on from a new Editing page in Settings (with a warning that it can break a plugin) instead of the toolbar button, and the choice is remembered between sessions. The merged patch remains editable at all times regardless of this setting
- [CHANGE] The Lua tab's conflict list is now a two-column tree, showing the handler as interface and method in the first column and its type argument in the second, and it starts collapsed so only the mod names are shown
- [CHANGE] Lua handler conflicts now use the same conflict colors as plugin records, with the color carried up to the mod name so a conflicting mod is visible without expanding it
- [CHANGE] Selecting a Lua handler conflict now shows only its Classification and Handler Body in the Edit panel, colored by the conflict severity; the interface, method, type, script path, and callback rows are no longer listed since they added no comparison value
- [CHANGE] The dialogue INFO chain now shows each plugin's response text for every INFO and highlights conflicts where plugins give the same INFO different text, instead of only showing a checkmark for presence
- [CHANGE] A merged patch can now be created with a single plugin loaded, instead of requiring at least two. With one plugin the patch starts empty and serves as a scaffold to copy records into by hand
- [CHANGE] Sub-record row labels in the record view no longer repeat the record type; a record's identifier and name rows now read "NAME - ID" and "FNAM - Name" instead of "NAME - Activator ID" and "FNAM - Activator Name"
- [CHANGE] A flag sub-record's individual flags now appear directly under the sub-record row in the record view, without a redundant intermediate "Flags" grouping row
- [CHANGE] A data sub-record that decodes to a single field now shows that field as a nested row under the sub-record instead of printing the value inline on the sub-record row
- [CHANGE] The dialogue response list is now a single collapsible "Dialogue Responses" group in the record view instead of a flat separator followed by loose rows
- [CHANGE] The status bar now reads "mode | path | plugin" (for example "MO2 | ...profiles/Default | Morrowind.esm") and no longer appends the selected record type and id
- [CHANGE] Each faction reaction now appears as a single collapsible "Faction Reaction" group in the record view, pairing the reacting faction with its reaction value, instead of separate loose rows
- [CHANGE] Sub-records that repeat within a record (such as a container's items and an NPC's spells) are now numbered in the record view, so each row reads "NPCO - Item #0", "NPCO - Item #1", and so on; sub-records that appear only once are left unnumbered
- [FIX] The Edit panel comparison now leaves a pane blank when the corresponding plugin has no version of the selected sub-record, instead of showing stray placeholder characters
- [FIX] Merging a record where two mods change different parts of the same multi-byte value (such as a weapon's stat) no longer produces a spliced value that neither mod set; each value is now taken whole from one mod
- [FIX] Merging a faction where two mods change different rank requirements now keeps both changes, combining the requirement data field by field instead of taking the whole block from a single mod
- [FIX] Copy Field to Merged Patch now works for fields shown under a grouping heading (such as a class's major and minor skills); previously the copy silently failed for those fields
- [FIX] Faction reaction values now line up by faction across plugin columns in the record view, instead of being matched by position so different factions shared a row
- [FIX] Editing a dialogue condition now updates that condition in place instead of adding a duplicate condition
- [FIX] The merged patch is now always loaded last, regardless of where MO2 or OpenMW places it in the load order, so its column stays at the far right and it wins over the plugins it merges
- [FIX] When several plugins each change the same field of a record and the last plugin reverts it to the original, the merged patch now keeps the change from the latest plugin that made one, instead of the earliest
- [FIX] Armor and clothing body-part records now merge correctly: a female or male body-part name added by one plugin is carried into the merged patch in its own body-part slot, instead of being dropped or attached to the wrong body part
- [FIX] Creature attack damage values (minimum and maximum for each of the three attacks) now merge as whole values, so a merged creature no longer ends up with a corrupted attack range
- [FIX] The Edit panel Diff highlighting now colors both comparison panes as soon as a record is selected, instead of coloring only the left pane until the Diff button was toggled off and back on

## [0.1060] - 2026-08-31

### yEditor
- [CHANGE] Merged-patch context menu actions renamed for clarity: "Remove Sub-Record", "Remove Group", and the navigation tree's "Remove" are now "Remove Sub-Record from Merged Patch", "Remove Group from Merged Patch", and "Remove Record from Merged Patch"
- [FIX] Text with accented or non-English characters (such as the curly quotes in book text) now renders correctly using the codepage chosen in settings, instead of showing replacement symbols
- [FIX] Changing the text codepage in settings now updates the navigation tree and record view immediately; the Edit panel updates the next time a record cell is selected
- [FIX] Clean All now works with a single loaded plugin, instead of wrongly reporting that two plugins are required
- [FIX] Excluding a sub-record now affects only top-level sub-records; a rule like `CELL:DATA` no longer also strips the matching sub-record from every referenced object inside cells during merge
- [FIX] Scanning for Lua handler conflicts no longer crashes when a mod contains files or folders whose names use accented, Cyrillic, or other non-English characters; such scripts are now read correctly, and an unreadable folder is skipped instead of aborting the scan

## [0.1054] - 2026-08-30

### yTranslator
- [NEW] Enchantment annotation: FNAM entries for weapons, armor, clothing, and books now show the enchantment ID in the Annotations panel when the item is enchanted
- [NEW] Revert from record table context menu: right-click selected entries → Revert restores previous text and status from history
- [NEW] Encoding line in Language settings: shows which codepage the selected native language uses for plugin text
- [NEW] Mark records as AI translation examples: right-click records to mark up to twenty as style examples sent to the AI provider
- [NEW] Fetch available models from the provider: a Refresh control in the Auto Translate panel pulls the current model list from the provider
- [CHANGE] Settings: "Providers" page renamed to "Auto Translation" with three tabs — Local Models, Web Providers, and Examples
- [CHANGE] Model selection moved from Settings into the Auto Translate panel
- [CHANGE] Find & Replace moved from the filter toolbar into its own tab in the left panel, after Statuses
- [CHANGE] History tab moved to the end of the bottom-left tabs; each history entry now shows its status and timestamp
- [CHANGE] Find & Replace no longer has a batch Undo button — each replacement is recorded in edit history and revertable per-entry
- [CHANGE] Localization documents (.top, .mrk, .yaml) no longer show the ID and Key columns, since those entries have no record type or key — only Original, Translation, and Status are shown
- [FIX] Web translation providers that send form-encoded requests now work — the request was previously rejected for a missing content type
- [FIX] Numeric web provider settings (such as a temperature value) are now sent as numbers, so providers that require a numeric field no longer reject the request
- [FIX] Quoted values in localization YAML files now decode \n and \t escapes into real line breaks and tabs instead of showing them as literal characters
- [FIX] Localization YAML block scalars written with the keep indicator (|+) are now read correctly instead of being treated as broken entries

### yEditor
- [NEW] Enable Editing: a single toolbar toggle makes decoded fields editable in the Edit panel for all plugins (combobox for flags and enums); starts disabled each time the app opens
- [NEW] View menu: Toggle Sidebar and Toggle Bottom Panel hide or show the navigation and edit/log panels; state persists across sessions
- [NEW] Lua handler conflict detection in a separate Lua tab: scans OpenMW Lua scripts and highlights conflicting handler registrations between mods
- [NEW] Exclude Sub-Record from context menu: right-click any sub-record row to add it to the exclusion list
- [NEW] Toolbar search: filter the navigation tree by record ID or display name with case-sensitive and regex support
- [NEW] No Filters button on the toolbar: clears Conflicts Only, the search field, and the advanced filter in one click
- [NEW] Save edited plugins on demand: right-click a plugin to save it, or use Save / Save All in the File menu; plugins with unsaved changes are marked with an asterisk in the plugin list and the window title
- [CHANGE] Filter dialog moved from View menu to toolbar button
- [CHANGE] Settings reorganized: Sub-Record Rules merged into Merged Patch page, Paths renamed to Output Paths, record type checkboxes replaced by TYPE:* syntax
- [CHANGE] File Header shown as flat entry at the top of each plugin (no nesting)
- [CHANGE] Unknown/binary sub-records now display hex bytes instead of placeholder
- [CHANGE] View menu: "Hide Duplicate Columns" renamed to "Show Only One Column Per Plugin", "Show Deleted Strikeout" renamed to "Strike Out Deleted Records" (disabled by default)
- [CHANGE] Conflicts Only, Advanced Filters, and search now compose: all active filters combine as a logical AND instead of overwriting each other
- [FIX] Record view context menu now works when right-clicking the label column
- [FIX] A plugin can no longer be both a guard patch and excluded from the merged patch — setting one clears the other
- [FIX] Edit panel now shows the full multi-line content of text sub-records (book TEXT, script SCTX/BNAM) instead of only the first line
- [FIX] Decoded fields inside CELL reference groups (X/Y/Z Position, rotations, door destinations) can now be edited when Enable Editing is active
- [FIX] Decoded fields grouped under a heading (creature stats attributes, stats, skills and attacks, and similar grouped fields) can now be edited when Enable Editing is active
- [FIX] Empty sub-records (e.g. FNAM with no display name) can now be edited — previously the empty value blocked the edit panel
- [FIX] Exclude Sub-Record context menu no longer adds duplicates if the rule already exists
- [FIX] Excluding a sub-record from context menu now immediately greys out the row in the record view
- [FIX] Excluded sub-records now grey out their decoded children (fields, flags) as well
- [FIX] Dialogue conditions (INFO records) now read as a single line such as `Journal "IL_TalosTreason" >= 10`, grouped under a numbered Condition entry; the condition type and comparison operator show as words (Journal, Dead, Local, `==`, `>=`), function conditions show the function name, variable conditions show the variable storage type, and the comparison value appears as a sub-record, instead of raw hex bytes

### Both Apps
- [NEW] Linux support: builds with CMake and system libraries, AUR package available
- [NEW] Cross-platform resource paths: shared data in `/usr/share/yampt/`, user data in `~/.yampt/`
- [FIX] Disabled widgets now show greyed-out text on all platforms
- [FIX] Workspace sidebar no longer shows a duplicate "Workspace" node when paths differ only by trailing slash
- [FIX] Converting a plugin whose master reference or file name has no extension no longer crashes
- [FIX] Converting a malformed plugin where a dialogue response appears before its topic no longer produces corrupted output
- [FIX] Cell names with accented or Cyrillic letters referenced in scripts are now translated correctly instead of being cut off at the first non-ASCII character
- [FIX] Case-insensitive search, glossary matching, and text highlighting now work for accented and Cyrillic letters, so a word matches regardless of the case of its non-ASCII letters

## [0.940] - 2026-07-29

### yTranslator
- [NEW] Find/Replace dialog (Tools menu) with regex, case sensitivity, and batch undo
- [NEW] "Replaced" status assigned to entries modified by Find/Replace
- [NEW] EET file import: export ESP-ESM Translator dictionaries to JSON (partial support)
- [NEW] Generate localization files (.cel, .mrk, .top) from dictionary with Hunspell inflection
- [NEW] Full script preview: selecting a script entry shows the entire script source in Preview tab
- [NEW] Script source stored as reference data in dictionaries for context lookup
- [NEW] Sync Scrolling: View menu toggle to bind scrolling between original and translation panes
- [NEW] Grammar check: quoted text highlighted in lighter color for visual distinction
- [NEW] Web translation providers: config-driven architecture (DeepL, Google, Claude via JSON config files)
- [NEW] Translation settings page: table showing all providers with API key fields
- [NEW] Translation engine: pre-substitution of glossary terms before sending to AI model
- [NEW] Translation engine: SCTX/BNAM tokenization — only translatable strings are sent to model
- [NEW] Inline table editing: double-click Translation column to edit single-line entries directly
- [NEW] Lua l10n workflow: auto-pairs foreign/native YAML files in the same directory
- [NEW] Lua l10n workflow: opening a native YAML allows editing, opening a foreign YAML shows read-only reference
- [NEW] Lua l10n workflow: save writes only translated entries to the native file
- [NEW] Lua l10n workflow: Make Translation creates a scaffold native file with all keys
- [NEW] Merge Dictionaries in Tools menu
- [NEW] View menu: Spell Check toggle
- [NEW] Spell check dictionaries: added de_DE, fr_FR, ru_RU, it_IT, hu_HU
- [NEW] First run dialog: added Italian and Hungarian languages
- [NEW] Language list externalized to `languages.json` — add new languages without recompiling
- [CHANGE] Status filter moved to a dedicated Statuses tab (same interaction as Filters tab)
- [CHANGE] Language settings: simplified panel (foreign/native language, spell check, tags)
- [CHANGE] Script editor: `say` keyword sound file path hidden from display and translation
- [CHANGE] Book Preview tab renamed to Preview
- [CHANGE] Translate tab renamed to Auto Translate
- [CHANGE] "Model" status display renamed to "Generated"
- [CHANGE] Filters and Statuses are now fully independent (no cross-reset)
- [CHANGE] YAML context menu: foreign files show "Make Translation", native files show "Save"
- [CHANGE] YAML files in workspace auto-loaded on startup
- [CHANGE] Translation button populates the translation editor with result
- [CHANGE] Translation button only works on untranslated entries
- [CHANGE] Translation button commits immediately and advances to the next row
- [CHANGE] Translation button works for YAML documents (not just dict)
- [CHANGE] Inline table editing advances to the next row after commit
- [CHANGE] Read-only documents disable editing actions in menus and shortcuts
- [CHANGE] Make Dict and Make Dict with Base merged into single "Make Dictionary" menu item
- [CHANGE] Make Base dialog: removed dictionary combo box (uses language settings)
- [CHANGE] Language settings: encoding and translation target auto-derived from language selection
- [CHANGE] Language settings: spell check auto-set to None when dictionary not found
- [CHANGE] All operation dialogs use consistent 450×400 initial size
- [CHANGE] Convert/Create: preserves original file timestamp
- [CHANGE] "Make Base" renamed to "Make Base Dictionary"
- [CHANGE] "Convert" renamed to "Convert Plugin"
- [CHANGE] "Create" renamed to "Create Patch Plugin"
- [FIX] Annotation highlight misalignment
- [FIX] EET import: raw SCTX script bodies no longer imported — only extracted translatable strings (MSGB, CELL, SAY, DIAL) are converted
- [FIX] Dictionary marked dirty on row click without editing
- [FIX] SCTX/BNAM validation: quotes no longer flagged as forbidden characters
- [FIX] Whitespace markers: newline indicator now visible at line breaks
- [FIX] Spell check: dictionary now loads correctly on startup
- [FIX] Grammar check: missing punctuation no longer flagged while cursor is at end of text
- [FIX] Propagation: entries with leading/trailing whitespace differences now match correctly

### yEditor
- [NEW] Dialogue INFO chain resolved using OpenMW ordering algorithm (PNAM-based insertion)
- [NEW] Plugin cleaning: removes evil GMSTs and junk cells from all loaded plugins
- [NEW] Header repair: update master file sizes in plugin headers to match actual files on disk
- [NEW] Header repair: update plugin version to 1.3
- [NEW] Cleaning settings page with toggleable removal options
- [NEW] Sub-Record Rules settings page for configuring conflict and merge behavior
- [NEW] Guard Patch: plugins before guard are excluded from merge for records the guard contains
- [NEW] Status bar shows load mode and path
- [CHANGE] Sub-record conflict rules applied at runtime (changes take effect immediately after settings)
- [CHANGE] Right panel columns redistribute on resize to fill available width
- [CHANGE] Nav tree decodes codepage characters correctly
- [CHANGE] Filter dialog: record type list unchecked by default (all unchecked = show all)

### Both Apps
- [NEW] Localization support: all UI strings wrapped with `tr()` for Qt translation system

## [0.842] - 2026-07-05

### yTranslator
- [NEW] Settings dialog (appearance, shortcuts, language, translation engine)
- [NEW] Workspace folder watches for file changes and refreshes automatically
- [NEW] Multi-layer highlighting (MWScript, hyperlinks, glossary, forbidden characters)
- [NEW] Merge dialog for combining dictionaries
- [CHANGE] Consistent syntax coloring across all editor panels

### yEditor
- [NEW] Automatic merged patch creation (leveled lists, dialogues, three-way object merge)
- [NEW] Fog fix, summon fix, and cell name fix applied automatically to merged patches
- [NEW] Copy records, groups, or individual fields to the merged patch via right-click menu
- [NEW] Remove records and groups from the merged patch via right-click menu
- [NEW] Decoded field view for CELL objects, leveled lists, containers, factions
- [NEW] Advanced filter dialog (conflict level, override status, record type, ID, name, deleted records)
- [NEW] Side-by-side text comparison panel
- [NEW] Settings dialog (appearance, paths, merge options)
- [NEW] Session persistence — remembers loaded plugins and window state between runs
- [NEW] Configurable merge: exclude plugins or records by pattern, toggle record types and automatic fixes
- [CHANGE] Conflict coloring now works at the individual field level (not just whole records)
- [CHANGE] Navigation tree inherits worst-case conflict color from children up to file level
- [CHANGE] Entries from different plugins aligned by content identity (item ID, object index, rank) instead of file order

### Both Apps
- [NEW] Dark mode

## [0.735] - 2026-06-21

### Both Apps
- [NEW] yTranslator: translation workbench with spell check, annotations, history, and translation suggestions
- [NEW] yEditor: plugin conflict viewer and merged patch creator (xEdit-like)
- [NEW] JSON dictionary format with per-entry status tracking
- [CHANGE] Rewritten from scratch as a Qt6 application suite
