# Design Document

## Overview

This document describes the technical approach for each improvement to the yampt codebase. The changes are grouped by component and ordered from lowest to highest risk. All changes are in-place modifications to existing files — no new source files are introduced except for the vcpkg manifest and updated project files.

## Architecture

The codebase has a clear layered structure:

```
main.cpp → UserInterface → DictCreator / DictMerger / EsmConverter
                         ↓
                    EsmReader  ScriptParser  DictReader
                         ↓
                       Tools (static utilities, logging)
```

No architectural changes are made. The improvements are targeted fixes and optimizations within existing classes.

---

## Component Designs

### Tools — Bug Fixes (Requirements 1–5)

**Req 1 — readFile reserve size**

Current code calls `file.tellg()` before seeking to end, so it always returns 0. Fix: seek to end with `file.seekg(0, std::ios::end)`, capture the size, then seek back to beginning with `file.seekg(0, std::ios::beg)` before reading.

```cpp
std::string Tools::readFile(const std::string & path)
{
    std::string content;
    std::ifstream file(path, std::ios::binary);
    if (file)
    {
        file.seekg(0, std::ios::end);
        content.reserve(static_cast<size_t>(file.tellg()));
        file.seekg(0, std::ios::beg);
        // ... existing read loop unchanged
    }
}
```

**Req 2 — convertStringByteArrayToUInt buffer initialization**

Current code copies up to 4 bytes into `buffer[4]` without zero-initializing it. When `str.size() == 1`, bytes 1–3 of `buffer` are uninitialized, and the function still reads all 4 bytes of `ubuffer` in the `size == 4` branch (unreachable for size 1, but the copy itself is UB). Fix: zero-initialize `buffer` before the copy, and only copy `str.size()` bytes.

```cpp
char buffer[4] = {};
str.copy(buffer, str.size());
```

**Req 3 — trimCR only removes trailing CR**

Current code erases the last character whenever `\r` appears anywhere in the string. Fix: check specifically that the last character is `\r`.

```cpp
std::string Tools::trimCR(std::string str)
{
    if (!str.empty() && str.back() == '\r')
        str.pop_back();
    return str;
}
```

**Req 4 — replaceNonReadableCharsWithDot signed char**

Current code casts to `int`, which is negative for chars 128–255 on platforms where `char` is signed. Fix: cast to `unsigned char` before the range check, and use `std::isprint` with the unsigned value.

```cpp
if (std::isprint(static_cast<unsigned char>(str[i])))
    text += str[i];
else
    text += '.';
```

**Req 5 — addAnnotations find all occurrences**

Current code finds the first occurrence of a keyword, checks if it is preceded by an alpha char, and if so does `pos += 1; continue` — but `pos` is a local variable that is overwritten at the top of the next iteration. The intent is to skip this occurrence and try the next. Fix: use a loop over all occurrences of the keyword in the source string.

```cpp
size_t search_from = 0;
while (true)
{
    pos = source_lc.find(key_text_lc, search_from);
    if (pos == std::string::npos) break;
    if (pos != 0 && std::isalpha(static_cast<unsigned char>(source_lc[pos - 1])))
    {
        search_from = pos + 1;
        continue;
    }
    // valid match — append annotation and break
    result += extended ? " [" + key_text + " -> " + val_text + "]"
                       : " [" + val_text + "]";
    break;
}
```

---

### DictMerger — Bug Fix (Requirement 6)

**mergeDict replace semantics**

The current `mergeDict` logs a warning and increments `counter_replaced` when a key already exists with a different value, but never actually overwrites the existing value. Fix: add `search->second = elem.second;` in the "found, different" branch.

```cpp
else if (search->second != elem.second)
{
    search->second = elem.second;   // ← add this line
    if (type != Tools::RecType::Glossary)
        Tools::addLog("Warning: replaced " + ...);
    counter_replaced++;
}
```

Annotations chapter must also be merged with the same last-wins semantics (Requirement 30 — see DictReader/Tools section below).

---

### ScriptParser — Bug Fixes (Requirements 7–8)

**Req 7 — end keyword detection parentheses**

The condition `line_lc == "end" || line_lc.size() > 3 && line_lc.substr(0, 4) == "end "` relies on `&&` binding tighter than `||`. Add explicit parentheses:

```cpp
if (line_lc == "end" ||
    (line_lc.size() > 3 && line_lc.substr(0, 4) == "end "))
```

**Req 8 — getpccell expression size**

The current code assumes the comparison expression always ends with `" == 1"` (5 bytes). For multi-digit values like `" == 10"` this is wrong. Fix: instead of hardcoding `+ 5`, scan forward from `pos_c + new_text.size()` to find the actual end of the expression (the next null byte or the end of the SCDT string).

```cpp
// Find end of expression dynamically
size_t expr_end = pos_c + new_text.size();
while (expr_end < new_SCDT.size() && new_SCDT[expr_end] != '\0')
    expr_end++;
end_of_expr = expr_end;
```

---

### DictCreator — Bug Fixes (Requirements 9–10)

**Req 9 — dangling reference in RAW/BASE constructors**

Both the RAW and BASE constructors initialize `merger(DictMerger())` — binding a `const DictMerger &` member to a temporary. The temporary is destroyed at the end of the constructor's member-initializer list, leaving `merger` dangling. Fix: add a `DictMerger owned_merger` member and use it when no external merger is provided.

```cpp
// In DictCreator.hpp — add member:
DictMerger owned_merger;

// RAW constructor:
DictCreator::DictCreator(const std::string & path)
    : esm(path), esm_ref(esm), mode(Tools::CreatorMode::RAW), merger(owned_merger)
```

**Req 10 — makeScriptMessages word boundary**

`makeScriptMessages` currently uses `std::string::find` to locate keywords, which matches substrings (e.g. "choice" inside "mychoice"). Fix: use the same `\b` word-boundary regex approach already used in `ScriptParser::convertLine`.

```cpp
std::vector<std::string> DictCreator::makeScriptMessages(const std::string & script_text)
{
    std::vector<std::string> messages;
    std::istringstream ss(script_text);
    std::string line;
    while (std::getline(ss, line))
    {
        auto line_lc = line;
        std::transform(line_lc.begin(), line_lc.end(), line_lc.begin(), ::tolower);
        for (const auto & kw : Tools::keywords)
        {
            std::regex r("\\b" + kw + "\\b", std::regex::optimize);
            if (std::regex_search(line_lc, r))
            {
                messages.push_back(line);
                break;
            }
        }
    }
    return messages;
}
```

---

### DictCreator — Feature Fix (Requirement 28)

**addHyperlinks for Greeting dialog**

`addHyperlinks` currently skips annotation for Voice (`"V"`) dialog type. The key prefix starts with the dialog type letter. Greeting (`"G"`) should be treated the same as Topic (`"T"`) — hyperlinks appended. Change the condition from "skip if not T" to "skip only if V".

```cpp
std::string DictCreator::addHyperlinks(const Tools::Entry & entry)
{
    const auto & key = entry.key_text;
    if (key.size() > 0 && key.substr(0, 1) == "V")
        return entry.val_text;  // Voice — no hyperlinks
    // All other types (T, G, P, J) get hyperlinks
    ...
}
```

---

### EsmReader — Feature Fix (Requirement 29)

**4-byte cell name subrecords**

`eraseNullChars` already strips the null terminator from subrecord text. The issue is that `makeDictCELL_Unordered_Pattern` uses `esm_cur.getValue().content` (raw binary including null) rather than `.text` (null-stripped) when building the pattern string for the cell NAME. Fix: use `.text` for the NAME portion of the pattern.

```cpp
std::string DictCreator::makeDictCELL_Unordered_Pattern(EsmReader & esm_cur)
{
    std::string pattern;
    esm_cur.setValue("DATA");
    pattern += esm_cur.getValue().content;
    esm_cur.setValue("NAME");
    while (esm_cur.getValue().exist)
    {
        esm_cur.setNextValue("NAME");
        pattern += esm_cur.getValue().text;  // ← .text not .content
    }
    return pattern;
}
```

---

### EsmReader — Bounds Checking (Requirements 19–20)

**Req 19–20 — malformed record/subrecord protection**

`splitFile` currently computes `rec_end = rec_beg + rec_size` without checking whether `rec_end` exceeds `content.size()`. If a record header contains a garbage size, `content.substr(rec_beg, rec_size)` throws `std::out_of_range`, which is caught and sets `is_loaded = false` — discarding all previously parsed records.

Fix in `splitFile`: validate each record size before advancing. If `rec_end > content.size()`, log the error, stop the loop, and set `is_loaded = true` (partial load — records parsed so far are retained).

Fix in `mainLoop`: before advancing `cur_pos += 8 + cur_size`, check that `cur_pos + 8 + cur_size <= rec->content.size()`. If not, break. Also check that `cur_size != 0` to prevent infinite loops.

```cpp
// splitFile guard:
if (rec_end > content.size())
{
    Tools::addLog("--> Warning: record at offset " + std::to_string(rec_beg)
                  + " declares size " + std::to_string(rec_size)
                  + " which exceeds file size. Stopping.\r\n");
    break;
}

// mainLoop guard:
if (cur_pos + 8 > rec->content.size()) break;
cur_size = ...;
if (cur_pos + 8 + cur_size > rec->content.size()) break;
if (cur_size == 0) break;
```

---

### DictReader — Performance (Requirement 13)

**Replace regex with sequential find**

The current `parseDict` uses `std::sregex_iterator` over the entire file content. For multi-megabyte dictionary files this is very slow. Replace with a `find`-based loop:

```cpp
void DictReader::parseDict(const std::string & content, const std::string & path)
{
    size_t pos = 0;
    while (true)
    {
        size_t rec_start = content.find("<record>", pos);
        if (rec_start == std::string::npos) break;

        size_t id_beg = content.find(sep[1], rec_start) + sep[1].size();
        size_t id_end = content.find(sep[2], id_beg);
        size_t key_beg = content.find(sep[3], id_end) + sep[3].size();
        size_t key_end = content.find(sep[4], key_beg);
        size_t val_beg = content.find(sep[5], key_end) + sep[5].size();
        size_t val_end = content.find(sep[6], val_beg);

        if (any are npos) break;

        const auto type = Tools::str2Type(content.substr(id_beg, id_end - id_beg));
        const auto key  = content.substr(key_beg, key_end - key_beg);
        const auto val  = content.substr(val_beg, val_end - val_beg);
        validateEntry({ key, val, type });
        counter_all++;
        pos = val_end + sep[6].size();
    }
    is_loaded = true;
}
```

Annotation blocks (`<!-- ... -->`) immediately following a `</val>` tag are extracted and stored in the `Annotations` chapter (Requirement 30).

---

### DictReader / Tools — Annotations Preservation (Requirement 30)

**Parse and store annotations**

After extracting each `<val>...</val>` block, check whether the next non-whitespace content is `<!--`. If so, extract the annotation text up to `-->` and insert it into `dict[Annotations][key]`.

**Merge annotations in DictMerger**

`mergeDict` already iterates all chapters including `Annotations`. The existing logic (insert if new, replace if different, skip if identical) applies correctly to the `Annotations` chapter without any special-casing.

**Write annotations in Tools::writeDict**

Already implemented — `writeDict` looks up `dict.at(Tools::RecType::Annotations).find(elem.first)` and writes the `<!-- ... -->` block. No change needed here.

---

### Tools — XML Escaping (Requirement 21)

**Escape `<val>` / `</val>` in dictionary values**

When `writeDict` writes a value, replace literal `<val>` with `&lt;val&gt;` and `</val>` with `&lt;/val&gt;`. When `parseDict` reads a value, reverse the substitution. Only these two sequences need escaping since they are the only delimiters that could appear inside a value field.

```cpp
// In writeDict, before writing elem.second:
std::string safe_val = elem.second;
replaceAll(safe_val, "<val>",  "&lt;val&gt;");
replaceAll(safe_val, "</val>", "&lt;/val&gt;");

// In parseDict, after extracting val:
replaceAll(val, "&lt;val&gt;",  "<val>");
replaceAll(val, "&lt;/val&gt;", "</val>");
```

---

### ScriptParser — Performance (Requirement 12)

**Cache compiled regex objects**

`convertLine` currently constructs a `std::regex` on every call. Move the per-keyword regex objects to static locals or class members initialized once.

```cpp
// In convertLine, replace:
std::regex r(s, std::regex::optimize);
// With a static cache:
static std::map<std::string, std::regex> regex_cache;
auto it = regex_cache.find(keyword);
if (it == regex_cache.end())
    it = regex_cache.emplace(keyword, std::regex("\\b" + keyword + "\\b", std::regex::optimize)).first;
const auto & r = it->second;
```

---

### ScriptParser — Performance (Requirement 14)

**Efficient case-insensitive fallback**

The current fallback in `findNewText` iterates the entire chapter linearly. Build a secondary `std::map<std::string, std::string>` keyed by uppercase version of the original key, constructed once per `DictMerger` instance (or lazily on first miss). Look up the uppercase version of `old_text` in O(log n).

Since `DictMerger` is const in `ScriptParser`, the uppercase index is built in `ScriptParser`'s constructor from `merger.getDict()`.

---

### Tools — String Copy Reduction (Requirement 11)

**Pass by const reference / return by const reference**

- `caseInsensitiveStringCmp`: parameters are already passed by value (needed for `transform`). No change.
- `eraseNullChars`, `trimCR`, `replaceNonReadableCharsWithDot`: already take by value or const ref appropriately.
- `DictReader::getName()`, `EsmReader::getName()`, `EsmReader::getTime()`: return `const auto &` — already correct.
- `EsmConverter::convertRecordContent`: review for intermediate `std::string` temporaries that can be replaced with in-place `erase`/`insert`.

---

### Tools — Logging (Requirement 15)

**Instance-based logging**

The static `log1`/`log2` strings in `Tools` are global mutable state. The minimal change that satisfies the requirement without restructuring the entire codebase: replace the static strings with a thread-local or a singleton logger object that can be reset between operations. For this project (single-threaded CLI), a simple approach is to keep the static strings but expose a `resetLog()` method so tests can clear state between runs.

A fuller approach — passing a logger reference through the call chain — is deferred as it requires touching every constructor signature. The minimal fix (reset capability) is implemented now.

---

### main.cpp — Exit Codes (Requirement 18)

**Return non-zero on error**

`main` currently always returns 0 (implicitly). `UserInterface` does not propagate errors. Fix:

1. Add a `bool hasError()` method to `UserInterface` that returns true if any `addLog` call with an error prefix occurred, or if any loaded file returned `is_loaded == false`.
2. Alternatively, have `Tools::addLog` set a static error flag when the message starts with `"--> Error"`.
3. `main` checks the flag and returns 1 if set.

The simplest approach: add `static bool error_flag` to `Tools`, set it in `addLog` when the entry starts with `"--> Error"`, expose `static bool hasError()`, and check it in `main`.

---

### includes.hpp — Boost Removal (Requirements 22, 26, 31, 32)

**Migrate to std::filesystem**

Replace:
```cpp
#include <boost/filesystem/operations.hpp>
```
With:
```cpp
#include <filesystem>
#include <chrono>
```

Replace all `boost::filesystem::last_write_time` calls in `esmreader.cpp` and `userinterface.cpp`:

```cpp
// EsmReader::setTime:
time = std::filesystem::last_write_time(path);
// Store as std::filesystem::file_time_type instead of std::time_t

// userinterface.cpp convertEsm:
std::filesystem::last_write_time(name, converter.getTime());

// userinterface.cpp createEsm:
std::filesystem::last_write_time(name, converter.getTime() + std::chrono::seconds(1));
```

Remove `#include <ctime>` from `includes.hpp` if unused after migration.

**vcxproj — C++ standard**

Add `<LanguageStandard>stdcpplatest</LanguageStandard>` to all four `<ClCompile>` property groups in both `yampt.vcxproj` and `yampt.tests.vcxproj`.

**vcxproj — Remove Boost NuGet**

Remove the `<Import>` and `<Target>` elements referencing `boost.1.71.0.0` and `boost_filesystem-vc142.1.71.0.0` from both vcxproj files. Remove `packages.config`.

**vcpkg.json**

Create `vcpkg.json` at the repository root. Initially empty of dependencies (Boost removed). If any other dependency is needed in future, it is declared here.

```json
{
  "name": "yampt",
  "version": "0.25",
  "dependencies": []
}
```

**Solution file — VS2022**

Update `yampt.sln` to set `VisualStudioVersion = 17.0.31903.59` and `MinimumVisualStudioVersion = 10.0.40219.1`. The `PlatformToolset` is already `v143` in all configurations — no change needed there.

---

### Header Files — Include Guards (Requirement 16)

All headers currently use `#ifndef` / `#define` / `#endif` guards. `includes.hpp` uses `#pragma once`. Standardize to `#pragma once` across all headers (simpler, no risk of name collision). Replace the `#ifndef` guards in `tools.hpp`, `dictreader.hpp`, `dictmerger.hpp`, `dictcreator.hpp`, `esmreader.hpp`, `esmconverter.hpp`, `scriptparser.hpp`, and `userinterface.hpp`.

---

### README — Documentation (Requirement 27)

Add a **Command Reference** section documenting all options:

- `--make-raw -f <file...>` — extract all translatable text
- `--make-base -f <src> <ref>` — create base dictionary from two language versions
- `--make-all -f <file...> -d <dict...>` — create full dictionary with translation status
- `--make-not -f <file...> -d <dict...>` — create dictionary of untranslated entries
- `--make-changed -f <file...> -d <dict...>` — create dictionary of changed entries
- `--merge -d <dict...> [-o <output>]` — merge dictionaries (last-wins)
- `--convert -f <file...> -d <dict...> [-s <suffix>]` — apply translation to plugins
- `--create -f <file...> -d <dict...>` — create plugin with only modified records
- `--add-hyperlinks` — append topic hyperlinks to dialog entries
- `--add-annotations` — append extended annotations
- `--windows-1250` — use Windows-1250 encoding
- `-s <suffix>` — output filename suffix for `--convert`

---

### Unit Tests (Requirements 23–25)

The test project (`yampt.tests`) already exists with Catch2 (`catch.hpp`) and stub test files. Tests are added to the existing files.

**DictReader tests** (`tests.dictreader.cpp`):
- Parse a minimal well-formed XML dictionary string → verify key/value pairs
- Parse a dictionary with a `<val>` literal inside a value → verify round-trip after escaping fix
- Verify CELL > 63 bytes is rejected
- Verify RNAM > 32 bytes is rejected
- Verify FNAM > 31 bytes is rejected
- Verify INFO > 1024 bytes is rejected
- Verify annotation blocks are parsed into the Annotations chapter

**DictMerger tests** (`tests.dictmerger.cpp`):
- Merge two non-overlapping dicts → all entries present
- Merge two dicts with same key, different value → last dict wins (value replaced)
- Merge two dicts with same key, same value → identical counter incremented
- Verify duplicate CELL values are detected

**EsmConverter tests** (`tests.esmconverter.cpp` — new file or `tests.esmreader.cpp`):
- Apply a single FNAM translation → verify subrecord content updated
- Verify record size field updated after content replacement
- Verify null-termination preserved for fixed-length fields

---

## File Change Summary

| File | Changes |
|------|---------|
| `yampt/tools.cpp` | Req 1, 2, 3, 4, 5, 11, 15, 18, 21 |
| `yampt/tools.hpp` | Req 15, 18 |
| `yampt/dictreader.cpp` | Req 13, 30 |
| `yampt/dictmerger.cpp` | Req 6, 30 |
| `yampt/dictcreator.cpp` | Req 9, 10, 28, 29 |
| `yampt/dictcreator.hpp` | Req 9 |
| `yampt/scriptparser.cpp` | Req 7, 8, 12, 14 |
| `yampt/esmreader.cpp` | Req 19, 20, 22 |
| `yampt/esmreader.hpp` | Req 22 |
| `yampt/userinterface.cpp` | Req 18, 22 |
| `yampt/includes.hpp` | Req 16, 22 |
| `yampt/tools.hpp` + all headers | Req 16 |
| `yampt/yampt.vcxproj` | Req 17, 22, 26, 31, 32 |
| `yampt.tests/yampt.tests.vcxproj` | Req 17, 22, 31, 32 |
| `yampt.sln` | Req 31 |
| `packages.config` | Req 22, 32 (remove) |
| `vcpkg.json` | Req 32 (new) |
| `README.md` | Req 27 |
| `yampt.tests/tests.dictreader.cpp` | Req 23 |
| `yampt.tests/tests.dictmerger.cpp` | Req 24 |
| `yampt.tests/tests.esmconverter.cpp` | Req 25 |
| `yampt/main.cpp` | Req 18 |
