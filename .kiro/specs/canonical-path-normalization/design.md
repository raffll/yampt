# Design — Canonical Path Normalization

## Context (current state after Option A)

- `string_utils::normalize_path(string_view)` replaces `\` → `/` and strips trailing slashes (preserving `/` and `C:/`).
- `session.cpp` has a file-local `static canonicalize_path` that calls `std::filesystem::weakly_canonical` — works but requires filesystem access (fails for non-existent paths, non-deterministic across mounts).
- Path comparisons throughout the codebase use raw `==` on normalized strings.
- No case-insensitive path comparison exists.
- No `.`/`..` resolution without filesystem access.

## Design Goals

Provide a purely lexical canonical path function and a platform-aware equivalence comparator in `string_utils` (yampt.core, no Qt dependency). Remove the `session.cpp` local `canonicalize_path` and replace it with the new shared function.

## API

### Location: `yampt.core/source/utility/string_utils.hpp`

```cpp
namespace string_utils
{

// Existing — separator normalization + trailing slash strip (unchanged)
inline std::string normalize_path(std::string_view input);

// New — full lexical canonicalization (R1)
std::string canonicalize_path(std::string_view input);

// New — platform-aware path equivalence (R2)
bool paths_equal(std::string_view lhs, std::string_view rhs);

} // namespace string_utils
```

### `canonicalize_path` algorithm

1. Replace `\` → `/`
2. Parse prefix:
   - If starts with `//` and third char is not `/` → UNC prefix `//server` (find next `/` to get `//server/share`)
   - If second char is `:` → drive prefix, lowercase the drive letter
   - If starts with `/` → Unix root
   - Otherwise → relative path
3. Split remainder into segments by `/`
4. Process segments:
   - Skip `.` (current directory — no-op)
   - On `..`: pop last segment if one exists and it isn't `..`; if relative path and nothing to pop, keep the `..`
   - Otherwise: push segment
5. Reassemble: prefix + join segments with `/`
6. Strip trailing `/` (same rules as `normalize_path`: preserve `/` and `X:/`)

### `paths_equal` algorithm

1. Call `canonicalize_path` on both inputs
2. On Windows (`#ifdef _WIN32`): compare with case-insensitive string comparison (`to_lower` both, then `==`)
3. On Linux: compare with `==`

### Implementation location

`canonicalize_path` involves a loop with segment splitting — not trivially inlineable. Declare in `string_utils.hpp`, define in a new `string_utils.cpp`.

This means yampt.core gains a `.cpp` file for `string_utils`. Currently everything is inline in the header. The `.cpp` will contain `canonicalize_path` and `paths_equal`; existing inline functions remain in the header.

## Migration plan

### Phase 1 — Add functions + tests

1. Create `yampt.core/source/utility/string_utils.cpp`
2. Add `canonicalize_path` and `paths_equal` implementations
3. Add to `yampt.core.vcxproj` and `.filters`
4. Write unit tests covering all path forms

### Phase 2 — Replace comparison sites

1. `file_list_t::scan_single_root`: use `canonicalize_path` for `fe.root_path`
2. `sidebar_model.cpp::build_render_model`: use `canonicalize_path` for roots_map keys
3. `sidebar_controller.cpp::scan_workspace`: use `paths_equal` for root deduplication
4. `session.cpp`: remove local `canonicalize_path`, use `string_utils::canonicalize_path`
5. `main_window.cpp` startup roots: use `paths_equal` for deduplication
6. `settings_store_t::workspace_roots` / `set_workspace_roots`: canonicalize on read/write
7. `dict_selection_dialog.cpp`: use `paths_equal` for path matching

### Phase 3 — Remove dead code

1. Remove `static canonicalize_path` from `session.cpp`
2. Audit remaining `normalize_path` + `==` sites — replace with `paths_equal` where semantically appropriate

## Edge cases

| Input | Canonical output |
|-------|-----------------|
| `""` | `""` |
| `/` | `/` |
| `C:/` | `c:/` |
| `C:\Users\..\Users\workspace\` | `c:/Users/workspace` |
| `//server/share/../share/folder/` | `//server/share/folder` |
| `/home/./user/../user/docs/` | `/home/user/docs` |
| `relative/../other` | `other` |
| `../../up` | `../../up` |
| `C://Users///file` | `c:/Users/file` |

## Rejected alternatives

### Use `std::filesystem::weakly_canonical` everywhere

Pros: handles symlinks, normalizes case on some platforms.
Cons: requires filesystem access (fails for non-existent paths), non-deterministic (different results on different machines for the same input string), not available in unit tests with synthetic paths. The existing `session.cpp` usage is kept as an optional "resolve symlinks" layer on top of the lexical canonicalization, not as the primary comparison mechanism.

### Case-fold all paths on all platforms

Would break Linux where `File.txt` and `file.txt` are genuinely different files. Case sensitivity must be platform-conditional.

### Store `std::filesystem::path` instead of `std::string`

Would require changing the entire codebase's path representation. Too invasive for the benefit. `std::string` with canonical normalization achieves the same comparison correctness.
