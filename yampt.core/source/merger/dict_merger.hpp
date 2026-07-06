#pragma once

#include "../io/dict_reader.hpp"
#include "../utility/domain_types.hpp"
#include "../utility/includes.hpp"

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

	void add_record(const rec_type_t type, const std::string & key_text, const std::string & new_text);

private:
	void merge_dict();
	void print_summary_log();

	std::vector<dict_reader_t> readers;
	dict_t dict;

	int counter_merged = 0;
	int counter_rejected = 0;
	int counter_identical = 0;
	int counter_all = 0;
};
