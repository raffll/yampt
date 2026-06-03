#include "dict_merger.hpp"

dict_merger_t::dict_merger_t()
{
	dict = tools_t::initialize_dict();
}

dict_merger_t::dict_merger_t(const std::vector<std::string> & paths)
{
	dict = tools_t::initialize_dict();

	for (const auto & path : paths)
	{
		dict_reader_t reader(path);
		readers.push_back(reader);
	}

	merge_dict();
	find_duplicate_values(tools_t::rec_type_t::cell);
	find_duplicate_values(tools_t::rec_type_t::dial);
	find_unused_info();
	print_summary_log();
}

void dict_merger_t::add_record(
    const tools_t::rec_type_t type,
    const std::string & key_text,
    const std::string & val_text)
{
	tools_t::record_entry_t entry;
	entry.key_text = key_text;
	entry.new_text = val_text;
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
					tools_t::add_log(
					    "[warning] replaced " + tools_t::type_to_str(type) + " record " + entry.key_text + "\r\n");
					counter_replaced++;
				}
				else
				{
					counter_identical++;
				}
			}
		}
	}

	tools_t::add_log("[info] merging complete\r\n");
}

void dict_merger_t::find_duplicate_values(tools_t::rec_type_t type)
{
	std::set<std::string> texts;
	for (const auto & entry : dict.at(type).records)
	{
		std::string text_lc = entry.new_text;
		transform(text_lc.begin(), text_lc.end(), text_lc.begin(), ::tolower);

		if (texts.insert(text_lc).second)
			continue;

		tools_t::add_log("[warning] duplicate " + tools_t::type_to_str(type) + " value " + entry.new_text + "\r\n");
	}
}

void dict_merger_t::find_unused_info()
{
	for (const auto & info : dict.at(tools_t::rec_type_t::info).records)
	{
		bool found = false;
		std::string text = info.key_text;
		if (text.size() < 1 || text.substr(0, 1) != "T")
			continue;

		size_t beg = text.find("^") + 1;
		size_t end = text.find_last_of("^");
		text = text.substr(beg, end - beg);

		for (const auto & dial : dict.at(tools_t::rec_type_t::dial).records)
		{
			if (text != dial.new_text)
				continue;

			found = true;
		}

		if (!found)
		{
			tools_t::add_log("[warning] dialog topic not found " + info.key_text + "\r\n");
		}
	}
}

void dict_merger_t::print_summary_log()
{
	std::string line = "merged=" + std::to_string(counter_merged) + ", replaced=" + std::to_string(counter_replaced) +
	                   ", identical=" + std::to_string(counter_identical) + "\r\n";

	tools_t::add_log(line);
}
