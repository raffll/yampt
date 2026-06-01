# Requirements Document

## Introduction

This feature integrates free online translation APIs into yampt to provide machine-translated first drafts for untranslated records. The key differentiator is hyperlink-aware translation: dialog text containing topic hyperlinks (marked with `@topic@` in OpenMW or implicitly matched by the engine) must preserve the topic references after translation, so that translated dialogue still links to the correct topics.

## Glossary

- **Translator**: The yampt component responsible for sending text to online translation APIs and receiving translated results
- **Hyperlink**: A topic reference embedded in dialog text that the game engine uses to create clickable links in the dialogue window
- **Topic_Marker**: A delimiter or pattern identifying a topic reference within dialog text (e.g. topic names that match DIAL entries)
- **Protected_Segment**: A portion of text that must not be translated (topic names, variable references like %PCName, script commands)
- **Translation_API**: A free online machine translation service (e.g. LibreTranslate, MyMemory, Lingva)
- **Batch_Translation**: Sending multiple records to the translation API in a single session to reduce overhead
- **Source_Language**: The language of the original (foreign) text in the dictionary keys
- **Target_Language**: The language the text should be translated into

## Requirements

### Requirement 1: Translation API Integration

**User Story:** As a translator, I want yampt to send untranslated records to a free online translation service, so that I get machine-translated first drafts instead of starting from scratch.

#### Acceptance Criteria

1. THE Translator SHALL support at least one free translation API that does not require payment (e.g. LibreTranslate self-hosted, MyMemory free tier, or Lingva)
2. WHEN the user invokes the `--translate` command with source and target language codes, THE Translator SHALL send untranslated values to the configured API and store the results
3. THE Translator SHALL support configuring the API endpoint URL via a command-line flag or configuration file
4. IF the translation API is unreachable or returns an error, THEN THE Translator SHALL log the error and leave the record untranslated rather than corrupting the dictionary

### Requirement 2: Hyperlink-Aware Translation

**User Story:** As a translator, I want topic names in dialog text to be preserved during machine translation, so that hyperlinks still work after translation.

#### Acceptance Criteria

1. BEFORE sending INFO text to the translation API, THE Translator SHALL identify all topic references (substrings matching DIAL dictionary keys) and replace them with placeholder tokens
2. AFTER receiving the translated text from the API, THE Translator SHALL replace the placeholder tokens with the translated topic names from the DIAL dictionary
3. WHEN a topic name appears in dialog text, THE Translator SHALL preserve its position relative to surrounding text (not move it to a different location in the sentence)
4. WHEN multiple topic references exist in a single INFO record, THE Translator SHALL protect and restore all of them independently

### Requirement 3: Protected Segments

**User Story:** As a translator, I want game variables and script references to be preserved during translation, so that the translated text still functions correctly in-game.

#### Acceptance Criteria

1. THE Translator SHALL identify and protect the following patterns from translation: `%PCName`, `%PCRace`, `%PCClass`, `%PCRank`, `%Name`, `%Faction`, `%NextPCRank`, `%cell`, and other `%`-prefixed variables
2. THE Translator SHALL identify and protect HTML-like tags used in Morrowind book text (e.g. `<BR>`, `<DIV>`, `<FONT>`, `<IMG>`)
3. AFTER translation, THE Translator SHALL restore all protected segments in their original form at the correct positions
4. IF the translation API reorders or removes a placeholder, THEN THE Translator SHALL log a warning for that record and keep the original untranslated text

### Requirement 4: Batch Processing

**User Story:** As a translator, I want to translate entire dictionaries in batch, so that I can get first drafts for hundreds of records without manual intervention.

#### Acceptance Criteria

1. WHEN the `--translate` command is applied to a dictionary, THE Translator SHALL process all records that have empty or identical key/value pairs (untranslated)
2. THE Translator SHALL respect API rate limits by introducing configurable delays between requests
3. THE Translator SHALL support resuming interrupted batch translations by skipping records that already have a non-empty translated value
4. THE Translator SHALL log progress during batch translation (e.g. "Translated 150/500 INFO records...")

### Requirement 5: Language Configuration

**User Story:** As a translator, I want to specify source and target languages, so that the translation API knows which direction to translate.

#### Acceptance Criteria

1. THE Translator SHALL accept `--source-lang` and `--target-lang` command-line flags with ISO 639-1 language codes (e.g. `en`, `pl`, `ru`, `de`, `fr`)
2. WHEN source language is not specified, THE Translator SHALL default to `en` (English)
3. WHEN target language is not specified, THE Translator SHALL report a syntax error

### Requirement 6: Output as Draft Dictionary

**User Story:** As a translator, I want machine translations saved as a separate draft dictionary, so that I can review and correct them before applying to the game.

**Note:** Draft dictionaries are intended to be opened in the translation editor (see `translation-editor` spec) for review and correction before use with `--convert`.

#### Acceptance Criteria

1. THE Translator SHALL write machine-translated records to a new dictionary file with `.DRAFT` suffix (e.g. `Plugin.DRAFT.xml` or `Plugin.DRAFT.json`)
2. THE Translator SHALL NOT overwrite existing user dictionaries or base dictionaries
3. THE Translator SHALL mark each machine-translated record with an annotation indicating it is a draft (e.g. via the Annotations chapter or a comment)
4. WHEN the `--json` flag is also provided, THE Translator SHALL output the draft dictionary in JSON format

### Requirement 7: Record Type Selection

**User Story:** As a translator, I want to choose which record types to translate, so that I can focus machine translation on dialog text while manually handling short entries like cell names.

#### Acceptance Criteria

1. THE Translator SHALL accept a `--types` flag that specifies which Record_Types to translate (e.g. `--types INFO,TEXT,DESC`)
2. WHEN `--types` is not specified, THE Translator SHALL translate all record types by default
3. WHEN `--types` is specified, THE Translator SHALL skip records of types not in the list

### Requirement 8: Translation Quality Indicators

**User Story:** As a translator, I want to see confidence or quality indicators for machine translations, so that I can prioritize which records need the most manual review.

#### Acceptance Criteria

1. IF the translation API provides a confidence score or match quality, THEN THE Translator SHALL store it as an annotation on the record
2. WHEN a translated text is significantly shorter or longer than the original (ratio below 0.3 or above 3.0), THE Translator SHALL flag the record as potentially problematic
3. WHEN a protected segment placeholder was not found in the translated output, THE Translator SHALL flag the record as requiring manual review
