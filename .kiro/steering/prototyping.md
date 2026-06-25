# Prototyping

When adding a new code path (e.g. a record type handler, alignment logic, format decoder), create a separate function — even if the logic is nearly identical to an existing one. Duplicate first, refactor later.

Do NOT add if/else branches inside existing functions to handle new cases. The branching makes both paths harder to read and harder to modify independently. A standalone function is easier to prototype, test in isolation, and eventually merge or delete.

Refactoring into shared code happens after both paths are stable and proven correct.
