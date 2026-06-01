# Implementation Tasks

## Task Group 1: Tools Bug Fixes

- [ ] 1. Fix Tools::readFile reserve size
  - In `yampt/tools.cpp`, add `file.seekg(0, std::ios::end)` before `file.tellg()` to get the actual file size, then `file.seekg(0, std::ios::beg)` before the read loop
  - Remove the current `size_t size = file.tellg();` line and replace with the seek-based approach
  - Verify `content.reserve(static_cast<size_t>(file.tellg()))` is called after seeking to end
  - **Requirements**: Req 1

- [ ] 2. Fix Tools::convertStringByteArrayToUInt buffer initialization
  - In `yampt/tools.cpp`, change `char buffer[4];` to `char buffer[4] = {};` to zero-initialize
  - Change `str.copy(buffer, 4)` to `str.copy(buffer, str.size())` to only copy available bytes
  - **Requirements**: Req 2

- [ ] 3. Fix Tools::trimCR to only remove trailing carriage return
  - In `yampt/tools.cpp`, replace the `str.find('\r')` check with `!str.empty() && str.back() == '\r'`
  - Replace `str.erase(str.size() - 1)` with `str.pop_back()`
  - **Requirements**: Req 3

- [ ] 4. Fix Tools::replaceNonReadableCharsWithDot signed char handling
  - In `yampt/tools.cpp`, replace the `static_cast<int>` range check with `std::isprint(static_cast<unsigned char>(str[i]))`
  - Remove the redundant `>= 0 && <= 255` bounds check
  - **Requirements**: Req 4

- [ ] 5. Fix Tools::addAnnotations to find all occurrences
  - In `yampt/tools.cpp`, replace the single `source_lc.find(key_text_lc)` call with a `while(true)` loop that tracks `search_from` and advances past non-matching positions
  - When a position is preceded by an alpha char, set `search_from = pos + 1` and `continue`; otherwise append the annotation and `break`
  - **Requirements**: Req 5

## Task Group 2: DictMerger Bug Fix

- [ ] 6. Fix DictMerger::mergeDict to actually replace records
  - In `yampt/dictmerger.cpp`, in the `else if (search->second != elem.second)` branch, add `search->second = elem.second;` before the log call
  - **Requirements**: Req 6

## Task Group 3: ScriptParser Bug Fixes

- [ ] 7. Fix ScriptParser end keyword detection parentheses
  - In `yampt/scriptparser.cpp`, in `convertScript()`, wrap the second condition in explicit parentheses: `(line_lc.size() > 3 && line_lc.substr(0, 4) == "end ")`
  - **Requirements**: Req 7

- [ ] 8. Fix ScriptParser getpccell expression size calculation
  - In `yampt/scriptparser.cpp`, in `convertTextInCompiled()`, replace the hardcoded `+ 5` suffix with a dynamic scan: advance `expr_end` from `pos_c + new_text.size()` forward while `new_SCDT[expr_end] != '\0'` and `expr_end < new_SCDT.size()`
  - **Requirements**: Req 8

## Task Group 4: DictCreator Bug Fixes

- [ ] 9. Fix DictCreator dangling reference in RAW and BASE constructors
  - In `yampt/dictcreator.hpp`, add a `DictMerger owned_merger;` member field before the existing `const DictMerger & merger;` member
  - In `yampt/dictcreator.cpp`, update the RAW constructor initializer list to use `merger(owned_merger)` instead of `merger(DictMerger())`
  - Update the BASE constructor initializer list to use `merger(owned_merger)` instead of `merger(DictMerger())`
  - **Requirements**: Req 9

- [ ] 10. Fix DictCreator::makeScriptMessages keyword false positives
  - In `yampt/dictcreator.cpp`, replace the `script_text.find(keyword)` substring search with a line-by-line loop using `std::regex` with `\\b` word boundaries, matching the approach in `ScriptParser::convertLine`
  - **Requirements**: Req 10

## Task Group 5: EsmReader Bounds Checking

- [ ] 11. Add bounds checking to EsmReader::splitFile
  - In `yampt/esmreader.cpp`, after computing `rec_end = rec_beg + rec_size`, add a guard: if `rec_end > content.size()`, log a warning with the offset and declared size, `break` out of the loop, and set `is_loaded = true` (partial load)
  - **Requirements**: Req 19

- [ ] 12. Add bounds checking to EsmReader::mainLoop
  - In `yampt/esmreader.cpp`, at the top of the `while` loop body, add: if `cur_pos + 8 > rec->content.size()` then `break`
  - After reading `cur_size`, add: if `cur_pos + 8 + cur_size > rec->content.size()` then `break`
  - Add: if `cur_size == 0` then `break` to prevent infinite loops
  - **Requirements**: Req 20

## Task Group 6: DictReader Performance and Annotations

- [ ] 13. Replace DictReader regex-based parsing with sequential find
  - In `yampt/dictreader.cpp`, replace the `std::sregex_iterator` loop in `parseDict()` with a `while(true)` loop using `content.find()` to locate `<record>`, then sequentially find `sep[1]`/`sep[2]` (id), `sep[3]`/`sep[4]` (key), `sep[5]`/`sep[6]` (val)
  - Break out of the loop if any `find()` returns `std::string::npos`
  - Advance `pos` to `val_end + sep[6].size()` after each record
  - **Requirements**: Req 13

- [ ] 14. Add annotation parsing to DictReader::parseDict
  - In `yampt/dictreader.cpp`, after extracting each `val_end`, check if the content immediately following `</val>` (skipping whitespace) starts with `<!--`
  - If so, extract the text up to `-->` and insert it into `dict[Tools::RecType::Annotations][key]`
  - **Requirements**: Req 30

## Task Group 7: XML Escaping

- [ ] 15. Add XML escaping for val tag sequences in Tools::writeDict
  - In `yampt/tools.cpp`, in `writeDict()`, before writing `elem.second`, create a copy and replace `<val>` with `&lt;val&gt;` and `</val>` with `&lt;/val&gt;`
  - In `yampt/dictreader.cpp`, in `parseDict()`, after extracting `val`, replace `&lt;val&gt;` back to `<val>` and `&lt;/val&gt;` back to `</val>`
  - **Requirements**: Req 21

## Task Group 8: ScriptParser Performance

- [ ] 16. Cache compiled regex objects in ScriptParser::convertLine
  - In `yampt/scriptparser.cpp`, in `convertLine(keyword, pos_in_expression, text_type)`, replace the per-call `std::regex r(s, std::regex::optimize)` construction with a `static std::map<std::string, std::regex> regex_cache` lookup; insert on first miss
  - **Requirements**: Req 12

- [ ] 17. Optimize ScriptParser::findNewText case-insensitive fallback
  - In `yampt/scriptparser.hpp`, add a `std::map<std::string, std::string> upper_index` member per chapter type, built lazily or in the constructor from `merger->getDict()`
  - In `yampt/scriptparser.cpp`, in `findNewText()`, replace the linear `for` loop fallback with an O(log n) lookup in the uppercase index
  - **Requirements**: Req 14

## Task Group 9: DictCreator Feature Fixes

- [ ] 18. Fix DictCreator::addHyperlinks for Greeting dialog type
  - In `yampt/dictcreator.cpp`, in `addHyperlinks()`, change the condition that skips hyperlink annotation from checking for non-"T" types to checking only for "V" (Voice) type
  - **Requirements**: Req 28

- [ ] 19. Fix DictCreator::makeDictCELL_Unordered_Pattern to use .text not .content
  - In `yampt/dictcreator.cpp`, in `makeDictCELL_Unordered_Pattern()`, change `pattern += esm_cur.getValue().content` (for the NAME subrecord) to `pattern += esm_cur.getValue().text`
  - **Requirements**: Req 29

## Task Group 10: Logging and Error Codes

- [ ] 20. Add error flag and resetLog to Tools
  - In `yampt/tools.hpp`, add `static bool error_flag;` and `static bool hasError()` and `static void resetLog()` declarations
  - In `yampt/tools.cpp`, define `bool Tools::error_flag = false;`, implement `hasError()` returning `error_flag`, implement `resetLog()` clearing `log1`, `log2`, and `error_flag`
  - In `addLog()`, set `error_flag = true` when `entry` starts with `"--> Error"`
  - **Requirements**: Req 15, Req 18

- [ ] 21. Return non-zero exit code from main on error
  - In `yampt/main.cpp`, change the `catch(...)` block to call `Tools::addLog("UNKNOWN error!\r\n")`
  - After `Tools::writeText(...)`, return `Tools::hasError() ? 1 : 0`
  - **Requirements**: Req 18

## Task Group 11: Boost Removal and std::filesystem Migration

- [ ] 22. Replace Boost.Filesystem with std::filesystem in includes.hpp and esmreader
  - In `yampt/includes.hpp`, replace `#include <boost/filesystem/operations.hpp>` with `#include <filesystem>` and `#include <chrono>`, and remove `#include <ctime>`
  - In `yampt/esmreader.hpp`, change the `std::time_t time` member to `std::filesystem::file_time_type time`
  - In `yampt/esmreader.cpp`, replace `boost::filesystem::last_write_time(path)` with `std::filesystem::last_write_time(path)` in `setTime()`
  - **Requirements**: Req 22

- [ ] 23. Replace Boost.Filesystem calls in userinterface.cpp
  - In `yampt/userinterface.cpp`, replace `boost::filesystem::last_write_time(name, converter.getTime())` with `std::filesystem::last_write_time(name, converter.getTime())`
  - Replace `boost::filesystem::last_write_time(name, converter.getTime() + 1)` with `std::filesystem::last_write_time(name, converter.getTime() + std::chrono::seconds(1))`
  - **Requirements**: Req 22

## Task Group 12: Project Configuration

- [ ] 24. Standardize include guards to #pragma once across all headers
  - In `yampt/tools.hpp`, `yampt/dictreader.hpp`, `yampt/dictmerger.hpp`, `yampt/dictcreator.hpp`, `yampt/esmreader.hpp`, `yampt/esmconverter.hpp`, `yampt/scriptparser.hpp`, `yampt/userinterface.hpp`: replace `#ifndef`/`#define`/`#endif` guards with `#pragma once`
  - **Requirements**: Req 16

- [ ] 25. Pin C++23 standard and remove Boost NuGet from yampt.vcxproj
  - In `yampt/yampt.vcxproj`, add `<LanguageStandard>stdcpplatest</LanguageStandard>` inside each of the four `<ClCompile>` `<ItemDefinitionGroup>` blocks (Debug|Win32, Debug|x64, Release|Win32, Release|x64)
  - Remove the `<Import>` elements for `boost.1.71.0.0` and `boost_filesystem-vc142.1.71.0.0` from the `<ImportGroup Label="ExtensionTargets">` section
  - Remove the `<Target Name="EnsureNuGetPackageBuildImports">` block entirely
  - Remove the `<None Include="..\packages.config" />` item
  - **Requirements**: Req 17, Req 22, Req 26, Req 31, Req 32

- [ ] 26. Pin C++23 standard and remove Boost NuGet from yampt.tests.vcxproj
  - In `yampt.tests/yampt.tests.vcxproj`, add `<LanguageStandard>stdcpplatest</LanguageStandard>` inside each of the four `<ClCompile>` `<ItemDefinitionGroup>` blocks
  - Remove any Boost NuGet `<Import>` and `<Target>` elements
  - **Requirements**: Req 17, Req 22, Req 31, Req 32

- [ ] 27. Update solution file to VS2022 and create vcpkg.json
  - In `yampt.sln`, set `VisualStudioVersion = 17.0.31903.59` and `MinimumVisualStudioVersion = 10.0.40219.1`
  - Delete `packages.config` from the repository root
  - Create `vcpkg.json` at the repository root with `{"name": "yampt", "version": "0.25", "dependencies": []}`
  - **Requirements**: Req 31, Req 32

## Task Group 13: README Documentation

- [ ] 28. Document all command-line options in README.md
  - In `README.md`, add a **Command Reference** section listing all commands (`--make-raw`, `--make-base`, `--make-all`, `--make-not`, `--make-changed`, `--merge`, `--convert`, `--create`) with their required `-f`/`-d` arguments and usage examples
  - Document `--add-hyperlinks`, `--add-annotations`, `--windows-1250`, and `-s <suffix>` options with descriptions
  - **Requirements**: Req 27

## Task Group 14: Unit Tests

- [ ] 29. Add DictReader unit tests
  - In `yampt.tests/tests.dictreader.cpp`, add Catch2 test cases that:
    - Parse a minimal well-formed XML dictionary string and verify key/value pairs are extracted correctly
    - Parse a dictionary containing a literal `<val>` in a value and verify round-trip correctness after the escaping fix
    - Verify that a CELL entry with val > 63 bytes is rejected (counter_invalid incremented, not inserted)
    - Verify that an RNAM entry with val > 32 bytes is rejected
    - Verify that an FNAM entry with val > 31 bytes is rejected
    - Verify that an INFO entry with val > 1024 bytes is rejected
    - Verify that annotation blocks following a record are stored in the Annotations chapter
  - **Requirements**: Req 23

- [ ] 30. Add DictMerger unit tests
  - In `yampt.tests/tests.dictmerger.cpp`, add Catch2 test cases that:
    - Merge two non-overlapping dictionaries and verify all entries are present in the result
    - Merge two dictionaries with the same key but different values and verify the later dictionary's value wins (last-wins semantics)
    - Merge two dictionaries with the same key and identical values and verify the identical counter is incremented
    - Verify that duplicate CELL values in the merged result are detected and logged
  - **Requirements**: Req 24

- [ ] 31. Add EsmConverter unit tests
  - In `yampt.tests/tests.esmconverter.cpp` (create if not present), add Catch2 test cases that:
    - Apply a single FNAM translation to a minimal synthetic TES3 record and verify the subrecord content is updated to the translated value
    - Verify the record size field is recalculated correctly after content replacement
    - Verify null-termination is preserved correctly for fixed-length fields after replacement
  - **Requirements**: Req 25
