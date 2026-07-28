Audit the yampt codebase. Find exactly 5 issues from any of these categories:

1. **Code bugs** — logic errors, off-by-one, null dereference, race conditions, undefined behavior
2. **Steering drift** — code that violates a steering rule (naming, nesting, line limits, file placement, comment presence, missing tooltips, missing tr() wrapping, etc.)
3. **Dead code** — unused functions, unreachable branches, stale includes, orphaned files
4. **Architecture violations** — wrong project dependency direction, Qt in yampt.core, missing vcxproj.filters sync, split classes, wrong folder placement
5. **Documentation drift** — README/CHANGELOG/manual text that contradicts current code behavior, or missing entries for shipped features
6. **Steering contradictions** — two steering rules that conflict, or a steering rule that no longer matches reality
7. **Better solutions** — a place where the current implementation is a workaround, shortcut, or overly complex compared to what the architecture supports

## Instructions

- Read the relevant source files before claiming anything.
- Report exactly 5 issues per response.
- Format (repeat for each issue):

**Category:** (from list above)
**Location:** file path + line range
**Problem:** one-paragraph description of what's wrong
**Evidence:** the code/text snippet proving it
**Proposed fix:** concrete description of the change (don't implement yet)

- Do NOT implement the fix. Wait for my approval.
- Do NOT report issues already listed in the steering `analysis-findings.md` files.
- Do NOT report issues in test files unless they violate the "don't delete tests" rule.
- Prioritize: bugs > architecture violations > steering drift > dead code > docs > better solutions > contradictions.
- If you found nothing after a thorough scan, say "No issues found" — do not invent problems.
