#pragma once

#include "../utility/domain_types.hpp"
#include "../utility/includes.hpp"

struct yyjson_val;

class dict_reader_t
{
public:
	dict_reader_t(const std::string & path);

	const auto & is_loaded() const
	{
		return m_loaded;
	}

	const auto & get_name() const
	{
		return name;
	}

	const auto & get_dict() const
	{
		return dict;
	}

private:
	void parse_json(const std::string & content, const std::string & path);
	void parse_chapter(yyjson_val * array, rec_type_t type, const std::string & type_str);
	void parse_record(yyjson_val * record, rec_type_t type, const std::string & type_str);
	void validate_entry(record_entry_t & entry, rec_type_t type);

	file_path_parts_t name;
	dict_t dict;
	bool m_loaded = false;
};
