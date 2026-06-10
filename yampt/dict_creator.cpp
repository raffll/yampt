#include "dict_creator.hpp"
#include "translation_engine.hpp"

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

dict_creator_t::dict_creator_t(
    const std::string & path,
    const std::string & path_ext,
    translation_engine_t * translation_engine)
    : esm(path)
    , esm_ext(path_ext)
    , esm_ref(esm_ext)
    , is_make_mode(false)
    , translation_engine_(translation_engine)
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
	build_text_match_index();

	make_dict_gmst();
	make_dict_fnam();
	make_dict_desc();
	make_dict_text();
	make_dict_rnam();
	make_dict_indx();
	make_dict_info();

	make_dict_cell_exterior();
	make_dict_cell_interior();
	make_dict_cell_default();
	make_dict_cell_regn();

	if (is_make_mode)
	{
		make_dict_dial();
		make_dict_script({ "INFO", "INAM", "BNAM", tools_t::rec_type_t::bnam });
		make_dict_script({ "SCPT", "SCHD", "SCTX", tools_t::rec_type_t::sctx });
	}
	else
	{
		make_dict_dial_unordered();
		make_dict_script_unordered({ "INFO", "INAM", "BNAM", tools_t::rec_type_t::bnam });
		make_dict_script_unordered({ "SCPT", "SCHD", "SCTX", tools_t::rec_type_t::sctx });
	}
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
					const auto & old_text = esm_ref.get_value().text;
					const auto & new_text = esm.get_value().text;
					const auto key_text = make_cell_key_text(old_text);
					insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::cell);

					if (!is_make_mode)
					{
						auto * entry = dict.at(tools_t::rec_type_t::cell).find(key_text);
						if (entry)
							entry->status = tools_t::status_t::wilderness;
					}

					break;
				}
			}
			break;
		}
	}
	print_log_line(tools_t::rec_type_t::cell);
}

void dict_creator_t::make_dict_cell_regn()
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
					const auto & old_text = esm_ref.get_value().text;
					const auto & new_text = esm.get_value().text;
					const auto key_text = make_cell_key_text(old_text);
					insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::cell);

					if (!is_make_mode)
					{
						auto * entry = dict.at(tools_t::rec_type_t::cell).find(key_text);
						if (entry)
							entry->status = tools_t::status_t::region;
					}

					break;
				}
			}
		}
	}
	print_log_line(tools_t::rec_type_t::cell);
}

void dict_creator_t::build_indexes()
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

	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "NPC_")
			continue;
		esm_ref.set_key("NAME");
		if (!esm_ref.get_key().exist)
			continue;
		npc_index.insert({ esm_ref.get_key().text, i });
	}

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
			esm_ref.set_key("NAME");
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
		if (esm.get_value().text.empty())
			continue;

		const auto & key_text = esm.get_record().id + "^" + esm.get_key().text;
		const auto & new_text = esm.get_value().text;

		std::string old_text;
		auto search = fnam_index.find(key_text);
		if (search != fnam_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_key("NAME");
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
			esm_ref.set_key("NAME");
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
			esm_ref.set_key("NAME");
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
				old_text = search->second;
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
			esm_ref.set_key("INDX");
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
			esm_ref.set_key("INAM");
			esm_ref.set_value("NAME");
			old_text = esm_ref.get_value().text;
		}
		else
		{
			old_text = new_text;
		}

		insert_entry(key_text, old_text, new_text, tools_t::rec_type_t::info);

		esm.select_record(i);
		esm.set_value("ONAM");
		if (!esm.get_value().exist || esm.get_value().text.empty())
			continue;

		const auto & speaker_id = esm.get_value().text;
		auto npc_search = npc_index.find(speaker_id);
		if (npc_search == npc_index.end())
			continue;

		esm_ref.select_record(npc_search->second);
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

		entry->speaker = speaker_id;
		entry->speaker_name = speaker_name;
		entry->gender = gender;
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

bool dict_creator_t::is_interior_cell(const std::string & data_content)
{
	if (data_content.size() < 4)
		return false;

	unsigned char flags_byte = static_cast<unsigned char>(data_content[0]);
	return (flags_byte & 0x01) != 0;
}

std::string dict_creator_t::make_exterior_coord_key(const std::string & data_content)
{
	if (data_content.size() < 12)
		return "";

	int32_t grid_x, grid_y;
	std::memcpy(&grid_x, data_content.data() + 4, 4);
	std::memcpy(&grid_y, data_content.data() + 8, 4);

	return "GRID[" + std::to_string(grid_x) + "," + std::to_string(grid_y) + "]";
}

dict_creator_t::cell_index_t dict_creator_t::build_cell_index(
    esm_reader_t & esm_src,
    std::set<std::string> & duplicates)
{
	cell_index_t index;

	for (size_t i = 0; i < esm_src.get_records().size(); ++i)
	{
		esm_src.select_record(i);
		if (esm_src.get_record().id != "CELL")
			continue;

		esm_src.set_value("DATA");
		if (!esm_src.get_value().exist)
			continue;

		if (!is_interior_cell(esm_src.get_value().content))
			continue;

		auto fingerprint = make_cell_fingerprint(esm_src);
		if (fingerprint.empty())
			continue;

		auto result = index.insert({ fingerprint, i });
		if (!result.second)
		{
			duplicates.insert(fingerprint);
			esm_src.set_value("NAME");
			std::string cell_name = esm_src.get_value().exist ? esm_src.get_value().text : "<unnamed>";
			tools_t::add_log("[warning] cell index: duplicate fingerprint in CELL \"" + cell_name + "\"\r\n");
		}
	}

	return index;
}

std::string dict_creator_t::make_cell_fingerprint(esm_reader_t & esm_src)
{
	const auto & content = esm_src.get_record().content;
	size_t pos = 16;

	std::vector<std::string> dodts;
	std::vector<std::string> ref_ids;
	bool after_frmr = false;

	while (pos + 8 <= content.size())
	{
		std::string sub_id = content.substr(pos, 4);
		size_t sub_size = tools_t::convert_string_byte_array_to_uint(content.substr(pos + 4, 4));
		if (sub_size == 0)
			break;
		if (pos + 8 + sub_size > content.size())
			break;

		if (sub_id == "DODT" && sub_size >= 12)
			dodts.push_back(content.substr(pos + 8, 12));

		if (sub_id == "FRMR")
			after_frmr = true;
		else if (sub_id == "NAME" && after_frmr && sub_size > 0)
		{
			std::string obj_id = content.substr(pos + 8, sub_size);
			size_t null_pos = obj_id.find('\0');
			if (null_pos != std::string::npos)
				obj_id = obj_id.substr(0, null_pos);
			if (!obj_id.empty())
				ref_ids.push_back(obj_id);
			after_frmr = false;
		}

		pos += 8 + sub_size;
	}

	if (dodts.empty() && ref_ids.empty())
		return {};

	std::sort(dodts.begin(), dodts.end());
	std::sort(ref_ids.begin(), ref_ids.end());

	std::string fingerprint;
	for (const auto & d : dodts)
		fingerprint += d;
	fingerprint += '\x01';
	for (const auto & id : ref_ids)
	{
		fingerprint += id;
		fingerprint += '\0';
	}
	return fingerprint;
}

std::string dict_creator_t::make_cell_key_text(const std::string & fingerprint)
{
	uint64_t hash = 14695981039346656037ULL;
	for (unsigned char c : fingerprint)
	{
		hash ^= c;
		hash *= 1099511628211ULL;
	}

	char buf[17];
	std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(hash));
	return std::string(buf);
}

void dict_creator_t::make_dict_cell_exterior()
{
	std::unordered_map<std::string, size_t> esm_index;

	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "CELL")
			continue;

		esm.set_value("NAME");
		if (!esm.get_value().exist || esm.get_value().text.empty())
			continue;

		esm.set_value("DATA");
		if (!esm.get_value().exist)
			continue;

		if (is_interior_cell(esm.get_value().content))
			continue;

		auto coord_key = make_exterior_coord_key(esm.get_value().content);
		if (!coord_key.empty())
			esm_index.insert({ coord_key, i });
	}

	std::vector<std::pair<size_t, std::string>> missing_cells;

	reset_counters();
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "CELL")
			continue;

		esm_ref.set_value("NAME");
		if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
			continue;

		const auto ref_cell_name = esm_ref.get_value().text;

		esm_ref.set_value("DATA");
		if (!esm_ref.get_value().exist)
		{
			missing_cells.push_back({ i, ref_cell_name });
			counter_missing++;
			continue;
		}

		if (is_interior_cell(esm_ref.get_value().content))
			continue;

		auto coord_key = make_exterior_coord_key(esm_ref.get_value().content);
		if (coord_key.empty())
		{
			missing_cells.push_back({ i, ref_cell_name });
			counter_missing++;
			continue;
		}

		auto match_it = esm_index.find(coord_key);
		if (match_it == esm_index.end())
		{
			missing_cells.push_back({ i, ref_cell_name });
			counter_missing++;
			continue;
		}

		esm.select_record(match_it->second);
		esm.set_value("NAME");
		const auto & val_text = esm.get_value().text;
		const auto key_text = make_cell_key_text(coord_key);
		insert_entry(key_text, ref_cell_name, val_text, tools_t::rec_type_t::cell);

		if (!is_make_mode)
		{
			auto * entry = dict.at(tools_t::rec_type_t::cell).find(key_text);
			if (entry && entry->status.empty())
				entry->status = tools_t::status_t::matched_by_coords;
		}
	}

	make_dict_cell_add_missing(missing_cells);
	print_log_line(tools_t::rec_type_t::cell);
}

void dict_creator_t::make_dict_cell_interior()
{
	std::set<std::string> duplicates;
	auto cell_index_esm = build_cell_index(esm, duplicates);

	std::unordered_map<std::string, int> dup_counter;
	std::set<size_t> matched_native_records;
	std::vector<std::pair<size_t, std::string>> missing_cells;

	reset_counters();
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "CELL")
			continue;

		esm_ref.set_value("NAME");
		if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
			continue;

		const auto ref_cell_name = esm_ref.get_value().text;

		esm_ref.set_value("DATA");
		if (!esm_ref.get_value().exist)
			continue;

		if (!is_interior_cell(esm_ref.get_value().content))
			continue;

		auto fingerprint = make_cell_fingerprint(esm_ref);
		if (fingerprint.empty())
		{
			missing_cells.push_back({ i, ref_cell_name });
			counter_missing++;
			continue;
		}

		auto match_it = cell_index_esm.find(fingerprint);
		if (match_it == cell_index_esm.end())
		{
			missing_cells.push_back({ i, ref_cell_name });
			counter_missing++;
			continue;
		}

		esm.select_record(match_it->second);
		esm.set_value("NAME");
		const auto & val_text = esm.get_value().text;

		auto key_text = make_cell_key_text(fingerprint);
		if (duplicates.count(fingerprint))
		{
			key_text += "^DUP_" + std::to_string(dup_counter[fingerprint]++);
			insert_entry(key_text, ref_cell_name, ref_cell_name, tools_t::rec_type_t::cell);

			auto * entry = dict.at(tools_t::rec_type_t::cell).find(key_text);
			if (entry)
				entry->status = tools_t::status_t::duplicate;
			continue;
		}

		matched_native_records.insert(match_it->second);
		insert_entry(key_text, ref_cell_name, val_text, tools_t::rec_type_t::cell);

		if (!is_make_mode)
		{
			auto * entry = dict.at(tools_t::rec_type_t::cell).find(key_text);
			if (entry && entry->status.empty())
				entry->status = tools_t::status_t::matched_by_fingerprint;
		}
	}

	make_dict_cell_interior_heuristic(missing_cells, matched_native_records);
	make_dict_cell_add_missing(missing_cells);
	print_log_line(tools_t::rec_type_t::cell);
}

static std::vector<std::string> split_words(const std::string & name)
{
	std::vector<std::string> words;
	std::string word;
	for (char c : name)
	{
		if (std::isalnum(static_cast<unsigned char>(c)))
			word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		else
		{
			if (!word.empty())
			{
				words.push_back(word);
				word.clear();
			}
		}
	}
	if (!word.empty())
		words.push_back(word);
	return words;
}

static int count_shared_words(const std::vector<std::string> & a, const std::vector<std::string> & b)
{
	int count = 0;
	for (const auto & w : a)
	{
		for (const auto & w2 : b)
		{
			if (w == w2)
			{
				count++;
				break;
			}
		}
	}
	return count;
}

void dict_creator_t::make_dict_cell_interior_heuristic(
    std::vector<std::pair<size_t, std::string>> & missing_cells,
    const std::set<size_t> & matched_native_records)
{
	std::vector<std::pair<size_t, std::string>> native_cells;

	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		if (matched_native_records.count(i))
			continue;

		esm.select_record(i);
		if (esm.get_record().id != "CELL")
			continue;

		esm.set_value("DATA");
		if (!esm.get_value().exist)
			continue;

		if (!is_interior_cell(esm.get_value().content))
			continue;

		esm.set_value("NAME");
		if (!esm.get_value().exist || esm.get_value().text.empty())
			continue;

		native_cells.push_back({ i, esm.get_value().text });
	}

	tools_t::add_log("=== HEURISTIC START ===\r\n", true);
	tools_t::add_log("Foreign missing: " + std::to_string(missing_cells.size()) + "\r\n", true);
	tools_t::add_log("Native unmatched: " + std::to_string(native_cells.size()) + "\r\n", true);
	if (translation_engine_ && translation_engine_->is_loaded())
		tools_t::add_log("Translation engine: active\r\n", true);
	else
		tools_t::add_log("Translation engine: inactive (heuristic skipped)\r\n", true);

	std::set<size_t> matched_native;
	std::set<size_t> matched_foreign;
	std::set<std::string> matched_native_names;

	for (size_t fi = 0; fi < missing_cells.size(); ++fi)
	{
		const auto & foreign_name = missing_cells[fi].second;
		for (size_t ni = 0; ni < native_cells.size(); ++ni)
		{
			if (matched_native.count(ni))
				continue;

			if (native_cells[ni].second == foreign_name)
			{
				matched_foreign.insert(fi);
				matched_native.insert(ni);
				matched_native_names.insert(foreign_name);

				tools_t::add_log("[EXACT] \"" + foreign_name + "\"\r\n", true);

				auto key_text = make_cell_key_text(foreign_name);
				insert_entry(key_text, foreign_name, foreign_name, tools_t::rec_type_t::cell);

				if (!is_make_mode)
				{
					auto * entry = dict.at(tools_t::rec_type_t::cell).find(key_text);
					if (entry && entry->status.empty())
						entry->status = tools_t::status_t::matched_by_name;
				}
				break;
			}
		}
	}

	int iteration = 0;
	bool progress = true;
	while (progress)
	{
		progress = false;
		iteration++;
		for (size_t fi = 0; fi < missing_cells.size(); ++fi)
		{
			if (matched_foreign.count(fi))
				continue;

			const auto & foreign_name = missing_cells[fi].second;
			std::vector<std::string> compare_words;
			bool used_translation = false;
			std::string translated_text;

			if (!translation_engine_ || !translation_engine_->is_loaded())
				continue;

			auto result = translation_engine_->translate(foreign_name);
			if (!result.success)
				continue;

			translated_text = result.text;
			auto translated_words = split_words(translated_text);
			auto original_words = split_words(foreign_name);
			compare_words = translated_words;
			for (const auto & w : original_words)
			{
				bool found = false;
				for (const auto & cw : compare_words)
				{
					if (cw == w) { found = true; break; }
				}
				if (!found)
					compare_words.push_back(w);
			}
			used_translation = true;

			int best_score = 0;
			int best_score_orig = 0;
			int best_score_model = 0;
			int best_count = 0;
			size_t best_ni = 0;
			std::string best_name;

			for (size_t ni = 0; ni < native_cells.size(); ++ni)
			{
				if (matched_native.count(ni))
					continue;

				auto esm_words = split_words(native_cells[ni].second);
				int score_orig = count_shared_words(original_words, esm_words);
				int score_model = count_shared_words(translated_words, esm_words);
				int score = count_shared_words(compare_words, esm_words);
				if (score > best_score)
				{
					best_score = score;
					best_score_orig = score_orig;
					best_score_model = score_model;
					best_count = 1;
					best_ni = ni;
					best_name = native_cells[ni].second;
				}
				else if (score == best_score && score > 0)
				{
					best_count++;
				}
			}

			bool resolved = false;

			if (best_score > 0 && best_count > 1)
			{
				bool all_same_name = true;
				for (size_t ni = 0; ni < native_cells.size(); ++ni)
				{
					if (matched_native.count(ni))
						continue;

					auto esm_words = split_words(native_cells[ni].second);
					int score = count_shared_words(compare_words, esm_words);
					if (score == best_score && native_cells[ni].second != best_name)
					{
						all_same_name = false;
						break;
					}
				}

				if (all_same_name)
				{
					resolved = true;
					tools_t::add_log(
					    "[TIE-SAME iter=" + std::to_string(iteration) +
					        " orig=" + std::to_string(best_score_orig) +
					        " model=" + std::to_string(best_score_model) +
					        " count=" + std::to_string(best_count) + "] \"" + foreign_name + "\" -> \"" + best_name +
					        "\"\r\n",
					    true);
				}
			}

			if (best_score > 0 && (best_count == 1 || resolved))
			{
				matched_foreign.insert(fi);
				matched_native.insert(best_ni);

				if (!resolved)
				{
					tools_t::add_log(
					    "[TRANSLATE iter=" + std::to_string(iteration) +
					        " orig=" + std::to_string(best_score_orig) +
					        " model=" + std::to_string(best_score_model) +
					        "] \"" + foreign_name + "\" -> \"" + best_name + "\"\r\n",
					    true);
				}

				auto key_text = make_cell_key_text(foreign_name);
				insert_entry(key_text, foreign_name, best_name, tools_t::rec_type_t::cell);

				if (!is_make_mode)
				{
					auto * entry = dict.at(tools_t::rec_type_t::cell).find(key_text);
					if (entry && entry->status.empty())
						entry->status =
						    resolved ? tools_t::status_t::matched_by_name : tools_t::status_t::matched_by_heuristic;
				}
				progress = true;
			}
			else if (best_score > 0 && best_count > 1 && !resolved)
			{
				tools_t::add_log(
				    "[TIE iter=" + std::to_string(iteration) +
				        " orig=" + std::to_string(best_score_orig) +
				        " model=" + std::to_string(best_score_model) +
				        " count=" + std::to_string(best_count) + "] \"" + foreign_name + "\"\r\n",
				    true);
			}
		}
	}

	tools_t::add_log("--- UNMATCHED FOREIGN ---\r\n", true);
	for (size_t fi = 0; fi < missing_cells.size(); ++fi)
	{
		if (!matched_foreign.count(fi))
			tools_t::add_log("  " + missing_cells[fi].second + "\r\n", true);
	}

	tools_t::add_log("--- UNMATCHED NATIVE ---\r\n", true);
	for (size_t ni = 0; ni < native_cells.size(); ++ni)
	{
		if (!matched_native.count(ni) && !matched_native_names.count(native_cells[ni].second))
			tools_t::add_log("  " + native_cells[ni].second + "\r\n", true);
	}

	std::vector<std::pair<size_t, std::string>> still_missing;
	for (size_t fi = 0; fi < missing_cells.size(); ++fi)
	{
		if (!matched_foreign.count(fi))
			still_missing.push_back(missing_cells[fi]);
	}
	missing_cells = std::move(still_missing);
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

				auto * entry = dict.at(tools_t::rec_type_t::dial).find(key_text);
				if (entry && entry->status.empty())
					entry->status = tools_t::status_t::matched_by_info;
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

void dict_creator_t::make_dict_cell_add_missing(const std::vector<std::pair<size_t, std::string>> & missing_cells)
{
	for (const auto & [rec_index, cell_name] : missing_cells)
	{
		const auto key_text = make_cell_key_text(cell_name);
		insert_entry(key_text, cell_name, cell_name, tools_t::rec_type_t::cell);

		if (!is_make_mode)
		{
			auto * entry = dict.at(tools_t::rec_type_t::cell).find(key_text);
			if (entry)
				entry->status = tools_t::status_t::missing;

			tools_t::add_log("[warning] missing CELL: " + cell_name + "\r\n");
		}
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
		insert_entry(key_text, key_text, key_text, tools_t::rec_type_t::dial);

		auto * entry = dict.at(tools_t::rec_type_t::dial).find(key_text);
		if (entry)
			entry->status = tools_t::status_t::missing;

		tools_t::add_log("[warning] missing DIAL: " + esm_ref.get_value().text + "\r\n");
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

				auto * entry = dict.at(ids.type).find(key_text);
				if (entry && entry->status.empty())
					entry->status = tools_t::status_t::matched_by_name;
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

static bool is_number_or_punct(char c)
{
	return (c >= '0' && c <= '9') || c == '.' || c == ',' || c == '-' || c == ':' || c == ';' || c == '!' || c == '?';
}

bool dict_creator_t::differs_only_in_numbers_or_punct(const std::string & a, const std::string & b)
{
	if (a.size() != b.size())
		return false;

	bool has_difference = false;

	for (size_t i = 0; i < a.size(); ++i)
	{
		if (a[i] == b[i])
			continue;

		if (!is_number_or_punct(a[i]) && !is_number_or_punct(b[i]))
			return false;

		has_difference = true;
	}

	return has_difference;
}

std::string dict_creator_t::adapt_translation(
    const std::string & source,
    const std::string & matched_source,
    const std::string & matched_translation)
{
	if (source.size() != matched_source.size())
		return matched_translation;

	std::string result = matched_translation;

	for (size_t i = 0; i < source.size() && i < matched_source.size(); ++i)
	{
		if (source[i] == matched_source[i])
			continue;

		size_t search_start = 0;
		auto pos = result.find(matched_source[i], search_start);
		bool replaced = false;

		while (pos != std::string::npos)
		{
			if (is_number_or_punct(result[pos]))
			{
				result[pos] = source[i];
				replaced = true;
				break;
			}

			search_start = pos + 1;
			pos = result.find(matched_source[i], search_start);
		}

		if (!replaced)
		{
			for (size_t j = 0; j < result.size(); ++j)
			{
				if (result[j] != matched_source[i])
					continue;
				result[j] = source[i];
				break;
			}
		}
	}

	return result;
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

	if (!is_make_mode)
	{
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
			doubled_entry.key_text = key_text + "^DUP_" + std::to_string(counter_doubled);
			doubled_entry.old_text = old_text;
			doubled_entry.new_text = new_text;
			doubled_entry.status = tools_t::status_t::duplicate;
			dict.at(type).insert(doubled_entry);
			counter_doubled++;
			counter_created++;
			tools_t::add_log("[warning] doubled " + tools_t::type_to_str(type) + ": " + key_text + "\r\n");
		}
		else
		{
			counter_identical++;
		}
		return;
	}

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

	if (base_entry && base_entry->old_text == old_text && base_entry->new_text == old_text)
	{
		entry.new_text = old_text;
		entry.status = tools_t::status_t::auto_identical;
		if (dict.at(type).insert(entry))
			counter_created++;
		else
			counter_identical++;
		return;
	}

	if (base_entry && base_entry->old_text == old_text && base_entry->new_text != old_text)
	{
		entry.new_text = base_entry->new_text;
		entry.status = tools_t::status_t::auto_base;
		if (dict.at(type).insert(entry))
			counter_created++;
		else
			counter_identical++;
		return;
	}

	if (!base_entry)
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
				entry.new_text = translated_match->new_text;
				entry.status = tools_t::status_t::auto_translated;
				if (dict.at(type).insert(entry))
					counter_created++;
				else
					counter_identical++;
				return;
			}
		}

		entry.new_text = old_text;
		entry.status = tools_t::status_t::untranslated;
		if (dict.at(type).insert(entry))
			counter_created++;
		else
			counter_identical++;
		return;
	}

	if (base_entry->old_text != old_text)
	{
		if (differs_only_in_numbers_or_punct(old_text, base_entry->old_text))
		{
			std::string adapted = adapt_translation(old_text, base_entry->old_text, base_entry->new_text);
			entry.new_text = adapted;
			entry.status = tools_t::status_t::auto_heuristic;
			if (dict.at(type).insert(entry))
				counter_created++;
			else
				counter_identical++;
			return;
		}

		entry.new_text = base_entry->new_text;
		entry.status = tools_t::status_t::auto_changed;
		if (dict.at(type).insert(entry))
			counter_created++;
		else
			counter_identical++;
		return;
	}

	entry.new_text = old_text;
	entry.status = tools_t::status_t::untranslated;
	if (dict.at(type).insert(entry))
		counter_created++;
	else
		counter_identical++;
}

void dict_creator_t::print_log_line(const tools_t::rec_type_t type)
{
	std::string line = "[info] " + tools_t::type_to_str(type) + ": created=" + std::to_string(counter_created);

	if (type == tools_t::rec_type_t::cell || type == tools_t::rec_type_t::dial)
		line += ", missing=" + std::to_string(counter_missing);

	line += ", identical=" + std::to_string(counter_identical);

	if (counter_doubled > 0)
		line += ", duplicate=" + std::to_string(counter_doubled);

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
