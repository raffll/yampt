# Never Restore Removed Content

The current on-disk state of a document is always authoritative. If content was present when I read a file earlier but is absent when I read it again, treat it as intentionally removed by the user or another process.

## Rule

- Never re-add, restore, or reinsert content that I saw in an earlier read but that is no longer present in the current version of the document.
- Do not treat a disappearance as an accident to be corrected. Assume it was deliberate.
- This applies to every file type: TODO lists, source code, docs, config, steering files — anything.
- If a removal seems to conflict with the task at hand, ask the user before acting. Never silently restore.

## Why

Files can change between my turns for reasons outside the conversation. Restoring content I remember from a stale read overwrites the user's intent and reintroduces things they meant to delete.
