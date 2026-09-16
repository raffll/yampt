# Removal From Master Is Intentional

When a plugin removes something that exists in the master, that removal is a deliberate authoring decision and must be honored by the merge. A removal always wins by default — the merge never resurrects master content that a plugin deleted.

## Rule

- If a sub-record, group, list entry, or field is present in the master but absent in a plugin version, treat the absence as an intentional delete, not as missing data to be filled back in from the master or another plugin.
- Do NOT re-add removed content during merge. A removal is a change like any other and follows normal last-changer-wins precedence: a plugin that removes an item is a plugin that changed it.
- This applies to every keyed/grouped merge: leveled-list entries, keyed lists (NPCO/NPCS/FACT), FRMR groups, and any element-wise merge. Union logic keeps items added by any plugin, but an item deleted by a plugin is dropped, not kept alive by the master.

## Consequence For FRMR / ANAM Conflicts

For the FRMR-group owner merge specifically: if an intermediate plugin removes the owner (ANAM present in master, absent in the intermediate) that is an intentional un-own and is respected — the group is emitted with the owner removed, not with the master's owner restored.

## Why

Modders remove references, owners, and list entries on purpose (cutting content, clearing ownership, deleting placed objects). Restoring what a plugin deleted silently reverts the modder's intent and reintroduces content they meant to be gone.
