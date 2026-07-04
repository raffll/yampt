#include "decoder/view_group_def.hpp"

static const std::vector<view_group_def_t> definitions = {
	{ "*", "Door Destination", { "DODT", "DNAM", nullptr } },
	{ "*", "Position", { "DATA", nullptr } },
};

const std::vector<view_group_def_t> & view_group_definitions()
{
	return definitions;
}

const char * find_group_label(const std::string & record_type, const std::string & sub_type)
{
	for (const auto & group : definitions)
	{
		if (group.record_type != std::string("*") && group.record_type != record_type)
			continue;

		for (int index = 0; index < 8 && group.sub_types[index] != nullptr; ++index)
		{
			if (sub_type == group.sub_types[index])
				return group.group_label;
		}
	}

	return nullptr;
}
