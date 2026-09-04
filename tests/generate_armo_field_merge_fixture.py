#!/usr/bin/env python3
"""Generate an ARMO element-wise AODT field-merge test case.

Reproduces the adamantium_helm case from the screenshot: one record present in
a master plus five plugins, exercising auto_merge_t's element-wise AODT merge
(record_behavior ARMO:AODT@24 element_wise + merge_fields_three_way).

Load order (last wins; auto_merge sees versions.front() = master, .back() = last esp):
  0 armo_master.esm        (Tribunal, "vanilla" / first)
  1 armo_esp1.esp          (Adamantium_2_TD)
  2 armo_esp2.esp          (Endgame Medium)
  3 armo_esp3.esp          (Where's Your Uniform)
  4 armo_esp4.esp          (Rational Names)
  5 armo_esp5.esp          (Where's Your Uniform - winner column)

AODT (24 bytes): Type@0 u32, Weight@4 f32, Value@8 u32, Health@12 u32,
                 Enchant Points@16 u32, Armor Rating@20 u32.

Per-field values across the six versions (v0..v5):
  Value        : 5000, 5000, 9500, 9500, 5000, 5000   -> merged 9500
  Health       :  900,  900,  575,  575,  900,  900   -> merged  575
  Enchant Pts  :  500,  500,  400,  400,  500,  500    -> merged  400
  Armor Rating :   70,   40,   50,   50,   70,   70     -> merged  50

For each field the winner (v5) reverted to the master value, but an intermediate
changed it, so the merge keeps the intermediate change. When several intermediates
changed the same field, the highest-priority (latest in load order) change wins.
For Armor Rating that resolves to 50 (v2/v3), not 40 (v1).
"""

import os
import struct

OUT_DIR = os.path.join(os.path.dirname(__file__), "fixtures", "armo_field_merge")


def sub(sub_type, data):
    return sub_type.encode("ascii") + struct.pack("<I", len(data)) + data


def zstr(text):
    return text.encode("ascii") + b"\x00"


def record(rec_type, body, flags=0):
    header = rec_type.encode("ascii")
    header += struct.pack("<I", len(body))
    header += struct.pack("<I", 0)
    header += struct.pack("<I", flags)
    return header + body


def aodt(value, health, enchant_points, armor_rating):
    body = bytearray(24)
    struct.pack_into("<I", body, 0, 0)                 # Type = Helmet
    struct.pack_into("<f", body, 4, 4.0)               # Weight
    struct.pack_into("<I", body, 8, value)             # Value
    struct.pack_into("<I", body, 12, health)           # Health
    struct.pack_into("<I", body, 16, enchant_points)   # Enchant Points
    struct.pack_into("<I", body, 20, armor_rating)     # Armor Rating
    return sub("AODT", bytes(body))


def armo(value, health, enchant_points, armor_rating):
    body = sub("NAME", zstr("adamantium_helm"))
    body += sub("MODL", zstr("A\\A_adamantium_helm.nif"))
    body += sub("FNAM", zstr("Adamantium Helmet"))
    body += aodt(value, health, enchant_points, armor_rating)
    body += sub("ITEX", zstr("A\\TX_adamantium_helm.tga"))
    return record("ARMO", body)


def tes3_header(is_master, description):
    hedr = sub(
        "HEDR",
        struct.pack("<f", 1.3)
        + struct.pack("<I", 1 if is_master else 0)
        + b"yampt fixtures".ljust(32, b"\x00")
        + description.encode("ascii").ljust(256, b"\x00")
        + struct.pack("<I", 1),
    )
    return record("TES3", hedr)


# (Value, Health, Enchant Points, Armor Rating) per version v0..v5
VERSIONS = [
    (5000, 900, 500, 70),  # v0 master  - vanilla
    (5000, 900, 500, 40),  # v1 esp1    - Armor Rating changed1
    (9500, 575, 400, 50),  # v2 esp2    - Value/Health/Enchant changed, Armor Rating changed2
    (9500, 575, 400, 50),  # v3 esp3    - same as v2
    (5000, 900, 500, 70),  # v4 esp4    - vanilla
    (5000, 900, 500, 70),  # v5 esp5    - vanilla (winner)
]

FILES = [
    ("armo_master.esm", True, "Tribunal"),
    ("armo_esp1.esp", False, "Adamantium_2_TD"),
    ("armo_esp2.esp", False, "Endgame Medium"),
    ("armo_esp3.esp", False, "Where's Your Uniform"),
    ("armo_esp4.esp", False, "Rational Names"),
    ("armo_esp5.esp", False, "Where's Your Uniform (winner)"),
]


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    for (filename, is_master, description), values in zip(FILES, VERSIONS):
        data = tes3_header(is_master, description) + armo(*values)
        with open(os.path.join(OUT_DIR, filename), "wb") as handle:
            handle.write(data)
        print(f"wrote {filename}: Value={values[0]} Health={values[1]} "
              f"EnchantPts={values[2]} ArmorRating={values[3]}")

    print("\nExpected merged AODT: Value=9500 Health=575 EnchantPts=400 ArmorRating=50")


if __name__ == "__main__":
    main()
