# Project Paths & Build

## Repository Layout

```
yampt/
├── yampt/                  # Core library + CLI (C++)
│   ├── main.cpp
│   ├── io/                # File I/O layer
│   │   ├── esm_reader.hpp / esm_reader.cpp
│   │   ├── dict_reader.hpp / dict_reader.cpp
│   │   ├── dict_writer.hpp / dict_writer.cpp
│   │   ├── json_reader.hpp
│   │   ├── file_list.hpp / file_list.cpp
│   │   └── codepage.hpp / codepage.cpp
│   ├── model/             # Domain logic
│   │   ├── dict_merger.hpp / dict_merger.cpp
│   │   ├── dict_creator.hpp / dict_creator.cpp
│   │   ├── dict_creator_single.cpp
│   │   ├── dict_creator_base.cpp
│   │   ├── dict_creator_base_ordered.cpp
│   │   ├── dict_kind.hpp
│   │   ├── esm_converter.hpp / esm_converter.cpp
│   │   ├── script_parser.hpp / script_parser.cpp
│   │   └── translation_engine.hpp / translation_engine.cpp
│   ├── utility/           # Pure helpers
│   │   ├── tools.hpp / tools.cpp
│   │   └── includes.hpp
│   ├── interface/         # CLI boundary
│   │   └── user_interface.hpp / user_interface.cpp
│   ├── plugin_scan/       # Plugin comparison/conflict detection
│   └── yampt.vcxproj
├── yampt.translator/       # GUI translation workbench (Qt6)
│   ├── main.cpp            # → yTranslator.exe
│   ├── main_window.hpp / main_window.cpp
│   ├── session.hpp / session.cpp
│   ├── model/             # Data models & documents
│   │   ├── document.hpp
│   │   ├── dict_document.hpp / dict_document.cpp
│   │   ├── yaml_document.hpp / yaml_document.cpp
│   │   ├── plugin_document.hpp
│   │   ├── record_table_model.hpp / record_table_model.cpp
│   │   ├── sidebar_model.hpp / sidebar_model.cpp
│   │   ├── table_row.hpp
│   │   └── table_builder.hpp / table_builder.cpp
│   ├── view/              # Qt widgets & panels
│   │   ├── record_table_view.hpp / record_table_view.cpp
│   │   ├── table_display.hpp / table_display.cpp
│   │   ├── sidebar_widget.hpp / sidebar_widget.cpp
│   │   ├── editor_panel.hpp / editor_panel.cpp
│   │   ├── editor_text_edit.hpp / editor_text_edit.cpp
│   │   ├── line_number_gutter.hpp / line_number_gutter.cpp
│   │   ├── annotations_panel.hpp / annotations_panel.cpp
│   │   ├── history_panel.hpp / history_panel.cpp
│   │   ├── log_tab.hpp / log_tab.cpp
│   │   ├── book_preview.hpp / book_preview.cpp
│   │   ├── validation_indicator.hpp / validation_indicator.cpp
│   │   ├── filter_tree.hpp / filter_tree.cpp
│   │   ├── status_filter_bar.hpp / status_filter_bar.cpp
│   │   └── translation_suggestion_tab.hpp / translation_suggestion_tab.cpp
│   ├── controller/        # Logic & orchestration
│   │   ├── editor_controller.hpp / editor_controller.cpp
│   │   ├── editor_config.hpp / editor_config.cpp
│   │   ├── search_engine.hpp / search_engine.cpp
│   │   ├── search_manager.hpp / search_manager.cpp
│   │   ├── validation_manager.hpp / validation_manager.cpp
│   │   ├── annotation_manager.hpp / annotation_manager.cpp
│   │   ├── history_manager.hpp / history_manager.cpp
│   │   ├── find_replace_service.hpp / find_replace_service.cpp
│   │   └── operation_executor.hpp / operation_executor.cpp
│   ├── dialog/            # Modal dialogs
│   │   ├── find_replace_dialog.hpp / find_replace_dialog.cpp
│   │   ├── dict_selection_dialog.hpp / dict_selection_dialog.cpp
│   │   ├── first_run_dialog.hpp / first_run_dialog.cpp
│   │   └── spell_context_menu.hpp / spell_context_menu.cpp
│   ├── highlight/         # Text coloring
│   │   ├── syntax_highlighter.hpp / syntax_highlighter.cpp
│   │   ├── hyperlink_highlighter.hpp / hyperlink_highlighter.cpp
│   │   ├── annotation_highlighter.hpp / annotation_highlighter.cpp
│   │   └── composite_highlighter.hpp / composite_highlighter.cpp
│   ├── provider/          # Translation backends
│   │   ├── translation_provider.hpp
│   │   ├── ctranslate2_provider.hpp / ctranslate2_provider.cpp
│   │   ├── deepl_provider.hpp / deepl_provider.cpp
│   │   ├── google_provider.hpp / google_provider.cpp
│   │   └── model_downloader.hpp / model_downloader.cpp
│   ├── utility/           # Helpers
│   │   ├── status_colors.hpp
│   │   ├── display_name.hpp / display_name.cpp
│   │   ├── encoding_utils.hpp / encoding_utils.cpp
│   │   ├── plugin_op.hpp
│   │   ├── spell_checker.hpp / spell_checker.cpp
│   │   └── grammar_checker.hpp / grammar_checker.cpp
│   ├── io/                # File format readers/writers
│   │   ├── yaml_l10n_reader.hpp / yaml_l10n_reader.cpp
│   │   └── yaml_l10n_writer.hpp / yaml_l10n_writer.cpp
│   └── yampt.translator.vcxproj
├── yampt.editor/           # Standalone editor app (Qt6)
│   ├── main.cpp            # → yEditor.exe
│   ├── editor_window.hpp / editor_window.cpp
│   ├── model/
│   │   ├── nav_tree_model.hpp / nav_tree_model.cpp
│   │   └── view_tree_model.hpp / view_tree_model.cpp
│   ├── view/
│   │   ├── editor_tab.hpp / editor_tab.cpp
│   │   └── messages_panel.hpp / messages_panel.cpp
│   ├── dialog/
│   │   ├── filter_dialog.hpp / filter_dialog.cpp
│   │   └── plugin_select_dialog.hpp / plugin_select_dialog.cpp
│   └── yampt.editor.vcxproj
├── yampt.tests/            # Catch2 unit tests
│   └── yampt.tests.vcxproj
├── external/               # Third-party (CTranslate2, yyjson)
├── models/                 # CTranslate2 translation models
├── x64/Debug/              # Build output
│   ├── yampt.exe
│   ├── yTranslator.exe
│   ├── yEditor.exe
│   └── yampt.tests.exe
├── yampt.sln               # VS 2026 solution
└── vcpkg.json              # vcpkg manifest
```

## External Dependencies — Read Only

NEVER modify files directly in the `external/` folder. These are upstream third-party sources and must remain byte-for-byte identical to their original releases. You can only add new files/folders or remove them entirely. If a library needs patching, do it via wrapper code in the project source files or `imconfig.h` overrides.

## Solution Filters

vcxproj.filters must follow this structure:
- **Root (no filter)** — project's own root source files (main.cpp, main_window.cpp, session.cpp)
- **model** — files in the `model/` subfolder
- **view** — files in the `view/` subfolder
- **controller** — files in the `controller/` subfolder
- **dialog** — files in the `dialog/` subfolder
- **highlight** — files in the `highlight/` subfolder
- **provider** — files in the `provider/` subfolder
- **utility** — files in the `utility/` subfolder
- **io** — files in the `io/` subfolder
- **yampt** — files referenced from `../yampt/` (core library)

Never use default VS filters like "Source Files", "Header Files", or "Resource Files".

## Build

Visual Studio (v145 toolset). Open `yampt.sln` and build. Output: `x64/Debug/yampt.exe`, `x64/Debug/yTranslator.exe`, `x64/Debug/yEditor.exe`.

MSBuild path:
```
C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe
```

Command-line build (Release x64):
```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" yampt.sln /p:Configuration=Release /p:Platform=x64 /m
```

Debug build:
```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" yampt.sln /p:Configuration=Debug /p:Platform=x64 /m
```

## Running Tests

```
x64\Debug\yampt.tests.exe
```

Catch2 header-only, self-contained.

## Integration Test Output Structure

Integration tests live in `scripts/run_integration.ps1` (PowerShell). Run from repo root:

```powershell
.\scripts\run_integration.ps1
```

Output goes to `tests/` relative to the working directory:

```
tests/
├── en/
│   ├── Morrowind_en.json
│   ├── Tribunal_en.json
│   ├── Bloodmoon_en.json
│   └── Morrowind_en_with_base.json
├── pl/
│   ├── Morrowind_en_pl.json
│   ├── Tribunal_en_pl.json
│   ├── Bloodmoon_en_pl.json
│   └── Merged_en_pl.json
├── de/
│   ├── Morrowind_en_de.json
│   └── ...
├── fr/
│   ├── Morrowind_en_fr.json
│   └── ...
└── converted/
    └── *.esp / *.omwaddon
```

## Integration Test Rules

- All make-base tests require the translation engine. No inactive-heuristic tests.
- Do not assert on `missing_count` or `heuristic_matches` — the logs capture this information. Tests only verify that output is produced and basic sanity (total > 0, cells non-empty).
- Tests that depend on previous output (merge, make-with-base) read from `tests/`.

## Unit Test Rules

- Unit tests (`[u]` tag) are purely in-memory. They must never create, write, or read files on disk.
- Integration tests (`[i]` tag) may read/write temporary files to the system temp directory (`std::filesystem::temp_directory_path()`). Clean up after each test.
- Unit tests verify logic only: data structure operations, string manipulation, parsing from in-memory strings, algorithm correctness.
- Never skip or weaken a test to make it pass. If a test fails, diagnose and fix the root cause.
- No comments in test code. No spec references, requirement IDs, property descriptions, or explanatory prose. The test name is the documentation.

## Unit Test Naming Convention

Format: `"class_t::method, description"` or `"class_t::nested_t::method, description"`

Examples:
- `"tools_t::chapter_t::insert, new and duplicate keys"`
- `"tools_t::is_fnam, true IDs"`
- `"dict_merger_t::add_record, inserts entry"`
- `"script_parser_t, dial keywords"`
- `"file_list_t::classify, edge cases"`
- `"dict_document_t, path round-trip"`

Rules:
- First part is the fully-qualified type path using `::` separators (with `_t` suffix)
- If the test is about a specific method, include the method name after `::` before the comma
- If the test is about general class behavior (not one method), use just the class name before the comma
- Description after the comma is a short lowercase phrase
- Total test name must be under 80 characters — the VS Catch2 test adapter truncates longer names and loses the tag, causing them to appear under "No Traits"

## Cell Heuristic Matching — Log Format

When the translation engine is active, unmatched interior cells go through heuristic matching. The log shows:

```
[TRANSLATE iter=1 orig=2 model=1] "Abaelun Mine" -> "Mine d'Abaelun"
[TIE-SAME iter=1 orig=1 model=3 count=2] "Some Cell" -> "Native Cell"
[TIE iter=2 orig=0 model=1 count=3] "Unresolved Cell"
```

Score breakdown:
- `orig` — number of words from the **original English name** that appear in the native cell name. Only matters for proper nouns that survive unchanged across languages (Abaelun, Dagoth, Arkngthand). Safety net for when the model fails.
- `model` — number of words from the **translation engine output** that appear in the native cell name. This is the primary signal. The engine translates "Hall of Centrifuge" → "Halle der Zentrifuge", which matches the German native cell directly.

`model` is more important than `orig`. A high `model` score means the translation quality was good and found real target-language matches. A high `orig` with low `model` means the engine produced garbage but a proper noun saved the match.

Log tags:
- `[EXACT]` — foreign name == native name (identical string, no translation needed)
- `[TRANSLATE]` — unique best match found via translation + word overlap
- `[TIE-SAME]` — multiple native cells tied on score but all have the same name (resolved)
- `[TIE]` — unresolved tie, cell skipped this iteration (may resolve in later iterations as candidates shrink)

## Kiro Helper Scripts

When Kiro needs to run analysis on the codebase (e.g. parsing Morrowind.json for data), write a temporary Python script to a temp file, execute it, then delete it. Use PowerShell to invoke Python:

```powershell
python tmp_script.py
```

Python is available on this system. Use it for JSON parsing, data analysis, and any scripting tasks that are too complex for inline PowerShell.

---

## yampt CLI Usage (Batch Workflow Templates)

The `scripts/` folder contains `.bat` templates showing how end-users invoke yampt. These are **not** Kiro automation — they are documentation of the tool's intended usage.

### Make base dictionary (two language ESMs → translation pairs)
```
yampt.exe --make-base -f "native\Morrowind.esm" "foreign\Morrowind.esm"
```

### Merge dictionaries (first-wins precedence)
```
yampt.exe --merge -d "dict1.xml" "dict2.xml" "dict3.xml" -o "MERGED.xml"
```

### Convert plugin (apply translation)
```
yampt.exe --convert --add-hyperlinks -f "input.esp" -d "base.xml" "user1.xml" "user2.xml"
```

### Create plugin (only modified records)
```
yampt.exe --create -f "input.esp" -d "base.xml" -s "_translated"
```

### Make dictionaries for untranslated/changed entries
```
yampt.exe --make-not -f "plugin.esp" -d "base.xml" "glossary.xml"
yampt.exe --make-changed -f "plugin.esp" -d "hyperlinks.xml" "glossary.xml"
```

### Make raw dictionary (extract all text from a single file)
```
yampt.exe --make-raw -f "Morrowind.esm"
```

---

## MWScript Records in TES3 Files

"Script" in the Morrowind modding context means **MWScript** — the game's built-in scripting language stored inside ESM/ESP plugin files. This is what yampt's `ScriptParser` class translates. Do NOT confuse with batch files or Kiro helper scripts.

Each `"type": "Script"` record in tes3conv JSON contains:
- `id` — MWScript name (e.g. `"AbebaalAttack"`)
- `header` — `num_shorts`, `num_longs`, `num_floats`, `bytecode_length`, `variables_length`
- `variables` — base64-encoded variable name table
- `bytecode` — base64-encoded compiled script data (SCDT subrecord)
- `text` — MWScript source code (SCTX subrecord), uses `\r\n` line endings

### ScriptParser Keyword Statistics (Morrowind.esm)

| Keyword | Scripts | Notes |
|---------|--------:|-------|
| messagebox | 113 | Translatable strings in quotes |
| say | 68 | Two quoted params: sound file + subtitle text |
| getpccell | 53 | Cell name in quotes; comparison always `== 0` or `== 1` in vanilla |
| positioncell | 34 | Cell name is 5th parameter (index 4) |
| addtopic | 19 | Dialog topic name in quotes |
| choice | 3 | Translatable choice options in quotes |
| showmap | 1 | Cell name in quotes |
| centeroncell | 0 | Not used in vanilla |
| aifollowcell | 0 | Not used in vanilla |
| aiescortcell | 0 | Not used in vanilla |
| placeitemcell | 0 | Not used in vanilla |

### getpccell Compiled Data (SCDT) Layout

In compiled MWScript bytecode, a getpccell expression is structured as:
```
[expr_size_byte] [X_marker] [?] [text_size_byte] [cell_name_text] [comparison_suffix]
```

- `expr_size_byte` — total bytes from this byte to end of expression
- The ScriptParser finds the `'X'` marker via `rfind('X', pos_c)` then goes back 2 bytes to locate `expr_size_byte`
- After replacing the cell name text, it recalculates `expr_size`
- Comparison suffix is typically `" == 1"` (5 bytes) or `" == 0"` (5 bytes) in vanilla
- Standalone usage (no comparison) takes the `!= " "` branch — expression ends at text end

### False Positive Risk in makeScriptMessages

Only 2 false positives in vanilla Morrowind.esm:
- `DaedraMehrunes`: "choice" inside filename `"A_MehrunesChoice1.wav"`
- `LorkhanHeart`: "say" inside variable name `countSays`

Low impact for vanilla, but mods with variables like `mychoice`, `sayHello` would be affected.


---

## Release Packaging (`pack_release.ps1`)

The `pack_release.ps1` script creates a zip archive from `x64\Release\` for distribution.

What is included:
- `yampt.exe`, `yTranslator.exe`, `yEditor.exe`
- All DLLs from the output dir (including `ctranslate2.dll`)
- `dictionaries/` folder (spell check dictionaries)
- `platforms/` folder (Qt platform plugins)

What is NOT included:
- `models/` — translation engine models are too large for distribution; users download them separately via `download_models.py`
- `scripts/` — batch templates live in the repo for reference, not in the release package
- `tests/`, `workspace/` — development artifacts
- `.pdb` files — debug symbols stay local
- `.ini` files — user-specific config
