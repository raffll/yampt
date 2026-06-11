# DIAL Matching — Findings & Design

## How DIAL Matching Works (make-base mode)

DIAL records hold dialogue topics. Each DIAL is followed by one or more INFO records (dialogue responses). The topic NAME is translatable — PL/DE/FR ESMs rename it (e.g. "kwama forager" → "zwiadowca kwama"). But the INFO chain underneath is structurally identical across languages.

The matching algorithm:
1. Build a fingerprint for each DIAL by reading sub-records from its first INFO (record at index i+1)
2. Build fingerprint maps for both ESMs
3. Look up foreign fingerprints in the native map — a match means same topic

## Fingerprint Composition

Current: `INAM` content only (first INFO's unique sequence ID).

Previously also included `SCVR` (condition variable string), but SCVR contains translatable text (cell names, faction names in conditions) which differs between languages — causing false mismatches.

## SCVR Problem (Fixed)

SCVR in INFO records contains condition strings like `"5sCell Balmora"`. These are language-specific — the PL ESM translates cell/faction names inside conditions. This caused 12+ topics to show as "missing" even though their INAM matched perfectly.

Fix: removed SCVR from the fingerprint. INAM alone is sufficient.

## "kwama" Bug (Root Cause Unknown)

"kwama" exists in both EN and PL with the same name and same first-INFO INAM. The pattern should match but was reported missing. Possible causes:
- Raw binary INAM bytes differ despite tes3conv showing the same decimal representation
- SCVR residue from a previous `set_value` call polluting the pattern
- Off-by-one in record indexing

After removing SCVR from the pattern, this should be resolved. If it persists, add diagnostic logging to dump raw pattern bytes.

## Uniqueness Analysis (Morrowind.esm)

| Fingerprint | Topics | Duplicates |
|-------------|-------:|-----------:|
| INAM only | 1668 | 30 |
| INAM + NNAM | 1670 | 28 |
| INAM + PNAM + NNAM | 1670 | 28 |

Adding PNAM/NNAM doesn't help — the 28 duplicates are single-INFO topics with identical empty prev/next chains.

## The 28 Irreducible Duplicates

These are structurally identical topics — one INFO each, same INAM bytes, same PNAM/NNAM. No first-INFO fingerprint variant can distinguish them:

- **Sanguine items** (20): All Sanguine quest reward topics share the same generic response INFO template. E.g. "Sanguine Balanced Armor" vs "Sanguine Leaping" vs "Sanguine Stolid Armor".
- **Quest topics** (4): "join the Mages Guild" vs "join House Hlaalu", "1000-drake pledge" vs "generous pledge", "pligrimage" vs "pilgrimage", "Idroso Vendu" vs "Ethal Seloth".
- **NPC names** (4): "Manilian Scerius" vs "Jadier Mannick" vs "Ciralinde" vs "Menelras" vs "S'Bakha" vs "Davina" — all share one generic NPC description INFO.

Tribunal: 0 duplicates. Bloodmoon: 1 duplicate.

## Behavior for Duplicates

The `patterns` map uses first-insertion-wins (`std::map::insert`). When multiple native topics produce the same fingerprint:
- Only the first native topic enters the map
- All foreign topics with that fingerprint match to the same (first) native entry
- No "missing" warning is generated — but duplicate topics get silently mis-paired

This is acceptable for vanilla (28 out of 1698 = 1.6%, all are obscure topics). The alternative — using all INAMs concatenated — would eliminate duplicates but significantly complicate the fingerprint logic and increase memory usage.

## key_text for DIAL

DIAL entries use a 16-char FNV-1a hash of the fingerprint (INAM bytes) as `key_text`. Same hashing approach as CELL records. The `old_text` field holds the actual topic name for display and `find_by_old_text()` lookups.
