#pragma once

#include "includes.hpp"
#include "tools.hpp"
#include "dict_reader.hpp"

class dict_merger_t
{
public:
	const auto & get_name(size_t i)
	{
		return readers[i].get_name();
	}

	const auto & get_dict() const
	{
		return dict;
	}

	dict_merger_t();
	dict_merger_t(const std::vector<std::string> & paths);

	void add_record(const tools_t::rec_type_t type, const std::string & key_text, const std::string & val_text);

private:
	void merge_dict();
	void print_summary_log();

	std::vector<dict_reader_t> readers;
	tools_t::dict_t dict;

	int counter_merged = 0;
	int counter_rejected = 0;
	int counter_identical = 0;
	int counter_all = 0;
};
