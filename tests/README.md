# Merge Test Fixtures

Synthetic TES3 plugins for exercising the yampt merger. Do not hand-edit;
regenerate with the scripts below. Each fixture set lives in its own subfolder.

| Subfolder | Generator | Purpose |
|-----------|-----------|---------|
| `all_cases/` | `scripts/generate_merge_fixtures.py` | Broad coverage of every merge path, record type, and edge case |
| `armo_field_merge/` | `scripts/generate_armo_field_merge_fixture.py` | ARMO element-wise AODT field merge; highest-priority intermediate wins a reverted field |
| `armo_cnam/` | `scripts/generate_armo_cnam_fixture.py` | ARMO body-part CNAM added only by an intermediate lands in its INDX group |

Regenerate all:

```
python scripts/generate_merge_fixtures.py
python scripts/generate_armo_field_merge_fixture.py
python scripts/generate_armo_cnam_fixture.py
```

## all_cases — Files and Load Order

Load in this order so the merger sees master → esp1 → esp2, making **esp2 the
winner** (`versions.back`) and the master the base (`versions.front`):

1. `all_cases/merge_master.esm`
2. `all_cases/merge_esp1.esp`
3. `all_cases/merge_esp2.esp`

Generic three-way merges require all three versions to be present, so most
records appear in all three files.

### What Each Record Exercises

| Record | ID | Merge path exercised |
|--------|----|----|
| LTEX | `AI_Grass_Dirt` | Generic three-way with a conflicting `INTV`. Set the `LTEX:INTV` exclude rule to verify the sub-record is stripped and the record is pruned. |
| WEAP | `iron_dagger` | Element-wise `WPDT` field merge — esp1 changes Value, esp2 changes Speed; both survive. |
| NPC_ | `hlaalu_guard` | Element-wise 52-byte `NPDT` — esp1 changes Strength, esp2 changes Health; both survive. |
| NPC_ | `mismatch_npc` | `NPDT` size mismatch — esp1 switches to a 12-byte NPDT, so that intermediate is skipped. |
| NPC_ | `merchant` | `NPCO` inventory union by 32-byte item id — esp1 adds `healing_potion`, esp2 adds `silver_ring`. |
| ENCH | `ench_fire` | `ENAM` 24-byte slot merge with magnitude min/max pairing — esp1 raises the min, esp2 leaves master values. |
| CREA | `scamp_summon` | Summon fixer — id is a known summon and the persistent bit is clear, so it is set. |
| CELL | `Fog Test Interior` | Fog fixer — interior cell with an `AMBI` fog density of 0, corrected to 0.01. |
| CELL | `Old Region Name` | Cell name fixer — exterior cell, esp1 renames it, esp2 reverts, so the rename is restored. |
| CELL | `Ref Test Interior` | FRMR reference-group merge — esp1 adds group index 3 as an intermediate addition. |
| DIAL / INFO | `Background` | Dialogue union — esp1 edits `info_bg_1` and adds `info_bg_2`, esp2 adds `info_bg_3`. |
| LEVI | `list_creatures` | Leveled-list union with deletion — esp1 drops `cliff_racer` and adds `kwama_forager`, esp2 adds `guar`. |
| GMST | `sMasterText` | Three-way string — esp1 edits, esp2 leaves master, so esp1's edit wins. |
| GMST | `iMasterInt` | Three-way int — esp2 changes the value and wins. |
| MISC | `master_only_item` | Present in only one plugin — never merged. |
| MISC | `esp1_only_item` / `esp2_only_item` | Added by a single esp — never merged. |
| MISC | `identical_item` | Byte-identical across all three — pruned as a no-op. |
| BOOK | `blocked_book` | Blocked record flag (`0x2000`) preserved through the merge. |

## armo_field_merge — Files and Load Order

Load `armo_field_merge/armo_master.esm` then `armo_esp1.esp` … `armo_esp5.esp`
(esp5 is the winner). One ARMO record `adamantium_helm` whose AODT fields are
changed by intermediates and reverted by the winner. Expected merged AODT:
Value 9500, Health 575, Enchant Points 400, Armor Rating 50 — the highest-priority
intermediate change wins each field.

## armo_cnam — Files and Load Order

Load `armo_cnam/armo_cnam_master.esm` then `armo_cnam_esp1.esp` (adds the CNAM)
then `armo_cnam_esp2.esp` (winner, no CNAM). The intermediate-only female
body-part name must land in its Cuirass body-part slot in the merged patch.

## Notes

- These are minimal, hand-crafted binary records — enough to trigger each code
  path, not full vanilla records.
- The output directories are git-ignored artifact territory; regenerate as needed.
