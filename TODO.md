# TODO

faction rep merge
some float dialogue condition are hex
in 3rd column show spell effects
LTEX;INTV exluded by deaf
if only 2 plugins, text is black, should be purple and grey
in left panel show first non-empty/existing records, not exactly previous
filed name default color should be also grey?
mayve indicator that min/max are bounded
dont wrap log lines
remove locking entire records, it is always single field/group copied or modified, maybe lock them automatically for mnerged patch and keep source indicator, and remove when source is not in load order
adavanmced filters records list, show sanme manes as nav tree
special, show excluded, show locked only
show optional fields, in white backgroud, light grey text, can be edited and added
record flags follow, last plugin wins
maybe better subrecords sorting, single values one before multi
DELE - showing 0 or garbade inseatd of DELETED
check priority excluded vs lock, exclude shoulk take precedense
sound generator record without ID?
bonus skills not merged?
race merged patch is broken
removing from merged patch also should lock that group as empty
NPCO follow removing rule if items in later mod are identical to master
millions gold? 60 rank?
autocalc vs non autocalc, autocalc should still show all values, but with "Auto" or empty?
NPC have door destination???
record flags can be divided
autocalc vs non autocalc prevenmt merging other values also
we know npc faction so we can resolve rank: 1 (Blabla)
reapetable subrecords whould have #0 even if only one, non repeatable no
exclude subrecord should not recalculate conflicts, they are valid, we need only different colors
diff dont work on editable fields
figure out how to show valid ranges for fields
move validation text from status to left of apply/combo buttons
INDX - Count shoul be not editbale, but autocals always, so exclude kind of in grey?
leveled items pc level is not merged
calc for each item not merged
id should also be not editable? allow to create new record?
landscape should not be editable/merge patch at all
we can show entire hex in edit panel, with line numbers, fixed  colum,n and rows
INTV is not merged, so there is hardcoded rule
ingredient data can be sorted per effect
Global var can be float or string in different p;lugin, what that mean???
rnam rank name can be #0 #1
If factin reaction shoul dignore empty? like cell?
Some faction reactions are merged some not
enam effect with #0
creatiure npco not merged
also npcs should follow remove spell rule
remove idle group in creature
container also should follor remove item rule
CTDT not resolving conflicts
adding new item to list may be when show all fields can show one empty at the end of list???
creature flags base? what mean?
AI_F more meaningluf fiedl names and why is so big numbers there?
diff is broken, it should be per character
book and script need also syntax coloring
how to merge armor body part? ignore non existent? ninf cnam and bnasm?
should_not_exist.esp in C:\OMEN\Morrowind\yampt\x64\Release\$(SolutionDir)
remove status column
move esp esm guard merged to left of plugin name
all excluded plugins, records, subrecords in light grey backgroud, grey text
all locked records and values in light blue background and blue text
remove exclude and lock glyphs completely from both panels
locking is not working for all fields
merged patch dont need override indicator, plugin < override < merged patch < guard
excluded plugins should be on list
there should be list of locked records and subrecords
locked records and subrecords should be kept as sidecar file for merged patch with binary data that will be reapplied after merged patch
always allow to lock merged patch, even if not active


editor: make field editing active-plugin-only; remove obsolete per-plugin in-place editing. Editing a non-active plugin column writes back to that plugin's own file (mutable_plugin + replace_record + mark_plugin_dirty + save_all_dirty), which is a separate path from active/merge editing. Since all record changes should target the active plugin (same store as Copy-to-Active), drop the source-plugin branch: the plugin_idx != -1 path in field_edit_controller (commit_to_source, field_edited signal, read_record_content/mutable_plugin/replace_record for edits), the "Direct Editing" settings page, and the m_editing_enabled half of editable_column_set_t::is_editable (leaving the active/merge column always editable). Also re-gate "Remove Record from Plugin" which currently keys off is_editing_enabled().

newly created file, should be at the end, check modified time, and apply correctly