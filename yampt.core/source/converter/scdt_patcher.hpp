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
	std::string required_opcode;
};

class scdt_patcher_t
{
public:
	explicit scdt_patcher_t(const std::string & original_scdt);

	text_patch_result_t apply_text_patch(const std::string & old_text, const text_patch_params_t & params);

	text_patch_result_t apply_message_patch(const std::string & old_blob, const std::string & new_blob);

	text_patch_result_t apply_button_patch(const std::string & old_button, const std::string & new_button);

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
	static constexpr size_t message_blob_size_field_length = 2;
	static constexpr size_t button_size_field_length = 1;
	static constexpr size_t button_null_terminator = 1;
	static constexpr size_t getpccell_marker_offset = 2;
	static constexpr size_t minimum_text_size = 4;

	bool find_text_in_scdt(const std::string & old_text);
	bool validate_text_size(const std::string & old_text);
	bool opcode_precedes_text(const std::string & required_opcode) const;
	bool blob_size_field_matches(const std::string & old_blob) const;
	bool button_size_field_matches(const std::string & old_button) const;
	void patch_button(const std::string & old_button, const std::string & new_button);
	size_t declared_text_size() const;
	size_t padded_text_size(const std::string & new_text, bool pad_short_to_minimum) const;
	void patch_text_size_byte(size_t stored_size);
	void patch_text_content(size_t old_content_size, const std::string & new_content);
	void patch_getpccell_expr_size(const std::string & old_text, const std::string & new_text);
	void patch_message_blob(const std::string & old_blob, const std::string & new_blob);

	std::string m_scdt;
	size_t m_cursor = 0;
};
