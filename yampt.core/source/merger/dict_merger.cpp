#include "dict_merger.hpp"

dict_merger_t::dict_merger_t()
{
	dict = tools_t::initialize_dict();
}

dict_merger_t::dict_merger_t(const std::vector<std::string> & paths)
{
	dict = tools_t::initialize_dict();

	for (auto it = paths.rbegin(); it != paths.rend(); ++it)
	{
		dict_reader_t reader(*it);
		readers.push_back(reader);
	}

	merge_dict();
	print_summary_log();
}

void dict_merger_t::add_record(
    const tools_t::rec_type_t type,
    const std::string & key_text,
    const std::string & new_text)
{
	tools_t::record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = key_text;
	entry.new_text = new_text;
	dict.at(type).insert(entry);
}

void dict_merger_t::merge_dict()
{
	for (const auto & reader : readers)
	{
		for (const auto & chapter : reader.get_dict())
		{
			const auto & type = chapter.first;
			for (const auto & entry : chapter.second.records)
			{
				auto * existing = dict.at(type).find(entry.key_text);
				if (existing == nullptr)
				{
					dict.at(type).insert(entry);
					counter_merged++;
				}
				else if (existing->new_text != entry.new_text)
				{
					counter_rejected++;
				}
				else
				{
					counter_identical++;
				}
			}
		}
	}
}

void dict_merger_t::print_summary_log()
{
	std::string line = "[info] merged=" + std::to_string(counter_merged) +
	                   ", rejected=" + std::to_string(counter_rejected) +
	                   ", identical=" + std::to_string(counter_identical) + "\r\n";

	tools_t::add_log(line);
}
