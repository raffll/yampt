# GUI Toolbar Buttons

All toolbar and toolbar-like bar buttons use `QToolButton`. Never use `QPushButton` on toolbars or horizontal action bars (replace bar, search bar).

- `QToolButton` — for all toolbar buttons, both action (click) and toggle (checkable). Flat by default, border on hover — native toolbar look.
- `QPushButton` — only for dialogs, panels, and editors (settings pages, merge dialog, history revert, editor Next button). Never on toolbars.
- `QAction` — only for menu bar items and context menus, never added directly to a toolbar via `toolbar->addAction()`.

This applies to both yampt.translator and yampt.editor.
