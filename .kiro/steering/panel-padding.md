# Panel Padding

Tab panels and dockable view widgets in yTranslator and yEditor follow one padding convention, decided by what the panel's outer layout directly contains.

## Rule

- **Single full-bleed widget** (the panel is just one `QListWidget`, `QTreeView`, `QTreeWidget`, `QPlainTextEdit`, `QTextBrowser`, `QScrollArea`, or `QSplitter` that fills the whole tab): outer layout margins are `(0, 0, 0, 0)`. The contained widget draws to the panel edges.

- **Mixed content** (the panel's outer layout holds several widgets — buttons, labels, combo boxes, checkboxes, field rows, etc.): outer layout margins are `(2, 2, 2, 2)`.

- **Scroll-area content padding**: when a panel is a single `QScrollArea` (so its outer layout is `(0, 0, 0, 0)` per the full-bleed rule), the inner content layout inside the scroll area uses `(4, 4, 4, 4)` so the scrolled entries have breathing room from the scroll viewport edges. This is the only place `4` is used.

## Examples

Full-bleed → `(0, 0, 0, 0)`:
- `filter_tree_view`, `status_filter_view`, `sidebar_view` (single list/tree)
- `log_view`, `messages_view` (single `QPlainTextEdit`)
- `nav_tree_view`, `lua_tree_view` (single `QTreeView`)
- `annotations_view` (single `QListWidget`)
- `history_view` in both apps (single `QScrollArea`; inner entries layout is `(4, 4, 4, 4)`)

Mixed content → `(2, 2, 2, 2)`:
- `translation_suggestion_view` (combos, buttons, labels, log area)
- Find & Replace toolbar (`m_replace_toolbar`: fields, checkboxes, buttons)

## Consistency

- The two history panels (yTranslator and yEditor) must keep identical panel padding: outer `(0, 0, 0, 0)`, inner entries `(4, 4, 4, 4)`. Structural differences (yTranslator rows carry a Revert button) do not change the panel-level padding.
- Equivalent panels across the two apps use the same padding.

## Not Covered

- Top-level window containers (`plugin_workspace_view` main layout, the main-window central layout) are not tab panels; they are outside this rule.
- Per-row or per-item internal layouts inside a panel (e.g. a history row's `(2, 2, 2, 2)` row layout) are content styling, not panel padding.
