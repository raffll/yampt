"""Trace one or more dialogue INFO ids through an MO2 profile's load order.

For each INFO id, lists every plugin (in load order) that defines it, with the
owning DIAL topic, PNAM and NNAM there — and the same for a given merged patch.
Use it to explain merged-patch dialogue mismatches.

Usage:
  py scripts/e2e/trace_info.py --mo2-root <dir> --profile <name> [--merged <patch.esp>] --out trace.txt <inam> [<inam> ...]
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from check_merged_patch import resolve_plugin_paths  # noqa: E402
from esp_inspect import decode_text, iter_records  # noqa: E402


def scan(path, wanted):
    """Yield (inam, dial, pnam, nnam, previous_record_type) for wanted INFO ids in a plugin."""
    current_dial = None
    previous_type = None
    for _, record_type, _, sub_records in iter_records(path):
        if record_type == "DIAL":
            current_dial = next((decode_text(data) for sub_type, data in sub_records if sub_type == "NAME"), "?")
        elif record_type == "INFO":
            values = {}
            for sub_type, data in sub_records:
                if sub_type in ("INAM", "PNAM", "NNAM") and sub_type not in values:
                    values[sub_type] = decode_text(data)
            inam = values.get("INAM", "")
            if inam in wanted:
                yield inam, current_dial, values.get("PNAM"), values.get("NNAM"), previous_type
        previous_type = record_type


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mo2-root", required=True)
    parser.add_argument("--profile", required=True)
    parser.add_argument("--merged")
    parser.add_argument("--out", required=True)
    parser.add_argument("inams", nargs="+")
    arguments = parser.parse_args()

    wanted = set(arguments.inams)
    found = {inam: [] for inam in arguments.inams}
    for position, (plugin, path) in enumerate(resolve_plugin_paths(arguments.mo2_root, arguments.profile)):
        if path is None:
            continue
        for inam, dial, pnam, nnam, previous in scan(path, wanted):
            found[inam].append(f"  [{position:03d}] {plugin}: DIAL={dial!r} PNAM={pnam!r} NNAM={nnam!r}")
    merged = {}
    if arguments.merged:
        for inam, dial, pnam, nnam, previous in scan(arguments.merged, wanted):
            merged[inam] = f"  MERGED: DIAL={dial!r} PNAM={pnam!r} NNAM={nnam!r} (record before it: {previous})"

    lines = []
    for inam in arguments.inams:
        lines.append(f"INFO {inam}:")
        lines.extend(found[inam] or ["  (not defined by any resolved plugin)"])
        lines.append(merged.get(inam, "  MERGED: (absent)"))
    with open(arguments.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
