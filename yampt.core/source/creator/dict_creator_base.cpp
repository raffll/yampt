#include "../translator/translation_engine.hpp"
#include "cell_matcher.hpp"
#include "dial_matcher.hpp"
#include "dict_creator.hpp"
#include "word_match_utils.hpp"
#include <hunspell/hunspell.hxx>

void dict_creator_t::make_dict_base()
{
	load_english_dict();

	build_gmst_index();
	build_fnam_index();
	build_desc_index();
	build_text_index();
	build_rnam_index();
	build_indx_index();
	build_npc_index();
	build_info_index();

	make_dict_base_gmst();
	make_dict_base_fnam();
	make_dict_base_desc();
	make_dict_base_text();
	make_dict_base_rnam();
	make_dict_base_indx();
	make_dict_base_dial();
	make_dict_base_info();
	make_dict_base_sctx();
	make_dict_base_bnam();
	make_dict_base_cell();
}

void dict_creator_t::make_dict_base_gmst()
{
	reset_counters();
	std::set<std::string> matched_keys;

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
		const auto & new_text = esm.get_value().text;

		auto search = gmst_index.find(key_text);
		if (search != gmst_index.end())
		{
			matched_keys.insert(key_text);
			esm_ref.select_record(search->second);
			esm_ref.set_key("NAME");
			esm_ref.set_value("STRV");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::gmst, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::gmst, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : gmst_index)
	{
		if (matched_keys.count(key))
			continue;

		esm_ref.select_record(rec_idx);
		esm_ref.set_value("STRV");
		if (!esm_ref.get_value().exist)
			continue;

		const auto & old_text = esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, tools_t::rec_type_t::gmst, status_t::missing);
	}
}

void dict_creator_t::make_dict_base_fnam()
{
	reset_counters();
	std::set<std::string> matched_keys;

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
		const auto & new_text = esm.get_value().text;

		auto search = fnam_index.find(key_text);
		if (search != fnam_index.end())
		{
			matched_keys.insert(key_text);
			esm_ref.select_record(search->second);
			esm_ref.set_key("NAME");
			esm_ref.set_value("FNAM");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::fnam, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::fnam, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : fnam_index)
	{
		if (matched_keys.count(key))
			continue;

		esm_ref.select_record(rec_idx);
		esm_ref.set_value("FNAM");
		if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
			continue;

		const auto & old_text = esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, tools_t::rec_type_t::fnam, status_t::missing);
	}
}

void dict_creator_t::make_dict_base_desc()
{
	reset_counters();
	std::set<std::string> matched_keys;

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
		const auto & new_text = esm.get_value().text;

		auto search = desc_index.find(key_text);
		if (search != desc_index.end())
		{
			matched_keys.insert(key_text);
			esm_ref.select_record(search->second);
			esm_ref.set_key("NAME");
			esm_ref.set_value("DESC");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::desc, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::desc, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : desc_index)
	{
		if (matched_keys.count(key))
			continue;

		esm_ref.select_record(rec_idx);
		esm_ref.set_value("DESC");
		if (!esm_ref.get_value().exist)
			continue;

		const auto & old_text = esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, tools_t::rec_type_t::desc, status_t::missing);
	}
}

void dict_creator_t::make_dict_base_text()
{
	reset_counters();
	std::set<std::string> matched_keys;

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
		const auto & new_text = esm.get_value().text;

		auto search = text_index.find(key_text);
		if (search != text_index.end())
		{
			matched_keys.insert(key_text);
			esm_ref.select_record(search->second);
			esm_ref.set_key("NAME");
			esm_ref.set_value("TEXT");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::text, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::text, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : text_index)
	{
		if (matched_keys.count(key))
			continue;

		esm_ref.select_record(rec_idx);
		esm_ref.set_value("TEXT");
		if (!esm_ref.get_value().exist)
			continue;

		const auto & old_text = esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, tools_t::rec_type_t::text, status_t::missing);
	}
}

void dict_creator_t::make_dict_base_rnam()
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
			const auto & new_text = esm.get_value().text;

			auto search = rnam_index.find(key_text);
			if (search != rnam_index.end())
			{
				const auto & old_text = search->second;
				insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::rnam, status_t::translated);
			}
			else
			{
				insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::rnam, status_t::mismatch);
			}

			esm.set_next_value("RNAM");
		}
	}
}

void dict_creator_t::make_dict_base_indx()
{
	reset_counters();
	std::set<std::string> matched_keys;

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
		const auto & new_text = esm.get_value().text;

		auto search = indx_index.find(key_text);
		if (search != indx_index.end())
		{
			matched_keys.insert(key_text);
			esm_ref.select_record(search->second);
			esm_ref.set_key("INDX");
			esm_ref.set_value("DESC");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::indx, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::indx, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : indx_index)
	{
		if (matched_keys.count(key))
			continue;

		esm_ref.select_record(rec_idx);
		esm_ref.set_value("DESC");
		if (!esm_ref.get_value().exist)
			continue;

		const auto & old_text = esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, tools_t::rec_type_t::indx, status_t::missing);
	}
}

void dict_creator_t::make_dict_base_info()
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
			{
				const auto & native_name = esm.get_value().text;
				const auto dial_type = tools_t::get_dialog_type(esm.get_key().content);
				auto it_map = dial_native_to_foreign.find(native_name);
				if (it_map != dial_native_to_foreign.end())
					key_prefix = dial_type + "^" + it_map->second;
				else
					key_prefix = dial_type + "^" + native_name;
			}

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
		const auto & new_text = esm.get_value().text;

		auto search = info_index.find(key_text);
		if (search != info_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_key("INAM");
			esm_ref.set_value("NAME");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::info, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::info, status_t::mismatch);
		}

		esm.select_record(i);
		esm.set_value("ONAM");
		if (!esm.get_value().exist || esm.get_value().text.empty())
			continue;

		const auto & speaker_id = esm.get_value().text;
		auto it_npc = npc_index.find(speaker_id);
		if (it_npc == npc_index.end())
			continue;

		esm_ref.select_record(it_npc->second);
		esm_ref.set_key("FNAM");
		esm_ref.set_value("FLAG");

		std::string speaker_name;
		if (esm_ref.get_key().exist)
			speaker_name = esm_ref.get_key().text;

		std::string gender;
		if (esm_ref.get_value().exist)
			gender =
			    ((tools_t::convert_string_byte_array_to_uint(esm_ref.get_value().content) & 0x0001) != 0) ? "F" : "M";

		auto * entry = dict.at(tools_t::rec_type_t::info).find(key_text);
		if (!entry)
			continue;

		entry->speaker_name = speaker_name;
		entry->gender = gender;
	}
}

void dict_creator_t::build_sctx_schd_index(std::unordered_map<std::string, size_t> & schd_index)
{
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "SCPT")
			continue;

		esm_ref.set_key("SCHD");
		if (!esm_ref.get_key().exist)
			continue;

		schd_index.insert({ esm_ref.get_key().text, i });
	}
}

void dict_creator_t::match_sctx_messages(
    const std::string & script_name,
    const std::vector<std::string> & native_messages,
    const std::unordered_map<std::string, size_t> & schd_index)
{
	auto search = schd_index.find(script_name);
	if (search == schd_index.end())
	{
		tools_t::add_log("[warning] SCTX not found: \"" + script_name + "\"\r\n");
		for (const auto & msg : native_messages)
		{
			const auto key_text = script_name + "^" + msg;
			insert_entry_base(key_text, "", msg, tools_t::rec_type_t::sctx, status_t::mismatch);
		}
		return;
	}

	esm_ref.select_record(search->second);
	esm_ref.set_key("SCHD");
	esm_ref.set_value("SCTX");
	if (!esm_ref.get_value().exist)
	{
		tools_t::add_log("[warning] SCTX not found: \"" + script_name + "\"\r\n");
		for (const auto & msg : native_messages)
		{
			const auto key_text = script_name + "^" + msg;
			insert_entry_base(key_text, "", msg, tools_t::rec_type_t::sctx, status_t::mismatch);
		}
		return;
	}

	const auto foreign_messages = make_script_messages(esm_ref.get_value().text);

	if (native_messages.size() != foreign_messages.size())
	{
		tools_t::add_log(
		    "[warning] SCTX line count mismatch: \"" + script_name + "\" (native=" +
		    std::to_string(native_messages.size()) + ", foreign=" + std::to_string(foreign_messages.size()) + ")\r\n");

		for (const auto & msg : foreign_messages)
		{
			const auto key_text = script_name + "^" + msg;
			insert_entry_base(key_text, msg, msg, tools_t::rec_type_t::sctx, status_t::mismatch);
		}
		return;
	}

	for (size_t k = 0; k < native_messages.size(); ++k)
	{
		const auto key_text = script_name + "^" + foreign_messages[k];
		const auto & old_text = foreign_messages[k];
		const auto & new_text = native_messages[k];
		insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::sctx, status_t::translated);
	}
}

void dict_creator_t::make_dict_base_sctx()
{
	reset_counters();

	std::unordered_map<std::string, size_t> schd_index;
	build_sctx_schd_index(schd_index);

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
		const auto native_messages = make_script_messages(esm.get_value().text);
		match_sctx_messages(script_name, native_messages, schd_index);
	}
}

void dict_creator_t::match_bnam_native_infos(
    const std::string & info_key,
    const std::vector<std::string> & native_messages)
{
	auto search = info_index.find(info_key);
	if (search == info_index.end())
	{
		for (const auto & msg : native_messages)
		{
			const auto key_text = info_key + "^" + msg;
			insert_entry_base(key_text, "", msg, tools_t::rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	esm_ref.select_record(search->second);
	esm_ref.set_value("BNAM");
	if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
	{
		for (const auto & msg : native_messages)
		{
			const auto key_text = info_key + "^" + msg;
			insert_entry_base(key_text, "", msg, tools_t::rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	const auto foreign_messages = make_script_messages(esm_ref.get_value().text);

	if (native_messages.size() != foreign_messages.size())
	{
		tools_t::add_log("[warning] BNAM line count mismatch: \"" + info_key + "\"\r\n");
		for (const auto & msg : foreign_messages)
		{
			const auto key_text = info_key + "^" + msg;
			insert_entry_base(key_text, msg, msg, tools_t::rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	for (size_t k = 0; k < native_messages.size(); ++k)
	{
		const auto key_text = info_key + "^" + foreign_messages[k];
		const auto & old_text = foreign_messages[k];
		const auto & new_text = native_messages[k];
		insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::bnam, status_t::translated);
	}
}

void dict_creator_t::collect_bnam_missing_topics(const std::set<std::string> & matched_foreign_topics)
{
	std::string foreign_dial_type;
	std::string foreign_dial_name;

	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		const auto & rec_id = esm_ref.get_record().id;

		if (rec_id == "DIAL")
		{
			esm_ref.set_key("DATA");
			esm_ref.set_value("NAME");
			if (esm_ref.get_key().exist && esm_ref.get_value().exist)
			{
				foreign_dial_type = tools_t::get_dialog_type(esm_ref.get_key().content);
				foreign_dial_name = esm_ref.get_value().text;
			}
			continue;
		}

		if (rec_id != "INFO")
			continue;

		if (matched_foreign_topics.count(foreign_dial_name))
			continue;

		esm_ref.set_key("INAM");
		if (!esm_ref.get_key().exist)
			continue;

		const auto & inam = esm_ref.get_key().text;

		esm_ref.set_value("BNAM");
		if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
			continue;

		const auto foreign_messages = make_script_messages(esm_ref.get_value().text);
		for (const auto & msg : foreign_messages)
		{
			const auto key_text = foreign_dial_type + "^" + foreign_dial_name + "^" + inam + "^" + msg;
			insert_entry_base(key_text, msg, msg, tools_t::rec_type_t::bnam, status_t::missing);
		}
	}
}

void dict_creator_t::make_dict_base_bnam()
{
	std::string dial_type;
	std::string dial_name;
	std::string info_inam;
	reset_counters();

	std::set<std::string> matched_foreign_topics;
	for (const auto & [native, foreign] : dial_native_to_foreign)
		matched_foreign_topics.insert(foreign);

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
				const auto & native_name = esm.get_value().text;
				auto it_map = dial_native_to_foreign.find(native_name);
				if (it_map != dial_native_to_foreign.end())
					dial_name = it_map->second;
				else
					dial_name.clear();
			}
			continue;
		}

		if (rec_id != "INFO")
			continue;

		if (dial_name.empty())
			continue;

		esm.set_key("INAM");
		if (!esm.get_key().exist)
			continue;

		info_inam = esm.get_key().text;

		esm.set_value("BNAM");
		if (!esm.get_value().exist || esm.get_value().text.empty())
			continue;

		const auto native_messages = make_script_messages(esm.get_value().text);
		const auto info_key = dial_type + "^" + dial_name + "^" + info_inam;
		match_bnam_native_infos(info_key, native_messages);
	}

	collect_bnam_missing_topics(matched_foreign_topics);
}

void dict_creator_t::make_dict_base_dial()
{
	reset_counters();
	auto status_fn = [this](const std::string & old_text, const std::string & new_text)
	{ return determine_status(old_text, new_text); };
	dial_matcher_t dial_matcher(esm, esm_ref, m_translation_engine, dict, status_fn);
	dial_matcher.match_topics();
	dial_native_to_foreign = dial_matcher.get_native_to_foreign();
}

void dict_creator_t::make_dict_base_cell()
{
	auto status_fn = [this](const std::string & old_text, const std::string & new_text)
	{ return determine_status(old_text, new_text); };
	cell_matcher_t cell_matcher(esm, esm_ref, m_translation_engine, dict, status_fn);
	cell_matcher.match_exterior_cells();
	cell_matcher.match_interior_cells();
	cell_matcher.match_default_cell_name();
	cell_matcher.match_region_names();
}

void dict_creator_t::insert_entry_base(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    tools_t::rec_type_t type,
    status_t status)
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

	const bool is_problem =
	    (status == status_t::missing || status == status_t::mismatch || status == status_t::duplicate);

	const bool is_status = is_problem || status == status_t::heuristic;

	tools_t::record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = is_status ? status : determine_status(old_text, new_text);

	if (dict.at(type).insert(entry))
	{
		counter_created++;
		return;
	}

	auto * dup = dict.at(type).find(key_text);
	if (dup)
	{
		if (dup->old_text == old_text && dup->new_text == new_text)
			return;

		dup->status = status_t::duplicate;
		if (dup->details.empty())
			dup->details = new_text;
		else
			dup->details += "|" + new_text;

		counter_doubled++;
	}
}

void dict_creator_t::build_gmst_index()
{
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "GMST")
			continue;

		esm_ref.set_key("NAME");
		if (!esm_ref.get_key().exist)
			continue;

		if (esm_ref.get_key().text.substr(0, 1) != "s")
			continue;

		gmst_index.insert({ esm_ref.get_key().text, i });
	}
}

void dict_creator_t::build_fnam_index()
{
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (!tools_t::is_fnam(esm_ref.get_record().id))
			continue;

		esm_ref.set_key("NAME");
		if (!esm_ref.get_key().exist)
			continue;

		if (esm_ref.get_key().text == "player")
			continue;

		fnam_index.insert({ esm_ref.get_record().id + "^" + esm_ref.get_key().text, i });
	}
}

void dict_creator_t::build_desc_index()
{
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		const auto & rec_id = esm_ref.get_record().id;
		if (rec_id != "BSGN" && rec_id != "CLAS" && rec_id != "RACE")
			continue;

		esm_ref.set_key("NAME");
		if (!esm_ref.get_key().exist)
			continue;

		desc_index.insert({ rec_id + "^" + esm_ref.get_key().text, i });
	}
}

void dict_creator_t::build_text_index()
{
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "BOOK")
			continue;

		esm_ref.set_key("NAME");
		if (!esm_ref.get_key().exist)
			continue;

		text_index.insert({ esm_ref.get_key().text, i });
	}
}

void dict_creator_t::build_rnam_index()
{
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "FACT")
			continue;

		esm_ref.set_key("NAME");
		esm_ref.set_value("RNAM");
		if (!esm_ref.get_key().exist)
			continue;

		while (esm_ref.get_value().exist)
		{
			rnam_index.insert(
			    { esm_ref.get_key().text + "^" + std::to_string(esm_ref.get_value().counter),
			      esm_ref.get_value().text });
			esm_ref.set_next_value("RNAM");
		}
	}
}

void dict_creator_t::build_indx_index()
{
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		const auto & rec_id = esm_ref.get_record().id;
		if (rec_id != "SKIL" && rec_id != "MGEF")
			continue;

		esm_ref.set_key("INDX");
		if (!esm_ref.get_key().exist)
			continue;

		indx_index.insert({ rec_id + "^" + tools_t::get_indx(esm_ref.get_key().content), i });
	}
}

void dict_creator_t::build_info_index()
{
	std::string info_prefix;
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		const auto & rec_id = esm_ref.get_record().id;

		if (rec_id == "DIAL")
		{
			esm_ref.set_key("DATA");
			esm_ref.set_value("NAME");
			if (!esm_ref.get_key().exist || !esm_ref.get_value().exist)
				continue;

			info_prefix = tools_t::get_dialog_type(esm_ref.get_key().content) + "^" + esm_ref.get_value().text;
			continue;
		}

		if (rec_id == "INFO")
		{
			esm_ref.set_key("INAM");
			if (!esm_ref.get_key().exist)
				continue;

			info_index.insert({ info_prefix + "^" + esm_ref.get_key().text, i });
			continue;
		}
	}
}

status_t dict_creator_t::determine_status(const std::string & old_text, const std::string & new_text) const
{
	if (old_text != new_text)
		return status_t::translated;

	if (m_base_mode == base_mode_t::full)
		return status_t::translated;

	if (!m_english_dict)
		return status_t::untranslated;

	if (is_proper_noun(old_text))
		return status_t::to_verify;

	return status_t::untranslated;
}

bool dict_creator_t::is_proper_noun(const std::string & text) const
{
	auto words = split_words(text);
	for (const auto & word : words)
	{
		if (word.size() < 3)
			continue;

		if (m_english_dict->spell(word))
			return false;
	}

	return true;
}
