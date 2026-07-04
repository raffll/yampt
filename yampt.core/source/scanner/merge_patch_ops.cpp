#include "merge_patch_ops.hpp"
#include <cstring>

patch_result_t merge_patch_ops_t::patch_sub_record(
    const std::string & merge_content,
    const std::string & source_content,
    const std::string & sub_type,
    int binary_idx)
{
	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);

	if (binary_idx < 0 || binary_idx >= static_cast<int>(source_subs.size()))
		return { false, {}, "binary_idx out of range" };

	const auto & source_sub = source_subs[binary_idx];

	bool is_multi_occurrence = is_content_matched_type(sub_type);

	if (is_multi_occurrence)
	{
		for (const auto & existing : merge_subs)
		{
			if (existing.type == sub_type && existing.data == source_sub.data)
				return { true, merge_content, sub_type };
		}

		merge_subs.push_back(source_sub);
	}
	else
	{
		int source_occurrence = 0;
		for (int s = 0; s < binary_idx; ++s)
		{
			if (source_subs[s].type == sub_type)
				++source_occurrence;
		}

		const auto merge_idx = sub_record_merge_t::find_by_type_and_occurrence(merge_subs, sub_type, source_occurrence);
		if (merge_idx >= 0)
		{
			merge_subs[merge_idx].data = source_sub.data;
		}
		else
		{
			int insert_pos = static_cast<int>(merge_subs.size());
			for (int s = binary_idx - 1; s >= 0; --s)
			{
				const auto & preceding = source_subs[s];
				int preceding_occurrence = 0;
				for (int p = 0; p < s; ++p)
				{
					if (source_subs[p].type == preceding.type)
						++preceding_occurrence;
				}

				const auto preceding_merge_idx =
				    sub_record_merge_t::find_by_type_and_occurrence(merge_subs, preceding.type, preceding_occurrence);

				if (preceding_merge_idx >= 0)
				{
					insert_pos = preceding_merge_idx + 1;
					break;
				}
			}

			merge_subs.insert(merge_subs.begin() + insert_pos, source_sub);
		}
	}

	auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	return { true, std::move(patched), sub_type };
}

bool merge_patch_ops_t::is_content_matched_type(const std::string & sub_type)
{
	return sub_type == "NPCS" || sub_type == "NPCO";
}

patch_result_t merge_patch_ops_t::patch_group(
    const std::string & merge_content,
    const std::string & source_content,
    const std::vector<std::pair<std::string, int>> & group_binary_indices)
{
	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);

	for (const auto & [child_sub_type, child_binary_idx] : group_binary_indices)
	{
		if (child_binary_idx < 0 || child_binary_idx >= static_cast<int>(source_subs.size()))
			continue;

		const auto & source_sub = source_subs[child_binary_idx];
		int source_occurrence = 0;
		for (int s = 0; s < child_binary_idx; ++s)
		{
			if (source_subs[s].type == child_sub_type)
				++source_occurrence;
		}

		const auto merge_idx =
		    sub_record_merge_t::find_by_type_and_occurrence(merge_subs, child_sub_type, source_occurrence);
		if (merge_idx < 0)
			merge_subs.push_back(source_sub);
		else
			merge_subs[merge_idx].data = source_sub.data;
	}

	auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	return { true, std::move(patched), "group" };
}

patch_result_t merge_patch_ops_t::patch_field(
    const std::string & merge_content,
    const std::string & source_content,
    const std::string & record_type,
    const std::string & sub_type,
    size_t sub_size,
    int binary_idx,
    int field_idx)
{
	const auto * schema = find_schema(record_type, sub_type, sub_size);
	if (!schema)
		return { false, {}, "no schema found" };

	if (field_idx < 0 || field_idx >= static_cast<int>(schema->field_count))
		return { false, {}, "field_idx out of range" };

	const auto & field = schema->fields[field_idx];

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);

	if (binary_idx < 0 || binary_idx >= static_cast<int>(source_subs.size()))
		return { false, {}, "binary_idx out of range" };

	int source_occurrence = 0;
	for (int s = 0; s < binary_idx; ++s)
	{
		if (source_subs[s].type == sub_type)
			++source_occurrence;
	}

	const auto merge_idx = sub_record_merge_t::find_by_type_and_occurrence(merge_subs, sub_type, source_occurrence);
	if (merge_idx < 0)
	{
		merge_subs.push_back(source_subs[binary_idx]);
	}
	else
	{
		const auto & source_data = source_subs[binary_idx].data;
		auto & merge_data = merge_subs[merge_idx].data;

		if (field.offset + field.size > source_data.size())
			return { false, {}, "source data too small for field" };

		if (field.offset + field.size > merge_data.size())
			return { false, {}, "merge data too small for field" };

		std::memcpy(&merge_data[field.offset], &source_data[field.offset], field.size);
	}

	auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);
	return { true, std::move(patched), std::string(field.name) };
}

std::string merge_patch_ops_t::extract_sub_type_from_field_name(const std::string & field_name)
{
	if (field_name.size() < 4)
		return {};

	auto space_pos = field_name.find(' ');
	if (space_pos == std::string::npos)
		return field_name.substr(0, 4);

	return field_name.substr(0, space_pos);
}
