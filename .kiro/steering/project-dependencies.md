# Project Dependencies

## Dependency Graph

```
yampt.core (StaticLibrary → yampt.core.lib)
   ↑ pure C++ — no Qt, no other projects
   │
yampt.qt (StaticLibrary → yampt.qt.lib)
   ↑ depends on: yampt.core + Qt
   │
├── yampt.cli      → links: yampt.core.lib
├── yampt.editor   → links: yampt.core.lib + yampt.qt.lib
├── yampt.translator → links: yampt.core.lib + yampt.qt.lib
└── yampt.tests    → links: yampt.core.lib + yampt.qt.lib
                     (+ compiles yampt.translator/yampt.editor .cpp directly)
```

## Rules

### yampt.core — Pure C++ Library

- **No Qt includes** (`QSettings`, `QString`, `QColor`, `QDir`, `QFile`, etc.)
- **No includes from other yampt projects** (yampt.qt, yampt.editor, yampt.translator)
- **Only external dependencies**: CTranslate2, vcpkg C++ libraries (sentencepiece, abseil), standard library
- **Include paths**: `$(ProjectDir)source`, `$(SolutionDir)external`, CTranslate2 include/build
- When a new `.cpp` file is added here, all consuming projects get it automatically through `yampt.core.lib`. No additional edits needed.

### yampt.qt — Qt Bridge Library

- **Depends on**: yampt.core (headers only via include path) + Qt6
- **Contains**: Qt-dependent utilities that wrap or extend core types with Qt features
- **Current contents**: `theme_system`, `conflict_types` (QColor helpers), `app_settings` (QSettings wrapper)
- **Include paths**: `$(ProjectDir)source`, `$(SolutionDir)yampt.core\source`, Qt6 includes
- When adding a Qt-dependent class that other projects need, put it here.

### yampt.cli — Console Application

- **Depends on**: yampt.core.lib (linked)
- **Does NOT depend on Qt or yampt.qt**
- **Contains**: CLI entry point, user_interface_t
- **Only own source files** — no cross-project `.cpp` compilation

### yampt.editor — Qt GUI Application (yEditor)

- **Depends on**: yampt.core.lib + yampt.qt.lib (linked via ProjectReference)
- **Contains**: Plugin conflict viewer, merged patch editor
- **Only own source files** — no cross-project `.cpp` compilation
- Include paths include `yampt.core\source` and `yampt.qt\source` for headers

### yampt.translator — Qt GUI Application (yTranslator)

- **Depends on**: yampt.core.lib + yampt.qt.lib (linked via ProjectReference)
- **Contains**: Translation workbench, dictionary editor
- **Only own source files** — no cross-project `.cpp` compilation
- Include paths include `yampt.core\source` and `yampt.qt\source` for headers

### yampt.tests — Test Application

- **Depends on**: yampt.core.lib + yampt.qt.lib (linked via ProjectReference)
- **Also compiles .cpp directly from**: yampt.translator, yampt.editor (because those are Applications, not libraries — no .lib to link)
- This is the ONLY project allowed to compile `.cpp` files from other projects
- Include paths include all project source directories for header access

## Adding New Files

| File depends on... | Put it in... |
|---|---|
| Nothing / standard C++ only | `yampt.core` |
| CTranslate2 | `yampt.core` |
| Qt classes (QColor, QSettings, QWidget, etc.) but is shared | `yampt.qt` |
| Qt + is specific to yEditor UI | `yampt.editor` |
| Qt + is specific to yTranslator UI | `yampt.translator` |
| Needs testing | If it's in yampt.core, tests get it free via lib. If it's in translator/editor, add the `.cpp` to yampt.tests vcxproj. |

## Never Do

- Never add Qt includes to yampt.core
- Never add `#include` of yampt.qt/yampt.editor/yampt.translator headers in yampt.core
- Never compile `.cpp` from yampt.core in other projects (they link the lib)
- Never compile `.cpp` from yampt.qt in other projects (they link the lib)
- Never add yampt.core.lib or yampt.qt.lib as AdditionalDependencies without also adding the ProjectReference (build order)

## Merge Output Paths (Hardcoded)

- **Folder mode**: `{selected_folder}/Merged Patch.esp`
- **MO2 mode**: `{MO2_root}/overwrite/Merged Patch.esp`
- **OpenMW mode**: `{openmw.cfg_dir}/data/Merged Patch.esp`

No settings for these paths — they are automatic and deterministic.
