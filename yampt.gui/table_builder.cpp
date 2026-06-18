#include "table_builder.hpp"

#include <unordered_map>

static const std::map<std::pair<tools_t::rec_type_t, std::string>, std::string> prefix_to_sub_type = {
	{{tools_t::rec_type_t::info, "T"}, "Topic"},
	{{tools_t::rec_type_t::info, "V"}, "Voice"},
	{{tools_t::rec_type_t::info, "G"}, "Greeting"},
	{{tools_t::rec_type_t::info, "P"}, "Persuasion"},
	{{tools_t::rec_type_t::info, "J"}, "Journal"},
	{{tools_t::rec_type_t::bnam, "T"}, "Topic"},
	{{tools_t::rec_type_t::bnam, "V"}, "Voice"},
	{{tools_t::rec_type_t::bnam, "G"}, "Greeting"},
	{{tools_t::rec_type_t::bnam, "P"}, "Persuasion"},
	{{tools_t::rec_type_t::bnam, "J"}, "Journal"},
	{{tools_t::rec_type_t::fnam, "ACTI"}, "ACTI"}, {{tools_t::rec_type_t::fnam, "ALCH"}, "ALCH"},
	{{tools_t::rec_type_t::fnam, "APPA"}, "APPA"}, {{tools_t::rec_type_t::fnam, "ARMO"}, "ARMO"},
	{{tools_t::rec_type_t::fnam, "BOOK"}, "BOOK"}, {{tools_t::rec_type_t::fnam, "BSGN"}, "BSGN"},
	{{tools_t::rec_type_t::fnam, "CLAS"}, "CLAS"}, {{tools_t::rec_type_t::fnam, "CLOT"}, "CLOT"},
	{{tools_t::rec_type_t::fnam, "CONT"}, "CONT"}, {{tools_t::rec_type_t::fnam, "CREA"}, "CREA"},
	{{tools_t::rec_type_t::fnam, "DOOR"}, "DOOR"}, {{tools_t::rec_type_t::fnam, "FACT"}, "FACT"},
	{{tools_t::rec_type_t::fnam, "INGR"}, "INGR"}, {{tools_t::rec_type_t::fnam, "LIGH"}, "LIGH"},
	{{tools_t::rec_type_t::fnam, "LOCK"}, "LOCK"}, {{tools_t::rec_type_t::fnam, "MISC"}, "MISC"},
	{{tools_t::rec_type_t::fnam, "NPC_"}, "NPC_"}, {{tools_t::rec_type_t::fnam, "PROB"}, "PROB"},
	{{tools_t::rec_type_t::fnam, "RACE"}, "RACE"}, {{tools_t::rec_type_t::fnam, "REGN"}, "REGN"},
	{{tools_t::rec_type_t::fnam, "REPA"}, "REPA"}, {{tools_t::rec_type_t::fnam, "SPEL"}, "SPEL"},
	{{tools_t::rec_type_t::fnam, "WEAP"}, "WEAP"},
	{{tools_t::rec_type_t::desc, "BSGN"}, "Birthsigns"},
	{{tools_t::rec_type_t::desc, "CLAS"}, "Classes"},
	{{tools_t::rec_type_t::desc, "RACE"}, "Races"},
	{{tools_t::rec_type_t::indx, "SKIL"}, "Skills"},
	{{tools_t::rec_type_t::indx, "MGEF"}, "Magic Effects"},
};

static const std::map<std::string, std::string> sub_type_to_prefix = {
	{"Topic", "T"}, {"Voice", "V"}, {"Greeting", "G"}, {"Persuasion", "P"}, {"Journal", "J"},
	{"ACTI", "ACTI"}, {"ALCH", "ALCH"}, {"APPA", "APPA"}, {"ARMO", "ARMO"}, {"BOOK", "BOOK"},
	{"BSGN", "BSGN"}, {"CLAS", "CLAS"}, {"CLOT", "CLOT"}, {"CONT", "CONT"}, {"CREA", "CREA"},
	{"DOOR", "DOOR"}, {"FACT", "FACT"}, {"INGR", "INGR"}, {"LIGH", "LIGH"}, {"LOCK", "LOCK"},
	{"MISC", "MISC"}, {"NPC_", "NPC_"}, {"PROB", "PROB"}, {"RACE", "RACE"}, {"REGN", "REGN"},
	{"REPA", "REPA"}, {"SPEL", "SPEL"}, {"WEAP", "WEAP"},
	{"Birthsigns", "BSGN"}, {"Classes", "CLAS"}, {"Races", "RACE"},
	{"Skills", "SKIL"}, {"Magic Effects", "MGEF"},
};

static std::string extract_info_prefix(const std::string & key_text)
{
	size_t first = key_text.find('^');
	if (first == std::string::npos)
		return {};

	size_t second = key_text.find('^', first + 1);
	if (second == std::string::npos)
		return {};

	size_t third = key_text.find('^', second + 1);
	if (third == std::string::npos)
		return key_text;

	return key_text.substr(0, third);
}

static bool has_sub_type(tools_t::rec_type_t type)
{
	return type == tools_t::rec_type_t::info ||
		type == tools_t::rec_type_t::bnam ||
		type == tools_t::rec_type_t::fnam ||
		type == tools_t::rec_type_t::desc ||
		type == tools_t::rec_type_t::indx;
}

static std::string classify_sub_type(tools_t::rec_type_t type, const std::string & key_text)
{
	auto caret_pos = key_text.find('^');
	if (caret_pos == std::string::npos || caret_pos == 0)
		return {};

	auto prefix = key_text.substr(0, caret_pos);
	auto it = prefix_to_sub_type.find({type, prefix});
	if (it != prefix_to_sub_type.end())
		return it->second;

	return {};
}

static bool passes_sub_type_filter(
	tools_t::rec_type_t type,
	const std::string & key_text,
	const std::set<std::string> & sub_type_filter,
	bool type_filter_solo)
{
	if (!type_filter_solo)
		return true;

	if (!has_sub_type(type))
		return true;

	auto caret_pos = key_text.find('^');
	if (caret_pos == std::string::npos || caret_pos == 0)
		return true;

	auto prefix = key_text.substr(0, caret_pos);
	for (const auto & sub : sub_type_filter)
	{
		auto it = sub_type_to_prefix.find(sub);
		if (it != sub_type_to_prefix.end() && it->second == prefix)
			return true;
	}

	return false;
}

table_build_result_t build_filtered_rows(
	const tools_t::dict_t & data,
	dict_kind_t kind,
	const std::set<tools_t::rec_type_t> & type_filter,
	const std::set<std::string> & sub_type_filter,
	const std::set<std::string> & status_filter,
	const search_engine_t & search,
	bool type_filter_solo)
{
	static const std::set<std::string> done_statuses_user = {
		"translated"
	};

	static const std::set<std::string> done_statuses_base = {
		"matched", "fingerprint", "coords", "heuristic", "exact",
		"info", "wilderness", "region"
	};

	const auto & done_statuses = (kind == dict_kind_t::base)
		? done_statuses_base : done_statuses_user;

	table_build_result_t result;
	auto & counts = result.counts;

	std::unordered_multimap<std::string, size_t> bnam_prefix_map;
	std::set<size_t> consumed_bnams;

	auto bnam_it = data.find(tools_t::rec_type_t::bnam);
	if (bnam_it != data.end())
	{
		for (size_t i = 0; i < bnam_it->second.records.size(); ++i)
		{
			const auto & entry = bnam_it->second.records[i];
			auto prefix = extract_info_prefix(entry.key_text);
			if (!prefix.empty())
				bnam_prefix_map.emplace(prefix, i);
		}
	}

	const bool info_in_filter = type_filter.empty() ||
		type_filter.count(tools_t::rec_type_t::info) > 0;

	for (const auto & [type, chapter] : data)
	{
		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			const auto & entry = chapter.records[i];

			counts.type_counts[type]++;
			counts.total_status_counts[entry.status]++;

			if (has_sub_type(type))
			{
				const auto sub = classify_sub_type(type, entry.key_text);
				if (!sub.empty())
				{
					counts.sub_type_total_counts[sub]++;
					if (done_statuses.count(entry.status))
						counts.sub_type_translated_counts[sub]++;
				}
			}

			if (done_statuses.count(entry.status))
				counts.translated_counts[type]++;

			// --- filter pipeline ---

			// 1. type_filter check (for filtered_status_counts and row emission)
			if (!type_filter.empty() && type_filter.count(type) == 0)
				continue;

			// 2. sub_type_filter check
			if (!passes_sub_type_filter(type, entry.key_text, sub_type_filter, type_filter_solo))
				continue;

			// 3. count filtered_status_counts (passes type + sub_type + search, but NOT status)
			//    Counted for ALL records including consumed BNAMs — this is status-independent
			{
				table_row_t tmp_row;
				tmp_row.type = type;
				tmp_row.key_text = entry.key_text;
				tmp_row.old_text = entry.old_text;
				tmp_row.new_text = entry.new_text;
				tmp_row.status = entry.status;
				tmp_row.record_index = i;

				if (!search.has_query() || search.matches(tmp_row))
					counts.filtered_status_counts[entry.status]++;
			}

			// 4. skip consumed BNAMs (for row emission only — counting is done above)
			if (type == tools_t::rec_type_t::bnam && consumed_bnams.count(i) > 0)
				continue;

			// 5. status_filter
			if (!status_filter.empty() && status_filter.count(entry.status) == 0)
				continue;

			// 6. search
			table_row_t row;
			row.type = type;
			row.key_text = entry.key_text;
			row.old_text = entry.old_text;
			row.new_text = entry.new_text;
			row.status = entry.status;
			row.record_index = i;

			if (search.has_query() && !search.matches(row))
				continue;

			// 7. add to output
			result.rows.push_back(std::move(row));

			// BNAM interleaving: after an INFO row passes all filters, emit matching BNAMs
			if (type == tools_t::rec_type_t::info && info_in_filter && bnam_it != data.end())
			{
				auto info_prefix = extract_info_prefix(entry.key_text);
				if (info_prefix.empty())
					continue;

				auto [begin, end] = bnam_prefix_map.equal_range(info_prefix);
				for (auto bit = begin; bit != end; ++bit)
				{
					const auto & bnam_entry = bnam_it->second.records[bit->second];

					if (!status_filter.empty() && status_filter.count(bnam_entry.status) == 0)
						continue;

					table_row_t child;
					child.type = tools_t::rec_type_t::bnam;
					child.key_text = bnam_entry.key_text;
					child.old_text = bnam_entry.old_text;
					child.new_text = bnam_entry.new_text;
					child.status = bnam_entry.status;
					child.record_index = bit->second;
					child.is_child = true;

					if (search.has_query() && !search.matches(child))
						continue;

					result.rows.push_back(std::move(child));
					consumed_bnams.insert(bit->second);
				}
			}
		}
	}

	// progress computation: count type-filtered records with done statuses
	for (const auto & [type, chapter] : data)
	{
		if (!type_filter.empty() && type_filter.count(type) == 0)
			continue;

		for (const auto & entry : chapter.records)
		{
			counts.progress_total++;
			if (done_statuses.count(entry.status))
				counts.progress_translated++;
		}
	}

	return result;
}
