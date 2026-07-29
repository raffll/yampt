# Manual Style

User manuals (`docs/*.md`) must be written in explanatory prose. Each section describes the feature as if explaining it to a user who has never seen the application.

## Rules

- Write full sentences, not terse bullet fragments.
- Explain what happens when the user performs an action, not just what the action is named.
- Describe the purpose and effect, not the implementation.
- Never mention internal technology, class names, file formats, or architecture decisions. The user does not care that something uses Qt, JSON configs, or CTranslate2 NLLB-600M — describe what it does for them.
- Bullet lists are acceptable for enumerating menu items, keyboard shortcuts, or settings pages — but each item should still have a complete explanation, not a two-word label.
- Do not repeat information across sections. State a fact once, in the most relevant section.
- Do not categorize lists with bold sub-headers unless the categories add real navigational value. A flat list with clear per-item descriptions is preferred.
- Keep the tone neutral and direct. No marketing, no exclamation marks, no "simply" or "just".
