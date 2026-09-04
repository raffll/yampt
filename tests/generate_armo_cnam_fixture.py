#!/usr/bin/env python3
"""Generate the ARMO body-part CNAM merge case (dreugh_cuirass from the screenshot).

Reproduces an ARMO record where a CNAM (Female Part Name) exists only in an
intermediate plugin. The group-aware ARMO merge must carry that CNAM into its
INDX body-part group in the merged patch, contiguous with the merged BNAM.

Load order (last wins):
  0 armo_cnam_master.esm   (Morrowind.esm / first)
  1 armo_cnam_esp1.esp     (Unique Uniques - Items.ESP / intermediate, adds CNAM)
  2 armo_cnam_esp2.esp     (Rational Names.esp / winner, no CNAM)

Expected merged AODT header + body part:
  MODL = Anu\\uni\\a\\cuirass_dreu_gnd.nif   (intermediate; winner reverted)
  FNAM = Dreugh Warlord Cuirass              (winner changed)
  ITEX = Anu\\uni\\a\\cuirass_dreugh.dds      (intermediate; winner reverted)
  Body Part (INDX Cuirass):
    BNAM = uni_cuirass_dreugh                (intermediate; winner reverted)
    CNAM = uni_cuirass_dreugh_f              (intermediate-only, carried into the group)
"""

import os
import struct

OUT_DIR = os.path.join(os.path.dirname(__file__), "fixtures", "armo_cnam")

CUIRASS_INDX = 1  # armor body-part index for Cuirass


def sub(sub_type, data):
    return sub_type.encode("ascii") + struct.pack("<I", len(data)) + data


def zstr(text):
    return text.encode("ascii") + b"\x00"


def record(rec_type, body):
    return rec_type.encode("ascii") + struct.pack("<I", len(body)) + struct.pack("<I", 0) + struct.pack("<I", 0) + body


def aodt():
    body = bytearray(24)
    struct.pack_into("<I", body, 0, 1)      # Type = Cuirass
    struct.pack_into("<f", body, 4, 27.0)   # Weight
    struct.pack_into("<I", body, 8, 5250)   # Value
    struct.pack_into("<I", body, 12, 1200)  # Health
    struct.pack_into("<I", body, 16, 180)   # Enchant Points
    struct.pack_into("<I", body, 20, 40)    # Armor Rating
    return sub("AODT", bytes(body))


def armo(modl, fnam, itex, bnam, cnam=None):
    body = sub("NAME", zstr("dreugh_cuirass_ttrm"))
    body += sub("MODL", zstr(modl))
    body += sub("FNAM", zstr(fnam))
    body += aodt()
    body += sub("ITEX", zstr(itex))
    body += sub("ENAM", zstr("daedric endurance_en"))
    body += sub("INDX", struct.pack("<I", CUIRASS_INDX))
    body += sub("BNAM", zstr(bnam))
    if cnam is not None:
        body += sub("CNAM", zstr(cnam))

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


FILES = [
    ("armo_cnam_master.esm", True, "Morrowind.esm",
     ("a\\A_Dreugh_Cuirass_GND.NIF", "Dreugh Cuirass", "a\\tx_dreugh_cuirass.tga", "a_dreugh_cuirass", None)),
    ("armo_cnam_esp1.esp", False, "Unique Uniques - Items",
     ("Anu\\uni\\a\\cuirass_dreu_gnd.nif", "Dreugh Cuirass", "Anu\\uni\\a\\cuirass_dreugh.dds",
      "uni_cuirass_dreugh", "uni_cuirass_dreugh_f")),
    ("armo_cnam_esp2.esp", False, "Rational Names",
     ("a\\A_Dreugh_Cuirass_GND.NIF", "Dreugh Warlord Cuirass", "a\\tx_dreugh_cuirass.tga", "a_dreugh_cuirass", None)),
]


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    for filename, is_master, description, fields in FILES:
        data = tes3_header(is_master, description) + armo(*fields)
        with open(os.path.join(OUT_DIR, filename), "wb") as handle:
            handle.write(data)
        cnam = fields[4] if fields[4] else "(none)"
        print(f"wrote {filename}: BNAM={fields[3]} CNAM={cnam}")

    print("\nExpected merged body part: BNAM=uni_cuirass_dreugh CNAM=uni_cuirass_dreugh_f")


if __name__ == "__main__":
    main()
