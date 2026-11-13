"""End-to-end checks on a merged patch produced by yEditor from an MO2 profile.

Checks (each reports PASS/FAIL with counts and examples):
  header     the merged patch carries yEditor's own header (author 'yEditor',
             description 'Auto-generated merged patch') — created-plugin path unchanged
  leveled    every LEVI/LEVC INDX count equals its number of INAM/CNAM entries
  dialogue_structure  every INFO's nearest preceding DIAL is a topic that INFO belongs
             to in the source plugins, and no INFO run follows a non-dialogue record
  info_links every INFO's PNAM/NNAM equals the PNAM/NNAM of the winning plugin
             (last plugin in load order that defines the same INFO id), i.e.
             dialogue links are never merged from a lower-priority plugin.
             FAILS if fewer than 90% of merged INFOs could be compared (no vacuous pass)

Plugins are resolved from the MO2 profile the same way yEditor does it: base
masters from the game Data Files folder, everything else from the highest-priority
enabled mod in modlist.txt that provides the file.

Usage:
  py scripts/e2e/check_merged_patch.py <merged_patch.esp> --mo2-root <dir> --profile <name> [--out report.json]
"""

import argparse
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from esp_inspect import decode_text, iter_records, read_header  # noqa: E402

leveled_item_sub = {"LEVI": "INAM", "LEVC": "CNAM"}
base_masters = ("morrowind.esm", "tribunal.esm", "bloodmoon.esm")
openmw_stub_suffixes = (".omwaddon.esp", ".omwscript.esp")
example_limit = 10


def read_lines(path):
    if not os.path.exists(path):
        return []
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return [line.rstrip("\r\n") for line in handle if line.strip() and not line.startswith("#")]


def game_data_dir(mo2_root):
    ini_path = os.path.join(mo2_root, "ModOrganizer.ini")
    with open(ini_path, "r", encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    match = re.search(r"^gamePath=@ByteArray\((.*)\)\s*$", text, re.MULTILINE) or re.search(r"^gamePath=(.*)$", text, re.MULTILINE)
    game_path = match.group(1).replace("\\\\", "\\").strip()
    return os.path.join(game_path, "Data Files")


def resolve_plugin_paths(mo2_root, profile):
    profile_dir = os.path.join(mo2_root, "profiles", profile)
    load_order = read_lines(os.path.join(profile_dir, "loadorder.txt"))
    enabled = [line[1:] for line in read_lines(os.path.join(profile_dir, "modlist.txt")) if line.startswith("+")]
    mods_dir = os.path.join(mo2_root, "mods")
    data_dir = game_data_dir(mo2_root)
    def resolve_one(plugin):
        for mod in enabled:  # modlist.txt lists highest priority first
            candidate = os.path.join(mods_dir, mod, plugin)
            if os.path.exists(candidate):
                return candidate
        if plugin.lower() in base_masters:
            candidate = os.path.join(data_dir, plugin)
            return candidate if os.path.exists(candidate) else None
        return None

    resolved = []
    for plugin in load_order:
        path = resolve_one(plugin)
        resolved.append((plugin, path))
        # Mirror yEditor: an OpenMW stub (X.omwaddon.esp) also loads its real file (X.omwaddon)
        # in the same load-order slot, right after the stub.
        if path is not None and plugin.lower().endswith(openmw_stub_suffixes):
            real_name = plugin[: -len(".esp")]
            real_path = resolve_one(real_name)
            if real_path is not None:
                resolved.append((real_name, real_path))
    return resolved


def iter_infos(path):
    """Yield ((dial_name_lower, inam_lower), {'PNAM': str|None, 'NNAM': str|None})."""
    current_dial = ""
    for _, record_type, _, sub_records in iter_records(path):
        if record_type == "DIAL":
            current_dial = next((decode_text(data) for sub_type, data in sub_records if sub_type == "NAME"), "")
            continue
        if record_type != "INFO":
            continue
        values = {"INAM": None, "PNAM": None, "NNAM": None}
        for sub_type, data in sub_records:
            if sub_type in values and values[sub_type] is None:
                values[sub_type] = decode_text(data)
        if values["INAM"]:
            yield (current_dial.lower(), values["INAM"].lower()), {"PNAM": values["PNAM"], "NNAM": values["NNAM"]}


def check_header(merged_path):
    header = read_header(merged_path) or {}
    ok = header.get("author") == "yEditor" and header.get("description") == "Auto-generated merged patch"
    return {"status": "PASS" if ok else "FAIL", "author": header.get("author"),
            "description": header.get("description"), "masters": len(header.get("masters", [])),
            "record_count": header.get("record_count")}


def check_leveled(merged_path):
    checked = 0
    failures = []
    for _, record_type, _, sub_records in iter_records(merged_path):
        if record_type not in leveled_item_sub:
            continue
        checked += 1
        item_sub = leveled_item_sub[record_type]
        entries = sum(1 for sub_type, _ in sub_records if sub_type == item_sub)
        indx = next((struct.unpack_from("<I", data, 0)[0] for sub_type, data in sub_records
                     if sub_type == "INDX" and len(data) >= 4), None)
        if indx is not None and indx != entries:
            name = next((decode_text(data) for sub_type, data in sub_records if sub_type == "NAME"), "?")
            failures.append({"type": record_type, "id": name, "indx": indx, "entries": entries})
    return {"status": "PASS" if not failures else "FAIL", "checked": checked,
            "mismatches": len(failures), "examples": failures[:example_limit]}


def collect_source_infos(merged_path, plugin_paths):
    """One pass over the load order for the INFOs present in the merged patch.
    Returns (winners: inam -> (plugin, links), topics: inam -> set of source DIAL names)."""
    merged_ids = {key[1] for key, _ in iter_infos(merged_path)}
    winners = {}
    topics = {}
    for plugin, path in plugin_paths:
        if path is None or os.path.basename(path).lower() == os.path.basename(merged_path).lower():
            continue
        for key, links in iter_infos(path):
            if key[1] not in merged_ids:
                continue
            winners[key[1]] = (plugin, links)  # later in load order overwrites -> winner
            topics.setdefault(key[1], set()).add(key[0])
    return winners, topics


def check_dialogue_structure(merged_path, source_topics):
    """TES3 rule: an INFO belongs to the nearest preceding DIAL. Every merged INFO's
    nearest preceding DIAL must be a topic that INFO belongs to in the source plugins."""
    previous_type = None
    current_dial = None
    misplaced = []
    orphan_runs = 0
    total_infos = 0
    dial_count = 0
    for _, record_type, _, sub_records in iter_records(merged_path):
        if record_type == "DIAL":
            dial_count += 1
            current_dial = next((decode_text(data) for sub_type, data in sub_records if sub_type == "NAME"), "?")
        elif record_type == "INFO":
            total_infos += 1
            if previous_type not in ("DIAL", "INFO"):
                orphan_runs += 1
            inam = next((decode_text(data) for sub_type, data in sub_records if sub_type == "INAM"), "?").lower()
            expected = source_topics.get(inam, set())
            actual = current_dial.lower() if current_dial is not None else None
            if actual not in expected:
                misplaced.append({"info": inam, "under_dial": current_dial, "expected": sorted(expected)})
        previous_type = record_type
    return {"status": "PASS" if not misplaced and not orphan_runs else "FAIL", "infos": total_infos,
            "dials": dial_count, "orphan_runs": orphan_runs, "misplaced": len(misplaced),
            "examples": misplaced[:example_limit]}


def check_info_links(merged_path, winners, minimum_coverage=0.9):
    # Key by INAM alone: a misplaced INFO in the patch has no reliable DIAL context.
    merged_infos = {key[1]: links for key, links in iter_infos(merged_path)}
    failures = []
    for info_id, merged_links in merged_infos.items():
        if info_id not in winners:
            continue
        plugin, winner_links = winners[info_id]
        if merged_links != winner_links:
            failures.append({"info": info_id, "merged": merged_links, "winner": winner_links, "winner_plugin": plugin})
    compared = sum(1 for info_id in merged_infos if info_id in winners)
    coverage = compared / len(merged_infos) if merged_infos else 1.0
    # A link check that compared almost nothing proves nothing: fail on low coverage.
    status = "PASS" if not failures and coverage >= minimum_coverage else "FAIL"
    return {"status": status, "merged_infos": len(merged_infos), "compared": compared,
            "coverage": round(coverage, 3), "mismatches": len(failures), "examples": failures[:example_limit]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("merged_patch")
    parser.add_argument("--mo2-root", required=True)
    parser.add_argument("--profile", required=True)
    parser.add_argument("--out")
    arguments = parser.parse_args()

    plugin_paths = resolve_plugin_paths(arguments.mo2_root, arguments.profile)
    winners, source_topics = collect_source_infos(arguments.merged_patch, plugin_paths)
    report = {
        "merged_patch": arguments.merged_patch,
        "unresolved_plugins": [plugin for plugin, path in plugin_paths if path is None],
        "header": check_header(arguments.merged_patch),
        "leveled": check_leveled(arguments.merged_patch),
        "dialogue_structure": check_dialogue_structure(arguments.merged_patch, source_topics),
        "info_links": check_info_links(arguments.merged_patch, winners),
    }
    checks = ("header", "leveled", "dialogue_structure", "info_links")
    report["overall"] = "PASS" if all(report[name]["status"] == "PASS" for name in checks) else "FAIL"
    text = json.dumps(report, indent=2, ensure_ascii=False)
    if arguments.out:
        with open(arguments.out, "w", encoding="utf-8") as handle:
            handle.write(text)
    else:
        print(text)
    return 0 if report["overall"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
