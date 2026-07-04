#include "cell_matcher.hpp"
#include "word_match_utils.hpp"
#include "../translator/translation_engine.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

cell_matcher_t::cell_matcher_t(
    esm_reader_t & esm_native,
    esm_reader_t & esm_foreign,
    translation_engine_t * translation_engine,
    tools_t::dict_t & output_dict,
    determine_status_fn determine_status)
    : m_esm_native(esm_native)
    , m_esm_foreign(esm_foreign)
    , m_translation_engine(translation_engine)
    , m_output_dict(output_dict)
    , m_determine_status(std::move(determine_status))
{
}

const cell_matcher_t::counters_t & cell_matcher_t::get_counters() const
{
	return m_counters;
}

void cell_matcher_t::insert_entry(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    status_t status)
{
	auto * existing = m_output_dict[tools_t::rec_type_t::cell].find(key_text);
	if (existing)
	{
		if (existing->old_text == old_text && existing->new_text == new_text)
			return;
	}

	tools_t::record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = status;

	if (m_output_dict[tools_t::rec_type_t::cell].insert(entry))
	{
		m_counters.created++;
		if (old_text == new_text && status == status_t::translated)
			m_counters.identical++;

		return;
	}

	auto * duplicate = m_output_dict[tools_t::rec_type_t::cell].find(key_text);
	if (duplicate && duplicate->old_text == old_text && duplicate->new_text == new_text)
		return;

	m_counters.doubled++;
}

bool cell_matcher_t::is_interior_cell(const std::string & data_content)
{
	if (data_content.size() < 4)
		return false;

	unsigned char flags_byte = static_cast<unsigned char>(data_content[0]);
	return (flags_byte & 0x01) != 0;
}

std::string cell_matcher_t::make_exterior_coord_key(const std::string & data_content)
{
	if (data_content.size() < 12)
		return "";

	int32_t grid_x, grid_y;
	std::memcpy(&grid_x, data_content.data() + 4, 4);
	std::memcpy(&grid_y, data_content.data() + 8, 4);

	return "GRID[" + std::to_string(grid_x) + "," + std::to_string(grid_y) + "]";
}

std::string cell_matcher_t::make_cell_key_text(const std::string & fingerprint)
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

std::string cell_matcher_t::make_cell_fingerprint(esm_reader_t & esm_src)
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
	for (const auto & dodt : dodts)
		fingerprint += dodt;

	fingerprint += '\x01';
	for (const auto & id : ref_ids)
	{
		fingerprint += id;
		fingerprint += '\0';
	}
	return fingerprint;
}

cell_matcher_t::fingerprint_index_t cell_matcher_t::build_cell_fingerprint_index(esm_reader_t & esm_src)
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
			tools_t::add_log(
			    "[warning] cell index: duplicate fingerprint in CELL \"" + cell_name + "\"\r\n");
		}
		positions.insert(i);
	}

	return index;
}

void cell_matcher_t::match_exterior_cells()
{
	std::unordered_map<std::string, size_t> native_coord_index;

	for (size_t i = 0; i < m_esm_native.get_records().size(); ++i)
	{
		m_esm_native.select_record(i);
		if (m_esm_native.get_record().id != "CELL")
			continue;

		m_esm_native.set_value("NAME");
		if (!m_esm_native.get_value().exist || m_esm_native.get_value().text.empty())
			continue;

		m_esm_native.set_value("DATA");
		if (!m_esm_native.get_value().exist)
			continue;

		if (is_interior_cell(m_esm_native.get_value().content))
			continue;

		auto coord_key = make_exterior_coord_key(m_esm_native.get_value().content);
		if (!coord_key.empty())
			native_coord_index.insert({ coord_key, i });
	}

	std::vector<std::pair<size_t, std::string>> missing_cells;

	for (size_t i = 0; i < m_esm_foreign.get_records().size(); ++i)
	{
		m_esm_foreign.select_record(i);
		if (m_esm_foreign.get_record().id != "CELL")
			continue;

		m_esm_foreign.set_value("NAME");
		if (!m_esm_foreign.get_value().exist || m_esm_foreign.get_value().text.empty())
			continue;

		const auto ref_cell_name = m_esm_foreign.get_value().text;

		m_esm_foreign.set_value("DATA");
		if (!m_esm_foreign.get_value().exist)
		{
			missing_cells.push_back({ i, ref_cell_name });
			m_counters.missing++;
			continue;
		}

		if (is_interior_cell(m_esm_foreign.get_value().content))
			continue;

		auto coord_key = make_exterior_coord_key(m_esm_foreign.get_value().content);
		if (coord_key.empty())
		{
			tools_t::add_log(
			    "[warning] malformed DATA in exterior cell: \"" + ref_cell_name + "\"\r\n", true);
			missing_cells.push_back({ i, ref_cell_name });
			m_counters.missing++;
			continue;
		}

		auto it_match = native_coord_index.find(coord_key);
		if (it_match == native_coord_index.end())
		{
			missing_cells.push_back({ i, ref_cell_name });
			m_counters.missing++;
			continue;
		}

		m_esm_native.select_record(it_match->second);
		m_esm_native.set_value("NAME");
		const auto & new_text = m_esm_native.get_value().text;
		insert_entry(ref_cell_name, ref_cell_name, new_text, status_t::translated);
	}

	add_missing_cells(missing_cells);
}

void cell_matcher_t::match_interior_cells()
{
	auto cell_index_native = build_cell_fingerprint_index(m_esm_native);

	std::set<size_t> matched_native_records;
	std::vector<std::pair<size_t, std::string>> missing_cells;

	for (size_t i = 0; i < m_esm_foreign.get_records().size(); ++i)
	{
		m_esm_foreign.select_record(i);
		if (m_esm_foreign.get_record().id != "CELL")
			continue;

		m_esm_foreign.set_value("NAME");
		if (!m_esm_foreign.get_value().exist || m_esm_foreign.get_value().text.empty())
			continue;

		const auto ref_cell_name = m_esm_foreign.get_value().text;

		m_esm_foreign.set_value("DATA");
		if (!m_esm_foreign.get_value().exist)
			continue;

		if (!is_interior_cell(m_esm_foreign.get_value().content))
			continue;

		auto fingerprint = make_cell_fingerprint(m_esm_foreign);
		if (fingerprint.empty())
		{
			tools_t::add_log(
			    "[warning] empty fingerprint for interior cell: \"" + ref_cell_name + "\"\r\n", true);
			missing_cells.push_back({ i, ref_cell_name });
			m_counters.missing++;
			continue;
		}

		auto it_match = cell_index_native.find(fingerprint);
		if (it_match == cell_index_native.end())
		{
			missing_cells.push_back({ i, ref_cell_name });
			m_counters.missing++;
			continue;
		}

		const auto & positions = it_match->second;
		if (positions.size() > 1)
		{
			missing_cells.push_back({ i, ref_cell_name });
			m_counters.missing++;
			continue;
		}

		const auto native_pos = *positions.begin();
		matched_native_records.insert(native_pos);

		m_esm_native.select_record(native_pos);
		m_esm_native.set_value("NAME");
		const auto & new_text = m_esm_native.get_value().text;

		insert_entry(ref_cell_name, ref_cell_name, new_text, status_t::translated);
	}

	match_interior_cells_heuristic(missing_cells, matched_native_records);
	add_missing_cells(missing_cells, m_native_candidates_str);
}

void cell_matcher_t::match_interior_cells_heuristic(
    std::vector<std::pair<size_t, std::string>> & missing_cells,
    const std::set<size_t> & matched_native_records)
{
	std::vector<std::pair<size_t, std::string>> native_cells;

	for (size_t i = 0; i < m_esm_native.get_records().size(); ++i)
	{
		if (matched_native_records.count(i))
			continue;

		m_esm_native.select_record(i);
		if (m_esm_native.get_record().id != "CELL")
			continue;

		m_esm_native.set_value("DATA");
		if (!m_esm_native.get_value().exist)
			continue;

		if (!is_interior_cell(m_esm_native.get_value().content))
			continue;

		m_esm_native.set_value("NAME");
		if (!m_esm_native.get_value().exist || m_esm_native.get_value().text.empty())
			continue;

		native_cells.push_back({ i, m_esm_native.get_value().text });
	}

	tools_t::add_log("=== HEURISTIC START ===\r\n", true);
	tools_t::add_log("Foreign missing: " + std::to_string(missing_cells.size()) + "\r\n", true);
	tools_t::add_log("Native unmatched: " + std::to_string(native_cells.size()) + "\r\n", true);
	if (m_translation_engine && m_translation_engine->is_loaded())
		tools_t::add_log("[info] translation engine: active (cell heuristic)\r\n");
	else
		tools_t::add_log("[info] translation engine: inactive (cell heuristic skipped)\r\n");

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

			if (native_cells[ni].second != foreign_name)
				continue;

			matched_foreign.insert(fi);
			matched_native.insert(ni);
			matched_native_names.insert(foreign_name);

			tools_t::add_log("[EXACT] \"" + foreign_name + "\"\r\n", true);

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
		for (size_t fi = 0; fi < missing_cells.size(); ++fi)
		{
			if (matched_foreign.count(fi))
				continue;

			const auto & foreign_name = missing_cells[fi].second;

			if (!m_translation_engine || !m_translation_engine->is_loaded())
				continue;

			auto result = m_translation_engine->translate(foreign_name);
			if (!result.success)
				continue;

			const auto & translated_text = result.text;
			auto translated_words = split_words(translated_text);
			auto original_words = split_words(foreign_name);
			auto compare_words = build_compare_words(translated_words, original_words);

			auto match = compute_best_match(
			    compare_words, original_words, translated_words, native_cells, matched_native);

			if (match.score <= 0)
				continue;

			bool resolved = false;
			if (match.count > 1)
			{
				resolved = check_all_same_name(compare_words, native_cells, matched_native, match);

				if (resolved)
				{
					tools_t::add_log(
					    "[TIE-SAME iter=" + std::to_string(iteration) +
					    " orig=" + std::to_string(match.score_orig) +
					    " model=" + std::to_string(match.score_model) +
					    " count=" + std::to_string(match.count) +
					    "] \"" + foreign_name + "\" => \"" + translated_text +
					    "\" -> \"" + match.name + "\"\r\n");
				}
			}

			if (match.count == 1 || resolved)
			{
				matched_foreign.insert(fi);
				matched_native.insert(match.index);

				if (!resolved)
				{
					tools_t::add_log(
					    "[TRANSLATE iter=" + std::to_string(iteration) +
					    " orig=" + std::to_string(match.score_orig) +
					    " model=" + std::to_string(match.score_model) +
					    "] \"" + foreign_name + "\" => \"" + translated_text +
					    "\" -> \"" + match.name + "\"\r\n");
				}

				const auto cell_status = resolved ? status_t::translated : status_t::heuristic;
				insert_entry(foreign_name, foreign_name, match.name, cell_status);
				progress = true;
			}
			else if (!resolved)
			{
				tools_t::add_log(
				    "[TIE iter=" + std::to_string(iteration) +
				    " orig=" + std::to_string(match.score_orig) +
				    " model=" + std::to_string(match.score_model) +
				    " count=" + std::to_string(match.count) +
				    "] \"" + foreign_name + "\"\r\n");
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

	if (missing_cells.empty())
		return;

	std::vector<std::string> unmatched_native_names;
	for (size_t ni = 0; ni < native_cells.size(); ++ni)
	{
		if (!matched_native.count(ni) && !matched_native_names.count(native_cells[ni].second))
			unmatched_native_names.push_back(native_cells[ni].second);
	}

	if (!unmatched_native_names.empty())
	{
		tools_t::add_log(
		    "[info] unmatched native CELL candidates (" +
		    std::to_string(unmatched_native_names.size()) + "):\r\n");
		for (const auto & name : unmatched_native_names)
			tools_t::add_log("  " + name + "\r\n");
	}

	std::string candidates_str;
	for (const auto & name : unmatched_native_names)
	{
		if (!candidates_str.empty())
			candidates_str += "|";

		candidates_str += name;
	}

	m_native_candidates_str = candidates_str;
}

void cell_matcher_t::add_missing_cells(
    const std::vector<std::pair<size_t, std::string>> & missing_cells,
    const std::string & native_candidates_str)
{
	for (const auto & [rec_index, cell_name] : missing_cells)
	{
		insert_entry(cell_name, cell_name, cell_name, status_t::missing);
		tools_t::add_log("[warning] missing CELL: " + cell_name + "\r\n");

		if (!native_candidates_str.empty())
		{
			auto * entry = m_output_dict[tools_t::rec_type_t::cell].find_by_old_text(cell_name);
			if (entry)
				entry->details = native_candidates_str;
		}
	}
}

void cell_matcher_t::match_default_cell_name()
{
	for (size_t i = 0; i < m_esm_native.get_records().size(); ++i)
	{
		m_esm_native.select_record(i);
		if (m_esm_native.get_record().id != "GMST")
			continue;

		m_esm_native.set_key("NAME");
		m_esm_native.set_value("STRV");
		if (m_esm_native.get_key().text != "sDefaultCellname")
			continue;

		if (!m_esm_native.get_value().exist)
			break;

		for (size_t k = 0; k < m_esm_foreign.get_records().size(); ++k)
		{
			m_esm_foreign.select_record(k);
			if (m_esm_foreign.get_record().id != "GMST")
				continue;

			m_esm_foreign.set_key("NAME");
			m_esm_foreign.set_value("STRV");
			if (m_esm_foreign.get_key().text != "sDefaultCellname")
				continue;

			if (!m_esm_foreign.get_value().exist)
				break;

			const auto & old_text = m_esm_foreign.get_value().text;
			const auto & new_text = m_esm_native.get_value().text;
			insert_entry(old_text, old_text, new_text, status_t::translated);

			break;
		}
		break;
	}
}

void cell_matcher_t::match_region_names()
{
	for (size_t i = 0; i < m_esm_native.get_records().size(); ++i)
	{
		m_esm_native.select_record(i);
		if (m_esm_native.get_record().id != "REGN")
			continue;

		m_esm_native.set_key("NAME");
		m_esm_native.set_value("FNAM");
		if (!m_esm_native.get_key().exist || !m_esm_native.get_value().exist)
			continue;

		for (size_t k = 0; k < m_esm_foreign.get_records().size(); ++k)
		{
			m_esm_foreign.select_record(k);
			if (m_esm_foreign.get_record().id != "REGN")
				continue;

			m_esm_foreign.set_key("NAME");
			m_esm_foreign.set_value("FNAM");
			if (m_esm_foreign.get_key().text != m_esm_native.get_key().text)
				continue;

			if (!m_esm_foreign.get_value().exist)
				break;

			const auto & old_text = m_esm_foreign.get_value().text;
			const auto & new_text = m_esm_native.get_value().text;
			insert_entry(old_text, old_text, new_text, status_t::translated);

			break;
		}
	}
}
