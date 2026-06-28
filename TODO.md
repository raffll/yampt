# TODO

Sorted by effort. Settings dialog takes priority.

---

## S — a few hours

### Settings dialog for both apps [S, PRIORITY]
Settings dialog similar to Notepad++/Visual Studio. Covers both yTranslator and yEditor. Central place for keyboard shortcuts, paths, encoding, spell check language, translation provider API keys, theme preferences.

Key setting: **Native language / Foreign language** pair (e.g. Native=PL, Foreign=EN). Selecting these auto-configures:
- Codepage/encoding (e.g. PL → Windows-1250, RU → Windows-1251)
- Spell check dictionary (e.g. PL → pl_PL, DE → de_DE)
- AI translation model target language (e.g. PL → eng_Latn→pol_Latn)
- Make-base default language tags
- Partial mode Hunspell dictionary (Foreign language → en_US, de_DE, etc.)

### Enable DeepL and Google translation providers [S]
`deepl_provider_t` and `google_provider_t` are fully implemented and compiled. The `source_combo_` dropdown is created but `setVisible(false)`. Unhide the combo, add all provider names to it, connect their `translation_finished` signals, and add an API key settings field for DeepL.

### Enable glossary function in translation tab [S]
`set_glossary_fn()` is declared and the member stored but never called from `main_window_t`. Wire it to `annotation_manager_` so translations get glossary term replacements suggested alongside model output.

### Remove dead `search_manager_t` [S]
Complete class in `controller/search_manager.hpp/.cpp`, compiled but never included or used anywhere. Superseded by `search_engine_t`. Remove from vcxproj and delete files.

### Rename misleading classes [S]
Several class names use vague words (manager, provider, service, engine) or are outright misleading:

| Current | Proposed | Rationale |
|---|---|---|
| `editor_tab_t` | `plugin_workspace_t` | Full editor workspace, not a tab |
| `search_engine_t` | `row_filter_t` | Stateless row-match predicate |
| `validation_manager_t` | `byte_limit_validator_t` | Validates byte length only |
| `search_manager_t` | (delete — see above) | Dead code |
| `history_manager_t` | `edit_history_t` | It IS the history |
| `annotation_manager_t` | `glossary_t` | Builds glossary and annotates text |
| `find_replace_service_t` | `find_replace_t` | Drop "service" |
| `translation_provider_t` | `translator_t` | Interface that translates text |
| `ctranslate2_provider_t` | `ctranslate2_translator_t` | Concrete translator |
| `deepl_provider_t` | `deepl_translator_t` | Concrete translator |
| `google_provider_t` | `google_translator_t` | Concrete translator |

Also rename `provider/` folder to `translator/`.

Move `file_list_t` from `yampt/io/` to `yampt.translator/model/`. It's a session-level file registry (workspace scan, classification, language detection) — not a file format reader. Only consumed by `yampt.translator` (sidebar_model, display_name, main_window).

### Extract duplicated static helpers to `tools.hpp` [S]
Multiple files reimplement the same utilities as local statics or lambdas:

| Function | Copies | Where |
|---|---|---|
| `to_lower(string)` | 4 | file_list.cpp, annotation_manager.cpp, search_engine.cpp, search_manager.cpp |
| `normalize_path` (backslash→forward slash) | 3 named + ~12 inline `std::replace` | session.cpp, sidebar_model.cpp, dict_selection_dialog.cpp, main_window.cpp, yaml_document.cpp, dict_document.cpp |
| `extract_filename` | 2 | file_list.cpp, sidebar_model.cpp |

Add to `tools_t` (or a new `string_utils.hpp` in `yampt/utility/`):
- `to_lower(std::string_view) -> std::string`
- `normalize_path(std::string_view) -> std::string` (replace `\\` with `/`)
- `extract_filename(std::string_view) -> std::string_view`

Then replace all local copies and inline lambdas with the shared version.

### Full refactoring audit [M]

Comprehensive list of naming, placement, and duplication issues. Items split into three sections: clear fixes, ambiguous (needs decision), and duplication.

#### A — Clear Fixes (obvious renames or moves)

| # | Current | Proposed | Why |
|---|---------|----------|-----|
| 1 | `editor_tab_t` (yampt.editor/view/) | `plugin_workspace_t` | It's the entire editor workspace (trees + toolbar + messages + scan), not a tab |
| 2 | `search_engine_t` (translator/controller/) | `row_filter_t` | Stateless predicate that tests one row |
| 3 | `validation_manager_t` (translator/controller/) | `byte_limit_validator_t` | Validates byte length vs record limit |
| 4 | `history_manager_t` (translator/controller/) | `edit_history_t` | It IS the edit history |
| 5 | `annotation_manager_t` (translator/controller/) | `glossary_t` | Builds glossary, annotates text |
| 6 | `find_replace_service_t` (translator/controller/) | `find_replace_t` | Drop "service" |
| 7 | `translation_provider_t` (translator/provider/) | `translator_t` | Strategy interface — it translates |
| 8 | `ctranslate2_provider_t` | `ctranslate2_translator_t` | Concrete translator |
| 9 | `deepl_provider_t` | `deepl_translator_t` | Concrete translator |
| 10 | `google_provider_t` | `google_translator_t` | Concrete translator |
| 11 | `provider/` folder | `translator/` | Contents are translation backends |
| 12 | `file_list_t` (yampt/io/) | move to `yampt.translator/model/` | Session-level registry, not core I/O |
| 13 | `menu_action_t` (inside file_list.hpp) | extract to own file in `yampt.translator/model/` | GUI enum doesn't belong in core library |
| 14 | `editor_config_t` (translator/controller/) | move to `yampt.translator/io/` | Pure INI file serialization, no orchestration |
| 15 | `status_colors.hpp` (translator/utility/) | move to `translator/view/` | Returns QColor — it's a view helper |
| 16 | `grammar_checker_t` (translator/utility/) | move to `translator/controller/` | Validation logic returning ExtraSelections |
| 17 | `plugin_op.hpp` (translator/utility/) | move to `translator/model/` | Domain enum for operation types |
| 18 | `status_display_name` (duplicated) | extract to `translator/utility/status_display.hpp` | Same if/else chain in record_table_model.cpp AND status_filter_bar.cpp |
| 19 | `tools_t::name_t` | `file_path_parts_t` | "name" is maximally ambiguous in a project with NPC names, topic names, plugin names |
| 20 | `tools_t::status_t` (string constants) | proper `enum class status_t` | Currently `const char *` fields compared via `==` against raw strings. Replace with a real enum class that cannot be compared to `std::string` directly. Serialize to/from string only at JSON read/write boundaries. Eliminates typo risk and scattered string comparisons across the codebase. |
| 21 | All view classes with `_panel`, `_tab`, `_bar`, `_widget` suffixes | uniform `_view_t` suffix | Layout position can change — "panel", "tab", "bar" describe where it sits now, not what it is. Use `_view_t` everywhere. Exceptions: `editor_text_edit_t` (describes widget type), `line_number_gutter_t` (describes what it IS). Full list: `annotations_panel_t`→`annotations_view_t`, `history_panel_t`→`history_view_t`, `editor_panel_t`→`editor_view_t`, `log_tab_t`→`log_view_t`, `translation_suggestion_tab_t`→`translation_suggestion_view_t`, `status_filter_bar_t`→`status_filter_view_t`, `sidebar_widget_t`→`sidebar_view_t`, `book_preview_t`→`book_preview_view_t`, `validation_indicator_t`→`validation_view_t`, `filter_tree_t`→`filter_tree_view_t`, `table_display_t`→`table_view_t`, `messages_panel_t`→`messages_view_t`, `editor_tab_t`→`plugin_workspace_view_t`, `editor_text_edit_t`→`translation_edit_view_t` |

#### B — Ambiguous (your decision needed)

| # | Item | Options | Notes |
|---|------|---------|-------|
| 1 | `tools_t` god-class | (a) decompose into `record_types.hpp`, `dict_types.hpp`, `logger_t`, `byte_utils.hpp`, `string_utils.hpp` (b) keep as-is with just extracting reusable statics | 200+ lines of unrelated concerns. Decomposition is clean but high-touch (everything depends on it). Could do incrementally. |
| 2 | `row_provider_t` (translator/model/) | (a) `row_source_t` (b) `indexed_rows_t` (c) keep — "provider" is acceptable for abstract interfaces | It's a 3-method interface. The name isn't great but it's small enough to be ignorable. |
| 3 | `display_name_t` (translator/utility/) | (a) `sidebar_label_t` (b) `item_label_builder_t` (c) keep — "display name" is clear enough in context | Builds sidebar labels with [BASE] [WIP] tags. Only used by sidebar code. |
| 4 | `session_t` (translator root) | (a) `document_store_t` (b) `open_documents_t` (c) keep — "session" is a known pattern | Manages open documents (open/close/find/save_all). Could argue either way. |
| 5 | `table_col_t` enum in `table_row.hpp` | (a) move to own `table_columns.hpp` in model/ (b) move to view/ since it's a view concern (c) keep — the model needs to know column indices for data access | It's used by both model and view code. |
| 6 | `sidebar_model.hpp` — file contains render DTOs + free functions | (a) split into `sidebar_render_types.hpp` (DTOs) + `sidebar_builder.hpp` (functions) (b) keep as-is — it's a cohesive unit for sidebar data | The free functions build the render model from file_list + session. |
| 7 | `editor_controller_t` (translator/controller/) | (a) keep (b) `editor_mediator_t` — it mediates between panel, document, and sub-objects | "Controller" is accurate for MVC but generic. It specifically mediates load/commit between panel and document. |
| 8 | `operation_executor_t` (translator/controller/) | (a) keep (b) `batch_runner_t` — it runs CLI-style batch operations | "Executor" is clear but pattern-ish. It wraps dict_creator + esm_converter for GUI use. |

#### C — Duplicated Functions (extract to shared utility)

| Function | Files | Proposed location |
|----------|-------|-------------------|
| `to_lower(string)` | file_list.cpp, annotation_manager.cpp, search_engine.cpp, search_manager.cpp, tools.cpp (as `case_insensitive_string_cmp`), script_parser.cpp, dict_creator.cpp | `string_utils.hpp` in `yampt/utility/` |
| `normalize_path()` (backslash → `/`) | session.cpp, sidebar_model.cpp, dict_selection_dialog.cpp, main_window.cpp (×8), yaml_document.cpp, dict_document.cpp, tests | `string_utils.hpp` in `yampt/utility/` |
| `extract_filename()` | file_list.cpp, sidebar_model.cpp, main_window.cpp (lambda), annotations_panel.cpp, dict_selection_dialog.cpp | `string_utils.hpp` in `yampt/utility/` |
| `status_display_name()` | record_table_model.cpp, status_filter_bar.cpp | `translator/utility/status_display.hpp` |
| `trim()` | editor_config.cpp (full trim), tools.cpp (`trim_cr` — related) | `string_utils.hpp` in `yampt/utility/` |
| `starts_with()` | editor_config.cpp | Use C++20 `std::string::starts_with()` directly |
| `parse_int_safe()` / `parse_float_safe()` | editor_config.cpp | `string_utils.hpp` if needed elsewhere, or leave (only 1 file) |
| Record/sub-record type tables | tools.cpp (`is_fnam`), table_builder.cpp, filter_tree.cpp | `record_types.hpp` in `yampt/utility/` or `yampt/model/` — see dedicated task below |

### Consolidate record/sub-record type tables [M]
The same lists of TES3 record types and sub-type categories are scattered across 4 files with different representations:

| File | What it defines |
|------|-----------------|
| `tools.cpp` → `is_fnam()` | 23 record IDs as string `==` chain |
| `table_builder.cpp` | `prefix_to_sub_type` (30 entries) + inverse `sub_type_to_prefix` |
| `filter_tree.cpp` | `type_order` + `info_sub_types` + `fnam_sub_types` + `desc_sub_types` + `indx_sub_types` |
| `sub_record_schema.cpp` | Field-level schema per sub-record type (separate concern, keep) |

The FNAM-eligible record list (ACTI, ALCH, APPA, ARMO, BOOK, BSGN, CLAS, CLOT, CONT, CREA, DOOR, FACT, INGR, LIGH, LOCK, MISC, NPC_, PROB, RACE, REGN, REPA, SPEL, WEAP) is duplicated verbatim in `is_fnam()`, `table_builder.cpp`, and `filter_tree.cpp`.

Extract to a single `record_types.hpp`:
- One `constexpr` array of FNAM-eligible record IDs (replace `is_fnam()` with a lookup)
- One struct per sub-type category holding the prefix/key, display name, and parent `rec_type_t`
- Bidirectional prefix↔sub_type lookups derived from the same source array
- `filter_tree.cpp` and `table_builder.cpp` both consume the shared definitions
- Adding a new record type means editing one file, not three

### Remove `model_downloader_t` [S]
Model will be shipped separately. Delete `provider/model_downloader.hpp/.cpp`, remove from vcxproj.

### Expose yEditor toolbar buttons [S]
7 buttons (`btn_load_`, `btn_new_`, `btn_save_`, `btn_merge_`, `btn_filter_`, `cmb_type_filter_`, `edt_search_`) are fully functional but all `setVisible(false)`. Also the Save menu action is hidden. Unhide when ready.

### Keyboard shortcuts [S]
- F8 — copy original to translated
- F9 — set status to In Progress
- F10 — set status to Translated
- Del — set status to Untranslated (clear translated column)

No shortcut handling code exists yet (`QKeySequence`/`setShortcut` not used anywhere in translator). Shortcuts panel lives inside the settings dialog.

### History should restore status [S]
Undo/revert in yTranslator should also restore the entry's status (not just the text). Handle propagated, model, and other auto-set statuses correctly when reverting.

### Script parser: skip bytecode modification when no translation found [S]
When `new_text == old_text` (cell/topic not in dictionary), the script parser still runs the full erase/insert cycle on SCTX and SCDT (which is a no-op but risks size byte validation false positives). After the size byte validation loop confirms the correct bytecode occurrence, check `if (new_text == old_text)` and advance `pos_c += old_text.size()` then return. The `insert_new_text` function should also early-return when `new_text == old_text`.

### Better explanation of partial mode [S]
Clarify in the GUI (tooltip, help text, or info panel) how the English dictionary comparison works in partial mode.

### Hyperlink spec [S]
Document how hyperlinks work in Morrowind dialogues, how yampt detects and applies them during conversion, and how the annotation system highlights them.

---

## M — a day or two

### Enable model downloader in GUI [M] — CANCELLED
~~Model will be shipped separately. Remove `model_downloader_t` instead (see S tasks).~~

### Better difference highlighting in adapted_from panel [M]
Improve the diff highlighting between the current text and the adapted_from source to make changes more visible.

### Merge as GUI option [M]
Add explicit merge functionality in the GUI. Currently users can load multiple dicts and the merge happens implicitly during convert. Add a dedicated merge button/dialog in yTranslator.

### Configurable source dictionary for partial mode [M]
Allow changing the Hunspell dictionary used in partial mode from English to another language (e.g. German, French). Currently hardcoded to `en_US.aff`/`en_US.dic`. Add a dropdown or file selector in the make-base dialog.

### Load TES3 top-level files and generate annotations [M]
Parse loaded plugin's FNAM, CELL, DIAL names and use them as annotation/glossary sources for the current dict. `plugin_document_t` exists as a stub placeholder — all methods are empty/return defaults.

### Hyperlinks more accurate [M]
Read OpenMW source code for exact hyperlink detection rules and improve matching accuracy in yampt's annotation/conversion system.

### EET file format converter to JSON [M]
Import `.eet` files (ESP-ESM Translator binary format) and convert to yampt's JSON dictionary format. The EET format is already reverse-engineered and documented in steering.

### Script parser: unquoted multi-word cell names [M]
Commands like `ShowMap Ald Velothi` use unquoted multi-word cell names. The token extractor only grabs the first word. Need special handling for commands where the cell name is the last parameter — take everything to end-of-line. Low frequency in practice (most mods use quotes).

### Version / Release [M]
- First public release was build735
- README show 735 state
- Update readme
- Fix build folder structure
- Update documents
- Include documents

---

## L — a week or more

### Merged Patch (yEditor) [L]
The merge infrastructure exists in yEditor (hidden `btn_merge_`, `on_create_merged_patch` handler, leveled list + dialogue merging, drag-and-drop support) but the workflow isn't exposed yet. Remaining:
- Default folder for folder/MO2/OpenMW
- Settings dialog, configurable paths
- Show button/dialog
- Make sure good conflicts are merged
- Show/hide merged patch column
- Allow drag and drop to merged patch

### Expand all remaining hex sub-records in yEditor [L]
Many sub-records still display as raw hex dumps. Add schemas for all common sub-records using OpenMW source as reference. Priority: DIAL DATA, DOOR DATA, LOCK DATA, AI_T, AI_F, AI_E, AI_A, BSGN, NAM0, WHGT, NAM5, XSCL, INTV, FLTV, INDX.

### Dark theme [L]
- Dark theme
- Unhardcode colors (status_colors.hpp, grammar_checker warning_format, etc.)
- Match better colors for dark theme

### Polish UI localization [L]
Move hardcoded UI strings to YAML l10n files so the interface can be displayed in Polish.

### Scan Lua files for translatable strings [L]
Parse OpenMW Lua scripts (l10n YAML, or string literals) to extract translatable text for dictionary creation.

---

## XL — multi-week

### Linux build support [XL]
No Windows-only dependencies identified. Needs CMakeLists.txt for Linux.
