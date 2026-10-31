# TODO

Bugs take precedence over features — always fix what is broken before adding anything new.

## Bugs

- EET dictionary import is incomplete: the easy gaps (weapon names, script message routing, status) are fixed; what remains is full-fidelity import of dialogue INFO and related structural entries. Producing a 100%-complete, applicable dictionary likely requires cross-referencing the source plugin, which is a design/UX decision — yTranslator

## Features

- let the user customize the colors in settings (bigger task than just adjusting defaults) — yTranslator
- colors blend together / hard to distinguish — adjust the palette so states are easier to tell apart (user reported with screenshot) — yTranslator
- Cross-workspace search window: search a phrase across all open dictionaries, results in a separate window with dictionary name column and highlighted match — yTranslator
- unify validation feedback across apps: yTranslator informs (three-tier, own validator) while yEditor blocks (binary, validator in yampt.core) — contradicts Consistent Across Apps; needs a design decision (shared validation core, unified message wording, deliberate block-vs-inform per app) before implementing
