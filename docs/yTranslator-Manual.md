# yTranslator — User Manual

Translation workbench for Morrowind ESM/ESP plugins.

## Getting Started

1. File → Add Folder — point to a directory containing plugins and/or dictionaries
2. The sidebar shows discovered files. Click a dictionary to open it, or right-click a plugin to run operations.

## Main Layout

- **Left top**: Files tab (sidebar) and Filters tab (record type list)
- **Left bottom**: Annotations, History, and Translate tabs
- **Right top**: Records table, Book Preview, and Log tabs
- **Right bottom**: Editor (Original text, Details, Translation)
- **Toolbar**: Search bar with filter toggles (Aa = case-sensitive, .* = regex, Key/Original/Translation column selectors)
- **Status bar**: Status filter buttons (colored by translation status)

## Sidebar

Files are color-coded: green = plugin, gold = base dictionary, blue = user dictionary, purple = YAML.

Right-click context menus:

**Plugins** (ESM/ESP):
- Make Dictionary — extract translatable text
- Make Base — compare two language versions to produce translation pairs
- Convert — apply translations back to a plugin
- Create — produce a new plugin with only translated records
- Delete

**Dictionaries** (JSON/XML):
- Save
- Delete

**YAML files**:
- Save
- Export (create native language version)
- Delete

**Folders**:
- Remove Folder / Delete Folder

## Operations

### Import Archive
File → Import Archive — extracts a zip or rar archive into the workspace folder. Requires 7za.exe next to the application. After extraction, the workspace auto-scans for new files.

### Make Dictionary
Extracts all translatable text from a plugin. Each entry starts as untranslated (old = new).

### Make Base
Compares a foreign-language ESM against a native-language ESM. Produces matched translation pairs. Two modes:
- **Full**: identical text = proper noun (status: translated)
- **Partial**: identical text checked against English dictionary — English words = untranslated, non-English = to_verify

### Convert
Applies translations from loaded dictionaries to a plugin. Only `translated` entries are used. Writes a new file.

### Create
Like Convert but the output contains only modified records.

### Merge Dictionaries (Tools menu)
Combines multiple dictionaries. Last in the list has highest priority.

## Editing

1. Click a row in the Records table
2. Edit the Translation panel
3. Press Apply (or Tab) to commit and advance
4. Ctrl+S to save

The editor shows:
- **Original**: source text (read-only)
- **Details**: adaptation source or match info (when available)
- **Translation**: your translation (editable)

For script records (SCTX/BNAM), only quoted strings are editable — the script structure is preserved automatically.

## Toolbar Search

Type in the search field to filter the records table. Toggle which columns are searched (Key, Original, Translation). Supports case-sensitive and regex modes.

## Status Filter Bar

Colored buttons below the toolbar. Click to filter by status, right-click to solo. Only statuses with entries are shown.

## Type Filter (Filters tab)

Left panel has a list of record types (CELL, DIAL, INFO, FNAM, TEXT, GMST, etc.). Click to toggle, right-click to solo.

## View Menu

- **Toggle Sidebar** — show/hide left panel
- **Toggle Bottom Panel** — show/hide editor
- **Spell Check** — underline misspelled words in translation
- **Grammar Check** — highlight grammar issues
- **Whitespace Markers** — show spaces and line endings

## Keyboard Shortcuts

- `Ctrl+S` — Save
- `F8` — Copy original to translation
- `F9` — Set status to In Progress
- `F10` — Set status to Translated
- `Tab` — Commit and next entry
- `Shift+Tab` — Previous entry
- `Escape` — Clear search

## Machine Translation (Translate tab)

Uses CTranslate2 with NLLB-600M model (offline, supports PL/DE/FR/RU/IT/HU).

- Select an untranslated entry → click Translate
- Status is set to `model` after translation

## Entry Statuses

Each dictionary entry has a status that determines whether it's applied during Convert/Create.

**Applied to plugin** (only these produce output):
- **Translated** — finished, human-approved translation
- **Matched** — paired automatically during make-base
- **Heuristic** — matched by the translation engine during make-base

**Needs work:**
- **Untranslated** — no translation yet
- **In Progress** — editing started but not finalized
- **Model** — machine-translated, awaiting review
- **Changed** — source text updated since translation was made
- **Adapted** — source differs in numbers/punctuation, translation auto-adjusted
- **Outdated** — source changed while translation was still unfinished
- **Ambiguous** — multiple conflicting translations exist

**Informational:**
- **Reused** — translation borrowed from another entry with the same text
- **Propagated** — auto-filled from another entry
- **Missing** — no native match found during make-base
- **Duplicate** — same key appeared multiple times
- **Mismatch** — extra native content with no foreign counterpart
- **Error** — validation problem

See [Dictionary Entry Statuses](Dictionary-Entry-Statuses.md) for the full reference.

## Settings (Ctrl+,)

- **Appearance**: Theme
- **Shortcuts**: Keybindings
- **Language**: Native/foreign language, spell check dictionaries
- **Translation**: API keys for DeepL and Google (alternative providers)
