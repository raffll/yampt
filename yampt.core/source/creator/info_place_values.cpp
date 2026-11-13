#include "info_place_values.hpp"
#include "../scanner/sub_record_merge.hpp"

namespace {

constexpr const char * place_filter_sub_type = "ANAM";
constexpr const char * condition_sub_type = "SCVR";
constexpr const char * info_record_type = "INFO";
constexpr size_t condition_type_offset = 1;
constexpr size_t condition_prefix_size = 5;
constexpr char condition_cell_type_char = 'B';

std::string without_trailing_terminators(const std::string & text)
{
	const auto last_kept = text.find_last_not_of('\0');
	if (last_kept == std::string::npos)
		return {};

	return text.substr(0, last_kept + 1);
}

void add_place_if_present(const std::string & raw_text, std::set<std::string> & place_values)
{
	const auto place = without_trailing_terminators(raw_text);
	if (place.empty())
		return;

	place_values.insert(place);
}

bool is_cell_condition(const std::string & condition_data)
{
	if (condition_data.size() <= condition_prefix_size)
		return false;

	return condition_data[condition_type_offset] == condition_cell_type_char;
}

} // namespace

void info_place_values::collect_from_info_record(
    const std::string & record_content,
    std::set<std::string> & place_values)
{
	for (const auto & sub_record : sub_record_merge_t::parse_sub_records(record_content))
	{
		if (sub_record.type == place_filter_sub_type)
		{
			add_place_if_present(sub_record.data, place_values);
			continue;
		}

		if (sub_record.type != condition_sub_type)
			continue;

		if (!is_cell_condition(sub_record.data))
			continue;

		add_place_if_present(sub_record.data.substr(condition_prefix_size), place_values);
	}
}

std::set<std::string> info_place_values::collect_from_plugin(const esm_reader_t & plugin)
{
	std::set<std::string> place_values;

	for (const auto & record : plugin.get_records())
	{
		if (record.id != info_record_type)
			continue;

		collect_from_info_record(record.content, place_values);
	}

	return place_values;
}
