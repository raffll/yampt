"""Inspect TES3 plugin files (.esp/.esm/.omwaddon) from Python, read-only.

Used by end-to-end checks to verify what yampt wrote: headers, masters, and
individual records/sub-records.

CLI:
  py scripts/esp_inspect.py header <plugin>
  py scripts/esp_inspect.py find   <plugin> <TYPE> [<id_substring>] [--limit 20]
  py scripts/esp_inspect.py record <plugin> <TYPE> <exact_id>        (all sub-records, hex for binary)
  py scripts/esp_inspect.py counts <plugin>

Library:
  from esp_inspect import read_header, iter_records, find_record
"""

import argparse
import collections
import json
import struct
import sys

record_header_size = 16
sub_header_size = 8
id_sub_records = ("NAME", "INAM")
text_sub_records = {"NAME", "INAM", "PNAM", "NNAM", "FNAM", "ONAM", "RNAM", "CNAM", "ANAM", "DNAM", "SNAM",
                    "BNAM", "SCRI", "MODL", "ITEX", "TEXT", "DESC", "SCTX", "MAST", "KNAM", "STRV", "INDX"}


def decode_text(raw):
    return raw.split(b"\x00", 1)[0].decode("cp1252", errors="replace")


def parse_sub_records(body):
    position = 0
    while position + sub_header_size <= len(body):
        sub_type = body[position:position + 4].decode("latin-1")
        sub_size = struct.unpack_from("<I", body, position + 4)[0]
        start = position + sub_header_size
        yield sub_type, body[start:start + sub_size]
        position = start + sub_size


def iter_records(path):
    """Yield (offset, type, record_flags, sub_records[list of (type, bytes)])."""
    with open(path, "rb") as handle:
        data = handle.read()
    position = 0
    while position + record_header_size <= len(data):
        record_type = data[position:position + 4].decode("latin-1")
        body_size = struct.unpack_from("<I", data, position + 4)[0]
        record_flags = struct.unpack_from("<I", data, position + 12)[0]
        body = data[position + record_header_size:position + record_header_size + body_size]
        yield position, record_type, record_flags, list(parse_sub_records(body))
        position += record_header_size + body_size


def record_id(sub_records):
    for wanted in id_sub_records:
        for sub_type, data in sub_records:
            if sub_type == wanted:
                return decode_text(data)
    return ""


def read_header(path):
    for _, record_type, record_flags, sub_records in iter_records(path):
        if record_type != "TES3":
            return None
        header = {"record_flags": record_flags, "masters": []}
        for sub_type, data in sub_records:
            if sub_type == "HEDR" and len(data) >= 300:
                header["version"] = round(struct.unpack_from("<f", data, 0)[0], 3)
                header["file_flags"] = struct.unpack_from("<I", data, 4)[0]
                header["author"] = decode_text(data[8:40])
                header["description"] = decode_text(data[40:296])
                header["record_count"] = struct.unpack_from("<I", data, 296)[0]
                header["hedr_hex"] = data.hex()
            elif sub_type == "MAST":
                header["masters"].append({"name": decode_text(data), "size": None})
            elif sub_type == "DATA" and header["masters"] and len(data) >= 8:
                header["masters"][-1]["size"] = struct.unpack_from("<Q", data, 0)[0]
        return header
    return None


def find_record(path, wanted_type, wanted_id, exact=True):
    matches = []
    for offset, record_type, record_flags, sub_records in iter_records(path):
        if record_type != wanted_type:
            continue
        current_id = record_id(sub_records)
        hit = current_id.lower() == wanted_id.lower() if exact else wanted_id.lower() in current_id.lower()
        if hit:
            matches.append({"offset": offset, "type": record_type, "id": current_id, "flags": record_flags,
                            "subs": sub_records})
    return matches


def render_sub(sub_type, data):
    if sub_type in text_sub_records and (not data or data[-1:] == b"\x00" or all(31 < b < 127 or b > 127 for b in data[:-1])):
        return f"{sub_type} [{len(data)}] text: {decode_text(data)!r}"
    return f"{sub_type} [{len(data)}] hex: {data.hex()}"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("header", "find", "record", "counts"))
    parser.add_argument("plugin")
    parser.add_argument("record_type", nargs="?")
    parser.add_argument("record_id", nargs="?", default="")
    parser.add_argument("--limit", type=int, default=20)
    arguments = parser.parse_args()

    if arguments.mode == "header":
        header = read_header(arguments.plugin)
        if header:
            header.pop("hedr_hex", None)
        print(json.dumps(header, indent=2, ensure_ascii=False))
    elif arguments.mode == "counts":
        counts = collections.Counter(record_type for _, record_type, _, _ in iter_records(arguments.plugin))
        print(json.dumps(dict(counts.most_common()), indent=2))
    elif arguments.mode == "find":
        for match in find_record(arguments.plugin, arguments.record_type, arguments.record_id, exact=False)[:arguments.limit]:
            print(f"{match['type']} {match['id']!r} @{match['offset']} subs={len(match['subs'])}")
    else:
        for match in find_record(arguments.plugin, arguments.record_type, arguments.record_id, exact=True):
            print(f"== {match['type']} {match['id']!r} @{match['offset']} flags=0x{match['flags']:x}")
            for sub_type, data in match["subs"]:
                print("   " + render_sub(sub_type, data))
    return 0


if __name__ == "__main__":
    sys.exit(main())
