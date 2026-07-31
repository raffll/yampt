# Yet Another Morrowind Plugin Tools

A suite of tools for working with Morrowind plugins. Includes a plugin conflict editor similar to xEdit, a translation workbench, and a CLI for batch operations.

## yEditor.exe — Plugin Editor

Qt6 GUI for viewing, comparing, and patching plugins. Similar to TES5Edit/xEdit.

**Viewing**
- **Load and compare any number of plugins simultaneously — no hardcoded limits**
- Load plugins from folder, Mod Organizer 2 profiles, or OpenMW cfg
- Navigation tree with xEdit-style conflict coloring inherited up to file level
- **Record comparison with decoded fields aligned by content identity across plugins**
- Side-by-side text comparison panel
- Advanced filter: conflict level, override status, record type, ID, name, deleted records

**Merging**
- Automatic merged patch: leveled lists, dialogues, three-way field-level merge for packed sub-records
- Automatic fixes: fog density, summon persistence, cell name reversion
- **Guard patch: plugins before the guard are excluded from merge for records the guard overrides**
- **Dialogue INFO chain resolved using OpenMW ordering algorithm**
- Copy or remove records, groups, or individual fields to/from merged patch
- Configurable merge: exclude plugins or records by pattern, toggle record types and fixes

**Maintenance**
- Plugin cleaning: batch-remove evil GMSTs and junk cells
- Header repair: update master file sizes to match actual files on disk, update plugin version to 1.3

**General**
- Session persistence: remembers loaded plugins and window state between runs
- Dark mode

## yTranslator.exe — Translation Workbench

Qt6 GUI for interactive plugin translation.

**Editing**
- Sidebar with workspace folders, auto-refresh on filesystem changes
- Record table filtered by type, sub-type, and translation status
- Three-panel editor: original text, adapted text, editable translation
- Inline table editing: double-click to edit single-line entries directly
- Entry validation, status tracking, history with undo/revert
- Find/Replace with regex, case sensitivity, and batch undo

**Analysis**
- Multi-layer syntax highlighting: MWScript keywords, hyperlinks, glossary terms, forbidden characters
- Spell checking with per-language Hunspell dictionaries
- Annotation system: hyperlinks, gender info, glossary matches from loaded base dicts
- Full script preview with sync scrolling between original and translation
- Book content preview with live update

**Translation**
- **Built-in offline translation engine — no API key required**
- Web translation providers: DeepL, Google Translate, Claude — add more via config files

**Workflows**
- **Lua l10n workflow: edit YAML localization files with auto-pairing and scaffold generation**
- **EET file import: convert ESP-ESM Translator dictionaries to JSON**
- **Generate localization files (.cel, .mrk, .top) from dictionary**

**General**
- Supported languages defined in `languages.json` — add new languages without recompiling
- Dark mode
- All CLI operations accessible from the GUI

## yampt.exe — Command Line

Batch tool for automated dictionary and conversion workflows.

**Dictionary**
- Create base dictionaries by pairing two language versions of a master file
- Create dictionaries from plugins using a base dictionary
- Merge multiple dictionaries with priority ordering
- **JSON dictionary format with status tracking**

**Conversion**
- Convert plugins by applying translations from dictionaries
- **Hyperlink insertion during conversion**
- Create patch plugins containing only modified records
- **Converts compiled script data without recompiling in TES CS**
