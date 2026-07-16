#include "table_builder.hpp"
#include <utility/record_types.hpp>
#include <unordered_map>

static const std::set<status_t> done_statuses = { status_t::translated };

template<typename array_t>
static std::string_view find_display_name_by_prefix(const array_t & sub_types, std::string_view prefix)
{
	for (const auto & entry : sub_types)
	{
		if (entry.prefix == prefix)
			return entry.display_name;
	}
	return {};
}

template<typename array_t>
static std::string_view find_prefix_by_display_name(const array_t & sub_types, std::string_view display_name)
{
	for (const auto & entry : sub_types)
	{
		if (entry.display_name == display_name)
			return entry.prefix;
	}
	return {};
}

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

static bool has_sub_type(rec_type_t type)
{
	return type == rec_type_t::info || type == rec_type_t::bnam || type == rec_type_t::fnam ||
	       type == rec_type_t::desc || type == rec_type_t::indx;
}

static std::string_view lookup_display_name(rec_type_t type, std::string_view prefix)
{
	switch (type)
	{
	case rec_type_t::info:
		return find_display_name_by_prefix(record_types::info_sub_types, prefix);
	case rec_type_t::bnam:
		return find_display_name_by_prefix(record_types::bnam_sub_types, prefix);
	case rec_type_t::fnam:
		return find_display_name_by_prefix(record_types::fnam_sub_types, prefix);
	case rec_type_t::desc:
		return find_display_name_by_prefix(record_types::desc_sub_types, prefix);
	case rec_type_t::indx:
		return find_display_name_by_prefix(record_types::indx_sub_types, prefix);
	default:
		return {};
	}
}

static std::string classify_sub_type(rec_type_t type, const std::string & key_text)
{
	const auto caret_pos = key_text.find('^');
	if (caret_pos == std::string::npos || caret_pos == 0)
		return {};

	const auto prefix = key_text.substr(0, caret_pos);
	const auto result = lookup_display_name(type, prefix);
	return std::string(result);
}

struct entry_identity_t
{
	rec_type_t type;
	const std::string & key_text;
};

static std::string_view lookup_prefix_by_display_name(rec_type_t type, std::string_view display_name)
{
	switch (type)
	{
	case rec_type_t::info:
		return find_prefix_by_display_name(record_types::info_sub_types, display_name);
	case rec_type_t::bnam:
		return find_prefix_by_display_name(record_types::bnam_sub_types, display_name);
	case rec_type_t::fnam:
		return find_prefix_by_display_name(record_types::fnam_sub_types, display_name);
	case rec_type_t::desc:
		return find_prefix_by_display_name(record_types::desc_sub_types, display_name);
	case rec_type_t::indx:
		return find_prefix_by_display_name(record_types::indx_sub_types, display_name);
	default:
		return {};
	}
}

static bool passes_sub_type_filter(const entry_identity_t & entry_id, const table_filter_params_t & params)
{
	if (!params.type_filter_solo)
		return true;

	if (!has_sub_type(entry_id.type))
		return true;

	const auto caret_pos = entry_id.key_text.find('^');
	if (caret_pos == std::string::npos || caret_pos == 0)
		return true;

	const auto prefix = std::string_view(entry_id.key_text).substr(0, caret_pos);
	for (const auto & sub_name : params.sub_type_filter)
	{
		const auto found_prefix = lookup_prefix_by_display_name(entry_id.type, sub_name);
		if (!found_prefix.empty() && found_prefix == prefix)
			return true;
	}

	return false;
}

struct entry_context_t
{
	rec_type_t type;
	rec_type_t count_type;
	const record_entry_t & entry;
	size_t record_index;
	const table_filter_params_t & params;
};

static void count_entry_statistics(dict_counts_t & counts, const entry_context_t & context)
{
	const auto & entry = context.entry;

	counts.type_counts[context.count_type]++;
	counts.total_status_counts[entry.status]++;

	if (has_sub_type(context.type))
	{
		const auto sub_name = classify_sub_type(context.type, entry.key_text);
		if (!sub_name.empty())
		{
			counts.sub_type_total_counts[sub_name]++;
			if (done_statuses.count(entry.status))
				counts.sub_type_translated_counts[sub_name]++;
		}
	}

	if (done_statuses.count(entry.status))
		counts.translated_counts[context.count_type]++;
}

struct bnam_emit_context_t
{
	std::vector<table_row_t> & output_rows;
	std::set<size_t> & consumed_bnams;
	const chapter_t & bnam_chapter;
	const std::unordered_multimap<std::string, size_t> & bnam_prefix_map;
	const table_filter_params_t & params;
};

static void emit_bnam_children(const std::string & info_prefix, bnam_emit_context_t & context)
{
	auto [range_begin, range_end] = context.bnam_prefix_map.equal_range(info_prefix);
	for (auto bnam_iter = range_begin; bnam_iter != range_end; ++bnam_iter)
	{
		const auto & bnam_entry = context.bnam_chapter.records[bnam_iter->second];

		const auto & status_filter = context.params.status_filter;
		if (!status_filter.empty() && status_filter.count(bnam_entry.status) == 0)
			continue;

		table_row_t child;
		child.type = rec_type_t::bnam;
		child.key_text = bnam_entry.key_text;
		child.old_text = bnam_entry.old_text;
		child.new_text = bnam_entry.new_text;
		child.status = bnam_entry.status;
		child.record_index = bnam_iter->second;
		child.is_child = true;

		if (context.params.search.has_query() && !context.params.search.matches(child))
			continue;

		context.output_rows.push_back(std::move(child));
		context.consumed_bnams.insert(bnam_iter->second);
	}
}

struct progress_input_t
{
	const dict_t & data;
	const table_filter_params_t & params;
};

static void compute_progress(dict_counts_t & counts, const progress_input_t & input)
{
	for (const auto & [type, chapter] : input.data)
	{
		if (type == rec_type_t::script)
			continue;

		const auto effective_type = (type == rec_type_t::bnam) ? rec_type_t::info : type;

		if (!input.params.type_filter.empty() && input.params.type_filter.count(effective_type) == 0)
			continue;

		for (const auto & entry : chapter.records)
		{
			if (!passes_sub_type_filter({ type, entry.key_text }, input.params))
				continue;

			counts.progress_total++;
			if (done_statuses.count(entry.status))
				counts.progress_translated++;
		}
	}
}

static table_row_t make_table_row(const entry_context_t & context)
{
	table_row_t row_item;
	row_item.type = context.type;
	row_item.key_text = context.entry.key_text;
	row_item.old_text = context.entry.old_text;
	row_item.new_text = context.entry.new_text;
	row_item.status = context.entry.status;
	row_item.record_index = context.record_index;
	return row_item;
}

static void count_filtered_status(dict_counts_t & counts, const entry_context_t & context)
{
	const auto & search = context.params.search;
	const auto tmp_row = make_table_row(context);
	if (!search.has_query() || search.matches(tmp_row))
		counts.filtered_status_counts[context.entry.status]++;
}

static std::unordered_multimap<std::string, size_t> build_bnam_prefix_map(const dict_t & data)
{
	std::unordered_multimap<std::string, size_t> bnam_prefix_map;
	auto bnam_it = data.find(rec_type_t::bnam);
	if (bnam_it == data.end())
		return bnam_prefix_map;

	for (size_t i = 0; i < bnam_it->second.records.size(); ++i)
	{
		const auto & entry = bnam_it->second.records[i];
		const auto prefix = extract_info_prefix(entry.key_text);
		if (!prefix.empty())
			bnam_prefix_map.emplace(prefix, i);
	}

	return bnam_prefix_map;
}

table_build_result_t build_filtered_rows(const dict_t & data, const table_filter_params_t & params)
{
	table_build_result_t result;
	auto & counts = result.counts;

	auto bnam_prefix_map = build_bnam_prefix_map(data);
	std::set<size_t> consumed_bnams;

	auto bnam_it = data.find(rec_type_t::bnam);

	const bool info_in_filter = params.type_filter.empty() || params.type_filter.count(rec_type_t::info) > 0;

	for (const auto & [type, chapter] : data)
	{
		if (type == rec_type_t::script)
			continue;

		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			const auto & entry = chapter.records[i];
			const auto count_type = (type == rec_type_t::bnam) ? rec_type_t::info : type;

			const entry_context_t context { type, count_type, entry, i, params };

			count_entry_statistics(counts, context);

			if (!params.type_filter.empty() && params.type_filter.count(count_type) == 0)
				continue;

			if (!passes_sub_type_filter({ type, entry.key_text }, params))
				continue;

			count_filtered_status(counts, context);

			if (type == rec_type_t::bnam && consumed_bnams.count(i) > 0)
				continue;

			if (!params.status_filter.empty() && params.status_filter.count(entry.status) == 0)
				continue;

			auto row_item = make_table_row(context);
			if (params.search.has_query() && !params.search.matches(row_item))
				continue;

			result.rows.push_back(std::move(row_item));

			const auto info_prefix = extract_info_prefix(entry.key_text);
			if (type == rec_type_t::info && info_in_filter && bnam_it != data.end() && !info_prefix.empty())
			{
				bnam_emit_context_t bnam_context {
					result.rows, consumed_bnams, bnam_it->second, bnam_prefix_map, params
				};
				emit_bnam_children(info_prefix, bnam_context);
			}
		}
	}

	compute_progress(counts, { data, params });
	return result;
}
