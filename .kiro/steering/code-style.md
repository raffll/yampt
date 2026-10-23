# Code Style

## File Organization

> Scope: the size limits below (max 1000 lines per file, max 50 lines per function, max 3 nesting levels, max 2 arguments) apply to **new and modified code**. Existing files/functions that already exceed a limit are grandfathered — do not retroactively split them, and do not flag them in audits. When you add a new file or function, it must fit. When you substantially edit an existing oversized unit, bring the part you touch into compliance (extract what you're changing) rather than growing it further; a full split is optional, not required.

- One file, one responsibility
- Max 1000 lines per file
- One class per file
- MVC pattern with files sorted in different folders
- Utility classes and functions separated from model classes
- Extract all types, enums, and lists of names into separate miscellaneous classes
- Extract all names and messages into YAML
- When splitting a large file, always extract into a new class with its own .hpp/.cpp pair — never split a single class across multiple .cpp files

Exceptions to the one-class-one-file rule:
- **Qt main windows** — a `_setup.cpp` companion file is acceptable for UI construction boilerplate (setup_*, connect_* methods). The class has one responsibility but Qt widget creation is verbose.
- **Qt tree models with complex decode** — a single model class may exceed 1000 lines if the decode logic is inseparable from the model's presentation role and has no external consumers.

## Main Window Anti-Gravity Rule

Never add new logic directly to an app's main-window class. These classes are signal routers — they connect UI events to controllers. New features go on the appropriate controller or a new controller. The main window only:
- Constructs views and controllers
- Connects signals to controller methods
- Reads controller results to update views

If a new feature needs orchestration (showing dialogs, running operations, updating multiple views), create or extend a controller for it. The main window calls one method on the controller — it does not contain the logic itself.

## Function Design

- Max 50 lines per function
- Max 3 nesting levels per function
- Max 2 function arguments — if more are needed, create a struct
- Remove unused function arguments — do not leave unnamed or commented-out parameters
- Always do testable interfaces
- Unit test for every function
- Every algorithm must be callable from a unit test without instantiating heavy dependencies (no file I/O, no UI, no loaded plugins). Extract pure logic into public static methods on a dedicated class that accept data via parameters and return results. Class methods that need infrastructure (loading files, accessing plugin arrays) are thin wrappers that collect data and delegate to the testable class.

## Naming

> Scope: the minimum-name-length rule (no name shorter than 5 characters) applies to **new and modified code** — new identifiers must comply. Existing short names are grandfathered; do not mass-rename them and do not flag them in audits. Rename a short name when you are already editing that symbol's surrounding code, not as a standalone sweep.

- No name shorter than 5 characters — no abbreviations
- Names must always match what the variable actually is; if reused, generalize the name
- Do not use meaningless words like `data`, `manager`, `provider`
- Use one word for one meaning — e.g. if "model" refers to both MVC and AI, explicitly name them differently (e.g. `view_model` vs `translation_model`)
- Design pattern names should be explicitly named as patterns (e.g. `_factory`, `_observer`)
- Always name `first`/`second` explicitly via structured binding or `const auto &`
- Use `m_` prefix for class member variables (not for POD/helper struct fields — those use plain snake_case)
- Use `ptr_` prefix for pointers
- Use `it_` prefix for iterators
- All names must be meaningful — do not hesitate to create very long names

## snake_case Everywhere

All identifiers use snake_case — variables, functions, methods, namespaces, file names.

## `_t` Suffix for All Classes and Types

All class names, type aliases, and interfaces use the `_t` suffix — no distinction between concrete and abstract:

```cpp
class dict_reader_t { ... };
class esm_converter_t { ... };
struct record_entry_t { ... };
using record_map_t = std::unordered_map<std::string, std::string>;

class document_t { virtual ~document_t() = default; ... };  // pure interface
class row_source_t { virtual ~row_source_t() = default; ... };  // pure interface
class translator_t { virtual ~translator_t() = default; ... };  // pure interface
```

Template arguments also use the `_t` suffix:

```cpp
template <typename container_t>
void process(const container_t & items);

template <typename value_t, typename key_t>
auto lookup(const key_t & key) -> value_t;
```

## File Names

Source files use snake_case with underscores separating words:

- `dict_reader.hpp` / `dict_reader.cpp`
- `esm_converter.hpp` / `esm_converter.cpp`
- `script_parser.hpp` / `script_parser.cpp`

## Enum Values

Enum values are lowercase snake_case:

```cpp
enum class rec_type_t { cell, dial, info, fnam, text, default_val, unknown };
enum class creator_mode_t { base };
enum class encoding_t { unknown, windows_1250 };
```

## Dictionary Entry Fields

The canonical field names for dictionary entries are `key_text`, `old_text`, `new_text`, `status`. Use these consistently:

- Struct fields: `entry.key_text`, `entry.old_text`, `entry.new_text`
- Local variables extracted from ESM: `const auto & key_text = esm.get_key().text;` (never `id`)
- JSON serialization keys: `"key"`, `"old"`, `"new"`, `"status"`

## Function Arguments Must Match in .hpp and .cpp

Parameter names in declarations (`.hpp`) must be identical to the corresponding definitions (`.cpp`). Never omit parameter names in declarations. When a mismatch exists, use the more descriptive snake_case name in both files.

## Use Named Constants for Status Strings

Never compare against raw status string literals (`"untranslated"`, `"translated"`, etc.). Always use the shared named status constants. This prevents typos and keeps all status values in one place. Serialize/deserialize status only at JSON boundaries — do not compare it to a raw string elsewhere.

## View Class Suffix: `_view_t`

All Qt widget classes in the `view/` folder use the `_view_t` suffix. Never use position-specific suffixes like `_panel`, `_tab`, `_bar`, `_widget` — layout can change and the name becomes misleading.

Exceptions (describe what the widget IS, not where it sits):
- `line_number_gutter_t` — it's a gutter

Examples:
```cpp
// Good
class annotations_view_t : public QWidget { ... };
class history_view_t : public QWidget { ... };
class log_view_t : public QWidget { ... };
class sidebar_view_t : public QWidget { ... };
class status_filter_view_t : public QWidget { ... };

// Bad
class annotations_panel_t : public QWidget { ... };
class log_tab_t : public QWidget { ... };
class status_filter_bar_t : public QWidget { ... };
class sidebar_widget_t : public QWidget { ... };
```

## Folder-Responsibility Naming

Each file is named for its own responsibility, not forced to match a sibling. If two files happen to share a base name (like `sidebar_view` + `sidebar_model`), the shared prefix makes the relationship obvious — but it's not required.

Folder placement determines the role:
- `view/` — Qt widgets. Named after what they display.
- `model/` — Data structures, documents, domain logic, row filtering. Named after what they hold or transform.
- `editor/` — Editing orchestration. Named after what they do to the active record.
- `highlighter/` — Text coloring, annotations, glossary, grammar checks. Named after what they mark.
- `dialog/` — Modal dialogs. Named after what they ask.
- `translator/` — Translation backends. Named after the provider.
- `io/` — File I/O, config persistence, external process invocation.
- `utility/` — Pure helpers without domain or UI coupling.

## No Duplicate File Names Within the Same Project

Source file names must be unique within a single project. Files with the same name in different projects (e.g. `appearance_settings_view` in yampt.translator and yampt.editor) are acceptable — they serve analogous roles in their respective applications.

## No Generic Class Names

Avoid meaningless words in class names: `manager`, `provider`, `service`, `handler`, `helper`, `data`, `info`, `context`, `item`, `entry`, `state`.

Acceptable suffixes: `_utils`, `_view`, `_model`, `_controller`.

Name the class after what it IS or what it DOES:
- `history_manager_t` → `edit_history_t` (it IS the history)
- `annotation_manager_t` → `glossary_t` (it builds and queries a glossary)
- `find_replace_service_t` → `find_replace_t` (it finds and replaces)
- `translation_provider_t` → `translator_t` (it translates)
- `search_engine_t` → `row_filter_t` (it filters rows)
- `validation_manager_t` → `byte_limit_validator_t` (it validates byte length)

Exception: `_controller_t` is acceptable for MVC controllers that mediate between view and model (e.g. `editor_controller_t`).

## File Placement Rules

Each folder has a clear responsibility. A file belongs to the folder that matches its single concern:

| Folder | Contains | Does NOT contain |
|--------|----------|------------------|
| `io/` | File format readers/writers (disk ↔ memory) | Domain logic, GUI types, enums |
| `model/` | Data structures, domain types, business logic | File I/O, Qt widgets, UI formatting |
| `view/` | Qt widgets, display formatting, colors | Business logic, file I/O |
| `editor/` | Orchestration, mediation between view + model | Pure I/O serialization, stateless helpers |
| `utility/` | Pure helpers without domain or UI coupling | Qt colors, domain enums, file serialization |

Specific rules:
- Enums describing domain operations belong in `model/`, not `utility/`
- Color mappings returning `QColor` belong in `view/`, not `utility/`
- INI/config file serialization belongs in `io/`, not `editor/`
- Session-level registries (file scanning, workspace state) belong in `model/`

## Types and Safety

- Do not mix types — e.g. enum with int, or string disguised as one type
- Always use smart pointers instead of raw pointers
- Use `constexpr` whenever possible
- Use `string_view` where appropriate
- Always create `const` ref or `const` variable unless mutation is necessary
- Do not use magic numbers
- Do not use magic offsets
- Do not use pointer arithmetic without explicitly stating that you are doing so

## `const auto &` by Default

Always use `const auto &` for local variables unless you explicitly need a copy. Only omit `const` or `&` when mutation or ownership transfer is intended.

```cpp
// Good
const auto & key_text = esm.get_key().text;
const auto & dict = creator.get_dict();

// Good — explicit copy needed
auto copy = original_string;
copy += "_suffix";

// Bad — unnecessary copy
auto key_text = esm.get_key().text;
```

## Code Quality

- SOLID, KISS, DRY principles
- Clean code
- Do not cut corners or do things the easiest way
- Every line of code must be understandable without reading the entire function
- Modernize — use C++20
- No comments unless the code cannot be explained by function or variable names alone. If a comment is unavoidable, keep it to one short line.
- No decorative comment banners (dashed lines, boxes, ASCII art).
- One class = one `.hpp` + one `.cpp`. Never split a class across multiple `.cpp` files. If a class exceeds 1000 lines, it has more than one responsibility — extract a new class, don't add a second `.cpp`.
- When hiding a UI feature (settings page, combo box, panel), fully remove it from the layout — do not just call `setVisible(false)`. Hidden widgets still occupy space, intercept events, and cause visual glitches. Remove the `addWidget`/`addTab` call entirely; keep the object alive only if its API is still used internally.

## Localization

All user-visible strings in yampt.translator and yampt.editor must be wrapped for Qt translation:

- QWidget subclasses: `tr("text")`
- Non-QObject classes: `QCoreApplication::translate("yTranslator", "text")` or `"yEditor"`
- Never leave raw string literals in UI code (menus, tooltips, labels, messages, dialog titles, button text)
- This includes: combo box items (`addItem`), checkbox labels, form row labels (`addRow`), context menu actions (`addAction`), `QInputDialog` titles and prompts, list widget category items, and toolbar/statusbar text
- Log messages stay English — they are developer-facing
- Shortcut key sequences (`"Ctrl+S"`, `"F10"`) are not translated
- Settings keys and internal identifiers are not translated
- Internal toolbar object names (`addToolBar("Main")`) are not translated

## Classes vs Namespaces

- **Class (`_t` suffix)** — has mutable state (member variables, including static mutable). Gets instantiated or manages a resource.
- **Namespace (`snake_case`)** — groups related functions with no shared mutable state. Constants (including `extern const`) are fine.
- **Struct (`_t` suffix)** — POD/data carriers with no behavior beyond trivial accessors.

Decision:
1. Does it hold mutable state? → class
2. Does it get instantiated? → class
3. Is it just functions + constants? → namespace

Never use a class with only static methods — use a namespace instead.

## Function Placement

- File-local `static` functions in `.cpp` — implementation-only helpers that serve one file. Internal types used by these helpers also live in the `.cpp`.
- Namespace functions in `.hpp` — shared logic callable from multiple files. Declared in a namespace in the header, defined in the `.cpp`.
- Class methods — behavior that operates on the class's own state.

Keep headers minimal. Only declare what external callers or tests need.

## Qt Boundary Rule

Qt-mandated names stay in camelCase. Everything you write fresh uses snake_case.

| Context | Convention | Example |
|---------|-----------|---------|
| Qt virtual overrides | camelCase (Qt's name) | `keyPressEvent`, `eventFilter`, `data` |
| Qt signal references in `connect()` | camelCase | `&QAction::triggered` |
| Your own signals | snake_case | `row_selected`, `filters_changed` |
| Your own slots / methods | snake_case | `on_row_selected`, `setup_toolbar` |
| Qt base class API calls | camelCase (Qt's API) | `menuBar()`, `statusBar()`, `addWidget()` |
| Your own classes | snake_case + `_t` | `main_window_t`, `filter_tree_t` |
| Local Qt objects | snake_case | `auto * file_menu = menuBar()->addMenu(...)` |

Rule: you do not rename what Qt gives you, you name what you create yourself. A developer reading `keyPressEvent` immediately knows it's a Qt override. A developer reading `on_row_selected` knows it's project code.

## No Deep Nesting

Never write deeply nested `if` blocks. Use early returns, `continue`, and `break` to flatten logic.

Always add a blank line after `continue`, `return`, or `break` — unless it's the last statement in the block.

Bad:
```cpp
if (base_dict)
{
    auto it = base_dict->find(type);
    if (it != base_dict->end())
    {
        const auto * entry = it->second.find(id);
        if (entry != nullptr)
        {
            if (entry->original == original)
            {
                // deep logic here
            }
        }
    }
}
```

Good:
```cpp
if (!base_dict)
    return;

auto it = base_dict->find(type);
if (it == base_dict->end())
    return;

const auto * entry = it->second.find(id);
if (entry == nullptr)
    return;

if (entry->original != original)
    return;

// flat logic here
```

Bad (no blank line after continue):
```cpp
if (!esm.get_key().exist)
    continue;
if (esm.get_key().text != "sDefaultCellname")
    continue;
```

Good:
```cpp
if (!esm.get_key().exist)
    continue;

if (esm.get_key().text != "sDefaultCellname")
    continue;
```

Exception — last statement in block needs no blank line:
```cpp
if (!esm.get_value().exist)
    continue;
}
```

## Fixing Compilation/Linker Errors

When fixing compile or linker errors, do NOT change the project architecture. Do NOT restructure include hierarchies, move code between projects, convert inline functions to out-of-line, or change how projects link against each other. The fix must preserve the existing structure — if a project compiles .cpp files directly from another project folder, the fix is to add the missing .cpp file to that project. If a header needs a forward declaration instead of a full include, use a forward declaration. Never "simplify" the build by changing how projects consume each other's code.

## Path Handling

Never build or compare filesystem paths by hand. Use the shared path helpers so separators, trailing slashes, case, and `..`/`.` segments are handled consistently.

### Joining Paths

Never concatenate a directory and a leaf with `+ "/" +`, `+ "\\" +`, or by relying on a trailing slash baked into the directory string. Use the shared path-join helper for C++ `std::string` paths, and the Qt file-path API for `QString` paths — each inserts exactly one separator regardless of trailing/leading slashes.

Directory accessors return a bare directory path with NO trailing slash — always join a leaf onto them with the helpers above, never by string addition.

### Normalizing and Comparing Paths

- To get a canonical form (forward slashes, resolved `.`/`..`, normalized drive/UNC prefix), use the shared canonicalize helper.
- To test whether two paths refer to the same location, use the shared path-equality helper (it canonicalizes both and is case-insensitive on Windows). Never compare raw path strings with `==`.
- When a canonical string is needed as a map/set key with case-insensitive dedup, lowercase the canonical form.

### No Local Reimplementations

Do not write local `static` helpers or lambdas that lowercase, normalize, join, or compare paths. Reuse the shared string/path utilities (per the "No Duplicated Utility Functions" rule below). If a needed operation is missing, add it to the shared utility module with a unit test, not inline.

## No Duplicated Utility Functions

Common string/path operations (lowercasing, normalizing backslashes, extracting a filename, trimming, joining, canonicalizing, comparing) must live in one shared string-utility module. Never reimplement them as local `static` functions or lambdas. If you need one, include the shared module; if it is missing an operation, add it there with a unit test rather than writing a local copy.

## No CLI Log Headers

Do not use decorative ASCII separator lines or column headers in log output. Each log line must be self-describing with labeled counters:

```
Good:  CELL: created=5, missing=2, identical=10, total=17

Bad:   -----------------------------------------------
                 Created / Missing / Identical /   All
       -----------------------------------------------
```

## Log Message Style

All log messages use a unified prefix scheme:

- `[error]` — unrecoverable errors (file not found, parse failure)
- `[warning]` — non-fatal issues (duplicate record, replaced value)
- `[info]` — progress and status (loading, writing, done)
- `[debug]` — temporary diagnostic messages (gated behind the debug flag — see engineering-discipline.md)
- No prefix — data lines (counter summaries)

Rules:
- Lowercase prefix tag, lowercase message body
- No ad-hoc prefixes: `-->`, `->`, `Error:`, `Warning:`, `INFO:`

```
Good:  [info] loading "Morrowind.esm"
       [error] cannot open "file.esp" for writing
       [warning] duplicate CELL record: Balmora

Bad:   --> Loading "Morrowind.esm"
       Error: loading "file.esp" (wrong path)
       Warning: duplicate CELL value Balmora
```

## GUI Log Tab Headers

The operation name shown as a section header in the GUI log panel uses lowercase with no extra formatting:

- `"make dict"`, `"make base"`, `"convert"`, `"create"`, `"merge"`
- `"download models"`, `"spelling"`

Do not use Title Case, ALL CAPS, or mixed casing for log tab headers.


