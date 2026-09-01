#pragma once

#include <string>
#include <vector>

struct edit_history_entry_t
{
	std::string timestamp;
	std::string plugin_filename;
	std::string description;
};

struct field_edit_record_t
{
	std::string plugin_filename;
	std::string record_type;
	std::string record_id;
	std::string field_name;
	std::string new_value;
};

struct record_removal_record_t
{
	std::string plugin_filename;
	std::string record_type;
	std::string record_id;
};

class edit_history_t
{
public:
	void record_field_edit(const field_edit_record_t & edit);
	void record_record_removal(const record_removal_record_t & removal);

	void clear();

	const std::vector<edit_history_entry_t> & entries() const;

private:
	void append(const std::string & plugin_filename, const std::string & description);

	std::vector<edit_history_entry_t> m_entries;
};
