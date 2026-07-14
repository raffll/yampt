# TODO

## Release tasks

- readme, feature categories
- nexus api: auto upload, auto description
- github: remove obsolete releases
- ship model, at least polish

## Features

### EET Convert (yTranslator)
Wire up EET file import/convert.

### Batch Clean (yEditor) — WIP
Code exists (`batch_cleaner_t`) but button is hidden — ITM detection produces inconsistent results when cleaned files are reloaded.

### Settings Panel (yTranslator)
Panel with all exceptions configurable.

### Remove --win1250 (yampt.cli)
Drop the legacy codepage flag from CLI.

### Plugin Header Management (yEditor or yampt.cli) [L]
- Update master file sizes in headers (Wrye Mash feature — needed after cleaning)
- Update plugin version to 1.3
- Reassign masters (swap one master for another)

Simple binary surgery on the TES3 header record. `esm_reader_t` already parses it.

## Nice-to-have

### Scan Lua Files for Translatable Strings [L]
Parse OpenMW Lua scripts (l10n YAML, or string literals) to extract translatable text for dictionary creation.

### Generate TOP/CEL/MRK Files from Dictionaries [XL]
Create `.top`, `.cel`, `.mrk` files based on DIAL/CELL dictionaries and the language spell checker dictionary. These are OpenMW cell/topic name files used by the engine for localized map markers and journal topics. Include generated files as annotation sources in yTranslator.

### Lua Handlers Comparison/Conflicts Detector (yEditor) [XL]
Detect conflicts between OpenMW Lua handler registrations across multiple mods (e.g. two mods registering `addSkillLevelUpHandler` that both return `false`). Show conflicts in yEditor's comparison view.

### Linux Build Support [XL]
No Windows-only dependencies identified. Needs CMakeLists.txt for Linux.
