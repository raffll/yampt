# Project Structure

This document is domain/architecture reference for the yampt solution: repository
layout, build commands, project dependency rules, include conventions, and where
auxiliary files live. Consult it before adding files, changing project references,
or touching the build.

---

## Repository Layout

```
yampt/
├── yampt.core/             # Core library (C++ static lib) → yampt.lib
│   ├── source/
│   │   ├── creator/       # dict_creator_t facade + strategy classes (single, base, ordered)
│   │   ├── merger/        # dict_merger_t
│   │   ├── converter/     # esm_converter_t, script_parser_t, scdt_patcher_t
│   │   ├── translator/    # translation_engine_t
│   │   ├── scanner/       # plugin_scan_t, plugin_index_t, record_conflict, merge_patch_store, merge_patch_ops, auto_merge, sub_record_merge, batch_cleaner, fixers
│   │   ├── decoder/       # conflict_slots, sub_record_iter, sub_record_schema, view_tree_format, content_alignment, view_group_def, field_encoder, field_validator, record_header_flags, scvr_condition
│   │   ├── io/            # File format readers/writers (ESM, JSON, codepage, file_list, yaml_l10n, binary_file_io)
│   │   └── utility/       # Pure helpers (domain_types, app_logger, string_utils, record_types, status_types, dict_kind, record_behavior, case_fold, char_diff, keyword_trie, language_config)
│   └── yampt.core.vcxproj
├── yampt.qt/               # Qt bridge library (C++ static lib + Qt6) → yampt.qt.lib
│   ├── source/            # theme_system, conflict_types, settings_store, path_resolver, resource_paths, translation_example
│   └── yampt.qt.vcxproj
├── yampt.cli/              # CLI entry point → yampt.exe (links yampt.lib)
│   ├── source/
│   │   ├── interface/     # CLI boundary (user_interface_t)
│   │   └── main.cpp
│   └── yampt.cli.vcxproj
├── yampt.translator/       # GUI translation workbench (Qt6) → yTranslator.exe
│   ├── source/
│   │   ├── controller/    # Orchestration (editor_controller, record_display_controller)
│   │   ├── model/         # Data models & documents (dict_document, yaml_document, table, sidebar, filter_state)
│   │   ├── view/          # Qt widgets (all use _view_t suffix) + display_name
│   │   ├── editor/        # Editing services (edit_history, find_replace, glossary, spell_checker, row_filter, byte_limit_validator, operation_executor)
│   │   ├── session/       # Session lifecycle (session, workspace_watcher, sidebar_controller, plugin_operations_controller)
│   │   ├── dialog/        # Modal dialogs
│   │   │   └── settings/  # Settings dialog pages
│   │   ├── highlighter/   # Text coloring (syntax, hyperlinks, annotations, grammar, highlight_applier, highlight_coordinator)
│   │   ├── translator/    # Translation backends (CTranslate2, DeepL, Google)
│   │   └── main.cpp
│   └── yampt.translator.vcxproj
├── yampt.editor/           # Standalone editor app (Qt6) → yEditor.exe
│   ├── source/
│   │   ├── controller/    # Orchestration (merge_controller, view_context_menu)
│   │   ├── model/         # Data models (nav_tree_model, view_tree_model, decode logic)
│   │   ├── view/          # Qt widgets + editor_delegates
│   │   ├── patcher/       # Patch building (patch_builder, plugin_cleaner)
│   │   ├── session/       # Session lifecycle (plugin_session, profile_reader)
│   │   ├── dialog/        # Modal dialogs
│   │   │   └── settings/  # Settings dialog pages
│   │   └── main.cpp
│   └── yampt.editor.vcxproj
├── yampt.tests/            # Catch2 unit tests
│   ├── source/
│   └── yampt.tests.vcxproj
├── external/               # Third-party submodules (CTranslate2, dictionaries) + local-only build artifacts/models
├── models/                 # CTranslate2 translation models
├── scripts/                # PowerShell/Python automation scripts
├── docs/dictionaries/      # Holds only LICENSE; .aff/.dic generated at build from external/dictionaries submodule
├── x64/Debug/              # Build output
├── yampt.sln               # VS 2026 solution
└── vcpkg.json              # vcpkg manifest
```

The vcxproj.filters are the authoritative file listing — they mirror disk 1:1. Do not maintain a separate file list here.

## Solution Filters

vcxproj.filters files must be flat — no `<Filter>` definitions, no `<Filter>` child elements on items. VS uses "Show All Files" mode to display the actual disk folder structure. The disk is the source of truth, not the filters file.

Never use default VS filters like "Source Files", "Header Files", or "Resource Files". Never add virtual folder groupings.

When adding, removing, or renaming a source file in any project, always update the corresponding `.vcxproj.filters` file in the same operation. Never leave filters out of sync with the vcxproj.

## Include Convention

- `#include "..."` (quotes) — same-project includes only
- `#include <...>` (angle brackets) — cross-project includes (resolved via `AdditionalIncludeDirectories`)

Each project's `AdditionalIncludeDirectories` contains `$(ProjectDir)source` for its own files, plus `$(SolutionDir)yampt.core\source` (and `$(SolutionDir)yampt.translator\source`, `$(SolutionDir)yampt.editor\source` for tests) for cross-project access.

Never use relative paths like `../../yampt/...` or `../yampt.translator/...` in `#include` directives. Use `<folder/file.hpp>` instead — it's portable across platforms and survives folder restructuring.

## External Dependencies — Read Only

The `external/` folder holds third-party git submodules — `external/CTranslate2` (`OpenNMT/CTranslate2`, pinned release tag) and `external/dictionaries` (`wooorm/dictionaries`) — plus local-only, gitignored content (the built CTranslate2 artifacts under `external/CTranslate2/build/`, translation models, `7za.exe`). NEVER modify files inside a submodule; they must remain byte-for-byte identical to upstream. To change a pinned version, update the submodule and commit the new gitlink. If a library needs patching, do it via wrapper code in the project source files. Other libraries (yyjson, nlohmann-json, sentencepiece, hunspell, Qt, Catch2) come from vcpkg — see `vcpkg.json` — not from `external/`.

Init the submodules after cloning:
```powershell
git submodule update --init --recursive external/CTranslate2
git submodule update --init external/dictionaries
```

## Build

Visual Studio (v145 toolset). Open `yampt.sln` and build. Output: `x64/Debug/yampt.exe`, `x64/Debug/yTranslator.exe`, `x64/Debug/yEditor.exe`.

MSBuild path — do NOT hardcode the VS edition (Community/Professional/Enterprise differ per machine). Discover it with `vswhere`:
```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"
```

Command-line build (Release x64):
```powershell
& $msbuild yampt.sln /p:Configuration=Release /p:Platform=x64 /m
```

Debug build:
```powershell
& $msbuild yampt.sln /p:Configuration=Debug /p:Platform=x64 /m
```

> First build is slow (~50 min on a cold vcpkg cache): vcpkg compiles Qt6 (qtbase) and other dependencies from source, and CTranslate2 builds on its first run. This is a one-time cost — once cached, subsequent builds take seconds to a few minutes. A long silent stretch during the Qt or CTranslate2 compile is normal progress, not a hang. A clean build ends with `Liczba błędów: 0` / `0 Error(s)` in the MSBuild summary.

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

Format: `"class_t::method, description"` or `"namespace::function, description"`

Examples:
- `"chapter_t::insert, new and duplicate keys"`
- `"domain_types::is_fnam, true IDs"`
- `"dict_merger_t::add_record, inserts entry"`
- `"script_parser_t::convert_script, dial keywords"`
- `"file_list_t::classify, edge cases"`
- `"string_utils::trim_cr, no CR present"`
- `"record_conflict::compute_conflict_all, override detected"`
- `"merge_patch_store_t::add, inserts record"`
- `"app_logger_t::add_log, sets error flag"`

Rules:
- Every test name must contain `::` — format is always `owner::member, description`
- First part is the fully-qualified owner using `::` separators — class (`_t` suffix) or namespace
- After `::` comes the method or function name being tested
- Never use just the class name before the comma without `::` — pick the primary method the test exercises
- Never use a bare function name without a class or namespace prefix
- Description after the comma is a short lowercase phrase
- Total test name has no hard length limit — the `.runsettings` uses `--reporter xml` for discovery which handles any length. Keep names concise for readability but don't truncate for tooling.

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
yampt.exe --make-base -f "foreign\Morrowind.esm" "native\Morrowind.esm"
```
The `-f` order is always foreign first, native second. This yields `old_text = foreign text`, `new_text = native text` (e.g. for EN→PL, `-f en\Morrowind.esm pl\Morrowind.esm` produces `old="Blades"` → `new="Ostrza"`).

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
- `providers/` folder (web translation provider configs)
- `languages.json` (language definitions)
- `docs/` folder (README.md, CHANGELOG.md, manuals)

What is NOT included:
- `models/` — translation engine models are too large for distribution; users download them separately via `download_models.py`
- `scripts/` — batch templates live in the repo for reference, not in the release package
- `tests/`, `workspace/` — development artifacts
- `.pdb` files — debug symbols stay local
- `.ini` files — user-specific config

---

## Project Dependency Graph

```
yampt.core (StaticLibrary → yampt.core.lib)
   ↑ pure C++ — no Qt, no other projects
   │
yampt.qt (StaticLibrary → yampt.qt.lib)
   ↑ depends on: yampt.core + Qt
   │
├── yampt.cli      → links: yampt.core.lib
├── yampt.editor   → links: yampt.core.lib + yampt.qt.lib
├── yampt.translator → links: yampt.core.lib + yampt.qt.lib
└── yampt.tests    → links: yampt.core.lib + yampt.qt.lib
                     (+ compiles yampt.translator/yampt.editor .cpp directly)
```

## Dependency Rules

### yampt.core — Pure C++ Library

- **No Qt includes** (`QSettings`, `QString`, `QColor`, `QDir`, `QFile`, etc.)
- **No includes from other yampt projects** (yampt.qt, yampt.editor, yampt.translator)
- **Only external dependencies**: CTranslate2, vcpkg C++ libraries (sentencepiece, abseil), standard library
- **Include paths**: `$(ProjectDir)source`, `$(SolutionDir)external`, CTranslate2 include/build
- When a new `.cpp` file is added here, all consuming projects get it automatically through `yampt.core.lib`. No additional edits needed.

### yampt.qt — Qt Bridge Library

- **Depends on**: yampt.core (headers only via include path) + Qt6
- **Contains**: Qt-dependent utilities that wrap or extend core types with Qt features
- **Current contents**: `theme_system`, `conflict_types` (QColor helpers), `settings_store` (QSettings wrapper), `path_resolver` (platform-aware path resolution), `resource_paths` (resource directory accessors), `translation_example`
- **Include paths**: `$(ProjectDir)source`, `$(SolutionDir)yampt.core\source`, Qt6 includes
- When adding a Qt-dependent class that other projects need, put it here.

### yampt.cli — Console Application

- **Depends on**: yampt.core.lib (linked)
- **Does NOT depend on Qt or yampt.qt**
- **Contains**: CLI entry point, user_interface_t
- **Only own source files** — no cross-project `.cpp` compilation

### yampt.editor — Qt GUI Application (yEditor)

- **Depends on**: yampt.core.lib + yampt.qt.lib (linked via ProjectReference)
- **Contains**: Plugin conflict viewer, merged patch editor
- **Only own source files** — no cross-project `.cpp` compilation
- Include paths include `yampt.core\source` and `yampt.qt\source` for headers

### yampt.translator — Qt GUI Application (yTranslator)

- **Depends on**: yampt.core.lib + yampt.qt.lib (linked via ProjectReference)
- **Contains**: Translation workbench, dictionary editor
- **Only own source files** — no cross-project `.cpp` compilation
- Include paths include `yampt.core\source` and `yampt.qt\source` for headers

### yampt.tests — Test Application

- **Depends on**: yampt.core.lib + yampt.qt.lib (linked via ProjectReference)
- **Also compiles .cpp directly from**: yampt.translator, yampt.editor (because those are Applications, not libraries — no .lib to link)
- This is the ONLY project allowed to compile `.cpp` files from other projects
- Include paths include all project source directories for header access

## Adding New Files

| File depends on... | Put it in... |
|---|---|
| Nothing / standard C++ only | `yampt.core` |
| CTranslate2 | `yampt.core` |
| Qt classes (QColor, QSettings, QWidget, etc.) but is shared | `yampt.qt` |
| Qt + is specific to yEditor UI | `yampt.editor` |
| Qt + is specific to yTranslator UI | `yampt.translator` |
| Needs testing | If it's in yampt.core, tests get it free via lib. If it's in translator/editor, add the `.cpp` to yampt.tests vcxproj. |

## Never Do

- Never add Qt includes to yampt.core
- Never add `#include` of yampt.qt/yampt.editor/yampt.translator headers in yampt.core
- Never compile `.cpp` from yampt.core in other projects (they link the lib)
- Never compile `.cpp` from yampt.qt in other projects (they link the lib)
- Never add yampt.core.lib or yampt.qt.lib as AdditionalDependencies without also adding the ProjectReference (build order)

## Output Directory (Configurable)

The output directory is configurable per mode in Settings → Paths. All files created by yEditor (merged patch, cleaned plugins) go into this directory.

- **Folder mode**: `{selected_folder}/{output_dir}/` — default: same as base directory (empty)
- **MO2 mode**: `{MO2_profile_dir}/{output_dir}/` — default: `../../overwrite`
- **OpenMW mode**: `{openmw.cfg_dir}/{output_dir}/` — default: `data`

The merged patch filename is always `Merged Patch.esp` — only the output directory is configurable.

---

## Auxiliary Files Live in the Solution Root

All auxiliary files live in the solution root (`yampt/`), not inside project subfolders.

- `scripts/` — PowerShell and Python scripts (pack_model.ps1, download_models.py, finetune_model.py, pack_release.ps1)
- `models/` — CTranslate2 translation models (nllb-600M/, etc.)
- `docs/dictionaries/` — holds only `LICENSE`; the `.aff`/`.dic` files are generated at build time from the `external/dictionaries` submodule and are gitignored
- `external/` — git submodules for third-party sources: `external/CTranslate2` (`OpenNMT/CTranslate2`, pinned release tag) and `external/dictionaries` (`wooorm/dictionaries`, source of all Hunspell spellcheck dictionaries). Also holds local-only, gitignored content (built CTranslate2 artifacts, translation models, `7za.exe`).
- `build/` — output of pack/release scripts (zip files, release artifacts)

Never place scripts, models, or dictionaries inside `yampt.translator/`, `yampt.editor/`, or other project subdirectories.

All pack scripts (pack_release.ps1, pack_model.ps1) must write their output to `build/` in the solution root.
