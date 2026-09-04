# All-record-types test plugin

`AllTypes.esp` contains exactly one record of every TES3 record type, for
exercising yEditor's decode, navigation, and record view across all types.

Each record carries its `NAME` (id) and, where the type uses one, an `FNAM`
(display name), plus the type-specific data sub-record sized to match yampt's
decode schema (`sub_record_schema.cpp`) so the record view decodes it into
readable fields rather than raw bytes. Data payloads are mostly zero-filled
(valid size, neutral values); a few carry sensible values (e.g. WEAP Health,
NPC_ Level) for readability.

## Record types included (41)

GMST, GLOB, CLAS, FACT, RACE, SOUN, SKIL, MGEF, SCPT, REGN, BSGN, LTEX, STAT,
DOOR, MISC, WEAP, CONT, SPEL, CREA, BODY, LIGH, ENCH, NPC_, ARMO, CLOT, REPA,
ACTI, APPA, LOCK, PROB, INGR, BOOK, ALCH, LEVI, LEVC, CELL, LAND, PGRD, SNDG,
DIAL, INFO.

Notes:
- The `CELL` is an interior cell with one placed reference (FRMR + NAME + DATA).
- The `DIAL` topic is followed by one `INFO` that belongs to it (INFO chain).
- `LEVI`/`LEVC` each hold one list entry (referencing `test_weapon` / `test_crea`).
- Cross-references (e.g. the leveled lists and the cell's placed object point at
  `test_weapon`/`test_crea`) are by id string; the referenced records exist in the
  same plugin.

## Regenerate

Built by a throwaway Python script (write / run / delete per the Kiro helper-script
rule). The layouts follow `yampt.core/source/decoder/sub_record_schema.cpp`; if a
schema's expected size changes, regenerate to match.
