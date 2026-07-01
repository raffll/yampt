# Yet Another Morrowind Plugin Tools

A suite of tools for working with Morrowind plugins (ESM/ESP/omwgame/omwaddon). Includes a plugin conflict editor similar to xEdit, a translation workbench, and a CLI for batch operations.

## Applications

### yampt.exe — Command Line

Batch tool for automated dictionary and conversion workflows.

- Create base dictionaries by pairing two language versions of the same master file
- Create dictionaries from plugins using a base dictionary
- Merge multiple dictionaries (last-listed wins)
- Convert plugins by applying translations from dictionaries
- Create patch plugins containing only modified records
- JSON dictionary format with status tracking
- Handles all translatable record types: CELL, DIAL, INFO, BNAM, SCTX, GMST, FNAM, DESC, TEXT, RNAM, INDX
- Converts compiled script data (SCDT) without needing to recompile in TES CS
- Neural-assisted cell matching via CTranslate2 translation model

### yTranslator.exe — Translation Workbench

Qt6 GUI for interactive plugin translation.

- Sidebar with workspace folders, auto-refresh on filesystem changes
- Record table with filtering by type, sub-type, and translation status
- Search with regex support
- Three-panel editor: original text, adapted text, editable translation
- Syntax highlighting for MWScript keywords, hyperlinks, and formatting tags
- Spell checking (Hunspell) with per-language dictionaries
- Translation suggestions from CTranslate2 local model, DeepL API, and Google API
- Annotation system showing hyperlinks, gender info, and glossary matches
- History panel with undo/revert (restores both text and status)
- Find and replace across the active dictionary
- Entry validation and status tracking
- Book content preview panel
- All CLI operations accessible from the GUI (make dict, make base, merge, convert, create)
- Session persistence and encoding selection (Windows-1250/1252)
- EET file format import (ESP-ESM Translator migration)
- Keyboard shortcuts for common translation actions (F8/F9/F10)

### yEditor.exe — Plugin Editor

Qt6 GUI for viewing, comparing, and patching plugins. Similar to TES5Edit/xEdit.

- Load plugins from folder, Mod Organizer 2 profiles, or OpenMW cfg
- Navigation tree showing file, record type, and individual records
- Comparison view with per-sub-record field values across all loaded plugins
- xEdit-style conflict coloring (background by conflict severity, text by override status)
- Conflict categories: no conflict, benign override, critical conflict
- Filter by conflict level, record type, record ID/name, deleted records
- Merged patch creation with leveled list merging and dialogue merging
- Drag-and-drop record copying to merged patch

## CLI Usage

```
yampt.exe --make -f <plugins...> [-d <base_dict>] [-o <output>]
yampt.exe --make-base -f <native.esm> <foreign.esm> [--translate <model_path>] [--partial]
yampt.exe --merge -d <dicts...> -o <output>
yampt.exe --convert -f <plugins...> -d <dicts...> [-s <suffix>] [--add-hyperlinks] [--windows-1250]
yampt.exe --create -f <plugins...> -d <dicts...> [-s <suffix>] [--windows-1250]
```

| Command | Description |
|---------|-------------|
| `--make` | Extract translatable text into a dictionary; with `-d` compares against base dict |
| `--make-base` | Create base dictionary from two language versions of the same file |
| `--merge` | Merge multiple dictionaries (last-listed wins) |
| `--convert` | Apply translation to plugins, replacing all matching text |
| `--create` | Create a patch plugin containing only records that differ from the original |

| Option | Description |
|--------|-------------|
| `--translate <path>` | Load CTranslate2 model for neural-assisted cell matching (make-base only) |
| `--partial` | Use partial mode for make-base (check identical entries against Hunspell dictionary) |
| `--add-hyperlinks` | Insert `@` hyperlink markers matching OpenMW detection rules (convert only) |
| `--windows-1250` | Use Windows-1250 encoding for output |
| `--debug` | Enable verbose script parser logging |
| `-s <suffix>` | Output filename suffix for convert/create |
| `-o <output>` | Output filename for merge and make |

## Building

Requires Visual Studio 2026 (v145 toolset), vcpkg (manifest mode), and Qt6.

Open `yampt.sln` and build. Output goes to `x64/Release/` (or `x64/Debug/`):
- `yampt.exe`
- `yTranslator.exe`
- `yEditor.exe`

Dependencies are managed via vcpkg manifest (`vcpkg.json`). CTranslate2 is built separately from `external/CTranslate2/` — see the steering docs for details.

## Version Scheme

Version format: `0.{git_build_number}` (e.g. `0.842`).

The build number is the total git commit count (`git rev-list --count HEAD`). It increments automatically with each commit. The version is resolved by `scripts/get_version.ps1` with fallback to a `VERSION` file or `0.0` in non-git environments.

## Release Packaging

Run from the repo root after building in Release configuration:

```powershell
.\scripts\pack_release.ps1
```

The script:
1. Resolves the current version via `scripts/get_version.ps1`
2. Stamps the version into `vcpkg.json` and a `VERSION` file (via `scripts/stamp_version.ps1`)
3. Copies executables, DLLs, `dictionaries/`, and `platforms/` from `x64/Release/` into `build/yampt_{version}/`
4. Creates a `build/yampt_{version}.7z` archive

With the `-Upload` flag and a `GITHUB_TOKEN` environment variable, the script also creates a GitHub release and uploads the archive.

The release package does NOT include `models/` (too large — users download separately via `download_models.py`) or `scripts/`.

## Translation Models

CTranslate2 models for neural-assisted cell matching are not included in the release. Download them with:

```powershell
python download_models.py
```

This requires `torch`, `transformers`, `ctranslate2`, `huggingface_hub`, and `sentencepiece`. Models are stored in `models/` relative to the working directory.

## License

MIT
