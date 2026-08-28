# TODO

## Bugs

null dereference risk in preview_view_t::on_apply_clicked (m_edit_controller not checked)
file_path_parts_t::set_name crash on extensionless filenames (substr(npos))
esm_converter convert_mast crash on dot-less MAST entries (substr(npos))
esm_converter convert_bnam missing dial_found guard (garbage key on malformed plugins)
web_translator float values inserted as strings in JSON body (no toDouble path)
web_translator response_path parser silent failure on malformed bracket
web_translator form POST missing Content-Type header
propagation overwrites user intent status to propagated on source entry
script_parser token regex too narrow for non-ASCII chars (only \xD1, should be \x80-\xFF)

## GUI

No Filters should be a plain push button, not a checkable toggle
No Filters should not reset regex, case, id, name buttons — only clear filter state
View menu toggle items (Sidebar, Bottom Panel) should be at the top
Rename: Hide Duplicates -> Hide Duplicated Records
Rename: Mark Deleted -> Mark Deleted with Strikethrough
On INFO chain cell, dont show edit values in preview panel
sidebar_controller scan_workspace path comparison without case normalization

## Steering Drift

field_def_role case in view_tree_model.cpp is 103 lines
exclude sub-record lambda nesting > 3 levels in view_context_menu.cpp
highlight_coordinator_t is static-only class, should be namespace
insert_duplicate has unused status parameter (dead code)
esm_converter.hpp and esm_reader.hpp accessors missing const qualifier
spell_checker duplicated case-insensitive search (DRY violation)
script_parser trim_last_new_line_chars unreachable || npos condition

## Documentation

yEditor-Manual.md missing Toggle Sidebar and Toggle Bottom Panel in View Menu section
settings_store INI section WebTranslators vs steering doc WebProviders (update doc)
yaml_l10n_reader missing \n \t escape handling in quoted values
yaml_l10n_reader missing |+ block scalar support
check all [error] [info] log messages for correct lowercase tags
