# No Regressions

Existing working behavior must keep working. A change may only add new behavior or fix a genuine bug. It must never make something that worked before stop working or silently work differently.

This rule exists because of a concrete failure: a consolidation refactor that folded the five list-merge mechanisms (ENAM, FACT, ARMO, NPCO, NPCS) into one keyed model broke all of them — ENAM merged non-existent fields, FACT and ARMO stopped merging entirely — and had to be reverted. That is the exact class of outcome this policy prevents.

## Forbidden

- Changing a code path that already works in order to clean up, unify, consolidate, or refactor, when that change alters behavior which has passing tests or known-correct output.
- Replacing a working mechanism with a "better" or more general one unless the replacement is proven behavior-identical first.
- Broad rewrites that put working features at risk for a non-functional goal (fewer files, one table, less duplication). The reverted consolidation is the cautionary example — do not reattempt it.

## Allowed

- Adding genuinely new behavior on an isolated path that does not touch existing working code. Example: CELL currently drops the FRMR region, so adding FRMR merge there regresses nothing.
- Fixing an actual bug. Making broken behavior correct is not a regression — it is the point.

## How to comply

- Prefer additive, isolated changes over editing shared working code.
- Before changing a working path, treat its existing tests and output as a contract. If you cannot show the change is behavior-preserving, do not make it.
- If a refactor cannot be proven safe, do not do it — or gate it behind the smallest possible surface and keep every existing mechanism intact.
- When in doubt, ask before altering working behavior. Do not assume a refactor is worth the risk.
