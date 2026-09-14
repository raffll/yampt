#include "scdt_patcher.hpp"
#include "../utility/app_logger.hpp"

scdt_patcher_t::scdt_patcher_t(const std::string & original_scdt)
    : m_scdt(original_scdt)
{}

text_patch_result_t scdt_patcher_t::apply_text_patch(
    const std::string & old_text,
    const text_patch_params_t & params)
{
	text_patch_result_t result;

	if (m_scdt.empty())
		return result;

	if (!find_text_in_scdt(old_text))
		return result;

	while (!validate_text_size(old_text) || !opcode_precedes_text(params.required_opcode))
	{
		result.had_false_positive = true;
		m_cursor += old_text.size();

		if (!find_text_in_scdt(old_text))
			return result;
	}

	const auto stored_size = padded_text_size(params.new_text, params.pad_short_to_minimum);
	const auto padding = std::string(stored_size - params.new_text.size(), '\0');
	const auto new_content = params.new_text + padding;
	const auto old_stored_size = params.is_getpccell ? old_text.size() : declared_text_size();

	patch_text_size_byte(stored_size);
	patch_text_content(old_stored_size, new_content);

	if (params.is_getpccell)
		patch_getpccell_expr_size(old_text, params.new_text);
	else
		m_cursor += stored_size;

	result.success = true;
	return result;
}

size_t scdt_patcher_t::declared_text_size() const
{
	const auto size_byte_pos = m_cursor - text_size_field_length;

	return domain_types::convert_string_byte_array_to_uint(m_scdt.substr(size_byte_pos, text_size_field_length));
}

size_t scdt_patcher_t::padded_text_size(const std::string & new_text, bool pad_short_to_minimum) const
{
	if (pad_short_to_minimum && new_text.size() < minimum_text_size)
		return minimum_text_size;

	return new_text.size();
}

text_patch_result_t scdt_patcher_t::apply_message_patch(const std::string & old_blob, const std::string & new_blob)
{
	text_patch_result_t result;

	if (m_scdt.empty())
		return result;

	if (!find_text_in_scdt(old_blob))
		return result;

	while (!blob_size_field_matches(old_blob))
	{
		result.had_false_positive = true;
		m_cursor += old_blob.size();

		if (!find_text_in_scdt(old_blob))
			return result;
	}

	if (old_blob == new_blob)
	{
		result.success = true;
		return result;
	}

	patch_message_blob(old_blob, new_blob);

	result.success = true;
	return result;
}

bool scdt_patcher_t::blob_size_field_matches(const std::string & old_blob) const
{
	if (m_cursor < message_blob_size_field_length)
		return false;

	const auto size_field_pos = m_cursor - message_blob_size_field_length;
	const auto declared_size = domain_types::convert_string_byte_array_to_uint(
	    m_scdt.substr(size_field_pos, message_blob_size_field_length));

	return declared_size == old_blob.size();
}

void scdt_patcher_t::patch_message_blob(const std::string & old_blob, const std::string & new_blob)
{
	const auto size_field_pos = m_cursor - message_blob_size_field_length;
	const auto encoded_size = domain_types::convert_uint_to_string_byte_array(new_blob.size());

	m_scdt.erase(size_field_pos, message_blob_size_field_length);
	m_scdt.insert(size_field_pos, encoded_size.substr(0, message_blob_size_field_length));

	m_scdt.erase(m_cursor, old_blob.size());
	m_scdt.insert(m_cursor, new_blob);

	m_cursor += new_blob.size();
}

text_patch_result_t scdt_patcher_t::apply_button_patch(const std::string & old_button, const std::string & new_button)
{
	text_patch_result_t result;

	if (m_scdt.empty())
		return result;

	if (!find_text_in_scdt(old_button))
		return result;

	while (!button_size_field_matches(old_button))
	{
		result.had_false_positive = true;
		m_cursor += old_button.size();

		if (!find_text_in_scdt(old_button))
			return result;
	}

	if (old_button != new_button)
		patch_button(old_button, new_button);
	else
		m_cursor += old_button.size();

	result.success = true;
	return result;
}

bool scdt_patcher_t::button_size_field_matches(const std::string & old_button) const
{
	if (m_cursor < button_size_field_length)
		return false;

	const auto size_field_pos = m_cursor - button_size_field_length;
	const auto declared_size =
	    domain_types::convert_string_byte_array_to_uint(m_scdt.substr(size_field_pos, button_size_field_length));

	return declared_size == old_button.size() + button_null_terminator;
}

void scdt_patcher_t::patch_button(const std::string & old_button, const std::string & new_button)
{
	const auto size_field_pos = m_cursor - button_size_field_length;
	const auto encoded_size = domain_types::convert_uint_to_string_byte_array(new_button.size() + button_null_terminator);

	m_scdt.erase(size_field_pos, button_size_field_length);
	m_scdt.insert(size_field_pos, encoded_size.substr(0, button_size_field_length));

	m_scdt.erase(m_cursor, old_button.size());
	m_scdt.insert(m_cursor, new_button);

	m_cursor += new_button.size() + button_null_terminator;
}

bool scdt_patcher_t::find_text_in_scdt(const std::string & old_text)
{
	m_cursor = m_scdt.find(old_text, m_cursor);
	return m_cursor != std::string::npos;
}

bool scdt_patcher_t::validate_text_size(const std::string & old_text)
{
	const auto size_byte_pos = m_cursor - text_size_field_length;
	const auto declared_size =
	    domain_types::convert_string_byte_array_to_uint(m_scdt.substr(size_byte_pos, text_size_field_length));

	return declared_size == old_text.size() || declared_size == old_text.size() + 1;
}

bool scdt_patcher_t::opcode_precedes_text(const std::string & required_opcode) const
{
	if (required_opcode.empty())
		return true;

	if (m_cursor < text_size_field_length + required_opcode.size())
		return false;

	const auto opcode_pos = m_cursor - text_size_field_length - required_opcode.size();

	return m_scdt.compare(opcode_pos, required_opcode.size(), required_opcode) == 0;
}

void scdt_patcher_t::patch_text_size_byte(size_t stored_size)
{
	const auto size_byte_pos = m_cursor - text_size_field_length;
	const auto encoded_size = domain_types::convert_uint_to_string_byte_array(stored_size);

	m_scdt.erase(size_byte_pos, text_size_field_length);
	m_scdt.insert(size_byte_pos, encoded_size.substr(0, text_size_field_length));
}

void scdt_patcher_t::patch_text_content(size_t old_content_size, const std::string & new_content)
{
	m_scdt.erase(m_cursor, old_content_size);
	m_scdt.insert(m_cursor, new_content);
}

void scdt_patcher_t::patch_getpccell_expr_size(const std::string & old_text, const std::string & new_text)
{
	const auto marker_pos = m_scdt.rfind('X', m_cursor);
	if (marker_pos == std::string::npos || marker_pos < getpccell_marker_offset)
		return;

	const auto expr_size_pos = marker_pos - getpccell_marker_offset;
	const auto old_expr_size =
	    domain_types::convert_string_byte_array_to_uint(m_scdt.substr(expr_size_pos, text_size_field_length));
	const auto expression_size = old_expr_size + new_text.size() - old_text.size();

	const auto encoded_size = domain_types::convert_uint_to_string_byte_array(expression_size);
	m_scdt.erase(expr_size_pos, text_size_field_length);
	m_scdt.insert(expr_size_pos, encoded_size.substr(0, text_size_field_length));

	m_cursor = expr_size_pos + expression_size;
}


