#include "dict_creator.hpp"

void dict_creator_t::make_dict_base_ordered()
{
	load_english_dict();

	build_npc_index();
	build_dial_map_ordered();

	std::string dial_type;
	std::string dial_foreign_name;

	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		esm_ref.select_record(i);
		const auto & rec_id = esm.get_record().id;

		if (rec_id == "GMST")
			process_gmst_ordered(i);
		else if (rec_id == "DIAL")
			process_dial_ordered(i, dial_type, dial_foreign_name);
		else if (rec_id == "INFO")
			process_info_ordered(i, dial_type, dial_foreign_name);
		else if (rec_id == "SCPT")
			process_sctx_ordered(i);
		else if (rec_id == "CELL")
			process_cell_ordered(i);
		else if (rec_id == "BOOK")
			process_text_ordered(i);
		else if (rec_id == "FACT")
			process_rnam_ordered(i);
		else if (rec_id == "BSGN" || rec_id == "CLAS" || rec_id == "RACE")
			process_desc_ordered(i);
		else if (rec_id == "SKIL" || rec_id == "MGEF")
			process_indx_ordered(i);
	}

	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		esm_ref.select_record(i);

		if (!tools_t::is_fnam(esm.get_record().id))
			continue;

		process_fnam_ordered(i);
	}

	process_cell_default_ordered();
	process_cell_region_ordered();
}

void dict_creator_t::build_dial_map_ordered()
{
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "DIAL")
			continue;

		esm.set_key("DATA");
		if (!esm.get_key().exist)
			continue;

		if (tools_t::get_dialog_type(esm.get_key().content) != "T")
			continue;

		esm.set_value("NAME");
		esm_ref.select_record(i);
		esm_ref.set_value("NAME");

		if (esm.get_value().exist && esm_ref.get_value().exist)
			dial_native_to_foreign[esm.get_value().text] = esm_ref.get_value().text;
	}
}

void dict_creator_t::process_gmst_ordered(size_t i)
{
	esm.set_key("NAME");
	esm.set_value("STRV");
	if (!esm.get_key().exist || !esm.get_value().exist)
		return;

	if (esm.get_key().text.substr(0, 1) != "s")
		return;

	const auto & key_text = esm.get_key().text;
	const auto & new_text = esm.get_value().text;

	esm_ref.set_key("NAME");
	esm_ref.set_value("STRV");
	if (!esm_ref.get_value().exist)
		return;

	const auto & old_text = esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::gmst, status_t::translated);
}

void dict_creator_t::process_fnam_ordered(size_t i)
{
	esm.set_key("NAME");
	esm.set_value("FNAM");
	if (!esm.get_key().exist || !esm.get_value().exist)
		return;

	if (esm.get_key().text == "player")
		return;

	if (esm.get_value().text.empty())
		return;

	const auto key_text = esm.get_record().id + "^" + esm.get_key().text;
	const auto & new_text = esm.get_value().text;

	esm_ref.set_key("NAME");
	esm_ref.set_value("FNAM");
	if (!esm_ref.get_value().exist)
		return;

	const auto & old_text = esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::fnam, status_t::translated);
}

void dict_creator_t::process_desc_ordered(size_t i)
{
	esm.set_key("NAME");
	esm.set_value("DESC");
	if (!esm.get_key().exist || !esm.get_value().exist)
		return;

	const auto key_text = esm.get_record().id + "^" + esm.get_key().text;
	const auto & new_text = esm.get_value().text;

	esm_ref.set_key("NAME");
	esm_ref.set_value("DESC");
	if (!esm_ref.get_value().exist)
		return;

	const auto & old_text = esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::desc, status_t::translated);
}

void dict_creator_t::process_text_ordered(size_t i)
{
	esm.set_key("NAME");
	esm.set_value("TEXT");
	if (!esm.get_key().exist || !esm.get_value().exist)
		return;

	const auto & key_text = esm.get_key().text;
	const auto & new_text = esm.get_value().text;

	esm_ref.set_key("NAME");
	esm_ref.set_value("TEXT");
	if (!esm_ref.get_value().exist)
		return;

	const auto & old_text = esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::text, status_t::translated);
}

void dict_creator_t::process_rnam_ordered(size_t i)
{
	esm.set_key("NAME");
	esm.set_value("RNAM");
	if (!esm.get_key().exist)
		return;

	esm_ref.set_key("NAME");
	esm_ref.set_value("RNAM");

	while (esm.get_value().exist)
	{
		const auto key_text = esm.get_key().text + "^" + std::to_string(esm.get_value().counter);
		const auto & new_text = esm.get_value().text;

		if (esm_ref.get_value().exist)
		{
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::rnam, status_t::translated);
			esm_ref.set_next_value("RNAM");
		}
		else
		{
			tools_t::add_log("[warning] RNAM count mismatch: \"" + esm.get_key().text + "\"\r\n");
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::rnam, status_t::mismatch);
		}

		esm.set_next_value("RNAM");
	}
}

void dict_creator_t::process_indx_ordered(size_t i)
{
	esm.set_key("INDX");
	esm.set_value("DESC");
	if (!esm.get_key().exist || !esm.get_value().exist)
		return;

	const auto key_text = esm.get_record().id + "^" + tools_t::get_indx(esm.get_key().content);
	const auto & new_text = esm.get_value().text;

	esm_ref.set_key("INDX");
	esm_ref.set_value("DESC");
	if (!esm_ref.get_value().exist)
		return;

	const auto & old_text = esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::indx, status_t::translated);
}

void dict_creator_t::process_cell_ordered(size_t i)
{
	esm.set_value("NAME");
	if (!esm.get_value().exist || esm.get_value().text.empty())
		return;

	const auto & new_text = esm.get_value().text;

	esm_ref.set_value("NAME");
	if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
		return;

	const auto & old_text = esm_ref.get_value().text;
	insert_entry_base(old_text, old_text, new_text, tools_t::rec_type_t::cell, status_t::translated);
}

void dict_creator_t::process_dial_ordered(size_t i, std::string & dial_type, std::string & dial_foreign_name)
{
	esm.set_key("DATA");
	if (!esm.get_key().exist)
		return;

	dial_type = tools_t::get_dialog_type(esm.get_key().content);

	esm.set_value("NAME");
	if (!esm.get_value().exist)
		return;

	const auto & native_name = esm.get_value().text;

	esm_ref.set_value("NAME");
	if (!esm_ref.get_value().exist)
		return;

	dial_foreign_name = esm_ref.get_value().text;

	if (dial_type != "T")
		return;

	insert_entry_base(
	    dial_foreign_name, dial_foreign_name, native_name, tools_t::rec_type_t::dial, status_t::translated);
}

void dict_creator_t::attach_speaker_metadata(const std::string & key_text, size_t record_index)
{
	esm.select_record(record_index);
	esm.set_value("ONAM");
	if (!esm.get_value().exist || esm.get_value().text.empty())
		return;

	const auto & speaker_id = esm.get_value().text;
	const auto npc_search = npc_index.find(speaker_id);
	if (npc_search == npc_index.end())
		return;

	esm_ref.select_record(npc_search->second);
	esm_ref.set_key("FNAM");
	esm_ref.set_value("FLAG");

	std::string speaker_name;
	if (esm_ref.get_key().exist)
		speaker_name = esm_ref.get_key().text;

	std::string gender;
	if (esm_ref.get_value().exist)
		gender = ((tools_t::convert_string_byte_array_to_uint(esm_ref.get_value().content) & 0x0001) != 0) ? "F" : "M";

	auto * entry = dict.at(tools_t::rec_type_t::info).find(key_text);
	if (!entry)
		return;

	entry->speaker_name = speaker_name;
	entry->gender = gender;
}

void dict_creator_t::process_info_ordered(
    size_t i,
    const std::string & dial_type,
    const std::string & dial_foreign_name)
{
	esm.set_key("INAM");
	if (!esm.get_key().exist)
		return;

	const auto inam = esm.get_key().text;

	esm.set_value("NAME");
	if (!esm.get_value().exist)
		return;

	const auto & new_text = esm.get_value().text;

	esm_ref.set_value("NAME");
	if (!esm_ref.get_value().exist)
		return;

	const auto & old_text = esm_ref.get_value().text;

	const auto key_text = dial_type + "^" + dial_foreign_name + "^" + inam;
	insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::info, status_t::translated);

	process_bnam_ordered(i, dial_type, dial_foreign_name, inam);
	attach_speaker_metadata(key_text, i);
}

void dict_creator_t::process_sctx_ordered(size_t i)
{
	esm.set_key("SCHD");
	esm.set_value("SCTX");
	if (!esm.get_key().exist || !esm.get_value().exist)
		return;

	const auto & script_name = esm.get_key().text;
	const auto native_messages = make_script_messages(esm.get_value().text);

	esm_ref.set_key("SCHD");
	esm_ref.set_value("SCTX");
	if (!esm_ref.get_value().exist)
	{
		tools_t::add_log("[warning] SCTX not found: \"" + script_name + "\"\r\n");
		for (const auto & msg : native_messages)
			insert_entry_base(script_name + "^" + msg, "", msg, tools_t::rec_type_t::sctx, status_t::mismatch);
		return;
	}

	const auto foreign_messages = make_script_messages(esm_ref.get_value().text);

	if (native_messages.size() != foreign_messages.size())
	{
		tools_t::add_log("[warning] SCTX line count mismatch: \"" + script_name + "\"\r\n");
		for (const auto & msg : foreign_messages)
			insert_entry_base(script_name + "^" + msg, msg, msg, tools_t::rec_type_t::sctx, status_t::mismatch);
		return;
	}

	for (size_t k = 0; k < native_messages.size(); ++k)
		insert_entry_base(
		    script_name + "^" + foreign_messages[k],
		    foreign_messages[k],
		    native_messages[k],
		    tools_t::rec_type_t::sctx,
		    status_t::translated);
}

void dict_creator_t::process_bnam_ordered(
    size_t i,
    const std::string & dial_type,
    const std::string & dial_foreign_name,
    const std::string & info_inam)
{
	esm.select_record(i);
	esm_ref.select_record(i);

	esm.set_value("BNAM");
	if (!esm.get_value().exist || esm.get_value().text.empty())
		return;

	const auto native_messages = make_script_messages(esm.get_value().text);

	esm_ref.set_value("BNAM");
	if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
	{
		const auto info_key = dial_type + "^" + dial_foreign_name + "^" + info_inam;
		tools_t::add_log("[warning] BNAM not found: \"" + info_key + "\"\r\n");
		for (const auto & msg : native_messages)
		{
			const auto key_text = dial_type + "^" + dial_foreign_name + "^" + info_inam + "^" + msg;
			insert_entry_base(key_text, "", msg, tools_t::rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	const auto foreign_messages = make_script_messages(esm_ref.get_value().text);

	if (native_messages.size() != foreign_messages.size())
	{
		const auto info_key = dial_type + "^" + dial_foreign_name + "^" + info_inam;
		tools_t::add_log("[warning] BNAM line count mismatch: \"" + info_key + "\"\r\n");
		for (const auto & msg : foreign_messages)
		{
			const auto key_text = dial_type + "^" + dial_foreign_name + "^" + info_inam + "^" + msg;
			insert_entry_base(key_text, msg, msg, tools_t::rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	for (size_t k = 0; k < native_messages.size(); ++k)
	{
		const auto key_text = dial_type + "^" + dial_foreign_name + "^" + info_inam + "^" + foreign_messages[k];
		const auto & old_text = foreign_messages[k];
		const auto & new_text = native_messages[k];
		insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::bnam, status_t::translated);
	}
}

void dict_creator_t::process_cell_default_ordered()
{
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "GMST")
			continue;

		esm.set_key("NAME");
		if (!esm.get_key().exist)
			continue;

		if (esm.get_key().text != "sDefaultCellname")
			continue;

		esm.set_value("STRV");
		if (!esm.get_value().exist)
			break;

		const auto & new_text = esm.get_value().text;

		esm_ref.select_record(i);
		esm_ref.set_key("NAME");
		esm_ref.set_value("STRV");
		if (!esm_ref.get_value().exist)
			break;

		const auto & old_text = esm_ref.get_value().text;
		insert_entry_base(old_text, old_text, new_text, tools_t::rec_type_t::cell, status_t::translated);
		break;
	}
}

void dict_creator_t::process_cell_region_ordered()
{
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "REGN")
			continue;

		esm.set_value("FNAM");
		if (!esm.get_value().exist || esm.get_value().text.empty())
			continue;

		const auto & new_text = esm.get_value().text;

		esm_ref.select_record(i);
		esm_ref.set_value("FNAM");
		if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
			continue;

		const auto & old_text = esm_ref.get_value().text;
		insert_entry_base(old_text, old_text, new_text, tools_t::rec_type_t::cell, status_t::translated);
	}
}
