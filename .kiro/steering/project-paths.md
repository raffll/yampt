# Project Paths & Build

## Repository Layout

```
yampt/
├── yampt/                  # Main project source (C++)
│   ├── main.cpp
│   ├── tools.hpp / tools.cpp
│   ├── dictreader.hpp / dictreader.cpp
│   ├── dictwriter.hpp / dictwriter.cpp
│   ├── dictmerger.hpp / dictmerger.cpp
│   ├── dictcreator.hpp / dictcreator.cpp
│   ├── esmreader.hpp / esmreader.cpp
│   ├── esmconverter.hpp / esmconverter.cpp
│   ├── scriptparser.hpp / scriptparser.cpp
│   ├── userinterface.hpp / userinterface.cpp
│   ├── includes.hpp
│   ├── json.hpp
│   └── yampt.vcxproj
├── yampt.tests/            # Catch2 unit tests
│   ├── catch.hpp
│   ├── tests.main.cpp
│   ├── tests.tools.cpp
│   ├── tests.dictreader.cpp
│   ├── tests.dictwriter.cpp
│   ├── tests.dictmerger.cpp
│   ├── tests.dictcreator.cpp
│   ├── tests.esmreader.cpp
│   ├── tests.scriptparser.cpp
│   └── yampt.tests.vcxproj
├── scripts/                # Batch file templates for yampt CLI workflows
├── x64/Debug/              # Build output
│   ├── yampt.exe
│   └── yampt.tests.exe
├── packages/               # NuGet packages (Boost 1.71, to be removed)
├── yampt.sln               # VS2022 solution
└── packages.config         # NuGet manifest (to be removed)
```

## External Dependencies — Read Only

NEVER modify files directly in the `external/` folder. These are upstream third-party sources and must remain byte-for-byte identical to their original releases. You can only add new files/folders or remove them entirely. If a library needs patching, do it via wrapper code in the project source files or `imconfig.h` overrides.

## Solution Filters

vcxproj.filters must follow this structure:
- **Root (no filter)** — project's own source files (files in the same directory as the vcxproj)
- **yampt** — files referenced from `../yampt/`
- **external** — files from `../external/` (yyjson, catch)
- **external\imgui** — imgui core files
- **external\imgui\backends** — SDL2/OpenGL3 backend files

Never use default VS filters like "Source Files", "Header Files", or "Resource Files".

## Build

Visual Studio (v143 toolset). Open `yampt.sln` and build. Output: `x64/Debug/yampt.exe`.

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

Integration tests (`[i]` tag) write output to `tests/` relative to the working directory:

```
tests/
├── en/
│   ├── Morrowind_en.json
│   ├── Morrowind_en.log
│   ├── Tribunal_en.json
│   ├── Tribunal_en.log
│   ├── Bloodmoon_en.json
│   ├── Bloodmoon_en.log
│   ├── Morrowind_en_with_base.json
│   └── Morrowind_en_with_base.log
├── pl/
│   ├── Morrowind_en_pl.json
│   ├── Morrowind_en_pl.log
│   ├── Tribunal_en_pl.json
│   ├── Tribunal_en_pl.log
│   ├── Bloodmoon_en_pl.json
│   ├── Bloodmoon_en_pl.log
│   ├── Merged_en_pl.json
│   └── Merged_en_pl.log
├── de/
│   ├── Morrowind_en_de.json
│   ├── Morrowind_en_de.log
│   └── ...
└── fr/
    ├── Morrowind_en_fr.json
    ├── Morrowind_en_fr.log
    └── ...
```

JSON and log files live side-by-side in the same language folder. Log filenames match their corresponding JSON filenames (same base name, `.log` extension).

## Integration Test Rules

- All make-base tests require the translation engine. No inactive-heuristic tests.
- Do not assert on `missing_count` or `heuristic_matches` — the logs capture this information. Tests only verify that output is produced and basic sanity (total > 0, cells non-empty).
- Tests that depend on previous output (merge, make-with-base) read from `tests/`.

## Unit Test Rules

- Unit tests (`[u]` tag) are purely in-memory. They must never create, write, or read files on disk.
- All file I/O testing (write-read round trips, JSON serialization, dict_writer output) belongs in integration tests (`[i]` tag).
- Unit tests verify logic only: data structure operations, string manipulation, parsing from in-memory strings, algorithm correctness.
- Never create or modify integration tests. Only the user writes and maintains integration tests.

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
