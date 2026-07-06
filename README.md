# Yet Another Morrowind Plugin Tools

A suite of tools for working with Morrowind plugins. Includes a plugin conflict editor similar to xEdit, a translation workbench, and a CLI for batch operations.

## Applications

### yEditor.exe — Plugin Editor

Qt6 GUI for viewing, comparing, and patching plugins. Similar to TES5Edit/xEdit.

- Load plugins from folder, Mod Organizer 2 profiles, or OpenMW cfg
- Navigation tree with conflict coloring inherited from children up to file level
- Comparison view with decoded fields across all loaded plugins
- xEdit-style coloring by conflict severity and override status
- Field decoding for CELL objects, leveled lists, containers, factions, and more
- Entries from different plugins aligned by content identity instead of file order
- Conflict categories: no conflict, benign override, critical conflict
- Advanced filter by conflict level, override status, record type, ID, name, deleted records
- Automatic merged patch creation for leveled lists, dialogues, and three-way object merge
- Automatic fixes in merged patch: fog density, summon durations, cell name conflicts
- Copy records, groups, or individual fields to merged patch via right-click menu
- Remove records and groups from merged patch via right-click menu
- Configurable merge: exclude plugins or records by pattern, toggle record types and fixes
- Side-by-side text comparison panel
- Session persistence
- Dark mode
- Settings dialog

### yTranslator.exe — Translation Workbench

Qt6 GUI for interactive plugin translation.

- Sidebar with workspace folders, auto-refresh on filesystem changes
- Record table with filtering by type, sub-type, and translation status
- Search with regex support
- Three-panel editor: original text, adapted text, editable translation
- Multi-layer syntax highlighting for MWScript keywords, hyperlinks, glossary terms, forbidden characters
- Spell checking with per-language dictionaries
- Annotation system showing hyperlinks, gender info, and glossary matches
- History panel with undo/revert
- Find and replace across the active dictionary
- Entry validation and status tracking
- Book content preview panel with live update while editing
- All CLI operations accessible from the GUI
- Dark mode
- Settings dialog
- Session persistence and encoding selection
- EET file format import
- Keyboard shortcuts for common translation actions

### yampt.exe — Command Line

Batch tool for automated dictionary and conversion workflows.

- Create base dictionaries by pairing two language versions of the same master file
- Create dictionaries from plugins using a base dictionary
- Merge multiple dictionaries
- Convert plugins by applying translations from dictionaries
- Create patch plugins containing only modified records
- JSON dictionary format with status tracking
- Handles all translatable record types
- Converts compiled script data without needing to recompile in TES CS
