#include "dict_creator.hpp"
#include "translation_engine.hpp"

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

void dict_creator_t::make_dict_base()
{
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
			esm_ref.select_record(search->second);
			esm_ref.set_key("NAME");
			esm_ref.set_value("STRV");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::gmst, tools_t::status_t::matched);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::gmst, tools_t::status_t::missing);
		}
	}
}

void dict_creator_t::make_dict_base_fnam()
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
		const auto & new_text = esm.get_value().text;

		auto search = fnam_index.find(key_text);
		if (search != fnam_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_key("NAME");
			esm_ref.set_value("FNAM");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::fnam, tools_t::status_t::matched);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::fnam, tools_t::status_t::missing);
		}
	}
}

void dict_creator_t::make_dict_base_desc()
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
		const auto & new_text = esm.get_value().text;

		auto search = desc_index.find(key_text);
		if (search != desc_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_key("NAME");
			esm_ref.set_value("DESC");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::desc, tools_t::status_t::matched);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::desc, tools_t::status_t::missing);
		}
	}
}

void dict_creator_t::make_dict_base_text()
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

		auto search = text_index.find(key_text);
		if (search != text_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_key("NAME");
			esm_ref.set_value("TEXT");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::text, tools_t::status_t::matched);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::text, tools_t::status_t::missing);
		}
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
				insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::rnam, tools_t::status_t::matched);
			}
			else
			{
				insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::rnam, tools_t::status_t::missing);
			}

			esm.set_next_value("RNAM");
		}
	}
}

void dict_creator_t::make_dict_base_indx()
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
		const auto & new_text = esm.get_value().text;

		auto search = indx_index.find(key_text);
		if (search != indx_index.end())
		{
			esm_ref.select_record(search->second);
			esm_ref.set_key("INDX");
			esm_ref.set_value("DESC");
			const auto & old_text = esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::indx, tools_t::status_t::matched);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::indx, tools_t::status_t::missing);
		}
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
				auto map_it = dial_native_to_foreign.find(native_name);
				if (map_it != dial_native_to_foreign.end())
					key_prefix = dial_type + "^" + map_it->second;
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
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::info, tools_t::status_t::matched);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, tools_t::rec_type_t::info, tools_t::status_t::missing);
		}

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

		entry->speaker_name = speaker_name;
		entry->gender = gender;
	}
}

void dict_creator_t::make_dict_base_sctx()
{
	reset_counters();

	std::unordered_map<std::string, size_t> schd_index;
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

		auto search = schd_index.find(script_name);
		if (search == schd_index.end())
		{
			tools_t::add_log("[warning] SCTX not found: \"" + script_name + "\"\r\n");
			for (const auto & msg : native_messages)
			{
				const auto key_text = script_name + "^" + msg;
				insert_entry_base(key_text, "", msg, tools_t::rec_type_t::sctx, tools_t::status_t::mismatch);
			}
			continue;
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
				insert_entry_base(key_text, "", msg, tools_t::rec_type_t::sctx, tools_t::status_t::mismatch);
			}
			continue;
		}

		const auto foreign_messages = make_script_messages(esm_ref.get_value().text);

		if (native_messages.size() != foreign_messages.size())
		{
			tools_t::add_log(
			    "[warning] SCTX line count mismatch: \"" + script_name +
			    "\" (native=" + std::to_string(native_messages.size()) +
			    ", foreign=" + std::to_string(foreign_messages.size()) + ")\r\n");

			for (const auto & msg : foreign_messages)
			{
				const auto key_text = script_name + "^" + msg;
				insert_entry_base(key_text, msg, msg, tools_t::rec_type_t::sctx, tools_t::status_t::mismatch);
			}
			continue;
		}

		for (size_t k = 0; k < native_messages.size(); ++k)
		{
			const auto key_text = script_name + "^" + foreign_messages[k];
			const auto & old_text = foreign_messages[k];
			const auto & new_text = native_messages[k];
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::sctx, tools_t::status_t::matched);
		}
	}
}

void dict_creator_t::make_dict_base_bnam()
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
				const auto & native_name = esm.get_value().text;
				auto map_it = dial_native_to_foreign.find(native_name);
				if (map_it != dial_native_to_foreign.end())
					dial_name = map_it->second;
				else
					dial_name = native_name;
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

		const auto native_messages = make_script_messages(esm.get_value().text);

		const auto info_key = dial_type + "^" + dial_name + "^" + info_inam;
		auto search = info_index.find(info_key);
		if (search == info_index.end())
		{
			tools_t::add_log("[warning] BNAM not found: \"" + info_key + "\"\r\n");
			for (const auto & msg : native_messages)
			{
				const auto key_text = info_key + "^" + msg;
				insert_entry_base(key_text, "", msg, tools_t::rec_type_t::bnam, tools_t::status_t::mismatch);
			}
			continue;
		}

		esm_ref.select_record(search->second);
		esm_ref.set_value("BNAM");
		if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
		{
			tools_t::add_log("[warning] BNAM not found: \"" + info_key + "\"\r\n");
			for (const auto & msg : native_messages)
			{
				const auto key_text = info_key + "^" + msg;
				insert_entry_base(key_text, "", msg, tools_t::rec_type_t::bnam, tools_t::status_t::mismatch);
			}
			continue;
		}

		const auto foreign_messages = make_script_messages(esm_ref.get_value().text);

		if (native_messages.size() != foreign_messages.size())
		{
			tools_t::add_log("[warning] BNAM line count mismatch: \"" + info_key + "\"\r\n");
			for (const auto & msg : foreign_messages)
			{
				const auto key_text = info_key + "^" + msg;
				insert_entry_base(key_text, msg, msg, tools_t::rec_type_t::bnam, tools_t::status_t::mismatch);
			}
			continue;
		}

		for (size_t k = 0; k < native_messages.size(); ++k)
		{
			const auto key_text = info_key + "^" + foreign_messages[k];
			const auto & old_text = foreign_messages[k];
			const auto & new_text = native_messages[k];
			insert_entry_base(key_text, old_text, new_text, tools_t::rec_type_t::bnam, tools_t::status_t::matched);
		}
	}
}

void dict_creator_t::make_dict_base_dial()
{
	const auto native_inam_index = build_dial_inam_index(esm);

	struct dial_entry_t
	{
		size_t pos;
		std::string name;
		std::string inam;
	};

	std::vector<dial_entry_t> unmatched_foreign;
	std::set<size_t> matched_native_records;

	reset_counters();
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "DIAL")
			continue;

		esm_ref.set_key("DATA");
		if (!esm_ref.get_key().exist)
			continue;

		if (tools_t::get_dialog_type(esm_ref.get_key().content) != "T")
			continue;

		esm_ref.set_value("NAME");
		if (!esm_ref.get_value().exist)
			continue;

		const auto foreign_name = esm_ref.get_value().text;

		if (i + 1 >= esm_ref.get_records().size())
		{
			unmatched_foreign.push_back({ i, foreign_name, "" });
			continue;
		}

		esm_ref.select_record(i + 1);
		if (esm_ref.get_record().id != "INFO")
		{
			unmatched_foreign.push_back({ i, foreign_name, "" });
			continue;
		}

		esm_ref.set_value("INAM");
		if (!esm_ref.get_value().exist || esm_ref.get_value().text.empty())
		{
			unmatched_foreign.push_back({ i, foreign_name, "" });
			continue;
		}

		const auto inam = esm_ref.get_value().text;

		auto match_it = native_inam_index.find(inam);
		if (match_it == native_inam_index.end())
		{
			unmatched_foreign.push_back({ i, foreign_name, inam });
			continue;
		}

		const auto & positions = match_it->second;
		if (positions.size() == 1)
		{
			const auto native_pos = *positions.begin();
			matched_native_records.insert(native_pos);

			esm.select_record(native_pos);
			esm.set_value("NAME");
			const auto & native_name = esm.get_value().text;

			dial_native_to_foreign[native_name] = foreign_name;
			insert_entry_base(
			    foreign_name, foreign_name, native_name, tools_t::rec_type_t::dial, tools_t::status_t::info);
		}
		else
		{
			unmatched_foreign.push_back({ i, foreign_name, inam });
		}
	}

	if (!translation_engine_ || !translation_engine_->is_loaded())
	{
		tools_t::add_log("Translation engine: inactive (heuristic skipped)\r\n", true);

		for (const auto & entry : unmatched_foreign)
		{
			insert_entry_base(
			    entry.name, entry.name, entry.name, tools_t::rec_type_t::dial, tools_t::status_t::missing);
		}

		for (const auto & entry : unmatched_foreign)
			tools_t::add_log("[warning] missing DIAL: " + entry.name + "\r\n");

		return;
	}

	struct native_candidate_t
	{
		size_t pos;
		std::string name;
	};

	std::vector<native_candidate_t> native_candidates;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		if (matched_native_records.count(i))
			continue;

		esm.select_record(i);
		if (esm.get_record().id != "DIAL")
			continue;

		esm.set_key("DATA");
		if (!esm.get_key().exist)
			continue;

		if (tools_t::get_dialog_type(esm.get_key().content) != "T")
			continue;

		esm.set_value("NAME");
		if (!esm.get_value().exist)
			continue;

		native_candidates.push_back({ i, esm.get_value().text });
	}

	tools_t::add_log("=== DIAL HEURISTIC START ===\r\n", true);
	tools_t::add_log("Foreign unmatched: " + std::to_string(unmatched_foreign.size()) + "\r\n", true);
	tools_t::add_log("Native unmatched: " + std::to_string(native_candidates.size()) + "\r\n", true);
	tools_t::add_log("Translation engine: active\r\n", true);

	std::set<size_t> matched_foreign_idx;
	std::set<size_t> matched_native_idx;

	for (size_t fi = 0; fi < unmatched_foreign.size(); ++fi)
	{
		const auto & foreign_name = unmatched_foreign[fi].name;
		for (size_t ni = 0; ni < native_candidates.size(); ++ni)
		{
			if (matched_native_idx.count(ni))
				continue;

			if (native_candidates[ni].name == foreign_name)
			{
				matched_foreign_idx.insert(fi);
				matched_native_idx.insert(ni);

				tools_t::add_log("[EXACT] \"" + foreign_name + "\"\r\n", true);

				dial_native_to_foreign[foreign_name] = foreign_name;
				insert_entry_base(
				    foreign_name, foreign_name, foreign_name, tools_t::rec_type_t::dial, tools_t::status_t::exact);
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
		for (size_t fi = 0; fi < unmatched_foreign.size(); ++fi)
		{
			if (matched_foreign_idx.count(fi))
				continue;

			const auto & foreign_name = unmatched_foreign[fi].name;

			auto result = translation_engine_->translate(foreign_name);
			if (!result.success)
				continue;

			const auto & translated_text = result.text;
			auto translated_words = split_words(translated_text);
			auto original_words = split_words(foreign_name);

			std::vector<std::string> compare_words = translated_words;
			for (const auto & w : original_words)
			{
				bool found = false;
				for (const auto & cw : compare_words)
				{
					if (cw == w)
					{
						found = true;
						break;
					}
				}
				if (!found)
					compare_words.push_back(w);
			}

			int best_score = 0;
			int best_score_orig = 0;
			int best_score_model = 0;
			int best_count = 0;
			size_t best_ni = 0;
			std::string best_name;

			for (size_t ni = 0; ni < native_candidates.size(); ++ni)
			{
				if (matched_native_idx.count(ni))
					continue;

				auto native_words = split_words(native_candidates[ni].name);
				int score_orig = count_shared_words(original_words, native_words);
				int score_model = count_shared_words(translated_words, native_words);
				int score = count_shared_words(compare_words, native_words);
				if (score > best_score)
				{
					best_score = score;
					best_score_orig = score_orig;
					best_score_model = score_model;
					best_count = 1;
					best_ni = ni;
					best_name = native_candidates[ni].name;
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
				for (size_t ni = 0; ni < native_candidates.size(); ++ni)
				{
					if (matched_native_idx.count(ni))
						continue;

					auto native_words = split_words(native_candidates[ni].name);
					int score = count_shared_words(compare_words, native_words);
					if (score == best_score && native_candidates[ni].name != best_name)
					{
						all_same_name = false;
						break;
					}
				}

				if (all_same_name)
				{
					resolved = true;
					tools_t::add_log(
					    "[TIE-SAME iter=" + std::to_string(iteration) + " orig=" + std::to_string(best_score_orig) +
					        " model=" + std::to_string(best_score_model) + " count=" + std::to_string(best_count) +
					        "] \"" + foreign_name + "\" -> \"" + best_name + "\"\r\n",
					    true);
				}
			}

			if (best_score > 0 && (best_count == 1 || resolved))
			{
				matched_foreign_idx.insert(fi);
				matched_native_idx.insert(best_ni);

				if (!resolved)
				{
					tools_t::add_log(
					    "[TRANSLATE iter=" + std::to_string(iteration) + " orig=" + std::to_string(best_score_orig) +
					        " model=" + std::to_string(best_score_model) + "] \"" + foreign_name + "\" -> \"" +
					        best_name + "\"\r\n",
					    true);
				}

				dial_native_to_foreign[best_name] = foreign_name;
				insert_entry_base(
				    foreign_name, foreign_name, best_name, tools_t::rec_type_t::dial, tools_t::status_t::heuristic);
				progress = true;
			}
			else if (best_score > 0 && best_count > 1 && !resolved)
			{
				tools_t::add_log(
				    "[TIE iter=" + std::to_string(iteration) + " orig=" + std::to_string(best_score_orig) +
				        " model=" + std::to_string(best_score_model) + " count=" + std::to_string(best_count) + "] \"" +
				        foreign_name + "\"\r\n",
				    true);
			}
		}
	}

	tools_t::add_log("--- UNMATCHED FOREIGN DIAL ---\r\n", true);
	for (size_t fi = 0; fi < unmatched_foreign.size(); ++fi)
	{
		if (matched_foreign_idx.count(fi))
			continue;

		const auto & name = unmatched_foreign[fi].name;
		tools_t::add_log("  " + name + "\r\n", true);

		insert_entry_base(name, name, name, tools_t::rec_type_t::dial, tools_t::status_t::missing);
	}

	tools_t::add_log("--- UNMATCHED NATIVE DIAL ---\r\n", true);
	for (size_t ni = 0; ni < native_candidates.size(); ++ni)
	{
		if (!matched_native_idx.count(ni))
			tools_t::add_log("  " + native_candidates[ni].name + "\r\n", true);
	}

	for (size_t fi = 0; fi < unmatched_foreign.size(); ++fi)
	{
		if (matched_foreign_idx.count(fi))
			continue;

		tools_t::add_log("[warning] missing DIAL: " + unmatched_foreign[fi].name + "\r\n");
	}
}

void dict_creator_t::make_dict_base_cell()
{
	make_dict_cell_exterior();
	make_dict_cell_interior();
	make_dict_cell_default();
	make_dict_cell_region();
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
					insert_entry_base(
					    old_text, old_text, new_text, tools_t::rec_type_t::cell, tools_t::status_t::wilderness);

					break;
				}
			}
			break;
		}
	}
}

void dict_creator_t::make_dict_cell_region()
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
					insert_entry_base(
					    old_text, old_text, new_text, tools_t::rec_type_t::cell, tools_t::status_t::region);

					break;
				}
			}
		}
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

dict_creator_t::fingerprint_index_t dict_creator_t::build_cell_fingerprint_index(esm_reader_t & esm_src)
{
	fingerprint_index_t index;

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

		auto & positions = index[fingerprint];
		if (!positions.empty())
		{
			esm_src.set_value("NAME");
			std::string cell_name = esm_src.get_value().exist ? esm_src.get_value().text : "<unnamed>";
			tools_t::add_log("[warning] cell index: duplicate fingerprint in CELL \"" + cell_name + "\"\r\n");
		}
		positions.insert(i);
	}

	return index;
}

dict_creator_t::fingerprint_index_t dict_creator_t::build_dial_inam_index(esm_reader_t & esm_src)
{
	fingerprint_index_t index;

	for (size_t i = 0; i < esm_src.get_records().size(); ++i)
	{
		esm_src.select_record(i);
		if (esm_src.get_record().id != "DIAL")
			continue;

		esm_src.set_key("DATA");
		if (!esm_src.get_key().exist)
			continue;

		if (tools_t::get_dialog_type(esm_src.get_key().content) != "T")
			continue;

		if (i + 1 >= esm_src.get_records().size())
			continue;

		esm_src.select_record(i + 1);
		if (esm_src.get_record().id != "INFO")
			continue;

		esm_src.set_value("INAM");
		if (!esm_src.get_value().exist)
			continue;

		const auto & inam = esm_src.get_value().text;
		if (inam.empty())
			continue;

		index[inam].insert(i);
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
			tools_t::add_log("[warning] malformed DATA in exterior cell: \"" + ref_cell_name + "\"\r\n", true);
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
		insert_entry_base(ref_cell_name, ref_cell_name, val_text, tools_t::rec_type_t::cell, tools_t::status_t::coords);
	}

	make_dict_cell_add_missing(missing_cells);
}

void dict_creator_t::make_dict_cell_interior()
{
	auto cell_index_esm = build_cell_fingerprint_index(esm);

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
			tools_t::add_log("[warning] empty fingerprint for interior cell: \"" + ref_cell_name + "\"\r\n", true);
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

		const auto & positions = match_it->second;
		if (positions.size() > 1)
		{
			missing_cells.push_back({ i, ref_cell_name });
			counter_missing++;
			continue;
		}

		const auto native_pos = *positions.begin();
		matched_native_records.insert(native_pos);

		esm.select_record(native_pos);
		esm.set_value("NAME");
		const auto & val_text = esm.get_value().text;

		insert_entry_base(
		    ref_cell_name, ref_cell_name, val_text, tools_t::rec_type_t::cell, tools_t::status_t::fingerprint);
	}

	make_dict_cell_interior_heuristic(missing_cells, matched_native_records);
	make_dict_cell_add_missing(missing_cells);
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

				insert_entry_base(
				    foreign_name, foreign_name, foreign_name, tools_t::rec_type_t::cell, tools_t::status_t::exact);
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
					if (cw == w)
					{
						found = true;
						break;
					}
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
					    "[TIE-SAME iter=" + std::to_string(iteration) + " orig=" + std::to_string(best_score_orig) +
					        " model=" + std::to_string(best_score_model) + " count=" + std::to_string(best_count) +
					        "] \"" + foreign_name + "\" -> \"" + best_name + "\"\r\n",
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
					    "[TRANSLATE iter=" + std::to_string(iteration) + " orig=" + std::to_string(best_score_orig) +
					        " model=" + std::to_string(best_score_model) + "] \"" + foreign_name + "\" -> \"" +
					        best_name + "\"\r\n",
					    true);
				}

				const auto * cell_status = resolved ? tools_t::status_t::exact : tools_t::status_t::heuristic;
				insert_entry_base(foreign_name, foreign_name, best_name, tools_t::rec_type_t::cell, cell_status);
				progress = true;
			}
			else if (best_score > 0 && best_count > 1 && !resolved)
			{
				tools_t::add_log(
				    "[TIE iter=" + std::to_string(iteration) + " orig=" + std::to_string(best_score_orig) +
				        " model=" + std::to_string(best_score_model) + " count=" + std::to_string(best_count) + "] \"" +
				        foreign_name + "\"\r\n",
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

void dict_creator_t::make_dict_cell_add_missing(const std::vector<std::pair<size_t, std::string>> & missing_cells)
{
	for (const auto & [rec_index, cell_name] : missing_cells)
	{
		insert_entry_base(cell_name, cell_name, cell_name, tools_t::rec_type_t::cell, tools_t::status_t::missing);
		tools_t::add_log("[warning] missing CELL: " + cell_name + "\r\n");
	}
}

void dict_creator_t::insert_entry_base(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    tools_t::rec_type_t type,
    const char * status)
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
	entry.status = status;

	if (dict.at(type).insert(entry))
	{
		counter_created++;
		return;
	}

	insert_duplicate(key_text, old_text, new_text, type, tools_t::status_t::duplicate);
}
