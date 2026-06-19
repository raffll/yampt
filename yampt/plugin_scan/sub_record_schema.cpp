#include "sub_record_schema.hpp"
#include <cstring>
#include <iterator>

static const char * const cell_flags[] = {
	"Interior", "Has Water", "Illegal to Sleep", nullptr,
	nullptr, nullptr, nullptr, "Behave like Exterior", nullptr
};

static const field_def_t cell_data_fields[] = {
	{"Flags", field_type_t::flags_u32, 0, 4, nullptr, cell_flags},
	{"Grid X", field_type_t::i32, 4, 4, nullptr, nullptr},
	{"Grid Y", field_type_t::i32, 8, 4, nullptr, nullptr},
};

static const char * const npc_flags[] = {
	"Female", "Essential", "Respawn", nullptr,
	"Autocalc", nullptr, nullptr, nullptr, nullptr
};

static const field_def_t npc_flag_fields[] = {
	{"Flags", field_type_t::flags_u32, 0, 4, nullptr, npc_flags},
};

static const field_def_t npc_npdt_12_fields[] = {
	{"Level", field_type_t::u16, 0, 2, nullptr, nullptr},
	{"Disposition", field_type_t::u8, 2, 1, nullptr, nullptr},
	{"Reputation", field_type_t::u8, 3, 1, nullptr, nullptr},
	{"Rank", field_type_t::u8, 4, 1, nullptr, nullptr},
	{"Unknown1", field_type_t::u8, 5, 1, nullptr, nullptr},
	{"Unknown2", field_type_t::u8, 6, 1, nullptr, nullptr},
	{"Unknown3", field_type_t::u8, 7, 1, nullptr, nullptr},
	{"Gold", field_type_t::u32, 8, 4, nullptr, nullptr},
};

static const field_def_t npc_npdt_52_fields[] = {
	{"Level", field_type_t::u16, 0, 2, nullptr, nullptr},
	{"Strength", field_type_t::u8, 2, 1, nullptr, nullptr},
	{"Intelligence", field_type_t::u8, 3, 1, nullptr, nullptr},
	{"Willpower", field_type_t::u8, 4, 1, nullptr, nullptr},
	{"Agility", field_type_t::u8, 5, 1, nullptr, nullptr},
	{"Speed", field_type_t::u8, 6, 1, nullptr, nullptr},
	{"Endurance", field_type_t::u8, 7, 1, nullptr, nullptr},
	{"Personality", field_type_t::u8, 8, 1, nullptr, nullptr},
	{"Luck", field_type_t::u8, 9, 1, nullptr, nullptr},
	{"Skills", field_type_t::raw, 10, 27, nullptr, nullptr},
	{"Reputation", field_type_t::u8, 37, 1, nullptr, nullptr},
	{"Health", field_type_t::u16, 38, 2, nullptr, nullptr},
	{"Magicka", field_type_t::u16, 40, 2, nullptr, nullptr},
	{"Fatigue", field_type_t::u16, 42, 2, nullptr, nullptr},
	{"Disposition", field_type_t::u8, 44, 1, nullptr, nullptr},
	{"Faction ID", field_type_t::u8, 45, 1, nullptr, nullptr},
	{"Rank", field_type_t::u8, 46, 1, nullptr, nullptr},
	{"Unknown", field_type_t::u8, 47, 1, nullptr, nullptr},
	{"Gold", field_type_t::u32, 48, 4, nullptr, nullptr},
};

static const char * const aidt_flags[] = {
	"Weapon", "Armor", "Clothing", "Books",
	"Ingredients", "Picks", "Probes", "Lights",
	"Apparatus", "Repair Items", "Miscellaneous",
	"Spells", "Magic Items", "Potions",
	"Training", "Spellmaking", "Enchanting", nullptr
};

static const field_def_t npc_aidt_fields[] = {
	{"Hello", field_type_t::u8, 0, 1, nullptr, nullptr},
	{"Unknown", field_type_t::u8, 1, 1, nullptr, nullptr},
	{"Fight", field_type_t::u8, 2, 1, nullptr, nullptr},
	{"Flee", field_type_t::u8, 3, 1, nullptr, nullptr},
	{"Alarm", field_type_t::u8, 4, 1, nullptr, nullptr},
	{"Unknown2", field_type_t::u8, 5, 1, nullptr, nullptr},
	{"Unknown3", field_type_t::u8, 6, 1, nullptr, nullptr},
	{"Unknown4", field_type_t::u8, 7, 1, nullptr, nullptr},
	{"Services", field_type_t::flags_u32, 8, 4, nullptr, aidt_flags},
};

static const char * const weapon_types[] = {
	"Short Blade 1H", "Long Blade 1H", "Long Blade 2H", "Blunt 1H",
	"Blunt 2H Close", "Blunt 2H Wide", "Spear 2H", "Axe 1H",
	"Axe 2H", "Marksman Bow", "Marksman Crossbow", "Marksman Thrown",
	"Arrow", "Bolt", nullptr
};

static const field_def_t weap_wpdt_fields[] = {
	{"Weight", field_type_t::f32, 0, 4, nullptr, nullptr},
	{"Value", field_type_t::u32, 4, 4, nullptr, nullptr},
	{"Type", field_type_t::enum_u16, 8, 2, weapon_types, nullptr},
	{"Health", field_type_t::u16, 10, 2, nullptr, nullptr},
	{"Speed", field_type_t::f32, 12, 4, nullptr, nullptr},
	{"Reach", field_type_t::f32, 16, 4, nullptr, nullptr},
	{"Enchant Pts", field_type_t::u16, 20, 2, nullptr, nullptr},
	{"Chop Min", field_type_t::u8, 22, 1, nullptr, nullptr},
	{"Chop Max", field_type_t::u8, 23, 1, nullptr, nullptr},
	{"Slash Min", field_type_t::u8, 24, 1, nullptr, nullptr},
	{"Slash Max", field_type_t::u8, 25, 1, nullptr, nullptr},
	{"Thrust Min", field_type_t::u8, 26, 1, nullptr, nullptr},
	{"Thrust Max", field_type_t::u8, 27, 1, nullptr, nullptr},
	{"Flags", field_type_t::flags_u32, 28, 4, nullptr, nullptr},
};

static const char * const armor_types[] = {
	"Helmet", "Cuirass", "Left Pauldron", "Right Pauldron",
	"Greaves", "Boots", "Left Gauntlet", "Right Gauntlet",
	"Shield", "Left Bracer", "Right Bracer", nullptr
};

static const field_def_t armo_aodt_fields[] = {
	{"Type", field_type_t::enum_u32, 0, 4, armor_types, nullptr},
	{"Weight", field_type_t::f32, 4, 4, nullptr, nullptr},
	{"Value", field_type_t::u32, 8, 4, nullptr, nullptr},
	{"Health", field_type_t::u32, 12, 4, nullptr, nullptr},
	{"Enchant Pts", field_type_t::u32, 16, 4, nullptr, nullptr},
	{"Armor Rating", field_type_t::u32, 20, 4, nullptr, nullptr},
};

static const field_def_t alch_aldt_fields[] = {
	{"Weight", field_type_t::f32, 0, 4, nullptr, nullptr},
	{"Value", field_type_t::u32, 4, 4, nullptr, nullptr},
	{"Flags", field_type_t::flags_u32, 8, 4, nullptr, nullptr},
};

static const char * const ench_types[] = {
	"Cast Once", "Cast on Strike", "Cast when Used", "Constant Effect", nullptr
};

static const field_def_t ench_endt_fields[] = {
	{"Type", field_type_t::enum_u32, 0, 4, ench_types, nullptr},
	{"Cost", field_type_t::u32, 4, 4, nullptr, nullptr},
	{"Charge", field_type_t::u32, 8, 4, nullptr, nullptr},
	{"Flags", field_type_t::flags_u32, 12, 4, nullptr, nullptr},
};

static const char * const spell_effect_range[] = {
	"Self", "Touch", "Target", nullptr
};

static const field_def_t enam_fields[] = {
	{"Effect ID", field_type_t::u16, 0, 2, nullptr, nullptr},
	{"Skill ID", field_type_t::i8, 2, 1, nullptr, nullptr},
	{"Attribute ID", field_type_t::i8, 3, 1, nullptr, nullptr},
	{"Range", field_type_t::enum_u32, 4, 4, spell_effect_range, nullptr},
	{"Area", field_type_t::u32, 8, 4, nullptr, nullptr},
	{"Duration", field_type_t::u32, 12, 4, nullptr, nullptr},
	{"Mag Min", field_type_t::u32, 16, 4, nullptr, nullptr},
	{"Mag Max", field_type_t::u32, 20, 4, nullptr, nullptr},
};

static const field_def_t book_bkdt_fields[] = {
	{"Weight", field_type_t::f32, 0, 4, nullptr, nullptr},
	{"Value", field_type_t::u32, 4, 4, nullptr, nullptr},
	{"Scroll", field_type_t::u32, 8, 4, nullptr, nullptr},
	{"Skill", field_type_t::i32, 12, 4, nullptr, nullptr},
	{"Enchant Pts", field_type_t::u32, 16, 4, nullptr, nullptr},
};

static const char * const creature_types[] = {
	"Creature", "Daedra", "Undead", "Humanoid", nullptr
};

static const field_def_t crea_npdt_fields[] = {
	{"Type", field_type_t::enum_u32, 0, 4, creature_types, nullptr},
	{"Level", field_type_t::u32, 4, 4, nullptr, nullptr},
	{"Strength", field_type_t::u32, 8, 4, nullptr, nullptr},
	{"Intelligence", field_type_t::u32, 12, 4, nullptr, nullptr},
	{"Willpower", field_type_t::u32, 16, 4, nullptr, nullptr},
	{"Agility", field_type_t::u32, 20, 4, nullptr, nullptr},
	{"Speed", field_type_t::u32, 24, 4, nullptr, nullptr},
	{"Endurance", field_type_t::u32, 28, 4, nullptr, nullptr},
	{"Personality", field_type_t::u32, 32, 4, nullptr, nullptr},
	{"Luck", field_type_t::u32, 36, 4, nullptr, nullptr},
	{"Health", field_type_t::u32, 40, 4, nullptr, nullptr},
	{"Magicka", field_type_t::u32, 44, 4, nullptr, nullptr},
	{"Fatigue", field_type_t::u32, 48, 4, nullptr, nullptr},
	{"Soul", field_type_t::u32, 52, 4, nullptr, nullptr},
	{"Combat", field_type_t::u32, 56, 4, nullptr, nullptr},
	{"Magic", field_type_t::u32, 60, 4, nullptr, nullptr},
	{"Stealth", field_type_t::u32, 64, 4, nullptr, nullptr},
	{"Attack 1 Min", field_type_t::u32, 68, 4, nullptr, nullptr},
	{"Attack 1 Max", field_type_t::u32, 72, 4, nullptr, nullptr},
	{"Attack 2 Min", field_type_t::u32, 76, 4, nullptr, nullptr},
	{"Attack 2 Max", field_type_t::u32, 80, 4, nullptr, nullptr},
	{"Attack 3 Min", field_type_t::u32, 84, 4, nullptr, nullptr},
	{"Attack 3 Max", field_type_t::u32, 88, 4, nullptr, nullptr},
	{"Gold", field_type_t::u32, 92, 4, nullptr, nullptr},
};

static const field_def_t cont_cndt_fields[] = {
	{"Weight", field_type_t::f32, 0, 4, nullptr, nullptr},
};

static const field_def_t fact_fadt_fields[] = {
	{"Attribute 1", field_type_t::u32, 0, 4, nullptr, nullptr},
	{"Attribute 2", field_type_t::u32, 4, 4, nullptr, nullptr},
	{"Rank Data", field_type_t::raw, 8, 200, nullptr, nullptr},
	{"Skill 1", field_type_t::i32, 208, 4, nullptr, nullptr},
	{"Skill 2", field_type_t::i32, 212, 4, nullptr, nullptr},
	{"Skill 3", field_type_t::i32, 216, 4, nullptr, nullptr},
	{"Skill 4", field_type_t::i32, 220, 4, nullptr, nullptr},
	{"Skill 5", field_type_t::i32, 224, 4, nullptr, nullptr},
	{"Skill 6", field_type_t::i32, 228, 4, nullptr, nullptr},
	{"Unknown", field_type_t::u32, 232, 4, nullptr, nullptr},
	{"Flags", field_type_t::flags_u32, 236, 4, nullptr, nullptr},
};

static const char * const class_specializations[] = {
	"Combat", "Magic", "Stealth", nullptr
};

static const field_def_t clas_cldt_fields[] = {
	{"Attribute 1", field_type_t::u32, 0, 4, nullptr, nullptr},
	{"Attribute 2", field_type_t::u32, 4, 4, nullptr, nullptr},
	{"Specialization", field_type_t::enum_u32, 8, 4, class_specializations, nullptr},
	{"Minor 1", field_type_t::u32, 12, 4, nullptr, nullptr},
	{"Major 1", field_type_t::u32, 16, 4, nullptr, nullptr},
	{"Minor 2", field_type_t::u32, 20, 4, nullptr, nullptr},
	{"Major 2", field_type_t::u32, 24, 4, nullptr, nullptr},
	{"Minor 3", field_type_t::u32, 28, 4, nullptr, nullptr},
	{"Major 3", field_type_t::u32, 32, 4, nullptr, nullptr},
	{"Minor 4", field_type_t::u32, 36, 4, nullptr, nullptr},
	{"Major 4", field_type_t::u32, 40, 4, nullptr, nullptr},
	{"Minor 5", field_type_t::u32, 44, 4, nullptr, nullptr},
	{"Major 5", field_type_t::u32, 48, 4, nullptr, nullptr},
	{"Flags", field_type_t::flags_u32, 52, 4, nullptr, nullptr},
	{"Auto Calc", field_type_t::u32, 56, 4, nullptr, nullptr},
};

static const field_def_t race_radt_fields[] = {
	{"Skill Bonuses", field_type_t::raw, 0, 56, nullptr, nullptr},
	{"Strength M", field_type_t::u32, 56, 4, nullptr, nullptr},
	{"Strength F", field_type_t::u32, 60, 4, nullptr, nullptr},
	{"Intelligence M", field_type_t::u32, 64, 4, nullptr, nullptr},
	{"Intelligence F", field_type_t::u32, 68, 4, nullptr, nullptr},
	{"Willpower M", field_type_t::u32, 72, 4, nullptr, nullptr},
	{"Willpower F", field_type_t::u32, 76, 4, nullptr, nullptr},
	{"Agility M", field_type_t::u32, 80, 4, nullptr, nullptr},
	{"Agility F", field_type_t::u32, 84, 4, nullptr, nullptr},
	{"Speed M", field_type_t::u32, 88, 4, nullptr, nullptr},
	{"Speed F", field_type_t::u32, 92, 4, nullptr, nullptr},
	{"Endurance M", field_type_t::u32, 96, 4, nullptr, nullptr},
	{"Endurance F", field_type_t::u32, 100, 4, nullptr, nullptr},
	{"Personality M", field_type_t::u32, 104, 4, nullptr, nullptr},
	{"Personality F", field_type_t::u32, 108, 4, nullptr, nullptr},
	{"Luck M", field_type_t::u32, 112, 4, nullptr, nullptr},
	{"Luck F", field_type_t::u32, 116, 4, nullptr, nullptr},
	{"Height M", field_type_t::f32, 120, 4, nullptr, nullptr},
	{"Height F", field_type_t::f32, 124, 4, nullptr, nullptr},
	{"Weight M", field_type_t::f32, 128, 4, nullptr, nullptr},
	{"Weight F", field_type_t::f32, 132, 4, nullptr, nullptr},
	{"Flags", field_type_t::flags_u32, 136, 4, nullptr, nullptr},
};

static const char * const leveled_flags[] = {
	"Calc from All Levels", "Calc for Each Item", nullptr
};

static const field_def_t levi_data_fields[] = {
	{"Flags", field_type_t::flags_u32, 0, 4, nullptr, leveled_flags},
};

static const field_def_t gmst_strv_fields[] = {
	{"Value", field_type_t::string_var, 0, 0, nullptr, nullptr},
};

static const field_def_t gmst_intv_fields[] = {
	{"Value", field_type_t::i32, 0, 4, nullptr, nullptr},
};

static const field_def_t gmst_fltv_fields[] = {
	{"Value", field_type_t::f32, 0, 4, nullptr, nullptr},
};

static const field_def_t info_data_fields[] = {
	{"Unknown1", field_type_t::u32, 0, 4, nullptr, nullptr},
	{"Disposition", field_type_t::u32, 4, 4, nullptr, nullptr},
	{"Rank", field_type_t::u8, 8, 1, nullptr, nullptr},
	{"Gender", field_type_t::u8, 9, 1, nullptr, nullptr},
	{"PC Rank", field_type_t::u8, 10, 1, nullptr, nullptr},
	{"Unknown2", field_type_t::u8, 11, 1, nullptr, nullptr},
};

static const field_def_t regn_weat_fields[] = {
	{"Clear", field_type_t::u8, 0, 1, nullptr, nullptr},
	{"Cloudy", field_type_t::u8, 1, 1, nullptr, nullptr},
	{"Foggy", field_type_t::u8, 2, 1, nullptr, nullptr},
	{"Overcast", field_type_t::u8, 3, 1, nullptr, nullptr},
	{"Rain", field_type_t::u8, 4, 1, nullptr, nullptr},
	{"Thunder", field_type_t::u8, 5, 1, nullptr, nullptr},
	{"Ash", field_type_t::u8, 6, 1, nullptr, nullptr},
	{"Blight", field_type_t::u8, 7, 1, nullptr, nullptr},
};

static const char * const spell_types[] = {
	"Spell", "Ability", "Blight", "Disease",
	"Curse", "Power", nullptr
};

static const char * const spell_flags[] = {
	"Auto Calc Cost", "PC Start Spell", "Always Succeeds", nullptr
};

static const field_def_t spel_spdt_fields[] = {
	{"Type", field_type_t::enum_u32, 0, 4, spell_types, nullptr},
	{"Cost", field_type_t::u32, 4, 4, nullptr, nullptr},
	{"Flags", field_type_t::flags_u32, 8, 4, nullptr, spell_flags},
};

static const std::vector<sub_record_schema_t> & build_schemas()
{
	static const std::vector<sub_record_schema_t> schemas = {
		{"CELL", "DATA", 12, cell_data_fields, std::size(cell_data_fields)},
		{"NPC_", "FLAG", 4, npc_flag_fields, std::size(npc_flag_fields)},
		{"NPC_", "NPDT", 12, npc_npdt_12_fields, std::size(npc_npdt_12_fields)},
		{"NPC_", "NPDT", 52, npc_npdt_52_fields, std::size(npc_npdt_52_fields)},
		{"NPC_", "AIDT", 12, npc_aidt_fields, std::size(npc_aidt_fields)},
		{"WEAP", "WPDT", 32, weap_wpdt_fields, std::size(weap_wpdt_fields)},
		{"ARMO", "AODT", 24, armo_aodt_fields, std::size(armo_aodt_fields)},
		{"ALCH", "ALDT", 12, alch_aldt_fields, std::size(alch_aldt_fields)},
		{"ENCH", "ENDT", 16, ench_endt_fields, std::size(ench_endt_fields)},
		{"*",    "ENAM", 24, enam_fields, std::size(enam_fields)},
		{"BOOK", "BKDT", 20, book_bkdt_fields, std::size(book_bkdt_fields)},
		{"CREA", "NPDT", 96, crea_npdt_fields, std::size(crea_npdt_fields)},
		{"CONT", "CNDT", 4, cont_cndt_fields, std::size(cont_cndt_fields)},
		{"FACT", "FADT", 240, fact_fadt_fields, std::size(fact_fadt_fields)},
		{"CLAS", "CLDT", 60, clas_cldt_fields, std::size(clas_cldt_fields)},
		{"RACE", "RADT", 140, race_radt_fields, std::size(race_radt_fields)},
		{"LEVI", "DATA", 4, levi_data_fields, std::size(levi_data_fields)},
		{"LEVC", "DATA", 4, levi_data_fields, std::size(levi_data_fields)},
		{"GMST", "STRV", 0, gmst_strv_fields, std::size(gmst_strv_fields)},
		{"GMST", "INTV", 4, gmst_intv_fields, std::size(gmst_intv_fields)},
		{"GMST", "FLTV", 4, gmst_fltv_fields, std::size(gmst_fltv_fields)},
		{"INFO", "DATA", 12, info_data_fields, std::size(info_data_fields)},
		{"REGN", "WEAT", 8, regn_weat_fields, std::size(regn_weat_fields)},
		{"SPEL", "SPDT", 12, spel_spdt_fields, std::size(spel_spdt_fields)},
	};
	return schemas;
}

const sub_record_schema_t * find_schema(const std::string & record_type,
                                         const std::string & sub_type,
                                         size_t data_size)
{
	const auto & schemas = build_schemas();
	for (const auto & s : schemas)
	{
		if (s.sub_type != sub_type)
			continue;

		if (std::strcmp(s.parent_type, "*") != 0 && s.parent_type != record_type)
			continue;

		if (s.expected_size != 0 && s.expected_size != data_size)
			continue;

		return &s;
	}
	return nullptr;
}

const std::vector<sub_record_schema_t> & all_schemas()
{
	return build_schemas();
}
