# Implementation Plan: yampt Unit Tests

## Overview

Expand the existing Catch-based test suite in `yampt.tests/` to cover all six modules. Tests are added to existing `.cpp` files where the module matches, and one new file (`tests.dictmerger.cpp`) is created. No production code is modified. All tests are built and run manually in Visual Studio.

## Tasks

- [x] 1. Expand tests.tools.cpp — string utilities and annotations
  - [x] 1.1 Add trimCR no-`\r` test and replaceNonReadableCharsWithDot tests
    - Add `TEST_CASE("trimCR, no CR present", "[u]")` verifying the string is returned unchanged
    - Add `TEST_CASE("replace non-readable chars with dot", "[u]")` with a printable-only string (preserved) and a string with non-printable bytes (replaced with `.`)
    - _Requirements: 2.7, 2.8, 2.9_

  - [x] 1.2 Write property tests for replaceNonReadableCharsWithDot (Properties 2 and 3)
    - **Property 2: Printable Characters Are Preserved**
    - Iterate all printable ASCII values (32–126), build single-char strings, verify output equals input
    - Also test a string containing all printable chars concatenated
    - **Property 3: Non-Printable Characters Are Replaced with Dot**
    - Iterate all byte values 0–255; for non-printable values verify output is `"."`; for printable values verify output equals input
    - **Validates: Requirements 2.8, 2.9**

  - [x] 1.3 Add addAnnotations edge-case tests
    - Add a `SECTION` for substring-of-longer-word: key `"clan"` in text `"clanfear"` — verify no match (returns `""`)
    - Add a `SECTION` for no match at all: key not present in text — verify returns `""`
    - _Requirements: 3.4, 3.5_

- [x] 2. Expand tests.tools.cpp — type conversion and dict utilities
  - [x] 2.1 Add type2Str / str2Type round-trip tests and getDialogType / getINDX / isFNAM tests
    - Add `TEST_CASE("type2Str and str2Type round-trip", "[u]")` iterating all RecType values with defined strings
    - Add `SECTION` for `str2Type` with an unrecognized string returning `RecType::Unknown`
    - Add `TEST_CASE("getDialogType all values", "[u]")` checking bytes 0–4 return `"T"`, `"V"`, `"G"`, `"P"`, `"J"`
    - Add `TEST_CASE("getINDX zero-padded output", "[u]")` checking a known 4-byte input produces a 3-digit zero-padded string
    - Add `TEST_CASE("isFNAM true IDs", "[u]")` iterating all 24 true record IDs
    - Add `TEST_CASE("isFNAM false IDs", "[u]")` checking `"CELL"`, `"INFO"`, `"DIAL"` return false
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6_

  - [x] 2.2 Write property test for RecType string round-trip (Property 4)
    - **Property 4: RecType String Round-Trip**
    - Enumerate all RecType values with defined strings in a vector; for each, verify `str2Type(type2Str(x)) == x`
    - **Validates: Requirements 4.1**

  - [x] 2.3 Add initializeDict and getNumberOfElementsInDict tests
    - Add `TEST_CASE("initializeDict has all expected keys", "[u]")` verifying all RecType keys including `Annotations` are present
    - Add `TEST_CASE("initializeDict all chapters empty", "[u]")` iterating all chapters and checking `.empty()`
    - Add `TEST_CASE("getNumberOfElementsInDict zero for empty dict", "[u]")`
    - Add `TEST_CASE("getNumberOfElementsInDict excludes Annotations", "[u]")` inserting entries into `Annotations` only and verifying count is 0
    - Add `TEST_CASE("getNumberOfElementsInDict correct total", "[u]")` inserting known entries into several non-Annotations chapters and verifying the sum
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5_

  - [x] 2.4 Write property tests for dict utilities (Properties 5 and 6)
    - **Property 5: Fresh Dict Has All Chapters Empty**
    - Call `initializeDict()`, iterate all chapters, verify `chapter.second.empty()` for each
    - **Property 6: Element Count Equals Sum of Non-Annotations Entries**
    - Add a known number of entries to several non-Annotations chapters; verify `getNumberOfElementsInDict` returns exactly that count
    - **Validates: Requirements 5.2, 5.5**

- [x] 3. Expand tests.tools.cpp — byte conversion round-trip
  - [x] 3.1 Add byte conversion round-trip property test (Property 1)
    - Add `TEST_CASE("byte conversion round-trip", "[u]")` iterating representative values: 0, 1, 127, 128, 255, 256, 65535, 65536, 0x7FFFFFFF, 0xFFFFFFFF, and several values with all four bytes non-zero
    - For each value, verify `convertStringByteArrayToUInt(convertUIntToStringByteArray(x)) == x`
    - _Requirements: 1.5_

- [x] 4. Checkpoint — Ensure all tests.tools.cpp tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 5. Expand tests.esmreader.cpp — load error and setModified
  - [x] 5.1 Add non-existent path and non-TES3 file load tests
    - Add `TEST_CASE("loading non-existent file", "[i]")` constructing `EsmReader` with a bogus path and verifying `isLoaded() == false`
    - Add `TEST_CASE("loading non-TES3 file", "[i]")` writing a small temp file without `TES3` magic bytes, constructing `EsmReader` from it, verifying `isLoaded() == false`, then removing the temp file
    - _Requirements: 6.2, 6.3_

  - [x] 5.2 Add setModified / getModifiedCount test
    - Add `TEST_CASE("setModified increments count", "[i]")` loading `master/en/Morrowind.esm`, calling `setModified` on a record, and verifying `getModifiedCount()` increments by one
    - _Requirements: 7.6_

- [x] 6. Fill tests.dictreader.cpp — all DictReader validation rules
  - [x] 6.1 Add helper functions and well-formed dict test
    - Add `writeTempDict(path, content)` and `removeTempFile(path)` helper functions at the top of the file
    - Replace the empty `TEST_CASE("parse dict")` body with a well-formed CELL entry test: write a temp dict, construct `DictReader`, verify `isLoaded() == true` and the key-value pair is present, remove temp file
    - _Requirements: 8.1_

  - [x] 6.2 Add CELL, RNAM, FNAM, and INFO length validation tests
    - Add `TEST_CASE("DictReader rejects CELL value > 63 bytes", "[i]")` with a 64-byte value; verify key absent from dict
    - Add `TEST_CASE("DictReader rejects RNAM value > 32 bytes", "[i]")` with a 33-byte value; verify key absent
    - Add `TEST_CASE("DictReader rejects FNAM value > 31 bytes", "[i]")` with a 32-byte value; verify key absent
    - Add `TEST_CASE("DictReader rejects INFO value > 1024 bytes", "[i]")` with a 1025-byte value; verify key absent
    - Add `TEST_CASE("DictReader accepts INFO value 513-1024 bytes", "[i]")` with a 513-byte value; verify key present
    - _Requirements: 8.2, 8.3, 8.4, 8.5, 8.6_

  - [x] 6.3 Add duplicate key and unknown type tests
    - Add `TEST_CASE("DictReader rejects duplicate key", "[i]")` with two entries sharing the same key and type; verify only one entry stored
    - Add `TEST_CASE("DictReader rejects unknown type string", "[i]")` with an unrecognized `<_id>` value; verify entry not inserted
    - _Requirements: 8.7, 8.8_

- [x] 7. Create tests.dictmerger.cpp — all DictMerger tests
  - [x] 7.1 Create tests.dictmerger.cpp with addRecord and merge-precedence tests
    - Create `yampt.tests/tests.dictmerger.cpp` with the standard includes (`catch.hpp`, `tools.hpp`, `dictmerger.hpp`)
    - Add `TEST_CASE("DictMerger addRecord inserts entry", "[u]")` using the default constructor and `addRecord`; verify entry appears in the correct chapter via `getDict()`
    - Add `TEST_CASE("DictMerger merge first-wins precedence", "[i]")` writing two temp dict files with the same key but different values; construct `DictMerger` with both paths; verify first dict's value is retained
    - Add `TEST_CASE("DictMerger key only in second dict", "[i]")` writing two temp dicts where a key exists only in the second; verify the key is present in the merged result
    - Add `TEST_CASE("DictMerger identical values", "[i]")` writing two temp dicts with the same key and identical values; verify the entry appears exactly once
    - _Requirements: 9.1, 9.2, 9.3, 9.4_

- [x] 8. Checkpoint — Ensure all DictReader and DictMerger tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 9. Expand tests.scriptparser.cpp — missing keyword and message paths
  - [x] 9.1 Add AddTopic no-match and comment tests
    - Add a `SECTION` for `AddTopic` with an argument not present in the merger dict — verify line unchanged
    - Verify the existing comment case (`; AddTopic`) and `Begin AddTopicScript` case are already covered; add any missing assertions
    - _Requirements: 10.3, 10.4, 10.8_

  - [x] 9.2 Add MessageBox replacement test
    - Add `TEST_CASE("script parser, MessageBox replacement", "[u]")` with a `MessageBox` line whose quoted string matches a BNAM entry; verify the string is replaced
    - _Requirements: 11.2_

  - [x] 9.3 Add end keyword and multi-line script tests
    - Add `TEST_CASE("script parser, end keyword stops processing", "[u]")` with a script containing `end` followed by an `AddTopic` line; verify the `AddTopic` line is not modified
    - Add `TEST_CASE("script parser, multi-line script", "[u]")` with a two-line script where each line has a replaceable keyword; verify both lines are processed independently and the full output is correct
    - _Requirements: 10.1, 11.4, 12.1_

  - [x] 9.4 Add trailing newline boundary tests
    - Add `TEST_CASE("script parser, trailing CRLF not appended", "[u]")` with an input ending in `\r\n`; verify the output does not have an extra `\r\n` appended beyond the input
    - Add a `SECTION` for input not ending in `\r\n`; verify output length is not shorter than input length
    - _Requirements: 12.2, 12.3_

- [x] 10. Final checkpoint — Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Properties 1–6 are implemented as loops within standard `TEST_CASE` bodies — no PBT library is added
- Temp dict files written in DictReader and DictMerger tests must be removed at the end of each test case
- The new `tests.dictmerger.cpp` file must be added to `yampt.tests.vcxproj` manually in Visual Studio

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "2.1", "3.1", "5.1", "5.2", "7.1"] },
    { "id": 1, "tasks": ["1.2", "1.3", "2.2", "2.3", "6.1"] },
    { "id": 2, "tasks": ["2.4", "6.2", "6.3", "9.1"] },
    { "id": 3, "tasks": ["9.2", "9.3", "9.4"] }
  ]
}
```
