"""Create a throwaway MO2 instance that READS the real mods but WRITES elsewhere.

The sandbox gets:
  ModOrganizer.ini           copied (game path resolution)
  profiles/<profile>/        copied (loadorder.txt, modlist.txt, plugins.txt, ...)
  overwrite/                 new, empty — yEditor's MO2-mode merged patch lands here
  mods                       a directory JUNCTION to the real mods folder (no copy)

WARNING: the junction is not write-protected. Only use the sandbox for operations
that write to overwrite/ (building the merged patch). Never edit a loaded corpus
plugin through it — edits save back to the plugin's original file. Always pair a
sandbox run with scripts/dir_fingerprint.py before/after on the real install.

Usage:
  py scripts/e2e/make_mo2_sandbox.py <real_mo2_root> <profile> <sandbox_dir>
  py scripts/e2e/make_mo2_sandbox.py --remove <sandbox_dir>     (removes junction safely, then the rest)
"""

import argparse
import os
import shutil
import subprocess
import sys


def remove_sandbox(sandbox_dir):
    mods_link = os.path.join(sandbox_dir, "mods")
    if os.path.lexists(mods_link):
        # rmdir on a junction removes only the link, never the target's contents.
        subprocess.run(["cmd", "/c", "rmdir", mods_link], check=True)
    if os.path.exists(sandbox_dir):
        shutil.rmtree(sandbox_dir)
    print(f"removed {sandbox_dir}")


def create_sandbox(real_root, profile, sandbox_dir):
    if os.path.exists(sandbox_dir):
        remove_sandbox(sandbox_dir)
    os.makedirs(os.path.join(sandbox_dir, "overwrite"))
    shutil.copy2(os.path.join(real_root, "ModOrganizer.ini"), os.path.join(sandbox_dir, "ModOrganizer.ini"))
    shutil.copytree(os.path.join(real_root, "profiles", profile), os.path.join(sandbox_dir, "profiles", profile))
    subprocess.run(["cmd", "/c", "mklink", "/J", os.path.join(sandbox_dir, "mods"), os.path.join(real_root, "mods")],
                   check=True, capture_output=True)
    print(os.path.join(sandbox_dir, "profiles", profile))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--remove", action="store_true")
    arguments = parser.parse_args()
    if arguments.remove:
        remove_sandbox(arguments.paths[0])
    else:
        real_root, profile, sandbox_dir = arguments.paths
        create_sandbox(real_root, profile, sandbox_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
