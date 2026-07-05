# Sub-Record Label Reference

Authoritative reference of all sub-record display labels in yEditor's view tree.
Labels resolve via: context description > schema label > generic description > raw code.

## Context Overrides (Record + Sub pairs)

| Record | Sub | Label |
|--------|-----|-------|
| ARMO | BNAM | Male Part Name |
| ARMO | CNAM | Female Part Name |
| ARMO | ENAM | Enchantment |
| CLOT | BNAM | Male Part Name |
| CLOT | CNAM | Female Part Name |
| CLOT | ENAM | Enchantment |
| WEAP | ENAM | Enchantment |
| BOOK | ENAM | Enchantment |
| CELL | ANAM | Owner |
| CELL | BNAM | Global/Rank |
| CELL | CNAM | Previous Owner |
| CELL | XCHG | Charge |
| CELL | XSOL | Soul |
| CELL | INTV | Charge/Count |
| CELL | FLTV | Lock Level |
| CELL | INDX | Faction Index |
| NPC_ | ANAM | Faction |
| NPC_ | BNAM | Head Model |
| NPC_ | CNAM | Class |
| NPC_ | DNAM | Hair Model |
| NPC_ | KNAM | Hair |
| NPC_ | CNDT | Cell Travel |
| INFO | ANAM | Cell |
| INFO | BNAM | Result Script |
| INFO | CNAM | Class |
| INFO | DNAM | PC Faction |
| INFO | FNAM | Faction |
| INFO | NAME | Response |
| INFO | SNAM | Sound File |
| INFO | RNAM | Race |
| INFO | INTV | Comparison Value |
| INFO | FLTV | Comparison Value |
| DOOR | SNAM | Open Sound |
| DOOR | ANAM | Close Sound |
| FACT | RNAM | Rank Name |
| FACT | ANAM | Reaction Faction |
| FACT | INTV | Reaction Value |
| LTEX | INTV | Texture Index |
| LTEX | DATA | Texture File |
| LAND | INTV | Grid |
| LAND | DATA | Flags |
| LAND | VNML | Normals |
| LAND | VHGT | Heights |
| LAND | WNAM | World Map |
| LAND | VCLR | Vertex Colors |
| LAND | VTEX | Textures |
| LEVC | CNAM | Creature |
| LEVC | NNAM | Chance None |
| LEVC | INDX | Count |
| LEVC | INTV | PC Level |
| LEVI | INAM | Item |
| LEVI | NNAM | Chance None |
| LEVI | INDX | Count |
| LEVI | INTV | PC Level |
| BSGN | TNAM | Texture |
| REGN | BNAM | Sleep Creature |
| REGN | CNAM | Map Color |
| REGN | SNAM | Sound Chance |
| SNDG | CNAM | Creature |
| SNDG | SNAM | Sound ID |
| SOUN | FNAM | Sound File |
| CREA | CNAM | Sound Gen Creature |
| MGEF | PTEX | Particle Texture |
| MGEF | ASND | Area Sound |
| MGEF | BSND | Bolt Sound |
| MGEF | CSND | Cast Sound |
| MGEF | HSND | Hit Sound |
| MGEF | AVFX | Area VFX |
| MGEF | BVFX | Bolt VFX |
| MGEF | CVFX | Cast VFX |
| MGEF | HVFX | Hit VFX |
| GLOB | FNAM | Type |
| PGRD | PGRP | Points |
| PGRD | PGRC | Connections |
| ALCH | TEXT | Icon |
| SCPT | SCVR | Variables |
| SCPT | SCDT | Bytecode |
| GMST | STRV | Value |
| GMST | INTV | Value |
| GMST | FLTV | Value |

## Generic Descriptions (fallback for all records)

| Sub | Label |
|-----|-------|
| NAME | ID |
| FNAM | Name |
| MODL | Model |
| SCRI | Script |
| ITEX | Icon |
| ENAM | Effect |
| ANAM | Faction/Owner |
| BNAM | Script Text |
| CNAM | Class |
| DNAM | Destination |
| ONAM | Actor |
| RNAM | Race |
| INDX | Index |
| INTV | Integer Value |
| FLTV | Float Value |
| STRV | String Value |
| INAM | Info ID |
| PNAM | Previous Info |
| NNAM | Next Info |
| SNAM | Sound |
| DATA | Data |
| FLAG | Flags |
| AIDT | AI Data |
| WEAT | Weather |
| WHGT | Water Height |
| AMBI | Ambient Light |
| RGNN | Region Name |
| DELE | Deleted |
| SCVR | Script Variable |
| SCHD | Header |
| SCTX | Script Source |
| SCDT | Compiled |
| HEDR | Header |
| MAST | Master File |
| DODT | Door Destination |
| FRMR | Object Reference |
| XSCL | Scale |
| NAM0 | Object Count |
| NAM5 | Map Color |
| NPCO | Item |
| NPCS | Spell/Ability |
| KNAM | Key |
| TNAM | Trap |
| UNAM | Blocked |
| AI_W | AI Wander |
| AI_T | AI Travel |
| AI_F | AI Follow |
| AI_E | AI Escort |
| AI_A | AI Activate |
| GLOB | Global |
| DESC | Description |
| TEXT | Text |
| QSTN | Quest Name |
| QSTF | Quest Finished |
| QSTR | Quest Restart |

## Record Display Names (used in schema labels)

| Code | Name |
|------|------|
| ACTI | Activator |
| ALCH | Potion |
| APPA | Apparatus |
| ARMO | Armor |
| BODY | Body Part |
| BOOK | Book |
| BSGN | Birthsign |
| CELL | Cell |
| CLAS | Class |
| CLOT | Clothing |
| CONT | Container |
| CREA | Creature |
| DIAL | Dialogue |
| DOOR | Door |
| ENCH | Enchantment |
| FACT | Faction |
| GLOB | Global |
| GMST | Game Setting |
| INFO | Info |
| INGR | Ingredient |
| LAND | Landscape |
| LEVI | Leveled Item |
| LEVC | Leveled Creature |
| LIGH | Light |
| LOCK | Lockpick |
| LTEX | Land Texture |
| MGEF | Magic Effect |
| MISC | Misc Item |
| NPC_ | NPC |
| PGRD | Pathgrid |
| PROB | Probe |
| RACE | Race |
| REGN | Region |
| REPA | Repair Item |
| SCPT | Script |
| SKIL | Skill |
| SNDG | Sound Generator |
| SOUN | Sound |
| SPEL | Spell |
| STAT | Static |
| WEAP | Weapon |

## Binary Sub-Records

These sub-records display as `<N bytes>` (field_type_t::binary):

| Record | Sub | Description |
|--------|-----|-------------|
| LAND | VNML | Vertex normals grid |
| LAND | VHGT | Vertex heights grid |
| LAND | WNAM | World map pixels |
| LAND | VCLR | Vertex colors grid |
| LAND | VTEX | Texture indices grid |
| PGRD | PGRP | Pathgrid point positions |
| PGRD | PGRC | Pathgrid connections |
| SCPT | SCVR | Script variable names |
| SCPT | SCDT | Compiled MWScript bytecode |
