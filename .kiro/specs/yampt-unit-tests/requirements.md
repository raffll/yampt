# Requirements Document

## Introduction

This feature adds comprehensive unit tests to the yampt (Yet Another Morrowind Plugin Translator) C++ project. The existing test project at `yampt/yampt.tests/` uses the Catch framework and already has partial coverage. The goal is to expand coverage to as many functions and code paths as possible across all source modules: `Tools`, `EsmReader`, `DictReader`, `DictMerger`, `DictCreator`, and `ScriptParser`. Tests are built and run manually in Visual Studio — no CI pipeline or external tooling is involved.

## Glossary

- **Catch**: The header-only C++ unit testing framework already used by the project (`catch.hpp`).
- **DictCreator**: Class that reads an ESM/ESP file and produces a translation dictionary.
- **DictMerger**: Class that merges one or more `DictReader` dictionaries into a single working dictionary.
- **DictReader**: Class that parses a yampt dictionary text file into an in-memory `Tools::Dict`.
- **EsmReader**: Class that parses a binary TES3 ESM/ESP plugin file into records and subrecords.
- **ScriptParser**: Class that rewrites Morrowind script text by substituting translated cell names, dialog topics, and messages.
- **Tools**: Static utility class providing shared types, constants, and pure helper functions.
- **Unit test**: A `[u]`-tagged Catch test case that exercises a single function in isolation with no file I/O.
- **Integration test**: An `[i]`-tagged Catch test case that reads actual ESM/dictionary files from disk.
- **Dict**: `Tools::Dict` — a `std::map<RecType, Chapter>` holding all translation entries.
- **Chapter**: `Tools::Chapter` — a `std::map<std::string, std::string>` for one record type.
- **RecType**: Enum class in `Tools` identifying the type of a translation record (CELL, DIAL, INFO, etc.).
- **SubRecord**: Nested struct in `EsmReader` representing a parsed subrecord with id, content, text, pos, size, and exist fields.

---

## Requirements

### Requirement 1: Tools — Byte Conversion Utilities

**User Story:** As a developer, I want unit tests for `Tools::convertStringByteArrayToUInt` and `Tools::convertUIntToStringByteArray`, so that I can verify the binary serialization logic that underpins all ESM record parsing.

#### Acceptance Criteria

1. THE Test_Suite SHALL verify that `convertStringByteArrayToUInt` returns the correct little-endian unsigned integer for a 4-byte input string.
2. THE Test_Suite SHALL verify that `convertStringByteArrayToUInt` returns the correct value for a 1-byte input string.
3. THE Test_Suite SHALL verify that `convertStringByteArrayToUInt` correctly handles 4-byte strings containing embedded null bytes.
4. THE Test_Suite SHALL verify that `convertUIntToStringByteArray` returns the correct 4-byte little-endian string for a given unsigned integer.
5. FOR ALL unsigned integers x in the representable range, `convertStringByteArrayToUInt(convertUIntToStringByteArray(x))` SHALL equal x (round-trip property).

---

### Requirement 2: Tools — String Manipulation Utilities

**User Story:** As a developer, I want unit tests for the string helper functions in `Tools`, so that I can confirm text processing behaves correctly for all edge cases encountered in ESM data.

#### Acceptance Criteria

1. THE Test_Suite SHALL verify that `caseInsensitiveStringCmp` returns true when two strings differ only in case.
2. THE Test_Suite SHALL verify that `caseInsensitiveStringCmp` returns false when two strings have different content.
3. THE Test_Suite SHALL verify that `eraseNullChars` removes all characters from the first null byte onward.
4. THE Test_Suite SHALL verify that `eraseNullChars` returns the original string unchanged when no null byte is present.
5. THE Test_Suite SHALL verify that `trimCR` removes the trailing `\r` character from a string ending with `\r`.
6. THE Test_Suite SHALL verify that `trimCR` removes only the last `\r` when a string contains `\r` in the middle and at the end.
7. THE Test_Suite SHALL verify that `trimCR` returns the original string unchanged when no `\r` is present.
8. THE Test_Suite SHALL verify that `replaceNonReadableCharsWithDot` preserves all printable ASCII characters.
9. THE Test_Suite SHALL verify that `replaceNonReadableCharsWithDot` replaces non-printable bytes with the `.` character.

---

### Requirement 3: Tools — Annotation and Glossary Utilities

**User Story:** As a developer, I want unit tests for `Tools::addAnnotations`, so that I can confirm that dialog topic glossary entries are correctly appended to translated text.

#### Acceptance Criteria

1. WHEN a glossary key appears as a whole word in the source text, THE Test_Suite SHALL verify that `addAnnotations` appends the translated value in brackets.
2. WHEN a glossary key appears in the source text in a different case, THE Test_Suite SHALL verify that `addAnnotations` still matches it (case-insensitive).
3. WHEN the `extended` flag is true, THE Test_Suite SHALL verify that `addAnnotations` appends both the key and value in the format `[key -> value]`.
4. WHEN a glossary key appears as a substring of a longer word (preceded by an alphabetic character), THE Test_Suite SHALL verify that `addAnnotations` does not produce a match.
5. WHEN no glossary key matches the source text, THE Test_Suite SHALL verify that `addAnnotations` returns an empty string.

---

### Requirement 4: Tools — Type Conversion and Classification

**User Story:** As a developer, I want unit tests for `Tools::type2Str`, `Tools::str2Type`, `Tools::getDialogType`, `Tools::getINDX`, and `Tools::isFNAM`, so that I can confirm record type identification is correct.

#### Acceptance Criteria

1. FOR ALL RecType values that have a defined string representation, `str2Type(type2Str(x))` SHALL equal x (round-trip property).
2. THE Test_Suite SHALL verify that `str2Type` returns `RecType::Unknown` for an unrecognized string.
3. THE Test_Suite SHALL verify that `getDialogType` returns `"T"` for a byte value of 0, `"V"` for 1, `"G"` for 2, `"P"` for 3, and `"J"` for 4.
4. THE Test_Suite SHALL verify that `getINDX` returns a zero-padded 3-digit decimal string for a 4-byte little-endian integer input.
5. THE Test_Suite SHALL verify that `isFNAM` returns true for each of the 24 record type IDs that carry an FNAM subrecord.
6. THE Test_Suite SHALL verify that `isFNAM` returns false for record type IDs that do not carry an FNAM subrecord (e.g., `"CELL"`, `"INFO"`, `"DIAL"`).

---

### Requirement 5: Tools — Dictionary Utilities

**User Story:** As a developer, I want unit tests for `Tools::initializeDict` and `Tools::getNumberOfElementsInDict`, so that I can confirm dictionary initialization and counting are correct.

#### Acceptance Criteria

1. THE Test_Suite SHALL verify that `initializeDict` returns a `Dict` containing entries for all expected `RecType` keys including `Annotations`.
2. THE Test_Suite SHALL verify that all chapters in a freshly initialized `Dict` are empty.
3. THE Test_Suite SHALL verify that `getNumberOfElementsInDict` returns 0 for an empty initialized dict.
4. THE Test_Suite SHALL verify that `getNumberOfElementsInDict` excludes the `Annotations` chapter from its count.
5. WHEN entries are added to multiple chapters, THE Test_Suite SHALL verify that `getNumberOfElementsInDict` returns the correct total excluding `Annotations`.

---

### Requirement 6: EsmReader — File Loading

**User Story:** As a developer, I want integration tests for `EsmReader` file loading, so that I can confirm the parser correctly handles valid and invalid inputs.

#### Acceptance Criteria

1. WHEN a valid TES3 ESM file is loaded, THE Test_Suite SHALL verify that `isLoaded()` returns true and `getName()` fields are populated correctly.
2. WHEN a non-existent file path is provided, THE Test_Suite SHALL verify that `isLoaded()` returns false.
3. WHEN a file that does not start with the `TES3` magic bytes is loaded, THE Test_Suite SHALL verify that `isLoaded()` returns false.

---

### Requirement 7: EsmReader — Record and Subrecord Access

**User Story:** As a developer, I want integration tests for `EsmReader` record selection and subrecord lookup, so that I can confirm the binary parsing logic handles all cases correctly.

#### Acceptance Criteria

1. WHEN `setKey` is called with a subrecord ID that does not exist in the selected record, THE Test_Suite SHALL verify that `getKey().exist` is false and `getKey().text` is `"N/A"`.
2. WHEN `setKey` is called with a valid subrecord ID, THE Test_Suite SHALL verify that `getKey().exist` is true and `getKey().text` contains the correct string.
3. WHEN `setValue` is called with a subrecord ID that does not exist, THE Test_Suite SHALL verify that `getValue().exist` is false, `getValue().text` is `"N/A"`, and `getValue().pos` equals the record content size.
4. WHEN `setValue` is called with a valid subrecord ID, THE Test_Suite SHALL verify that `getValue().exist` is true, `getValue().text` is correct, and `getValue().pos` and `getValue().size` reflect the correct byte offsets.
5. WHEN `setNextValue` is called after a successful `setValue`, THE Test_Suite SHALL verify that the next occurrence of the same subrecord ID is found with updated text, pos, and size.
6. WHEN `setModified` is called on a record, THE Test_Suite SHALL verify that `getModifiedCount()` increments by one.

---

### Requirement 8: DictReader — Dictionary Parsing

**User Story:** As a developer, I want unit tests for `DictReader` that exercise the parsing logic using in-memory dictionary content, so that I can confirm all validation rules are enforced without requiring dictionary files on disk.

#### Acceptance Criteria

1. WHEN a well-formed dictionary string is parsed, THE Test_Suite SHALL verify that `isLoaded()` returns true and the correct key-value pairs appear in the dict.
2. WHEN a dictionary string contains a CELL entry whose value exceeds 63 bytes, THE Test_Suite SHALL verify that the entry is rejected and does not appear in the dict.
3. WHEN a dictionary string contains an RNAM entry whose value exceeds 32 bytes, THE Test_Suite SHALL verify that the entry is rejected and does not appear in the dict.
4. WHEN a dictionary string contains an FNAM entry whose value exceeds 31 bytes, THE Test_Suite SHALL verify that the entry is rejected and does not appear in the dict.
5. WHEN a dictionary string contains an INFO entry whose value exceeds 1024 bytes, THE Test_Suite SHALL verify that the entry is rejected and does not appear in the dict.
6. WHEN a dictionary string contains an INFO entry whose value is between 513 and 1024 bytes, THE Test_Suite SHALL verify that the entry is accepted and appears in the dict.
7. WHEN a dictionary string contains two entries with the same key and type, THE Test_Suite SHALL verify that only the first entry is stored and the second is counted as doubled.
8. WHEN a dictionary string contains an entry with an unrecognized type string, THE Test_Suite SHALL verify that the entry is not inserted and is counted as invalid.

---

### Requirement 9: DictMerger — Dictionary Merging

**User Story:** As a developer, I want unit tests for `DictMerger`, so that I can confirm that merging multiple dictionaries follows the correct precedence rules.

#### Acceptance Criteria

1. WHEN `addRecord` is called, THE Test_Suite SHALL verify that the entry appears in the correct chapter of the merged dict.
2. WHEN two dictionaries share the same key with different values, THE Test_Suite SHALL verify that the first dictionary's value is retained in the merged result.
3. WHEN a key exists only in the second dictionary, THE Test_Suite SHALL verify that the key is added to the merged result.
4. WHEN two dictionaries share the same key with identical values, THE Test_Suite SHALL verify that the entry appears exactly once in the merged result.

---

### Requirement 10: ScriptParser — Keyword-Based Topic and Cell Replacement

**User Story:** As a developer, I want unit tests for `ScriptParser` covering all keyword-driven replacement paths, so that I can confirm that script lines are rewritten correctly for all supported commands.

#### Acceptance Criteria

1. WHEN a script line contains `AddTopic` with a quoted argument that matches a DIAL entry, THE Test_Suite SHALL verify that the argument is replaced with the translated value.
2. WHEN a script line contains `AddTopic` with an unquoted argument that matches a DIAL entry, THE Test_Suite SHALL verify that the argument is replaced with the translated value.
3. WHEN a script line contains `AddTopic` as a comment (preceded by `;`), THE Test_Suite SHALL verify that the line is not modified.
4. WHEN a script line contains `AddTopic` as part of a longer identifier (e.g., `Begin AddTopicScript`), THE Test_Suite SHALL verify that the line is not modified.
5. WHEN a script line contains `GetPCCell` with a quoted cell name that matches a CELL entry, THE Test_Suite SHALL verify that the cell name is replaced.
6. WHEN a script line contains `AiFollowCell` with a cell name as the second argument, THE Test_Suite SHALL verify that the cell name is replaced.
7. WHEN a script line contains `PositionCell` with a cell name as the fifth argument, THE Test_Suite SHALL verify that the cell name is replaced.
8. WHEN a script line references a topic or cell name that is not present in the merger dict, THE Test_Suite SHALL verify that the line is left unchanged.

---

### Requirement 11: ScriptParser — Message Replacement

**User Story:** As a developer, I want unit tests for `ScriptParser` covering `MessageBox`, `Choice`, and `Say` message replacement, so that I can confirm that in-script dialog strings are correctly substituted.

#### Acceptance Criteria

1. WHEN a script line contains `Say` with a sound file and a message string that matches a BNAM entry, THE Test_Suite SHALL verify that the message string is replaced and the sound file argument is preserved.
2. WHEN a script line contains `MessageBox` with a quoted string that matches a BNAM entry, THE Test_Suite SHALL verify that the string is replaced.
3. WHEN a script line contains `Say` as part of a longer identifier (e.g., `Begin SayScript`), THE Test_Suite SHALL verify that the line is not modified.
4. WHEN a script contains multiple lines, THE Test_Suite SHALL verify that each line is processed independently and the full script output is correct.

---

### Requirement 12: ScriptParser — Script Boundary Handling

**User Story:** As a developer, I want unit tests for `ScriptParser` edge cases around script structure, so that I can confirm that the `End` keyword and trailing newline handling work correctly.

#### Acceptance Criteria

1. WHEN a script line is exactly `end` (case-insensitive), THE Test_Suite SHALL verify that no replacements are attempted on that line or any subsequent lines.
2. WHEN the input script ends with `\r\n`, THE Test_Suite SHALL verify that the output script does not have a trailing `\r\n` appended.
3. WHEN the input script does not end with `\r\n`, THE Test_Suite SHALL verify that the output script length is not reduced below the input length.

---

### Requirement 13: DictCreator — RAW and BASE Mode Integration Tests

**User Story:** As a developer, I want integration tests for `DictCreator` that verify dictionary creation from actual ESM files, so that I can confirm the full extraction pipeline produces correct output.

#### Acceptance Criteria

1. WHEN `DictCreator` is constructed in RAW mode with a single ESM path, THE Test_Suite SHALL verify that the resulting dict contains correct entries for CELL, DIAL, INDX, RNAM, DESC, GMST, FNAM, INFO, TEXT, BNAM, and SCTX chapters.
2. WHEN `DictCreator` is constructed in BASE mode with two ESM files in the same record order, THE Test_Suite SHALL verify that keys come from the reference ESM and values come from the translated ESM.
3. WHEN `DictCreator` is constructed in BASE mode with two ESM files in different record order, THE Test_Suite SHALL verify that matching is performed by content pattern and that unmatched records are marked with the `MISSING` error tag.
