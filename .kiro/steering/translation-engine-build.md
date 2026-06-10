# Translation Engine Build

## Quick Reproduction (From Scratch)

1. **Clone CTranslate2** into `external/CTranslate2/` with submodules
2. **Build CTranslate2**:
   ```powershell
   $cmake = "C:\OMEN\Morrowind\vcpkg\downloads\tools\cmake-4.3.2-windows\cmake-4.3.2-windows-x86_64\bin\cmake.exe"
   cd external/CTranslate2
   & $cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DWITH_MKL=OFF -DWITH_CUDA=OFF -DWITH_CUDNN=OFF -DWITH_DNNL=OFF -DWITH_OPENBLAS=OFF -DWITH_RUY=ON -DBUILD_CLI=OFF -DBUILD_TESTS=OFF -DOPENMP_RUNTIME=NONE
   & $cmake --build build --config Release
   ```
3. **vcpkg install** — just build the solution in VS, vcpkg manifest mode restores packages automatically using the `x64-windows-v143` triplet
4. **Download models**:
   ```powershell
   python -m pip install torch transformers ctranslate2 huggingface_hub sentencepiece
   python download_models.py
   ```
5. **Copy DLL**: `copy external\CTranslate2\build\Release\ctranslate2.dll x64\Release\`
6. **Build solution** in VS (Release x64)
7. **Run** from repo root (so `models/` relative path works)

## VS 18 STL Bug (Root Cause of All Build Issues)

VS 18 (Visual Studio 2026) introduced internal STL dispatch symbols that cannot be resolved at link time:
- `__std_rotate`
- `__std_find_first_not_of_trivial_pos_1`

These are called by abseil (a sentencepiece dependency). They don't exist in any linkable `.lib`. This is a VS 18 toolchain bug.

## Solution: Custom vcpkg Triplet with v143 Toolset

Force vcpkg to compile all packages using the v143 toolset (MSVC 14.44) instead of v144 (VS 18). The v143 STL doesn't have the broken dispatch symbols.

Files:
- `triplets/x64-windows-v143.cmake` — `VCPKG_PLATFORM_TOOLSET v143`, dynamic CRT
- `vcpkg-configuration.json` — `"overlay-triplets": ["./triplets"]`
- All vcxprojs: `<VcpkgTriplet>x64-windows-v143</VcpkgTriplet>`
- SDL2 include hardcoded: `$(SolutionDir)vcpkg_installed\x64-windows-v143\x64-windows-v143\include\SDL2`
- vcpkg auto-links sentencepiece + abseil — no manual lib listing needed
- Do NOT add abseil libs manually. Do NOT add sentencepiece.lib manually. vcpkg handles it.
- Do NOT set RuntimeLibrary to MultiThreaded. Keep default (/MD).

## CTranslate2

- Source at `external/CTranslate2/`
- Built separately via CMake (VS 18 2026 generator, Release, Ruy backend)
- Produces `ctranslate2.dll` + `ctranslate2.lib` at `external/CTranslate2/build/Release/`
- Must copy `ctranslate2.dll` to output dir (`x64/Debug/` or `x64/Release/`) for runtime
- Include paths: `$(SolutionDir)external\CTranslate2\include` and `$(SolutionDir)external\CTranslate2\build`
- Link: `ctranslate2.lib` with lib dir `$(SolutionDir)external\CTranslate2\build\Release`

## Model Download

Prerequisites:
```powershell
python -m pip install torch transformers ctranslate2 huggingface_hub sentencepiece
```

Model repos:
- EN→DE: `Helsinki-NLP/opus-mt-en-de`
- EN→PL: `gsarti/opus-mt-tc-en-pl` (not Helsinki-NLP — `opus-mt-en-pl` doesn't exist as standalone)
- EN→FR: `Helsinki-NLP/opus-mt-en-fr`

Script `download_models.py` in repo root:
```python
import ctranslate2
import os
from huggingface_hub import hf_hub_download

models = [
    ("en-de", "Helsinki-NLP/opus-mt-en-de"),
    ("en-pl", "gsarti/opus-mt-tc-en-pl"),
    ("en-fr", "Helsinki-NLP/opus-mt-en-fr"),
]

for lang_pair, repo in models:
    base_dir = f"models/{lang_pair}"
    model_dir = f"{base_dir}/model"
    os.makedirs(base_dir, exist_ok=True)
    converter = ctranslate2.converters.TransformersConverter(repo)
    converter.convert(model_dir, quantization="int8", force=True)
    hf_hub_download(repo, "source.spm", local_dir=base_dir)
    hf_hub_download(repo, "target.spm", local_dir=base_dir)
```

Run: `python download_models.py`

Conversion notes:
- Do NOT use `OpusMTConverter` — it expects Marian format (`decoder.yml`), HuggingFace models use transformers format
- Do NOT use `--model_name` CLI flag — newer ctranslate2 changed to `--model_dir` and expects local path
- Use `TransformersConverter` Python API directly — it downloads + converts in one step

Expected structure:
```
models/
├── en-de/
│   ├── model/       (CTranslate2 model.bin + config)
│   ├── source.spm
│   └── target.spm
├── en-pl/
└── en-fr/
```

## Runtime

- `ctranslate2.dll` must be next to the exe
- `models/` directory must be relative to working directory (or exe location)
- If `translation_engine_t::load()` is not called or model is missing, dict_creator falls back to word-overlap heuristic (original behavior)

## What NOT to Do

- Do NOT use vcpkg's default `x64-windows` triplet — abseil triggers VS 18 STL bug
- Do NOT add `sentencepiece` to `external/` and build from source — same abseil issue
- Do NOT mix `/MT` and `/MD` — causes RuntimeLibrary mismatch with vcpkg deps
- Do NOT add abseil .lib files manually to AdditionalDependencies
- Do NOT use `libcpmt.lib` as a workaround
