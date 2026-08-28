# Consistent Across Apps

When implementing a feature or pattern that exists in one app (yTranslator or yEditor), use the same approach in the other app if applicable. This includes:

- Settings persistence: use the same `settings_store_t` methods and INI key naming
- View menu toggles: same action names, same signal/slot pattern (`QAction::toggled` → `QWidget::setVisible`)
- Toolbar layout: same button naming conventions
- Session state save/restore: same pattern (`save_session_state` / `restore_session_state` or `save_config` / `load_config`)

Before implementing something new, check if the other app already solved the same problem. If it did, replicate the pattern — do not invent a different approach.
