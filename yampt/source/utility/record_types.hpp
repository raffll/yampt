#pragma once

#include "tools.hpp"
#include <array>
#include <string_view>

namespace record_types {

struct fnam_record_t
{
	std::string_view record_id;
	std::string_view display_name;
};

// clang-format off
constexpr std::array fnam_records = {
	fnam_record_t{ "ACTI", "Activators" },
	fnam_record_t{ "ALCH", "Potions" },
	fnam_record_t{ "APPA", "Apparatus" },
	fnam_record_t{ "ARMO", "Armor" },
	fnam_record_t{ "BOOK", "Books" },
	fnam_record_t{ "BSGN", "Birthsigns" },
	fnam_record_t{ "CLAS", "Classes" },
	fnam_record_t{ "CLOT", "Clothing" },
	fnam_record_t{ "CONT", "Containers" },
	fnam_record_t{ "CREA", "Creatures" },
	fnam_record_t{ "DOOR", "Doors" },
	fnam_record_t{ "FACT", "Factions" },
	fnam_record_t{ "INGR", "Ingredients" },
	fnam_record_t{ "LIGH", "Lights" },
	fnam_record_t{ "LOCK", "Lockpicks" },
	fnam_record_t{ "MISC", "Miscellaneous" },
	fnam_record_t{ "NPC_", "NPCs" },
	fnam_record_t{ "PROB", "Probes" },
	fnam_record_t{ "RACE", "Races" },
	fnam_record_t{ "REGN", "Regions" },
	fnam_record_t{ "REPA", "Repair Items" },
	fnam_record_t{ "SPEL", "Spells" },
	fnam_record_t{ "WEAP", "Weapons" },
};

struct sub_type_t
{
	std::string_view prefix;
	std::string_view display_name;
	tools_t::rec_type_t parent_type;
};

constexpr std::array info_sub_types = {
	sub_type_t{ "T", "Topic",      tools_t::rec_type_t::info },
	sub_type_t{ "V", "Voice",      tools_t::rec_type_t::info },
	sub_type_t{ "G", "Greeting",   tools_t::rec_type_t::info },
	sub_type_t{ "P", "Persuasion", tools_t::rec_type_t::info },
	sub_type_t{ "J", "Journal",    tools_t::rec_type_t::info },
};

constexpr std::array bnam_sub_types = {
	sub_type_t{ "T", "Topic",      tools_t::rec_type_t::bnam },
	sub_type_t{ "V", "Voice",      tools_t::rec_type_t::bnam },
	sub_type_t{ "G", "Greeting",   tools_t::rec_type_t::bnam },
	sub_type_t{ "P", "Persuasion", tools_t::rec_type_t::bnam },
	sub_type_t{ "J", "Journal",    tools_t::rec_type_t::bnam },
};

constexpr std::array fnam_sub_types = {
	sub_type_t{ "ACTI", "ACTI", tools_t::rec_type_t::fnam },
	sub_type_t{ "ALCH", "ALCH", tools_t::rec_type_t::fnam },
	sub_type_t{ "APPA", "APPA", tools_t::rec_type_t::fnam },
	sub_type_t{ "ARMO", "ARMO", tools_t::rec_type_t::fnam },
	sub_type_t{ "BOOK", "BOOK", tools_t::rec_type_t::fnam },
	sub_type_t{ "BSGN", "BSGN", tools_t::rec_type_t::fnam },
	sub_type_t{ "CLAS", "CLAS", tools_t::rec_type_t::fnam },
	sub_type_t{ "CLOT", "CLOT", tools_t::rec_type_t::fnam },
	sub_type_t{ "CONT", "CONT", tools_t::rec_type_t::fnam },
	sub_type_t{ "CREA", "CREA", tools_t::rec_type_t::fnam },
	sub_type_t{ "DOOR", "DOOR", tools_t::rec_type_t::fnam },
	sub_type_t{ "FACT", "FACT", tools_t::rec_type_t::fnam },
	sub_type_t{ "INGR", "INGR", tools_t::rec_type_t::fnam },
	sub_type_t{ "LIGH", "LIGH", tools_t::rec_type_t::fnam },
	sub_type_t{ "LOCK", "LOCK", tools_t::rec_type_t::fnam },
	sub_type_t{ "MISC", "MISC", tools_t::rec_type_t::fnam },
	sub_type_t{ "NPC_", "NPC_", tools_t::rec_type_t::fnam },
	sub_type_t{ "PROB", "PROB", tools_t::rec_type_t::fnam },
	sub_type_t{ "RACE", "RACE", tools_t::rec_type_t::fnam },
	sub_type_t{ "REGN", "REGN", tools_t::rec_type_t::fnam },
	sub_type_t{ "REPA", "REPA", tools_t::rec_type_t::fnam },
	sub_type_t{ "SPEL", "SPEL", tools_t::rec_type_t::fnam },
	sub_type_t{ "WEAP", "WEAP", tools_t::rec_type_t::fnam },
};

constexpr std::array desc_sub_types = {
	sub_type_t{ "BSGN", "Birthsigns", tools_t::rec_type_t::desc },
	sub_type_t{ "CLAS", "Classes",    tools_t::rec_type_t::desc },
	sub_type_t{ "RACE", "Races",      tools_t::rec_type_t::desc },
};

constexpr std::array indx_sub_types = {
	sub_type_t{ "SKIL", "Skills",        tools_t::rec_type_t::indx },
	sub_type_t{ "MGEF", "Magic Effects", tools_t::rec_type_t::indx },
};
// clang-format on

constexpr bool is_fnam_eligible(std::string_view record_id)
{
	for (const auto & entry : fnam_records)
	{
		if (entry.record_id == record_id)
			return true;
	}
	return false;
}

constexpr std::string_view fnam_display_name(std::string_view record_id)
{
	for (const auto & entry : fnam_records)
	{
		if (entry.record_id == record_id)
			return entry.display_name;
	}
	return {};
}

} // namespace record_types
