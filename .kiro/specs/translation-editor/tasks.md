# Tasks

## Task 1: Project Scaffolding and Build Setup

Create the `yampt.gui` project within the existing Visual Studio solution with Dear ImGui + SDL2 + OpenGL3 backend, producing a minimal window that compiles and links.

- [ ] Create `yampt.gui/` directory with `yampt.gui.vcxproj` and `.filters` file
- [ ] Add the project to `yampt.sln`
- [ ] Add Dear ImGui source files (core + SDL2 + OpenGL3 backends) to the project
- [ ] Add SDL2 dependency via vcpkg (update `vcpkg.json`)
- [ ] Create `yampt.gui/main.cpp` with SDL2 window creation, ImGui context init, and a render loop that shows an empty window titled "yampt.gui"
- [ ] Configure project to reference yampt's existing source files (`tools.cpp`, `dictreader.cpp`, `dictwriter.cpp`, `dictmerger.cpp`) as shared items or via a static lib
- [ ] Verify the project builds with the v143 toolset (x64 Debug and Release)

## Task 2: Editor Application Shell and Config Persistence

Implement `editor_app_t` with the frame loop structure, menu bar, and `editor_config_t` for persisting settings to `yampt_gui.ini`.

- [ ] Create `yampt.gui/editor_app.hpp` and `yampt.gui/editor_app.cpp` with `init()`, `frame()`, `shutdown()`, `wants_quit()`
- [ ] Implement `render_menu_bar()` with File menu (Open User Dict, Open Source Dict, Save, Save As, Exit) and View menu (placeholder)
- [ ] Create `yampt.gui/editor_config.hpp` and `yampt.gui/editor_config.cpp` with `load()`/`save()` using ImGui INI-style format
- [ ] Persist and restore `split_ratio`, `column_widths`, `last_user_dict_path`, `last_source_dict_path`
- [ ] Wire the quit confirmation (unsaved changes prompt) into `wants_quit()`

## Task 3: Editor State and Dictionary Loading

Implement `editor_state_t` that wraps dictionary loading/saving using the existing `dict_reader_t` and `dict_writer_t` classes.

- [ ] Create `yampt.gui/editor_state.hpp` and `yampt.gui/editor_state.cpp`
- [ ] Implement `load_user_dict(path)` — detect format by extension, parse via `dict_reader_t`, populate internal `dict_t`
- [ ] Implement `load_source_dict(path)` — same parsing, stored as read-only
- [ ] Implement `save_user_dict()` and `save_user_dict_as(path)` — write via `dict_writer_t`, preserve encoding and format
- [ ] Implement `has_unsaved_changes()` and `mark_modified()` tracking
- [ ] Wire File menu actions to `editor_state_t` methods with native file dialogs (via `tinyfiledialogs` or Win32 `GetOpenFileName`)

## Task 4: Two-Panel Layout with Resizable Splitter

Implement the side-by-side panel layout with a draggable vertical splitter.

- [ ] In `editor_app_t::render_panels()`, create two ImGui child windows side by side using `split_ratio`
- [ ] Implement splitter drag logic (invisible button between panels that adjusts `split_ratio` on drag)
- [ ] Ensure both panels maintain proportional sizing on window resize
- [ ] Store `split_ratio` in `editor_config_t` for persistence

## Task 5: Table-Based Record Display (Left Panel)

Render the user dictionary as a scrollable table with resizable columns in the left panel.

- [ ] Use `ImGui::BeginTable` with columns: ID, Original Text, Translated Text, Status
- [ ] Implement `ImGuiTableFlags_Resizable` for column width dragging
- [ ] Use `ImGuiListClipper` for virtualized scrolling (only render visible rows)
- [ ] Display record type as a prefix in the ID column (e.g. `CELL: Balmora`)
- [ ] Display total record count and current scroll position in the panel header
- [ ] Persist column widths in `editor_config_t`

## Task 6: Table-Based Record Display (Right Panel)

Render the source dictionary as a read-only scrollable table in the right panel, mirroring the left panel layout.

- [ ] Reuse the same table rendering logic as Task 5 but with all fields read-only
- [ ] Apply a slightly different background tint to distinguish from the editable panel
- [ ] Implement independent vertical scrolling from the left panel

## Task 7: Record Matching and Synchronized Scrolling

Match records between panels by type+key and synchronize selection.

- [ ] Build a lookup map from `(rec_type_t, key)` → row index for both panels
- [ ] When the user selects a row in one panel, scroll the other panel to the matching record
- [ ] Display empty placeholder rows in the right panel for records that exist only in the left panel
- [ ] Highlight the matched record pair with a subtle selection color

## Task 8: Inline Editing of Translation Values

Enable editing Value fields in the left panel table.

- [ ] On double-click of a Value cell, activate `ImGui::InputText` (single-line) or `ImGui::InputTextMultiline` (for TEXT/INFO types)
- [ ] Confirm edit on Enter (single-line) or Ctrl+Enter (multi-line); cancel on Escape
- [ ] On confirm, update the record in `editor_state_t`, call `mark_modified()`, and record in history
- [ ] Auto-set status to `"translated"` when a value is manually edited

## Task 9: Record Type Filtering

Implement the record type filter control in the toolbar.

- [ ] Add a row of checkboxes/toggles in the toolbar, one per `rec_type_t` (CELL, DIAL, INFO, FNAM, TEXT, GMST, DESC, RNAM, INDX, BNAM, SCTX)
- [ ] Implement `get_filtered_records()` in `editor_state_t` that respects the active type filter
- [ ] Apply the filter to both panels simultaneously
- [ ] Show record count per type next to each toggle

## Task 10: Text Search and Highlight

Implement text search with match highlighting.

- [ ] Create `yampt.gui/search_manager.hpp` and `yampt.gui/search_manager.cpp`
- [ ] Add a search input field in the toolbar
- [ ] Implement `find_all()` that scans filtered records for matches in key or value text
- [ ] Highlight matching substrings in the table cells using colored text spans
- [ ] Implement `next_match()` / `prev_match()` with F3 / Shift+F3 keyboard shortcuts
- [ ] Implement case-sensitive toggle
- [ ] Implement "Go to Row" (Ctrl+G) popup that jumps to a specific row number

## Task 11: Find and Replace

Extend search with replace and replace-all functionality.

- [ ] Add a Replace input field below the Search field in the toolbar
- [ ] Implement `replace_current()` — replace the currently highlighted match in the value field
- [ ] Implement `replace_all()` — replace all matches in all visible left-panel value fields, mark each as modified
- [ ] Display replacement count in the status bar after replace-all
- [ ] Add keyboard shortcuts (Ctrl+H to open replace, Enter to replace next)

## Task 12: Validation Manager

Implement byte-length validation with visual indicators on record rows.

- [ ] Create `yampt.gui/validation_manager.hpp` and `yampt.gui/validation_manager.cpp`
- [ ] Implement `validate()` with the rules: CELL > 63 = error, FNAM > 31 = error, RNAM > 32 = error, INFO > 512 = caution, INFO > 1024 = error
- [ ] Display colored indicators in the table row (yellow for caution, red for error)
- [ ] Show byte count tooltip on hover over the indicator
- [ ] Auto-set status to `"has_errors"` when validation fails

## Task 13: Record Status Display and Filtering

Implement status column rendering with pastel colors and the status summary bar.

- [ ] Define pastel color palette for each status (untranslated, translated, auto_identical, auto_heuristic, validated, changed, has_errors)
- [ ] Render the Status column cell with the appropriate background color
- [ ] Implement the status summary bar below the toolbar showing counts per status in colored cells
- [ ] Left-click on a summary cell → filter to show only that status
- [ ] Right-click on a summary cell → hide that status (inverse filter)
- [ ] Support combining multiple status filters
- [ ] Right-click context menu on records to manually change status
- [ ] Allow promoting auto_identical / auto_heuristic to validated

## Task 14: Unsaved Changes Indicator and Close Prompt

Implement the dirty-state indicator and save-on-close dialog.

- [ ] Append `" *"` to the window title when `has_unsaved_changes()` is true
- [ ] On close attempt (SDL_QUIT or File > Exit), show a modal dialog: Save / Discard / Cancel
- [ ] Wire Save to `save_user_dict()`, Discard to quit without saving, Cancel to abort close

## Task 15: History Manager

Implement per-record change history with persistence.

- [ ] Create `yampt.gui/history_manager.hpp` and `yampt.gui/history_manager.cpp`
- [ ] Record each value change with ISO 8601 timestamp
- [ ] Implement `get_history()` returning the list of previous values for a record
- [ ] Implement `revert()` to restore a previous value
- [ ] Implement `load_from_file()` / `save_to_file()` for the `.history.json` sidecar
- [ ] Display a "modified this session" indicator (dot or icon) on changed records

## Task 16: History View Panel

Render the history panel UI.

- [ ] Add a toggleable history panel (View menu or keyboard shortcut)
- [ ] When a record is selected, display its change history as a scrollable list
- [ ] Each entry shows the value text (truncated) and timestamp
- [ ] Double-click or "Revert" button restores that value and records the revert as a new history entry

## Task 17: Syntax Highlighter — MWScript (SCTX)

Implement syntax highlighting for SCTX records.

- [ ] Create `yampt.gui/syntax_highlighter.hpp` and `yampt.gui/syntax_highlighter.cpp`
- [ ] Implement `tokenize()` for SCTX: identify MWScript functions (MessageBox, Say, Journal, Choice, AddTopic, GetPCCell, etc.), comments (`;` lines), and string literals (`"..."`)
- [ ] Use `string::find`-based tokenization (no regex per design rules)
- [ ] Render tokenized text with colored spans in the table cell and inline editor

## Task 18: Syntax Highlighter — Book HTML Tags (TEXT)

Implement tag highlighting for TEXT records.

- [ ] Extend `syntax_highlighter_t::tokenize()` for TEXT type: identify `<DIV>`, `</DIV>`, `<FONT>`, `</FONT>`, `<BR>`, `<P>`, `<IMG>`, `<B>` and attributes
- [ ] Render tags in a muted/dimmed color so translatable content stands out
- [ ] Maintain highlighting in the inline multi-line editor during editing

## Task 19: Annotation Manager — DIAL Topics

Implement DIAL topic detection and annotation in INFO records.

- [ ] Create `yampt.gui/annotation_manager.hpp` and `yampt.gui/annotation_manager.cpp`
- [ ] On dictionary load, build a sorted list of DIAL keys (longest first for greedy matching)
- [ ] Implement `annotate()` for INFO records: case-insensitive substring search with word-boundary check (preceding char not alphabetic)
- [ ] Highlight matched topics in Morrowind hyperlink blue within the table cell
- [ ] Show tooltip with translated DIAL name on hover

## Task 20: Annotation Manager — Glossary and Speaker

Extend annotations with glossary terms and NPC gender flags.

- [ ] Support loading a Glossary dictionary (FNAM-derived names) as additional reference
- [ ] Support loading an NPC_FLAG dictionary for speaker gender
- [ ] Add glossary term matching (same logic as DIAL topics)
- [ ] Display annotations panel with sections: Hyperlinks, Glossary, Speaker
- [ ] Click on a proposal → insert translated term at cursor or copy to clipboard
- [ ] Update annotations dynamically when dictionaries are modified

## Task 21: Enchanted Item Indicator for FNAM Records

Show enchantment status for weapon/armor/clothing FNAM records.

- [ ] Support loading an enchantment reference dictionary (or derive from ESM data)
- [ ] When an FNAM record belongs to an enchanted item, display "⚡" indicator in the row
- [ ] Show enchantment name in the annotations panel when the record is selected

## Task 22: Base Dictionary Manager

Implement multi-dictionary merge and auto-translation.

- [ ] Create `yampt.gui/base_dict_manager.hpp` and `yampt.gui/base_dict_manager.cpp`
- [ ] Implement `set_paths()` / `reload()` using `dict_merger_t` with first-wins semantics
- [ ] Add a configuration panel (View menu) for adding, removing, and reordering base dict paths
- [ ] Persist base dict paths in `editor_config_t`
- [ ] Display base translation proposals in the annotations panel for matching records

## Task 23: Auto-Translate from Base

Implement the "Auto-translate from base" action.

- [ ] Implement `auto_translate_from_base()`: for each untranslated record, if the key matches exactly in the merged base dict, copy the translation
- [ ] Set status to `"auto_identical"` for auto-translated records
- [ ] Display a summary dialog: translated count, skipped (no match), skipped (text changed)

## Task 24: Auto-Translate Identical Records

Implement the "Auto-translate identical" action.

- [ ] Implement `auto_translate_identical()`: find untranslated records whose text is identical to an already-translated record, copy that translation
- [ ] Set status to `"auto_identical"` for these records
- [ ] Display summary dialog with counts

## Task 25: Heuristic Auto-Translation

Implement heuristic matching for similar records.

- [ ] Detect records that differ only in numbers or punctuation from an already-translated record
- [ ] Adapt the translation by substituting the differing parts from the source
- [ ] Set status to `"auto_heuristic"` for these records
- [ ] Allow user review and promotion to validated

## Task 26: Fuzzy Matching Proposals

Implement fuzzy match display in the annotations panel.

- [ ] Implement `find_fuzzy_matches()` using edit-distance comparison relative to text length
- [ ] Display up to 5 fuzzy matches with similarity percentage
- [ ] Highlight differences between source text and matched text
- [ ] Proposals are display-only — require explicit user action to accept

## Task 27: Changed-in-Base Detection and Diff View

Detect records whose source text differs from the base dictionary and show a diff.

- [ ] During base dict load, compare each source record's key text against the base version
- [ ] Set status to `"changed"` for records where the text differs
- [ ] When a changed record is selected, display a simple inline diff (added/removed text highlighted) in the annotations panel

## Task 28: Spell Checker Integration

Integrate Hunspell for spell checking translated text.

- [ ] Create `yampt.gui/spell_checker.hpp` and `yampt.gui/spell_checker.cpp`
- [ ] Add Hunspell dependency via vcpkg
- [ ] Implement `init()`, `is_correct()`, `suggest()`, `add_to_custom_dict()`
- [ ] Render red wavy underline on misspelled words in value cells
- [ ] Right-click context menu with suggestions and "Add to dictionary" option
- [ ] Exclude HTML tags (TEXT records) and MWScript code (SCTX records) from checking
- [ ] Add spell check language configuration to `editor_config_t`

## Task 29: Status Persistence in Dictionary Files

Ensure record statuses are saved and loaded with the dictionary.

- [ ] When saving JSON format, include the `"status"` field in each record entry
- [ ] When saving XML format, store status as an attribute or additional tag per record
- [ ] On load, restore statuses from the file; default to `"untranslated"` if missing
