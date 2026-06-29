# Naming Conventions

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

Never compare against raw status string literals (`"untranslated"`, `"translated"`, etc.). Always use the named constants from `tools_t::status_t::` — e.g. `tools_t::status_t::untranslated`, `tools_t::status_t::adapted`. This prevents typos and keeps all status values in one place.

Target: replace the `const char *` constants with a proper `enum class status_t`. After migration, status cannot be compared to `std::string` directly — serialize/deserialize only at JSON boundaries.

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

## MVC Triplet Naming

When a feature has all three MVC layers, name them with the same base and different suffixes:

```
record_table_view_t      (view/)
record_table_model_t     (model/)
record_table_controller_t (controller/)   — if it exists
```

The base name (`record_table`) is the feature. The suffix (`_view`, `_model`, `_controller`) is the role. File names follow: `record_table_view.hpp`, `record_table_model.hpp`, `record_table_controller.hpp`.

This makes it obvious which files form a group. When only two layers exist (e.g. view + model, no controller), just use two files with matching base names.

## No Duplicate File Names Across Projects

Every source file name must be unique across the entire solution (yampt, yampt.translator, yampt.editor, yampt.tests). Even if files live in different folders, identical names cause confusion in search results, tabs, and build logs.

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
| `controller/` | Orchestration, mediation between view + model | Pure I/O serialization, stateless helpers |
| `utility/` | Pure helpers without domain or UI coupling | Qt colors, domain enums, file serialization |

Specific rules:
- Enums describing domain operations belong in `model/`, not `utility/`
- Color mappings returning `QColor` belong in `view/`, not `utility/`
- INI/config file serialization belongs in `io/`, not `controller/`
- Session-level registries (file scanning, workspace state) belong in `model/`

## No Duplicated Utility Functions

Common string/path operations must live in one shared location (`yampt/utility/string_utils.hpp`). Never reimplement as local `static` functions or lambdas:

- `to_lower(std::string_view) -> std::string`
- `normalize_path(std::string_view) -> std::string` (backslash → forward slash)
- `extract_filename(std::string_view) -> std::string_view`
- `trim(std::string_view) -> std::string`

If you need one of these, include `string_utils.hpp`. Do not write a local copy.

## No CLI Log Headers

Do not use decorative ASCII separator lines or column headers in log output. Each log line must be self-describing with labeled counters:

```cpp
// Good
tools_t::add_log("CELL: created=5, missing=2, identical=10, total=17\r\n");

// Bad
tools_t::add_log("-----------------------------------------------\r\n"
                 "          Created / Missing / Identical /   All\r\n"
                 "-----------------------------------------------\r\n");
```

## Log Message Style

All log messages use a unified prefix scheme:

- `[error]` — unrecoverable errors (file not found, parse failure)
- `[warning]` — non-fatal issues (duplicate record, replaced value)
- `[info]` — progress and status (loading, writing, done)
- No prefix — data lines (counter summaries, debug traces)

Rules:
- Lowercase prefix tag, lowercase message body
- No trailing `!` — plain statements only
- No `...` ellipsis — use present tense (`loading "file.esp"` not `loading "file.esp"..."`)
- No ad-hoc prefixes: `-->`, `->`, `Error:`, `Warning:`, `INFO:`

```cpp
// Good
tools_t::add_log("[info] loading \"Morrowind.esm\"\r\n");
tools_t::add_log("[error] cannot open \"file.esp\" for writing\r\n");
tools_t::add_log("[warning] duplicate CELL record: Balmora\r\n");

// Bad
tools_t::add_log("--> Loading \"Morrowind.esm\"...\r\n");
tools_t::add_log("--> Error loading \"file.esp\" (wrong path)!\r\n");
tools_t::add_log("Warning: duplicate CELL value Balmora\r\n");
```

## GUI Log Tab Headers

The `log_tab_t::append_log(operation_name, text)` first argument is the operation name displayed as a section header in the log panel. Use lowercase with no extra formatting:

- `"make dict"`, `"make base"`, `"convert"`, `"create"`, `"merge"`
- `"download models"`, `"spelling"`

Do not use Title Case, ALL CAPS, or mixed casing for log tab headers.

## Examples

```cpp
// Variables and functions
auto record_count = get_record_count();
void parse_dictionary(const std::string& file_path);

// Classes and types
class script_parser_t { ... };
struct filter_state_t { ... };
enum class record_type_t { cell, dial, info, fnam, text };
```

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
