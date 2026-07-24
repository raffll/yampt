# TODO

## Features

### Plugin Header Management (yEditor or yampt.cli) [L]
- Update master file sizes in headers (Wrye Mash feature — needed after cleaning)
- Update plugin version to 1.3
- Reassign masters (swap one master for another)

Simple binary surgery on the TES3 header record. `esm_reader_t` already parses it.

## Nice-to-have

### Scan Lua Files for Translatable Strings [L]
Parse OpenMW Lua scripts (l10n YAML, or string literals) to extract translatable text for dictionary creation.

### Lua Handlers Comparison/Conflicts Detector (yEditor) [XL]
Detect conflicts between OpenMW Lua handler registrations across multiple mods (e.g. two mods registering `addSkillLevelUpHandler` that both return `false`). Show conflicts in yEditor's comparison view.

### Linux Build Support [XL]
No Windows-only dependencies identified. Needs CMakeLists.txt for Linux.
