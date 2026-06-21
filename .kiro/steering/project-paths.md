# Project Paths & Build

## Repository Layout

```
yampt/
├── yampt/                  # Core library + CLI (C++)
│   ├── main.cpp
│   ├── tools.hpp / tools.cpp
│   ├── dict_reader.hpp / dict_reader.cpp
│   ├── dict_writer.hpp / dict_writer.cpp
│   ├── dict_merger.hpp / dict_merger.cpp
│   ├── dict_creator.hpp / dict_creator.cpp
│   ├── esm_reader.hpp / esm_reader.cpp
│   ├── esm_converter.hpp / esm_converter.cpp
│   ├── script_parser.hpp / script_parser.cpp
│   ├── user_interface.hpp / user_interface.cpp
│   ├── translation_engine.hpp / translation_engine.cpp
│   ├── file_list.hpp / file_list.cpp
│   ├── plugin_scan/       # Plugin comparison/conflict detection
│   ├── includes.hpp
│   └── yampt.vcxproj
├── yampt.translator/       # GUI translation workbench (Qt6)
│   ├── main.cpp            # → yTranslator.exe
│   ├── main_window.hpp / main_window.cpp
│   ├── editor/            # Editor tab (xEdit-like plugin viewer)
│   ├── dictionaries/      # Hunspell spell check dictionaries
│   └── yampt.translator.vcxproj
├── yampt.editor/           # Standalone editor app (Qt6)
│   ├── main.cpp            # → yEditor.exe
│   ├── editor_window.hpp / editor_window.cpp
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
- **Root (no filter)** — project's own source files (files in the same directory as the vcxproj)
- **yampt** — files referenced from `../yampt/`
- **editor** — files from the `editor/` subfolder (yampt.translator and yampt.editor)
- **external** — files from `../external/` (yyjson, catch)

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
- All file I/O testing (write-read round trips, JSON serialization, dict_writer output) belongs in integration tests (`[i]` tag).
- Unit tests verify logic only: data structure operations, string manipulation, parsing from in-memory strings, algorithm correctness.
- Never create or modify integration tests. Only the user writes and maintains integration tests.
- Never skip or weaken a test to make it pass. If a test fails, diagnose and fix the root cause.

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
