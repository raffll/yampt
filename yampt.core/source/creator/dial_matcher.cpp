#include "dial_matcher.hpp"
#include "../utility/app_logger.hpp"
#include "../translator/translation_engine.hpp"
#include "word_match_utils.hpp"

dial_matcher_t::dial_matcher_t(
    esm_reader_t & esm_native,
    esm_reader_t & esm_foreign,
    translation_engine_t * translation_engine,
    dict_t & output_dict,
    determine_status_fn determine_status)
    : m_esm_native(esm_native)
    , m_esm_foreign(esm_foreign)
    , m_translation_engine(translation_engine)
    , m_output_dict(output_dict)
    , m_determine_status(std::move(determine_status))
{}

const std::unordered_map<std::string, std::string> & dial_matcher_t::get_native_to_foreign() const
{
	return m_native_to_foreign;
}

const dial_matcher_t::counters_t & dial_matcher_t::get_counters() const
{
	return m_counters;
}

void dial_matcher_t::insert_entry(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    status_t status)
{
	auto * existing = m_output_dict[rec_type_t::dial].find(key_text);
	if (existing && existing->old_text == old_text && existing->new_text == new_text)
		return;

	record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = status;

	if (m_output_dict[rec_type_t::dial].insert(entry))
	{
		m_counters.created++;
		return;
	}

	m_counters.doubled++;
}

dial_matcher_t::fingerprint_index_t dial_matcher_t::build_inam_index(esm_reader_t & esm_source)
{
	fingerprint_index_t index;

	for (size_t i = 0; i < esm_source.get_records().size(); ++i)
	{
		esm_source.select_record(i);
		if (esm_source.get_record().id != "DIAL")
			continue;

		esm_source.set_key("DATA");
		if (!esm_source.get_key().exist)
			continue;

		if (domain_types_t::get_dialog_type(esm_source.get_key().content) != "T")
			continue;

		if (i + 1 >= esm_source.get_records().size())
			continue;

		esm_source.select_record(i + 1);
		if (esm_source.get_record().id != "INFO")
			continue;

		esm_source.set_value("INAM");
		if (!esm_source.get_value().exist)
			continue;

		const auto & inam = esm_source.get_value().text;
		if (inam.empty())
			continue;

		index[inam].insert(i);
	}

	return index;
}

void dial_matcher_t::match_topics()
{
	const auto native_inam_index = build_inam_index(m_esm_native);

	std::vector<std::pair<size_t, std::string>> unmatched_foreign;
	std::set<size_t> matched_native_records;

	match_by_inam(native_inam_index, unmatched_foreign, matched_native_records);

	if (!m_translation_engine || !m_translation_engine->is_loaded())
	{
		report_unmatched(unmatched_foreign, matched_native_records);
		return;
	}

	match_by_translation(unmatched_foreign, matched_native_records);
}

void dial_matcher_t::match_by_inam(
    const fingerprint_index_t & native_inam_index,
    std::vector<std::pair<size_t, std::string>> & unmatched_foreign,
    std::set<size_t> & matched_native_records)
{
	for (size_t i = 0; i < m_esm_foreign.get_records().size(); ++i)
	{
		m_esm_foreign.select_record(i);
		if (m_esm_foreign.get_record().id != "DIAL")
			continue;

		m_esm_foreign.set_key("DATA");
		if (!m_esm_foreign.get_key().exist)
			continue;

		if (domain_types_t::get_dialog_type(m_esm_foreign.get_key().content) != "T")
			continue;

		m_esm_foreign.set_value("NAME");
		if (!m_esm_foreign.get_value().exist)
			continue;

		const auto foreign_name = m_esm_foreign.get_value().text;

		if (i + 1 >= m_esm_foreign.get_records().size())
		{
			unmatched_foreign.push_back({ i, foreign_name });
			continue;
		}

		m_esm_foreign.select_record(i + 1);
		if (m_esm_foreign.get_record().id != "INFO")
		{
			unmatched_foreign.push_back({ i, foreign_name });
			continue;
		}

		m_esm_foreign.set_value("INAM");
		if (!m_esm_foreign.get_value().exist || m_esm_foreign.get_value().text.empty())
		{
			unmatched_foreign.push_back({ i, foreign_name });
			continue;
		}

		const auto inam = m_esm_foreign.get_value().text;

		auto it_match = native_inam_index.find(inam);
		if (it_match == native_inam_index.end())
		{
			unmatched_foreign.push_back({ i, foreign_name });
			continue;
		}

		const auto & positions = it_match->second;
		if (positions.size() != 1)
		{
			unmatched_foreign.push_back({ i, foreign_name });
			continue;
		}

		const auto native_pos = *positions.begin();
		matched_native_records.insert(native_pos);

		m_esm_native.select_record(native_pos);
		m_esm_native.set_value("NAME");
		const auto & native_name = m_esm_native.get_value().text;

		m_native_to_foreign[native_name] = foreign_name;
		insert_entry(foreign_name, foreign_name, native_name, status_t::translated);
	}
}

void dial_matcher_t::match_by_translation(
    std::vector<std::pair<size_t, std::string>> & unmatched_foreign,
    const std::set<size_t> & matched_native_records)
{
	std::vector<std::pair<size_t, std::string>> native_candidates;
	for (size_t i = 0; i < m_esm_native.get_records().size(); ++i)
	{
		if (matched_native_records.count(i))
			continue;

		m_esm_native.select_record(i);
		if (m_esm_native.get_record().id != "DIAL")
			continue;

		m_esm_native.set_key("DATA");
		if (!m_esm_native.get_key().exist)
			continue;

		if (domain_types_t::get_dialog_type(m_esm_native.get_key().content) != "T")
			continue;

		m_esm_native.set_value("NAME");
		if (!m_esm_native.get_value().exist)
			continue;

		native_candidates.push_back({ i, m_esm_native.get_value().text });
	}

	app_logger_t::add_log("=== DIAL HEURISTIC START ===\r\n", true);
	app_logger_t::add_log("Foreign unmatched: " + std::to_string(unmatched_foreign.size()) + "\r\n", true);
	app_logger_t::add_log("Native unmatched: " + std::to_string(native_candidates.size()) + "\r\n", true);
	app_logger_t::add_log("[info] translation engine: active (DIAL heuristic)\r\n");

	std::set<size_t> matched_foreign_idx;
	std::set<size_t> matched_native_idx;

	for (size_t fi = 0; fi < unmatched_foreign.size(); ++fi)
	{
		const auto & foreign_name = unmatched_foreign[fi].second;
		for (size_t ni = 0; ni < native_candidates.size(); ++ni)
		{
			if (matched_native_idx.count(ni))
				continue;

			if (native_candidates[ni].second != foreign_name)
				continue;

			matched_foreign_idx.insert(fi);
			matched_native_idx.insert(ni);

			app_logger_t::add_log("[EXACT] \"" + foreign_name + "\"\r\n", true);

			m_native_to_foreign[foreign_name] = foreign_name;
			insert_entry(foreign_name, foreign_name, foreign_name, status_t::translated);
			break;
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

			const auto & foreign_name = unmatched_foreign[fi].second;

			auto result = m_translation_engine->translate(foreign_name);
			if (!result.success)
				continue;

			const auto & translated_text = result.text;
			auto translated_words = split_words(translated_text);
			auto original_words = split_words(foreign_name);
			auto compare_words = build_compare_words(translated_words, original_words);

			auto match = compute_best_match(
			    compare_words, original_words, translated_words, native_candidates, matched_native_idx);

			if (match.score <= 0)
				continue;

			bool resolved = false;
			if (match.count > 1)
			{
				resolved = check_all_same_name(compare_words, native_candidates, matched_native_idx, match);

				if (resolved)
				{
					app_logger_t::add_log(
					    "[TIE-SAME iter=" + std::to_string(iteration) + " orig=" + std::to_string(match.score_orig) +
					    " model=" + std::to_string(match.score_model) + " count=" + std::to_string(match.count) +
					    "] \"" + foreign_name + "\" => \"" + translated_text + "\" -> \"" + match.name + "\"\r\n");
				}
			}

			if (match.count == 1 || resolved)
			{
				matched_foreign_idx.insert(fi);
				matched_native_idx.insert(match.index);

				if (!resolved)
				{
					app_logger_t::add_log(
					    "[TRANSLATE iter=" + std::to_string(iteration) + " orig=" + std::to_string(match.score_orig) +
					    " model=" + std::to_string(match.score_model) + "] \"" + foreign_name + "\" => \"" +
					    translated_text + "\" -> \"" + match.name + "\"\r\n");
				}

				m_native_to_foreign[match.name] = foreign_name;
				insert_entry(foreign_name, foreign_name, match.name, status_t::heuristic);
				progress = true;
			}
			else if (!resolved)
			{
				app_logger_t::add_log(
				    "[TIE iter=" + std::to_string(iteration) + " orig=" + std::to_string(match.score_orig) +
				    " model=" + std::to_string(match.score_model) + " count=" + std::to_string(match.count) + "] \"" +
				    foreign_name + "\"\r\n");
			}
		}
	}

	app_logger_t::add_log("--- UNMATCHED FOREIGN DIAL ---\r\n", true);

	std::vector<std::string> unmatched_native_names;
	for (size_t ni = 0; ni < native_candidates.size(); ++ni)
	{
		if (!matched_native_idx.count(ni))
			unmatched_native_names.push_back(native_candidates[ni].second);
	}

	std::string candidates_str;
	for (const auto & name : unmatched_native_names)
	{
		if (!candidates_str.empty())
			candidates_str += "|";

		candidates_str += name;
	}

	for (size_t fi = 0; fi < unmatched_foreign.size(); ++fi)
	{
		if (matched_foreign_idx.count(fi))
			continue;

		const auto & name = unmatched_foreign[fi].second;
		app_logger_t::add_log("  " + name + "\r\n", true);

		insert_entry(name, name, name, status_t::missing);
		m_counters.missing++;

		if (!candidates_str.empty())
		{
			auto * entry = m_output_dict[rec_type_t::dial].find(name);
			if (entry)
				entry->details = candidates_str;
		}
	}

	app_logger_t::add_log("--- UNMATCHED NATIVE DIAL ---\r\n", true);
	for (size_t ni = 0; ni < native_candidates.size(); ++ni)
	{
		if (!matched_native_idx.count(ni))
			app_logger_t::add_log("  " + native_candidates[ni].second + "\r\n", true);
	}

	for (size_t fi = 0; fi < unmatched_foreign.size(); ++fi)
	{
		if (matched_foreign_idx.count(fi))
			continue;

		app_logger_t::add_log("[warning] missing DIAL: " + unmatched_foreign[fi].second + "\r\n");
	}

	if (!unmatched_native_names.empty())
	{
		app_logger_t::add_log(
		    "[info] unmatched native DIAL candidates (" + std::to_string(unmatched_native_names.size()) + "):\r\n");
		for (const auto & name : unmatched_native_names)
			app_logger_t::add_log("  " + name + "\r\n");
	}
}

void dial_matcher_t::report_unmatched(
    const std::vector<std::pair<size_t, std::string>> & unmatched_foreign,
    const std::set<size_t> & matched_native_records)
{
	app_logger_t::add_log("[info] translation engine: inactive (DIAL heuristic skipped)\r\n");

	std::vector<std::string> native_names;
	for (size_t i = 0; i < m_esm_native.get_records().size(); ++i)
	{
		if (matched_native_records.count(i))
			continue;

		m_esm_native.select_record(i);
		if (m_esm_native.get_record().id != "DIAL")
			continue;

		m_esm_native.set_key("DATA");
		if (!m_esm_native.get_key().exist)
			continue;

		if (domain_types_t::get_dialog_type(m_esm_native.get_key().content) != "T")
			continue;

		m_esm_native.set_value("NAME");
		if (!m_esm_native.get_value().exist)
			continue;

		native_names.push_back(m_esm_native.get_value().text);
	}

	std::string candidates_str;
	for (const auto & name : native_names)
	{
		if (!candidates_str.empty())
			candidates_str += "|";

		candidates_str += name;
	}

	for (const auto & [pos, name] : unmatched_foreign)
	{
		insert_entry(name, name, name, status_t::missing);
		m_counters.missing++;

		if (!candidates_str.empty())
		{
			auto * entry = m_output_dict[rec_type_t::dial].find(name);
			if (entry)
				entry->details = candidates_str;
		}
	}

	for (const auto & [pos, name] : unmatched_foreign)
		app_logger_t::add_log("[warning] missing DIAL: " + name + "\r\n");

	if (!native_names.empty())
	{
		app_logger_t::add_log("[info] unmatched native DIAL candidates (" + std::to_string(native_names.size()) + "):\r\n");
		for (const auto & name : native_names)
			app_logger_t::add_log("  " + name + "\r\n");
	}
}
