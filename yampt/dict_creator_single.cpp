#include "dict_creator.hpp"

void dict_creator_t::make_dict_single()
{
	build_npc_index();
	build_text_match_index();

	make_dict_single_gmst();
	make_dict_single_fnam();
	make_dict_single_desc();
	make_dict_single_text();
	make_dict_single_rnam();
	make_dict_single_indx();
	make_dict_single_info();
	make_dict_single_sctx();
	make_dict_single_bnam();
	make_dict_single_dial();
	make_dict_single_cell();
}

void dict_creator_t::make_dict_single_gmst()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "GMST")
			continue;

		esm.set_key("NAME");
		esm.set_value("STRV");
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		if (esm.get_key().text.substr(0, 1) != "s")
			continue;

		const auto & key_text = esm.get_key().text;
		const auto & text = esm.get_value().text;

		if (mode == mode_t::single_with_base)
			insert_entry_single_with_base(key_text, text, text, tools_t::rec_type_t::gmst);
		else
			insert_entry_single(key_text, text, text, tools_t::rec_type_t::gmst);
	}
	print_log_line(tools_t::rec_type_t::gmst);
}

void dict_creator_t::make_dict_single_fnam()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (!tools_t::is_fnam(esm.get_record().id))
			continue;

		esm.set_key("NAME");
		esm.set_value("FNAM");
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		if (esm.get_key().text == "player")
			continue;

		if (esm.get_value().text.empty())
			continue;

		const auto key_text = esm.get_record().id + "^" + esm.get_key().text;
		const auto & text = esm.get_value().text;

		if (mode == mode_t::single_with_base)
			insert_entry_single_with_base(key_text, text, text, tools_t::rec_type_t::fnam);
		else
			insert_entry_single(key_text, text, text, tools_t::rec_type_t::fnam);
	}
	print_log_line(tools_t::rec_type_t::fnam);
}

void dict_creator_t::make_dict_single_desc()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		const auto & rec_id = esm.get_record().id;
		if (rec_id != "BSGN" && rec_id != "CLAS" && rec_id != "RACE")
			continue;

		esm.set_key("NAME");
		esm.set_value("DESC");
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		const auto key_text = rec_id + "^" + esm.get_key().text;
		const auto & text = esm.get_value().text;

		if (mode == mode_t::single_with_base)
			insert_entry_single_with_base(key_text, text, text, tools_t::rec_type_t::desc);
		else
			insert_entry_single(key_text, text, text, tools_t::rec_type_t::desc);
	}
	print_log_line(tools_t::rec_type_t::desc);
}

void dict_creator_t::make_dict_single_text()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "BOOK")
			continue;

		esm.set_key("NAME");
		esm.set_value("TEXT");
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		const auto & key_text = esm.get_key().text;
		const auto & text = esm.get_value().text;

		if (mode == mode_t::single_with_base)
			insert_entry_single_with_base(key_text, text, text, tools_t::rec_type_t::text);
		else
			insert_entry_single(key_text, text, text, tools_t::rec_type_t::text);
	}
	print_log_line(tools_t::rec_type_t::text);
}

void dict_creator_t::make_dict_single_rnam()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "FACT")
			continue;

		esm.set_key("NAME");
		esm.set_value("RNAM");
		if (!esm.get_key().exist)
			continue;

		while (esm.get_value().exist)
		{
			const auto key_text = esm.get_key().text + "^" + std::to_string(esm.get_value().counter);
			const auto & text = esm.get_value().text;

			if (mode == mode_t::single_with_base)
				insert_entry_single_with_base(key_text, text, text, tools_t::rec_type_t::rnam);
			else
				insert_entry_single(key_text, text, text, tools_t::rec_type_t::rnam);

			esm.set_next_value("RNAM");
		}
	}
	print_log_line(tools_t::rec_type_t::rnam);
}

void dict_creator_t::make_dict_single_indx()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		const auto & rec_id = esm.get_record().id;
		if (rec_id != "SKIL" && rec_id != "MGEF")
			continue;

		esm.set_key("INDX");
		esm.set_value("DESC");
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		const auto key_text = rec_id + "^" + tools_t::get_indx(esm.get_key().content);
		const auto & text = esm.get_value().text;

		if (mode == mode_t::single_with_base)
			insert_entry_single_with_base(key_text, text, text, tools_t::rec_type_t::indx);
		else
			insert_entry_single(key_text, text, text, tools_t::rec_type_t::indx);
	}
	print_log_line(tools_t::rec_type_t::indx);
}

void dict_creator_t::make_dict_single_info()
{
	std::string key_prefix;
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id == "DIAL")
		{
			esm.set_key("DATA");
			esm.set_value("NAME");
			if (esm.get_key().exist && esm.get_value().exist)
				key_prefix = tools_t::get_dialog_type(esm.get_key().content) + "^" + esm.get_value().text;

			continue;
		}

		if (esm.get_record().id != "INFO")
			continue;

		esm.set_key("INAM");
		if (!esm.get_key().exist)
			continue;

		esm.set_value("NAME");
		if (!esm.get_value().exist)
			continue;

		const auto key_text = key_prefix + "^" + esm.get_key().text;
		const auto & text = esm.get_value().text;

		if (mode == mode_t::single_with_base)
			insert_entry_single_with_base(key_text, text, text, tools_t::rec_type_t::info);
		else
			insert_entry_single(key_text, text, text, tools_t::rec_type_t::info);

		esm.select_record(i);
		esm.set_value("ONAM");
		if (!esm.get_value().exist || esm.get_value().text.empty())
			continue;

		const auto & speaker_id = esm.get_value().text;
		auto npc_search = npc_index.find(speaker_id);
		if (npc_search == npc_index.end())
			continue;

		esm.select_record(npc_search->second);
		esm.set_key("FNAM");
		esm.set_value("FLAG");

		std::string speaker_name;
		if (esm.get_key().exist)
			speaker_name = esm.get_key().text;

		std::string gender;
		if (esm.get_value().exist)
			gender = ((tools_t::convert_string_byte_array_to_uint(esm.get_value().content) & 0x0001) != 0) ? "F" : "M";

		auto * entry = dict.at(tools_t::rec_type_t::info).find(key_text);
		if (!entry)
			continue;

		entry->speaker_name = speaker_name;
		entry->gender = gender;
	}
	print_log_line(tools_t::rec_type_t::info);
}

void dict_creator_t::make_dict_single_sctx()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "SCPT")
			continue;

		esm.set_key("SCHD");
		esm.set_value("SCTX");
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		const auto & script_name = esm.get_key().text;
		const auto & messages = make_script_messages(esm.get_value().text);

		for (size_t k = 0; k < messages.size(); ++k)
		{
			const auto key_text = script_name + "^" + messages[k];
			const auto & old_text = messages[k];

			if (mode == mode_t::single_with_base)
				insert_entry_single_with_base(key_text, old_text, old_text, tools_t::rec_type_t::sctx);
			else
				insert_entry_single(key_text, old_text, old_text, tools_t::rec_type_t::sctx);
		}
	}
	print_log_line(tools_t::rec_type_t::sctx);
}

void dict_creator_t::make_dict_single_bnam()
{
	std::string dial_type;
	std::string dial_name;
	std::string info_inam;
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		const auto & rec_id = esm.get_record().id;

		if (rec_id == "DIAL")
		{
			esm.set_key("DATA");
			esm.set_value("NAME");
			if (esm.get_key().exist && esm.get_value().exist)
			{
				dial_type = tools_t::get_dialog_type(esm.get_key().content);
				dial_name = esm.get_value().text;
			}
			continue;
		}

		if (rec_id != "INFO")
			continue;

		esm.set_key("INAM");
		if (!esm.get_key().exist)
			continue;

		info_inam = esm.get_key().text;

		esm.set_value("BNAM");
		if (!esm.get_value().exist || esm.get_value().text.empty())
			continue;

		const auto & messages = make_script_messages(esm.get_value().text);
		for (size_t k = 0; k < messages.size(); ++k)
		{
			const auto key_text = dial_type + "^" + dial_name + "^" + info_inam + "^" + messages[k];
			const auto & old_text = messages[k];

			if (mode == mode_t::single_with_base)
				insert_entry_single_with_base(key_text, old_text, old_text, tools_t::rec_type_t::bnam);
			else
				insert_entry_single(key_text, old_text, old_text, tools_t::rec_type_t::bnam);
		}
	}
	print_log_line(tools_t::rec_type_t::bnam);
}

void dict_creator_t::make_dict_single_dial()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "DIAL")
			continue;

		esm.set_key("DATA");
		esm.set_value("NAME");
		if (tools_t::get_dialog_type(esm.get_key().content) != "T")
			continue;

		if (!esm.get_value().exist)
			continue;

		const auto & text = esm.get_value().text;

		if (mode == mode_t::single_with_base)
			insert_entry_single_with_base(text, text, text, tools_t::rec_type_t::dial);
		else
			insert_entry_single(text, text, text, tools_t::rec_type_t::dial);
	}
	print_log_line(tools_t::rec_type_t::dial);
}

void dict_creator_t::make_dict_single_cell()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "CELL")
			continue;

		esm.set_value("NAME");
		if (!esm.get_value().exist || esm.get_value().text.empty())
			continue;

		const auto & text = esm.get_value().text;

		if (mode == mode_t::single_with_base)
			insert_entry_single_with_base(text, text, text, tools_t::rec_type_t::cell);
		else
			insert_entry_single(text, text, text, tools_t::rec_type_t::cell);
	}
	print_log_line(tools_t::rec_type_t::cell);
}

void dict_creator_t::build_text_match_index()
{
	text_match_index_.clear();

	if (!base_dict)
		return;

	for (const auto & [type, chapter] : *base_dict)
	{
		for (const auto & entry : chapter.records)
		{
			if (entry.old_text.empty())
				continue;

			text_match_index_[entry.old_text].push_back(&entry);
		}
	}
}

void dict_creator_t::insert_entry_single(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    tools_t::rec_type_t type)
{
	counter_all++;

	const bool is_text_keyed =
	    (type == tools_t::rec_type_t::cell || type == tools_t::rec_type_t::dial || type == tools_t::rec_type_t::sctx ||
	     type == tools_t::rec_type_t::bnam);

	auto * existing = dict.at(type).find(key_text);
	if (existing && is_text_keyed)
	{
		if (existing->old_text == old_text && existing->new_text == new_text)
			return;
	}

	tools_t::record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = tools_t::status_t::untranslated;

	if (dict.at(type).insert(entry))
	{
		counter_created++;
		return;
	}

	insert_duplicate(key_text, old_text, new_text, type, tools_t::status_t::untranslated);
}

void dict_creator_t::insert_entry_single_with_base(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    tools_t::rec_type_t type)
{
	counter_all++;

	const bool is_text_keyed =
	    (type == tools_t::rec_type_t::cell || type == tools_t::rec_type_t::dial || type == tools_t::rec_type_t::sctx ||
	     type == tools_t::rec_type_t::bnam);

	auto * existing = dict.at(type).find(key_text);
	if (existing && is_text_keyed)
	{
		if (existing->old_text == old_text && existing->new_text == new_text)
			return;
	}

	auto it = base_dict->find(type);
	if (it == base_dict->end())
	{
		insert_as_untranslated(key_text, old_text, type);
		return;
	}

	const auto * base_entry = it->second.find(key_text);

	if (!base_entry)
	{
		insert_via_text_match(key_text, old_text, type);
		return;
	}

	if (base_entry->status == tools_t::status_t::error)
	{
		insert_with_status(key_text, old_text, base_entry->new_text, type, tools_t::status_t::error);
		return;
	}

	if (base_entry->old_text == old_text && base_entry->new_text == old_text)
	{
		insert_with_status(key_text, old_text, old_text, type, tools_t::status_t::identical);
		return;
	}

	if (base_entry->old_text == old_text)
	{
		insert_with_status(key_text, old_text, base_entry->new_text, type, tools_t::status_t::translated);
		return;
	}

	if (differs_only_in_numbers_or_punct(old_text, base_entry->old_text))
	{
		const auto & adapted = adapt_translation(old_text, base_entry->old_text, base_entry->new_text);
		insert_with_status(key_text, old_text, adapted, type, tools_t::status_t::adapted);
		return;
	}

	insert_with_status(key_text, old_text, base_entry->new_text, type, tools_t::status_t::changed);
}

void dict_creator_t::insert_as_untranslated(
    const std::string & key_text,
    const std::string & old_text,
    tools_t::rec_type_t type)
{
	tools_t::record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = old_text;
	entry.status = tools_t::status_t::untranslated;

	if (dict.at(type).insert(entry))
	{
		counter_created++;
		return;
	}

	insert_duplicate(key_text, old_text, old_text, type, tools_t::status_t::untranslated);
}

void dict_creator_t::insert_with_status(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    tools_t::rec_type_t type,
    const char * status)
{
	tools_t::record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = status;

	if (dict.at(type).insert(entry))
	{
		counter_created++;
		return;
	}

	insert_duplicate(key_text, old_text, new_text, type, status);
}

void dict_creator_t::insert_via_text_match(
    const std::string & key_text,
    const std::string & old_text,
    tools_t::rec_type_t type)
{
	auto text_it = text_match_index_.find(old_text);
	if (text_it != text_match_index_.end())
	{
		const tools_t::record_entry_t * translated_match = nullptr;
		int match_count = 0;

		for (const auto * candidate : text_it->second)
		{
			if (candidate->new_text == candidate->old_text)
				continue;

			translated_match = candidate;
			++match_count;
			if (match_count > 1)
				break;
		}

		if (match_count == 1 && translated_match)
		{
			insert_with_status(key_text, old_text, translated_match->new_text, type, tools_t::status_t::reused);
			return;
		}
	}

	insert_as_untranslated(key_text, old_text, type);
}
