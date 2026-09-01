# Path Handling

Never build or compare filesystem paths by hand. Use the shared path mechanisms so separators, trailing slashes, case, and `..`/`.` segments are handled consistently.

## Joining Paths

Never concatenate a directory and a leaf with `+ "/" +`, `+ "\\" +`, or by relying on a trailing slash baked into the directory string.

- C++ `std::string` paths: `string_utils::join_path(base, leaf)` — inserts exactly one separator regardless of a trailing slash on `base` or a leading slash on `leaf`.
- Qt `QString` paths: `QDir(dir).filePath(name)`.

Directory accessors do NOT return a trailing slash. `resource_paths::*_dir()` (`config_dir`, `workspace_dir`, `models_dir`, `providers_dir`, `dictionaries_dir`, `translations_dir`) and `settings_store_t::settings_dir()` return a bare directory path — always join a leaf onto them with the helpers above, never by string addition.

## Normalizing and Comparing Paths

- To get a canonical form of a path (forward slashes, resolved `.`/`..`, normalized drive/UNC prefix): `string_utils::canonicalize_path`.
- To test whether two paths refer to the same location: `string_utils::paths_equal` (canonicalizes both and is case-insensitive on Windows). Never compare raw path strings with `==`.
- When a canonical string is needed as a map/set key with case-insensitive dedup, use `string_utils::to_lower(canonicalize_path(path))`.
- `string_utils::normalize_path` only swaps backslashes and strips trailing slashes; prefer `canonicalize_path` when segment resolution or comparison matters.

## No Local Reimplementations

Do not write local `static` helpers or lambdas that lowercase, normalize, join, or compare paths. Reuse the functions in `string_utils` (per the naming-conventions "No Duplicated Utility Functions" rule). If a needed operation is missing, add it to `string_utils` with a unit test, not inline.
