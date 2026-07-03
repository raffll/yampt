#pragma once

#include <decoder/sub_record_schema.hpp>
#include <scanner/sub_record_merge.hpp>
#include <string>
#include <vector>

struct patch_result_t
{
	bool success = false;
	std::string content;
	std::string description;
};

class merge_patch_ops_t
{
public:
	static patch_result_t patch_sub_record(
	    const std::string & merge_content,
	    const std::string & source_content,
	    const std::string & sub_type,
	    int binary_idx);

	static patch_result_t patch_group(
	    const std::string & merge_content,
	    const std::string & source_content,
	    const std::vector<std::pair<std::string, int>> & group_binary_indices);

	static patch_result_t patch_field(
	    const std::string & merge_content,
	    const std::string & source_content,
	    const std::string & record_type,
	    const std::string & sub_type,
	    size_t sub_size,
	    int binary_idx,
	    int field_idx);

	static std::string extract_sub_type_from_field_name(const std::string & field_name);
};
