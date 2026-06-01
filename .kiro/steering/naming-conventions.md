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
