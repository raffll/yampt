---
name: audit
description: Audit the yampt codebase for bugs, GUI behavior issues, steering-rule violations, dead code, architecture violations, documentation drift, steering contradictions, and better-solution opportunities. Use when the user asks to audit, review, or find issues in the codebase.
---

# Codebase Audit

Audit the yampt codebase. Find issues from any of these categories:

1. **Code bugs** — logic errors, off-by-one, null dereference, race conditions, undefined behavior
2. **GUI behavior** — visible UI glitches, wrong widget state, missing feedback, broken interactions, features running when disabled
3. **Steering drift** — code that violates a steering rule (naming, nesting, line limits, file placement, comment presence, missing tooltips, missing tr() wrapping, etc.)
4. **Dead code** — unused functions, unreachable branches, stale includes, orphaned files
5. **Architecture violations** — wrong project dependency direction, Qt in yampt.core, missing vcxproj.filters sync, split classes, wrong folder placement
6. **Documentation drift** — README/CHANGELOG/manual text that contradicts current code behavior, or missing entries for shipped features
7. **Steering contradictions** — two steering rules that conflict, or a steering rule that no longer matches reality
8. **Better solutions** — a place where the current implementation is a workaround, shortcut, or overly complex compared to what the architecture supports

## Step 0 — Audit the Steering Rules and This Skill First

Before auditing any code, audit the Kiro configuration itself. Read every file in
`.kiro/steering/`, every file in `.kiro/docs/`, every hook in `.kiro/hooks/`, and
this skill (`.kiro/skills/audit/SKILL.md`), then check for:

- **Internal contradictions** — two rules that conflict, a rule that contradicts a doc, or a rule that contradicts itself.
- **Stale references** — a steering rule, doc, or hook pointing at a file, path, class, command, or doc that no longer exists or was renamed (e.g. a `.kiro/docs/` pointer that moved, a hook invoking a missing script).
- **Duplication** — the same content stated in two steering files, in both a steering file and a doc, or across two docs.
- **Rule vs reference separation** — steering files must contain only behavioral rules; domain knowledge, formats, and architecture belong in `.kiro/docs/`. Flag domain content that leaked into steering, or rules buried in docs.
- **Content that no longer matches reality** — a steering directive, doc, or hook describing structure or behavior that has since changed.
- **Hook correctness** — each `.kiro/hooks/*.json` is valid (trigger, matcher, action), its command/script exists, and it does not conflict with another hook or a steering rule.
- **This skill's own correctness** — categories, paths, and the `known-issues.md` reference below are all still valid.

Report any Step 0 findings as category **Steering contradictions** using the same
format below, before the code findings. If the steering rules, docs, hooks, and
this skill are all clean, say "Kiro config (steering, docs, hooks, skill): clean"
and proceed to the code audit.

## Instructions

- Read the relevant source files before claiming anything.
- Report every issue you find — there is no cap. Do not stop at a fixed count or artificially trim the list.
- Present findings as a numbered list in the chat response.
- Always wait for explicit approval before implementing any fix. Never fix issues without asking first.
- Format (repeat for each issue):

**Category:** (from list above)
**Location:** file path + line range
**Problem:** one-paragraph description of what's wrong
**Evidence:** the code/text snippet proving it
**Proposed fix:** concrete description of the change (don't implement yet)

- Do NOT implement the fix. Wait for my approval.
- Do NOT report issues already listed in `.kiro/docs/known-issues.md`.
- Do NOT report issues in test files unless they violate the "don't delete tests" rule.
- Prioritize: GUI behavior > bugs > architecture violations > steering drift > dead code > docs > better solutions > contradictions.
- If you found nothing after a thorough scan, say "No issues found" — do not invent problems.

## Summary by Urgency Tier

At the end of the chat response, summarize all findings as a list grouped into urgency tiers:

- **Critical** — data loss, crashes, security, silently wrong output.
- **High** — broken behavior, architecture violations, GUI defects users will hit.
- **Medium** — steering drift, dead code, maintainability issues.
- **Low** — docs drift, better-solution opportunities, cosmetic.

Under each tier, list its issues as one line each: `#N — <category> — <one-line description>`. Omit a tier that has no issues.

## Save the Report to a Dated File

After presenting the findings and the tiered summary in chat, also write the full
report to `.kiro/audits/audit-<YYYY-MM-DD>.md` (create the `.kiro/audits/`
directory if needed). Use the current date. If a file for today already exists,
append a time-stamped section rather than overwriting it.

The file contains the same content as the chat response: every finding in the
per-issue format above, followed by the urgency-tier summary. This is the only
file the audit may create — do not write any other output files.
