# Settings Dialog Layout

Both yTranslator and yEditor use the same settings dialog pattern:
- `setMinimumSize(600, 450)`
- Left panel: `QListWidget` with `setFixedWidth(150)` for category navigation
- Right panel: `QStackedWidget` for content pages
- Bottom: `QDialogButtonBox` with OK / Apply / Cancel

## Page Order

Both apps follow the same category order:
1. Appearance
2. Shortcuts
3. Language
4. Others (app-specific pages)

**yTranslator** "Others":
- Translation

**yEditor** "Others":
- Paths
- Merge

Missing panels (Shortcuts, Language in yEditor) will be added later.

## Rules

- Both dialogs must use the same default size (`setMinimumSize(600, 450)`)
- The left panel width is always 150px
- Appearance is always the first category, followed by Shortcuts, Language, then app-specific pages
- `setCurrentRow(0)` on open (always opens to Appearance)
- No separators in the category list — just plain items
