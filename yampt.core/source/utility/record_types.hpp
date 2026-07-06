#pragma once

#include "domain_types.hpp"
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
	rec_type_t parent_type;
};

constexpr std::array info_sub_types = {
	sub_type_t{ "T", "Topic",      rec_type_t::info },
	sub_type_t{ "V", "Voice",      rec_type_t::info },
	sub_type_t{ "G", "Greeting",   rec_type_t::info },
	sub_type_t{ "P", "Persuasion", rec_type_t::info },
	sub_type_t{ "J", "Journal",    rec_type_t::info },
};

constexpr std::array bnam_sub_types = {
	sub_type_t{ "T", "Topic",      rec_type_t::bnam },
	sub_type_t{ "V", "Voice",      rec_type_t::bnam },
	sub_type_t{ "G", "Greeting",   rec_type_t::bnam },
	sub_type_t{ "P", "Persuasion", rec_type_t::bnam },
	sub_type_t{ "J", "Journal",    rec_type_t::bnam },
};

constexpr std::array fnam_sub_types = {
	sub_type_t{ "ACTI", "ACTI", rec_type_t::fnam },
	sub_type_t{ "ALCH", "ALCH", rec_type_t::fnam },
	sub_type_t{ "APPA", "APPA", rec_type_t::fnam },
	sub_type_t{ "ARMO", "ARMO", rec_type_t::fnam },
	sub_type_t{ "BOOK", "BOOK", rec_type_t::fnam },
	sub_type_t{ "BSGN", "BSGN", rec_type_t::fnam },
	sub_type_t{ "CLAS", "CLAS", rec_type_t::fnam },
	sub_type_t{ "CLOT", "CLOT", rec_type_t::fnam },
	sub_type_t{ "CONT", "CONT", rec_type_t::fnam },
	sub_type_t{ "CREA", "CREA", rec_type_t::fnam },
	sub_type_t{ "DOOR", "DOOR", rec_type_t::fnam },
	sub_type_t{ "FACT", "FACT", rec_type_t::fnam },
	sub_type_t{ "INGR", "INGR", rec_type_t::fnam },
	sub_type_t{ "LIGH", "LIGH", rec_type_t::fnam },
	sub_type_t{ "LOCK", "LOCK", rec_type_t::fnam },
	sub_type_t{ "MISC", "MISC", rec_type_t::fnam },
	sub_type_t{ "NPC_", "NPC_", rec_type_t::fnam },
	sub_type_t{ "PROB", "PROB", rec_type_t::fnam },
	sub_type_t{ "RACE", "RACE", rec_type_t::fnam },
	sub_type_t{ "REGN", "REGN", rec_type_t::fnam },
	sub_type_t{ "REPA", "REPA", rec_type_t::fnam },
	sub_type_t{ "SPEL", "SPEL", rec_type_t::fnam },
	sub_type_t{ "WEAP", "WEAP", rec_type_t::fnam },
};

constexpr std::array desc_sub_types = {
	sub_type_t{ "BSGN", "Birthsigns", rec_type_t::desc },
	sub_type_t{ "CLAS", "Classes",    rec_type_t::desc },
	sub_type_t{ "RACE", "Races",      rec_type_t::desc },
};

constexpr std::array indx_sub_types = {
	sub_type_t{ "SKIL", "Skills",        rec_type_t::indx },
	sub_type_t{ "MGEF", "Magic Effects", rec_type_t::indx },
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
