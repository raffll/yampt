#include "dictcreator.hpp"

dict_creator_t::dict_creator_t(
	const std::string & plugin_path,
	const tools_t::dict_t * base_dict
)
	: esm(plugin_path)
	, esm_ref(esm)
	, base_dict(base_dict)
	, is_make_mode(true)
{
	dict = tools_t::initializeDict();

	if (esm.isLoaded())
		make_dict();
}


dict_creator_t::dict_creator_t(
	const std::string & path,
	const std::string & path_ext
)
	: esm(path)
	, esm_ext(path_ext)
	, esm_ref(esm_ext)
	, is_make_mode(false)
{
	dict = tools_t::initializeDict();

	if (esm.isLoaded() &&
		esm_ext.isLoaded())
	{
		make_dict();
	}
}


void dict_creator_t::make_dict()
{
	tools_t::addLog("-----------------------------------------------\r\n"
				  "          Created / Missing / Identical /   All\r\n"
				  "-----------------------------------------------\r\n");

	build_indexes();

	make_dict_GMST();
	make_dict_FNAM();
	make_dict_DESC();
	make_dict_TEXT();
	make_dict_RNAM();
	make_dict_INDX();
	make_dict_FLAG();
	make_dict_INFO();

	if (is_make_mode)
	{
		make_dict_CELL();
		make_dict_CELL_default();
		make_dict_CELL_REGN();
		make_dict_DIAL();
		make_dict_script({ "INFO", "INAM", "BNAM", tools_t::rec_type_t::BNAM });
		make_dict_script({ "SCPT", "SCHD", "SCTX", tools_t::rec_type_t::SCTX });
	}
	else
	{
		make_dict_CELL_Unordered();
		make_dict_CELL_Unordered_Default();
		make_dict_CELL_Unordered_REGN();
		make_dict_DIAL_Unordered();
		make_dict_Script_Unordered({ "INFO", "INAM", "BNAM", tools_t::rec_type_t::BNAM });
		make_dict_Script_Unordered({ "SCPT", "SCHD", "SCTX", tools_t::rec_type_t::SCTX });
		make_dict_FNAM_glossary();

		tools_t::addLog("--> Check dictionary for \"MISSING\" keyword!\r\n"
					  "    Missing CELL and DIAL records needs to be added manually!\r\n");
	}

	tools_t::addLog("-----------------------------------------------\r\n");
}


void dict_creator_t::make_dict_CELL_Unordered_Default()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "GMST")
			continue;

		esm.setKey("NAME");
		esm.setValue("STRV");
		if (esm.getKey().text == "sDefaultCellname" &&
			esm.getValue().exist)
		{
			for (size_t k = 0; k < esm_ref.getRecords().size(); ++k)
			{
				esm_ref.selectRecord(k);
				if (esm_ref.getRecord().id != "GMST")
					continue;

				esm_ref.setKey("NAME");
				esm_ref.setValue("STRV");
				if (esm_ref.getKey().text == "sDefaultCellname" &&
					esm_ref.getValue().exist)
				{
					const auto & key_text = esm_ref.getValue().text;
					const auto & val_text = esm.getValue().text;
					insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::CELL);
					break;
				}
			}
			break;
		}
	}
	print_log_line(tools_t::rec_type_t::Default);
}


void dict_creator_t::make_dict_CELL_Unordered_REGN()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "REGN")
			continue;

		esm.setKey("NAME");
		esm.setValue("FNAM");
		if (esm.getKey().exist &&
			esm.getValue().exist)
		{
			for (size_t k = 0; k < esm_ref.getRecords().size(); ++k)
			{
				esm_ref.selectRecord(k);
				if (esm_ref.getRecord().id != "REGN")
					continue;

				esm_ref.setKey("NAME");
				esm_ref.setValue("FNAM");
				if (esm_ref.getKey().text == esm.getKey().text &&
					esm_ref.getValue().exist)
				{
					const auto & key_text = esm_ref.getValue().text;
					const auto & val_text = esm.getValue().text;
					insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::CELL);
					break;
				}
			}
		}
	}
	print_log_line(tools_t::rec_type_t::REGN);
}


void dict_creator_t::build_indexes()
{
	std::string info_prefix;

	for (size_t i = 0; i < esm_ref.getRecords().size(); ++i)
	{
		esm_ref.selectRecord(i);
		const auto & rec_id = esm_ref.getRecord().id;

		if (rec_id == "GMST")
		{
			esm_ref.setKey("NAME");
			if (!esm_ref.getKey().exist)
				continue;
			if (esm_ref.getKey().text.substr(0, 1) != "s")
				continue;
			gmst_index.insert({ esm_ref.getKey().text, i });
			continue;
		}

		if (tools_t::isFNAM(rec_id))
		{
			esm_ref.setKey("NAME");
			if (!esm_ref.getKey().exist)
				continue;
			if (esm_ref.getKey().text == "player")
				continue;
			fnam_index.insert({ rec_id + "^" + esm_ref.getKey().text, i });
			continue;
		}

		if (rec_id == "BSGN" || rec_id == "CLAS" || rec_id == "RACE")
		{
			esm_ref.setKey("NAME");
			if (!esm_ref.getKey().exist)
				continue;
			desc_index.insert({ rec_id + "^" + esm_ref.getKey().text, i });
			continue;
		}

		if (rec_id == "BOOK")
		{
			esm_ref.setKey("NAME");
			if (!esm_ref.getKey().exist)
				continue;
			text_index.insert({ esm_ref.getKey().text, i });
			continue;
		}

		if (rec_id == "FACT")
		{
			esm_ref.setKey("NAME");
			esm_ref.setValue("RNAM");
			if (!esm_ref.getKey().exist)
				continue;
			while (esm_ref.getValue().exist)
			{
				rnam_index.insert({ esm_ref.getKey().text + "^" + std::to_string(esm_ref.getValue().counter), i });
				esm_ref.setNextValue("RNAM");
			}
			continue;
		}

		if (rec_id == "SKIL" || rec_id == "MGEF")
		{
			esm_ref.setKey("INDX");
			if (!esm_ref.getKey().exist)
				continue;
			indx_index.insert({ rec_id + "^" + tools_t::getINDX(esm_ref.getKey().content), i });
			continue;
		}

		if (rec_id == "NPC_")
		{
			esm_ref.setKey("NAME");
			if (!esm_ref.getKey().exist)
				continue;
			flag_index.insert({ esm_ref.getKey().text, i });
			continue;
		}

		if (rec_id == "DIAL")
		{
			esm_ref.setKey("DATA");
			esm_ref.setValue("NAME");
			if (!esm_ref.getKey().exist || !esm_ref.getValue().exist)
				continue;
			info_prefix = tools_t::getDialogType(esm_ref.getKey().content) + "^" +
				translate_dialog_topic(esm_ref.getValue().text);
			continue;
		}

		if (rec_id == "INFO")
		{
			esm_ref.setKey("INAM");
			if (!esm_ref.getKey().exist)
				continue;
			info_index.insert({ info_prefix + "^" + esm_ref.getKey().text, i });
			continue;
		}
	}
}


void dict_creator_t::make_dict_CELL()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "CELL")
			continue;

		esm.setValue("NAME");
		if (!esm.getValue().exist || esm.getValue().text.empty())
			continue;

		const auto & id = esm.getValue().text;
		const auto & old_text = esm.getValue().text;
		insert_entry(id, old_text, "", tools_t::rec_type_t::CELL);
	}
	print_log_line(tools_t::rec_type_t::CELL);
}


void dict_creator_t::make_dict_CELL_default()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "GMST")
			continue;

		esm.setKey("NAME");
		esm.setValue("STRV");
		if (esm.getKey().text != "sDefaultCellname")
			continue;
		if (!esm.getValue().exist)
			continue;

		const auto & id = esm.getValue().text;
		const auto & old_text = esm.getValue().text;
		insert_entry(id, old_text, "", tools_t::rec_type_t::CELL);
	}
	print_log_line(tools_t::rec_type_t::Default);
}


void dict_creator_t::make_dict_CELL_REGN()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "REGN")
			continue;

		esm.setValue("FNAM");
		if (!esm.getValue().exist)
			continue;

		const auto & id = esm.getValue().text;
		const auto & old_text = esm.getValue().text;
		insert_entry(id, old_text, "", tools_t::rec_type_t::CELL);
	}
	print_log_line(tools_t::rec_type_t::REGN);
}


void dict_creator_t::make_dict_GMST()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "GMST")
			continue;

		esm.setKey("NAME");
		esm.setValue("STRV");
		if (!esm.getKey().exist || !esm.getValue().exist)
			continue;
		if (esm.getKey().text.substr(0, 1) != "s")
			continue;

		const auto & id = esm.getKey().text;
		const auto & new_text = esm.getValue().text;

		std::string old_text;
		auto search = gmst_index.find(id);
		if (search != gmst_index.end())
		{
			esm_ref.selectRecord(search->second);
			esm_ref.setValue("STRV");
			old_text = esm_ref.getValue().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(id, old_text, new_text, tools_t::rec_type_t::GMST);
	}
	print_log_line(tools_t::rec_type_t::GMST);
}


void dict_creator_t::make_dict_FNAM()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (!tools_t::isFNAM(esm.getRecord().id))
			continue;

		esm.setKey("NAME");
		esm.setValue("FNAM");
		if (!esm.getKey().exist || !esm.getValue().exist)
			continue;
		if (esm.getKey().text == "player")
			continue;

		const auto & id = esm.getRecord().id + "^" + esm.getKey().text;
		const auto & new_text = esm.getValue().text;

		std::string old_text;
		auto search = fnam_index.find(id);
		if (search != fnam_index.end())
		{
			esm_ref.selectRecord(search->second);
			esm_ref.setValue("FNAM");
			old_text = esm_ref.getValue().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(id, old_text, new_text, tools_t::rec_type_t::FNAM);
	}
	print_log_line(tools_t::rec_type_t::FNAM);
}


void dict_creator_t::make_dict_FNAM_glossary()
{
	std::unordered_map<std::string, size_t> ext_index;
	for (size_t k = 0; k < esm_ref.getRecords().size(); ++k)
	{
		esm_ref.selectRecord(k);
		if (!tools_t::isFNAM(esm_ref.getRecord().id))
			continue;

		esm_ref.setKey("NAME");
		if (!esm_ref.getKey().exist)
			continue;

		ext_index.insert({ esm_ref.getRecord().id + "^" + esm_ref.getKey().text, k });
	}

	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (!tools_t::isFNAM(esm.getRecord().id))
			continue;

		esm.setKey("NAME");
		esm.setValue("FNAM");
		if (esm.getKey().exist &&
			esm.getValue().exist &&
			esm.getValue().text != "")
		{
			auto search = ext_index.find(esm.getRecord().id + "^" + esm.getKey().text);
			if (search == ext_index.end())
				continue;

			esm_ref.selectRecord(search->second);
			esm_ref.setValue("FNAM");
			if (esm_ref.getValue().exist &&
				esm_ref.getValue().text != "")
			{
				const auto & key_text = esm_ref.getValue().text;
				const auto & val_text =
					esm.getValue().text + " "
					+ esm.getRecord().id + "^"
					+ esm.getKey().text;
				insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::Glossary);
			}
		}
	}
	print_log_line(tools_t::rec_type_t::Glossary);
}


void dict_creator_t::make_dict_DESC()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "BSGN" &&
			esm.getRecord().id != "CLAS" &&
			esm.getRecord().id != "RACE")
			continue;

		esm.setKey("NAME");
		esm.setValue("DESC");
		if (!esm.getKey().exist || !esm.getValue().exist)
			continue;

		const auto & id = esm.getRecord().id + "^" + esm.getKey().text;
		const auto & new_text = esm.getValue().text;

		std::string old_text;
		auto search = desc_index.find(id);
		if (search != desc_index.end())
		{
			esm_ref.selectRecord(search->second);
			esm_ref.setValue("DESC");
			old_text = esm_ref.getValue().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(id, old_text, new_text, tools_t::rec_type_t::DESC);
	}
	print_log_line(tools_t::rec_type_t::DESC);
}


void dict_creator_t::make_dict_TEXT()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "BOOK")
			continue;

		esm.setKey("NAME");
		esm.setValue("TEXT");
		if (!esm.getKey().exist || !esm.getValue().exist)
			continue;

		const auto & id = esm.getKey().text;
		const auto & new_text = esm.getValue().text;

		std::string old_text;
		auto search = text_index.find(id);
		if (search != text_index.end())
		{
			esm_ref.selectRecord(search->second);
			esm_ref.setValue("TEXT");
			old_text = esm_ref.getValue().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(id, old_text, new_text, tools_t::rec_type_t::TEXT);
	}
	print_log_line(tools_t::rec_type_t::TEXT);
}


void dict_creator_t::make_dict_RNAM()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "FACT")
			continue;

		esm.setKey("NAME");
		esm.setValue("RNAM");
		if (!esm.getKey().exist)
			continue;

		while (esm.getValue().exist)
		{
			const auto & id = esm.getKey().text + "^" + std::to_string(esm.getValue().counter);
			const auto & new_text = esm.getValue().text;

			std::string old_text;
			auto search = rnam_index.find(id);
			if (search != rnam_index.end())
			{
				esm_ref.selectRecord(search->second);
				esm_ref.setKey("NAME");
				esm_ref.setValue("RNAM");
				while (esm_ref.getValue().exist && esm_ref.getValue().counter != esm.getValue().counter)
					esm_ref.setNextValue("RNAM");
				if (esm_ref.getValue().exist)
					old_text = esm_ref.getValue().text;
				else
					old_text = new_text;
			}
			else
			{
				old_text = new_text;
			}

			insert_entry(id, old_text, new_text, tools_t::rec_type_t::RNAM);
			esm.setNextValue("RNAM");
		}
	}
	print_log_line(tools_t::rec_type_t::RNAM);
}


void dict_creator_t::make_dict_INDX()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "SKIL" &&
			esm.getRecord().id != "MGEF")
			continue;

		esm.setKey("INDX");
		esm.setValue("DESC");
		if (!esm.getKey().exist || !esm.getValue().exist)
			continue;

		const auto & id = esm.getRecord().id + "^" + tools_t::getINDX(esm.getKey().content);
		const auto & new_text = esm.getValue().text;

		std::string old_text;
		auto search = indx_index.find(id);
		if (search != indx_index.end())
		{
			esm_ref.selectRecord(search->second);
			esm_ref.setValue("DESC");
			old_text = esm_ref.getValue().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(id, old_text, new_text, tools_t::rec_type_t::INDX);
	}
	print_log_line(tools_t::rec_type_t::INDX);
}


void dict_creator_t::make_dict_DIAL()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "DIAL")
			continue;

		esm.setKey("DATA");
		esm.setValue("NAME");
		if (tools_t::getDialogType(esm.getKey().content) != "T")
			continue;
		if (!esm.getValue().exist)
			continue;

		const auto & id = esm.getValue().text;
		const auto & old_text = esm.getValue().text;
		insert_entry(id, old_text, "", tools_t::rec_type_t::DIAL);
	}
	print_log_line(tools_t::rec_type_t::DIAL);
}


void dict_creator_t::make_dict_FLAG()
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "NPC_")
			continue;

		esm.setKey("NAME");
		esm.setValue("FLAG");
		if (!esm.getKey().exist || !esm.getValue().exist)
			continue;

		const auto & id = esm.getKey().text;
		const auto & new_text =
			((tools_t::convertStringByteArrayToUInt(esm.getValue().content) & 0x0001) != 0)
			? "F" : "M";

		std::string old_text;
		auto search = flag_index.find(id);
		if (search != flag_index.end())
		{
			esm_ref.selectRecord(search->second);
			esm_ref.setValue("FLAG");
			old_text = ((tools_t::convertStringByteArrayToUInt(esm_ref.getValue().content) & 0x0001) != 0)
				? "F" : "M";
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(id, old_text, new_text, tools_t::rec_type_t::NPC_FLAG);
	}
	print_log_line(tools_t::rec_type_t::NPC_FLAG);
}


void dict_creator_t::make_dict_INFO()
{
	std::string key_prefix;
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id == "DIAL")
		{
			esm.setKey("DATA");
			esm.setValue("NAME");
			if (esm.getKey().exist && esm.getValue().exist)
			{
				key_prefix = tools_t::getDialogType(esm.getKey().content) + "^" +
					translate_dialog_topic(esm.getValue().text);
			}
			continue;
		}

		if (esm.getRecord().id != "INFO")
			continue;

		esm.setKey("INAM");
		if (!esm.getKey().exist)
			continue;

		esm.setValue("NAME");
		if (!esm.getValue().exist)
			continue;

		const auto & id = key_prefix + "^" + esm.getKey().text;
		const auto & new_text = esm.getValue().text;

		std::string old_text;
		auto search = info_index.find(id);
		if (search != info_index.end())
		{
			esm_ref.selectRecord(search->second);
			esm_ref.setValue("NAME");
			old_text = esm_ref.getValue().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(id, old_text, new_text, tools_t::rec_type_t::INFO);
	}
	print_log_line(tools_t::rec_type_t::INFO);
}


void dict_creator_t::make_dict_script(const ids & ids)
{
	reset_counters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != ids.rec_id)
			continue;

		esm.setKey(ids.key_id);
		esm.setValue(ids.val_id);
		if (!esm.getKey().exist || !esm.getValue().exist)
			continue;

		const auto & messages = make_script_messages(esm.getValue().text);
		for (size_t k = 0; k < messages.size(); ++k)
		{
			const auto & id = esm.getKey().text + "^" + messages.at(k);
			insert_entry(id, id, "", ids.type);
		}
	}
	print_log_line(ids.type);
}


void dict_creator_t::make_dict_CELL_Unordered()
{
	auto patterns_ext = make_dict_CELL_Unordered_PatternsExt();
	const auto & patterns = make_dict_CELL_Unordered_Patterns();

	reset_counters();
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search != patterns.end())
		{
			esm.selectRecord(search->second);
			esm.setValue("NAME");
			esm_ref.selectRecord(patterns_ext[i].pos);
			esm_ref.setValue("NAME");
			if (esm.getValue().exist &&
				esm.getValue().text != "" &&
				esm_ref.getValue().exist &&
				esm_ref.getValue().text != "")
			{
				const auto & key_text = esm_ref.getValue().text;
				const auto & val_text = esm.getValue().text;
				insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::CELL);
			}
		}
		else
		{
			patterns_ext[i].missing = true;
			counter_missing++;
		}
	}
	make_dict_CELL_Unordered_AddMissing(patterns_ext);
	print_log_line(tools_t::rec_type_t::CELL);
}


void dict_creator_t::make_dict_DIAL_Unordered()
{
	auto patterns_ext = make_dict_DIAL_Unordered_PatternsExt();
	const auto & patterns = make_dict_DIAL_Unordered_Patterns();

	reset_counters();
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search != patterns.end())
		{
			esm.selectRecord(search->second);
			esm.setValue("NAME");
			esm_ref.selectRecord(patterns_ext[i].pos);
			esm_ref.setValue("NAME");
			if (esm.getValue().exist &&
				esm_ref.getValue().exist)
			{
				const auto & key_text = esm_ref.getValue().text;
				const auto & val_text = esm.getValue().text;
				insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::DIAL);
			}
		}
		else
		{
			patterns_ext[i].missing = true;
			counter_missing++;
		}
	}
	make_dict_DIAL_Unordered_AddMissing(patterns_ext);
	print_log_line(tools_t::rec_type_t::DIAL);
}


dict_creator_t::patterns_ext_t dict_creator_t::make_dict_CELL_Unordered_PatternsExt()
{
	patterns_ext_t patterns_ext;
	for (size_t i = 0; i < esm_ref.getRecords().size(); ++i)
	{
		esm_ref.selectRecord(i);
		if (esm_ref.getRecord().id != "CELL")
			continue;

		esm_ref.setValue("NAME");
		if (esm_ref.getValue().exist &&
			esm_ref.getValue().text != "")
		{
			patterns_ext.push_back({ make_dict_CELL_Unordered_Pattern(esm_ref), i, false });
		}
	}
	return patterns_ext;
}


dict_creator_t::patterns_ext_t dict_creator_t::make_dict_DIAL_Unordered_PatternsExt()
{
	patterns_ext_t patterns_ext;
	for (size_t i = 0; i < esm_ref.getRecords().size(); ++i)
	{
		esm_ref.selectRecord(i);
		if (esm_ref.getRecord().id != "DIAL")
			continue;

		esm_ref.setKey("DATA");
		if (tools_t::getDialogType(esm_ref.getKey().content) == "T")
		{
			patterns_ext.push_back({ make_dict_DIAL_Unordered_Pattern(esm_ref, i), i, false });
		}
	}
	return patterns_ext;
}


dict_creator_t::patterns dict_creator_t::make_dict_CELL_Unordered_Patterns()
{
	patterns patterns;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "CELL")
			continue;

		esm.setValue("NAME");
		if (esm.getValue().exist &&
			esm.getValue().text != "")
		{
			patterns.insert({ make_dict_CELL_Unordered_Pattern(esm), i });
		}
	}
	return patterns;
}


dict_creator_t::patterns dict_creator_t::make_dict_DIAL_Unordered_Patterns()
{
	patterns patterns;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != "DIAL")
			continue;

		esm.setKey("DATA");
		if (tools_t::getDialogType(esm.getKey().content) == "T")
		{
			patterns.insert({ make_dict_DIAL_Unordered_Pattern(esm, i), i });
		}
	}
	return patterns;
}


std::string dict_creator_t::make_dict_CELL_Unordered_Pattern(esm_reader_t & esm_cur)
{
	std::string pattern;
	esm_cur.setValue("DATA");
	pattern += esm_cur.getValue().content;
	esm_cur.setValue("NAME");
	while (esm_cur.getValue().exist)
	{
		esm_cur.setNextValue("NAME");
		pattern += esm_cur.getValue().content;
	}
	return pattern;
}


std::string dict_creator_t::make_dict_DIAL_Unordered_Pattern(esm_reader_t & esm_cur, size_t i)
{
	std::string pattern;
	esm_cur.selectRecord(i + 1);
	esm_cur.setValue("INAM");
	pattern += esm_cur.getValue().content;
	esm_cur.setValue("SCVR");
	pattern += esm_cur.getValue().content;
	return pattern;
}


void dict_creator_t::make_dict_CELL_Unordered_AddMissing(const patterns_ext_t & patterns_ext)
{
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		if (!patterns_ext[i].missing)
			continue;

		esm_ref.selectRecord(patterns_ext[i].pos);
		esm_ref.setValue("NAME");
		if (!esm_ref.getValue().exist)
			continue;

		if (esm_ref.getValue().text == "")
			continue;

		const auto & key_text = esm_ref.getValue().text;
		const auto & val_text = "MISSING";
		insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::CELL);
		tools_t::addLog("Missing CELL: " + esm_ref.getValue().text + "\r\n");
	}
}


void dict_creator_t::make_dict_DIAL_Unordered_AddMissing(const patterns_ext_t & patterns_ext)
{
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		if (!patterns_ext[i].missing)
			continue;

		esm_ref.selectRecord(patterns_ext[i].pos);
		esm_ref.setValue("NAME");
		if (!esm_ref.getValue().exist)
			continue;

		const auto & key_text = esm_ref.getValue().text;
		const auto & val_text = "MISSING";
		insert_entry(key_text, key_text, val_text, tools_t::rec_type_t::DIAL);
		tools_t::addLog("Missing DIAL: " + esm_ref.getValue().text + "\r\n");
	}
}


void dict_creator_t::make_dict_Script_Unordered(const ids & ids)
{
	auto patterns_ext = make_dict__Unordered_PatternsExt(ids);
	const auto & patterns = make_dict__Unordered_Patterns(ids);

	reset_counters();
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search == patterns.end())
			continue;

		esm.selectRecord(search->second);
		esm.setKey(ids.key_id);
		esm.setValue(ids.val_id);
		esm_ref.selectRecord(patterns_ext[i].pos);
		esm_ref.setKey(ids.key_id);
		esm_ref.setValue(ids.val_id);
		if (esm.getKey().exist &&
			esm.getValue().exist &&
			esm_ref.getKey().exist &&
			esm_ref.getValue().exist)
		{
			const auto & message = make_script_messages(esm.getValue().text);
			const auto & message_ext = make_script_messages(esm_ref.getValue().text);
			if (message.size() != message_ext.size())
				continue;

			for (size_t k = 0; k < message.size(); ++k)
			{
				const auto & key_text = esm_ref.getKey().text + "^" + message_ext.at(k);
				const auto & val_text = esm.getKey().text + "^" + message.at(k);
				insert_entry(key_text, key_text, val_text, ids.type);
			}
		}
	}
	print_log_line(ids.type);
}


dict_creator_t::patterns_ext_t dict_creator_t::make_dict__Unordered_PatternsExt(const ids & ids)
{
	patterns_ext_t patterns_ext;
	for (size_t i = 0; i < esm_ref.getRecords().size(); ++i)
	{
		esm_ref.selectRecord(i);
		if (esm_ref.getRecord().id != ids.rec_id)
			continue;

		esm_ref.setKey(ids.key_id);
		if (!esm_ref.getKey().exist)
			continue;

		patterns_ext.push_back({ esm_ref.getKey().text, i, false });
	}
	return patterns_ext;
}


dict_creator_t::patterns dict_creator_t::make_dict__Unordered_Patterns(const ids & ids)
{
	patterns patterns;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecord().id != ids.rec_id)
			continue;

		esm.setKey(ids.key_id);
		if (!esm.getKey().exist)
			continue;

		patterns.insert({ esm.getKey().text, i });
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


void dict_creator_t::insert_entry(const std::string & id, const std::string & old_text,
							  const std::string & new_text, tools_t::rec_type_t type)
{
	counter_all++;

	tools_t::RecordEntry entry;
	entry.key_text = id;
	entry.old_text = old_text;

	if (is_make_mode)
	{
		if (!base_dict)
		{
			entry.new_text = old_text;
			entry.status = tools_t::Status::untranslated;
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
			entry.status = tools_t::Status::untranslated;
			if (dict.at(type).insert(entry))
				counter_created++;
			else
				counter_identical++;
			return;
		}

		const auto * base_entry = it->second.find(id);
		if (!base_entry)
		{
			entry.new_text = old_text;
			entry.status = tools_t::Status::untranslated;
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
				entry.status = tools_t::Status::auto_identical;
			else
				entry.status = tools_t::Status::translated;
		}
		else
		{
			entry.status = tools_t::Status::changed;
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

	auto * existing = dict.at(type).find(id);
	if (existing != nullptr && existing->new_text != new_text)
	{
		tools_t::RecordEntry doubled_entry;
		doubled_entry.key_text = id + "^DOUBLED_" + std::to_string(counter_doubled);
		doubled_entry.old_text = old_text;
		doubled_entry.new_text = new_text;
		doubled_entry.status = "";
		dict.at(type).insert(doubled_entry);
		counter_doubled++;
		counter_created++;
		if (type != tools_t::rec_type_t::Glossary)
			tools_t::addLog("Warning: Doubled " + tools_t::type2Str(type) + ": " + id + "\r\n");
	}
	else
	{
		counter_identical++;
	}
}


void dict_creator_t::print_log_line(const tools_t::rec_type_t type)
{
	std::string type_str = tools_t::type2Str(type);
	type_str.resize(12, ' ');

	std::ostringstream ss;
	ss << type_str << std::setw(5) << std::to_string(counter_created) << " / ";

	if (type == tools_t::rec_type_t::CELL ||
		type == tools_t::rec_type_t::DIAL)
	{
		ss << std::setw(7) << std::to_string(counter_missing) << " / ";
	}
	else
	{
		ss << std::setw(7) << "-" << " / ";
	}

	ss << std::setw(8) << std::to_string(counter_identical) << " / ";
	ss << std::setw(6) << std::to_string(counter_all) << "\r\n";

	tools_t::addLog(ss.str());
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
		line = tools_t::trimCR(line);
		line_lc = line;
		transform(line_lc.begin(), line_lc.end(),
				  line_lc.begin(), ::tolower);

		size_t keyword_pos;
		std::set<size_t> keyword_pos_coll;

		for (const auto & keyword : tools_t::keywords)
		{
			keyword_pos = line_lc.find(keyword);
			if (keyword_pos == std::string::npos)
				continue;
			if (keyword_pos > 0 &&
				(std::isalnum(static_cast<unsigned char>(line_lc[keyword_pos - 1])) ||
				 line_lc[keyword_pos - 1] == '_'))
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

		if (keyword_pos != std::string::npos &&
			line.rfind(";", keyword_pos) == std::string::npos &&
			line.find("\"", keyword_pos) != std::string::npos)
		{
			messages.push_back(line);
		}
	}
	return messages;
}
