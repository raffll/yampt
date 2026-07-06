#include "scdt_patcher.hpp"

scdt_patcher_t::scdt_patcher_t(const std::string & original_scdt)
    : m_scdt(original_scdt)
{}

text_patch_result_t scdt_patcher_t::apply_text_patch(
    const std::string & old_text,
    const std::string & new_text,
    bool is_getpccell)
{
	text_patch_result_t result;

	if (m_scdt.empty())
		return result;

	if (!find_text_in_scdt(old_text))
		return result;

	while (!validate_text_size(old_text))
	{
		result.had_false_positive = true;
		m_cursor += old_text.size();

		if (!find_text_in_scdt(old_text))
			return result;
	}

	patch_text_size_byte(new_text);
	patch_text_content(old_text, new_text);

	if (is_getpccell)
		patch_getpccell_expr_size(new_text);
	else
		m_cursor += new_text.size();

	result.success = true;
	return result;
}

text_patch_result_t scdt_patcher_t::apply_message_patch(
    const std::vector<std::string> & segments_old,
    const std::vector<std::string> & segments_new)
{
	text_patch_result_t result;

	if (m_scdt.empty())
		return result;

	if (segments_old.size() != segments_new.size())
		return result;

	for (size_t index = 0; index < segments_old.size(); ++index)
	{
		if (!find_text_in_scdt(segments_old[index]))
			return result;

		if (segments_old[index] == segments_new[index])
		{
			m_cursor += segments_old[index].size();
			continue;
		}

		if (segments_old[index] == " " || segments_old[index] == "\t")
			return result;

		if (index == 0)
			patch_first_message_segment(segments_old[index], segments_new[index]);
		else
			patch_later_message_segment(segments_old[index], segments_new[index]);
	}

	result.success = true;
	return result;
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
	    domain_types_t::convert_string_byte_array_to_uint(m_scdt.substr(size_byte_pos, text_size_field_length));

	return declared_size == old_text.size() || declared_size == old_text.size() + 1;
}

void scdt_patcher_t::patch_text_size_byte(const std::string & new_text)
{
	const auto size_byte_pos = m_cursor - text_size_field_length;
	const auto encoded_size = domain_types_t::convert_uint_to_string_byte_array(new_text.size());

	m_scdt.erase(size_byte_pos, text_size_field_length);
	m_scdt.insert(size_byte_pos, encoded_size.substr(0, text_size_field_length));
}

void scdt_patcher_t::patch_text_content(const std::string & old_text, const std::string & new_text)
{
	m_scdt.erase(m_cursor, old_text.size());
	m_scdt.insert(m_cursor, new_text);
}

void scdt_patcher_t::patch_getpccell_expr_size(const std::string & new_text)
{
	const auto text_end_pos = m_cursor + new_text.size();
	size_t end_of_expression = 0;
	size_t expression_size = 0;

	const auto byte_after_text = m_scdt.substr(text_end_pos, 1);
	if (byte_after_text != " ")
	{
		end_of_expression = text_end_pos;
		const auto marker_pos = m_scdt.rfind('X', m_cursor);
		const auto expr_size_pos = marker_pos - getpccell_marker_offset;
		expression_size = end_of_expression - expr_size_pos;
		m_cursor = expr_size_pos;
	}
	else
	{
		end_of_expression = text_end_pos;
		while (end_of_expression < m_scdt.size() && m_scdt[end_of_expression] != '\0')
			++end_of_expression;

		const auto marker_pos = m_scdt.rfind('X', m_cursor);
		const auto expr_size_pos = marker_pos - getpccell_marker_offset;
		expression_size = end_of_expression - expr_size_pos - 1;
		m_cursor = expr_size_pos;
	}

	const auto encoded_size = domain_types_t::convert_uint_to_string_byte_array(expression_size);
	m_scdt.erase(m_cursor, text_size_field_length);
	m_scdt.insert(m_cursor, encoded_size.substr(0, text_size_field_length));
	m_cursor += expression_size;
}

void scdt_patcher_t::patch_first_message_segment(const std::string & segment_old, const std::string & segment_new)
{
	const auto size_field_pos = m_cursor - message_first_size_field_length;
	const auto encoded_size = domain_types_t::convert_uint_to_string_byte_array(segment_new.size());

	m_scdt.erase(size_field_pos, message_first_size_field_length);
	m_scdt.insert(size_field_pos, encoded_size.substr(0, message_first_size_field_length));
	m_scdt.erase(m_cursor, segment_old.size());
	m_scdt.insert(m_cursor, segment_new);
	m_cursor += segment_new.size();
}

void scdt_patcher_t::patch_later_message_segment(const std::string & segment_old, const std::string & segment_new)
{
	const auto size_field_pos = m_cursor - message_other_size_field_length;
	const auto new_size_with_null = segment_new.size() + message_other_null_terminator;
	const auto encoded_size = domain_types_t::convert_uint_to_string_byte_array(new_size_with_null);

	m_scdt.erase(size_field_pos, message_other_size_field_length);
	m_scdt.insert(size_field_pos, encoded_size.substr(0, message_other_size_field_length));
	m_scdt.erase(m_cursor, segment_old.size());
	m_scdt.insert(m_cursor, segment_new);
	m_cursor += segment_new.size();
}
