# ESM File Format Reference

Source: Dave Humphrey's ESM format documentation.

## Record Structure

```
4 bytes: char Name[4]       — record type (e.g. "CELL", "NPC_", "INFO")
4 bytes: long Size           — size excluding 16-byte header
4 bytes: long Header1        — usually 0 (deleted/ignored flag)
4 bytes: long Flags          — 0x2000=Blocked, 0x0400=Persistent
? bytes: SubRecords[]        — variable count, use Size to know when to stop
```

## Sub-Record Structure

```
4 bytes: char Name[4]       — sub-record type (e.g. "DATA", "NAME", "FNAM")
4 bytes: long Size           — size excluding 8-byte header
? bytes: data                — format depends on sub-record type
```

## CELL Record (type 36)

```
NAME = Cell ID string (empty for exterior cells → region name used instead)
DATA = Cell Data (12 bytes)
    long Flags
        0x01 = Interior
        0x02 = Has Water
        0x04 = Illegal to Sleep
        0x80 = Behave like exterior (Tribunal)
    long GridX
    long GridY
RGNN = Region name string
NAM0 = Number of objects in cell (4 bytes, long, optional)
```

Interior-only sub-records:
```
WHGT = Water Height (4 bytes, float)
AMBI = Ambient Light (16 bytes): AmbientColor, SunlightColor, FogColor, FogDensity
```

Exterior-only sub-records:
```
NAM5 = Map Color (4 bytes, COLORREF)
```

Referenced Object Data (repeating groups inside CELL):
```
FRMR = Object Index (4 bytes, long) — unique ID within cell
NAME = Object ID string
XSCL = Scale (4 bytes, float, optional)
DODT = Door destination (24 bytes, Door objects only)
    float XPos, YPos, ZPos
    float XRotate, YRotate, ZRotate
DNAM = Door exit cell name (interior destination)
FLTV = Lock level (optional, follows DNAM)
KNAM = Door key ID
TNAM = Trap name
ANAM = Owner ID string
BNAM = Global variable/rank ID
INTV = Number of uses (4 bytes, long)
DATA = Reference Position (24 bytes)
    float XPos, YPos, ZPos
    float XRotate, YRotate, ZRotate
```

### Cell Coordinate Layout

- **Exterior cells**: DATA bytes 4-7 = GridX (int32 LE), bytes 8-11 = GridY (int32 LE). Flags byte 0 bit 0 = 0.
- **Interior cells**: Flags byte 0 bit 0 = 1. GridX/GridY are meaningless. Interior cells are identified by door markers (DODT+DNAM pairs) in other cells pointing to them.

### Door Markers (DODT + DNAM)

Inside a CELL record's referenced objects, a DODT sub-record followed by a DNAM sub-record indicates a door that teleports to another cell:
- DODT (24 bytes): destination XYZ position + XYZ rotation
- DNAM: destination cell name string (for interior cells)

If DNAM is absent, the door leads to an exterior cell (determined by DODT coordinates).

## NPC_ Record (type 23)

```
NAME = NPC ID string
FNAM = NPC display name
RNAM = Race Name
ANAM = Faction name
CNAM = Class name
FLAG = NPC Flags (4 bytes, long)
    0x0001 = Female
    0x0002 = Essential
    0x0004 = Respawn
    0x0010 = Autocalc
```

Gender: bit 0 of FLAG (0x0001). If set → Female, otherwise Male.

## INFO Record (type 41)

Dialogue response record, belongs to previous DIAL record.

```
INAM = Info ID string (unique sequence)
PNAM = Previous info ID
NNAM = Next info ID
DATA = Info data (12 bytes)
    long Unknown1
    long Disposition
    byte Rank
    byte Gender (0xFF=None, 0x00=Male, 0x01=Female)
    byte PCRank
    byte Unknown2
ONAM = Actor string (NPC ID filter)
RNAM = Race string
CNAM = Class string
FNAM = Faction string
ANAM = Cell string
DNAM = PC Faction string
NAME = Response text (512 max)
SNAM = Sound filename
SCVR = Condition variable string
BNAM = Result script text
```

### INFO's ANAM vs NPC_'s ANAM

- In INFO records: ANAM = Cell filter string
- In NPC_ records: ANAM = Faction name
- In CELL referenced objects: ANAM = Owner ID

The same sub-record ID means different things depending on parent record type.

## DIAL Record (type 40)

```
NAME = Dialogue ID string (topic name)
DATA = Dialogue Type (1 byte)
    0 = Regular Topic
    1 = Voice
    2 = Greeting
    3 = Persuasion
    4 = Journal
```

All following INFO records belong to this DIAL until the next DIAL or end of file.

## REGN Record (type 10)

```
NAME = Region ID string
FNAM = Region display name string
WEAT = Weather data (8 bytes)
BNAM = Sleep creature string
CNAM = Map Color (4 bytes)
SNAM = Sound records (multiple)
```

## GMST Record (type 1)

```
NAME = Setting ID string (starts with 's' for string, 'i' for int, 'f' for float)
STRV = String value
INTV = Integer value (4 bytes)
FLTV = Float value (4 bytes)
```

`sDefaultCellname` is the GMST that holds the wilderness cell name (e.g. "Wilderness" in EN, "Wildnis" in DE).

## Byte Order

All multi-byte integers are little-endian. Use `tools_t::convert_string_byte_array_to_uint()` for 4-byte values.

## yampt's esm_reader_t API

- `select_record(i)` — selects record at index i
- `get_record().id` — 4-char record type string
- `get_record().content` — raw bytes of the entire record (including 16-byte header)
- `set_key("SUBID")` / `set_value("SUBID")` — finds a sub-record within the selected record
- `get_key().exist` / `get_value().exist` — whether the sub-record was found
- `get_key().text` / `get_value().text` — null-terminated string content
- `get_key().content` / `get_value().content` — raw binary content (use for DATA, FLAG, etc.)
- `set_next_value("SUBID")` — finds next occurrence of the same sub-record type

Sub-record parsing starts at offset 16 (after the record header). The content returned by `get_value().content` is the raw sub-record data (excluding the 8-byte sub-record header).
