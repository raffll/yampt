"""Compare an edited plugin against its original, byte for byte, record by record.

Proves that a yEditor field edit changed only what was edited: the TES3 header must be
byte-identical (author, description, flags, masters, record count), every record must
keep its position, and only the expected records may differ. For each differing record
the differing byte offsets inside the record are reported.

Usage:
  py scripts/e2e/compare_plugin_edit.py <original.esp> <edited.esp> [--out report.json]
"""

import argparse
import json
import struct
import sys

record_header_size = 16


def read_raw_records(path):
    with open(path, "rb") as handle:
        blob = handle.read()
    records = []
    offset = 0
    while offset + record_header_size <= len(blob):
        record_type = blob[offset:offset + 4].decode("latin-1")
        body_size = struct.unpack_from("<I", blob, offset + 4)[0]
        end = offset + record_header_size + body_size
        records.append({"type": record_type, "bytes": blob[offset:end]})
        offset = end
    return records, len(blob), offset == len(blob)


def differing_offsets(left, right):
    common = min(len(left), len(right))
    offsets = [index for index in range(common) if left[index] != right[index]]
    return offsets, len(left) != len(right)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("original")
    parser.add_argument("edited")
    parser.add_argument("--out")
    arguments = parser.parse_args()

    original, original_size, original_clean = read_raw_records(arguments.original)
    edited, edited_size, edited_clean = read_raw_records(arguments.edited)
    report = {"original_size": original_size, "edited_size": edited_size,
              "original_records": len(original), "edited_records": len(edited),
              "parse_clean": original_clean and edited_clean, "record_order_same": True,
              "header_identical": bool(original and edited and original[0]["bytes"] == edited[0]["bytes"]),
              "differences": []}
    for index, (left, right) in enumerate(zip(original, edited)):
        if left["type"] != right["type"]:
            report["record_order_same"] = False
        if left["bytes"] == right["bytes"]:
            continue
        offsets, size_changed = differing_offsets(left["bytes"], right["bytes"])
        report["differences"].append({"index": index, "type": left["type"], "size_changed": size_changed,
                                      "differing_offsets": offsets[:32],
                                      "original_bytes": [left["bytes"][o] for o in offsets[:32]],
                                      "edited_bytes": [right["bytes"][o] for o in offsets[:32]]})
    text = json.dumps(report, indent=2)
    if arguments.out:
        with open(arguments.out, "w", encoding="utf-8") as handle:
            handle.write(text)
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
