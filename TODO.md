# TODO

## Medium

- EET dictionary import is incomplete: importing an EET dictionary converts to JSON but produces incomplete output; finish the EET-to-JSON import so the conversion is complete — yTranslator
- PNAM etc should not be taken from last?
- colors blend together / hard to distinguish — adjust the palette so states are easier to tell apart (user reported with screenshot) — yTranslator
- let the user customize the colors in settings (bigger task than just adjusting defaults) — yTranslator
- unify validation feedback across apps: yTranslator informs (three-tier, own validator) while yEditor blocks (binary, validator in yampt.core) — contradicts Consistent Across Apps; needs a design decision (shared validation core, unified message wording, deliberate block-vs-inform per app) before implementing

## Hard

- Cross-workspace search window: search a phrase across all open dictionaries, results in a separate window with dictionary name column and highlighted match — yTranslator
- value changed on locked field doesn't survive merged patch — yEditor

## Hard to Fix

- editor diff off by couple characters
- NPDT 12 vs 52 merged patch

