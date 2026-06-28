# Naming Conventions

## snake_case Everywhere

All identifiers use snake_case — variables, functions, methods, namespaces, file names.

## `_t` Suffix for Classes and Types

All class names and type aliases use the `_t` suffix:

```cpp
class dict_reader_t { ... };
class esm_converter_t { ... };
struct record_entry_t { ... };
using record_map_t = std::unordered_map<std::string, std::string>;
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
