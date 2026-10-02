# TODO

## Easy

- title should also add [Active]
- remove save, save all, and any code related to saving file on demand, asterisk etc
- Filter bar: add a clear button (as an alternative to Esc) — yTranslator
- Plugin list navigation: pressing a letter key should jump to the first plugin starting with that letter — yEditor
- Hyperlink highlighting in original/translated text boxes should only apply to DIAL/INFO record types, not to item names, cell names etc. — yTranslator
- Propagation should skip entries already marked as `translated` — yTranslator

## Medium

- Fix capitalisation bug: changing case on one entry (e.g. CELL) incorrectly changes another (e.g. DIAL) that shares the same `old_text` — yTranslator
- Show NPC gender in the FNAM section — visual indicator (icon or label) — yTranslator
- advanced filters: locked only, excluded only — yEditor
- Auto-save after edit not working — user applied an FNAM edit but changes were not saved to disk — yEditor
- PNAM etc should not be taken from last?

## Hard

- Cross-workspace search window: search a phrase across all open dictionaries, results in a separate window with dictionary name column and highlighted match — yTranslator
- Records misclassified with long modlists — Script records appear under CELL, Armor under NPC; happens with plugins near end of load order (v0.1233) — yEditor
- Crash when loading a second ESP from a different folder — yEditor
- Investigate whether UTF-8 / GBK encoded plugin files cause silent save failure or data corruption — yEditor
- lua in advanced filters — yEditor
- value changed on locked field doesn't survive merged patch — yEditor

## Hard to Fix

- editor diff off by couple characters
- NPDT 12 vs 52 merged patch
