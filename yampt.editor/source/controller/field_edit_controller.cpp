#include "controller/field_edit_controller.hpp"
#include "session/plugin_session.hpp"
#include <decoder/field_encoder.hpp>
#include <decoder/field_validator.hpp>
#include <decoder/sub_record_iter.hpp>
#include <cstring>

struct sub_offset_result_t
{
	bool found = false;
	size_t byte_offset = 0;
};

static sub_offset_result_t find_frmr_sub_offset(const std::string & content, const field_edit_request_t & request)
{
	sub_record_iter_t iterator(content);
	sub_record_view_t sub_view;
	bool in_target_group = false;
	int count = 0;

	while (iterator.next(sub_view))
	{
		if (sub_view.type == "FRMR")
		{
			const auto ref_index = read_frmr_ref_index(sub_view.data, sub_view.size);
			in_target_group = (static_cast<int>(ref_index) == request.object_ref_index);
			count = 0;
			continue;
		}

		if (!in_target_group)
			continue;

		if (sub_view.type != request.sub_type)
			continue;

		if (count == request.occurrence)
			return { true, sub_view.offset };

		++count;
	}

	return {};
}

static sub_offset_result_t find_sub_record_offset(const std::string & content, const field_edit_request_t & request)
{
	if (request.object_ref_index >= 0)
		return find_frmr_sub_offset(content, request);

	sub_record_iter_t iterator(content);
	sub_record_view_t sub_view;
	int count = 0;

	while (iterator.next(sub_view))
	{
		if (sub_view.type != request.sub_type)
			continue;

		if (count == request.occurrence)
			return { true, sub_view.offset };

		++count;
	}

	return {};
}

static size_t read_sub_size(const std::string & content, size_t sub_byte_offset)
{
	uint32_t value = 0;
	std::memcpy(&value, content.data() + sub_byte_offset + 4, 4);
	return static_cast<size_t>(value);
}

static bool is_variable_size_field(field_type_t field_type)
{
	return field_type == field_type_t::string_var;
}

field_edit_controller_t::field_edit_controller_t(plugin_session_t & session, QObject * parent)
    : QObject(parent)
    , m_session(session)
{}

edit_result_t field_edit_controller_t::commit_field_edit(const field_edit_request_t & request)
{
	std::string owned_content;
	const std::string * content_ptr = nullptr;

	if (request.plugin_idx == -1)
	{
		content_ptr = m_session.scan().find_merge_content(request.record_type, request.record_id);
	}
	else
	{
		owned_content = m_session.scan().read_record_content(request.plugin_idx, request.record_index);
		if (!owned_content.empty())
			content_ptr = &owned_content;
	}

	if (!content_ptr)
		return { false, "record content not found" };

	const auto & content = *content_ptr;
	const auto sub_result = find_sub_record_offset(content, request);
	if (!sub_result.found)
		return { false, "sub-record not found at expected occurrence" };

	if (request.field.name == nullptr)
		return { false, "no schema defined for this sub-record" };

	const auto existing_sub_size = read_sub_size(content, sub_result.byte_offset);
	const auto * existing_sub_data = content.data() + sub_result.byte_offset + 8;

	const auto validation =
	    field_validator::validate_field(request.field, request.input_text, request.codepage, existing_sub_size);

	if (!validation.valid)
		return { false, validation.error_message };

	const field_encoder::encode_context_t encode_ctx {
		request.field, request.input_text, request.codepage, existing_sub_data, existing_sub_size
	};

	const auto encoded = field_encoder::encode_field(encode_ctx);
	auto patched = field_encoder::patch_sub_record(content, sub_result.byte_offset, request.field, encoded);

	if (is_variable_size_field(request.field.type) || request.field.type == field_type_t::raw)
		patched = field_encoder::patch_record_size(patched);

	if (request.plugin_idx == -1)
		return commit_to_merge(request, patched);

	return commit_to_source(request, patched);
}

edit_result_t field_edit_controller_t::commit_to_merge(
    const field_edit_request_t & request,
    const std::string & patched_content)
{
	const auto & rec_type = request.record_type;
	const auto & record_id = request.record_id;

	if (m_session.scan().is_merge_pinned(rec_type, record_id))
		m_session.scan().pin_record_to_merge(rec_type, record_id, patched_content);
	else
		m_session.scan().copy_record_to_merge_raw(rec_type, record_id, patched_content);

	m_session.scan().recompute_single_conflict(rec_type, record_id);
	emit record_modified(true, {});
	return { true, {} };
}

edit_result_t field_edit_controller_t::commit_to_source(
    const field_edit_request_t & request,
    const std::string & patched_content)
{
	auto & plugin = m_session.scan().mutable_plugin(request.plugin_idx);
	plugin.select_record(request.record_index);
	plugin.replace_record(patched_content);

	m_session.mark_plugin_dirty(request.plugin_idx);
	m_session.scan().recompute_single_conflict(request.record_type, request.record_id);

	const auto & plugin_path = m_session.scan().plugin_path(request.plugin_idx);
	emit record_modified(false, plugin_path);
	return { true, {} };
}
