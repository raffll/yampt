# yTranslator — User Manual

Translation workbench for Morrowind ESM/ESP plugins.

## Getting Started

1. File → Add Folder — point to a directory containing plugins and/or dictionaries
2. The sidebar shows discovered files. Click a dictionary to open it, or right-click a plugin to run operations.

## Main Layout

- **Left top**: Files tab (sidebar), Filters tab (record type list), and Statuses tab (status filter)
- **Left bottom**: Annotations, History, and Auto Translate tabs
- **Right top**: Records table, Preview, and Log tabs
- **Right bottom**: Editor (Original text, Details, Translation)
- **Toolbar**: Search bar with filter toggles (Aa = case-sensitive, .* = regex, Key/Original/Translation column selectors)

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

**YAML files** (foreign):
- Make Translation (create native language scaffold)
- Delete

**YAML files** (native):
- Save
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

## Status Filter (Statuses tab)

List of all statuses with colored bullet indicators and counters. Click to solo one status, right-click to toggle. "All" row resets the filter. Filters and Statuses are independent.

## Type Filter (Filters tab)

Left panel has a list of record types (CELL, DIAL, INFO, FNAM, TEXT, GMST, etc.). Click to toggle, right-click to solo.

## View Menu

- **Toggle Sidebar** — show/hide left panel
- **Toggle Bottom Panel** — show/hide editor
- **Spell Check** — underline misspelled words in translation
- **Grammar Check** — highlight grammar issues (double spaces, unmatched quotes/parens, missing punctuation, quoted text in lighter color)
- **Whitespace Markers** — show spaces and line endings
- **Sync Scrolling** — bind scrolling between original and translation panes (editor and preview)

## Keyboard Shortcuts

- `Ctrl+S` — Save
- `F8` — Copy original to translation
- `F9` — Set status to In Progress
- `F10` — Set status to Translated
- `Tab` — Commit and next entry
- `Shift+Tab` — Previous entry
- `Escape` — Clear search

## Machine Translation (Auto Translate tab)

Select a provider from the combo box:

- **CTranslate2** — offline model (supports PL/DE/FR/RU/IT/HU). No internet required.
- **Web providers** — DeepL, Google Translate, Claude, and others. Requires API key (Settings → Translation).

Usage:
- Select an untranslated entry → click Translate
- Status is set to `Generated` after translation

Additional providers can be added by placing a configuration file in the `providers/` folder next to the application.

## Entry Statuses

Each dictionary entry has a status. Only **Translated** entries are applied during Convert/Create — all others are skipped. You can manually set **Translated**, **In Progress**, or **Untranslated** via right-click context menu.

- **Translated** — the translation is approved. This is the only status that produces output when running Convert or Create.
- **Untranslated** — no translation exists. The original and translation fields contain the same text.
- **In Progress** — assigned automatically when you edit a translation. Indicates work has started but the entry is not yet approved.
- **Generated** — assigned when the Auto Translate button fills in a translation. Review the result, then set to Translated if correct.
- **To Verify** — assigned during Make Base (partial mode) when the original and native text are identical but no English words were detected. May be a proper noun that needs no translation, or may be an untranslated entry in a language that shares words with English. Check manually.
- **Changed** — the original text in the source plugin differs from what was originally translated. The existing translation may no longer be accurate. Compare the current original with the old original shown in the Details panel.
- **Adapted** — no entry with a matching key was found in the base dictionary, but another entry with identical original text provided a translation. The translation may not fit this context. The Details panel shows which entry it was adapted from.
- **Outdated** — like Changed, but the entry had not been approved as Translated before the source text changed.
- **Ambiguous** — multiple entries in the base dictionary offer different translations for the same original text. The Details panel lists all candidates. Pick the correct one and set to Translated.
- **Reused** — the base dictionary contained a matching original text under a different key. The translation was copied from that entry.
- **Propagated** — after you committed a translation, all other entries sharing the same original text were updated to match.
- **Missing** — during Make Base, this record existed in the foreign file but no corresponding record was found in the native file. Requires manual translation.
- **Duplicate** — the same key appeared more than once in the source plugin. Only the first occurrence is stored.
- **Mismatch** — during Make Base, a record existed in the native file with no corresponding record in the foreign file. Informational; no action needed.
- **Error** — the translation exceeds the maximum byte length allowed for this sub-record type and cannot be written to the plugin. Shorten the translation.

## Settings (Ctrl+,)

- **Appearance**: Theme
- **Shortcuts**: Keybindings
- **Language**: Native/foreign language, spell check dictionaries
- **Translation**: API keys table for web providers (DeepL, Google, Claude, etc.)
