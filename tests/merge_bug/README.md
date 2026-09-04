# Merge byte-splice reproduction (WEAP/WPDT Health)

These three synthetic plugins reproduce the three-way merge byte-splice bug: the
element-wise merge (`sub_record_merge_t::merge_bytes_three_way`) merges PER BYTE,
so when two mods change different bytes of the same multi-byte numeric field, the
merge splices them into a value neither mod set.

## Files

- `Base.esm` — master file (TES3 header master flag set). One weapon `repro_sword`, WPDT `Health` (u16 at offset 10) = **256** (bytes `00 01`).
- `ModA_lowbyte.esp` — masters `Base.esm`. Same weapon, `Health` = **511** (`0x01FF`, bytes `ff 01`) — only the **low** byte changed vs base.
- `ModB_highbyte.esp` — masters `Base.esm`. Same weapon, `Health` = **25600** (`0x6400`, bytes `00 64`) — only the **high** byte changed vs base.

## Reproduce in yEditor

1. Load all three plugins (`Base.esm`, `ModA_lowbyte.esp`, `ModB_highbyte.esp`) in load order.
2. Create the merged patch (auto-merge). The weapon has a true 3-way conflict (base + two overrides), so WPDT is element-wise merged.
3. Inspect `repro_sword`'s `Health` in the merged patch.

- **Expected (correct) result:** `Health = 25600` — the winning plugin (ModB) should win the whole field.
- **Buggy result:** `Health = 0x64FF = 25855` — the low byte `FF` comes from ModA and the high byte `64` from ModB, producing a value neither mod set, sitting between base and ModB. This is the "value that wasn't added by any mod, between two values" artifact.

## Fix direction

Element-wise merge must operate at FIELD granularity (per the WPDT/NPDT/etc.
sub-record schema field boundaries), taking each multi-byte field wholly from one
side, never byte-spliced. See the corresponding TODO entry and the unit test
`tests.merge_byte_splice.cpp`.
