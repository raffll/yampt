"""
Renames all trailing-underscore member variables to m_ prefix across the yampt codebase.

Usage: python scripts/rename_trailing_underscore.py [--dry-run]

Pass --dry-run to see what would be changed without modifying files.
"""

import re
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXTENSIONS = ('.hpp', '.cpp', '.h')
SKIP_DIRS = {'external', '.git', 'x64', 'build', 'vcpkg_installed', '.kiro', 'models', 'dictionaries'}

DRY_RUN = '--dry-run' in sys.argv


def collect_source_files():
    files = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for f in filenames:
            if any(f.endswith(ext) for ext in EXTENSIONS):
                files.append(os.path.join(dirpath, f))
    return files


def find_static_members(files):
    """Find member names that are declared as static in headers."""
    static_pattern = re.compile(r'\bstatic\b.+?(\w+_)\s*[=;{\[]')
    static_names = set()

    for path in files:
        if not path.endswith('.hpp') and not path.endswith('.h'):
            continue
        with open(path, 'r', encoding='utf-8', errors='ignore') as fh:
            for line in fh:
                m = static_pattern.search(line)
                if not m:
                    continue
                name = m.group(1)
                if name.endswith('_') and not name.startswith('m_') and not name.startswith('s_'):
                    if len(name) > 3 and not name.rstrip('_').isupper():
                        static_names.add(name)

    return static_names


def collect_member_names(files):
    """Find all trailing-underscore member names declared in headers."""
    # Match identifiers ending in _ followed by = ; { or [
    # Use a pattern that works after pointer/reference chars like * &
    decl_pattern = re.compile(r'(\w+_)\s*[=;{\[]')
    names = set()

    for path in files:
        if not path.endswith('.hpp') and not path.endswith('.h'):
            continue
        with open(path, 'r', encoding='utf-8', errors='ignore') as fh:
            content = fh.read()

        for match in decl_pattern.finditer(content):
            name = match.group(1)
            if not name.endswith('_'):
                continue
            if name.startswith('m_') or name.startswith('s_'):
                continue
            if name.rstrip('_').isupper():
                continue
            if len(name) <= 3:
                continue
            if name.endswith('_t_'):
                continue
            # Skip common false positives (type names, macros)
            if name.startswith('Q') and name[1:2].isupper():
                continue
            names.add(name)

    return names


def build_rename_map(names, static_names):
    """Build old_name -> new_name mapping."""
    rename = {}
    for name in sorted(names):
        if name in static_names:
            new_name = 's_' + name[:-1]
        else:
            new_name = 'm_' + name[:-1]
        rename[name] = new_name
    return rename


def apply_renames(files, rename_map):
    """Replace all occurrences in source files."""
    if not rename_map:
        print("  Nothing to rename.")
        return 0, 0

    # Sort by length descending to match longer names first
    sorted_names = sorted(rename_map.keys(), key=len, reverse=True)
    pattern = re.compile(r'\b(' + '|'.join(re.escape(n) for n in sorted_names) + r')')

    total_replacements = 0
    modified_files = 0

    for path in files:
        with open(path, 'r', encoding='utf-8', errors='ignore') as fh:
            original = fh.read()

        def replacer(match):
            return rename_map[match.group(1)]

        modified = pattern.sub(replacer, original)

        if modified != original:
            count = len(pattern.findall(original))
            total_replacements += count
            modified_files += 1
            rel = os.path.relpath(path, ROOT)

            if DRY_RUN:
                print(f"  [dry-run] {rel}: {count} replacements")
            else:
                with open(path, 'w', encoding='utf-8', newline='') as fh:
                    fh.write(modified)
                print(f"  {rel}: {count} replacements")

    return modified_files, total_replacements


def main():
    print(f"Root: {ROOT}")
    print(f"Mode: {'DRY RUN' if DRY_RUN else 'LIVE'}")
    print()

    files = collect_source_files()
    print(f"Scanning {len(files)} source files...")

    names = collect_member_names(files)
    static_names = find_static_members(files)
    print(f"Found {len(names)} trailing-underscore member names ({len(static_names)} static)")

    rename_map = build_rename_map(names, static_names)

    print(f"\nApplying {len(rename_map)} renames...")
    modified_files, total_replacements = apply_renames(files, rename_map)

    print(f"\nDone: {modified_files} files modified, {total_replacements} total replacements")

    if DRY_RUN:
        print("\nRe-run without --dry-run to apply changes.")


if __name__ == '__main__':
    main()
