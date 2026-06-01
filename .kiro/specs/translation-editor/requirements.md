# Requirements Document

## Introduction

A graphical two-panel translation editor for yampt XML dictionaries. The editor provides a WinMerge-like side-by-side layout where the left panel shows the user's editable translation dictionary and the right panel shows source/untranslated records for reference. The goal is to replace the current workflow of manually editing XML files in a text editor with a purpose-built tool that understands the yampt dictionary structure.

## Technology Recommendation

**Recommended: Dear ImGui** (with SDL2 or GLFW backend)

Rationale:
- **Same language (C++)** — integrates directly into the existing yampt Visual Studio solution, reuses the existing `DictReader`/`DictMerger`/`Tools` classes without any bridging layer
- **Minimal dependency** — single header + a few source files, no installer or runtime distribution needed
- **Fast to develop** — immediate-mode paradigm means no widget lifecycle management, no signal/slot wiring, no layout XML; the UI is just C++ function calls each frame
- **Lightweight** — produces a small executable with no DLL dependencies beyond the OS
- **Good enough visuals** — supports scrollable lists, text input, split panels, filtering; not native-looking but perfectly functional for a developer tool
- **Easy deployment** — single .exe, no Qt DLLs, no Python runtime, no .NET framework requirement

Alternatives considered:
- Qt: Excellent widgets but adds ~50MB of DLLs and significant build complexity (MOC, CMake integration)
- C# WinForms: Native look, fast development, but requires a separate project in a different language and cannot reuse yampt's C++ dictionary classes directly
- Python + PyQt: Fastest prototyping but adds a Python runtime dependency and cannot reuse C++ code without bindings

## Glossary

- **Editor**: The graphical translation editor application
- **Dictionary**: A yampt dictionary file (XML or JSON format) containing translation records organized by record type
- **Record**: A single key-value translation entry within a dictionary, identified by its type and key
- **Record_Type**: The category of a record (CELL, DIAL, INFO, FNAM, TEXT, GMST, DESC, RNAM, INDX, BNAM, SCTX)
- **Left_Panel**: The editable panel displaying the user's translation dictionary
- **Right_Panel**: The read-only reference panel displaying source/untranslated records
- **Key**: The original-language text that identifies a record (content between `<key></key>` tags)
- **Value**: The translated text for a record (content between `<val></val>` tags)
- **Source_Dictionary**: A NOTFOUND, CHANGED, or RAW dictionary used as reference in the right panel
- **User_Dictionary**: The dictionary being edited by the user in the left panel
- **Status**: The translation state of a record (Untranslated, Draft, Validated, Needs_Review)
- **DIAL_Topic**: A dialogue topic name (DIAL record Key) that appears as a clickable hyperlink in-game within INFO text
- **MWScript**: The scripting language used by Morrowind's engine, found in SCTX records
- **Glossary**: A dictionary of FNAM-derived name translations (NPC names, item names, place names) used as reference annotations
- **NPC_FLAG**: A dictionary mapping NPC IDs to their gender flag (M for male, F for female), used to indicate the speaker's gender in INFO records

## Requirements

### Requirement 1: Load User Dictionary

**User Story:** As a translator, I want to open my translation dictionary file in the left panel, so that I can view and edit my translations.

#### Acceptance Criteria

1. WHEN the user selects a dictionary file to open, THE Editor SHALL parse the file using the appropriate parser (JSON for `.json` files, XML for `.xml` files) and display all records in the Left_Panel
2. WHEN a dictionary file contains records of multiple Record_Types, THE Editor SHALL group and display records by their Record_Type
3. IF a dictionary file cannot be parsed, THEN THE Editor SHALL display an error message indicating the file path and the nature of the parsing failure
4. THE Editor SHALL preserve the original encoding of the dictionary file (windows-1250, windows-1251, windows-1252) when loading records
5. THE Editor SHALL support both JSON and XML dictionary formats transparently, detecting the format by file extension

### Requirement 2: Load Source Dictionary

**User Story:** As a translator, I want to load a source/untranslated dictionary in the right panel, so that I can see the original text that needs translation.

#### Acceptance Criteria

1. WHEN the user selects a source dictionary file, THE Editor SHALL parse the file (JSON or XML, detected by extension) and display all records in the Right_Panel as read-only content
2. THE Editor SHALL support loading NOTFOUND, CHANGED, and RAW dictionary types as source dictionaries
3. IF a source dictionary file cannot be parsed, THEN THE Editor SHALL display an error message indicating the file path and the nature of the parsing failure

### Requirement 3: Two-Panel Side-by-Side Layout

**User Story:** As a translator, I want to see both panels side by side at all times, so that I can compare source text with my translation without switching views.

#### Acceptance Criteria

1. THE Editor SHALL display the Left_Panel and Right_Panel side by side, both visible simultaneously
2. THE Editor SHALL allow the user to resize the panel split position by dragging a divider between panels
3. WHILE the application window is resized, THE Editor SHALL maintain the proportional split between panels

### Requirement 4: Table-Based Record Display

**User Story:** As a translator, I want each dictionary entry displayed as a row in a table with resizable columns, so that I can browse records efficiently and adjust the layout to my needs.

#### Acceptance Criteria

1. THE Editor SHALL display records in a table with the following columns: ID (Record_Type + Key identifier), Original Text (Key), Translated Text (Value), and Status
2. THE Editor SHALL allow the user to resize each column width by dragging the column header borders
3. THE Editor SHALL support vertical scrolling in both panels independently
4. WHEN a panel contains more records than fit in the visible area, THE Editor SHALL provide a scrollbar for navigation
5. THE Editor SHALL display the total number of records and the current scroll position for each panel
6. THE Editor SHALL persist column width settings between sessions

### Requirement 5: Edit Translation Values

**User Story:** As a translator, I want to edit the value field of records in the left panel, so that I can provide or correct translations.

#### Acceptance Criteria

1. WHEN the user clicks on a Value field in the Left_Panel, THE Editor SHALL activate an inline text editor for that field
2. THE Editor SHALL support multi-line text editing for records that contain line breaks (TEXT, INFO types)
3. WHILE editing a Value field, THE Editor SHALL allow the user to confirm changes by pressing Enter (single-line) or a dedicated confirm action (multi-line)
4. WHILE editing a Value field, THE Editor SHALL allow the user to cancel changes by pressing Escape, restoring the previous value

### Requirement 6: Save Dictionary

**User Story:** As a translator, I want to save my edited dictionary back to a file, so that my translations are preserved for use with yampt's convert command.

#### Acceptance Criteria

1. WHEN the user triggers the save action, THE Editor SHALL write all records from the Left_Panel to the original file in the same format it was loaded from (JSON or XML)
2. THE Editor SHALL preserve the record ordering by Record_Type when saving (CELL, DIAL, INFO, FNAM, TEXT, GMST, DESC, RNAM, INDX, BNAM, SCTX)
3. THE Editor SHALL preserve the original file encoding when saving
4. WHEN the user triggers "Save As", THE Editor SHALL allow choosing a new file path and format (JSON or XML based on the chosen extension) for the output
5. IF a save operation fails, THEN THE Editor SHALL display an error message and retain all unsaved changes in memory

### Requirement 7: Filter, Search, and Replace Records

**User Story:** As a translator, I want to filter records by type, search by text, and replace text across records, so that I can quickly find and batch-edit translations.

#### Acceptance Criteria

1. THE Editor SHALL provide a filter control that allows selecting one or more Record_Types to display
2. WHEN a Record_Type filter is active, THE Editor SHALL display only records matching the selected types in both panels
3. THE Editor SHALL provide a text search field that filters records by matching against Key or Value content
4. WHEN a text search is active, THE Editor SHALL highlight matching text within the visible records
5. THE Editor SHALL provide a Replace field alongside the Search field
6. WHEN the user triggers "Replace", THE Editor SHALL replace the current match in the selected Value field with the replacement text
7. WHEN the user triggers "Replace All", THE Editor SHALL replace all matches in all visible Value fields in the Left_Panel and mark each modified record as changed
8. THE Editor SHALL support case-sensitive and case-insensitive search modes via a toggle
9. THE Editor SHALL provide "Find Next" and "Find Previous" navigation to jump between matches sequentially
10. THE Editor SHALL provide a "Go to Row" function (e.g. via Ctrl+G) that allows the user to jump directly to a specific row number in the table

### Requirement 8: Record Matching Between Panels

**User Story:** As a translator, I want to see the corresponding source record next to my translation, so that I can compare them directly.

#### Acceptance Criteria

1. WHEN both panels have dictionaries loaded, THE Editor SHALL match records between panels by their Record_Type and Key
2. WHEN the user selects a record in one panel, THE Editor SHALL scroll the other panel to show the matching record (if it exists)
3. WHEN a record in the Left_Panel has no matching record in the Right_Panel, THE Editor SHALL display an empty placeholder row in the Right_Panel at the corresponding position

### Requirement 9: Unsaved Changes Indicator

**User Story:** As a translator, I want to know when I have unsaved changes, so that I do not accidentally lose my work.

#### Acceptance Criteria

1. WHILE the Left_Panel contains modifications that have not been saved, THE Editor SHALL display a visual indicator (such as an asterisk in the title bar) showing unsaved changes exist
2. WHEN the user attempts to close the Editor with unsaved changes, THE Editor SHALL prompt the user to save, discard, or cancel the close action

### Requirement 10: Validation Feedback

**User Story:** As a translator, I want to see warnings when my translations exceed length limits, so that I can fix them before applying the dictionary.

#### Acceptance Criteria

1. WHILE a CELL record Value exceeds 63 bytes, THE Editor SHALL display a warning indicator on that record row
2. WHILE a FNAM record Value exceeds 31 bytes, THE Editor SHALL display a warning indicator on that record row
3. WHILE a RNAM record Value exceeds 32 bytes, THE Editor SHALL display a warning indicator on that record row
4. WHILE an INFO record Value exceeds 512 bytes, THE Editor SHALL display a caution indicator on that record row
5. WHILE an INFO record Value exceeds 1024 bytes, THE Editor SHALL display an error indicator on that record row


### Requirement 11: Record Status Tracking

**User Story:** As a translator, I want records marked with specific statuses reflecting their translation state, so that I can track progress and identify records needing attention.

#### Acceptance Criteria

1. THE Editor SHALL support the following statuses for each record:
   - **Untranslated** — no translation provided yet
   - **Translated** — manually translated by the user
   - **Auto_Identical** — automatically translated because another record with identical original text was already translated
   - **Auto_Heuristic** — automatically translated by heuristic matching (e.g. only numbers or punctuation differ between this record and an already-translated one)
   - **Validated** — translation reviewed and approved by the user
   - **Changed_In_Base** — the record's Key was found in a base dictionary, but the original text differs from the base version (the text was modified in the plugin)
   - **Has_Errors** — the translation contains known issues (exceeds length limits, missing required tags, broken formatting)
2. THE Editor SHALL display the current status in the Status column of the table using a distinct pastel background color for each status (soft, muted tones rather than saturated/neon colors)
3. THE Editor SHALL display a status summary bar showing the count of records in each status, each count displayed in a colored cell matching the status pastel color, with the total record count at the end
4. WHEN the user clicks a status cell in the summary bar, THE Editor SHALL filter the table to show only records with that status
5. WHEN the user right-clicks a status cell in the summary bar, THE Editor SHALL hide records with that status from the table (inverse filter)
6. THE Editor SHALL support combining multiple status filters (e.g. show only Untranslated + Changed_In_Base, or hide Validated + Auto_Identical)
7. WHEN the user right-clicks a record or uses a keyboard shortcut, THE Editor SHALL allow changing the record's status manually
8. THE Editor SHALL allow filtering records by one or more statuses (e.g. show only Changed_In_Base records)
5. THE Editor SHALL persist record statuses when saving the dictionary (stored as metadata alongside the record)
6. WHEN a record's Value is modified manually, THE Editor SHALL set its status to Translated
7. WHEN a record has status Changed_In_Base, THE Editor SHALL display a diff view showing the differences between the base dictionary's original text and the current source text, so the translator can see exactly what changed
8. WHEN heuristic auto-translation is applied (Auto_Heuristic), THE Editor SHALL adapt the translation by substituting the differing parts (numbers, punctuation) from the source into the translated text pattern
9. THE Editor SHALL allow the user to promote Auto_Identical and Auto_Heuristic records to Validated status after review

### Requirement 12: Record Change History

**User Story:** As a translator, I want to see the history of changes made to a record, so that I can review previous translations and revert if needed.

#### Acceptance Criteria

1. THE Editor SHALL maintain a per-record history of Value changes made during the current editing session
2. WHEN the user selects a record and opens the history view, THE Editor SHALL display a list of previous values with timestamps
3. THE Editor SHALL allow the user to revert a record's Value to any previous entry in its history
4. THE Editor SHALL persist the change history to a sidecar file (e.g. `.history.json`) alongside the dictionary so that history survives across sessions
5. THE Editor SHALL display a visual indicator on records that have been modified in the current session

### Requirement 13: MWScript Syntax Highlighting

**User Story:** As a translator, I want MWScript keywords and functions highlighted in SCTX records, so that I can distinguish translatable strings from script code.

#### Acceptance Criteria

1. WHEN displaying a record of type SCTX, THE Editor SHALL apply syntax highlighting to the Value text
2. THE Editor SHALL highlight MWScript functions (e.g. MessageBox, Say, Journal, Choice) in a distinct color
3. THE Editor SHALL highlight MWScript comments (lines starting with `;`) in a distinct color
4. THE Editor SHALL highlight string literals (text within double quotes) in a distinct color, as these are the translatable portions
5. THE Editor SHALL NOT apply syntax highlighting to non-SCTX record types

### Requirement 14: Book HTML Tag Highlighting

**User Story:** As a translator, I want Morrowind book formatting tags highlighted in TEXT records, so that I can see the structure without the tags obscuring the translatable content.

#### Acceptance Criteria

1. WHEN displaying a record of type TEXT, THE Editor SHALL highlight HTML-like formatting tags in a distinct color
2. THE Editor SHALL recognize and highlight the following Morrowind book tags: `<DIV>`, `</DIV>`, `<FONT>`, `</FONT>`, `<BR>`, `<P>`, `<IMG>`, `<B>`, and their attributes
3. THE Editor SHALL display tags in a muted/dimmed color so that the actual translatable text content stands out visually
4. WHILE editing a TEXT record, THE Editor SHALL maintain tag highlighting in the inline editor

### Requirement 15: DIAL Topic, Glossary, and Speaker Annotations in INFO Records

**User Story:** As a translator, I want dialogue topic references, glossary terms, and speaker gender highlighted and proposed within INFO records, so that I can use correct translated topic names, consistent terminology, and gender-appropriate grammar.

#### Acceptance Criteria

1. WHEN displaying a record of type INFO, THE Editor SHALL highlight substrings that match known DIAL record Keys from the loaded dictionaries (case-insensitive, word-boundary aware)
2. THE Editor SHALL use a distinct color (e.g. the Morrowind hyperlink blue) for DIAL topic highlights
3. WHEN the user hovers over a highlighted DIAL topic, THE Editor SHALL display a tooltip showing the translated DIAL name (if available in the Left_Panel dictionary)
4. THE Editor SHALL display a proposals/annotations panel (below or beside the editing area) with the following sections when an INFO record is selected:
   - **Hyperlinks**: all DIAL topics found in the record's text, showing `[original -> translated]` for each
   - **Glossary**: all Glossary terms (FNAM-derived names: NPCs, items, places) found in the record's text, showing `[original -> translated]` for each
   - **Speaker**: the gender (M or F) of the NPC associated with the INFO record, derived from the NPC_FLAG dictionary
5. WHEN the user clicks a proposed topic or glossary term in the annotations panel, THE Editor SHALL insert the translated term at the cursor position in the Value field (or copy it to clipboard)
6. THE Editor SHALL update annotations dynamically when DIAL, Glossary, or NPC_FLAG records are modified in the loaded dictionaries
7. THE Editor SHALL match topics and glossary terms using the same logic as yampt's `addAnnotations`: case-insensitive substring search with word-boundary check (preceding character must not be alphabetic)
8. THE Editor SHALL support loading a Glossary dictionary (containing FNAM-derived name translations) as an additional reference source for annotations
9. THE Editor SHALL support loading an NPC_FLAG dictionary (containing NPC gender flags) as an additional reference source for speaker annotations

### Requirement 18: Enchanted Item Indicator for FNAM Records

**User Story:** As a translator, I want to see whether an item whose name I'm translating is enchanted, so that I can apply the appropriate naming convention (e.g. adding a prefix or suffix indicating magic).

#### Acceptance Criteria

1. WHEN displaying a record of type FNAM that belongs to a weapon, armor, or clothing item, THE Editor SHALL display an indicator if the item has an enchantment
2. THE Editor SHALL show the enchantment name (ENAM reference) in the annotations panel when an enchanted item's FNAM record is selected
3. THE Editor SHALL derive enchantment data from the loaded ESM/source files or from a supplementary enchantment dictionary provided by the user
4. THE Editor SHALL use a distinct icon or label (e.g. "⚡ Enchanted") in the record row to mark enchanted items at a glance

### Requirement 16: Spell Checking

**User Story:** As a translator, I want spell checking on my translated text, so that I can catch typos before applying the dictionary to the game.

#### Acceptance Criteria

1. THE Editor SHALL provide spell checking for Value fields in the Left_Panel
2. THE Editor SHALL underline misspelled words with a red wavy underline
3. WHEN the user right-clicks a misspelled word, THE Editor SHALL display a context menu with spelling suggestions
4. THE Editor SHALL allow the user to add words to a custom dictionary (for game-specific terms like place names, creature names, etc.)
5. THE Editor SHALL support configuring the spell check language via a settings option
6. THE Editor SHALL exclude content within HTML tags (in TEXT records) and MWScript code (in SCTX records) from spell checking

### Requirement 17: Multiple Base Dictionaries with Ordered Merge

**User Story:** As a translator, I want to load multiple base dictionaries and have them merged in priority order, so that I can combine translations from different sources (e.g. Morrowind base, Tribunal, Bloodmoon) and automatically apply known translations to unchanged records.

#### Acceptance Criteria

1. THE Editor SHALL allow the user to load one or more base dictionary files as reference sources, in addition to the User_Dictionary and Source_Dictionary
2. THE Editor SHALL merge base dictionaries in the order they are listed: the first dictionary has highest priority, later dictionaries only contribute records not already present in earlier ones
3. THE Editor SHALL display the base dictionary list in a configuration panel where the user can add, remove, and reorder entries
4. WHEN a record in the Source_Dictionary (Right_Panel) has a matching Key in the merged base dictionaries, THE Editor SHALL display the base translation as a proposal in the annotations panel
5. WHEN the user triggers "Auto-translate from base", THE Editor SHALL copy translations from the merged base dictionaries into the User_Dictionary for all records whose Key text matches exactly between the source and the base dictionary
6. WHEN the user triggers "Auto-translate identical", THE Editor SHALL find all records in the Source_Dictionary that share identical text with another record already translated in the User_Dictionary, and copy that translation to the untranslated record
7. THE Editor SHALL visually distinguish auto-translated records (e.g. with a specific Status or color) so the user can review them
8. THE Editor SHALL report a summary after auto-translation: number of records translated, number skipped (no match), number skipped (text changed)
9. THE Editor SHALL persist the base dictionary file list between sessions so the user does not need to reconfigure it each time
10. THE Editor SHALL support both JSON and XML format base dictionaries, consistent with the existing dictionary loading logic
11. WHEN a record has no exact match in the base dictionaries or identical records, THE Editor SHALL search for similar records (fuzzy matching) and display them as proposals in the annotations panel
12. THE Editor SHALL consider a record "similar" when its text differs by a small edit distance (e.g. a few changed words, added/removed punctuation, or minor rephrasing) relative to the total text length
13. THE Editor SHALL display fuzzy match proposals with a similarity percentage and highlight the differences between the source text and the matched text
14. THE Editor SHALL NOT auto-apply fuzzy matches — they are proposals only, requiring explicit user action to accept
