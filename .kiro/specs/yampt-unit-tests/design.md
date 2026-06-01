# Design Document: yampt Unit Tests

## Overview

This document describes the design for expanding the unit test suite of the yampt project. The existing `yampt.tests/` project uses the Catch framework (header-only, `catch.hpp`) and already has partial coverage across five test files. The goal is to fill the gaps in coverage for all six modules — `Tools`, `EsmReader`, `DictReader`, `DictMerger`, `DictCreator`, and `ScriptParser` — without changing any production code.

Tests are built and run manually in Visual Studio. No CI pipeline or external tooling is involved. The test binary is configured with a custom `main()` in `tests.main.cpp` that delegates to `Catch::Session`.

### Research Summary

The codebase was read in full before writing this design. Key findings:

- `DictReader` always reads from a file path via `Tools::readFile`. There is no constructor that accepts in-memory content. Testing validation rules (CELL ≤ 63 bytes, RNAM ≤ 32 bytes, etc.) therefore requires writing minimal temporary dictionary files to disk, or using actual dictionary files that contain known entries. The simplest approach is to write small temp files inline in the test using `std::ofstream`, then construct `DictReader` from that path, then delete the file.
- `DictMerger` has a default constructor that creates an empty dict and an `addRecord` method that inserts directly. Merge-precedence tests (Requirement 9.2–9.4) can be exercised by constructing two `DictMerger` instances and calling `addRecord`, but the actual `mergeDict` path (which enforces first-wins precedence) is only triggered by the path-based constructor. To test merge precedence without real dict files, we write two minimal temp dict files and pass their paths to the `DictMerger(vector<string>)` constructor.
- `ScriptParser` is fully testable without file I/O: it takes a `DictMerger` reference, and `DictMerger::addRecord` populates the dict directly.
- `EsmReader` and `DictCreator` require actual ESM/ESP files. The existing tests already use `master/en/Morrowind.esm` as the working directory relative path.
- `Tools` static functions are pure and require no setup.

---

## Architecture

The test suite is a single Visual Studio project (`yampt.tests.vcxproj`) that compiles all `tests.*.cpp` files and links against the yampt source files. There is no separate test runner binary — the project produces one executable.

```
yampt.tests/
  catch.hpp               — Catch v1 header-only framework
  tests.main.cpp          — CATCH_CONFIG_RUNNER + main()
  tests.tools.cpp         — Tools static function tests  [u]
  tests.esmreader.cpp     — EsmReader integration tests  [i]
  tests.dictreader.cpp    — DictReader tests             [u]/[i]
  tests.dictmerger.cpp    — DictMerger tests             [u]   (new)
  tests.scriptparser.cpp  — ScriptParser tests           [u]
  tests.dictcreator.cpp   — DictCreator integration tests[i]
```

Tags follow the existing convention: `[u]` for unit tests (no file I/O), `[i]` for integration tests (reads files from disk).

---

## Components and Interfaces

### Tools (tests.tools.cpp)

All functions under test are `static` members of the `Tools` class. No setup or teardown is needed.

Functions with existing coverage:
- `convertStringByteArrayToUInt` — 4-byte and 1-byte cases, null-byte case
- `convertUIntToStringByteArray` — basic case
- `caseInsensitiveStringCmp` — true and false cases
- `eraseNullChars` — null-terminated and null-in-middle cases
- `trimCR` — trailing `\r` and mid-string `\r` cases
- `addAnnotations` — whole-word match, case-insensitive match, extended flag

Functions requiring new tests:
- `trimCR` — no `\r` present (returns unchanged)
- `replaceNonReadableCharsWithDot` — printable chars preserved, non-printable replaced
- `addAnnotations` — substring-of-longer-word (no match), no match at all
- `type2Str` / `str2Type` — round-trip for all defined types, Unknown for unrecognized string
- `getDialogType` — all 5 dialog type bytes (0–4)
- `getINDX` — zero-padded output for known values
- `isFNAM` — all 24 true IDs, representative false IDs
- `initializeDict` — all expected keys present, all chapters empty
- `getNumberOfElementsInDict` — zero for empty dict, excludes Annotations, correct total

### EsmReader (tests.esmreader.cpp)

Integration tests that load `master/en/Morrowind.esm`. Existing coverage is good. Missing:
- Loading a non-existent path → `isLoaded()` false
- Loading a file that does not start with `TES3` → `isLoaded()` false
- `setModified` → `getModifiedCount()` increments

### DictReader (tests.dictreader.cpp)

Currently empty (`TEST_CASE("parse dict")` with no body). All tests need to be written.

The approach: write a helper function `writeTempDict(const std::string& path, const std::string& content)` that creates a minimal dict file, and a corresponding cleanup. Each test case writes a small dict string, constructs a `DictReader`, checks the result, then removes the temp file.

Dict file format (from `Tools::writeDict` output):
```
<record>
	<_id>TYPE</_id>
	<key>KEY_TEXT</key>
	<val>VAL_TEXT</val>
</record>
```

### DictMerger (tests.dictmerger.cpp — new file)

New test file. Tests use `DictMerger()` default constructor + `addRecord` for unit tests, and the path-based constructor with temp dict files for merge-precedence tests.

### ScriptParser (tests.scriptparser.cpp)

Existing coverage handles `AddTopic`, `GetPCCell`, `AiFollowCell`, `PositionCell`, `Say`, and `MessageBox`. Missing:
- `end` keyword stops processing
- Trailing newline trimming behavior
- `MessageBox` with a quoted string matching a BNAM entry
- Multi-line script with multiple replacements

### DictCreator (tests.dictcreator.cpp)

Already fully covered by existing integration tests.

---

## Data Models

### Temp Dict File Format

For `DictReader` and `DictMerger` tests that need in-memory-style testing, a helper writes a minimal dict file:

```cpp
void writeTempDict(const std::string & path, const std::string & content)
{
    std::ofstream f(path, std::ios::binary);
    f << content;
}

void removeTempFile(const std::string & path)
{
    std::remove(path.c_str());
}
```

A minimal valid dict entry string:
```
<record>\r\n\t<_id>CELL</_id>\r\n\t<key>Balmora</key>\r\n\t<val>Balmora</val>\r\n</record>\r\n
```

### ScriptParser Test Pattern

Each ScriptParser test constructs a `DictMerger`, populates it with `addRecord`, then constructs a `ScriptParser` and checks `getNewScript()`:

```cpp
DictMerger merger;
merger.addRecord(Tools::RecType::DIAL, "Test", "Result");
ScriptParser parser(Tools::RecType::BNAM, merger, "", "", input_line, "");
REQUIRE(parser.getNewScript() == expected_line);
```

---

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system — essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

This feature does not use a property-based testing library. The properties below are expressed as universally quantified statements and are implemented as exhaustive or parameterized Catch test cases that iterate over representative input sets. This is appropriate because:

- The input spaces for the relevant functions are either finite (enum values, byte values 0–255) or can be covered by a representative sample without a PBT library.
- The project uses Catch v1 (header-only), which does not include a built-in property-based testing facility.
- Adding a PBT library (e.g. RapidCheck) would require changes to the Visual Studio project configuration, which is out of scope.

The properties are implemented as loops or parameterized `SECTION` blocks within standard `TEST_CASE` bodies.

---

### Property 1: Byte Conversion Round-Trip

*For any* unsigned 32-bit integer value, converting it to a 4-byte little-endian string and back shall produce the original value.

**Validates: Requirements 1.5**

Implementation: iterate over a representative set of values including 0, 1, 127, 128, 255, 256, 65535, 65536, 0x7FFFFFFF, 0xFFFFFFFF, and a selection of values with all four bytes non-zero.

---

### Property 2: Printable Characters Are Preserved by replaceNonReadableCharsWithDot

*For any* string composed entirely of printable ASCII characters (values 32–126), `replaceNonReadableCharsWithDot` shall return the string unchanged.

**Validates: Requirements 2.8**

Implementation: iterate over all printable ASCII characters individually, and test a string containing all of them concatenated.

---

### Property 3: Non-Printable Characters Are Replaced with Dot

*For any* string containing non-printable byte values (values 0–31 and 127–255 that are not `std::isprint`), each such byte shall be replaced with `'.'` in the output.

**Validates: Requirements 2.9**

Implementation: iterate over all byte values 0–255, build a single-character string for each, and verify that `replaceNonReadableCharsWithDot` returns `"."` for non-printable values and the original character for printable values.

---

### Property 4: RecType String Round-Trip

*For any* `RecType` value that has a defined string representation (all values except `Unknown`), `str2Type(type2Str(x))` shall equal `x`.

**Validates: Requirements 4.1**

Implementation: enumerate all `RecType` values with defined strings in a vector and verify the round-trip for each.

---

### Property 5: Fresh Dict Has All Chapters Empty

*For any* chapter in a freshly initialized `Tools::Dict`, the chapter shall contain zero entries.

**Validates: Requirements 5.2**

Implementation: call `initializeDict()`, iterate all chapters, verify `chapter.second.empty()` for each.

---

### Property 6: Element Count Equals Sum of Non-Annotations Entries

*For any* number of entries added to non-Annotations chapters of a `Tools::Dict`, `getNumberOfElementsInDict` shall return exactly that count.

**Validates: Requirements 5.5**

Implementation: add a known number of entries to several chapters (excluding Annotations), verify the count matches.

---

## Error Handling

### DictReader Validation Errors

`DictReader` silently rejects invalid entries (too-long values, unknown type strings, duplicate keys) and increments internal counters. Tests verify the rejection by checking that the key is absent from the dict after construction. There is no exception thrown — the `is_loaded` flag remains `true` as long as the file was parseable.

### EsmReader Load Errors

When a file does not exist or does not start with `TES3`, `EsmReader` sets `is_loaded = false`. Tests verify this by checking `isLoaded()` after construction.

### Temp File Cleanup

Tests that write temp files must remove them in all code paths. Use a RAII guard or explicit cleanup at the end of each `TEST_CASE`. Since Catch does not support fixtures in v1 in the same way as v2, cleanup is done manually at the end of each test case body.

---

## Testing Strategy

### Unit Tests (`[u]` tag)

Unit tests exercise pure functions and classes that require no file I/O:

- `Tools` static functions — all functions listed in Requirements 1–5
- `DictMerger` — `addRecord` and merge-precedence behavior (using temp dict files for the latter)
- `ScriptParser` — all keyword and message replacement paths, boundary handling

### Integration Tests (`[i]` tag)

Integration tests read actual files from disk:

- `EsmReader` — loads `master/en/Morrowind.esm`, tests record/subrecord access
- `DictReader` — loads temp dict files written inline in the test
- `DictCreator` — loads `master/en/Morrowind.esm`, `master/pl/Morrowind.esm`, `master/de/Morrowind.esm`

### Coverage Gaps Addressed

| Module | Existing Tests | New Tests Added |
|---|---|---|
| Tools — byte conversion | 4-byte, 1-byte, null-byte, round-trip | Round-trip over representative values |
| Tools — string utils | trimCR, eraseNullChars, caseInsensitive | trimCR no-\r, replaceNonReadable (all bytes) |
| Tools — annotations | whole-word, case-insensitive, extended | Substring-of-word (no match), no match |
| Tools — type conversion | none | type2Str/str2Type round-trip, getDialogType all 5, getINDX, isFNAM all 24+false |
| Tools — dict utils | none | initializeDict keys, empty chapters, getNumberOfElements |
| EsmReader | load, setKey, setValue, setNextValue | Non-existent path, non-TES3 file, setModified |
| DictReader | empty stub | All 8 validation rules via temp files |
| DictMerger | none | addRecord, merge precedence (first-wins), key-only-in-second, identical |
| ScriptParser | AddTopic, cell keywords, Say | end keyword, trailing newline, MessageBox, multi-line |
| DictCreator | RAW, BASE same-order, BASE diff-order | Already complete |

### Test File Organization

New tests are added to existing files where the module matches. One new file is created:

- `tests.dictmerger.cpp` — new file for `DictMerger` tests

The `tests.dictreader.cpp` file currently has an empty test case body; it will be filled with the DictReader validation tests.

### Property Test Configuration

Properties 1–6 are implemented as standard Catch `TEST_CASE` bodies with internal loops. Each loop iteration is a separate logical check. No minimum iteration count applies since the input spaces are finite or the representative sets are chosen to cover all boundary conditions.
