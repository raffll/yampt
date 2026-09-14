Audit the yampt codebase. Find issues from any of these categories:

1. **Code bugs** — logic errors, off-by-one, null dereference, race conditions, undefined behavior
2. **GUI behavior** — visible UI glitches, wrong widget state, missing feedback, broken interactions, features running when disabled
3. **Steering drift** — code that violates a steering rule (naming, nesting, line limits, file placement, comment presence, missing tooltips, missing tr() wrapping, etc.)
4. **Dead code** — unused functions, unreachable branches, stale includes, orphaned files
5. **Architecture violations** — wrong project dependency direction, Qt in yampt.core, missing vcxproj.filters sync, split classes, wrong folder placement
6. **Documentation drift** — README/CHANGELOG/manual text that contradicts current code behavior, or missing entries for shipped features
7. **Steering contradictions** — two steering rules that conflict, or a steering rule that no longer matches reality
8. **Better solutions** — a place where the current implementation is a workaround, shortcut, or overly complex compared to what the architecture supports

## Instructions

- Read the relevant source files before claiming anything.
- Report exactly 10 issues per response.
- Present findings as a numbered list in the chat response. Do NOT create any output files (no AUDIT_RESULTS.md, no markdown documents).
- Always wait for explicit approval before implementing any fix. Never fix issues without asking first.
- Format (repeat for each issue):

**Category:** (from list above)
**Location:** file path + line range
**Problem:** one-paragraph description of what's wrong
**Evidence:** the code/text snippet proving it
**Proposed fix:** concrete description of the change (don't implement yet)

- Do NOT implement the fix. Wait for my approval.
- Do NOT report issues already listed in the steering `analysis-findings.md` files.
- Do NOT report issues in test files unless they violate the "don't delete tests" rule.
- Prioritize: GUI behavior > bugs > architecture violations > steering drift > dead code > docs > better solutions > contradictions.
- If you found nothing after a thorough scan, say "No issues found" — do not invent problems.
- At the end, add a summary table listing all issues sorted by priority (bugs first, then architecture, steering drift, etc.) with one-line descriptions.
