# Committing

## Only Commit When Asked

Never create a commit unless the user explicitly asks. Staging (`git add`) for inspection is fine; committing is not, until requested.

## Respect the Requested Scope

When the user says "commit all", stage and commit the entire working tree in a single commit. Do not try to split it into multiple commits, do not hand-craft partial patches, and do not reorganize staging. Do exactly what was asked.

When the user asks for separate commits by concern, split them — otherwise default to one commit for everything currently changed.

## No Patch Surgery

Never split a single file's changes across commits with `git apply`, hand-written patch hunks, or interactive line staging unless the user explicitly requests that granularity. It is fragile and error-prone. Prefer whole-file staging.

## Submodules

`git add -A` stages submodule directories as gitlinks (mode `160000`). The "adding embedded git repository" advisory is harmless when `.gitmodules` already defines the submodule — verify with `git ls-files --stage <path>` showing mode `160000`, then proceed.

## Commit Message Style

- Short imperative subject line, under 70 characters.
- When one commit spans multiple concerns (because the user asked to commit everything at once), use a brief bulleted body listing each concern.
- Do not skip hooks (`--no-verify`) unless asked.
- Do not amend already-pushed commits. Prefer new commits.
- Never modify git config.

## Line Endings

CRLF/LF warnings from git on Windows (`autocrlf` normalization) are informational, not errors. Do not treat them as failures.
