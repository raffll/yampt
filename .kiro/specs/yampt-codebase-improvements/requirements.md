# Requirements Document

## Introduction

This document captures requirements for improving the yampt (Yet Another Morrowind Plugin Translator) codebase. The improvements address bugs, correctness issues, performance problems, robustness gaps, and architectural weaknesses identified through comprehensive code analysis. The project is a C++ command-line tool that parses TES3 binary plugin files, extracts translatable text, manages XML-based dictionaries, and converts compiled script data.

## Glossary

- **Yampt**: The yampt.exe command-line application for translating Morrowind plugins
- **TES3_File**: A binary file in the TES3 format (.esm, .esp, .omwgame, .omwaddon) containing game records
- **Record**: A top-level data structure within a TES3_File, identified by a 4-character ID (e.g. CELL, DIAL, INFO)
- **Subrecord**: A nested data structure within a Record, also identified by a 4-character ID
- **Dictionary**: An XML-based file containing key-value translation pairs organized by record type
- **DictReader**: The component responsible for parsing dictionary XML files into in-memory data structures
- **DictMerger**: The component responsible for combining multiple dictionaries into a single merged dictionary
- **DictCreator**: The component responsible for extracting translatable text from TES3 files into dictionary format
- **EsmReader**: The component responsible for parsing TES3 binary files into records and subrecords
- **EsmConverter**: The component responsible for applying dictionary translations to TES3 file records
- **ScriptParser**: The component responsible for translating script source text and compiled script data (SCDT)
- **SCDT**: Compiled script data stored as a binary blob within SCPT records
- **Tools**: The static utility class providing file I/O, byte conversion, string manipulation, and logging

## Requirements

### Requirement 1: Fix readFile Reserve Size

**User Story:** As a developer, I want readFile to correctly determine file size before reading, so that large files are loaded without repeated memory reallocations.

#### Acceptance Criteria

1. WHEN a file is opened for reading, THE Tools SHALL seek to the end of the file to determine its size, then seek back to the beginning before reading content
2. WHEN the file size is determined, THE Tools SHALL reserve exactly that number of bytes in the output string so that reading the file incurs at most one allocation
3. WHEN readFile is called with a valid path, THE Tools SHALL produce byte-for-byte identical content to the current implementation
4. IF the file cannot be opened, THEN THE Tools SHALL return an empty string

### Requirement 2: Fix convertStringByteArrayToUInt Buffer Initialization

**User Story:** As a developer, I want convertStringByteArrayToUInt to avoid undefined behavior when processing short byte arrays, so that the program behaves predictably on all platforms.

#### Acceptance Criteria

1. THE Tools SHALL zero-initialize the buffer array before copying input bytes in convertStringByteArrayToUInt
2. WHEN the input string has fewer than 4 bytes, THE Tools SHALL only read the number of bytes actually present
3. WHEN the input string has exactly 1 byte, THE Tools SHALL return the unsigned value of that single byte

### Requirement 3: Fix trimCR to Only Remove Trailing Carriage Return

**User Story:** As a developer, I want trimCR to only remove a carriage return character at the end of the string, so that strings with embedded carriage returns are not corrupted.

#### Acceptance Criteria

1. WHEN the last character of the string is a carriage return, THE Tools SHALL remove only that last character
2. WHEN the string contains a carriage return at a position other than the end, THE Tools SHALL leave the string unchanged
3. WHEN the string contains no carriage return, THE Tools SHALL leave the string unchanged

### Requirement 4: Fix replaceNonReadableCharsWithDot Signed Char Handling

**User Story:** As a developer, I want replaceNonReadableCharsWithDot to correctly handle extended characters (128-255), so that the function works consistently regardless of platform char signedness.

#### Acceptance Criteria

1. THE Tools SHALL cast characters to unsigned char before performing range comparisons in replaceNonReadableCharsWithDot
2. WHEN a character has a value between 32 and 126 inclusive, THE Tools SHALL preserve it in the output
3. WHEN a character has a value outside the printable ASCII range, THE Tools SHALL replace it with a dot

### Requirement 5: Fix addAnnotations to Find All Occurrences

**User Story:** As a developer, I want addAnnotations to check all occurrences of a keyword in the source text, so that valid matches preceded by alphabetic characters at one position do not prevent finding the keyword at other positions.

#### Acceptance Criteria

1. WHEN the first occurrence of a keyword is preceded by an alphabetic character, THE Tools SHALL continue searching for subsequent occurrences of that keyword
2. WHEN a valid occurrence of a keyword is found (not preceded by an alphabetic character), THE Tools SHALL add the annotation for that keyword
3. WHEN no valid occurrence of a keyword exists in the source text, THE Tools SHALL skip that keyword entirely

### Requirement 6: Fix DictMerger mergeDict to Actually Replace Records

**User Story:** As a developer, I want the merge operation to replace existing records with newer values from later dictionaries, so that the last-wins merge semantics documented in the README are correctly implemented.

#### Acceptance Criteria

1. WHEN a record key exists in the merged dictionary and a later dictionary contains the same key with a different value (exact string comparison), THE DictMerger SHALL overwrite the existing value in the merged dictionary with the value from the later dictionary
2. WHEN a record is replaced, THE DictMerger SHALL increment the replaced counter by 1 for each replacement occurrence
3. WHEN a record is replaced and the record type is not Glossary, THE DictMerger SHALL log a warning that includes the record type and the record key
4. WHEN multiple dictionaries are merged, THE DictMerger SHALL iterate dictionaries in the order they were provided so that the final merged dictionary contains the value from the last dictionary that defined each key
5. WHEN a record key exists in the merged dictionary and a later dictionary contains the same key with an identical value, THE DictMerger SHALL increment the identical counter and leave the existing value unchanged

### Requirement 7: Fix ScriptParser convertLine End Keyword Detection Readability

**User Story:** As a developer, I want the end keyword detection in convertScript to use explicit parentheses, so that the logical intent is clear and not reliant on implicit operator precedence.

#### Acceptance Criteria

1. THE ScriptParser SHALL use explicit parentheses around the compound condition checking for the "end" keyword
2. THE ScriptParser SHALL detect "end" as a standalone keyword (exact match) or followed by a space as the line prefix

### Requirement 8: Fix ScriptParser getpccell Expression Size Calculation

**User Story:** As a developer, I want the getpccell compiled script conversion to handle variable-length comparison expressions, so that multi-digit comparison values do not corrupt the compiled data.

#### Acceptance Criteria

1. WHEN converting getpccell expressions in compiled script data, THE ScriptParser SHALL dynamically determine the end-of-expression position rather than assuming a fixed 5-byte suffix
2. WHEN the comparison expression has more than one digit, THE ScriptParser SHALL correctly calculate the expression size byte

### Requirement 9: Fix DictCreator Dangling Reference

**User Story:** As a developer, I want DictCreator to safely manage its reference to DictMerger, so that no constructor path produces a dangling reference.

#### Acceptance Criteria

1. WHEN DictCreator is constructed in RAW mode (without an external DictMerger), THE DictCreator SHALL own its own DictMerger instance rather than binding a reference to a temporary
2. THE DictCreator SHALL guarantee that the merger member remains valid for the entire lifetime of the DictCreator object

### Requirement 10: Fix makeScriptMessages Keyword False Positives

**User Story:** As a developer, I want makeScriptMessages to use word boundary detection when searching for keywords, so that variable names containing keywords (e.g. "mychoice") do not produce false positive matches.

#### Acceptance Criteria

1. WHEN searching for keywords in script text, THE DictCreator SHALL use word boundary matching consistent with the approach used in ScriptParser::convertLine
2. WHEN a keyword appears as a substring of a larger identifier, THE DictCreator SHALL not treat it as a keyword match

### Requirement 11: Reduce Excessive String Copying

**User Story:** As a developer, I want core functions to avoid unnecessary string copies, so that processing large TES3 files and dictionaries is more memory-efficient.

#### Acceptance Criteria

1. WHEN passing strings that are only read (not modified), THE Yampt SHALL use const reference parameters instead of value parameters
2. WHEN returning strings that are already stored as member variables, THE Yampt SHALL return const references instead of copies
3. THE EsmConverter SHALL minimize intermediate string allocations in convertRecordContent by using in-place operations where feasible

### Requirement 12: Cache Compiled Regex Objects in ScriptParser

**User Story:** As a developer, I want regex objects to be compiled once and reused, so that script conversion does not waste time recompiling the same patterns on every line.

#### Acceptance Criteria

1. THE ScriptParser SHALL compile regex patterns once (at construction or as static members) rather than on every call to convertLine
2. WHEN processing multiple script lines, THE ScriptParser SHALL reuse pre-compiled regex objects

### Requirement 13: Replace Regex-Based Dictionary Parsing with Sequential Parser

**User Story:** As a developer, I want dictionary parsing to use a sequential string search approach, so that loading multi-megabyte dictionary files completes in reasonable time.

#### Acceptance Criteria

1. THE DictReader SHALL parse dictionary files using sequential string search (find-based) rather than regex iteration over the entire file content
2. WHEN parsing a valid dictionary file, THE DictReader SHALL produce the same key-value pairs as the current regex-based implementation
3. WHEN parsing a dictionary file, THE DictReader SHALL handle multiline values between val tags correctly

### Requirement 14: Optimize ScriptParser findNewText Fallback Search

**User Story:** As a developer, I want the case-insensitive dictionary lookup fallback to be more efficient, so that script conversion with large dictionaries does not degrade to linear scan performance.

#### Acceptance Criteria

1. WHEN the exact-match map lookup fails, THE ScriptParser SHALL use a more efficient case-insensitive search strategy than iterating the entire dictionary chapter
2. THE ScriptParser SHALL produce the same translation results as the current linear scan fallback

### Requirement 15: Replace Global Static Log with Instance-Based Logging

**User Story:** As a developer, I want logging to be instance-based rather than global static state, so that the code is testable in isolation and potentially reentrant.

#### Acceptance Criteria

1. THE Yampt SHALL provide a logging mechanism that does not rely on global mutable static strings
2. WHEN multiple operations run in sequence, THE Yampt SHALL maintain separate or clearly scoped log contexts
3. THE Yampt SHALL preserve the current behavior of writing combined logs to yampt.log at program exit

### Requirement 16: Standardize Include Guards

**User Story:** As a developer, I want all header files to use a consistent include guard style, so that the codebase follows a uniform convention.

#### Acceptance Criteria

1. THE Yampt SHALL use the same include guard mechanism across all header files
2. THE Yampt SHALL not mix pragma once with ifndef guards within the same project

### Requirement 17: Pin C++ Standard in Project Configuration

**User Story:** As a developer, I want the C++ standard to be explicitly specified in the project file, so that builds are reproducible regardless of compiler default settings.

#### Acceptance Criteria

1. THE Yampt SHALL specify the C++ language standard (C++23) explicitly in the vcxproj configuration using `<LanguageStandard>stdcpplatest</LanguageStandard>`
2. THE Yampt SHALL apply the language standard setting to all build configurations (Debug and Release, Win32 and x64) in both the main project and the test project

### Requirement 18: Propagate Errors as Return Codes

**User Story:** As a user, I want yampt to return a non-zero exit code when errors occur, so that batch scripts and automation can detect failures.

#### Acceptance Criteria

1. WHEN an error occurs during file loading (file not found or unreadable), parsing (malformed ESM or dictionary structure), or conversion, THE Yampt SHALL return exit code 1 from main
2. WHEN all operations complete successfully, THE Yampt SHALL return exit code 0
3. WHEN a syntax error is detected in command-line arguments (unrecognized command, missing required files, or invalid option combinations), THE Yampt SHALL return exit code 1
4. IF an unhandled exception is caught by the top-level exception handler, THEN THE Yampt SHALL return exit code 1

### Requirement 19: Add Bounds Checking to Binary Parsing

**User Story:** As a user, I want yampt to gracefully handle malformed TES3 files, so that a single corrupt record does not crash the entire operation.

#### Acceptance Criteria

1. WHEN a record header specifies a size that exceeds the remaining file content, THE EsmReader SHALL log a message identifying the record offset and declared size, discard that record, and stop reading further records from the file
2. WHEN a subrecord header specifies a size that exceeds the parent record boundary, THE EsmReader SHALL log a message identifying the subrecord type and offset within the record, and stop processing subrecords for that record while retaining any subrecords already parsed
3. IF one or more records are discarded due to a malformed record header, THEN THE EsmReader SHALL retain all records that were successfully parsed before the malformed record and set the file status to loaded
4. WHEN the mainLoop cursor position plus 8 bytes (subrecord header) would exceed the record content size, THE EsmReader SHALL treat the record as truncated and stop subrecord iteration for that record

### Requirement 20: Protect Against Malformed Subrecord Sizes in mainLoop

**User Story:** As a developer, I want the EsmReader mainLoop to validate subrecord sizes before advancing the position, so that garbage size values do not cause out-of-bounds memory access.

#### Acceptance Criteria

1. WHEN a subrecord size would cause the current position to exceed the record boundary, THE EsmReader SHALL stop iteration and report the error
2. WHEN a subrecord size is zero and would cause an infinite loop, THE EsmReader SHALL detect the condition and break out of the loop

### Requirement 21: Escape Special Characters in Dictionary XML

**User Story:** As a user, I want dictionary values containing XML-like tag sequences to be handled correctly, so that text containing literal "<val>" or "</val>" does not corrupt the dictionary.

**Note:** This requirement addresses the XML format's inherent limitation. The JSON dictionary format (see `json-dictionary-format` spec) eliminates this class of bugs entirely for users who migrate to JSON. This fix remains necessary for backward compatibility with existing XML dictionaries.

#### Acceptance Criteria

1. WHEN writing dictionary values that contain the literal text of XML delimiters, THE Tools SHALL escape or encode those sequences to prevent misparsing
2. WHEN reading dictionary values, THE DictReader SHALL correctly decode any escaped sequences back to their original form
3. FOR ALL valid dictionary entries, writing then reading SHALL produce the original key-value pair (round-trip property)

### Requirement 22: Migrate from Boost.Filesystem to std::filesystem

**User Story:** As a developer, I want the project to use the C++23 standard library filesystem and chrono, so that the external Boost dependency can be removed entirely and the codebase can leverage modern C++ features.

#### Acceptance Criteria

1. THE Yampt SHALL replace the `#include <boost/filesystem/operations.hpp>` directive with `#include <filesystem>` and `#include <chrono>`, and replace all `boost::filesystem::last_write_time` calls with `std::filesystem::last_write_time`, using `std::filesystem::file_time_type` directly instead of `std::time_t` for timestamp storage
2. THE Yampt SHALL remove the Boost NuGet package references from `packages.config` and the corresponding target imports and validation checks from the `.vcxproj` files
3. THE Yampt SHALL set the C++ language standard to C++23 (`stdcpplatest`) in all project build configurations (Debug/Release, Win32/x64) for both the main project and the test project
4. WHEN writing a converted plugin file, THE Yampt SHALL set the output file's last-write-time to the same value as the source file's last-write-time (for `--convert`) or source file's last-write-time plus 1 second using `std::chrono::seconds(1)` (for `--create`), matching the current Boost-based behavior
5. THE Yampt SHALL remove `#include <ctime>` from `includes.hpp` if no other code depends on it after the migration

### Requirement 23: Add Unit Tests for DictReader

**User Story:** As a developer, I want DictReader to have unit test coverage, so that parsing correctness can be verified automatically.

#### Acceptance Criteria

1. THE Yampt SHALL include unit tests that verify DictReader correctly parses well-formed dictionary files
2. THE Yampt SHALL include unit tests that verify DictReader handles malformed dictionary files gracefully
3. THE Yampt SHALL include unit tests that verify DictReader validates entry constraints (CELL max 63 bytes, RNAM max 32 bytes, FNAM max 31 bytes, INFO max 1024 bytes)

### Requirement 24: Add Unit Tests for DictMerger

**User Story:** As a developer, I want DictMerger to have unit test coverage, so that merge semantics (especially the replace behavior from Requirement 6) can be verified.

#### Acceptance Criteria

1. THE Yampt SHALL include unit tests that verify DictMerger correctly merges non-overlapping dictionaries
2. THE Yampt SHALL include unit tests that verify DictMerger replaces records with last-wins semantics
3. THE Yampt SHALL include unit tests that verify DictMerger detects duplicate CELL and DIAL values

### Requirement 25: Add Unit Tests for EsmConverter

**User Story:** As a developer, I want EsmConverter to have unit test coverage for core conversion logic, so that translation application correctness can be verified.

#### Acceptance Criteria

1. THE Yampt SHALL include unit tests that verify EsmConverter correctly applies dictionary translations to record content
2. THE Yampt SHALL include unit tests that verify EsmConverter correctly updates record and subrecord size fields after content replacement
3. THE Yampt SHALL include unit tests that verify EsmConverter handles null-termination rules correctly for each record type

### Requirement 26: Fix NuGet Package Toolset Mismatch

**User Story:** As a developer, I want the NuGet package targets to match the project toolset, so that builds do not produce warnings or link against mismatched binaries.

#### Acceptance Criteria

1. IF Boost is still required, THEN THE Yampt SHALL reference NuGet packages matching the v143 toolset
2. WHEN the project is built, THE Yampt SHALL not produce toolset mismatch warnings

### Requirement 27: Document All Command-Line Options

**User Story:** As a user, I want all command-line options to be documented in the README, so that I can discover and use features like --create, --add-annotations, --windows-1250, and -s.

#### Acceptance Criteria

1. THE Yampt README SHALL document the --create command with usage examples
2. THE Yampt README SHALL document the --add-annotations option
3. THE Yampt README SHALL document the --windows-1250 option
4. THE Yampt README SHALL document the -s (suffix) option with usage examples

### Requirement 28: Add Hyperlinks to Greeting Dialog INFO Records

**User Story:** As a translator, I want hyperlinks to be added to the end of Greeting 0–9 dialog responses, so that topic references embedded in greeting text are annotated the same way as regular topic dialog responses.

#### Acceptance Criteria

1. WHEN `addHyperlinks` processes an INFO record whose key prefix starts with `"G"` (Greeting dialog type), THE DictCreator SHALL append hyperlink annotations to the value text, using the same logic applied to non-Voice INFO records
2. WHEN `addHyperlinks` processes an INFO record whose key prefix starts with `"V"` (Voice dialog type), THE DictCreator SHALL leave the value text unchanged (existing behavior preserved)
3. WHEN `addHyperlinks` processes an INFO record whose key prefix starts with any dialog type other than `"V"` (including `"T"` Topic, `"G"` Greeting, `"P"` Persuasion, `"J"` Journal), THE DictCreator SHALL append hyperlink annotations if matching DIAL entries are found
4. WHEN the resulting annotated text would exceed 1024 bytes, THE DictCreator SHALL truncate it to 1024 bytes (existing truncation behavior preserved)

### Requirement 29: Handle 4-Byte Cell Name Subrecords in Pattern Matching

**User Story:** As a translator, I want short cell names like "Vos" that are stored as exactly 4 bytes (including the null terminator) to be matched correctly during unordered dictionary creation, so that cells with short names are not incorrectly treated as missing.

#### Acceptance Criteria

1. WHEN a CELL record's NAME subrecord has a binary size of 4 bytes (e.g. a 3-character name followed by a null byte), THE EsmReader SHALL extract the text correctly by stripping the null terminator, producing a 3-character string
2. WHEN building unordered CELL patterns, THE DictCreator SHALL use the null-stripped cell name text (not the raw binary content including the null byte) as the pattern key, so that the same cell name from two different files produces identical pattern strings
3. WHEN `convertStringByteArrayToUInt` is called with a 4-byte string that represents a cell name rather than a numeric value, THE Tools SHALL not be invoked for that purpose — cell name length is determined from the subrecord size field, not by interpreting the name bytes as an integer

### Requirement 30: Preserve Annotations When Merging Dictionaries

**User Story:** As a translator, I want annotations (comments) from input dictionaries to be carried over into the merged output, so that translator notes and hyperlink annotations are not silently lost during a merge operation.

#### Acceptance Criteria

1. WHEN `DictReader` parses an XML dictionary file that contains `<!-- ... -->` annotation blocks following a `<record>` entry, THE DictReader SHALL extract the annotation text and store it in the `Annotations` chapter of the internal Dict, keyed by the same key as the corresponding record
2. WHEN `DictMerger::mergeDict` processes input dictionaries, THE DictMerger SHALL also merge the `Annotations` chapter using the same last-wins semantics applied to all other record types
3. WHEN a record is replaced during merge (later dictionary wins), THE DictMerger SHALL also replace the annotation for that key if the later dictionary provides one, or retain the existing annotation if the later dictionary has none for that key
4. WHEN the merged dictionary is written to an output file, THE Tools::writeDict SHALL include the merged annotations in the output, preserving the `<!-- ... -->` format for each record that has an annotation

### Requirement 31: Upgrade Visual Studio Project to VS2022 (v143 Toolset)

**User Story:** As a developer, I want the project files to target the VS2022 toolset and platform toolset v143, so that the project builds cleanly in Visual Studio 2022 without legacy toolset warnings.

#### Acceptance Criteria

1. THE Yampt `.vcxproj` files SHALL set `<PlatformToolset>v143</PlatformToolset>` in all build configurations (Debug/Release, Win32/x64) for both the main project and the test project
2. THE Yampt solution file SHALL set the `VisualStudioVersion` to `17.0` (VS2022) and update the `MinimumVisualStudioVersion` accordingly
3. WHEN the project is opened in Visual Studio 2022, THE Yampt SHALL not prompt for a toolset upgrade or produce toolset mismatch warnings
4. THE Yampt SHALL retain all existing build configurations and output paths unchanged after the toolset upgrade

### Requirement 32: Migrate Dependency Management to vcpkg

**User Story:** As a developer, I want the project to use vcpkg for managing C++ dependencies, so that acquiring and updating libraries is reproducible and does not rely on manually placed NuGet packages or pre-installed system libraries.

#### Acceptance Criteria

1. THE Yampt SHALL include a `vcpkg.json` manifest file at the repository root that declares all required dependencies with pinned versions
2. WHEN Boost is still required (i.e. Requirement 22 has not been completed), THE `vcpkg.json` SHALL declare `boost-filesystem` as a dependency; once Requirement 22 is complete, the Boost entry SHALL be removed
3. THE Yampt `.vcxproj` files SHALL integrate with vcpkg via the vcpkg toolchain or MSBuild integration so that dependencies are resolved automatically during build
4. THE `packages.config` NuGet file SHALL be removed once all dependencies are managed through vcpkg
5. WHEN a developer clones the repository and runs the build, THE Yampt SHALL acquire all dependencies through vcpkg without requiring manual download or installation steps
