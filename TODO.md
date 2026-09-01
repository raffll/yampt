# TODO

both apps: backup saved files — before overwriting a file on save, copy it to a `.bak` in the same folder as the original. One on/off checkbox in a new "Backup" settings tab, in both yTranslator and yEditor. Spec first.

translator: whole-dictionary spell check — scan every entry's translation for typos using the native-language Hunspell dictionary, ignoring all-uppercase words (acronyms/tags). Mark entries that contain misspellings with a new `misspelled` status (added to status_types) so they can be filtered and reviewed. `misspelled` is NOT an approved status — it is skipped during --convert/--create like every non-`translated` status.

translator: simplify the AI prompt — keep only one language-variable pair (drop the `{{..._upper}}` variants). The source/target language is substituted automatically when the prompt is built; the language template variables are not exposed to the user. The Prompt settings box shows only the editable non-technical instruction text, pre-filled with the default instruction; the technical/language part is fixed and not editable.

translator: rename "Merge Dictionaries" to "Dictionary Merger" (menu action and dialog title). Add an output-filename edit box in the merge dialog, defaulting to a name derived from the inputs (e.g. `dict1+dict2_MERGED.json`).

translator: inflection generation for .top files uses the Hunspell spellcheck dictionary, whose affix coverage is incomplete — some valid inflected forms are never generated (e.g. Polish `chorobą`, because `choroba` lacks the affix flag that produces the singular instrumental). Consider a proper morphological dictionary (e.g. Morfeusz/SGJP for Polish) to generate full paradigms. Large dependency change and PL-only.

editor: drag & drop a decoded cell value between plugin columns when Enable Editing is on — dropping onto another plugin column applies the value as a field edit; dropping onto the merged-patch column copies it into the merge. Both are recorded in the History tab.

editor: when the nav selection changes, also reset/clear the Edit panel

editor: hisotry tab same as translator

maybe:
editor: use text [TAGS] instead of icons for Guard Patch, Exclude from Merged Patch, Active Plugin, Overridden-by-Override — or keep multiple icons but add tooltips
