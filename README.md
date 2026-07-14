# Yet Another Morrowind Plugin Tools

A suite of tools for working with Morrowind plugins. Includes a plugin conflict editor similar to xEdit, a translation workbench, and a CLI for batch operations.

## Applications

### yEditor.exe — Plugin Editor

Qt6 GUI for viewing, comparing, and patching plugins. Similar to TES5Edit/xEdit.

**Loading**
- Load plugins from folder, Mod Organizer 2 profiles, or OpenMW cfg
- Session persistence across restarts

**Conflict Detection**
- Navigation tree with conflict coloring inherited from children up to file level
- xEdit-style coloring by conflict severity and override status
- Conflict categories: no conflict, benign override, critical conflict
- Advanced filter by conflict level, override status, record type, ID, name, deleted records

**Record Comparison**
- Comparison view with decoded fields across all loaded plugins
- Field decoding for CELL objects, leveled lists, containers, factions, and more
- Entries from different plugins aligned by content identity instead of file order
- Side-by-side text comparison panel

**Merged Patch**
- Automatic merged patch creation for leveled lists, dialogues, and three-way object merge
- Three-way field-level merge for packed sub-records (NPDT, AIDT, WPDT, AODT, AI_W)
- Automatic fixes: fog density, summon persistence, cell name reversion
- Copy records, groups, or individual fields to merged patch via right-click menu
- Remove records and groups from merged patch via right-click menu
- Configurable merge: exclude plugins or records by pattern, toggle record types and fixes

**Interface**
- Dark mode
- Settings dialog

### yTranslator.exe — Translation Workbench

Qt6 GUI for interactive plugin translation.

**Workspace**
- Sidebar with workspace folders, auto-refresh on filesystem changes
- Session persistence and encoding selection
- EET file format import

**Editing**
- Record table with filtering by type, sub-type, and translation status
- Three-panel editor: original text, adapted text, editable translation
- Search with regex support
- Find and replace across the active dictionary
- Entry validation and status tracking
- History panel with undo/revert
- Keyboard shortcuts for common translation actions

**Highlighting**
- Multi-layer syntax highlighting for MWScript keywords, hyperlinks, glossary terms, forbidden characters
- Spell checking with per-language dictionaries
- Annotation system showing hyperlinks, gender info, and glossary matches
- Book content preview panel with live update while editing

**Machine Translation**
- Offline translation via CTranslate2 (NLLB-600M model)
- Supports Polish, German, French, Russian, Italian, Hungarian
- Batch translation of all untranslated entries

**Operations**
- All CLI operations accessible from the GUI (make dict, make base, convert, create, merge)

**Interface**
- Dark mode
- Settings dialog

### yampt.exe — Command Line

Batch tool for automated dictionary and conversion workflows.

**Dictionary Creation**
- Create base dictionaries by pairing two language versions of the same master file
- Create dictionaries from plugins using a base dictionary
- Merge multiple dictionaries

**Plugin Conversion**
- Convert plugins by applying translations from dictionaries
- Create patch plugins containing only modified records
- Converts compiled script data without needing to recompile in TES CS

**Format**
- JSON dictionary format with status tracking
- Handles all translatable record types
