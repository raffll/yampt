# Requirements — Canonical Path Normalization

## Overview

yampt runs on Windows and Linux, and handles paths from multiple sources: user settings (QSettings), Qt APIs (`QDir`, `QCoreApplication::applicationDirPath`), the C++ filesystem library (`std::filesystem::path`), and raw user input. These sources produce paths with inconsistent separators, trailing slashes, case differences, and dot segments. The current `string_utils::normalize_path` only replaces backslashes with forward slashes — insufficient for reliable path comparison and deduplication across platforms.

This feature introduces a full canonical path normalization function and a path comparison function that produce consistent, deterministic representations regardless of how a path was obtained, on both Windows and Linux.

## Problem Statement

Paths that refer to the same filesystem location can differ in:
1. Separator style (`\` vs `/`)
2. Trailing slashes (`C:/Users/workspace/` vs `C:/Users/workspace`)
3. Case on Windows (`C:/Users` vs `c:/users`)
4. Dot segments (`C:/Users/../Users/workspace` vs `C:/Users/workspace`)
5. Redundant separators (`C://Users///workspace` vs `C:/Users/workspace`)
6. UNC prefix representation (`\\server\share` vs `//server/share`)

Raw string comparison (`==`) fails when any of these differ, causing bugs like the duplicate Workspace sidebar node.

## Terminology

- **Normalized path** — a path with backslashes replaced by forward slashes and trailing slashes stripped. The current `normalize_path` output after the Option A fix.
- **Canonical path** — a fully resolved, unique representation of a filesystem location: normalized separators, no trailing slash, no `.`/`..` segments, no redundant separators, consistent case on case-insensitive filesystems.
- **Path equivalence** — two path strings refer to the same filesystem location regardless of representational differences.

## Requirements

### R1 — Canonical path function

**User story:** As a developer, I want a single function that produces the canonical form of any path, so I can use it as a map key or comparison target without worrying about representational differences.

Acceptance criteria:
1. The function SHALL replace all backslashes with forward slashes.
2. The function SHALL strip trailing slashes, preserving Unix root `/` and Windows drive roots (e.g. `C:/`).
3. The function SHALL collapse redundant consecutive separators (e.g. `C://Users///file` → `C:/Users/file`), preserving the UNC prefix `//` at position 0-1.
4. The function SHALL resolve `.` and `..` segments lexically (without filesystem access).
5. On Windows, the function SHALL lowercase the drive letter (e.g. `C:/` → `c:/`).
6. The function SHALL NOT perform case folding on the path body — case-insensitive comparison is a separate concern (R2).
7. The function SHALL handle UNC paths (`//server/share/folder`) correctly, preserving the double-slash prefix.
8. The function SHALL handle empty input by returning an empty string.
9. The function SHALL be a pure function with no filesystem I/O (no `stat`, no `realpath`, no `weakly_canonical`).

### R2 — Path equivalence comparison

**User story:** As a developer, I want to compare two paths for filesystem equivalence without manually normalizing both, so path deduplication works correctly on all platforms.

Acceptance criteria:
1. The function SHALL return true when two paths refer to the same location after canonicalization.
2. On Windows (compile-time or runtime detection), the function SHALL perform case-insensitive comparison on the full path.
3. On Linux, the function SHALL perform case-sensitive comparison.
4. The function SHALL accept `std::string_view` parameters for zero-copy usage.
5. The function SHALL be usable in `if` conditions, map comparators, and set predicates.

### R3 — Replace existing normalize_path usages at comparison sites

**User story:** As a developer, I want all path comparison sites to use the canonical/equivalence functions, so no path bugs remain from representational differences.

Acceptance criteria:
1. All sites that compare paths with `==` for deduplication or lookup SHALL use `paths_equal` or canonical keys.
2. All `std::map<std::string, ...>` keyed by path SHALL use the canonical form as key.
3. The file_list root deduplication, sidebar model roots_map, session document lookup, and settings workspace_roots persistence SHALL all use canonical paths.
4. No behavioral regression for existing tests.

### R4 — Backward compatibility

**User story:** As a user, I want my existing settings and workspace paths to keep working after the upgrade.

Acceptance criteria:
1. Paths stored in QSettings (workspace_roots, recent files) SHALL be read and canonicalized on load — no manual migration needed.
2. Paths stored in dictionary JSON files (file references) SHALL be canonicalized on read.
3. The existing `normalize_path` function SHALL remain available for code that only needs separator normalization (e.g. display purposes).

### R5 — Cross-platform correctness

**User story:** As a developer, I want the path functions to work correctly on both Windows and Linux without platform-specific code at call sites.

Acceptance criteria:
1. The canonical function SHALL compile and produce correct output on both Windows and Linux.
2. Platform-specific behavior (drive letters, UNC paths, case sensitivity) SHALL be handled internally via compile-time `#ifdef` or `constexpr` platform detection.
3. Unit tests SHALL cover Windows-style paths (`C:\Users\...`), Linux-style paths (`/home/user/...`), and UNC paths (`\\server\share\...`) regardless of build platform.
