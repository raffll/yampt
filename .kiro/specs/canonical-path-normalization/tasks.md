# Tasks — Canonical Path Normalization

## Phase 1 — Core implementation

- [x] Create `yampt.core/source/utility/string_utils.cpp` with `canonicalize_path` and `paths_equal` implementations
- [x] Add `string_utils.cpp` to `yampt.core.vcxproj` and `yampt.core.vcxproj.filters`
- [x] Add declarations to `string_utils.hpp` (non-inline, since defined in .cpp)
- [x] Write unit tests for `canonicalize_path` covering: empty, Unix root, drive roots, dot segments, double-dot segments, redundant separators, UNC paths, trailing slashes, relative paths with `..`
- [x] Write unit tests for `paths_equal` covering: case differences on Windows, identical paths, trailing slash differences, separator differences

## Phase 2 — Migration

- [x] `file_list_t::scan_single_root` — use `canonicalize_path` for `fe.root_path` assignment
- [x] `sidebar_model.cpp::build_render_model` — use `canonicalize_path` for `roots_map` keys
- [x] `sidebar_controller.cpp` — use `paths_equal` for workspace root deduplication
- [x] `session.cpp` — remove local `canonicalize_path`, replace with `string_utils::canonicalize_path`
- [x] `main_window.cpp` startup — use `paths_equal` for root deduplication check
- [x] `settings_store_t` — canonicalize workspace_roots on read and write
- [x] `dict_selection_dialog.cpp` — use `paths_equal` for saved-order matching

## Phase 3 — Cleanup

- [x] Audit all remaining `normalize_path` + `==` comparison sites — replace with `paths_equal` where the intent is filesystem equivalence
- [x] Update `resource_paths::workspace_dir()` and other `resolve_*` functions to NOT append trailing slash (now redundant since `canonicalize_path` strips it)
- [x] Verify no test regressions
