# TODO

## Easy

- loading ESPs from a second folder crashes the editor silently: "open folder" loads all ESPs in one folder fine, but once a folder is loaded, opening an ESP/folder from a different location closes yEditor with no message. Intended behavior: unload the current folder and load the new one (loading only, no live multi-folder watch — parsing is too slow for that) — yEditor

## Medium

- LEVI (leveled list) INDX count not updated when editing after a merged patch: the INDX item count is correct when the merged patch is first created, but copying/editing items in a LEVI afterward does not recompute INDX, so the count no longer matches the number of INAM/INTV item entries. The engine then reads a LEVI where it expects an INAM and the file becomes unreadable — reported error "ESM Error: Unknown subrecord / Record: LEVI / Subrecord: INAM" in OpenMW and the CS; re-creating the merged patch without edits fixes it. Recompute INDX on every LEVI edit. Check containers (NPCO) for the same count-desync. — yEditor
- LEVI/container duplicate items collapsed in the view: when a leveled list (or container) holds multiple copies of the same item, yEditor shows only one entry, so the displayed entry count disagrees with INDX. Example: base-game "l_b_amulets" has 3 items (2x "l_n_amulet", 1x "l_m_amulet") but yEditor shows a single "l_n_amulet" row — two rows against an INDX count of 3. Likely the same root cause as the INDX desync above (dedup on display dropping duplicate item rows). Preserve and display every duplicate item. — yEditor
- propagation is one-way: translating a text sets all duplicate occurrences to Propagated, but later editing one occurrence changes only that single record (to In Progress) and does not re-propagate the edit to the other duplicates — yEditor
- advanced filters: locked only, excluded only — yEditor
- EET dictionary import is incomplete: importing an EET dictionary converts to JSON but produces incomplete output; finish the EET-to-JSON import so the conversion is complete — yTranslator
- PNAM etc should not be taken from last?
- colors blend together / hard to distinguish — adjust the palette so states are easier to tell apart (user reported with screenshot) — yTranslator
- let the user customize the colors in settings (bigger task than just adjusting defaults) — yTranslator
- unify validation feedback across apps: yTranslator informs (three-tier, own validator) while yEditor blocks (binary, validator in yampt.core) — contradicts Consistent Across Apps; needs a design decision (shared validation core, unified message wording, deliberate block-vs-inform per app) before implementing

## Hard

- Cross-workspace search window: search a phrase across all open dictionaries, results in a separate window with dictionary name column and highlighted match — yTranslator
- MO2 mode: resolve Kezyma OpenMW Player stub plugins — a `.omwaddon.esp` / `.omwscript.esp` double-extension file is an empty placeholder MO2 uses to put the real `.omwaddon`/`.omwscript` into the load order; yEditor loads the empty stub instead of the real file. In MO2 mode, detect the double extension and load the real file (strip the trailing `.esp`). Only affects the MO2 loading path — loading the OpenMW config directly already reads the real `.omwaddon`/`.omwscript` files correctly. Same `.esp`-suffix trick applies to other MO2-invisible plugins (e.g. deltaplugin). — yEditor
- value changed on locked field doesn't survive merged patch — yEditor

## Hard to Fix

- editor diff off by couple characters
- NPDT 12 vs 52 merged patch

