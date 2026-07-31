# Changelog Categories

## Tags

- `[NEW]` — a capability that didn't exist before.
- `[CHANGE]` — an existing feature now behaves differently (workflow, logic, defaults, restrictions, renames). The user must adapt.
- `[FIX]` — something was broken and now works as originally intended. The user doesn't adapt — things just work correctly now.
- `[REMOVE]` — a feature or option that existed is gone entirely.

## Decision Test

- If the user would say "that wasn't working right" → `[FIX]`.
- If the user would say "that works differently now" → `[CHANGE]`.

Renames (menu items, labels, tabs) are `[CHANGE]` because the user sees a different name and must find the new location.

## Section Order

Sections within a version are ordered:

1. `### yTranslator`
2. `### yEditor`
3. `### Both Apps`

## Entry Ordering

Within each section, entries are sorted by tag:

1. `[NEW]`
2. `[CHANGE]`
3. `[FIX]`
4. `[REMOVE]`

When adding a new entry, insert it after the last entry of the same tag (or before the first entry of the next tag if none exist yet).

## Rules

- Never use `[FIX]` for a deliberate behavioral change, even if the old behavior was suboptimal.
- Never use `[CHANGE]` for a bug where the feature simply didn't do what it was supposed to.
- If a fix also changes visible behavior (e.g. a broken feature now works but differently than users expected), use `[FIX]` and describe the correct behavior.
- Settings pages are infrastructure, not features. Do not list them in the CHANGELOG or README. Describe the capability the setting controls instead.

## Update Documents

When asked to "update documents" or "update docs", update all of the following:

- `README.md` — feature summary
- `CHANGELOG.md` — version history
- `docs/README.bbcode` — BBCode version of the README for Nexus Mods
- `docs/yTranslator-Manual.md` — yTranslator user manual
- `docs/yEditor-Manual.md` — yEditor user manual
- `docs/yampt-CLI-Manual.md` — CLI user manual

Only update files relevant to the change. A yEditor-only feature does not touch the yTranslator manual.
