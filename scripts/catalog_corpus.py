"""Catalog a Mod Organizer 2 instance as a reusable test corpus.

READ-ONLY: never writes into the corpus. Writes:
  <out_dir>/<name>-catalog.json   full per-plugin data
  <out_dir>/<name>-summary.md     human/agent-readable summary + suggested test targets

Usage:
  py scripts/catalog_corpus.py <mo2_root> <out_dir> [--name poti]
"""

import argparse
import collections
import json
import os
import struct
import sys
import time

plugin_extensions = (".esm", ".esp", ".omwaddon", ".omwgame")
other_extensions = (".omwscripts", ".bsa")
record_header_size = 16
sub_header_size = 8
hedr_version_offset = 0
hedr_flags_offset = 4
hedr_author_offset = 8
hedr_author_length = 32
hedr_description_offset = 40
hedr_description_length = 256
hedr_record_count_offset = 296
tes3_record_flag_offset = 12
openmw_stub_suffixes = (".omwaddon.esp", ".omwaddon.esm", ".omwscripts.esp", ".omwscripts.esm", ".omwgame.esp")


def clean_text(raw_bytes):
    return raw_bytes.split(b"\x00", 1)[0].decode("cp1252", errors="replace")


def parse_sub_records(body):
    position = 0
    while position + sub_header_size <= len(body):
        sub_type = body[position:position + 4].decode("latin-1")
        sub_size = struct.unpack_from("<I", body, position + 4)[0]
        start = position + sub_header_size
        yield sub_type, body[start:start + sub_size]
        position = start + sub_size


def parse_tes3_header(handle):
    header = handle.read(record_header_size)
    if len(header) < record_header_size or header[:4] != b"TES3":
        return None, 0

    body_size = struct.unpack_from("<I", header, 4)[0]
    record_flags = struct.unpack_from("<I", header, tes3_record_flag_offset)[0]
    body = handle.read(body_size)
    info = {"record_flags": record_flags, "masters": []}

    for sub_type, data in parse_sub_records(body):
        if sub_type == "HEDR" and len(data) >= hedr_record_count_offset + 4:
            info["version"] = round(struct.unpack_from("<f", data, hedr_version_offset)[0], 3)
            info["file_flags"] = struct.unpack_from("<I", data, hedr_flags_offset)[0]
            info["author"] = clean_text(data[hedr_author_offset:hedr_author_offset + hedr_author_length])
            info["description"] = clean_text(
                data[hedr_description_offset:hedr_description_offset + hedr_description_length])
            info["record_count"] = struct.unpack_from("<I", data, hedr_record_count_offset)[0]
        elif sub_type == "MAST":
            info["masters"].append({"name": clean_text(data), "size": None})
        elif sub_type == "DATA" and info["masters"] and len(data) >= 8:
            info["masters"][-1]["size"] = struct.unpack_from("<Q", data, 0)[0]

    return info, record_header_size + body_size


def count_record_types(handle, start_offset, file_size):
    counts = collections.Counter()
    position = start_offset
    malformed = False
    while position + record_header_size <= file_size:
        handle.seek(position)
        header = handle.read(record_header_size)
        if len(header) < record_header_size:
            break
        record_type = header[:4]
        if not all(32 <= byte < 127 for byte in record_type):
            malformed = True
            break
        body_size = struct.unpack_from("<I", header, 4)[0]
        counts[record_type.decode("latin-1")] += 1
        position += record_header_size + body_size
    return dict(counts), malformed


def scan_plugin(path):
    file_size = os.path.getsize(path)
    entry = {"size": file_size}
    with open(path, "rb") as handle:
        header, consumed = parse_tes3_header(handle)
        if header is None:
            entry["error"] = "not a TES3 file"
            return entry
        entry.update(header)
        counts, malformed = count_record_types(handle, consumed, file_size)
        entry["record_types"] = counts
        entry["total_records"] = sum(counts.values())
        if malformed:
            entry["malformed_tail"] = True
    return entry


def read_lines(path):
    if not os.path.exists(path):
        return []
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return [line.rstrip("\r\n") for line in handle if line.strip() and not line.startswith("#")]


def catalog_mods(mods_dir):
    plugins = {}
    others = []
    for mod_name in sorted(os.listdir(mods_dir)):
        mod_path = os.path.join(mods_dir, mod_name)
        if not os.path.isdir(mod_path):
            continue
        for directory, _, filenames in os.walk(mod_path):
            for filename in filenames:
                lower = filename.lower()
                full_path = os.path.join(directory, filename)
                relative = os.path.relpath(full_path, mod_path)
                if lower.endswith(plugin_extensions):
                    record = scan_plugin(full_path)
                    record.update({"file": filename, "mod": mod_name, "relative": relative,
                                   "stub": lower.endswith(openmw_stub_suffixes)})
                    plugins.setdefault(lower, []).append(record)
                elif lower.endswith(other_extensions):
                    others.append({"file": filename, "mod": mod_name, "relative": relative,
                                   "size": os.path.getsize(full_path)})
    return plugins, others


def resolve_load_order(profile_dir, plugins):
    load_order = read_lines(os.path.join(profile_dir, "loadorder.txt"))
    modlist = read_lines(os.path.join(profile_dir, "modlist.txt"))
    enabled_mods = [line[1:] for line in modlist if line.startswith("+")]
    priority = {mod: index for index, mod in enumerate(reversed(enabled_mods))}

    resolved = []
    missing = []
    for position, plugin_name in enumerate(load_order):
        candidates = [record for record in plugins.get(plugin_name.lower(), []) if record["mod"] in priority]
        if not candidates:
            missing.append(plugin_name)
            resolved.append({"position": position, "plugin": plugin_name, "mod": None})
            continue
        winner = max(candidates, key=lambda record: priority[record["mod"]])
        resolved.append({"position": position, "plugin": plugin_name, "mod": winner["mod"],
                         "providers": len(candidates)})
    return {"load_order_count": len(load_order), "enabled_mods": len(enabled_mods),
            "modlist_count": len(modlist), "resolved": resolved,
            "not_found_in_enabled_mods": missing}


def pick_targets(flat_plugins):
    real = [record for record in flat_plugins if "error" not in record and not record["stub"]]
    targets = {}
    with_author = [record for record in real if record.get("author", "").strip() and record.get("author") != "yEditor"]
    targets["header_preserve_small_with_author"] = sorted(
        (record for record in with_author if record.get("masters")), key=lambda record: record["size"])[:5]
    targets["esm_flagged"] = sorted(
        (record for record in real if record.get("file_flags", 0) & 1), key=lambda record: record["size"])[:5]
    targets["largest"] = sorted(real, key=lambda record: record["size"], reverse=True)[:5]
    targets["most_masters"] = sorted(real, key=lambda record: len(record.get("masters", [])), reverse=True)[:5]
    for record_type in ("INFO", "NPC_", "LEVI", "LEVC", "CELL", "SCPT", "DIAL"):
        targets[f"small_with_{record_type}"] = sorted(
            (record for record in real if record.get("record_types", {}).get(record_type)),
            key=lambda record: record["size"])[:3]
    targets["malformed_or_unreadable"] = [record for record in flat_plugins
                                          if "error" in record or record.get("malformed_tail")][:20]
    targets["edited_by_yeditor"] = [record for record in real if record.get("author") == "yEditor"][:50]
    return targets


def brief(record):
    return {key: record.get(key) for key in ("file", "mod", "size", "author", "record_count", "total_records")}


def write_summary(path, name, catalog):
    flat = catalog["flat"]
    lines = [f"# Test corpus: {name}", "", f"Root: `{catalog['root']}`", f"Generated: {catalog['generated']}", ""]
    lines.append(f"- Plugin files (.esm/.esp/.omwaddon/.omwgame): {len(flat)}")
    lines.append(f"- Other (.omwscripts/.bsa): {len(catalog['others'])}")
    by_extension = collections.Counter(os.path.splitext(record["file"])[1].lower() for record in flat)
    lines.append(f"- By extension: {dict(by_extension)}")
    lines.append(f"- OpenMW double-extension stubs: {sum(1 for record in flat if record['stub'])}")
    lines.append(f"- Unreadable / malformed tail: {sum(1 for record in flat if 'error' in record or record.get('malformed_tail'))}")
    lines.append(f"- Already authored 'yEditor' (header overwritten by the old save bug): "
                 f"{sum(1 for record in flat if record.get('author') == 'yEditor')}")
    totals = collections.Counter()
    for record in flat:
        totals.update(record.get("record_types", {}))
    lines.append(f"- Records across all plugins: {sum(totals.values())}")
    lines.append("- Top record types: " + ", ".join(f"{key} {value}" for key, value in totals.most_common(15)))
    for profile in catalog["profiles"]:
        info = catalog["profiles"][profile]
        lines += ["", f"## Profile: {profile}",
                  f"- loadorder.txt entries: {info['load_order_count']}",
                  f"- modlist.txt entries: {info['modlist_count']} (enabled: {info['enabled_mods']})",
                  f"- load-order plugins not found in an enabled mod: {len(info['not_found_in_enabled_mods'])}"]
        if info["not_found_in_enabled_mods"]:
            lines.append("  - " + ", ".join(info["not_found_in_enabled_mods"][:40]))
    lines += ["", "## Suggested test targets (smallest first)"]
    for target, records in catalog["targets"].items():
        lines.append(f"- **{target}**:")
        for record in records:
            short = brief(record)
            lines.append(f"  - `{short['file']}` ({short['size']} B, mod `{short['mod']}`, "
                         f"author `{short['author']}`, records {short['total_records']})")
    lines += ["", "Full data: the matching `-catalog.json` (per plugin: header, masters, record-type counts, "
              "providing mod; per profile: resolved load order)."]
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mo2_root")
    parser.add_argument("out_dir")
    parser.add_argument("--name", default="corpus")
    arguments = parser.parse_args()

    started = time.time()
    os.makedirs(arguments.out_dir, exist_ok=True)
    plugins, others = catalog_mods(os.path.join(arguments.mo2_root, "mods"))
    flat = [record for records in plugins.values() for record in records]

    profiles = {}
    profiles_dir = os.path.join(arguments.mo2_root, "profiles")
    if os.path.isdir(profiles_dir):
        for profile in sorted(os.listdir(profiles_dir)):
            profile_dir = os.path.join(profiles_dir, profile)
            if os.path.isdir(profile_dir):
                profiles[profile] = resolve_load_order(profile_dir, plugins)

    catalog = {"root": arguments.mo2_root, "generated": time.strftime("%Y-%m-%d %H:%M:%S"),
               "flat": flat, "others": others, "profiles": profiles, "targets": pick_targets(flat),
               "scan_seconds": round(time.time() - started, 1)}

    with open(os.path.join(arguments.out_dir, f"{arguments.name}-catalog.json"), "w", encoding="utf-8") as handle:
        json.dump(catalog, handle, ensure_ascii=False)
    write_summary(os.path.join(arguments.out_dir, f"{arguments.name}-summary.md"), arguments.name, catalog)
    print(f"done: {len(flat)} plugins in {catalog['scan_seconds']}s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
