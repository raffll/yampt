"""Generate real Morrowind plugin files (.esm/.esp) that demonstrate yEditor's
merge behaviors, so they can be opened in yEditor and inspected visually.

Output: tests/merge_fixtures/<scenario>/  (relative to the working directory)

Each scenario folder holds a master .esm plus one or more plugin .esp files.
Load the whole folder in yEditor (Open Folder) and create a merged patch to
see the merged result. Plugins are ordered by modification time so the ".esp"
files load after the master; p2 loads after p1.

Run from the repo root:  py scripts/make_merge_fixtures.py
"""

import os
import struct
import time


def sub(sub_type, data):
    return sub_type.encode("ascii") + struct.pack("<I", len(data)) + data


def record(rec_type, body, flags=0):
    header = rec_type.encode("ascii") + struct.pack("<I", len(body)) + struct.pack("<I", 0) + struct.pack("<I", flags)
    return header + body


def zstr(text):
    return text.encode("ascii") + b"\x00"


def u16(value):
    return struct.pack("<H", value)


def u32(value):
    return struct.pack("<I", value)


def tes3_header(description):
    hedr = struct.pack("<f", 1.3)
    hedr += struct.pack("<I", 0)
    hedr += description.encode("ascii")[:32].ljust(32, b"\x00")
    hedr += b"".ljust(256, b"\x00")
    hedr += struct.pack("<I", 0)
    return record("TES3", sub("HEDR", hedr))


def levi(list_id, data_flags, chance_none, entries):
    body = sub("NAME", zstr(list_id))
    body += sub("DATA", u32(data_flags))
    body += sub("NNAM", bytes([chance_none]))
    body += sub("INDX", u32(len(entries)))
    for item_id, level in entries:
        body += sub("INAM", zstr(item_id))
        body += sub("INTV", u16(level))
    return record("LEVI", body)


def levc(list_id, data_flags, chance_none, entries):
    body = sub("NAME", zstr(list_id))
    body += sub("DATA", u32(data_flags))
    body += sub("NNAM", bytes([chance_none]))
    body += sub("INDX", u32(len(entries)))
    for creature_id, level in entries:
        body += sub("CNAM", zstr(creature_id))
        body += sub("INTV", u16(level))
    return record("LEVC", body)


NPC_FLAG_AUTOCALC = 0x0010


def npdt_autocalc(level, gold):
    data = bytearray(12)
    struct.pack_into("<H", data, 0, level)
    struct.pack_into("<I", data, 8, gold)
    return bytes(data)


def npdt_explicit(level, gold):
    data = bytearray(52)
    struct.pack_into("<H", data, 0, level)
    struct.pack_into("<I", data, 48, gold)
    return bytes(data)


def npc(npc_id, flags, npdt=None):
    body = sub("NAME", zstr(npc_id))
    body += sub("FNAM", zstr(npc_id.replace("_", " ").title()))
    if npdt is not None:
        body += sub("NPDT", npdt)
    body += sub("FLAG", u32(flags))
    return record("NPC_", body)


def write_plugin(folder, filename, records):
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, filename)
    with open(path, "wb") as file_handle:
        for rec in records:
            file_handle.write(rec)
    return path


def stamp_order(paths):
    base = time.time()
    for index, path in enumerate(paths):
        stamp = base + index
        os.utime(path, (stamp, stamp))


# Every leveled-list scenario is a separate record ID that lives in the master
# and in whichever plugins participate. One folder = one master + three plugins;
# create a single merged patch to inspect every expected result at once.
#
# List ID                | Master              | Plugin1        | Plugin2                 | Plugin3   | Expected merged
# sc_level_change         | dwe_long@13         | -              | dwe_long@14             | -         | dwe_long@14
# sc_level_conflict       | dwe_long@13         | dwe_long@20    | -                       | dwe_long@30 | dwe_long@30 (last wins)
# sc_count_change         | gold@x1             | -              | gold@x3                 | -         | gold@x3
# sc_count_conflict       | gold@x1             | gold@x2        | -                       | gold@x5   | gold@x5 (last wins)
# sc_count_not_summed     | (empty)             | iron@x3        | -                       | iron@x3   | iron@x3 (not x6)
# sc_count_from_last      | (empty)             | iron@x3        | iron@x2                 | -         | iron@x2 (last wins)
# sc_weighting            | iron@x3             | iron@x3        | -                       | -         | iron@x3 (preserved)
# sc_unchanged_level      | dwe@13              | -              | dwe@13 + newitem@5      | -         | dwe@13, newitem@5
# sc_remove_item          | keep@1, drop@1      | -              | keep@1                  | -         | keep only
# sc_remove_wins          | contested@1,safe@1  | safe@1         | contested@1x2,safe@1    | -         | safe only (deletion wins)
# sc_no_delete_all_keep   | item@1              | item@x2        | item@1                  | -         | item present
# sc_add_different        | item_a@1            | item_a,item_b  | item_a,item_c@5         | -         | a, b, c
# sc_calc_flags           | flags=0             | flags=1 (bit0) | flags=2 (bit1)          | -         | flags=3 (both)
# sc_creature_list (LEVC) | rat@1               | -              | rat@1, mudcrab@3        | -         | rat, mudcrab
#
# NPC scenarios (NPC_ records, from earlier merge items):
# NPC ID                  | Master              | Plugin1        | Plugin2                 | Plugin3   | Expected merged
# sc_npc_flag_bits         | FLAG=0             | FLAG=Female    | FLAG=Essential          | -         | FLAG=Female+Essential (per bit)
# sc_npc_autocalc          | autocalc NPDT g=10 | -              | explicit NPDT g=500     | -         | explicit stats win, autocalc bit cleared


def master_records():
    return [
        tes3_header("merge fixtures master"),
        levi("sc_level_change", 1, 0, [("dwe_long", 13)]),
        levi("sc_level_conflict", 1, 0, [("dwe_long", 13)]),
        levi("sc_count_change", 1, 0, [("gold_001", 1)]),
        levi("sc_count_conflict", 1, 0, [("gold_001", 1)]),
        levi("sc_count_not_summed", 1, 0, []),
        levi("sc_count_from_last", 1, 0, []),
        levi("sc_weighting", 1, 0, [("iron_sword", 1), ("iron_sword", 1), ("iron_sword", 1)]),
        levi("sc_unchanged_level", 1, 0, [("dwe_long", 13)]),
        levi("sc_remove_item", 1, 0, [("keep_item", 1), ("drop_item", 1)]),
        levi("sc_remove_wins", 1, 0, [("contested", 1), ("safe_item", 1)]),
        levi("sc_no_delete_all_keep", 1, 0, [("item_a", 1)]),
        levi("sc_add_different", 1, 0, [("item_a", 1)]),
        levi("sc_calc_flags", 0, 0, [("iron_dagger", 1)]),
        levc("sc_creature_list", 1, 0, [("rat", 1)]),
        npc("sc_npc_flag_bits", 0),
        npc("sc_npc_autocalc", NPC_FLAG_AUTOCALC, npdt_autocalc(5, 10)),
    ]


def plugin1_records():
    return [
        tes3_header("plugin 1"),
        levi("sc_level_conflict", 1, 0, [("dwe_long", 20)]),
        levi("sc_count_conflict", 1, 0, [("gold_001", 1), ("gold_001", 1)]),
        levi("sc_count_not_summed", 1, 0, [("iron_sword", 1), ("iron_sword", 1), ("iron_sword", 1)]),
        levi("sc_count_from_last", 1, 0, [("iron_sword", 1), ("iron_sword", 1), ("iron_sword", 1)]),
        levi("sc_weighting", 1, 0, [("iron_sword", 1), ("iron_sword", 1), ("iron_sword", 1)]),
        levi("sc_remove_wins", 1, 0, [("safe_item", 1)]),
        levi("sc_no_delete_all_keep", 1, 0, [("item_a", 1), ("item_a", 1)]),
        levi("sc_add_different", 1, 0, [("item_a", 1), ("item_b", 1)]),
        levi("sc_calc_flags", 1, 0, [("iron_dagger", 1)]),
        npc("sc_npc_flag_bits", 0x0001),
    ]


def plugin2_records():
    return [
        tes3_header("plugin 2"),
        levi("sc_level_change", 1, 0, [("dwe_long", 14)]),
        levi("sc_count_change", 1, 0, [("gold_001", 1), ("gold_001", 1), ("gold_001", 1)]),
        levi("sc_count_from_last", 1, 0, [("iron_sword", 1), ("iron_sword", 1)]),
        levi("sc_unchanged_level", 1, 0, [("dwe_long", 13), ("new_item", 5)]),
        levi("sc_remove_item", 1, 0, [("keep_item", 1)]),
        levi("sc_remove_wins", 1, 0, [("contested", 1), ("contested", 1), ("safe_item", 1)]),
        levi("sc_no_delete_all_keep", 1, 0, [("item_a", 1)]),
        levi("sc_add_different", 1, 0, [("item_a", 1), ("item_c", 5)]),
        levi("sc_calc_flags", 2, 0, [("iron_dagger", 1)]),
        levc("sc_creature_list", 1, 0, [("rat", 1), ("mudcrab", 3)]),
        npc("sc_npc_flag_bits", 0x0002),
        npc("sc_npc_autocalc", 0, npdt_explicit(5, 500)),
    ]


def plugin3_records():
    return [
        tes3_header("plugin 3"),
        levi("sc_level_conflict", 1, 0, [("dwe_long", 30)]),
        levi("sc_count_conflict", 1, 0, [("gold_001", 1), ("gold_001", 1), ("gold_001", 1),
                                         ("gold_001", 1), ("gold_001", 1)]),
        levi("sc_count_not_summed", 1, 0, [("iron_sword", 1), ("iron_sword", 1), ("iron_sword", 1)]),
    ]


def main():
    folder = os.path.join("tests", "merge_fixtures", "leveled_lists")
    master = write_plugin(folder, "Master.esm", master_records())
    plugin1 = write_plugin(folder, "Plugin1.esp", plugin1_records())
    plugin2 = write_plugin(folder, "Plugin2.esp", plugin2_records())
    plugin3 = write_plugin(folder, "Plugin3.esp", plugin3_records())
    stamp_order([master, plugin1, plugin2, plugin3])

    print("Wrote merge fixtures to " + os.path.abspath(folder))
    print("Open the folder in yEditor (Open Folder), load all four, and Create Merged Patch.")
    print("Each sc_* leveled list demonstrates one scenario; see the table in this script.")


if __name__ == "__main__":
    main()
