# No Fallbacks, No Generic Paths

Never add a fallback branch or a generic catch-all path. Every case must be handled explicitly and correctly by name/type/schema. A fallback always hides a real mismatch and breaks a different case that was relying on the exact behavior.

## Rule

- Do NOT add "if the specific lookup fails, use something close" logic (e.g. same-index field, nearest size, first-match, whole-value byte guess, default schema).
- Do NOT add a generic branch that runs when no specific rule matches. If nothing matches, that is a bug in the data/schema — fix the data/schema so the specific path matches.
- When a lookup by name/type/size can miss, the fix is to make the name/type/size correct at the source (schema tables, record definitions), NOT to soften the lookup.
- If a value genuinely cannot be resolved, surface it loudly (log `[error]`, or show an explicit error marker), never silently substitute an approximation.

## Why

Fallbacks and generic paths look harmless but are the top source of regressions here:
- A schema-decode "same-index" fallback fixed one field mismatch but broke the NPC autocalc case (fields legitimately absent at 12-byte NPDT started showing wrong values).
- A whole-value byte merge fallback dropped per-field/per-bit merges for every sub-record not on an allow-list.

Each time, the fallback masked the real problem and corrupted an unrelated case. The correct fix is always the specific, explicit one.

## What To Do Instead

1. Identify the exact case that isn't matching.
2. Fix the schema/definition/table so the specific lookup matches by name/type/size.
3. If the case is genuinely distinct, add an explicit branch for it — named, not generic.
4. If it cannot be resolved, emit an explicit error, never a guess.
