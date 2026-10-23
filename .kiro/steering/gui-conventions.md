# GUI Conventions

## Tooltips

Every new QPushButton, QToolButton, or QAction that is visible in the UI must have a `setToolTip(...)` call immediately after creation. No exceptions.

Tooltips should be short (under 60 characters), descriptive, and written in sentence fragment style without a trailing period.

## Compact Tables

Every `QTableWidget` in settings dialogs must set compact row height:

```cpp
table->verticalHeader()->setDefaultSectionSize(24);
```

This applies to both yampt.translator and yampt.editor settings pages. The default Qt row height is too tall for dense data tables like provider lists and shortcut tables.

## Panel Padding

Tab panels and dockable view widgets in yTranslator and yEditor follow one padding convention, decided by what the panel's outer layout directly contains. Apply the principle; the code is the source of truth for which panel is which — do not maintain a per-panel registry here.

### Rule

- **Single full-bleed widget** — the panel's outer layout holds exactly one widget that fills the whole tab (a `QListWidget`, `QTreeView`, `QTreeWidget`, `QPlainTextEdit`, `QTextBrowser`, `QScrollArea`, or `QSplitter`): outer margins `(0, 0, 0, 0)`, so the widget draws to the panel edges.

- **Mixed content** — the outer layout holds several widgets (buttons, labels, combo boxes, checkboxes, field rows, etc.): outer margins `(2, 2, 2, 2)`.

- **Scroll-area inner content** — when a panel is a single `QScrollArea` (outer `(0, 0, 0, 0)` per the full-bleed rule), the content layout inside the scroll area uses `(4, 4, 4, 4)` so scrolled entries have breathing room from the viewport edges. This is the only place `4` is used.

### Consistency

- Equivalent panels across the two apps use the same padding for the same structure.
- If a panel changes structure (e.g. gains a button and stops being a single full-bleed widget), re-derive its padding from the rule above.

### Not Covered

- Top-level window containers (main-window central layout, an app's main workspace layout) are not tab panels; they are outside this rule.
- Per-row or per-item internal layouts inside a panel are content styling, not panel padding.

## Enable vs Hide

Two distinct UI concepts. Never conflate them.

- **Enabled / disabled** — the control stays visible but is greyed out and non-interactive when disabled. The user still sees it exists; it just cannot be used in the current state. Use `setEnabled(false)` (QAction/QWidget). Never remove or hide a control to express "disabled".
- **Show / hide** — the control is not present at all. Hiding means it does not appear in the UI. Use this only when the control is irrelevant in the current context, not merely temporarily unavailable.

### Rule

When a request says "enable" or "disable" an action, keep it visible and toggle its enabled state (greyed out when disabled). When a request says "show" or "hide", add or remove the control from the UI entirely.

For context-menu actions that cannot be greyed (a menu built fresh each time), "disabled" means adding the action with `setEnabled(false)` so it appears greyed in the menu — not omitting it.

### Menu Item Default

Every menu item (context menu or menu bar) is visible by default and stays in the menu regardless of state. When its precondition is not met, add it with `setEnabled(false)` so it appears greyed out — never omit it from the menu. Omit a menu item only when it is fundamentally irrelevant to the node or context, not when it is merely temporarily unavailable.

## No Keyboard Shortcuts Without Confirmation

Never add, remove, or change keyboard shortcuts without explicit user approval. Always propose the shortcut and wait for confirmation before implementing it.

This applies to all keybindings: menu accelerators, QAction shortcuts, QShortcut instances, and any other key sequence assignments.

## Consistent Across Apps

When implementing a feature or pattern that exists in one app (yTranslator or yEditor), use the same approach in the other app if applicable. This includes:

- Settings persistence: use the same shared settings store and INI key naming
- View menu toggles: same action names, same signal/slot pattern (`QAction::toggled` → `QWidget::setVisible`)
- Toolbar layout: same button naming conventions
- Session state save/restore: the same save/restore pattern already used by the other app

Before implementing something new, check if the other app already solved the same problem. If it did, replicate the pattern — do not invent a different approach.
