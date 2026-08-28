# Changelog

## [XXX]

### yTranslator
- [NEW] Enchantment annotation: FNAM entries for weapons, armor, clothing, and books now show the enchantment ID in the Annotations panel when the item is enchanted
- [NEW] Revert from record table context menu: right-click selected entries → Revert restores previous text and status from history
- [NEW] Encoding line in Language settings: shows which codepage the selected native language uses for plugin text
- [NEW] Mark records as AI translation examples: right-click records to mark up to twenty as style examples sent to the AI provider
- [NEW] Fetch available models from the provider: a Refresh control in the Auto Translate panel pulls the current model list from the provider
- [CHANGE] Settings: "Providers" page renamed to "Auto Translation" with three tabs — Local Models, Web Providers, and Examples
- [CHANGE] Model selection moved from Settings into the Auto Translate panel
- [CHANGE] Find/Replace moved from the filter toolbar into its own tab in the left panel, after Statuses
- [CHANGE] History tab moved to the end of the bottom-left tabs; each history entry now shows its status and timestamp
- [CHANGE] Find/Replace no longer has a batch Undo button — each replacement is recorded in edit history and revertable per-entry

### Both Apps
- [NEW] Linux support: builds with CMake and system libraries, AUR package available
- [NEW] Cross-platform resource paths: shared data in `/usr/share/yampt/`, user data in `~/.yampt/`
- [FIX] Disabled widgets now show greyed-out text on all platforms
- [FIX] Workspace sidebar no longer shows a duplicate "Workspace" node when paths differ only by trailing slash

### yEditor
- [NEW] Enable Editing: a single toolbar toggle makes decoded fields editable in the Edit panel for all plugins (combobox for flags and enums); starts disabled each time the app opens
- [NEW] View menu: Toggle Sidebar and Toggle Bottom Panel hide or show the navigation and edit/log panels; state persists across sessions
- [NEW] Lua handler conflict detection in a separate Lua tab: scans OpenMW Lua scripts and highlights conflicting handler registrations between mods
- [NEW] Exclude Sub-Record from context menu: right-click any sub-record row to add it to the exclusion list
- [NEW] Toolbar search: filter the navigation tree by record ID or display name with case-sensitive and regex support
- [NEW] Reset button on the toolbar: clears Conflicts Only, the search field, and the advanced filter in one click
- [CHANGE] Filter dialog moved from View menu to toolbar button
- [CHANGE] Reset button renamed to "No Filters" and moved before the Conflicts Only toggle
- [CHANGE] Navigation panel split into two tabs: Plugins (ESP/ESM record tree) and Lua (handler registrations by mod)
- [CHANGE] Settings reorganized: Sub-Record Rules merged into Merged Patch page, Paths renamed to Output Paths, record type checkboxes replaced by TYPE:* syntax
- [CHANGE] File Header shown as flat entry at the top of each plugin (no nesting)
- [CHANGE] Unknown/binary sub-records now display hex bytes instead of placeholder
- [CHANGE] View menu: "Hide Duplicate Columns" renamed to "Hide Duplicates", "Show Deleted Strikeout" renamed to "Mark Deleted" (disabled by default)
- [FIX] Record view context menu now works when right-clicking the label column
- [FIX] A plugin can no longer be both a guard patch and excluded from the merged patch — setting one clears the other
- [FIX] Edit panel now shows the full multi-line content of text sub-records (book TEXT, script SCTX/BNAM) instead of only the first line
- [FIX] Decoded fields inside CELL reference groups (X/Y/Z Position, rotations, door destinations) can now be edited when Enable Editing is active
- [FIX] Empty sub-records (e.g. FNAM with no display name) can now be edited — previously the empty value blocked the edit panel
- [FIX] Exclude Sub-Record context menu no longer adds duplicates if the rule already exists
- [FIX] Excluding a sub-record from context menu now immediately greys out the row in the record view
- [FIX] Excluded sub-records now grey out their decoded children (fields, flags) as well

## [0.940] - 2026-07-29

### yTranslator
- [NEW] Find/Replace dialog (Tools menu) with regex, case sensitivity, and batch undo
- [NEW] "Replaced" status assigned to entries modified by Find/Replace
- [NEW] EET file import: export ESP-ESM Translator dictionaries to JSON (partial support)
- [NEW] Generate localization files (.cel, .mrk, .top) from dictionary with Hunspell inflection
- [NEW] Full script preview: selecting a script entry shows the entire script source in Preview tab
- [NEW] Script source stored as reference data in dictionaries for context lookup
- [NEW] Sync Scrolling: View menu toggle to bind scrolling between original and translation panes
- [NEW] Grammar check: quoted text highlighted in lighter color for visual distinction
- [NEW] Web translation providers: config-driven architecture (DeepL, Google, Claude via JSON config files)
- [NEW] Translation settings page: table showing all providers with API key fields
- [NEW] Translation engine: pre-substitution of glossary terms before sending to AI model
- [NEW] Translation engine: SCTX/BNAM tokenization — only translatable strings are sent to model
- [NEW] Inline table editing: double-click Translation column to edit single-line entries directly
- [NEW] Lua l10n workflow: auto-pairs foreign/native YAML files in the same directory
- [NEW] Lua l10n workflow: opening a native YAML allows editing, opening a foreign YAML shows read-only reference
- [NEW] Lua l10n workflow: save writes only translated entries to the native file
- [NEW] Lua l10n workflow: Make Translation creates a scaffold native file with all keys
- [NEW] Merge Dictionaries in Tools menu
- [NEW] View menu: Spell Check toggle
- [NEW] Spell check dictionaries: added de_DE, fr_FR, ru_RU, it_IT, hu_HU
- [NEW] First run dialog: added Italian and Hungarian languages
- [NEW] Language list externalized to `languages.json` — add new languages without recompiling
- [CHANGE] Status filter moved to a dedicated Statuses tab (same interaction as Filters tab)
- [CHANGE] Language settings: simplified panel (foreign/native language, spell check, tags)
- [CHANGE] Script editor: `say` keyword sound file path hidden from display and translation
- [CHANGE] Book Preview tab renamed to Preview
- [CHANGE] Translate tab renamed to Auto Translate
- [CHANGE] "Model" status display renamed to "Generated"
- [CHANGE] Filters and Statuses are now fully independent (no cross-reset)
- [CHANGE] YAML context menu: foreign files show "Make Translation", native files show "Save"
- [CHANGE] YAML files in workspace auto-loaded on startup
- [CHANGE] Translation button populates the translation editor with result
- [CHANGE] Translation button only works on untranslated entries
- [CHANGE] Translation button commits immediately and advances to the next row
- [CHANGE] Translation button works for YAML documents (not just dict)
- [CHANGE] Inline table editing advances to the next row after commit
- [CHANGE] Read-only documents disable editing actions in menus and shortcuts
- [CHANGE] Make Dict and Make Dict with Base merged into single "Make Dictionary" menu item
- [CHANGE] Make Base dialog: removed dictionary combo box (uses language settings)
- [CHANGE] Language settings: encoding and translation target auto-derived from language selection
- [CHANGE] Language settings: spell check auto-set to None when dictionary not found
- [CHANGE] All operation dialogs use consistent 450×400 initial size
- [CHANGE] Convert/Create: preserves original file timestamp
- [CHANGE] "Make Base" renamed to "Make Base Dictionary"
- [CHANGE] "Convert" renamed to "Convert Plugin"
- [CHANGE] "Create" renamed to "Create Patch Plugin"
- [FIX] Annotation highlight misalignment
- [FIX] EET import: raw SCTX script bodies no longer imported — only extracted translatable strings (MSGB, CELL, SAY, DIAL) are converted
- [FIX] Dictionary marked dirty on row click without editing
- [FIX] SCTX/BNAM validation: quotes no longer flagged as forbidden characters
- [FIX] Whitespace markers: newline indicator now visible at line breaks
- [FIX] Spell check: dictionary now loads correctly on startup
- [FIX] Grammar check: missing punctuation no longer flagged while cursor is at end of text
- [FIX] Propagation: entries with leading/trailing whitespace differences now match correctly

### yEditor
- [NEW] Dialogue INFO chain resolved using OpenMW ordering algorithm (PNAM-based insertion)
- [NEW] Plugin cleaning: removes evil GMSTs and junk cells from all loaded plugins
- [NEW] Header repair: update master file sizes in plugin headers to match actual files on disk
- [NEW] Header repair: update plugin version to 1.3
- [NEW] Cleaning settings page with toggleable removal options
- [NEW] Sub-Record Rules settings page for configuring conflict and merge behavior
- [NEW] Guard Patch: plugins before guard are excluded from merge for records the guard contains
- [NEW] Status bar shows load mode and path
- [CHANGE] Sub-record conflict rules applied at runtime (changes take effect immediately after settings)
- [CHANGE] Right panel columns redistribute on resize to fill available width
- [CHANGE] Nav tree decodes codepage characters correctly
- [CHANGE] Filter dialog: record type list unchecked by default (all unchecked = show all)

### Both Apps
- [NEW] Localization support: all UI strings wrapped with `tr()` for Qt translation system

## [0.842] - 2026-07-05

### yTranslator
- [NEW] Settings dialog (appearance, shortcuts, language, translation engine)
- [NEW] Workspace folder watches for file changes and refreshes automatically
- [NEW] Multi-layer highlighting (MWScript, hyperlinks, glossary, forbidden characters)
- [NEW] Merge dialog for combining dictionaries
- [CHANGE] Consistent syntax coloring across all editor panels

### yEditor
- [NEW] Automatic merged patch creation (leveled lists, dialogues, three-way object merge)
- [NEW] Fog fix, summon fix, and cell name fix applied automatically to merged patches
- [NEW] Copy records, groups, or individual fields to the merged patch via right-click menu
- [NEW] Remove records and groups from the merged patch via right-click menu
- [NEW] Decoded field view for CELL objects, leveled lists, containers, factions
- [NEW] Advanced filter dialog (conflict level, override status, record type, ID, name, deleted records)
- [NEW] Side-by-side text comparison panel
- [NEW] Settings dialog (appearance, paths, merge options)
- [NEW] Session persistence — remembers loaded plugins and window state between runs
- [NEW] Configurable merge: exclude plugins or records by pattern, toggle record types and automatic fixes
- [CHANGE] Conflict coloring now works at the individual field level (not just whole records)
- [CHANGE] Navigation tree inherits worst-case conflict color from children up to file level
- [CHANGE] Entries from different plugins aligned by content identity (item ID, object index, rank) instead of file order

### Both Apps
- [NEW] Dark mode

## [0.735] - 2026-06-21

### Both Apps
- [NEW] yTranslator: translation workbench with spell check, annotations, history, and translation suggestions
- [NEW] yEditor: plugin conflict viewer and merged patch creator (xEdit-like)
- [NEW] JSON dictionary format with per-entry status tracking
- [CHANGE] Rewritten from scratch as a Qt6 application suite
