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

## Documentation

yaml_l10n_reader missing \n \t escape handling in quoted values
yaml_l10n_reader missing |+ block scalar support
check all [error] [info] log messages for correct lowercase tags
