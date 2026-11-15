"""Find files by case-insensitive name substring under one or more roots.

Faster and more reliable than shell globbing on large trees; skips noisy build
and VCS folders.

Usage:
  py scripts/find_files.py <substring> <root> [<root> ...] [--ext .esp,.esm] [--limit 50] [--out found.txt]
"""

import argparse
import os
import sys

skipped_directories = {".git", ".vs", "x64", "node_modules", "vcpkg", "vcpkg_installed", "buildtrees", "__pycache__"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("substring")
    parser.add_argument("roots", nargs="+")
    parser.add_argument("--ext", default="")
    parser.add_argument("--limit", type=int, default=50)
    parser.add_argument("--out")
    arguments = parser.parse_args()

    needle = arguments.substring.lower()
    extensions = tuple(ext.strip().lower() for ext in arguments.ext.split(",") if ext.strip())
    found = []
    for root in arguments.roots:
        for directory, subdirectories, filenames in os.walk(root):
            subdirectories[:] = [name for name in subdirectories if name.lower() not in skipped_directories]
            for filename in filenames:
                lower = filename.lower()
                if needle in lower and (not extensions or lower.endswith(extensions)):
                    path = os.path.join(directory, filename)
                    found.append(f"{os.path.getsize(path):>12}  {path}")
                    if len(found) >= arguments.limit:
                        break
            if len(found) >= arguments.limit:
                break

    text = "\n".join(found) + f"\n{len(found)} match(es)\n"
    if arguments.out:
        with open(arguments.out, "w", encoding="utf-8") as handle:
            handle.write(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
