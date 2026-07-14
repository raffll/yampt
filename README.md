# Yet Another Morrowind Plugin Tools

A suite of tools for working with Morrowind plugins. Includes a plugin conflict editor similar to xEdit, a translation workbench, and a CLI for batch operations.

## yEditor.exe — Plugin Editor

Qt6 GUI for viewing, comparing, and patching plugins. Similar to TES5Edit/xEdit.

- Load plugins from folder, Mod Organizer 2 profiles, or OpenMW cfg
- Navigation tree with xEdit-style conflict coloring inherited up to file level
- Record comparison with decoded fields aligned by content identity across plugins
- Side-by-side text comparison panel
- Automatic merged patch: leveled lists, dialogues, three-way field-level merge for packed sub-records
- Automatic fixes: fog density, summon persistence, cell name reversion
- Copy or remove records, groups, or individual fields to/from merged patch
- Configurable merge: exclude plugins or records by pattern, toggle record types and fixes

## yTranslator.exe — Translation Workbench

Qt6 GUI for interactive plugin translation.

- Sidebar with workspace folders, auto-refresh on filesystem changes
- Record table filtered by type, sub-type, and translation status
- Three-panel editor: original text, adapted text, editable translation
- Entry validation, status tracking, history with undo/revert
- Multi-layer syntax highlighting: MWScript keywords, hyperlinks, glossary terms, forbidden characters
- Spell checking with per-language Hunspell dictionaries
- Annotation system: hyperlinks, gender info, glossary matches from loaded base dicts
- Book content preview with live update
- Offline machine translation via CTranslate2 (NLLB-600M)
- Supports Polish, German, French, Russian, Italian, Hungarian
- All CLI operations accessible from the GUI

## yampt.exe — Command Line

Batch tool for automated dictionary and conversion workflows.

- Create base dictionaries by pairing two language versions of a master file
- Create dictionaries from plugins using a base dictionary
- Merge multiple dictionaries with priority ordering
- Convert plugins by applying translations from dictionaries
- Create patch plugins containing only modified records
- Converts compiled script data without recompiling in TES CS
- JSON dictionary format with status tracking
