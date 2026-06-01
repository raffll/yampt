# Requirements Document

## Introduction

This feature integrates free online translation services into yampt to auto-translate dictionary entries while preserving Morrowind dialog hyperlinks. Morrowind dialog text contains topic keywords that function as clickable hyperlinks — these must remain untranslated (or translated consistently with the DIAL dictionary) to maintain in-game functionality. The feature adds a new command-line mode that sends dictionary values to a translation API, protects topic names from mistranslation using a placeholder mechanism, and produces a translated dictionary ready for manual review.

## Glossary

- **Translator**: The yampt online translation subsystem that sends text to external translation APIs and receives translated results
- **Topic_Protector**: The subsystem responsible for identifying topic names (DIAL entries) within text and shielding them from translation
- **Placeholder**: A numbered token (e.g. `{{1}}`, `{{2}}`) that replaces a topic name before text is sent to the translation API
- **Translation_API**: An external HTTP REST service that accepts source text and returns translated text (e.g. LibreTranslate, MyMemory)
- **Dictionary**: An XML file containing key-value pairs organized by record type (CELL, DIAL, INFO, TEXT, etc.) as used by yampt
- **Topic_Name**: A DIAL record value — a translated dialog topic name that functions as a hyperlink keyword in-game
- **Hyperlink**: An in-game clickable word in dialog text that matches a topic name, allowing the player to ask about that topic
- **Rate_Limiter**: The subsystem that throttles API requests to stay within free-tier usage limits

## Requirements

### Requirement 1: Translate Dictionary Command

**User Story:** As a translator, I want to auto-translate dictionary entries via an online service, so that I can get a first-pass translation without manually translating every entry.

#### Acceptance Criteria

1. WHEN the `--translate` command is invoked with a dictionary file and a target language, THE Translator SHALL send each untranslated value to the configured Translation_API and write a new dictionary file with translated values
2. THE Translator SHALL accept a `--service` option specifying which Translation_API to use (libretranslate, mymemory)
3. THE Translator SHALL accept a `--source-lang` and `--target-lang` option specifying ISO 639-1 language codes
4. THE Translator SHALL accept a `--api-url` option to specify the Translation_API endpoint URL
5. IF the `--api-key` option is provided, THEN THE Translator SHALL include the API key in requests to the Translation_API

### Requirement 2: Topic Name Protection via Placeholders

**User Story:** As a translator, I want topic names in dialog text to be protected from translation, so that hyperlinks remain functional after machine translation.

#### Acceptance Criteria

1. WHEN translating an INFO record value, THE Topic_Protector SHALL scan the text for all known Topic_Names (from the DIAL chapter of the provided dictionaries) and replace each occurrence with a unique Placeholder before sending to the Translation_API
2. WHEN translating a TEXT record value, THE Topic_Protector SHALL apply the same Placeholder substitution as for INFO records
3. THE Topic_Protector SHALL perform case-insensitive matching when scanning for Topic_Names in source text
4. THE Topic_Protector SHALL match Topic_Names only at word boundaries to avoid replacing partial words
5. WHEN multiple Topic_Names overlap in the source text, THE Topic_Protector SHALL prefer the longest matching Topic_Name
6. WHEN a translated response is received from the Translation_API, THE Topic_Protector SHALL replace each Placeholder with the corresponding translated Topic_Name from the DIAL dictionary

### Requirement 3: Placeholder Format and Integrity

**User Story:** As a translator, I want placeholders to survive machine translation intact, so that topic names can be reliably restored after translation.

#### Acceptance Criteria

1. THE Topic_Protector SHALL use the format `{{N}}` (where N is a sequential integer starting at 1) for Placeholders
2. IF a Placeholder is missing or malformed in the translated response, THEN THE Translator SHALL log a warning identifying the affected record and insert the original Topic_Name untranslated
3. IF the Translation_API returns text containing unreplaced Placeholders after restoration, THEN THE Translator SHALL log an error for that record

### Requirement 4: Record Type Filtering

**User Story:** As a translator, I want to control which record types are translated, so that I can avoid translating entries that should remain unchanged (like script lines or cell names).

#### Acceptance Criteria

1. THE Translator SHALL translate only INFO, TEXT, DESC, FNAM, GMST, and INDX record types by default
2. THE Translator SHALL accept a `--types` option to specify which record types to translate
3. THE Translator SHALL skip DIAL, CELL, BNAM, and SCTX record types (these require exact matching or are code)
4. WHEN a record type is excluded from translation, THE Translator SHALL copy the original value unchanged into the output dictionary

### Requirement 5: Rate Limiting and Retry

**User Story:** As a translator, I want the tool to respect API rate limits, so that my requests are not rejected and I stay within free-tier quotas.

#### Acceptance Criteria

1. THE Rate_Limiter SHALL enforce a configurable delay between consecutive API requests (default: 1 second)
2. THE Translator SHALL accept a `--delay` option specifying the delay in milliseconds between requests
3. IF the Translation_API returns an HTTP 429 (Too Many Requests) response, THEN THE Rate_Limiter SHALL wait for the duration specified in the Retry-After header (or 60 seconds if absent) before retrying
4. IF the Translation_API returns an HTTP 5xx error, THEN THE Translator SHALL retry the request up to 3 times with exponential backoff
5. IF all retries are exhausted for a record, THEN THE Translator SHALL log an error and copy the original value unchanged into the output dictionary

### Requirement 6: Translation Progress and Resumption

**User Story:** As a translator, I want to resume an interrupted translation session, so that I do not re-translate entries that were already processed.

#### Acceptance Criteria

1. WHILE translating, THE Translator SHALL write each successfully translated record to the output dictionary immediately (incremental writes)
2. WHEN the `--translate` command is invoked and the output dictionary already exists, THE Translator SHALL skip records whose keys already have a translated value in the output file
3. THE Translator SHALL display progress to the console showing the current record number, total records, and percentage complete
4. IF the user interrupts the process (Ctrl+C), THEN THE Translator SHALL finish writing the current record and exit cleanly

### Requirement 7: LibreTranslate Integration

**User Story:** As a translator, I want to use LibreTranslate as a translation backend, so that I can self-host the service or use public instances.

#### Acceptance Criteria

1. WHEN `--service libretranslate` is specified, THE Translator SHALL send POST requests to the `/translate` endpoint with JSON body containing `q`, `source`, `target`, and optionally `api_key` fields
2. THE Translator SHALL parse the JSON response and extract the translated text from the `translatedText` field
3. IF the LibreTranslate instance returns an error response with a JSON `error` field, THEN THE Translator SHALL log the error message

### Requirement 8: MyMemory API Integration

**User Story:** As a translator, I want to use the MyMemory translation API as an alternative backend, so that I have a fallback when LibreTranslate is unavailable.

#### Acceptance Criteria

1. WHEN `--service mymemory` is specified, THE Translator SHALL send GET requests to the MyMemory API endpoint with `q`, `langpair` (formatted as `source|target`), and optionally `de` (email) query parameters
2. THE Translator SHALL parse the JSON response and extract the translated text from `responseData.translatedText`
3. IF the MyMemory API returns a `responseStatus` other than 200, THEN THE Translator SHALL log the error and treat the record as untranslated

### Requirement 9: HTTP Client

**User Story:** As a developer, I want yampt to make HTTP requests to translation APIs, so that the tool can communicate with external services.

#### Acceptance Criteria

1. THE Translator SHALL use an HTTP client capable of making GET and POST requests with JSON payloads
2. THE Translator SHALL set a request timeout of 30 seconds per API call
3. IF a network error or timeout occurs, THEN THE Translator SHALL log the error and apply the retry logic from Requirement 5
4. THE Translator SHALL send a User-Agent header identifying itself as `yampt/<version>`

### Requirement 10: Output Dictionary Format

**User Story:** As a translator, I want the translated dictionary to be in standard yampt XML format, so that I can review it in a text editor and use it with the existing `--convert` command.

#### Acceptance Criteria

1. THE Translator SHALL write the output dictionary in the same XML format as other yampt dictionaries (using `<record>`, `<_id>`, `<key>`, `<val>` tags)
2. THE Translator SHALL preserve the original key text unchanged in the output dictionary
3. THE Translator SHALL add an annotation comment containing the original untranslated value for each translated record, enabling manual review
4. WHEN a DIAL dictionary is provided for topic protection, THE Translator SHALL include the DIAL entries unchanged in the output dictionary so the result is self-contained
