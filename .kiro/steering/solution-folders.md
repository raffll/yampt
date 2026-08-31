# Solution Folders

All auxiliary files live in the solution root (`yampt/`), not inside project subfolders.

- `scripts/` — PowerShell and Python scripts (pack_model.ps1, download_models.py, finetune_model.py, pack_release.ps1)
- `models/` — CTranslate2 translation models (nllb-600M/, etc.)
- `dictionaries/` — holds only `LICENSE`; the `.aff`/`.dic` files are generated at build time from the `external/dictionaries` submodule and are gitignored
- `external/` — git submodules for third-party sources: `external/CTranslate2` (`OpenNMT/CTranslate2`, pinned release tag) and `external/dictionaries` (`wooorm/dictionaries`, source of all Hunspell spellcheck dictionaries). Also holds local-only, gitignored content (built CTranslate2 artifacts, translation models, `7za.exe`).
- `build/` — output of pack/release scripts (zip files, release artifacts)

Never place scripts, models, or dictionaries inside `yampt.translator/`, `yampt.editor/`, or other project subdirectories.

All pack scripts (pack_release.ps1, pack_model.ps1) must write their output to `build/` in the solution root.
