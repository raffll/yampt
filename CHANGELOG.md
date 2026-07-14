# Changelog

## [xxx]

### Added — yTranslator
- Translation engine: pre-substitution of glossary terms before sending to AI model
- Translation engine: SCTX/BNAM tokenization — only translatable strings are sent to model
- Translation engine: status `model` assigned to AI-translated entries
- Translation engine: model-translated entries do not propagate automatically
- Translation panel: DeepL and Google providers shown in combo (marked as not supported)
- Book Preview: MWScript syntax highlighting for SCTX/BNAM entries (keywords, strings, comments)
- Book Preview: live update while editing script entries
- Inline table editing: double-click Translation column to edit single-line entries directly
- Language settings: simplified panel (foreign/native language, spell check, tags)
- Language settings: encoding and translation target auto-derived from language selection
- Language settings: spell check auto-set to None when dictionary not found
- First run dialog: added Italian and Hungarian languages
- Spell check dictionaries: added de_DE, fr_FR, ru_RU, it_IT, hu_HU
- Script editor: `say` keyword sound file path hidden from display and translation
- Lua l10n workflow: auto-pairs foreign/native YAML files in the same directory
- Lua l10n workflow: opening a native YAML allows editing, opening a foreign YAML shows read-only reference
- Lua l10n workflow: save writes only translated entries to the native file
- Lua l10n workflow: export creates a scaffold native file with all keys
- Merge Dictionaries in Tools menu
- View menu: Spell Check toggle

### Added — yEditor
- Guard Patch: plugins before guard are excluded from merge for records the guard contains
- Status bar shows load mode and path

### Added — Both Apps
- Localization support: all UI strings wrapped with `tr()` for Qt translation system

### Changed — yTranslator
- Translation button populates the translation editor with result
- Translation button only works on untranslated entries
- Translation button commits immediately and advances to the next row
- Translation button works for YAML documents (not just dict)
- Inline table editing advances to the next row after commit
- Polymorphic document commit: unified commit flow for all document types
- Document permissions control context menu and shortcut availability
- Make Dict and Make Dict with Base merged into single "Make Dictionary" menu item
- Make Base dialog: removed dictionary combo box (uses language settings)
- All operation dialogs use consistent 450×400 initial size
- API key fields: removed Show/Hide toggle buttons (always masked)
- Convert/Create: preserves original file timestamp

### Changed — yEditor
- Right panel columns redistribute on resize to fill available width
- Nav tree decodes codepage characters correctly
- Filter dialog: record type list unchecked by default (all unchecked = show all)

### Fixed — yTranslator
- Annotation highlight misalignment
- Dictionary marked dirty on row click without editing
- SCTX/BNAM validation: quotes no longer flagged as forbidden characters
- Whitespace markers: newline indicator now visible at line breaks
- Spell check: dictionary now loads correctly on startup
- Grammar check: missing punctuation no longer flagged while cursor is at end of text

## [0.842] - 2026-07-05

### Added — yEditor
- Automatic merged patch creation (leveled lists, dialogues, three-way object merge)
- Fog fix, summon fix, and cell name fix applied automatically to merged patches
- Copy records, groups, or individual fields to the merged patch via right-click menu
- Remove records and groups from the merged patch via right-click menu
- Decoded field view for CELL objects, leveled lists, containers, factions
- Advanced filter dialog (conflict level, override status, record type, ID, name, deleted records)
- Side-by-side text comparison panel
- Settings dialog (appearance, paths, merge options)
- Session persistence — remembers loaded plugins and window state between runs
- Configurable merge: exclude plugins or records by pattern, toggle record types and automatic fixes

### Added — yTranslator
- Settings dialog (appearance, shortcuts, language, translation engine)
- Workspace folder watches for file changes and refreshes automatically
- Multi-layer highlighting (MWScript, hyperlinks, glossary, forbidden characters)
- Merge dialog for combining dictionaries

### Improved — yEditor
- Conflict coloring now works at the individual field level (not just whole records)
- Navigation tree inherits worst-case conflict color from children up to file level
- Entries from different plugins aligned by content identity (item ID, object index, rank) instead of file order

### Improved — yTranslator
- Consistent syntax coloring across all editor panels

### Added — Both Apps
- Dark mode

## [0.735] - 2026-06-21

### Added
- yTranslator: translation workbench with spell check, annotations, history, and translation suggestions
- yEditor: plugin conflict viewer and merged patch creator (xEdit-like)
- JSON dictionary format with per-entry status tracking

### Changed
- Rewritten from scratch as a Qt6 application suite
