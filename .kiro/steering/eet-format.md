# EET File Format

Binary format used by ESP-ESM Translator. Reverse-engineered from sample files — no official documentation exists.

## Header

```
Offset  Size  Field
0       4     magic: "EET_"
4       4     version: uint32 (always 1)
8       4     unknown: uint32 (seen: 3)
12      4     game_tag: "GAME"
16      2     game_name_len: uint16
18      N     game_name: ASCII (e.g. "Morrowind")
18+N    4     line_tag: "LINE"
22+N    4     entry_count: uint32
```

## Entry (repeated entry_count times)

All integers are little-endian. All strings are length-prefixed (uint32 length, then N bytes of data — no null terminator).

```
Offset  Size  Field
0       4     total_size: uint32 (bytes after this field until entry end)
4       4     rec_type_len: uint32
8       N     rec_type: ASCII ("GMST", "INFO", "CELL", "NPC_", etc.)
8+N     4     context_len: uint32 (0 for most records, non-zero for INFO)
12+N    M     context: UTF-8 (empty for non-INFO; INAM identifier for INFO)
12+N+M  4     key_len: uint32
16+N+M  K     key: UTF-8 (record ID: "sRestIllegal", "Barbarian", dialog name, etc.)
16+N+M+K 4    sub_type_len: uint32
20+N+M+K S    sub_type: ASCII ("STRV", "FNAM", "NAME", "DESC", "TEXT", "SCTX", etc.)
20+N+M+K+S 4  orig_len: uint32
24+N+M+K+S O  orig: UTF-8 (original text)
24+N+M+K+S+O 4 trans_len: uint32
28+N+M+K+S+O T trans: UTF-8 (translated text)
remaining      tail: variable-length metadata
```

## Tail Structure

The tail occupies all remaining bytes in the entry (total_size - consumed fields). Known fields:

```
Offset  Size  Field
0       4     unknown (always 0)
4       4     unknown (always 1)
8       1     status_byte
9       1     unknown (always 0x00)
10      4     color (0xFFFFFF00 seen)
14      4     unknown (always 0)
18      4     hash1 (CRC or similar)
22      4     stripped_orig_len: uint32
26      N     stripped_orig: original text with spaces/punctuation removed
26+N    4     hash2 (CRC or similar)
30+N    4     unknown (always 0)
34+N    4     unknown (always 0)
38+N    4     terminator (0xFFFFFFFF)
```

## Status Byte Values

| Value | Meaning |
|-------|---------|
| `0x63` ('c') | Confirmed/translated |
| `0xFF` | Untranslated/empty |

## Record Type + Sub Type Combinations (from Morrowind Rebirth sample)

| rec_type | sub_type | Count | yampt mapping |
|----------|----------|------:|---------------|
| NPC_ | FNAM | 3415 | fnam |
| CELL | NAME | 1425 | cell |
| SPEL | FNAM | 1098 | fnam |
| CELL | DNAM | 1094 | cell |
| WEAP | FNAM | 1054 | fnam |
| PGRD | NAME | 881 | cell |
| ARMO | FNAM | 755 | fnam |
| BOOK | FNAM | 754 | fnam |
| BOOK | TEXT | 747 | text |
| CONT | FNAM | 738 | fnam |
| MISC | FNAM | 722 | fnam |
| CLOT | FNAM | 708 | fnam |
| CREA | FNAM | 579 | fnam |
| ALCH | FNAM | 347 | fnam |
| DOOR | FNAM | 273 | fnam |
| ACTI | FNAM | 197 | fnam |
| LIGH | FNAM | 169 | fnam |
| INGR | FNAM | 156 | fnam |
| SCPT | SCTX | 152 | sctx |
| MGEF | DESC | 128 | desc |
| SCPT | MSGB | 59 | bnam |
| FACT | RNAM | 21 | rnam |
| CLAS | FNAM | 18 | fnam |
| SCPT | CELL | 7 | bnam |
| NPC_ | DNAM | 7 | fnam |
| CLAS | DESC | 5 | desc |
| SCPT | SAY_ | 5 | bnam |
| REGN | FNAM | 5 | cell |
| FACT | FNAM | 4 | fnam |
| SCPT | DIAL | 4 | bnam |
| NPC_ | CNDT | 3 | fnam |
| GMST | STRV | 2 | gmst |
| APPA | FNAM | 2 | fnam |
| REPA | FNAM | 2 | fnam |
| DIAL | NAME | 1 | dial |

## Context Field

- Empty (len=0) for all record types except INFO
- For INFO records: contains the INAM identifier string (a numeric sequence ID like "24834195854992215000")
- 1536 out of 17284 entries in the sample have non-empty context (all are INFO)

## Text Encoding

All text fields (context, key, orig, trans) are UTF-8 encoded. The EET application stores translations in UTF-8 regardless of the game's original codepage.
