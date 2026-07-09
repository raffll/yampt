# Changelog

## [xxx]
- Transaltor -> annottaion highlight misaligmnet fix
- Translator -> support for more languages


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
