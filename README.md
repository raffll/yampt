# Yet Another Morrowind Plugin Tools

A suite of tools for working with Morrowind plugins (ESM/ESP/omwgame/omwaddon). Includes a plugin conflict editor similar to xEdit, a translation workbench, and a CLI for batch operations.

## Applications

### yEditor — Plugin Editor (70% DONE)

Qt6 GUI for viewing, comparing, and patching plugins. Similar to TES5Edit/xEdit.

- Load plugins from folder, Mod Organizer 2 profiles, or OpenMW cfg
- Navigation tree showing file → record type → individual records
- Comparison view with per-sub-record field values across all loaded plugins
- xEdit-style conflict coloring (background by conflict severity, text by override status)
- Conflict categories: no conflict, benign override, critical conflict
- Filter by conflict level, record type, record ID/name, deleted records
- ITM (Identical to Master) detection and one-click removal (WIP)
- Merged patch creation with leveled list merging and dialogue merging (WIP)
- Drag-and-drop record copying to merged patch (WIP)

### yTranslator — Translation Workbench (90% DONE)

Qt6 GUI for interactive plugin translation.

- Sidebar with workspace folders, auto-refresh on filesystem changes
- Record table with filtering by type, sub-type, and translation status
- Search with regex support
- Three-panel editor: original text, adapted text, editable translation
- Syntax highlighting for MWScript keywords, hyperlinks, and formatting tags
- Spell checking (Hunspell) with per-language dictionaries
- Grammar checking
- Translation suggestions from three sources: local CTranslate2 model, DeepL API, Google API (WIP)
- Annotation system showing hyperlinks, gender info, and glossary matches
- History panel with undo support
- Find & replace across the active dictionary
- Entry validation and status tracking (untranslated, translated, in progress, error, etc.)
- Book content preview panel
- All CLI operations accessible from the GUI (make dict, make base, merge, convert, create)
- Session persistence and encoding selection (Windows-1250/1252)

### yampt — Command Line

Batch tool for automated dictionary and conversion workflows.

- Create base dictionaries by pairing two language versions of the same master file
- Create dictionaries from plugins
- Merge multiple dictionaries
- Convert plugins by applying translations from dictionaries
- Create patch plugins containing only modified records
- JSON dictionary format
- Handles all translatable record types: CELL, DIAL, INFO, BNAM, SCTX, GMST, FNAM, DESC, TEXT, RNAM, INDX
- Converts compiled script data (SCDT) without needing to recompile in TES CS

## CLI Usage

```
yampt.exe --make -f <plugins...> [-d <base_dict>] [-o <output>]
yampt.exe --make-base -f <native.esm> <foreign.esm> [--translate <model_path>]
yampt.exe --merge -d <dicts...> -o <output>
yampt.exe --convert -f <plugins...> -d <dicts...> [-s <suffix>] [--windows-1250]
yampt.exe --create -f <plugins...> -d <dicts...> [-s <suffix>] [--windows-1250]
```

| Command | Description |
|---------|-------------|
| `--make` | Extract translatable text into a dictionary; with `-d` compares against base dict |
| `--make-base` | Create base dictionary from two language versions of the same file |
| `--merge` | Merge multiple dictionaries (first-wins precedence) |
| `--convert` | Apply translation to plugins, replacing all matching text |
| `--create` | Create a patch plugin containing only records that differ from the original |

| Option | Description |
|--------|-------------|
| `--translate <path>` | Load CTranslate2 model for neural-assisted cell matching (make-base only) |
| `--windows-1250` | Use Windows-1250 encoding for output |
| `--debug` | Enable verbose script parser logging |
| `-s <suffix>` | Output filename suffix for convert/create |
| `-o <output>` | Output filename for merge and make |
