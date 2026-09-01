# Always Update Docs

Every user-visible change ships with its documentation update in the same task. Do not ask whether to update docs, and do not defer it — update them as part of the change.

## Rule

When a change adds, alters, or removes anything a user can see or do (features, behavior, menu labels, dialog text, tooltips, tabs, settings effects, workflow), update all relevant documents in the same turn:

- `CHANGELOG.md` — tagged entry per the changelog-categories rule
- `README.md` and `docs/README.bbcode` — when the change is feature-level (keep the two mirrored)
- `docs/yTranslator-Manual.md`, `docs/yEditor-Manual.md`, `docs/yampt-CLI-Manual.md` — the manual(s) for the affected app

Only touch the documents relevant to the change (a yEditor-only change does not touch the yTranslator manual). Follow the manual-style rule for prose and the changelog-categories rule for tags, ordering, and the full "update docs" definition.

## Not User-Visible = No Docs

Internal-only changes (refactors, build system, tests, scripts, private helpers, decoder labels with no user-facing surface) do not go in the CHANGELOG, README, or manuals. See the not-implemented and changelog-categories rules for what is excluded.

## Never Split Code and Docs

Never land a user-visible code change without its doc update, and never write a doc entry for something not yet implemented (see not-implemented). Code and docs move together.
