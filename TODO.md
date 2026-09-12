# TODO

remove status column
move esp esm guard merged to left of plugin name
all excluded plugins, records, subrecords in grey backgroud black text
all locked records and values in blue background and dark blue text
remove exclude and lock glyphs completely from both panels
locking is not working
merged patch dont need override indicator
excluded plugins should be on list
there should be list of locked records and subrecords
locked records and subrecords should be kept as sidecar file for merged patch with binary data that will be reapplied after merged patch
always allow to lock merged patch, even if not active


editor: make field editing active-plugin-only; remove obsolete per-plugin in-place editing. Editing a non-active plugin column writes back to that plugin's own file (mutable_plugin + replace_record + mark_plugin_dirty + save_all_dirty), which is a separate path from active/merge editing. Since all record changes should target the active plugin (same store as Copy-to-Active), drop the source-plugin branch: the plugin_idx != -1 path in field_edit_controller (commit_to_source, field_edited signal, read_record_content/mutable_plugin/replace_record for edits), the "Direct Editing" settings page, and the m_editing_enabled half of editable_column_set_t::is_editable (leaving the active/merge column always editable). Also re-gate "Remove Record from Plugin" which currently keys off is_editing_enabled().

newly created file, should be at the end, check modified time, and apply correctly