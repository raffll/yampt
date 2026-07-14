# yTranslator — User Manual

Translation workbench for Morrowind ESM/ESP plugins. Extracts translatable text, builds translation dictionaries, applies translations back to plugins, and provides an editing environment with spell check, machine translation, and glossary support.

## Getting Started

1. Launch `yTranslator.exe`
2. Add a workspace folder via File → Add Folder (point to a directory containing plugins and/or dictionaries)
3. The sidebar shows all discovered files. Click a dictionary to open it for editing, or right-click a plugin to run operations.

## Window Layout

- **Left panel (top)**: Files tab (sidebar tree) and Filters tab (record type filter)
- **Left panel (bottom)**: Annotations, History, and Translate tabs
- **Right panel (top)**: Records table, Book Preview, and Log tabs
- **Right panel (bottom)**: Editor with Original text, Details, and Translation panels
- **Toolbar**: Search bar with case-sensitive, regex, and column filter toggles
- **Status filter bar**: Colored buttons for each translation status

## Sidebar

The sidebar shows workspace folders and their contents. Items display tags:

- `[BASE]` — dictionary loaded as a base reference
- `[EN]`, `[PL]`, `[DE]`, `[FR]`, `[RU]` — detected language (ESM files only)
- `*` prefix — unsaved changes
- `[UNLOADED]` — workspace file not yet loaded into memory

Right-click items for context menus:

- **Plugin files**: Make Dict, Make Base, Convert, Create
- **Dictionary files**: Save, Save As, Unload
- **Workspace files**: Delete

## Operations

### Make Dict
Extracts all translatable text from a single plugin into a new dictionary. Each entry has `old_text` = `new_text` (untranslated).

### Make Base
Compares two language versions of the same ESM (e.g. English Morrowind.esm vs Polish Morrowind.esm) to create translation pairs automatically. Two modes:

- **Full mode**: All entries get status `translated` (identical entries are proper nouns)
- **Partial mode**: Identical entries are checked against an English dictionary — English words are marked `untranslated`, non-English words are marked `to_verify`

### Make Dict with Base
Creates a dictionary for a plugin using an existing base dictionary as reference. Matches entries by key and by text content, producing statuses like `translated`, `adapted`, `changed`, `reused`, `ambiguous`.

### Convert
Applies translations from loaded dictionaries to a plugin. Only entries with status `translated` are applied. Writes a new ESP file.

### Create
Like Convert, but produces a plugin containing only the modified records (omwaddon-style).

### Merge Dictionaries
Combines multiple dictionaries into one. Available via Tools → Merge Dictionaries or sidebar context menu. Last-listed dictionary has highest priority.

## Editing Translations

1. Click a row in the Records table
2. The Original panel shows the source text, the Translation panel is editable
3. Type your translation, then press the Apply button or Tab to advance to the next entry
4. Press Ctrl+S to save

### Editor Features

- **Spell check**: Red underline on misspelled words (toggle via View → Spell Check)
- **Grammar check**: Highlights grammar issues (toggle via View → Grammar Check)
- **Annotations**: Green highlights show glossary matches (NPC names, items, cells from base dictionaries)
- **Topic links**: Blue highlights show DIAL topic references
- **Forbidden characters**: Orange background on characters invalid in Morrowind's encoding
- **Byte limit validation**: Status bar warning when translation exceeds sub-record size limits
- **Script mode**: For SCTX/BNAM records, only quoted strings are editable — the script structure is preserved

### Keyboard Shortcuts

- `Ctrl+S` — Save
- `Ctrl+F` — Find/Replace
- `F8` — Copy original to translation
- `F9` — Set status to In Progress
- `F10` — Set status to Translated
- `Tab` — Commit and advance to next entry
- `Shift+Tab` — Commit and go to previous entry
- `Escape` — Clear search / close dialog

## Machine Translation

The Translate tab provides automatic translation via CTranslate2 (NLLB-600M model, offline).

1. Select an `untranslated` entry
2. Click the Translate button — the model produces a suggestion
3. Edit if needed, then commit

Batch translation: translates all `untranslated` entries in sequence. Entries are set to status `model` after translation.

## Filtering

### Type filter (Filters tab)
Click record types to show/hide them. Right-click to solo a type. Types: CELL, DIAL, INFO, FNAM, TEXT, GMST, DESC, RNAM, INDX, SCTX, BNAM.

### Status filter bar
Click status buttons to filter by translation status. Right-click to solo. Statuses with zero count are hidden.

### Search bar
Free-text search across Key, Original, and/or Translation columns. Supports case-sensitive and regex modes.

## Settings

Preferences (Ctrl+,) has four pages:

- **Appearance**: Light/dark theme
- **Shortcuts**: Customizable keybindings
- **Language**: Native/foreign language selection, spell check dictionaries, language tags
- **Translation**: DeepL and Google API keys for alternative translation providers

## File Formats

- **JSON dictionaries** (`.json`): Primary format. Contains entries with key, old, new, status, details fields.
- **XML dictionaries** (`.xml`): Legacy format, read-only compatibility.
- **YAML l10n files**: OpenMW localization files, editable in yTranslator.
- **ESM/ESP plugins**: Binary Morrowind plugin files, used as input for operations.

## Tips

- Base dictionaries provide annotations and glossary matches — load them alongside your working dictionary for better context
- Use partial mode for Make Base when working with partially translated ESMs (e.g. EET imports)
- The Book Preview tab shows TEXT records rendered with Morrowind formatting tags
- Import Archive (File menu) extracts zip/rar archives directly into the workspace folder
