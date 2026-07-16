# yTranslator — User Manual

Translation workbench for Morrowind ESM/ESP plugins.

## Getting Started

Open File → Add Folder and point it to a directory containing your plugins and dictionaries. The sidebar populates with all discovered files. Click a dictionary to open it for editing. Right-click a plugin to run operations like Make Dictionary or Convert.

## Main Layout

- **Left top** — Files tab (sidebar with loaded files), Filters tab (record type list), and Statuses tab (status filter with counters).
- **Left bottom** — Annotations tab (glossary matches for the current entry), History tab (edit history), and Auto Translate tab (translation providers).
- **Right top** — Records table showing all entries in the active dictionary. Preview tab renders book HTML. Log tab shows operation output.
- **Right bottom** — Editor with three panels: Original (read-only source text), Details (adaptation info when available), and Translation (editable).
- **Toolbar** — Search field with toggle buttons: Aa for case-sensitive, .* for regex, and column selectors (Key, Original, Translation) to control which fields are searched.

## Sidebar

Files are color-coded by type: green for plugins, gold for base dictionaries, blue for user dictionaries, purple for YAML localization files. An asterisk prefix indicates unsaved changes.

Right-click a file or folder to access its context menu. Plugins offer Make Dictionary, Make Base, Convert, Create, and Delete. Dictionaries offer Save and Delete. YAML foreign files offer Make Translation (creates a native scaffold) and Delete. YAML native files offer Save and Delete. Folders offer Remove Folder (unload from sidebar) and Delete Folder (remove from disk).

## Operations

### Import Archive

File → Import Archive opens a file dialog for zip or rar archives. The contents are extracted into the workspace folder. Requires 7za.exe in the same directory as the application. After extraction the workspace auto-scans and shows new files in the sidebar.

### Make Dictionary

Right-click a plugin and select Make Dictionary. This reads all translatable records from the plugin and creates a new dictionary file. Every entry starts with status Untranslated — the original and translation fields are identical.

If you select "with Base" variant, the operation also applies translations from loaded base dictionaries to entries that match by key or by original text.

### Make Base

Right-click a foreign-language plugin (e.g. English Morrowind.esm) and select Make Base. A dialog asks you to pick the native-language version (e.g. Polish Morrowind.esm). The application compares both files record by record to produce matched translation pairs.

Two modes are available:

- **Full** — all matched entries receive status Translated. When the original and native text are identical, the entry is treated as a proper noun (same word in both languages).
- **Partial** — identical entries are checked against an English dictionary. If English words are detected, the entry is marked Untranslated (likely not yet translated). If no English words are found, it is marked To Verify (probably a proper noun, but confirm manually).

### Convert

Right-click a plugin and select Convert. This applies all Translated entries from loaded dictionaries to the plugin and writes a new output file. Entries with any other status are ignored — the original plugin text stays unchanged for those records.

### Create

Works like Convert but the output file contains only the records that were actually modified. Use this to produce a lightweight translation patch.

### Merge Dictionaries

Tools → Merge Dictionaries opens a dialog where you select multiple dictionaries and an output path. Dictionaries are merged in priority order — the last one in the list wins when entries conflict.

## Editing

Click a row in the Records table to load it into the editor. The Original panel shows the source text. The Translation panel is where you type your translation. Press Tab to commit your edit and advance to the next row. Press Ctrl+S to save the dictionary to disk.

When you commit an edit, the entry status changes to In Progress automatically. To mark it as final, press F10 (sets status to Translated). Press F9 to explicitly set In Progress without committing.

For script records (SCTX/BNAM), the editor shows only the translatable quoted strings extracted from the script. The surrounding code structure is preserved — you cannot accidentally break the script by editing.

The Details panel appears when an entry has adaptation or conflict information. For Adapted entries it shows the source translation that was modified. For Changed entries it shows the old original text so you can see what changed. For Ambiguous entries it lists all conflicting translations.

## Toolbar Search

Type text into the search field to filter the Records table. Only rows matching the query are shown. Use the toggle buttons to control the search:

- **Aa** — case-sensitive matching.
- **.\*** — interpret the query as a regular expression.
- **Key / Original / Translation** — choose which columns to search. Multiple can be active at once.

Press Escape to clear the search and show all rows again.

## Status Filter (Statuses tab)

The Statuses tab shows a list of all statuses present in the current dictionary, each with a colored bullet and a count. Click a status to solo it (show only entries with that status). Right-click to toggle individual statuses on or off. Click "All" to reset and show everything. The status filter operates independently from the type filter.

## Type Filter (Filters tab)

The Filters tab shows record types present in the current dictionary (CELL, DIAL, INFO, FNAM, TEXT, GMST, etc.). Click a type to solo it. Right-click to toggle. Works independently from the status filter.

## View Menu

- **Toggle Sidebar** — show or hide the left panel entirely.
- **Toggle Bottom Panel** — show or hide the editor area.
- **Spell Check** — when enabled, misspelled words in the Translation panel are underlined in red. Uses Hunspell dictionaries configured in Settings → Language.
- **Grammar Check** — highlights common issues: double spaces, unmatched quotes or parentheses, missing terminal punctuation. Quoted text within entries is shown in a lighter color for visual distinction.
- **Whitespace Markers** — renders spaces as dots and line endings as paragraph marks in the editor panels.
- **Sync Scrolling** — locks the scroll position between the Original and Translation panels so they stay aligned as you scroll either one.

## Keyboard Shortcuts

- `Ctrl+S` — save the current dictionary to disk.
- `F8` — copy the original text into the translation field (useful as a starting point).
- `F9` — commit the current edit and set status to In Progress.
- `F10` — commit the current edit and set status to Translated.
- `Tab` — commit the current edit (status becomes In Progress) and select the next row.
- `Shift+Tab` — select the previous row without committing.
- `Escape` — clear the search field.

## Auto Translate

The Auto Translate tab at the bottom-left provides machine translation. Select a provider from the combo box, then click Translate to fill the translation field with a suggestion.

- **CTranslate2** — an offline translation model that runs locally. Supports Polish, German, French, Russian, Italian, and Hungarian. Does not require an internet connection. The model must be present in the `models/` folder next to the application.
- **Web providers** — online services like DeepL, Google Translate, and Claude. Each requires an API key configured in Settings → Translation. The source language is read from your Language settings automatically.

After a successful translation, the entry status is set to Generated. Review the result and set to Translated when satisfied.

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

## Settings

Open Settings via Ctrl+, or the Edit menu. Four pages are available:

- **Appearance** — choose between light and dark theme.
- **Shortcuts** — customize keyboard shortcuts for all actions. Conflicts are highlighted in red.
- **Language** — set the foreign language (source) and native language (target). Choose a spell check dictionary for the Translation panel. Configure the English dictionary used for partial mode in Make Base.
- **Translation** — shows a table of all discovered web translation providers. Enter your API key for each service you want to use. The Status column shows whether a key is configured.
