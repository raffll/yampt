# Documentation

## Remove, Don't Mark Done

In any document (TODO.md, CHANGELOG notes, planning docs, lists, steering, docs), never leave an entry marked as done, completed, resolved, fixed, or struck through, and never keep an entry describing something that no longer exists. Delete it outright. A document reflects the current state only — not a history of what was finished or removed. The CHANGELOG is the sole exception: it is a deliberate, versioned history and keeps its past entries.

Every user-visible change ships with its documentation update in the same task. Do not ask whether to update docs, and do not defer it — update them as part of the change.

### Rule

When a change adds, alters, or removes anything a user can see or do (features, behavior, menu labels, dialog text, tooltips, tabs, settings effects, workflow), update all relevant documents in the same turn:

- `CHANGELOG.md` — tagged entry per the changelog-categories rule below
- `README.md` and `docs/README.bbcode` — when the change is feature-level (keep the two mirrored)
- `docs/yTranslator-Manual.md`, `docs/yEditor-Manual.md`, `docs/yampt-CLI-Manual.md` — the manual(s) for the affected app

Only touch the documents relevant to the change (a yEditor-only change does not touch the yTranslator manual). Follow the manual-style rule for prose and the changelog-categories rule for tags, ordering, and the full "update docs" definition.

### Not User-Visible = No Docs

Internal-only changes (refactors, build system, tests, scripts, private helpers, decoder labels with no user-facing surface) do not go in the CHANGELOG, README, or manuals. See the "Not Implemented Yet" section and the changelog-categories rules for what is excluded.

### Never Split Code and Docs

Never land a user-visible code change without its doc update, and never write a doc entry for something not yet implemented (see "Not Implemented Yet"). Code and docs move together.

## Sync Docs With Code

When renaming or changing any user-visible text (menu labels, dialog titles, tooltips, button text, tab names), always update both:

1. The source code (the actual string)
2. The corresponding manual/README/CHANGELOG entry (if one exists)

Never change one without the other. If a label is renamed in code, find and update it in docs. If docs describe a label, verify the code matches.

## Changelog Categories

### Tags

- `[NEW]` — a capability that didn't exist before.
- `[CHANGE]` — an existing feature now behaves differently (workflow, logic, defaults, restrictions, renames). The user must adapt.
- `[FIX]` — something was broken and now works as originally intended. The user doesn't adapt — things just work correctly now.
- `[REMOVE]` — a feature or option that existed is gone entirely.

### Decision Test

- If the user would say "that wasn't working right" → `[FIX]`.
- If the user would say "that works differently now" → `[CHANGE]`.

Renames (menu items, labels, tabs) are `[CHANGE]` because the user sees a different name and must find the new location.

### Section Order

Sections within a version are ordered:

1. `### yTranslator`
2. `### yEditor`
3. `### Both Apps`

### Entry Ordering

Within each section, entries are sorted by tag:

1. `[NEW]`
2. `[CHANGE]`
3. `[FIX]`
4. `[REMOVE]`

When adding a new entry, insert it after the last entry of the same tag (or before the first entry of the next tag if none exist yet).

### Rules

- Never use `[FIX]` for a deliberate behavioral change, even if the old behavior was suboptimal.
- Never use `[CHANGE]` for a bug where the feature simply didn't do what it was supposed to.
- If a fix also changes visible behavior (e.g. a broken feature now works but differently than users expected), use `[FIX]` and describe the correct behavior.
- Settings pages are infrastructure, not features. Do not list them in the CHANGELOG or README. Describe the capability the setting controls instead.
- Never include unit tests, test files, or test-related changes in the CHANGELOG or README.
- Never include scripts (PowerShell, Python, automation) in the CHANGELOG or README.
- Never include build system changes (vcxproj, paths, MSBuild targets) in the CHANGELOG or README.
- Only user-visible features, fixes, and behavioral changes belong in the CHANGELOG.
- Never include fixes for regressions introduced in the same release. If a refactor broke something and we fixed it before shipping, neither the break nor the fix appears in the CHANGELOG.
- The README describes what the application does for end users — not internal tooling.

### Update Documents

When asked to "update documents" or "update docs", update all of the following:

- `README.md` — feature summary
- `CHANGELOG.md` — version history
- `docs/README.bbcode` — BBCode version of the README for Nexus Mods
- `docs/yTranslator-Manual.md` — yTranslator user manual
- `docs/yEditor-Manual.md` — yEditor user manual
- `docs/yampt-CLI-Manual.md` — CLI user manual

Only update files relevant to the change. A yEditor-only feature does not touch the yTranslator manual.

### BBCode Sync Rule

`docs/README.bbcode` must always mirror `README.md` in structure, content, and emphasis. When updating the README, rewrite the BBCode file to match — same sections, same groupings, same bold items. Never update one without the other.

## Manual Style

User manuals (`docs/*.md`) must be written in explanatory prose. Each section describes the feature as if explaining it to a user who has never seen the application.

### Rules

- Write full sentences, not terse bullet fragments.
- Explain what happens when the user performs an action, not just what the action is named.
- Describe the purpose and effect, not the implementation.
- Never mention internal technology, class names, file formats, or architecture decisions. The user does not care that something uses Qt, JSON configs, or CTranslate2 NLLB-600M — describe what it does for them.
- Bullet lists are acceptable for enumerating menu items, keyboard shortcuts, or settings pages — but each item should still have a complete explanation, not a two-word label.
- Do not repeat information across sections. State a fact once, in the most relevant section.
- Do not categorize lists with bold sub-headers unless the categories add real navigational value. A flat list with clear per-item descriptions is preferred.
- Keep the tone neutral and direct. No marketing, no exclamation marks, no "simply" or "just".

## Not Implemented Yet

Do not include unfinished features in the README, CHANGELOG, or user manuals.

### Pre-Release Documentation Is Valid

Features listed in the CHANGELOG `[XXX]` section (unreleased) may already be documented in the README, manuals, and BBCode. This is intentional — documentation is written alongside implementation, not deferred until release. Do not report this as documentation drift.
