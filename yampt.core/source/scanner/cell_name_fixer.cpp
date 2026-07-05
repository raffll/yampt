#include "cell_name_fixer.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../utility/tools.hpp"
#include <cstdint>
#include <cstring>

static constexpr uint32_t interior_flag = 0x01;
static constexpr size_t data_sub_record_size = 12;
static constexpr size_t sub_record_header_size = 8;
static constexpr size_t record_body_size_offset = 4;
static constexpr size_t minimum_versions_required = 3;

bool cell_name_fixer_t::is_exterior_cell(const std::string & content)
{
	sub_record_iter_t iter(content);
	sub_record_view_t view;

	while (iter.next(view))
	{
		if (view.type != "DATA" || view.size != data_sub_record_size)
			continue;

		uint32_t flags = 0;
		std::memcpy(&flags, view.data, sizeof(uint32_t));
		return (flags & interior_flag) == 0;
	}

	return false;
}

std::string cell_name_fixer_t::get_cell_name(const std::string & content)
{
	sub_record_iter_t iter(content);
	sub_record_view_t view;

	while (iter.next(view))
	{
		if (view.type != "NAME")
			continue;

		std::string name(view.data, view.size);
		if (!name.empty() && name.back() == '\0')
			name.pop_back();

		return name;
	}

	return {};
}

std::string cell_name_fixer_t::set_cell_name(const std::string & content, const std::string & name)
{
	sub_record_iter_t iter(content);
	sub_record_view_t view;

	while (iter.next(view))
	{
		if (view.type != "NAME")
			continue;

		const auto new_name_with_null = name + '\0';
		const auto new_data_size = new_name_with_null.size();
		const auto old_data_size = view.size;
		const auto size_difference = static_cast<int64_t>(new_data_size) - static_cast<int64_t>(old_data_size);

		auto result = content.substr(0, view.offset + 4);

		const auto size_as_array = tools_t::convert_uint_to_string_byte_array(new_data_size);
		result += size_as_array;
		result += new_name_with_null;
		result += content.substr(view.offset + sub_record_header_size + old_data_size);

		const auto old_record_body_size =
		    tools_t::convert_string_byte_array_to_uint(result.substr(record_body_size_offset, 4));
		const auto new_record_body_size =
		    static_cast<size_t>(static_cast<int64_t>(old_record_body_size) + size_difference);
		const auto body_size_array = tools_t::convert_uint_to_string_byte_array(new_record_body_size);
		result.replace(record_body_size_offset, 4, body_size_array);

		return result;
	}

	return content;
}

std::string cell_name_fixer_t::find_last_intermediate_rename(
    const std::vector<std::string> & version_contents,
    const std::string & first_name)
{
	const auto last_intermediate = static_cast<int64_t>(version_contents.size()) - 2;

	for (auto index = last_intermediate; index >= 1; --index)
	{
		const auto & intermediate = version_contents[static_cast<size_t>(index)];
		const auto intermediate_name = get_cell_name(intermediate);

		if (intermediate_name.empty())
			continue;

		if (intermediate_name != first_name)
			return intermediate_name;
	}

	return {};
}

std::string cell_name_fixer_t::apply(const std::vector<std::string> & version_contents)
{
	if (version_contents.size() < minimum_versions_required)
		return {};

	const auto & first_content = version_contents.front();
	if (!is_exterior_cell(first_content))
		return {};

	const auto first_name = get_cell_name(first_content);
	const auto & winner_content = version_contents.back();
	const auto winner_name = get_cell_name(winner_content);

	const auto intermediate_rename = find_last_intermediate_rename(version_contents, first_name);
	if (intermediate_rename.empty())
		return {};

	const auto winner_reverted = (winner_name == first_name) || winner_name.empty();
	if (!winner_reverted)
		return {};

	return set_cell_name(winner_content, intermediate_rename);
}
