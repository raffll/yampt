# Building yampt on Linux

## Dependencies

### Arch / Manjaro

From official repos:
```bash
sudo pacman -S cmake qt6-base hunspell nlohmann-json yyjson catch2
```

From AUR (via `yay` or `paru`):
```bash
yay -S ctranslate2 sentencepiece rapidcheck
```

### Ubuntu / Debian

```bash
sudo apt install build-essential cmake git pkg-config \
    qt6-base-dev libhunspell-dev nlohmann-json3-dev \
    libgl1-mesa-dev libxkbcommon-dev libfontconfig1-dev libssl-dev
```

sentencepiece, ctranslate2, yyjson, catch2, and rapidcheck may need to be built from source on Debian-based systems.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Output binaries are in `build/bin/`:
- `yampt` — CLI tool
- `yampt-translator` — translation workbench GUI
- `yampt-editor` — plugin editor GUI
- `yampt-tests` — test runner

## Install

```bash
sudo cmake --install build
```

Installs to:
- `/usr/bin/` — binaries
- `/usr/share/yampt/` — languages.json, providers/, dictionaries/
- `/usr/share/doc/yampt/` — README, CHANGELOG, manuals
- `/usr/share/licenses/yampt/` — LICENSE

## AUR Package

```bash
cd linux
makepkg -si
```

## File Locations

| Purpose | Linux | Windows |
|---------|-------|---------|
| Config (ini files) | `~/.yampt/` | exe directory |
| Workspace | `~/.yampt/workspace/` | exe directory |
| Models | `~/.yampt/models/` | exe directory |
| Crash logs | `~/.yampt/` | exe directory |
| Shared data | `/usr/share/yampt/` | exe directory |
| User overrides | `~/.yampt/` | exe directory |

Resource lookup order:
1. `~/.yampt/` — user overrides (custom providers, extra dictionaries)
2. `/usr/share/yampt/` — system defaults installed by the package
3. Executable directory — fallback for local dev builds

## Notes

- The first build after installing dependencies is a full compile (~5 min). Subsequent builds are incremental.
- Translation models are not included in the package (too large). Download them separately into `~/.yampt/models/`.
- CTranslate2 from AUR builds with OpenBLAS and Ruy backends (no CUDA).
- Qt6 Fusion style is used on all platforms.
