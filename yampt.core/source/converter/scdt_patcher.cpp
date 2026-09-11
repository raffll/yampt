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

	while (!validate_text_size(old_text))
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
		const auto is_later_segment = index > 0;

		if (!find_message_segment(segments_old[index], is_later_segment))
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
		{
			const auto new_size_with_null = segments_new[index].size() + message_other_null_terminator;
			if (new_size_with_null > 255)
			{
				app_logger_t::add_log("[error] message segment exceeds 255 byte limit, skipping SCDT patch\r\n");
				return result;
			}

			patch_later_message_segment(segments_old[index], segments_new[index]);
		}
	}

	result.success = true;
	return result;
}

bool scdt_patcher_t::find_text_in_scdt(const std::string & old_text)
{
	m_cursor = m_scdt.find(old_text, m_cursor);
	return m_cursor != std::string::npos;
}

bool scdt_patcher_t::find_message_segment(const std::string & segment_old, bool is_later_segment)
{
	if (!find_text_in_scdt(segment_old))
		return false;

	if (!is_later_segment)
		return true;

	while (!segment_boundary_matches(segment_old))
	{
		m_cursor += segment_old.size();

		if (!find_text_in_scdt(segment_old))
			return false;
	}

	return true;
}

bool scdt_patcher_t::segment_boundary_matches(const std::string & segment_old) const
{
	if (m_cursor < message_other_size_field_length)
		return false;

	const auto size_field_pos = m_cursor - message_other_size_field_length;
	const auto declared_size = domain_types::convert_string_byte_array_to_uint(
	    m_scdt.substr(size_field_pos, message_other_size_field_length));

	return declared_size == segment_old.size() || declared_size == segment_old.size() + message_other_null_terminator;
}

bool scdt_patcher_t::validate_text_size(const std::string & old_text)
{
	const auto size_byte_pos = m_cursor - text_size_field_length;
	const auto declared_size =
	    domain_types::convert_string_byte_array_to_uint(m_scdt.substr(size_byte_pos, text_size_field_length));

	return declared_size == old_text.size() || declared_size == old_text.size() + 1;
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

void scdt_patcher_t::patch_first_message_segment(const std::string & segment_old, const std::string & segment_new)
{
	const auto size_field_pos = m_cursor - message_first_size_field_length;
	const auto encoded_size = domain_types::convert_uint_to_string_byte_array(segment_new.size());

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
	const auto encoded_size = domain_types::convert_uint_to_string_byte_array(new_size_with_null);

	m_scdt.erase(size_field_pos, message_other_size_field_length);
	m_scdt.insert(size_field_pos, encoded_size.substr(0, message_other_size_field_length));
	m_scdt.erase(m_cursor, segment_old.size());
	m_scdt.insert(m_cursor, segment_new);
	m_cursor += segment_new.size() + message_other_null_terminator;
}
