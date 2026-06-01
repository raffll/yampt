# Requirements Document

## Introduction

This feature adds support for exporting OpenMW translation files (`.cel`, `.top`, `.mrk`) from yampt's merged dictionaries. OpenMW uses these plain-text, tab-separated files for runtime localization without modifying ESM/ESP binary data. yampt already holds the necessary data in its CELL and DIAL dictionary chapters — this feature exposes it in the format OpenMW expects.

## Glossary

- **Exporter**: The yampt component responsible for generating OpenMW translation files from merged dictionaries.
- **Translation_File**: A plain-text file in tab-separated format (`key<TAB>value`) consumed by OpenMW for runtime localization. Includes `.cel`, `.top`, and `.mrk` extensions.
- **Cell_File**: A `.cel` translation file mapping internal cell NAME subrecords to localized display names.
- **Topic_File**: A `.top` translation file mapping inflected/declined topic phrase forms back to the canonical (nominative) topic ID.
- **Keyword_File**: A `.mrk` translation file mapping topic IDs to the clickable keyword text displayed in dialogue and journal.
- **Merged_Dictionary**: The result of combining one or more yampt XML dictionaries via the `--merge` or inline merge operation, containing CELL, DIAL, and other chapters.
- **Native_Encoding**: The Windows code page used by the target game installation (e.g. windows-1250 for Polish, windows-1251 for Russian, windows-1252 for English/French/German).
- **ESM_Name**: The base filename (without extension) of the source ESM/ESP, used to derive output filenames (e.g. `Morrowind` produces `Morrowind.cel`).

## Requirements

### Requirement 1: Export Command

**User Story:** As a translator, I want a command-line option to export OpenMW translation files from my merged dictionaries, so that I can use them with OpenMW without modifying the ESM binary.

#### Acceptance Criteria

1. WHEN the user invokes yampt with the `--export-omw` command and provides dictionary paths via `-d` and an ESM file path via `-f`, THE Exporter SHALL generate `.cel`, `.top`, and `.mrk` files.
2. THE Exporter SHALL derive output filenames from the ESM_Name (e.g. input `Morrowind.esm` produces `Morrowind.cel`, `Morrowind.top`, `Morrowind.mrk`).
3. WHEN no dictionaries are provided with the `--export-omw` command, THE Exporter SHALL report a syntax error and produce no output files.
4. WHEN no ESM file path is provided with the `--export-omw` command, THE Exporter SHALL report a syntax error and produce no output files.

### Requirement 2: Cell File Generation

**User Story:** As a translator, I want the `.cel` file to contain cell name mappings from my dictionary, so that OpenMW displays localized cell names in the GUI.

#### Acceptance Criteria

1. THE Exporter SHALL write one line per CELL dictionary entry to the Cell_File.
2. THE Exporter SHALL format each Cell_File line as `foreign_cell_name<TAB>native_cell_name` where the key is the foreign (original) cell name and the value is the translated (native) cell name.
3. THE Exporter SHALL exclude CELL entries where the key equals the value (no translation needed).
4. THE Exporter SHALL write the Cell_File using the specified Native_Encoding.

### Requirement 3: Topic File Generation

**User Story:** As a translator, I want the `.top` file to map translated topic phrase forms back to the foreign topic ID, so that OpenMW resolves explicit `@topic@` links in dialogue correctly.

#### Acceptance Criteria

1. THE Exporter SHALL write one line per DIAL dictionary entry of type Topic to the Topic_File.
2. THE Exporter SHALL format each Topic_File line as `native_topic<TAB>foreign_topic` where the key is the translated topic name and the value is the original foreign topic name.
3. THE Exporter SHALL exclude DIAL entries that are not of dialog type Topic (type byte = 0, prefix "T" in yampt key format).
4. THE Exporter SHALL write the Topic_File using the specified Native_Encoding.

### Requirement 4: Keyword File Generation

**User Story:** As a translator, I want the `.mrk` file to map foreign topic IDs to translated display keywords, so that OpenMW highlights the correct localized text in dialogue.

#### Acceptance Criteria

1. THE Exporter SHALL write one line per DIAL dictionary entry of type Topic to the Keyword_File.
2. THE Exporter SHALL format each Keyword_File line as `foreign_topic<TAB>native_topic` where the key is the original foreign topic name and the value is the translated topic name.
3. THE Exporter SHALL exclude DIAL entries that are not of dialog type Topic (type byte = 0, prefix "T" in yampt key format).
4. THE Exporter SHALL write the Keyword_File using the specified Native_Encoding.

### Requirement 5: Encoding Support

**User Story:** As a translator working with non-Latin scripts, I want to specify the target encoding for the exported files, so that OpenMW can correctly decode them at load time.

#### Acceptance Criteria

1. WHEN the user specifies `--windows-1250` on the command line, THE Exporter SHALL write output files encoded in windows-1250.
2. WHEN the user specifies `--windows-1251` on the command line, THE Exporter SHALL write output files encoded in windows-1251.
3. WHEN the user specifies `--windows-1252` on the command line, THE Exporter SHALL write output files encoded in windows-1252.
4. WHEN no encoding flag is specified, THE Exporter SHALL write output files in the same byte encoding as the dictionary source data (pass-through without conversion).
5. THE Exporter SHALL add support for the `--windows-1251` encoding flag to the existing command-line parser.

### Requirement 6: DIAL Key Parsing

**User Story:** As a developer, I want the exporter to correctly extract the topic name from yampt's DIAL dictionary key format, so that the exported files contain clean topic strings.

#### Acceptance Criteria

1. WHEN processing a DIAL dictionary entry, THE Exporter SHALL extract the topic name from the yampt key format `T^topic_name^` by stripping the type prefix and caret delimiters.
2. WHEN a DIAL key does not start with "T" (indicating a non-Topic dialog type such as Voice, Greeting, Persuasion, or Journal), THE Exporter SHALL skip the entry for `.top` and `.mrk` generation.

### Requirement 7: Output File Format Compliance

**User Story:** As a translator, I want the exported files to be fully compatible with OpenMW's translation loader, so that they work without manual editing.

#### Acceptance Criteria

1. THE Exporter SHALL use a single TAB character (0x09) as the separator between key and value on each line.
2. THE Exporter SHALL terminate each line with a newline character.
3. THE Exporter SHALL produce files that contain no BOM (Byte Order Mark).
4. THE Exporter SHALL skip entries where either the key or value is empty after extraction.
5. THE Exporter SHALL skip entries where the key contains a TAB character (which would corrupt the format).

### Requirement 8: Logging and Summary

**User Story:** As a translator, I want to see a summary of how many entries were exported to each file, so that I can verify the export completed correctly.

#### Acceptance Criteria

1. WHEN export completes successfully, THE Exporter SHALL log the number of entries written to each output file.
2. WHEN a dictionary chapter contains zero exportable entries for a file type, THE Exporter SHALL skip creating that file and log a message indicating no records were available.
3. IF an output file cannot be written (e.g. permission denied), THEN THE Exporter SHALL log an error message identifying the file path.
