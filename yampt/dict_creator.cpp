#include "dict_creator.hpp"

dict_creator_t::dict_creator_t(const std::string & plugin_path, const tools_t::dict_t * base_dict)
    : esm(plugin_path)
    , esm_ref(esm)
    , base_dict(base_dict)
    , is_make_mode(true)
{
	dict = tools_t::initialize_dict();

	if (esm.is_loaded())
		make_dict();
}

dict_creator_t::dict_creator_t(const std::string & path, const std::string & path_ext)
    : esm(path)
    , esm_ext(path_ext)
    , esm_ref(esm_ext)
    , is_make_mode(false)
{
	dict = tools_t::initialize_dict();

	if (esm.is_loaded() && esm_ext.is_loaded())
	{
		make_dict();
	}
}

void dict_creator_t::make_dict()
{
	build_indexes();

	make_dict_gmst();
	make_dict_fnam();
	make_dict_desc();
	make_dict_text();
	make_dict_rnam();
	make_dict_indx();
	make_dict_flag();
	make_dict_info();

	if (is_make_mode)
	{
		make_dict_cell();
		make_dict_cell_default();
		make_dict_cell_regn();
		make_dict_dial();
		make_dict_script({ "INFO", "INAM", "BNAM", tools_t::rec_type_t::bnam });
		make_dict_script({ "SCPT", "SCHD", "SCTX", tools_t::rec_type_t::sctx });
	}
	else
	{
		make_dict_cell_unordered();
		make_dict_cell_unordered_default();
		make_dict_cell_unordered_regn();
		make_dict_dial_unordered();
		make_dict_script_unordered({ "INFO", "INAM", "BNAM", tools_t::rec_type_t::bnam });
		make_dict_script_unordered({ "SCPT", "SCHD", "SCTX", tools_t::rec_type_t::sctx });
		make_dict_fnam_glossary();

		tools_t::add_log(
		    "[warn] check dictionary for \"MISSING\" keyword\r\n"
		    "    missing CELL and DIAL records need to be added manually\r\n");
	}
}

void dict_creator_t::make_dict_cell_unordered_default()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "GMST")
			continue;

		esm.set_key("NAME");
		esm.set_value("STRV");
		if (esm.get_key().text == "sDefaultCellname" && esm.get_value().exist)
		{
			for (size_t k = 0; k < esm_ref.get_records().size(); ++k)
			{
				esm_ref.select_record(k);
				if (esm_ref.get_record().id != "GMST")
					continue;

				esm_ref.set_key("NAME");
				esm_ref.set_value("STRV");
				if (esm_ref.get_key().text == "sDefaultCellname" && esm_ref.get_value().exist)
				{
					const auto & key_text = esm_ref.get_value().text;
					const auto & val_text = esm.get_value().text;
					insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::cell);
					break;
				}
			}
			break;
		}
	}
	print_log_line(tools_t::rec_type_t::default_val);
}

void dict_creator_t::make_dict_cell_unordered_regn()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "REGN")
			continue;

		esm.set_key("NAME");
		esm.set_value("FNAM");
		if (esm.get_key().exist && esm.get_value().exist)
		{
			for (size_t k = 0; k < esm_ref.get_records().size(); ++k)
			{
				esm_ref.select_record(k);
				if (esm_ref.get_record().id != "REGN")
					continue;

				esm_ref.set_key("NAME");
				esm_ref.set_value("FNAM");
				if (esm_ref.get_key().text == esm.get_key().text && esm_ref.get_value().exist)
				{
					const auto & key_text = esm_ref.get_value().text;
					const auto & val_text = esm.get_value().text;
					insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::cell);
					break;
				}
			}
		}
	}
	print_log_line(tools_t::rec_type_t::regn);
}

void dict_creator_t::build_indexes()
{
	std::string info_prefix;

	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		const auto & rec_id = esm_ref.get_record().id;

		if (rec_id == "GMST")
		{
			esm_ref.set_key("NAME");
			if (!esm_ref.get_key().exist)
				continue;
			if (esm_ref.get_key().text.substr(0, 1) != "s")
				continue;
			gmst_index.insert({ esm_ref.get_key().text, i });
			continue;
		}

		if (tools_t::is_fnam(rec_id))
		{
			esm_ref.set_key("NAME");
			if (!esm_ref.get_key().exist)
				continue;
			if (esm_ref.get_key().text == "player")
				continue;
			fnam_index.insert({ rec_id + "^" + esm_ref.get_key().text, i });
			continue;
		}

		if (rec_id == "BSGN" || rec_id == "CLAS" || rec_id == "RACE")
		{
			esm_ref.set_key("NAME");
			if (!esm_ref.get_key().exist)
				continue;
			desc_index.insert({ rec_id + "^" + esm_ref.get_key().text, i });
			continue;
		}

		if (rec_id == "BOOK")
		{
			esm_ref.set_key("NAME");
			if (!esm_ref.get_key().exist)
				continue;
			text_index.insert({ esm_ref.get_key().text, i });
			continue;
		}

		if (rec_id == "FACT")
		{
			esm_ref.set_key("NAME");
			esm_ref.set_value("RNAM");
			if (!esm_ref.get_key().exist)
				continue;
			while (esm_ref.get_value().exist)
			{
				rnam_index.insert({ esm_ref.get_key().text + "^" + std::to_string(esm_ref.get_value().counter), i });
				esm_ref.set_next_value("RNAM");
			}
			continue;
		}

		if (rec_id == "SKIL" || rec_id == "MGEF")
		{
			esm_ref.set_key("INDX");
			if (!esm_ref.get_key().exist)
				continue;
			indx_index.insert({ rec_id + "^" + tools_t::get_indx(esm_ref.get_key().content), i });
			continue;
		}

		if (rec_id == "NPC_")
		{
			esm_ref.set_key("NAME");
			if (!esm_ref.get_key().exist)
				continue;
			flag_index.insert({ esm_ref.get_key().text, i });
			continue;
		}

		if (rec_id == "DIAL")
		{
			esm_ref.set_key("DATA");
			esm_ref.set_value("NAME");
			if (!esm_ref.get_key().exist || !esm_ref.get_value().exist)
				continue;
			info_prefix = tools_t::get_dialog_type(esm_ref.get_key().content) + "^" +
			              translate_dialog_topic(esm_ref.get_value().text);
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

void dict_creator_t::make_dict_cell()
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

		const auto & key_text = esm.get_value().text;
		const auto & old_text = esm.get_value().text;
		insert_entry(key_text, old_text, "", tools_t::rec_type_t::cell);
	}
	print_log_line(tools_t::rec_type_t::cell);
}

void dict_creator_t::make_dict_cell_default()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "GMST")
			continue;

		esm.set_key("NAME");
		esm.set_value("STRV");
		if (esm.get_key().text != "sDefaultCellname")
			continue;
		if (!esm.get_value().exist)
			continue;

		const auto & key_text = esm.get_value().text;
		const auto & old_text = esm.get_value().text;
		insert_entry(key_text, old_text, "", tools_t::rec_type_t::cell);
	}
	print_log_line(tools_t::rec_type_t::default_val);
}

void dict_creator_t::make_dict_cell_regn()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "REGN")
			continue;

		esm.set_value("FNAM");
		if (!esm.get_value().exist)
			continue;

		const auto & key_text = esm.get_value().text;
		const auto & old_text = esm.get_value().text;
		insert_entry(key_text, old_text, "", tools_t::rec_type_t::cell);
	}
	print_log_line(tools_t::rec_type_t::regn);
}

void dict_creator_t::make_dict_gmst()
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
		const auto & new_text = esm.get_value().text;

		std::string old_text;
		auto search = gmst_index.find(key_text);
		if (search != gmst_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_value("STRV");
			old_text = esm_ref.get_value().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::gmst);
	}
	print_log_line(tools_t::rec_type_t::gmst);
}

void dict_creator_t::make_dict_fnam()
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

		const auto & key_text = esm.get_record().id + "^" + esm.get_key().text;
		const auto & new_text = esm.get_value().text;

		std::string old_text;
		auto search = fnam_index.find(key_text);
		if (search != fnam_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_value("FNAM");
			old_text = esm_ref.get_value().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::fnam);
	}
	print_log_line(tools_t::rec_type_t::fnam);
}

void dict_creator_t::make_dict_fnam_glossary()
{
	std::unordered_map<std::string, size_t> ext_index;
	for (size_t k = 0; k < esm_ref.get_records().size(); ++k)
	{
		esm_ref.select_record(k);
		if (!tools_t::is_fnam(esm_ref.get_record().id))
			continue;

		esm_ref.set_key("NAME");
		if (!esm_ref.get_key().exist)
			continue;

		ext_index.insert({ esm_ref.get_record().id + "^" + esm_ref.get_key().text, k });
	}

	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (!tools_t::is_fnam(esm.get_record().id))
			continue;

		esm.set_key("NAME");
		esm.set_value("FNAM");
		if (esm.get_key().exist && esm.get_value().exist && esm.get_value().text != "")
		{
			auto search = ext_index.find(esm.get_record().id + "^" + esm.get_key().text);
			if (search == ext_index.end())
				continue;

			esm_ref.select_record(search->second);
			esm_ref.set_value("FNAM");
			if (esm_ref.get_value().exist && esm_ref.get_value().text != "")
			{
				const auto & key_text = esm_ref.get_value().text;
				const auto & val_text = esm.get_value().text + " " + esm.get_record().id + "^" + esm.get_key().text;
				insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::glossary);
			}
		}
	}
	print_log_line(tools_t::rec_type_t::glossary);
}

void dict_creator_t::make_dict_desc()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "BSGN" && esm.get_record().id != "CLAS" && esm.get_record().id != "RACE")
			continue;

		esm.set_key("NAME");
		esm.set_value("DESC");
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		const auto & key_text = esm.get_record().id + "^" + esm.get_key().text;
		const auto & new_text = esm.get_value().text;

		std::string old_text;
		auto search = desc_index.find(key_text);
		if (search != desc_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_value("DESC");
			old_text = esm_ref.get_value().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::desc);
	}
	print_log_line(tools_t::rec_type_t::desc);
}

void dict_creator_t::make_dict_text()
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
		const auto & new_text = esm.get_value().text;

		std::string old_text;
		auto search = text_index.find(key_text);
		if (search != text_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_value("TEXT");
			old_text = esm_ref.get_value().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::text);
	}
	print_log_line(tools_t::rec_type_t::text);
}

void dict_creator_t::make_dict_rnam()
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
			const auto & key_text = esm.get_key().text + "^" + std::to_string(esm.get_value().counter);
			const auto & new_text = esm.get_value().text;

			std::string old_text;
			auto search = rnam_index.find(key_text);
			if (search != rnam_index.end())
			{
				esm_ref.select_record(search->second);
				esm_ref.set_key("NAME");
				esm_ref.set_value("RNAM");
				while (esm_ref.get_value().exist && esm_ref.get_value().counter != esm.get_value().counter)
					esm_ref.set_next_value("RNAM");
				if (esm_ref.get_value().exist)
					old_text = esm_ref.get_value().text;
				else
					old_text = new_text;
			}
			else
			{
				old_text = new_text;
			}

			insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::rnam);
			esm.set_next_value("RNAM");
		}
	}
	print_log_line(tools_t::rec_type_t::rnam);
}

void dict_creator_t::make_dict_indx()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "SKIL" && esm.get_record().id != "MGEF")
			continue;

		esm.set_key("INDX");
		esm.set_value("DESC");
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		const auto & key_text = esm.get_record().id + "^" + tools_t::get_indx(esm.get_key().content);
		const auto & new_text = esm.get_value().text;

		std::string old_text;
		auto search = indx_index.find(key_text);
		if (search != indx_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_value("DESC");
			old_text = esm_ref.get_value().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::indx);
	}
	print_log_line(tools_t::rec_type_t::indx);
}

void dict_creator_t::make_dict_dial()
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

		const auto & key_text = esm.get_value().text;
		const auto & old_text = esm.get_value().text;
		insert_entry(key_text, old_text, "", tools_t::rec_type_t::dial);
	}
	print_log_line(tools_t::rec_type_t::dial);
}

void dict_creator_t::make_dict_flag()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "NPC_")
			continue;

		esm.set_key("NAME");
		esm.set_value("FLAG");
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		const auto & key_text = esm.get_key().text;
		const auto & new_text =
		    ((tools_t::convert_string_byte_array_to_uint(esm.get_value().content) & 0x0001) != 0) ? "F" : "M";

		std::string old_text;
		auto search = flag_index.find(key_text);
		if (search != flag_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_value("FLAG");
			old_text =
			    ((tools_t::convert_string_byte_array_to_uint(esm_ref.get_value().content) & 0x0001) != 0) ? "F" : "M";
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::npc_flag);
	}
	print_log_line(tools_t::rec_type_t::npc_flag);
}

void dict_creator_t::make_dict_info()
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
				key_prefix = tools_t::get_dialog_type(esm.get_key().content) + "^" +
				             translate_dialog_topic(esm.get_value().text);
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

		const auto & key_text = key_prefix + "^" + esm.get_key().text;
		const auto & new_text = esm.get_value().text;

		std::string old_text;
		auto search = info_index.find(key_text);
		if (search != info_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_value("NAME");
			old_text = esm_ref.get_value().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::info);
	}
	print_log_line(tools_t::rec_type_t::info);
}

void dict_creator_t::make_dict_script(const ids & ids)
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != ids.rec_id)
			continue;

		esm.set_key(ids.key_id);
		esm.set_value(ids.val_id);
		if (!esm.get_key().exist || !esm.get_value().exist)
			continue;

		const auto & messages = make_script_messages(esm.get_value().text);
		for (size_t k = 0; k < messages.size(); ++k)
		{
			const auto & key_text = esm.get_key().text + "^" + messages.at(k);
			insert_entry(key_text, key_text, "", ids.type);
		}
	}
	print_log_line(ids.type);
}

void dict_creator_t::make_dict_cell_unordered()
{
	auto patterns_ext = make_dict_cell_unordered_patterns_ext();
	const auto & patterns = make_dict_cell_unordered_patterns();

	reset_counters();
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search != patterns.end())
		{
			esm.select_record(search->second);
			esm.set_value("NAME");
			esm_ref.select_record(patterns_ext[i].pos);
			esm_ref.set_value("NAME");
			if (esm.get_value().exist && esm.get_value().text != "" && esm_ref.get_value().exist &&
			    esm_ref.get_value().text != "")
			{
				const auto & key_text = esm_ref.get_value().text;
				const auto & val_text = esm.get_value().text;
				insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::cell);
			}
		}
		else
		{
			patterns_ext[i].missing = true;
			counter_missing++;
		}
	}
	make_dict_cell_unordered_add_missing(patterns_ext);
	print_log_line(tools_t::rec_type_t::cell);
}

void dict_creator_t::make_dict_dial_unordered()
{
	auto patterns_ext = make_dict_dial_unordered_patterns_ext();
	const auto & patterns = make_dict_dial_unordered_patterns();

	reset_counters();
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search != patterns.end())
		{
			esm.select_record(search->second);
			esm.set_value("NAME");
			esm_ref.select_record(patterns_ext[i].pos);
			esm_ref.set_value("NAME");
			if (esm.get_value().exist && esm_ref.get_value().exist)
			{
				const auto & key_text = esm_ref.get_value().text;
				const auto & val_text = esm.get_value().text;
				insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::dial);
			}
		}
		else
		{
			patterns_ext[i].missing = true;
			counter_missing++;
		}
	}
	make_dict_dial_unordered_add_missing(patterns_ext);
	print_log_line(tools_t::rec_type_t::dial);
}

dict_creator_t::patterns_ext_t dict_creator_t::make_dict_cell_unordered_patterns_ext()
{
	patterns_ext_t patterns_ext;
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "CELL")
			continue;

		esm_ref.set_value("NAME");
		if (esm_ref.get_value().exist && esm_ref.get_value().text != "")
		{
			patterns_ext.push_back({ make_dict_cell_unordered_pattern(esm_ref), i, false });
		}
	}
	return patterns_ext;
}

dict_creator_t::patterns_ext_t dict_creator_t::make_dict_dial_unordered_patterns_ext()
{
	patterns_ext_t patterns_ext;
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "DIAL")
			continue;

		esm_ref.set_key("DATA");
		if (tools_t::get_dialog_type(esm_ref.get_key().content) == "T")
		{
			patterns_ext.push_back({ make_dict_dial_unordered_pattern(esm_ref, i), i, false });
		}
	}
	return patterns_ext;
}

dict_creator_t::patterns dict_creator_t::make_dict_cell_unordered_patterns()
{
	patterns patterns;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "CELL")
			continue;

		esm.set_value("NAME");
		if (esm.get_value().exist && esm.get_value().text != "")
		{
			patterns.insert({ make_dict_cell_unordered_pattern(esm), i });
		}
	}
	return patterns;
}

dict_creator_t::patterns dict_creator_t::make_dict_dial_unordered_patterns()
{
	patterns patterns;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "DIAL")
			continue;

		esm.set_key("DATA");
		if (tools_t::get_dialog_type(esm.get_key().content) == "T")
		{
			patterns.insert({ make_dict_dial_unordered_pattern(esm, i), i });
		}
	}
	return patterns;
}

std::string dict_creator_t::make_dict_cell_unordered_pattern(esm_reader_t & esm_cur)
{
	std::string pattern;
	esm_cur.set_value("DATA");
	pattern += esm_cur.get_value().content;
	esm_cur.set_value("NAME");
	while (esm_cur.get_value().exist)
	{
		esm_cur.set_next_value("NAME");
		pattern += esm_cur.get_value().content;
	}
	return pattern;
}

std::string dict_creator_t::make_dict_dial_unordered_pattern(esm_reader_t & esm_cur, size_t i)
{
	std::string pattern;
	esm_cur.select_record(i + 1);
	esm_cur.set_value("INAM");
	pattern += esm_cur.get_value().content;
	esm_cur.set_value("SCVR");
	pattern += esm_cur.get_value().content;
	return pattern;
}

void dict_creator_t::make_dict_cell_unordered_add_missing(const patterns_ext_t & patterns_ext)
{
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		if (!patterns_ext[i].missing)
			continue;

		esm_ref.select_record(patterns_ext[i].pos);
		esm_ref.set_value("NAME");
		if (!esm_ref.get_value().exist)
			continue;

		if (esm_ref.get_value().text == "")
			continue;

		const auto & key_text = esm_ref.get_value().text;
		const auto & val_text = "MISSING";
		insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::cell);
		tools_t::add_log("missing CELL: " + esm_ref.get_value().text + "\r\n");
	}
}

void dict_creator_t::make_dict_dial_unordered_add_missing(const patterns_ext_t & patterns_ext)
{
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		if (!patterns_ext[i].missing)
			continue;

		esm_ref.select_record(patterns_ext[i].pos);
		esm_ref.set_value("NAME");
		if (!esm_ref.get_value().exist)
			continue;

		const auto & key_text = esm_ref.get_value().text;
		const auto & val_text = "MISSING";
		insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::dial);
		tools_t::add_log("missing DIAL: " + esm_ref.get_value().text + "\r\n");
	}
}

void dict_creator_t::make_dict_script_unordered(const ids & ids)
{
	auto patterns_ext = make_dict_unordered_patterns_ext(ids);
	const auto & patterns = make_dict_unordered_patterns(ids);

	reset_counters();
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search == patterns.end())
			continue;

		esm.select_record(search->second);
		esm.set_key(ids.key_id);
		esm.set_value(ids.val_id);
		esm_ref.select_record(patterns_ext[i].pos);
		esm_ref.set_key(ids.key_id);
		esm_ref.set_value(ids.val_id);
		if (esm.get_key().exist && esm.get_value().exist && esm_ref.get_key().exist && esm_ref.get_value().exist)
		{
			const auto & message = make_script_messages(esm.get_value().text);
			const auto & message_ext = make_script_messages(esm_ref.get_value().text);
			if (message.size() != message_ext.size())
				continue;

			for (size_t k = 0; k < message.size(); ++k)
			{
				const auto & key_text = esm_ref.get_key().text + "^" + message_ext.at(k);
				const auto & val_text = esm.get_key().text + "^" + message.at(k);
				insert_entry(key_text, key_text, val_text, ids.type);
			}
		}
	}
	print_log_line(ids.type);
}

dict_creator_t::patterns_ext_t dict_creator_t::make_dict_unordered_patterns_ext(const ids & ids)
{
	patterns_ext_t patterns_ext;
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != ids.rec_id)
			continue;

		esm_ref.set_key(ids.key_id);
		if (!esm_ref.get_key().exist)
			continue;

		patterns_ext.push_back({ esm_ref.get_key().text, i, false });
	}
	return patterns_ext;
}

dict_creator_t::patterns dict_creator_t::make_dict_unordered_patterns(const ids & ids)
{
	patterns patterns;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != ids.rec_id)
			continue;

		esm.set_key(ids.key_id);
		if (!esm.get_key().exist)
			continue;

		patterns.insert({ esm.get_key().text, i });
	}
	return patterns;
}

void dict_creator_t::reset_counters()
{
	counter_created = 0;
	counter_missing = 0;
	counter_doubled = 0;
	counter_identical = 0;
	counter_all = 0;
}

void dict_creator_t::insert_entry(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    tools_t::rec_type_t type)
{
	counter_all++;

	tools_t::record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;

	if (is_make_mode)
	{
		if (!base_dict)
		{
			entry.new_text = old_text;
			entry.status = tools_t::status_t::untranslated;
			if (dict.at(type).insert(entry))
				counter_created++;
			else
				counter_identical++;
			return;
		}

		auto it = base_dict->find(type);
		if (it == base_dict->end())
		{
			entry.new_text = old_text;
			entry.status = tools_t::status_t::untranslated;
			if (dict.at(type).insert(entry))
				counter_created++;
			else
				counter_identical++;
			return;
		}

		const auto * base_entry = it->second.find(key_text);
		if (!base_entry)
		{
			entry.new_text = old_text;
			entry.status = tools_t::status_t::untranslated;
			if (dict.at(type).insert(entry))
				counter_created++;
			else
				counter_identical++;
			return;
		}

		entry.new_text = base_entry->new_text;
		if (base_entry->old_text == old_text)
		{
			if (old_text == base_entry->new_text)
				entry.status = tools_t::status_t::auto_identical;
			else
				entry.status = tools_t::status_t::translated;
		}
		else
		{
			entry.status = tools_t::status_t::changed;
		}

		if (dict.at(type).insert(entry))
			counter_created++;
		else
			counter_identical++;
		return;
	}

	entry.new_text = new_text;
	entry.status = "";

	if (dict.at(type).insert(entry))
	{
		counter_created++;
		return;
	}

	auto * existing = dict.at(type).find(key_text);
	if (existing != nullptr && existing->new_text != new_text)
	{
		tools_t::record_entry_t doubled_entry;
		doubled_entry.key_text = key_text + "^DOUBLED_" + std::to_string(counter_doubled);
		doubled_entry.old_text = old_text;
		doubled_entry.new_text = new_text;
		doubled_entry.status = "";
		dict.at(type).insert(doubled_entry);
		counter_doubled++;
		counter_created++;
		if (type != tools_t::rec_type_t::glossary)
			tools_t::add_log("[warn] doubled " + tools_t::type_to_str(type) + ": " + key_text + "\r\n");
	}
	else
	{
		counter_identical++;
	}
}

void dict_creator_t::print_log_line(const tools_t::rec_type_t type)
{
	std::string line = tools_t::type_to_str(type) + ": created=" + std::to_string(counter_created);

	if (type == tools_t::rec_type_t::cell || type == tools_t::rec_type_t::dial)
	{
		line += ", missing=" + std::to_string(counter_missing);
	}

	line += ", identical=" + std::to_string(counter_identical);
	line += ", total=" + std::to_string(counter_all);
	line += "\r\n";

	tools_t::add_log(line);
}

std::string dict_creator_t::translate_dialog_topic(std::string to_translate)
{
	return to_translate;
}

std::vector<std::string> dict_creator_t::make_script_messages(const std::string & script_text)
{
	std::vector<std::string> messages;
	std::string line;
	std::string line_lc;
	std::istringstream ss(script_text);

	while (std::getline(ss, line))
	{
		line = tools_t::trim_cr(line);
		line_lc = line;
		transform(line_lc.begin(), line_lc.end(), line_lc.begin(), ::tolower);

		size_t keyword_pos;
		std::set<size_t> keyword_pos_coll;

		for (const auto & keyword : tools_t::keywords)
		{
			keyword_pos = line_lc.find(keyword);
			if (keyword_pos == std::string::npos)
				continue;
			if (keyword_pos > 0 &&
			    (std::isalnum(static_cast<unsigned char>(line_lc[keyword_pos - 1])) || line_lc[keyword_pos - 1] == '_'))
				continue;
			if (keyword_pos + keyword.size() < line_lc.size() &&
			    (std::isalnum(static_cast<unsigned char>(line_lc[keyword_pos + keyword.size()])) ||
			     line_lc[keyword_pos + keyword.size()] == '_'))
				continue;
			keyword_pos_coll.insert(keyword_pos);
		}

		if (keyword_pos_coll.empty())
			continue;

		keyword_pos = *keyword_pos_coll.begin();

		if (keyword_pos != std::string::npos && line.rfind(";", keyword_pos) == std::string::npos &&
		    line.find("\"", keyword_pos) != std::string::npos)
		{
			messages.push_back(line);
		}
	}
	return messages;
}
