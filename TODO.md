# TODO

## Medium

- advanced filters: locked only, excluded only — yEditor
- PNAM etc should not be taken from last?
- colors blend together / hard to distinguish — adjust the palette so states are easier to tell apart (user reported with screenshot) — yTranslator
- let the user customize the colors in settings (bigger task than just adjusting defaults) — yTranslator
- "Convert to base dictionary" — user asked for a right-click → convert-to-base action. To CHECK: base/non-base distinction was supposedly removed (all dicts uniform, [BASE] tag is just from the filename), so this may be moot — verify whether it still makes sense or whether something is actually needed — yTranslator

## Hard

- Cross-workspace search window: search a phrase across all open dictionaries, results in a separate window with dictionary name column and highlighted match — yTranslator
- Crash when loading a second ESP from a different folder — yEditor
- MO2 mode: resolve Kezyma OpenMW Player stub plugins — a `.omwaddon.esp` / `.omwscript.esp` double-extension file is an empty placeholder MO2 uses to put the real `.omwaddon`/`.omwscript` into the load order; yEditor loads the empty stub instead of the real file. In MO2 mode, detect the double extension and load the real file (strip the trailing `.esp`). — yEditor
- Investigate whether UTF-8 / GBK encoded plugin files cause silent save failure or data corruption — yEditor
- value changed on locked field doesn't survive merged patch — yEditor

## Hard to Fix

- editor diff off by couple characters
- NPDT 12 vs 52 merged patch
