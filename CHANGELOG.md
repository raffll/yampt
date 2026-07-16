# Changelog

## [xxx]

### yTranslator
- [NEW] Full script preview: selecting a script entry shows the entire script source in Preview tab
- [NEW] Script source stored as reference data in dictionaries for context lookup
- [NEW] Sync Scrolling: View menu toggle to bind scrolling between original and translation panes
- [NEW] Grammar check: quoted text highlighted in lighter color for visual distinction
- [NEW] Claude translation provider: AI-powered translation with glossary context in prompt
- [NEW] Translation settings page: Claude API key configuration
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
- [CHANGE] Status filter moved to a dedicated Statuses tab (same interaction as Filters tab)
- [CHANGE] Language settings: simplified panel (foreign/native language, spell check, tags)
- [CHANGE] Script editor: `say` keyword sound file path hidden from display and translation
- [CHANGE] Book Preview tab renamed to Preview
- [CHANGE] Translate tab renamed to AI Translate
- [CHANGE] "Model" status display renamed to "AI Translated"
- [CHANGE] Filters and Statuses are now fully independent (no cross-reset)
- [CHANGE] YAML context menu: foreign files show "Make Translation", native files show "Save"
- [CHANGE] YAML files in workspace auto-loaded on startup
- [CHANGE] Translation button populates the translation editor with result
- [CHANGE] Translation button only works on untranslated entries
- [CHANGE] Translation button commits immediately and advances to the next row
- [CHANGE] Translation button works for YAML documents (not just dict)
- [CHANGE] Inline table editing advances to the next row after commit
- [CHANGE] Polymorphic document commit: unified commit flow for all document types
- [CHANGE] Document permissions control context menu and shortcut availability
- [CHANGE] Make Dict and Make Dict with Base merged into single "Make Dictionary" menu item
- [CHANGE] Make Base dialog: removed dictionary combo box (uses language settings)
- [CHANGE] Language settings: encoding and translation target auto-derived from language selection
- [CHANGE] Language settings: spell check auto-set to None when dictionary not found
- [CHANGE] All operation dialogs use consistent 450×400 initial size
- [CHANGE] Convert/Create: preserves original file timestamp
- [FIX] Annotation highlight misalignment
- [FIX] Dictionary marked dirty on row click without editing
- [FIX] SCTX/BNAM validation: quotes no longer flagged as forbidden characters
- [FIX] Whitespace markers: newline indicator now visible at line breaks
- [FIX] Spell check: dictionary now loads correctly on startup
- [FIX] Grammar check: missing punctuation no longer flagged while cursor is at end of text
- [FIX] Propagation: entries with leading/trailing whitespace differences now match correctly

### yEditor
- [NEW] Guard Patch: plugins before guard are excluded from merge for records the guard contains
- [NEW] Status bar shows load mode and path
- [CHANGE] Right panel columns redistribute on resize to fill available width
- [CHANGE] Nav tree decodes codepage characters correctly
- [CHANGE] Filter dialog: record type list unchecked by default (all unchecked = show all)

### Both Apps
- [NEW] Localization support: all UI strings wrapped with `tr()` for Qt translation system

## [0.842] - 2026-07-05

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

### yTranslator
- [NEW] Settings dialog (appearance, shortcuts, language, translation engine)
- [NEW] Workspace folder watches for file changes and refreshes automatically
- [NEW] Multi-layer highlighting (MWScript, hyperlinks, glossary, forbidden characters)
- [NEW] Merge dialog for combining dictionaries
- [CHANGE] Consistent syntax coloring across all editor panels

### Both Apps
- [NEW] Dark mode

## [0.735] - 2026-06-21

### Both Apps
- [NEW] yTranslator: translation workbench with spell check, annotations, history, and translation suggestions
- [NEW] yEditor: plugin conflict viewer and merged patch creator (xEdit-like)
- [NEW] JSON dictionary format with per-entry status tracking
- [CHANGE] Rewritten from scratch as a Qt6 application suite
