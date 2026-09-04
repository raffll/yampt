#!/usr/bin/env python3
"""Generate synthetic TES3 test fixtures for the yampt merger.

Produces three plugins in scripts/fixtures/:
  merge_master.esm   - the base master (versions.front / "first")
  merge_esp1.esp     - intermediate plugin
  merge_esp2.esp     - highest-priority plugin (versions.back / "winner")

Every record below is annotated with the exact merge path it exercises.
Load order for the merger is master -> esp1 -> esp2, so esp2 is the winner.

Wire format (little-endian):
  sub-record: 4-char type + u32 size + data
  record:     4-char type + u32 body_size + u32 header1 + u32 flags + body
"""

import os
import struct

OUT_DIR = os.path.join(os.path.dirname(__file__), "fixtures")

FLAG_BLOCKED = 0x2000
FLAG_PERSISTENT = 0x0400


# --------------------------------------------------------------------------
# byte builders
# --------------------------------------------------------------------------
def sub(sub_type, data):
    assert len(sub_type) == 4
    return sub_type.encode("ascii") + struct.pack("<I", len(data)) + data


def zstr(text):
    """Null-terminated string sub-record payload."""
    return text.encode("ascii") + b"\x00"


def fixed_str(text, length):
    raw = text.encode("ascii")[:length]
    return raw + b"\x00" * (length - len(raw))


def record(rec_type, body, flags=0, header1=0):
    assert len(rec_type) == 4
    header = rec_type.encode("ascii")
    header += struct.pack("<I", len(body))
    header += struct.pack("<I", header1)
    header += struct.pack("<I", flags)
    return header + body


# --------------------------------------------------------------------------
# domain sub-record helpers
# --------------------------------------------------------------------------
def name(text):
    return sub("NAME", zstr(text))


def fnam(text):
    return sub("FNAM", zstr(text))


def intv_u32(value):
    return sub("INTV", struct.pack("<I", value))


def intv_u16(value):
    return sub("INTV", struct.pack("<H", value))


def data_str(text):
    return sub("DATA", zstr(text))


def cell_data(flags, grid_x=0, grid_y=0):
    return sub("DATA", struct.pack("<iii", flags, grid_x, grid_y))


def ambi(ambient, sunlight, fog_color, fog_density):
    return sub("AMBI", struct.pack("<IIIf", ambient, sunlight, fog_color, fog_density))


def npc_npdt_52(level, strength, health):
    body = bytearray(52)
    struct.pack_into("<H", body, 0, level)
    body[2] = strength
    struct.pack_into("<H", body, 38, health)
    return sub("NPDT", bytes(body))


def npc_npdt_12(level, gold):
    body = bytearray(12)
    struct.pack_into("<H", body, 0, level)
    struct.pack_into("<I", body, 8, gold)
    return sub("NPDT", bytes(body))


def npc_flag(value):
    return sub("FLAG", struct.pack("<I", value))


def npco(count, item_id):
    return sub("NPCO", struct.pack("<i", count) + fixed_str(item_id, 32))


def wpdt(weight, value, wtype, health, speed):
    body = bytearray(32)
    struct.pack_into("<f", body, 0, weight)
    struct.pack_into("<I", body, 4, value)
    struct.pack_into("<H", body, 8, wtype)
    struct.pack_into("<H", body, 10, health)
    struct.pack_into("<f", body, 12, speed)
    return sub("WPDT", bytes(body))


def enam_slot(effect, magnitude_min, magnitude_max):
    body = bytearray(24)
    struct.pack_into("<H", body, 0, effect)
    struct.pack_into("<I", body, 12, magnitude_min)
    struct.pack_into("<I", body, 16, magnitude_max)
    return body


def enam(effect, magnitude_min, magnitude_max):
    return sub("ENAM", enam_slot(effect, magnitude_min, magnitude_max))


def dial(text, dtype):
    return record("DIAL", name(text) + sub("DATA", struct.pack("<B", dtype)))


def info(inam, prev, response, dial_kind=0):
    body = name(inam)
    if prev:
        body += sub("PNAM", zstr(prev))
    body += sub("DATA", struct.pack("<IIbbb", dial_kind, 0, 0, -1, 0) + b"\x00")
    body += sub("NAME", zstr(response))
    return record("INFO", body)


def frmr(index, obj_id, extra=b""):
    return sub("FRMR", struct.pack("<I", index)) + name(obj_id) + extra


def levi(list_id, flags, items):
    body = name(list_id)
    body += sub("DATA", struct.pack("<I", flags))
    body += sub("NNAM", struct.pack("<B", 0))
    body += sub("INDX", struct.pack("<I", len(items)))
    for item_id, level in items:
        body += sub("INAM", zstr(item_id))
        body += intv_u16(level)
    return record("LEVI", body)


def gmst(setting_id, value):
    body = name(setting_id)
    if setting_id.startswith("s"):
        body += sub("STRV", zstr(value))
    elif setting_id.startswith("i"):
        body += sub("INTV", struct.pack("<i", value))
    else:
        body += sub("FLTV", struct.pack("<f", value))
    return record("GMST", body)


# --------------------------------------------------------------------------
# TES3 header
# --------------------------------------------------------------------------
def tes3_header(is_master, description, record_count):
    version = struct.pack("<f", 1.3)
    file_flags = struct.pack("<I", 1 if is_master else 0)
    author = fixed_str("yampt fixtures", 32)
    desc = fixed_str(description, 256)
    count = struct.pack("<I", record_count)
    hedr = sub("HEDR", version + file_flags + author + desc + count)
    return record("TES3", hedr)


# --------------------------------------------------------------------------
# fixture contents
# --------------------------------------------------------------------------
def build_master():
    recs = []

    # --- LTEX: three-way + the LTEX:INTV exclude rule (screenshot case) ---
    recs.append(record("LTEX", name("AI_Grass_Dirt") + intv_u32(36) + data_str("tx_ai_grass_dirt_01.tga")))

    # --- WEAP: element-wise WPDT field merge (disjoint fields) ---
    recs.append(record("WEAP", name("iron_dagger") + fnam("Iron Dagger") + wpdt(8.0, 10, 0, 100, 1.5)))

    # --- NPC_: element-wise 52-byte NPDT field merge ---
    recs.append(record("NPC_", name("hlaalu_guard") + fnam("Hlaalu Guard") + npc_flag(0x0002) + npc_npdt_52(5, 40, 60)))

    # --- NPC_: NPDT size mismatch (52 here, esp1 switches to 12 -> esp1 skipped) ---
    recs.append(record("NPC_", name("mismatch_npc") + fnam("Mismatch NPC") + npc_npdt_52(3, 30, 50)))

    # --- NPC_: NPCO inventory union by 32-byte item id ---
    recs.append(record("NPC_", name("merchant") + fnam("Merchant") + npc_npdt_52(10, 50, 80)
                        + npco(1, "gold_001") + npco(1, "iron_dagger")))

    # --- ENCH: ENAM 24-byte slot merge + magnitude min/max pairing ---
    recs.append(record("ENCH", name("ench_fire") + sub("ENDT", struct.pack("<IIII", 0, 10, 100, 0))
                        + enam(1, 5, 10)))

    # --- CREA: known summon, persistent bit clear -> summon fixer sets it ---
    crea_body = bytearray(name("scamp_summon") + fnam("Summoned Scamp"))
    recs.append(record("CREA", bytes(crea_body)))

    # --- CELL interior: fog fixer (AMBI fog density 0 -> 0.01) ---
    recs.append(record("CELL", name("Fog Test Interior") + cell_data(0x01)
                       + ambi(0x404040, 0x808080, 0x202020, 0.0)))

    # --- CELL exterior: cell name fixer (esp1 renames, esp2 reverts) ---
    recs.append(record("CELL", name("Old Region Name") + cell_data(0x00, 5, 5)))

    # --- CELL with FRMR reference groups (merge_cell_refs path) ---
    recs.append(record("CELL", name("Ref Test Interior") + cell_data(0x01)
                       + frmr(1, "chest_01") + frmr(2, "torch_01")))

    # --- DIAL + INFO children: dialogue union merge ---
    recs.append(dial("Background", 0))
    recs.append(info("info_bg_1", "", "Original background response.", 0))

    # --- LEVI leveled list: union + deletion + max count ---
    recs.append(levi("list_creatures", 0x01, [("rat", 1), ("cliff_racer", 3)]))

    # --- GMST: string, int, float variants ---
    recs.append(gmst("sMasterText", "Original"))
    recs.append(gmst("iMasterInt", 100))

    # --- Record present ONLY in master (versions<2 -> never merged) ---
    recs.append(record("MISC", name("master_only_item") + fnam("Master Only")
                       + sub("MCDT", struct.pack("<fII", 1.0, 5, 0))))

    # --- Identical across all three (prune_unchanged removes no-op) ---
    recs.append(record("MISC", name("identical_item") + fnam("Identical")
                       + sub("MCDT", struct.pack("<fII", 2.0, 10, 0))))

    # --- Blocked-flag record (header flags preserved from winner) ---
    recs.append(record("BOOK", name("blocked_book") + fnam("Blocked Book")
                       + sub("BKDT", bytes(20)), flags=FLAG_BLOCKED))

    header = tes3_header(True, "yampt merge fixtures - master", len(recs))
    return header + b"".join(recs)


def build_esp1():
    recs = []

    # LTEX: esp1 changes INTV (conflict; excluded when LTEX:INTV rule set)
    recs.append(record("LTEX", name("AI_Grass_Dirt") + intv_u32(6) + data_str("tx_ai_grass_dirt_01.tga")))

    # WEAP: esp1 changes Value (offset 4) only
    recs.append(record("WEAP", name("iron_dagger") + fnam("Iron Dagger") + wpdt(8.0, 25, 0, 100, 1.5)))

    # NPC_ element-wise: esp1 changes Strength (offset 2)
    recs.append(record("NPC_", name("hlaalu_guard") + fnam("Hlaalu Guard") + npc_flag(0x0002) + npc_npdt_52(5, 99, 60)))

    # NPC_ mismatch: esp1 switches to 12-byte NPDT -> this intermediate skipped
    recs.append(record("NPC_", name("mismatch_npc") + fnam("Mismatch NPC") + npc_npdt_12(3, 500)))

    # NPC_ NPCO: esp1 adds a new item
    recs.append(record("NPC_", name("merchant") + fnam("Merchant") + npc_npdt_52(10, 50, 80)
                        + npco(1, "gold_001") + npco(1, "iron_dagger") + npco(1, "healing_potion")))

    # ENCH: esp1 raises magnitude min (should re-couple with max)
    recs.append(record("ENCH", name("ench_fire") + sub("ENDT", struct.pack("<IIII", 0, 10, 100, 0))
                        + enam(1, 8, 10)))

    # CREA: unchanged (summon fixer acts regardless)
    recs.append(record("CREA", name("scamp_summon") + fnam("Summoned Scamp")))

    # CELL fog: unchanged content
    recs.append(record("CELL", name("Fog Test Interior") + cell_data(0x01)
                       + ambi(0x404040, 0x808080, 0x202020, 0.0)))

    # CELL exterior: esp1 renames the cell
    recs.append(record("CELL", name("New Region Name") + cell_data(0x00, 5, 5)))

    # CELL FRMR: esp1 adds a new reference group (frmr index 3)
    recs.append(record("CELL", name("Ref Test Interior") + cell_data(0x01)
                       + frmr(1, "chest_01") + frmr(2, "torch_01") + frmr(3, "urn_01")))

    # DIAL: esp1 edits the existing INFO and adds a new one
    recs.append(dial("Background", 0))
    recs.append(info("info_bg_1", "", "Edited background response from esp1.", 0))
    recs.append(info("info_bg_2", "info_bg_1", "New response added by esp1.", 0))

    # LEVI: esp1 adds kwama, removes cliff_racer
    recs.append(levi("list_creatures", 0x01, [("rat", 1), ("kwama_forager", 2)]))

    # GMST: esp1 changes string value
    recs.append(gmst("sMasterText", "Edited by esp1"))
    recs.append(gmst("iMasterInt", 100))

    # identical MISC (unchanged)
    recs.append(record("MISC", name("identical_item") + fnam("Identical")
                       + sub("MCDT", struct.pack("<fII", 2.0, 10, 0))))

    # blocked book (unchanged)
    recs.append(record("BOOK", name("blocked_book") + fnam("Blocked Book")
                       + sub("BKDT", bytes(20)), flags=FLAG_BLOCKED))

    # Record added ONLY by esp1 (present in one plugin only from esp side)
    recs.append(record("MISC", name("esp1_only_item") + fnam("Esp1 Only")
                       + sub("MCDT", struct.pack("<fII", 3.0, 15, 0))))

    header = tes3_header(False, "yampt merge fixtures - esp1", len(recs))
    return header + b"".join(recs)


def build_esp2():
    recs = []

    # LTEX: esp2 (winner) keeps master INTV=36 -> row exists, INTV differs from esp1
    recs.append(record("LTEX", name("AI_Grass_Dirt") + intv_u32(36) + data_str("tx_ai_grass_dirt_01.tga")))

    # WEAP: esp2 changes Speed (offset 12) only -> disjoint from esp1's Value edit
    recs.append(record("WEAP", name("iron_dagger") + fnam("Iron Dagger") + wpdt(8.0, 10, 0, 100, 2.5)))

    # NPC_ element-wise: esp2 changes Health (offset 38) -> disjoint from esp1's Strength
    recs.append(record("NPC_", name("hlaalu_guard") + fnam("Hlaalu Guard") + npc_flag(0x0002) + npc_npdt_52(5, 40, 150)))

    # NPC_ mismatch: esp2 keeps 52-byte NPDT (winner)
    recs.append(record("NPC_", name("mismatch_npc") + fnam("Mismatch NPC") + npc_npdt_52(3, 30, 50)))

    # NPC_ NPCO: esp2 adds a different new item
    recs.append(record("NPC_", name("merchant") + fnam("Merchant") + npc_npdt_52(10, 50, 80)
                        + npco(1, "gold_001") + npco(1, "iron_dagger") + npco(1, "silver_ring")))

    # ENCH: esp2 leaves magnitude at master values (esp1's change wins by three-way)
    recs.append(record("ENCH", name("ench_fire") + sub("ENDT", struct.pack("<IIII", 0, 10, 100, 0))
                        + enam(1, 5, 10)))

    # CREA: unchanged
    recs.append(record("CREA", name("scamp_summon") + fnam("Summoned Scamp")))

    # CELL fog: unchanged (winner) -> fog fixer runs on the merged/winner content
    recs.append(record("CELL", name("Fog Test Interior") + cell_data(0x01)
                       + ambi(0x404040, 0x808080, 0x202020, 0.0)))

    # CELL exterior: esp2 reverts to master name -> cell name fixer restores esp1's rename
    recs.append(record("CELL", name("Old Region Name") + cell_data(0x00, 5, 5)))

    # CELL FRMR: esp2 keeps master groups (1,2); esp1's group 3 is an intermediate addition
    recs.append(record("CELL", name("Ref Test Interior") + cell_data(0x01)
                       + frmr(1, "chest_01") + frmr(2, "torch_01")))

    # DIAL: esp2 adds yet another INFO -> union of all INFO children
    recs.append(dial("Background", 0))
    recs.append(info("info_bg_1", "", "Original background response.", 0))
    recs.append(info("info_bg_3", "info_bg_1", "New response added by esp2.", 0))

    # LEVI: esp2 adds guar -> merged = rat + kwama (esp1) + guar (esp2), cliff_racer deleted
    recs.append(levi("list_creatures", 0x01, [("rat", 1), ("guar", 2)]))

    # GMST: esp2 leaves string at master -> esp1's edit wins by three-way
    recs.append(gmst("sMasterText", "Original"))
    recs.append(gmst("iMasterInt", 250))

    # identical MISC (unchanged) -> pruned
    recs.append(record("MISC", name("identical_item") + fnam("Identical")
                       + sub("MCDT", struct.pack("<fII", 2.0, 10, 0))))

    # blocked book (unchanged) -> pruned; header flags preserved when kept
    recs.append(record("BOOK", name("blocked_book") + fnam("Blocked Book")
                       + sub("BKDT", bytes(20)), flags=FLAG_BLOCKED))

    # Record added ONLY by esp2
    recs.append(record("MISC", name("esp2_only_item") + fnam("Esp2 Only")
                       + sub("MCDT", struct.pack("<fII", 4.0, 20, 0))))

    header = tes3_header(False, "yampt merge fixtures - esp2", len(recs))
    return header + b"".join(recs)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    outputs = {
        "merge_master.esm": build_master(),
        "merge_esp1.esp": build_esp1(),
        "merge_esp2.esp": build_esp2(),
    }
    for filename, data in outputs.items():
        path = os.path.join(OUT_DIR, filename)
        with open(path, "wb") as handle:
            handle.write(data)
        print(f"wrote {filename}: {len(data)} bytes")


if __name__ == "__main__":
    main()
