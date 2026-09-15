#pragma once

#include <map>
#include <string>

namespace record_type_name {

inline const char * display(const std::string & type)
{
	static const std::map<std::string, const char *> names = {
		{ "ACTI", "Activator" },
		{ "ALCH", "Potion" },
		{ "APPA", "Apparatus" },
		{ "ARMO", "Armor" },
		{ "BODY", "Body Part" },
		{ "BOOK", "Book" },
		{ "BSGN", "Birthsign" },
		{ "CELL", "Cell" },
		{ "CLAS", "Class" },
		{ "CLOT", "Clothing" },
		{ "CONT", "Container" },
		{ "CREA", "Creature" },
		{ "DIAL", "Dialogue" },
		{ "DOOR", "Door" },
		{ "ENCH", "Enchantment" },
		{ "FACT", "Faction" },
		{ "GLOB", "Global" },
		{ "GMST", "Game Setting" },
		{ "INFO", "Dialogue Response" },
		{ "INGR", "Ingredient" },
		{ "LAND", "Landscape" },
		{ "LEVC", "Leveled Creature" },
		{ "LEVI", "Leveled Item" },
		{ "LIGH", "Light" },
		{ "LOCK", "Lockpick" },
		{ "LTEX", "Land Texture" },
		{ "MGEF", "Magic Effect" },
		{ "MISC", "Misc. Item" },
		{ "NPC_", "NPC" },
		{ "PGRD", "Path Grid" },
		{ "PROB", "Probe" },
		{ "RACE", "Race" },
		{ "REGN", "Region" },
		{ "REPA", "Repair Item" },
		{ "SCPT", "Script" },
		{ "SKIL", "Skill" },
		{ "SNDG", "Sound Generator" },
		{ "SOUN", "Sound" },
		{ "SPEL", "Spell" },
		{ "SSCR", "Start Script" },
		{ "STAT", "Static" },
		{ "TES3", "File Header" },
		{ "WEAP", "Weapon" },
	};

	const auto it_name = names.find(type);
	if (it_name != names.end())
		return it_name->second;

	return nullptr;
}

} // namespace record_type_name
