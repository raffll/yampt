# Enable vs Hide

Two distinct UI concepts. Never conflate them.

- **Enabled / disabled** — the control stays visible but is greyed out and non-interactive when disabled. The user still sees it exists; it just cannot be used in the current state. Use `setEnabled(false)` (QAction/QWidget). Never remove or hide a control to express "disabled".
- **Show / hide** — the control is not present at all. Hiding means it does not appear in the UI. Use this only when the control is irrelevant in the current context, not merely temporarily unavailable.

## Rule

When a request says "enable" or "disable" an action, keep it visible and toggle its enabled state (greyed out when disabled). When a request says "show" or "hide", add or remove the control from the UI entirely.

For context-menu actions that cannot be greyed (a menu built fresh each time), "disabled" means adding the action with `setEnabled(false)` so it appears greyed in the menu — not omitting it.

## Menu Item Default

Every menu item (context menu or menu bar) is visible by default and stays in the menu regardless of state. When its precondition is not met, add it with `setEnabled(false)` so it appears greyed out — never omit it from the menu. Omit a menu item only when it is fundamentally irrelevant to the node or context, not when it is merely temporarily unavailable.
