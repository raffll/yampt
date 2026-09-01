#pragma once

#include "../model/edit_log.hpp"
#include <decoder/sub_record_schema.hpp>
#include <io/codepage.hpp>
#include <string>
#include <QObject>

class plugin_session_t;

struct field_edit_request_t
{
	std::string record_type;
	std::string record_id;
	std::string sub_type;
	int occurrence = 0;
	int object_ref_index = -1;
	int bit_index = -1;
	int plugin_idx = -1;
	size_t record_index = 0;
	field_def_t field = {};
	std::string input_text;
	codepage_t codepage = codepage_t::windows_1252;
};

struct edit_result_t
{
	bool success = false;
	std::string error_message;
};

class field_edit_controller_t : public QObject
{
	Q_OBJECT

public:
	explicit field_edit_controller_t(plugin_session_t & session, QObject * parent = nullptr);

	edit_result_t commit_field_edit(const field_edit_request_t & request);

signals:
	void record_modified(bool is_merge_edit, const std::string & saved_path);
	void field_edited(const field_edit_record_t & edit);

private:
	edit_result_t commit_to_merge(const field_edit_request_t & request, const std::string & patched_content);
	edit_result_t commit_to_source(const field_edit_request_t & request, const std::string & patched_content);

	plugin_session_t & m_session;
};
