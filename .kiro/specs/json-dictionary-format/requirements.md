# Requirements Document

## Introduction

yampt (Yet Another Morrowind Plugin Translator) currently stores translation dictionaries in a custom pseudo-XML format that suffers from parsing fragility, poor performance on large files, and lack of standard tooling support. This feature replaces XML with JSON as the primary dictionary format, enabling reliable parsing, better performance, human readability, and compatibility with standard JSON tools (jq, VS Code, etc.). XML support is retained only for reading existing files and converting them to JSON.

## Glossary

- **Dictionary**: A file containing translation records that map original-language text keys to translated-language text values, grouped by record type
- **Record_Type**: A four-letter identifier (CELL, DIAL, INFO, FNAM, etc.) classifying the kind of translatable text in a Morrowind plugin
- **Dictionary_Reader**: The yampt component responsible for loading and parsing dictionary files into the internal Dict data structure
- **Dictionary_Writer**: The yampt component responsible for serializing the internal Dict data structure to a dictionary file on disk
- **Dict**: The internal in-memory representation of a dictionary, organized as a map of Record_Type to Chapter (key-value pairs)
- **JSON_Dictionary**: A dictionary file stored in JSON format with the `.json` extension
- **XML_Dictionary**: A dictionary file stored in the legacy pseudo-XML format with the `.xml` extension; supported for reading and conversion to JSON only
- **Format_Detector**: The yampt component responsible for determining whether a dictionary file is in JSON or XML format; used during read operations to select the appropriate parser
- **Record_Object**: The per-entry JSON object within a Record_Type chapter, containing the original text, translated text, and all metadata fields
- **Original**: The source-language text extracted from the plugin (the key used for lookup); stored in the `original` field of a Record_Object
- **Translation**: The target-language text provided by the translator; stored in the `translation` field of a Record_Object
- **Status**: The translation state of a record (Untranslated, Translated, Auto_Identical, Auto_Heuristic, Validated, Changed_In_Base, Has_Errors); stored in the `status` field
- **Hyperlinks**: DIAL topic names found in an INFO record's text that appear as clickable links in-game; stored in the `hyperlinks` array field
- **Glossary_Annotations**: FNAM-derived name translations (NPC names, item names, place names) relevant to a record; stored in the `glossary` array field
- **Gender**: The speaker gender flag (M or F) for INFO records, derived from the NPC_FLAG dictionary; stored in the `gender` field
- **History**: A list of previous Value strings with timestamps for a record; stored in the `history` array field

## Requirements

### Requirement 1: JSON Dictionary Reading

**User Story:** As a translator, I want yampt to read dictionaries stored in JSON format, so that I can use a more reliable and standard file format for my translation work.

#### Acceptance Criteria

1. WHEN a JSON dictionary file is provided as input, THE Dictionary_Reader SHALL parse the file and populate the internal Dict structure with all records, using the `"original"` field as the lookup key and the `"translation"` field as the value
2. WHEN a JSON dictionary file contains records grouped by Record_Type, THE Dictionary_Reader SHALL correctly associate each Record_Object with its Record_Type
3. WHEN a JSON dictionary file contains Record_Objects with metadata fields (`status`, `hyperlinks`, `glossary`, `gender`, `history`), THE Dictionary_Reader SHALL load and preserve those fields in the internal representation
4. IF a JSON dictionary file contains malformed JSON, THEN THE Dictionary_Reader SHALL report a descriptive error message including the file path
5. IF a JSON dictionary file contains an unrecognized Record_Type, THEN THE Dictionary_Reader SHALL log a warning and skip the unrecognized entries
6. WHEN a JSON dictionary file is loaded, THE Dictionary_Reader SHALL apply the same validation rules as for XML dictionaries (CELL max 63 bytes, RNAM max 32 bytes, FNAM max 31 bytes, INFO max 1024 bytes)
7. WHEN a JSON dictionary file contains a Record_Object missing the `"original"` field, THE Dictionary_Reader SHALL log a warning and skip that record

### Requirement 2: JSON Dictionary Writing

**User Story:** As a translator, I want yampt to write dictionaries in JSON format, so that I can produce output files that are easy to read, edit, and process with standard tools.

#### Acceptance Criteria

1. WHEN the Dictionary_Writer outputs a JSON dictionary, THE Dictionary_Writer SHALL produce a valid JSON file parseable by any compliant JSON parser
2. WHEN the Dictionary_Writer outputs a JSON dictionary, THE Dictionary_Writer SHALL group records by Record_Type as top-level object keys, each containing a JSON array of Record_Objects
3. WHEN the Dictionary_Writer outputs a JSON dictionary, THE Dictionary_Writer SHALL write each record as a Record_Object with at minimum the `"original"` and `"translation"` fields
4. WHEN the Dictionary_Writer outputs a JSON dictionary and a record has metadata fields set (status, hyperlinks, glossary, gender, history), THE Dictionary_Writer SHALL include those fields in the Record_Object
5. WHEN the Dictionary_Writer outputs a JSON dictionary, THE Dictionary_Writer SHALL omit optional metadata fields that are empty or unset, rather than writing them as `null` or empty arrays
6. WHEN the Dictionary_Writer outputs a JSON dictionary, THE Dictionary_Writer SHALL preserve all characters in `"original"` and `"translation"` values without corruption, including characters that break the XML format (such as `</val>` or `<key>` appearing in text content)
7. WHEN the Dictionary_Writer outputs a JSON dictionary, THE Dictionary_Writer SHALL use UTF-8 encoding
8. WHEN the Dictionary_Writer outputs a JSON dictionary, THE Dictionary_Writer SHALL produce human-readable output with indentation

### Requirement 3: Format Detection

**User Story:** As a translator, I want yampt to automatically detect whether a dictionary file is in JSON or XML format when reading, so that I can load legacy XML files without extra flags.

#### Acceptance Criteria

1. WHEN a dictionary file with `.json` extension is provided, THE Format_Detector SHALL select the JSON parser
2. WHEN a dictionary file with `.xml` extension is provided, THE Format_Detector SHALL select the XML parser (read-only path)
3. IF a dictionary file has an unrecognized extension, THEN THE Format_Detector SHALL attempt to detect the format by inspecting the file content
4. WHEN multiple dictionary files are provided in a single command (via `-d` flag), THE Format_Detector SHALL detect the format of each file independently

### Requirement 4: Command Compatibility

**User Story:** As a translator, I want all yampt commands to produce JSON output by default and accept XML input files, so that I can migrate my workflow without losing access to existing dictionaries.

#### Acceptance Criteria

1. WHEN the `--make-raw` command is executed, THE Dictionary_Writer SHALL produce output in JSON format
2. WHEN the `--make-base` command is executed, THE Dictionary_Writer SHALL produce output in JSON format
3. WHEN the `--make-all` command is executed, THE Dictionary_Writer SHALL produce output in JSON format
4. WHEN the `--make-not` command is executed, THE Dictionary_Writer SHALL produce output in JSON format
5. WHEN the `--make-changed` command is executed, THE Dictionary_Writer SHALL produce output in JSON format
6. WHEN the `--merge` command is executed with a mix of JSON and XML input dictionaries, THE Dictionary_Reader SHALL read records from both formats and THE Dictionary_Writer SHALL produce output in JSON format
7. WHEN the `--convert` command is executed with dictionaries provided via `-d`, THE Dictionary_Reader SHALL accept both JSON and XML input files and apply translations to the plugin
8. THE Dictionary_Writer SHALL NOT produce XML output for any command; XML is a read-only legacy format

### Requirement 5: JSON Structure

**User Story:** As a translator, I want the JSON dictionary to use a clear, self-contained record format that stores both the original and translated text alongside all metadata, so that I can use it directly in a translation editor without needing a separate sidecar file.

**Note:** The JSON library chosen here (design phase decision) must also be used by the translation editor (`translation-editor` spec) to avoid duplicate dependencies.

#### Acceptance Criteria

1. THE JSON_Dictionary SHALL use a top-level JSON object with Record_Type strings as keys
2. THE JSON_Dictionary SHALL represent each Record_Type chapter as a JSON array of Record_Objects (preserving insertion order)
3. EACH Record_Object SHALL contain the following fields:
   - `"original"` (string, required) — the source-language text extracted from the plugin; this is the lookup key
   - `"translation"` (string, required) — the translated text; empty string `""` when not yet translated
   - `"status"` (string, optional) — one of `"Untranslated"`, `"Translated"`, `"Auto_Identical"`, `"Auto_Heuristic"`, `"Validated"`, `"Changed_In_Base"`, `"Has_Errors"`; omitted when not set
   - `"hyperlinks"` (array of objects, optional) — DIAL topic references found in the record text; each object has `"original"` and `"translation"` string fields; omitted when empty
   - `"glossary"` (array of objects, optional) — FNAM-derived name annotations relevant to the record; each object has `"original"` and `"translation"` string fields; omitted when empty
   - `"gender"` (string, optional) — speaker gender for INFO records: `"M"` or `"F"`; omitted when not applicable
   - `"history"` (array of objects, optional) — previous translation values; each object has `"value"` (string) and `"timestamp"` (ISO 8601 string) fields; omitted when empty
4. THE JSON_Dictionary SHALL omit optional fields entirely (not write them as `null`) when they have no value, to keep files compact
5. THE JSON_Dictionary SHALL preserve the insertion order of Record_Objects within each Record_Type chapter
6. WHEN a dictionary contains Glossary or Annotations records, THE JSON_Dictionary SHALL include them as separate top-level keys using the same array-of-Record_Objects structure
7. THE JSON_Dictionary SHALL use the `"original"` field as the canonical lookup key when reading records into the internal Dict structure (replacing the previous flat key→value mapping)

#### Example Structure

```json
{
  "CELL": [
    {
      "original": "Seyda Neen",
      "translation": "Sejda Neen",
      "status": "Validated"
    }
  ],
  "INFO": [
    {
      "original": "Greetings, traveler. Have you heard about the Nerevarine?",
      "translation": "Witaj, wędrowcze. Słyszałeś o Nerevarinie?",
      "status": "Translated",
      "hyperlinks": [
        { "original": "Nerevarine", "translation": "Nerevarinie" }
      ],
      "gender": "M",
      "history": [
        { "value": "Witaj, podróżniku.", "timestamp": "2025-01-15T10:30:00Z" }
      ]
    },
    {
      "original": "I have nothing to say to you.",
      "translation": "",
      "status": "Untranslated",
      "gender": "F"
    }
  ],
  "FNAM": [
    {
      "original": "Iron Sword",
      "translation": "Żelazny Miecz",
      "status": "Validated"
    }
  ]
}
```

### Requirement 6: XML Migration

**User Story:** As a translator, I want to convert my existing XML dictionaries to JSON, so that I can migrate to the new format without losing any data.

#### Acceptance Criteria

1. WHEN the `--convert-xml` command is executed with an XML dictionary file as input, THE Dictionary_Writer SHALL produce an equivalent JSON dictionary file with all key-value pairs preserved
2. WHEN converting an XML dictionary to JSON, THE Dictionary_Writer SHALL initialize each Record_Object with `"status": "Untranslated"` for records whose value is empty, and `"status": "Translated"` for records that have a non-empty translation value
3. WHEN converting an XML dictionary to JSON, THE Dictionary_Writer SHALL output the file with the same base name and a `.json` extension unless an explicit output path is provided via `-o`
4. THE `--convert-xml` command SHALL be the only supported path for producing JSON output from XML input; no other command SHALL write XML output

### Requirement 7: XML-to-JSON Migration Fidelity

**User Story:** As a translator, I want to convert my existing XML dictionaries to JSON without losing any translation data, so that I can migrate safely.

#### Acceptance Criteria

1. FOR ALL valid XML dictionaries, reading the XML file and writing it as JSON then reading the JSON file back SHALL produce an equivalent Dict with all key-value pairs intact
2. WHEN converting from XML to JSON, THE Dictionary_Writer SHALL preserve all key-value pairs and their Record_Type associations without data loss
3. WHEN converting from XML to JSON, metadata fields not present in XML (`hyperlinks`, `glossary`, `gender`, `history`) SHALL be omitted from the output Record_Objects

### Requirement 8: Performance

**User Story:** As a translator, I want JSON dictionary parsing to be faster than the current XML regex-based parsing, so that working with large multi-MB dictionaries is more responsive.

#### Acceptance Criteria

1. WHEN parsing a JSON dictionary, THE Dictionary_Reader SHALL not use regex over the entire file content
2. WHEN parsing a JSON dictionary of equivalent content, THE Dictionary_Reader SHALL complete parsing in less time than the XML parser takes for the same data

### Requirement 9: Metadata Preservation During Merge

**User Story:** As a translator, I want metadata fields (status, hyperlinks, glossary, gender, history) preserved when merging JSON dictionaries, so that my translation progress and annotations are not lost during workflow operations.

#### Acceptance Criteria

1. WHEN the `--merge` command merges two JSON dictionaries and both contain a Record_Object with the same `"original"` key, THE Dictionary_Writer SHALL use the metadata fields from the higher-priority input dictionary
2. WHEN the `--merge` command reads an XML input dictionary alongside JSON dictionaries, THE Dictionary_Reader SHALL load the XML records as plain key-value pairs with no metadata; the merged JSON output SHALL initialize those records with `"status": "Translated"` if the translation is non-empty, or `"status": "Untranslated"` if empty
3. WHEN yampt commands that produce new dictionaries (`--make-raw`, `--make-base`, `--make-all`, `--make-not`, `--make-changed`) write JSON output, THE Dictionary_Writer SHALL initialize each Record_Object with `"status": "Untranslated"` and omit all other optional metadata fields
4. WHEN the `--convert` command applies translations from a dictionary, THE Dictionary_Reader SHALL use the `"translation"` field (JSON) or the value field (XML) as the translation value, ignoring all metadata fields during conversion
