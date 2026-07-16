# yampt — CLI Manual

Command-line tool for batch translation of Morrowind ESM/ESP plugins.

## Usage

```
yampt.exe <command> [options]
```

Running yampt without arguments prints the version number.

## Commands

### --make

Extracts all translatable text from one or more plugins and writes a dictionary file. Every entry starts as Untranslated (old text and new text are identical).

```
yampt.exe --make -f "Plugin.esp"
yampt.exe --make -f "Plugin.esp" -o "output.json"
yampt.exe --make -f "Plugin.esp" -d "base.json"
```

When a base dictionary is provided with `-d`, entries that match by key or original text receive their translations from the base. Without `-d`, a raw dictionary is created. If `-o` is not specified, the output file is named after the input plugin with a `.json` extension.

### --make-base

Compares two language versions of the same plugin to produce matched translation pairs. Requires exactly two files with `-f`: the first is the native-language version (its text becomes `new_text`), the second is the foreign-language version (its text becomes `old_text`).

```
yampt.exe --make-base -f "Morrowind_PL.esm" "Morrowind_EN.esm"
yampt.exe --make-base -f "Native.esm" "Foreign.esm" --partial
yampt.exe --make-base -f "Native.esm" "Foreign.esm" --translate "models/nllb-600M"
```

The output file is named automatically with a `.BASE.json` suffix.

Two modes control how identical-text entries are handled:

- **Full** (default) — identical entries are treated as proper nouns and receive status Translated.
- **Partial** (`--partial` flag) — identical entries are checked against an English dictionary. Entries containing English words are marked Untranslated. Entries with no English words are marked To Verify.

The `--translate` option loads a translation model from the specified directory. When active, unmatched interior cell names are translated and compared against native candidates using word overlap scoring. This improves cell matching for languages where cell names are fully translated.

### --merge

Combines multiple dictionaries into one. The last dictionary in the list has highest priority — its entries win when keys conflict.

```
yampt.exe --merge -d "dict1.json" "dict2.json" "dict3.json" -o "merged.json"
```

The `-o` flag is required. XML dictionary files are no longer supported.

### --convert

Applies translations from dictionaries to a plugin. Only entries with status Translated are used. The output replaces translated records in a copy of the original plugin. The original file is not modified.

```
yampt.exe --convert -f "Plugin.esp" -d "base.json" "user.json"
yampt.exe --convert -f "Plugin.esp" -d "merged.json" -s "_translated"
```

The `-s` flag appends a suffix to the output filename (e.g. `Plugin_translated.esp`). Without it, the output overwrites the input path. Multiple dictionaries can be provided — they are merged internally with last-wins priority.

### --create

Works like --convert but the output contains only the records that were actually modified. Use this to produce a lightweight translation patch that can be loaded alongside the original plugin.

```
yampt.exe --create -f "Plugin.esp" -d "base.json" -s "_patch"
```

The output is named `Plugin.CREATED.esp` (or with the suffix if `-s` is provided).

## Options

- `-f <path>` — input plugin file. Can be specified multiple times for --make and --convert.
- `-d <path>` — dictionary file. Can be specified multiple times (merged internally).
- `-o <path>` — output path (required for --merge, optional for --make).
- `-s <suffix>` — filename suffix appended to the output (used with --convert).
- `--translate <model_dir>` — load a translation model for heuristic cell matching during --make-base.
- `--partial` — use partial mode for --make-base (check identical entries against English dictionary).
- `--debug` — enable verbose logging (shows script parser traces and internal diagnostics).

## Examples

Make a base dictionary from English and Polish Morrowind:

```
yampt.exe --make-base -f "Morrowind_PL.esm" "Morrowind_EN.esm"
```

Make base with translation engine for better cell matching:

```
yampt.exe --make-base -f "Morrowind_PL.esm" "Morrowind_EN.esm" --translate "models/nllb-600M"
```

Merge three dictionaries (Bloodmoon wins over Tribunal wins over Morrowind):

```
yampt.exe --merge -d "Morrowind.json" "Tribunal.json" "Bloodmoon.json" -o "merged.json"
```

Convert a plugin using the merged dictionary:

```
yampt.exe --convert -f "MyMod.esp" -d "merged.json"
```

Create a patch plugin:

```
yampt.exe --create -f "MyMod.esp" -d "merged.json"
```

Extract a raw dictionary from a plugin:

```
yampt.exe --make -f "MyMod.esp"
```

Extract with base dictionary applied:

```
yampt.exe --make -f "MyMod.esp" -d "merged.json"
```
