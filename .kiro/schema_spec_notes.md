# Wildcard-free schema spec (OpenMW-grounded, working notes)

Format per row: SUB — Label — [R]epeatable/[S]ingle — size(0=string/var) — layout-array

Data-block layouts already exist in sub_record_schema.cpp (reuse). String rows use the
existing *_string_fields arrays but need PER-PARENT label variants where meaning differs.

Repeatable set (from task 1): ENAM(effect), NPCO, NPCS, AI_W/T/F/E/A, DODT, DNAM,
FACT RNAM+ANAM+INTV, LEVI INAM+INTV, LEVC CNAM+INTV, REGN SNAM, ARMO/CLOT INDX+BNAM+CNAM.

## Item/object records (read order per OpenMW)

ACTI: NAME(ID,S) MODL(Model,S) FNAM(Name,S) SCRI(Script,S)
ALCH: NAME(ID) MODL(Model) TEXT(Icon,S) SCRI(Script) FNAM(Name) ALDT(Data,S,12) ENAM(Effect,R,24)
APPA: NAME MODL FNAM AADT(Data,S,16) SCRI ITEX(Icon)
ARMO: NAME MODL FNAM AODT(Data,S,24) SCRI ITEX ENAM(Enchantment,S,0-string) INDX(Armor Index,R,1) BNAM(Male Part,R,string) CNAM(Female Part,R,string)
BODY: NAME MODL FNAM(Race,S) BYDT(Data,S,4)
BOOK: NAME MODL FNAM BKDT(Data,S,20) SCRI ITEX ENAM(Enchantment,S,0) TEXT(Text,S,string)
BSGN: NAME FNAM TNAM(Texture,S) DESC(Description,S) NPCS(Power,R,string)
CLAS: NAME FNAM CLDT(Data,S,60) DESC
CLOT: NAME MODL FNAM CTDT(Data,S,12) SCRI ITEX ENAM(Enchantment,S,0) INDX(Clothing Index,R,1) BNAM(Male Part,R) CNAM(Female Part,R)
CONT: NAME MODL FNAM CNDT(Weight,S,4) FLAG(Flags,S,4) SCRI NPCO(Item,R,36)
CREA: NAME MODL CNAM(Original,S) FNAM SCRI NPDT(Data,S,96) FLAG(Flags,S,4) XSCL(Scale,S,4) NPCO(Item,R,36) NPCS(Spell,R) AIDT(AI Data,S,12) DODT(Travel Destination,R,24) DNAM(Destination Cell,R) AI_W(AI Wander,R,14) AI_T(AI Travel,R,16) AI_F(AI Follow,R,48) AI_E(AI Escort,R,48) AI_A(AI Activate,R,33) CNDT(AI Cell,R,string)
DOOR: NAME FNAM MODL SCRI SNAM(Open Sound,S) ANAM(Close Sound,S)
ENCH: NAME ENDT(Data,S,16) ENAM(Effect,R,24)
FACT: NAME FNAM RNAM(Rank Name,R,32) FADT(Data,S,240) ANAM(Reaction Faction,R,string) INTV(Reaction Value,R,4)
GLOB: NAME FNAM(Type,S,string-1char) FLTV(Value,S,4)
GMST: NAME STRV(Value,S,string) INTV(Value,S,4) FLTV(Value,S,4)
INGR: NAME MODL FNAM IRDT(Data,S,56) SCRI ITEX
LIGH: NAME MODL FNAM ITEX LHDT(Data,S,24) SCRI SNAM(Sound,S)
LOCK: NAME MODL FNAM LKDT(Data,S,16) SCRI ITEX
LTEX: NAME INTV(Index,S,4-uint32) DATA(Texture,S,string)
MGEF: INDX(Effect Index,S,4) MEDT(Data,S,36) ITEX(Icon) PTEX(Particle Texture) BSND(Bolt Sound) CSND(Casting Sound) HSND(Hit Sound) ASND(Area Sound) CVFX(Casting Visual) BVFX(Bolt Visual) HVFX(Hit Visual) AVFX(Area Visual) DESC(Description)  [NO NAME]
MISC: NAME MODL FNAM MCDT(Data,S,12) SCRI ITEX
NPC_: NAME MODL FNAM RNAM(Race,S) CNAM(Class,S) ANAM(Faction,S) BNAM(Head,S) KNAM(Hair,S) SCRI NPDT(Data,S,52|12) FLAG(Flags,S,4) NPCS(Spell,R) NPCO(Item,R,36) AIDT(AI Data,S,12) DODT(Travel Destination,R,24) DNAM(Destination Cell,R) AI_W AI_T AI_F AI_E AI_A CNDT(AI Cell,R,string)
PROB: NAME MODL FNAM PBDT(Data,S,16) SCRI ITEX
RACE: NAME FNAM RADT(Data,S,140) DESC NPCS(Power,R)
REGN: NAME FNAM WEAT(Weather,S,10|8) BNAM(Sleep List,S) CNAM(Map Color,S,4) SNAM(Sound,R,33)
REPA: NAME MODL FNAM RIDT(Data,S,16) SCRI ITEX
SCPT: SCHD(Header,S,52) SCVR(Variables,S,binary) SCDT(Bytecode,S,binary) SCTX(Source,S,string)  [NO NAME]
SKIL: INDX(Skill Index,S,4) SKDT(Data,S,24) DESC  [NO NAME]
SNDG: NAME DATA(Sound Type,S,4) CNAM(Creature,S) SNAM(Sound,S)
SOUN: NAME FNAM(Filename,S) DATA(Data,S,3)
SPEL: NAME FNAM SPDT(Data,S,12) ENAM(Effect,R,24)
SSCR: NAME DATA(Data,S,string)
STAT: NAME MODL
WEAP: NAME MODL FNAM WPDT(Data,S,32) SCRI ITEX ENAM(Enchantment,S,0)

## Dialogue

DIAL: NAME(ID,S) DATA(Type,S,1)
INFO: INAM(Info ID,S) PNAM(Previous,S) NNAM(Next,S) DATA(Data,S,12) ONAM(Actor,S) RNAM(Race,S) CNAM(Class,S) FNAM(Faction,S) ANAM(Cell,S) DNAM(PC Faction,S) SNAM(Sound,S) NAME(Response,S) SCVR(Condition,R,binary) INTV(Comparison Value,S,4) FLTV(Comparison Value,S,4) BNAM(Result Script,S) QSTN(Quest Name,S,1) QSTF(Quest Finished,S,1) QSTR(Quest Restart,S,1)

## World

CELL body: NAME(ID,S) DATA(Data,S,12) INTV(Water Level Int,S,4) WHGT(Water Height,S,4) AMBI(Ambient,S,16) RGNN(Region,S) NAM5(Map Color,S,4) NAM0(Ref Count,S,4)
CELL ref-object (repeatable groups): FRMR(Object Ref,R,4) NAME(Object ID,R,string) UNAM(Blocked,R,1) XSCL(Scale,R,4) ANAM(Owner,R,string) BNAM(Global Var,R,string) XSOL(Soul,R,string) CNAM(Faction,R,string) INDX(Faction Rank,R,4) XCHG(Enchant Charge,R,4-float) INTV(Uses,R,4) NAM9(Count,R,4) DODT(Door Destination,R,24) DNAM(Destination Cell,R,string) FLTV(Lock Level,R,4) KNAM(Key,R,string) TNAM(Trap,R,string) DATA(Position,R,24) MVRF(Moved Ref,R,4) CNDT(Moved Destination,R,8)
NOTE CELL collisions distinguished by size: DATA 12(body)/24(ref pos); NAME body(cell id)/ref(obj id); INTV 4(water/uses); CNDT 8(moved). Existing schema already has cell_data 12, cell_ref_data 24, cell_ambi 16, whgt 4, nam0 4, nam5 4, cell_fltv 4, cell_nam9 4, mvrf 4, cell_cndt_grid 8, rgnn string.

PGRD: NAME(Cell,S) DATA(Data,S,12) PGRP(Points,S,binary) PGRC(Connections,S,binary)
LAND: INTV(Grid,S,8) DATA(Flags,S,4) VNML(Normals,S,bin) VHGT(Heights,S,bin) WNAM(Map LOD,S,bin) VCLR(Colors,S,bin) VTEX(Textures,S,bin)
LEVI: NAME DATA(Flags,S,4) NNAM(Chance None,S,1) INDX(Count,S,4) INAM(Item,R,string) INTV(Level,R,2)
LEVC: NAME DATA(Flags,S,4) NNAM(Chance None,S,1) INDX(Count,S,4) CNAM(Creature,R,string) INTV(Level,R,2)

## TES3 header (record view? usually not shown; include for completeness)
TES3: HEDR(Header,S,300) MAST(Master File,R,string) DATA(Master Size,R,8) [GMDT/SCRD/SCRS save-only, skip]

## Notes for encoding
- All records also end with DELE (deletion marker) — yampt handles DELE specially (shows DELETED), not via schema. Do NOT add DELE rows.
- Shared string labels differ per parent (ANAM, CNAM, BNAM, SNAM, INTV, DATA, FNAM, NAME) -> per-parent label needed.
- Repeatable flag true only on the rows marked R above.
- MGEF/SKIL/SCPT have no NAME (keyed by INDX/SCHD).
