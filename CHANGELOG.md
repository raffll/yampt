# Changelog

## [0.999-alpha] - 2026-06-21

### Added
- yTranslator: GUI translation workbench with spell check, annotations, history, and translation suggestions
- yEditor: plugin conflict viewer and merged patch creator (xEdit-like)
- Sub-record schema decoding for all major record types in yEditor
- MO2 profile and OpenMW config loading in yEditor
- Neural translation engine (CTranslate2) for cell matching in base mode
- Bundled 7za.exe for archive import
- JSON dictionary format with per-entry status tracking

### Changed
- Rewritten from scratch as a Qt6 application suite
- All dependencies managed via vcpkg (Qt6, Hunspell, SentencePiece, yyjson, Catch2, nlohmann-json)
- Renamed executables: yTranslator.exe, yEditor.exe (CLI stays yampt.exe)
