# Panel Padding

Tab panels and dockable view widgets in yTranslator and yEditor follow one padding convention, decided by what the panel's outer layout directly contains. Apply the principle; the code is the source of truth for which panel is which — do not maintain a per-panel registry here.

## Rule

- **Single full-bleed widget** — the panel's outer layout holds exactly one widget that fills the whole tab (a `QListWidget`, `QTreeView`, `QTreeWidget`, `QPlainTextEdit`, `QTextBrowser`, `QScrollArea`, or `QSplitter`): outer margins `(0, 0, 0, 0)`, so the widget draws to the panel edges.

- **Mixed content** — the outer layout holds several widgets (buttons, labels, combo boxes, checkboxes, field rows, etc.): outer margins `(2, 2, 2, 2)`.

- **Scroll-area inner content** — when a panel is a single `QScrollArea` (outer `(0, 0, 0, 0)` per the full-bleed rule), the content layout inside the scroll area uses `(4, 4, 4, 4)` so scrolled entries have breathing room from the viewport edges. This is the only place `4` is used.

## Consistency

- Equivalent panels across the two apps use the same padding for the same structure.
- If a panel changes structure (e.g. gains a button and stops being a single full-bleed widget), re-derive its padding from the rule above.

## Not Covered

- Top-level window containers (main-window central layout, an app's main workspace layout) are not tab panels; they are outside this rule.
- Per-row or per-item internal layouts inside a panel are content styling, not panel padding.
