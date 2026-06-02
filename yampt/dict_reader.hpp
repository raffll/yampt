#pragma once

#include "includes.hpp"
#include "tools.hpp"

class dict_reader_t
{
public:
	dict_reader_t(const std::string & path);

	const auto & is_loaded() const
	{
		return loaded_;
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
	void validate_entry(tools_t::record_entry_t & entry, tools_t::rec_type_t type);

	tools_t::name_t name;
	tools_t::dict_t dict;
	bool loaded_ = false;
};
