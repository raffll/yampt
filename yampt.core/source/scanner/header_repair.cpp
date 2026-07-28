#include "header_repair.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../utility/domain_types.hpp"
#include "../utility/string_utils.hpp"
#include <cstring>
#include <filesystem>

static constexpr size_t hedr_version_offset = 0;
static constexpr size_t hedr_version_size = 4;
static constexpr size_t master_data_size = 8;
static constexpr size_t record_header_size = 16;
static constexpr size_t sub_record_header_size = 8;

static void write_float(std::string & data, size_t offset, float value)
{
	std::memcpy(data.data() + offset, &value, sizeof(float));
}

static void write_uint64(std::string & data, size_t offset, uint64_t value)
{
	std::memcpy(data.data() + offset, &value, sizeof(uint64_t));
}

struct sub_record_location_t
{
	size_t pos = 0;
	size_t data_offset = 0;
	size_t data_size = 0;
	bool found = false;
};

static std::vector<sub_record_location_t> find_all_sub_records(
    const std::string & content,
    const std::string & target_type)
{
	std::vector<sub_record_location_t> locations;
	size_t scan_pos = record_header_size;

	while (scan_pos + sub_record_header_size <= content.size())
	{
		const auto sub_type = content.substr(scan_pos, 4);
		const auto sub_size = domain_types::convert_string_byte_array_to_uint(
		    content.substr(scan_pos + 4, 4));

		if (sub_size == 0)
			break;

		if (scan_pos + sub_record_header_size + sub_size > content.size())
			break;

		if (sub_type == target_type)
		{
			sub_record_location_t location;
			location.pos = scan_pos;
			location.data_offset = scan_pos + sub_record_header_size;
			location.data_size = sub_size;
			location.found = true;
			locations.push_back(location);
		}

		scan_pos += sub_record_header_size + sub_size;
	}

	return locations;
}

static sub_record_location_t find_first_sub_record(
    const std::string & content,
    const std::string & target_type)
{
	auto all = find_all_sub_records(content, target_type);
	if (all.empty())
		return {};

	return all.front();
}

struct mast_data_pair_t
{
	sub_record_location_t mast_location;
	sub_record_location_t data_location;
	std::string master_name;
};

static std::vector<mast_data_pair_t> find_mast_data_pairs(const std::string & content)
{
	std::vector<mast_data_pair_t> pairs;
	size_t scan_pos = record_header_size;

	while (scan_pos + sub_record_header_size <= content.size())
	{
		const auto sub_type = content.substr(scan_pos, 4);
		const auto sub_size = domain_types::convert_string_byte_array_to_uint(
		    content.substr(scan_pos + 4, 4));

		if (sub_size == 0)
			break;

		if (scan_pos + sub_record_header_size + sub_size > content.size())
			break;

		if (sub_type == "MAST")
		{
			mast_data_pair_t pair;
			pair.mast_location.pos = scan_pos;
			pair.mast_location.data_offset = scan_pos + sub_record_header_size;
			pair.mast_location.data_size = sub_size;
			pair.mast_location.found = true;

			auto raw_name = content.substr(scan_pos + sub_record_header_size, sub_size);
			pair.master_name = string_utils::erase_null_chars(raw_name);

			const auto next_pos = scan_pos + sub_record_header_size + sub_size;
			if (next_pos + sub_record_header_size <= content.size())
			{
				const auto next_type = content.substr(next_pos, 4);
				const auto next_size = domain_types::convert_string_byte_array_to_uint(
				    content.substr(next_pos + 4, 4));

				if (next_type == "DATA" && next_size == master_data_size)
				{
					pair.data_location.pos = next_pos;
					pair.data_location.data_offset = next_pos + sub_record_header_size;
					pair.data_location.data_size = next_size;
					pair.data_location.found = true;
				}
			}

			pairs.push_back(pair);
		}

		scan_pos += sub_record_header_size + sub_size;
	}

	return pairs;
}

std::string header_repair_t::update_master_sizes(
    const std::string & header_content,
    const std::string & plugin_directory)
{
	auto result = header_content;
	const auto pairs = find_mast_data_pairs(result);

	for (const auto & pair : pairs)
	{
		if (!pair.data_location.found)
			continue;

		const auto master_path = plugin_directory + "/" + pair.master_name;

		std::error_code error_code;
		const auto file_size = std::filesystem::file_size(master_path, error_code);
		if (error_code)
			continue;

		const auto actual_size = static_cast<uint64_t>(file_size);
		write_uint64(result, pair.data_location.data_offset, actual_size);
	}

	return result;
}

std::string header_repair_t::update_version_to_1_3(const std::string & header_content)
{
	auto result = header_content;
	const auto hedr = find_first_sub_record(result, "HEDR");
	if (!hedr.found)
		return result;

	if (hedr.data_size < hedr_version_size)
		return result;

	static constexpr float version_1_3 = 1.3f;
	write_float(result, hedr.data_offset + hedr_version_offset, version_1_3);
	return result;
}
