"""Fingerprint folders (path, size, mtime) and diff two fingerprints.

Use before/after an end-to-end test to prove the app did not write into a
folder it must only read (e.g. the real MO2 corpus).

Usage:
  py scripts/dir_fingerprint.py snap <out.json> <dir> [<dir> ...] [--ext .esp,.esm,.omwaddon]
  py scripts/dir_fingerprint.py diff <before.json> <after.json> [--out diff.txt]
"""

import argparse
import json
import os
import sys


def snapshot(directories, extensions):
    entries = {}
    for root in directories:
        for directory, _, filenames in os.walk(root):
            for filename in filenames:
                if extensions and not filename.lower().endswith(extensions):
                    continue
                path = os.path.join(directory, filename)
                try:
                    status = os.stat(path)
                except OSError:
                    continue
                entries[path] = [status.st_size, int(status.st_mtime)]
    return entries


def diff(before, after):
    added = sorted(set(after) - set(before))
    removed = sorted(set(before) - set(after))
    changed = sorted(path for path in set(before) & set(after) if before[path] != after[path])
    return added, removed, changed


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("snap", "diff"))
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--ext", default="")
    parser.add_argument("--out")
    arguments = parser.parse_args()

    if arguments.mode == "snap":
        output_path, directories = arguments.paths[0], arguments.paths[1:]
        extensions = tuple(ext.strip().lower() for ext in arguments.ext.split(",") if ext.strip())
        entries = snapshot(directories, extensions)
        with open(output_path, "w", encoding="utf-8") as handle:
            json.dump(entries, handle)
        print(f"{len(entries)} files")
        return 0

    with open(arguments.paths[0], "r", encoding="utf-8") as handle:
        before = json.load(handle)
    with open(arguments.paths[1], "r", encoding="utf-8") as handle:
        after = json.load(handle)
    added, removed, changed = diff(before, after)
    lines = [f"before={len(before)} after={len(after)} added={len(added)} removed={len(removed)} changed={len(changed)}"]
    lines += [f"ADDED   {path}" for path in added]
    lines += [f"REMOVED {path}" for path in removed]
    lines += [f"CHANGED {path} {before[path]} -> {after[path]}" for path in changed]
    text = "\n".join(lines) + "\n"
    if arguments.out:
        with open(arguments.out, "w", encoding="utf-8") as handle:
            handle.write(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
