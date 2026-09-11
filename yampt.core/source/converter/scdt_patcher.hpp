#pragma once

#include "../utility/domain_types.hpp"
#include <string>
#include <vector>

struct text_patch_result_t
{
	bool success = false;
	bool had_false_positive = false;
};

struct text_patch_params_t
{
	std::string new_text;
	bool is_getpccell = false;
	bool pad_short_to_minimum = false;
};

class scdt_patcher_t
{
public:
	explicit scdt_patcher_t(const std::string & original_scdt);

	text_patch_result_t apply_text_patch(const std::string & old_text, const text_patch_params_t & params);

	text_patch_result_t apply_message_patch(
	    const std::vector<std::string> & segments_old,
	    const std::vector<std::string> & segments_new);

	const std::string & get_scdt() const
	{
		return m_scdt;
	}

	bool is_empty() const
	{
		return m_scdt.empty();
	}

private:
	static constexpr size_t text_size_field_length = 1;
	static constexpr size_t message_first_size_field_length = 2;
	static constexpr size_t message_other_size_field_length = 1;
	static constexpr size_t message_other_null_terminator = 1;
	static constexpr size_t getpccell_marker_offset = 2;
	static constexpr size_t minimum_text_size = 4;

	bool find_text_in_scdt(const std::string & old_text);
	bool find_message_segment(const std::string & segment_old, bool is_later_segment);
	bool segment_boundary_matches(const std::string & segment_old) const;
	bool validate_text_size(const std::string & old_text);
	size_t declared_text_size() const;
	size_t padded_text_size(const std::string & new_text, bool pad_short_to_minimum) const;
	void patch_text_size_byte(size_t stored_size);
	void patch_text_content(size_t old_content_size, const std::string & new_content);
	void patch_getpccell_expr_size(const std::string & old_text, const std::string & new_text);
	void patch_first_message_segment(const std::string & segment_old, const std::string & segment_new);
	void patch_later_message_segment(const std::string & segment_old, const std::string & segment_new);

	std::string m_scdt;
	size_t m_cursor = 0;
};
