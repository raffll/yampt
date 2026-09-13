# TODO

view treen column 0, field name default color should be also grey <- should be identical to master
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



editor: make field editing active-plugin-only; remove obsolete per-plugin in-place editing. Editing a non-active plugin column writes back to that plugin's own file (mutable_plugin + replace_record + mark_plugin_dirty + save_all_dirty), which is a separate path from active/merge editing. Since all record changes should target the active plugin (same store as Copy-to-Active), drop the source-plugin branch: the plugin_idx != -1 path in field_edit_controller (commit_to_source, field_edited signal, read_record_content/mutable_plugin/replace_record for edits), the "Direct Editing" settings page, and the m_editing_enabled half of editable_column_set_t::is_editable (leaving the active/merge column always editable). Also re-gate "Remove Record from Plugin" which currently keys off is_editing_enabled().

newly created file, should be at the end, check modified time, and apply correctly


## Faction (FACT) merge

Root cause: FACT inter-faction reactions are stored as paired INTV (reaction value) + ANAM (reaction faction name) sub-records, but the merger matches sub-records positionally by (type, occurrence). It has no concept that INTV+ANAM form one reaction keyed by faction name. This is the common cause of the three reaction bugs below. Fix by adding a faction-specific keyed-union merge in sub_record_merge_t (parallel to merge_armor_parts / NPCO union), keying reactions on the ANAM faction name and emitting INTV+ANAM as a coupled pair. Header sub-records (NAME, FNAM, FADT, RNAM) keep the existing generic merge so FADT element-wise merge still applies.

- faction rep merge: reactions present and identical in several plugins are dropped from the merged patch when the winner/base lists reactions in a different order (positional match fails). Union all reactions across plugins by faction name so every reaction appears once.
- faction reaction absent-vs-conflict: a reaction present and identical in the plugins that define it but absent in one plugin is falsely flagged as a conflict. Absent reactions must be ignored, like CELL slots (skip_non_existent). Decide alongside the merge fix so conflict display and merge output agree.
- some faction reactions merged, some not: same positional-matching root cause; resolved by the keyed-union merge.
- reaction value conflict policy (same faction, different INTV across plugins): last-listed plugin wins, consistent with the merger's precedence. CONFIRM.
- reaction removal policy (reaction present in base, deleted in a higher-priority plugin): decide whether to respect the removal or union everything (armor/NPCO currently union). CONFIRM before implementing.

## Faction rank names

- rnam rank name can be #0 #1: FACT RNAM rank names repeat, so they must carry occurrence indices (#0, #1, ...) in both panels, matching the repeatable-subrecord numbering rule.
- resolve NPC rank to rank name: an NPC's rank is a number; using the NPC's faction (ANAM) we can look up the faction's RNAM list and display "1 (Rank Name)" instead of a bare number.
- NPC "door destination" misdecode: NPC ANAM (faction) appears to be decoded/labelled as a door destination field. Verify the NPC_ ANAM schema mapping is not colliding with the door ANAM/DNAM mapping.


## Deferred (need decision)

locked bit/field on a repeated sub-record (occurrence > 1) behind a flags group or cell-ref: coloring/menu shows it as not locked because occurrence is left at 0 on flags-group and cell-ref child rows (view_tree_decode.cpp / view_tree_decode_cell.cpp) and row_is_locked reads occurrence from the flags-group parent. reapply still writes it; only the match/display is wrong
excluded plugins should be on list: add an "Excluded Plugins" tab to the Merged Patch settings page (merge_settings_view) listing the session's excluded plugins. Excluded plugins are session state (plugin_session_t, merge/excluded_plugins), not settings_store_t, so the settings dialog must be given the session. DECIDE: read-only list vs a Remove button that re-includes a plugin (needs session write-back + nav refresh + session save)
there should be list of locked records and subrecords: a central list of all active merge locks (from plugin_scan_t::active_locks(), each merge_lock_t has rec_type/record_id/scope/sub_type). Mirror the excluded-plugins list placement (Merged Patch settings page tab). DECIDE with the excluded-plugins item: read-only vs a Remove button (remove_active_lock + reapply/refresh)
always allow to lock merged patch, even if not active: currently the lock menu gates on is_on_active && is_on_merged_patch (view_context_menu.cpp), and locks live in the ACTIVE plugin's store (plugin_scan_t::active_locks / m_active_store), reapplied to the active merge output. To lock the merged patch while another plugin is active, locks must address the merged patch independently of the active store (dedicated merged-patch lock store, always loaded from the sidecar), and the menu gate must allow is_on_merged_patch regardless of active. Contradicts the current design-decisions rule "locking only when merged patch is active" — that rule needs updating too. DECIDE: on lock while not active, re-apply to on-disk merged patch immediately vs only record in sidecar for next merge regeneration
in 3rd column show spell effects: predates nav-tree Status column removal (no 3rd column now). Likely means the record view — show a readable spell-effect summary on the collapsed ENAM group row (like the faction-reaction summary), e.g. "Restore Health, Self, 10pts, 30s". Raw ENAM fields already decode (Effect/Skill/Attribute/Range/Area/Duration/Mag). DECIDE exact presentation and where
