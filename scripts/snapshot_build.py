"""Copy the runtime files of a build output folder to a separate snapshot folder.

Lets end-to-end/UI testing run the apps from a stable copy while another build
rewrites x64/<Configuration> (a running exe would otherwise lock the link step).
Build intermediates (.obj, .pdb, .lib, logs, ...) are skipped.

Usage:
  py scripts/snapshot_build.py [--config Release] [--dest DIR]
Prints the snapshot path; default dest is %TEMP%/yampt_snapshot_<config>.
"""

import argparse
import os
import shutil
import sys
import tempfile

repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
skipped_extensions = (".obj", ".pdb", ".ilk", ".iobj", ".ipdb", ".lib", ".exp", ".tlog", ".log", ".lastbuildstate")


def snapshot(source, destination):
    if os.path.exists(destination):
        shutil.rmtree(destination)
    copied = 0
    for directory, _, filenames in os.walk(source):
        relative = os.path.relpath(directory, source)
        target_directory = os.path.join(destination, relative)
        for filename in filenames:
            if filename.lower().endswith(skipped_extensions):
                continue
            os.makedirs(target_directory, exist_ok=True)
            shutil.copy2(os.path.join(directory, filename), os.path.join(target_directory, filename))
            copied += 1
    return copied


# Session keys that would make a snapshot reopen (and auto-save into) the user's
# real data on launch. Stripped so every snapshot starts with an empty session.
session_keys_to_strip = ("load_base_path", "active_plugin_path", "LastDirectory", "workspace", "recent")


def sanitize_ini(path):
    if not os.path.exists(path):
        return False
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        lines = handle.readlines()
    kept = []
    for line in lines:
        key = line.split("=", 1)[0].strip()
        if any(key.lower().startswith(prefix.lower()) for prefix in session_keys_to_strip):
            continue
        if key == "load_source":
            line = "load_source=0\n"
        kept.append(line)
    with open(path, "w", encoding="utf-8") as handle:
        handle.writelines(kept)
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", default="Release")
    parser.add_argument("--dest")
    parser.add_argument("--keep-session", action="store_true", help="do not strip saved session paths")
    arguments = parser.parse_args()
    source = os.path.join(repo_root, "x64", arguments.config)
    destination = arguments.dest or os.path.join(tempfile.gettempdir(), f"yampt_snapshot_{arguments.config.lower()}")
    copied = snapshot(source, destination)
    sanitized = [] if arguments.keep_session else [name for name in ("yEditor.ini", "yTranslator.ini")
                                                   if sanitize_ini(os.path.join(destination, name))]
    print(f"{destination} ({copied} files; sanitized: {', '.join(sanitized) or 'none'})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
