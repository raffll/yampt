#include "sub_record_schema.hpp"
#include <cstring>

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

static const char * const cell_flags[] = {
	"Interior", "Has Water", "Illegal to Sleep", "_", "_", "_", "_", "Behave like Exterior",
};

static const field_def_t cell_data_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, cell_flags, ARRAY_COUNT(cell_flags) },
	{ "Grid X", field_type_t::i32, 4, 4, nullptr, nullptr },
	{ "Grid Y", field_type_t::i32, 8, 4, nullptr, nullptr },
};

static const char * const npc_flags[] = { "Female", "Essential", "Respawn", "_", "Autocalc" };

static const field_def_t npc_flag_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, npc_flags, ARRAY_COUNT(npc_flags) },
};

static const field_def_t npc_npdt_12_fields[] = {
	{ "Level", field_type_t::u16, 0, 2, nullptr, nullptr },
	{ "Disposition", field_type_t::u8, 2, 1, nullptr, nullptr },
	{ "Reputation", field_type_t::u8, 3, 1, nullptr, nullptr },
	{ "Rank", field_type_t::u8, 4, 1, nullptr, nullptr },
	{ "Unknown1", field_type_t::u8, 5, 1, nullptr, nullptr },
	{ "Unknown2", field_type_t::u8, 6, 1, nullptr, nullptr },
	{ "Unknown3", field_type_t::u8, 7, 1, nullptr, nullptr },
	{ "Gold", field_type_t::u32, 8, 4, nullptr, nullptr },
};

static const field_def_t npc_npdt_52_fields[] = {
	{ "Level", field_type_t::u16, 0, 2, nullptr, nullptr },
	{ "Strength", field_type_t::u8, 2, 1, nullptr, nullptr },
	{ "Intelligence", field_type_t::u8, 3, 1, nullptr, nullptr },
	{ "Willpower", field_type_t::u8, 4, 1, nullptr, nullptr },
	{ "Agility", field_type_t::u8, 5, 1, nullptr, nullptr },
	{ "Speed", field_type_t::u8, 6, 1, nullptr, nullptr },
	{ "Endurance", field_type_t::u8, 7, 1, nullptr, nullptr },
	{ "Personality", field_type_t::u8, 8, 1, nullptr, nullptr },
	{ "Luck", field_type_t::u8, 9, 1, nullptr, nullptr },
	{ "Skills", field_type_t::raw, 10, 27, nullptr, nullptr },
	{ "Reputation", field_type_t::u8, 37, 1, nullptr, nullptr },
	{ "Health", field_type_t::u16, 38, 2, nullptr, nullptr },
	{ "Magicka", field_type_t::u16, 40, 2, nullptr, nullptr },
	{ "Fatigue", field_type_t::u16, 42, 2, nullptr, nullptr },
	{ "Disposition", field_type_t::u8, 44, 1, nullptr, nullptr },
	{ "Faction ID", field_type_t::u8, 45, 1, nullptr, nullptr },
	{ "Rank", field_type_t::u8, 46, 1, nullptr, nullptr },
	{ "Unknown", field_type_t::u8, 47, 1, nullptr, nullptr },
	{ "Gold", field_type_t::u32, 48, 4, nullptr, nullptr },
};

static const char * const aidt_flags[] = { "Weapon",        "Armor",      "Clothing",    "Books",     "Ingredients",
	                                       "Picks",         "Probes",     "Lights",      "Apparatus", "Repair Items",
	                                       "Miscellaneous", "Spells",     "Magic Items", "Potions",   "Training",
	                                       "Spellmaking",   "Enchanting" };

static const field_def_t npc_aidt_fields[] = {
	{ "Hello", field_type_t::u16, 0, 2, nullptr, nullptr },
	{ "Fight", field_type_t::u8, 2, 1, nullptr, nullptr },
	{ "Flee", field_type_t::u8, 3, 1, nullptr, nullptr },
	{ "Alarm", field_type_t::u8, 4, 1, nullptr, nullptr },
	{ "Services", field_type_t::flags_u32, 8, 4, nullptr, aidt_flags, ARRAY_COUNT(aidt_flags) },
};

static const char * const weapon_types[] = {
	"Short Blade 1H",    "Long Blade 1H",   "Long Blade 2H", "Blunt 1H", "Blunt 2H Close",
	"Blunt 2H Wide",     "Spear 2H",        "Axe 1H",        "Axe 2H",   "Marksman Bow",
	"Marksman Crossbow", "Marksman Thrown", "Arrow",         "Bolt",     nullptr
};

static const field_def_t weap_wpdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Type", field_type_t::enum_u16, 8, 2, weapon_types, nullptr },
	{ "Health", field_type_t::u16, 10, 2, nullptr, nullptr },
	{ "Speed", field_type_t::f32, 12, 4, nullptr, nullptr },
	{ "Reach", field_type_t::f32, 16, 4, nullptr, nullptr },
	{ "Enchant Pts", field_type_t::u16, 20, 2, nullptr, nullptr },
	{ "Chop Min", field_type_t::u8, 22, 1, nullptr, nullptr },
	{ "Chop Max", field_type_t::u8, 23, 1, nullptr, nullptr },
	{ "Slash Min", field_type_t::u8, 24, 1, nullptr, nullptr },
	{ "Slash Max", field_type_t::u8, 25, 1, nullptr, nullptr },
	{ "Thrust Min", field_type_t::u8, 26, 1, nullptr, nullptr },
	{ "Thrust Max", field_type_t::u8, 27, 1, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 28, 4, nullptr, nullptr },
};

static const char * const armor_types[] = { "Helmet",  "Cuirass",     "Left Pauldron", "Right Pauldron",
	                                        "Greaves", "Boots",       "Left Gauntlet", "Right Gauntlet",
	                                        "Shield",  "Left Bracer", "Right Bracer",  nullptr };

static const field_def_t armo_aodt_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, armor_types, nullptr },
	{ "Weight", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Health", field_type_t::u32, 12, 4, nullptr, nullptr },
	{ "Enchant Pts", field_type_t::u32, 16, 4, nullptr, nullptr },
	{ "Armor Rating", field_type_t::u32, 20, 4, nullptr, nullptr },
};

static const field_def_t alch_aldt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 8, 4, nullptr, nullptr },
};

static const char * const ench_types[] = { "Cast Once",
	                                       "Cast on Strike",
	                                       "Cast when Used",
	                                       "Constant Effect",
	                                       nullptr };

static const field_def_t ench_endt_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, ench_types, nullptr },
	{ "Cost", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Charge", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 12, 4, nullptr, nullptr },
};

static const char * const spell_effect_range[] = { "Self", "Touch", "Target", nullptr };

static const char * const attribute_names[] = { "Strength",    "Intelligence", "Willpower", "Agility",
	                                            "Speed",       "Endurance",    "Personality", "Luck", nullptr };

static const char * const skill_names[] = {
	"Block",       "Armorer",      "Medium Armor", "Heavy Armor",  "Blunt Weapon",
	"Long Blade",  "Axe",          "Spear",        "Athletics",    "Enchant",
	"Destruction", "Alteration",   "Illusion",     "Conjuration",  "Mysticism",
	"Restoration", "Alchemy",      "Unarmored",    "Security",     "Sneak",
	"Acrobatics",  "Light Armor",  "Short Blade",  "Marksman",     "Mercantile",
	"Speechcraft", "Hand-to-hand", nullptr
};

static const char * const effect_names[] = {
	"Water Breathing", "Swift Swim", "Water Walking", "Shield", "Fire Shield",
	"Lightning Shield", "Frost Shield", "Burden", "Feather", "Jump",
	"Levitate", "Slow Fall", "Lock", "Open", "Fire Damage",
	"Shock Damage", "Frost Damage", "Drain Attribute", "Drain Health", "Drain Magicka",
	"Drain Fatigue", "Drain Skill", "Damage Attribute", "Damage Health", "Damage Magicka",
	"Damage Fatigue", "Damage Skill", "Poison", "Weakness to Fire", "Weakness to Frost",
	"Weakness to Shock", "Weakness to Magicka", "Weakness to Common Disease", "Weakness to Blight Disease",
	"Weakness to Corprus Disease", "Weakness to Poison", "Weakness to Normal Weapons",
	"Disintegrate Weapon", "Disintegrate Armor", "Invisibility", "Chameleon", "Light",
	"Sanctuary", "Night Eye", "Charm", "Paralyze", "Silence",
	"Blind", "Sound", "Calm Humanoid", "Calm Creature", "Frenzy Humanoid",
	"Frenzy Creature", "Demoralize Humanoid", "Demoralize Creature", "Rally Humanoid",
	"Rally Creature", "Dispel", "Soultrap", "Telekinesis", "Mark",
	"Recall", "Divine Intervention", "Almsivi Intervention", "Detect Animal",
	"Detect Enchantment", "Detect Key", "Spell Absorption", "Reflect",
	"Cure Common Disease", "Cure Blight Disease", "Cure Corprus Disease", "Cure Poison",
	"Cure Paralyzation", "Restore Attribute", "Restore Health", "Restore Magicka",
	"Restore Fatigue", "Restore Skill", "Fortify Attribute", "Fortify Health",
	"Fortify Magicka", "Fortify Fatigue", "Fortify Skill", "Fortify Maximum Magicka",
	"Absorb Attribute", "Absorb Health", "Absorb Magicka", "Absorb Fatigue",
	"Absorb Skill", "Resist Fire", "Resist Frost", "Resist Shock",
	"Resist Magicka", "Resist Common Disease", "Resist Blight Disease", "Resist Corprus Disease",
	"Resist Poison", "Resist Normal Weapons", "Resist Paralysis", "Remove Curse",
	"Turn Undead", "Summon Scamp", "Summon Clannfear", "Summon Daedroth",
	"Summon Dremora", "Summon Ancestral Ghost", "Summon Skeletal Minion", "Summon Bonewalker",
	"Summon Greater Bonewalker", "Summon Bonelord", "Summon Winged Twilight", "Summon Hunger",
	"Summon Golden Saint", "Summon Flame Atronach", "Summon Frost Atronach", "Summon Storm Atronach",
	"Fortify Attack", "Command Creature", "Command Humanoid", "Bound Dagger",
	"Bound Longsword", "Bound Mace", "Bound Battle Axe", "Bound Spear",
	"Bound Longbow", "Extra Spell", "Bound Cuirass", "Bound Helm",
	"Bound Boots", "Bound Shield", "Bound Gloves", "Corprus",
	"Vampirism", "Summon Centurion Sphere", "Sun Damage", "Stunted Magicka",
	"Summon Fabricant", "Summon Wolf", "Summon Bear", "Summon Bonewolf",
	"Summon Creature 04", "Summon Creature 05", nullptr
};

static const field_def_t enam_fields[] = {
	{ "Effect ID", field_type_t::enum_u16, 0, 2, effect_names, nullptr },
	{ "Skill ID", field_type_t::i8, 2, 1, skill_names, nullptr },
	{ "Attribute ID", field_type_t::i8, 3, 1, attribute_names, nullptr },
	{ "Range", field_type_t::enum_u32, 4, 4, spell_effect_range, nullptr },
	{ "Area", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Duration", field_type_t::u32, 12, 4, nullptr, nullptr },
	{ "Mag Min", field_type_t::u32, 16, 4, nullptr, nullptr },
	{ "Mag Max", field_type_t::u32, 20, 4, nullptr, nullptr },
};

static const field_def_t book_bkdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Scroll", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Skill", field_type_t::i32, 12, 4, nullptr, nullptr },
	{ "Enchant Pts", field_type_t::u32, 16, 4, nullptr, nullptr },
};

static const char * const creature_types[] = { "Creature", "Daedra", "Undead", "Humanoid", nullptr };

static const field_def_t crea_npdt_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, creature_types, nullptr },
	{ "Level", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Strength", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Intelligence", field_type_t::u32, 12, 4, nullptr, nullptr },
	{ "Willpower", field_type_t::u32, 16, 4, nullptr, nullptr },
	{ "Agility", field_type_t::u32, 20, 4, nullptr, nullptr },
	{ "Speed", field_type_t::u32, 24, 4, nullptr, nullptr },
	{ "Endurance", field_type_t::u32, 28, 4, nullptr, nullptr },
	{ "Personality", field_type_t::u32, 32, 4, nullptr, nullptr },
	{ "Luck", field_type_t::u32, 36, 4, nullptr, nullptr },
	{ "Health", field_type_t::u32, 40, 4, nullptr, nullptr },
	{ "Magicka", field_type_t::u32, 44, 4, nullptr, nullptr },
	{ "Fatigue", field_type_t::u32, 48, 4, nullptr, nullptr },
	{ "Soul", field_type_t::u32, 52, 4, nullptr, nullptr },
	{ "Combat", field_type_t::u32, 56, 4, nullptr, nullptr },
	{ "Magic", field_type_t::u32, 60, 4, nullptr, nullptr },
	{ "Stealth", field_type_t::u32, 64, 4, nullptr, nullptr },
	{ "Attack 1 Min", field_type_t::u32, 68, 4, nullptr, nullptr },
	{ "Attack 1 Max", field_type_t::u32, 72, 4, nullptr, nullptr },
	{ "Attack 2 Min", field_type_t::u32, 76, 4, nullptr, nullptr },
	{ "Attack 2 Max", field_type_t::u32, 80, 4, nullptr, nullptr },
	{ "Attack 3 Min", field_type_t::u32, 84, 4, nullptr, nullptr },
	{ "Attack 3 Max", field_type_t::u32, 88, 4, nullptr, nullptr },
	{ "Gold", field_type_t::u32, 92, 4, nullptr, nullptr },
};

static const field_def_t cont_cndt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
};

static const field_def_t fact_fadt_fields[] = {
	{ "Attribute 1", field_type_t::u32, 0, 4, nullptr, nullptr },
	{ "Attribute 2", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Rank Data", field_type_t::raw, 8, 200, nullptr, nullptr },
	{ "Skill 1", field_type_t::i32, 208, 4, nullptr, nullptr },
	{ "Skill 2", field_type_t::i32, 212, 4, nullptr, nullptr },
	{ "Skill 3", field_type_t::i32, 216, 4, nullptr, nullptr },
	{ "Skill 4", field_type_t::i32, 220, 4, nullptr, nullptr },
	{ "Skill 5", field_type_t::i32, 224, 4, nullptr, nullptr },
	{ "Skill 6", field_type_t::i32, 228, 4, nullptr, nullptr },
	{ "Unknown", field_type_t::u32, 232, 4, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 236, 4, nullptr, nullptr },
};

static const char * const class_specializations[] = { "Combat", "Magic", "Stealth", nullptr };

static const char * const class_flags[] = { "Playable" };

static const field_def_t clas_cldt_fields[] = {
	{ "Attribute 1", field_type_t::enum_u32, 0, 4, attribute_names, nullptr },
	{ "Attribute 2", field_type_t::enum_u32, 4, 4, attribute_names, nullptr },
	{ "Specialization", field_type_t::enum_u32, 8, 4, class_specializations, nullptr },
	{ "Major 1", field_type_t::enum_u32, 16, 4, skill_names, nullptr },
	{ "Major 2", field_type_t::enum_u32, 24, 4, skill_names, nullptr },
	{ "Major 3", field_type_t::enum_u32, 32, 4, skill_names, nullptr },
	{ "Major 4", field_type_t::enum_u32, 40, 4, skill_names, nullptr },
	{ "Major 5", field_type_t::enum_u32, 48, 4, skill_names, nullptr },
	{ "Minor 1", field_type_t::enum_u32, 12, 4, skill_names, nullptr },
	{ "Minor 2", field_type_t::enum_u32, 20, 4, skill_names, nullptr },
	{ "Minor 3", field_type_t::enum_u32, 28, 4, skill_names, nullptr },
	{ "Minor 4", field_type_t::enum_u32, 36, 4, skill_names, nullptr },
	{ "Minor 5", field_type_t::enum_u32, 44, 4, skill_names, nullptr },
	{ "Flags", field_type_t::flags_u32, 52, 4, nullptr, class_flags, ARRAY_COUNT(class_flags) },
	{ "Auto Calc", field_type_t::u32, 56, 4, nullptr, nullptr },
};

static const field_def_t race_radt_fields[] = {
	{ "Skill Bonuses", field_type_t::raw, 0, 56, nullptr, nullptr },
	{ "Strength M", field_type_t::u32, 56, 4, nullptr, nullptr },
	{ "Strength F", field_type_t::u32, 60, 4, nullptr, nullptr },
	{ "Intelligence M", field_type_t::u32, 64, 4, nullptr, nullptr },
	{ "Intelligence F", field_type_t::u32, 68, 4, nullptr, nullptr },
	{ "Willpower M", field_type_t::u32, 72, 4, nullptr, nullptr },
	{ "Willpower F", field_type_t::u32, 76, 4, nullptr, nullptr },
	{ "Agility M", field_type_t::u32, 80, 4, nullptr, nullptr },
	{ "Agility F", field_type_t::u32, 84, 4, nullptr, nullptr },
	{ "Speed M", field_type_t::u32, 88, 4, nullptr, nullptr },
	{ "Speed F", field_type_t::u32, 92, 4, nullptr, nullptr },
	{ "Endurance M", field_type_t::u32, 96, 4, nullptr, nullptr },
	{ "Endurance F", field_type_t::u32, 100, 4, nullptr, nullptr },
	{ "Personality M", field_type_t::u32, 104, 4, nullptr, nullptr },
	{ "Personality F", field_type_t::u32, 108, 4, nullptr, nullptr },
	{ "Luck M", field_type_t::u32, 112, 4, nullptr, nullptr },
	{ "Luck F", field_type_t::u32, 116, 4, nullptr, nullptr },
	{ "Height M", field_type_t::f32, 120, 4, nullptr, nullptr },
	{ "Height F", field_type_t::f32, 124, 4, nullptr, nullptr },
	{ "Weight M", field_type_t::f32, 128, 4, nullptr, nullptr },
	{ "Weight F", field_type_t::f32, 132, 4, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 136, 4, nullptr, nullptr },
};

static const char * const leveled_flags[] = { "Calc from All Levels", "Calc for Each Item" };

static const field_def_t levi_data_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, leveled_flags, ARRAY_COUNT(leveled_flags) },
};

static const field_def_t gmst_strv_fields[] = {
	{ "Value", field_type_t::string_var, 0, 0, nullptr, nullptr },
};

static const field_def_t gmst_intv_fields[] = {
	{ "Value", field_type_t::i32, 0, 4, nullptr, nullptr },
};

static const field_def_t gmst_fltv_fields[] = {
	{ "Value", field_type_t::f32, 0, 4, nullptr, nullptr },
};

static const field_def_t info_data_fields[] = {
	{ "Unknown1", field_type_t::u32, 0, 4, nullptr, nullptr },
	{ "Disposition", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Rank", field_type_t::u8, 8, 1, nullptr, nullptr },
	{ "Gender", field_type_t::u8, 9, 1, nullptr, nullptr },
	{ "PC Rank", field_type_t::u8, 10, 1, nullptr, nullptr },
	{ "Unknown2", field_type_t::u8, 11, 1, nullptr, nullptr },
};

static const field_def_t regn_weat_fields[] = {
	{ "Clear", field_type_t::u8, 0, 1, nullptr, nullptr }, { "Cloudy", field_type_t::u8, 1, 1, nullptr, nullptr },
	{ "Foggy", field_type_t::u8, 2, 1, nullptr, nullptr }, { "Overcast", field_type_t::u8, 3, 1, nullptr, nullptr },
	{ "Rain", field_type_t::u8, 4, 1, nullptr, nullptr },  { "Thunder", field_type_t::u8, 5, 1, nullptr, nullptr },
	{ "Ash", field_type_t::u8, 6, 1, nullptr, nullptr },   { "Blight", field_type_t::u8, 7, 1, nullptr, nullptr },
};

static const char * const spell_types[] = { "Spell", "Ability", "Blight", "Disease", "Curse", "Power", nullptr };

static const char * const spell_flags[] = { "Auto Calc Cost", "PC Start Spell", "Always Succeeds" };

static const field_def_t spel_spdt_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, spell_types, nullptr },
	{ "Cost", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 8, 4, nullptr, spell_flags, ARRAY_COUNT(spell_flags) },
};

static const field_def_t npco_fields[] = {
	{ "Count", field_type_t::i32, 0, 4, nullptr, nullptr },
	{ "Item ID", field_type_t::string_fixed, 4, 32, nullptr, nullptr },
};

static const char * const mgef_schools[] = { "Alteration", "Conjuration", "Destruction", "Illusion",
	                                         "Mysticism",  "Restoration", nullptr };

static const char * const mgef_flags[] = {
	"_", "_", "_", "_", "_", "_", "_", "_", "_", "Spellmaking", "Enchanting", "Negative",
};

static const field_def_t mgef_medt_fields[] = {
	{ "School", field_type_t::enum_u32, 0, 4, mgef_schools, nullptr },
	{ "Base Cost", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 8, 4, nullptr, mgef_flags, ARRAY_COUNT(mgef_flags) },
	{ "Red", field_type_t::u32, 12, 4, nullptr, nullptr },
	{ "Blue", field_type_t::u32, 16, 4, nullptr, nullptr },
	{ "Green", field_type_t::u32, 20, 4, nullptr, nullptr },
	{ "Speed", field_type_t::f32, 24, 4, nullptr, nullptr },
	{ "Size", field_type_t::f32, 28, 4, nullptr, nullptr },
	{ "Size Cap", field_type_t::f32, 32, 4, nullptr, nullptr },
};

static const char * const skill_specializations[] = { "Combat", "Magic", "Stealth", nullptr };

static const field_def_t skil_skdt_fields[] = {
	{ "Attribute", field_type_t::u32, 0, 4, nullptr, nullptr },
	{ "Specialization", field_type_t::enum_u32, 4, 4, skill_specializations, nullptr },
	{ "Use Value 1", field_type_t::f32, 8, 4, nullptr, nullptr },
	{ "Use Value 2", field_type_t::f32, 12, 4, nullptr, nullptr },
	{ "Use Value 3", field_type_t::f32, 16, 4, nullptr, nullptr },
	{ "Use Value 4", field_type_t::f32, 20, 4, nullptr, nullptr },
};

static const field_def_t cell_ambi_fields[] = {
	{ "Ambient Color", field_type_t::u32, 0, 4, nullptr, nullptr },
	{ "Sunlight Color", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Fog Color", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Fog Density", field_type_t::f32, 12, 4, nullptr, nullptr },
};

static const field_def_t cell_dodt_fields[] = {
	{ "X Pos", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Y Pos", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Z Pos", field_type_t::f32, 8, 4, nullptr, nullptr },
	{ "X Rotate", field_type_t::f32, 12, 4, nullptr, nullptr },
	{ "Y Rotate", field_type_t::f32, 16, 4, nullptr, nullptr },
	{ "Z Rotate", field_type_t::f32, 20, 4, nullptr, nullptr },
};

static const char * const cont_flags[] = { "Organic", "Respawns", "_", "Default" };

static const field_def_t cont_flag_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, cont_flags, ARRAY_COUNT(cont_flags) },
};

static const char * const crea_flags[] = {
	"Biped", "Respawn", "Weapon and Shield", "_", "Swims", "Flies", "Walks", "_",
	"_", "_", "Skeleton Blood", "Metal Blood",
};

static const field_def_t crea_flag_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, crea_flags, ARRAY_COUNT(crea_flags) },
};

static const field_def_t levi_intv_fields[] = {
	{ "PC Level", field_type_t::u16, 0, 2, nullptr, nullptr },
};

static const char * const clothing_types[] = { "Pants",      "Shoes", "Shirt", "Belt",   "Robe", "Right Glove",
	                                           "Left Glove", "Skirt", "Ring",  "Amulet", nullptr };

static const field_def_t clot_ctdt_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, clothing_types, nullptr },
	{ "Weight", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Value", field_type_t::u16, 8, 2, nullptr, nullptr },
	{ "Enchant Pts", field_type_t::u16, 10, 2, nullptr, nullptr },
};

static const char * const light_flags[] = { "Dynamic",     "Can Carry",    "Negative", "Flicker",    "Fire",
	                                        "Off Default", "Flicker Slow", "Pulse",    "Pulse Slow" };

static const field_def_t ligh_lhdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Time", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Radius", field_type_t::u32, 12, 4, nullptr, nullptr },
	{ "Red", field_type_t::u8, 16, 1, nullptr, nullptr },
	{ "Green", field_type_t::u8, 17, 1, nullptr, nullptr },
	{ "Blue", field_type_t::u8, 18, 1, nullptr, nullptr },
	{ "Null", field_type_t::u8, 19, 1, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 20, 4, nullptr, light_flags, ARRAY_COUNT(light_flags) },
};

static const field_def_t ingr_irdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Effect 1", field_type_t::i32, 8, 4, nullptr, nullptr },
	{ "Effect 2", field_type_t::i32, 12, 4, nullptr, nullptr },
	{ "Effect 3", field_type_t::i32, 16, 4, nullptr, nullptr },
	{ "Effect 4", field_type_t::i32, 20, 4, nullptr, nullptr },
	{ "Skill 1", field_type_t::i32, 24, 4, nullptr, nullptr },
	{ "Skill 2", field_type_t::i32, 28, 4, nullptr, nullptr },
	{ "Skill 3", field_type_t::i32, 32, 4, nullptr, nullptr },
	{ "Skill 4", field_type_t::i32, 36, 4, nullptr, nullptr },
	{ "Attribute 1", field_type_t::i32, 40, 4, nullptr, nullptr },
	{ "Attribute 2", field_type_t::i32, 44, 4, nullptr, nullptr },
	{ "Attribute 3", field_type_t::i32, 48, 4, nullptr, nullptr },
	{ "Attribute 4", field_type_t::i32, 52, 4, nullptr, nullptr },
};

static const field_def_t scpt_schd_fields[] = {
	{ "Name", field_type_t::string_fixed, 0, 32, nullptr, nullptr },
	{ "Num Shorts", field_type_t::u32, 32, 4, nullptr, nullptr },
	{ "Num Longs", field_type_t::u32, 36, 4, nullptr, nullptr },
	{ "Num Floats", field_type_t::u32, 40, 4, nullptr, nullptr },
	{ "Script Data Size", field_type_t::u32, 44, 4, nullptr, nullptr },
	{ "Local Var Size", field_type_t::u32, 48, 4, nullptr, nullptr },
};

static const field_def_t misc_mcdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Unknown", field_type_t::u32, 8, 4, nullptr, nullptr },
};

static const char * const appa_types[] = { "Mortar and Pestle", "Alembic", "Calcinator", "Retort", nullptr };

static const field_def_t appa_aadt_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, appa_types, nullptr },
	{ "Quality", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Weight", field_type_t::f32, 8, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 12, 4, nullptr, nullptr },
};

static const field_def_t repa_ridt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Uses", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Quality", field_type_t::f32, 12, 4, nullptr, nullptr },
};

static const field_def_t cell_ref_data_fields[] = {
	{ "X Pos", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Y Pos", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Z Pos", field_type_t::f32, 8, 4, nullptr, nullptr },
	{ "X Rotate", field_type_t::f32, 12, 4, nullptr, nullptr },
	{ "Y Rotate", field_type_t::f32, 16, 4, nullptr, nullptr },
	{ "Z Rotate", field_type_t::f32, 20, 4, nullptr, nullptr },
};

static const char * const body_parts[] = { "Head",      "Hair",     "Neck",     "Chest",   "Groin",
	                                       "Hand",      "Wrist",    "Forearm",  "Upperarm", "Foot",
	                                       "Ankle",     "Knee",     "Upperleg", "Clavicle", "Tail", nullptr };

static const char * const body_vampire[] = { "No", "Yes", nullptr };

static const char * const body_gender[] = { "Male", "Female", nullptr };

static const char * const body_part_types[] = { "Skin", "Clothing", "Armor", nullptr };

static const field_def_t body_bydt_fields[] = {
	{ "Part", field_type_t::enum_u8, 0, 1, body_parts, nullptr },
	{ "Vampire", field_type_t::enum_u8, 1, 1, body_vampire, nullptr },
	{ "Gender", field_type_t::enum_u8, 2, 1, body_gender, nullptr },
	{ "Part Type", field_type_t::enum_u8, 3, 1, body_part_types, nullptr },
};

static const field_def_t ai_w_fields[] = {
	{ "Distance", field_type_t::u16, 0, 2, nullptr, nullptr },
	{ "Duration", field_type_t::u16, 2, 2, nullptr, nullptr },
	{ "Time of Day", field_type_t::u8, 4, 1, nullptr, nullptr },
	{ "Idle 1", field_type_t::u8, 5, 1, nullptr, nullptr },
	{ "Idle 2", field_type_t::u8, 6, 1, nullptr, nullptr },
	{ "Idle 3", field_type_t::u8, 7, 1, nullptr, nullptr },
	{ "Idle 4", field_type_t::u8, 8, 1, nullptr, nullptr },
	{ "Idle 5", field_type_t::u8, 9, 1, nullptr, nullptr },
	{ "Idle 6", field_type_t::u8, 10, 1, nullptr, nullptr },
	{ "Idle 7", field_type_t::u8, 11, 1, nullptr, nullptr },
	{ "Idle 8", field_type_t::u8, 12, 1, nullptr, nullptr },
};

static const std::vector<sub_record_schema_t> & build_schemas()
{
	static const std::vector<sub_record_schema_t> schemas = {
		{ "CELL", "DATA", 12, cell_data_fields, ARRAY_COUNT(cell_data_fields) },
		{ "CELL", "AMBI", 16, cell_ambi_fields, ARRAY_COUNT(cell_ambi_fields) },
		{ "CELL", "DODT", 24, cell_dodt_fields, ARRAY_COUNT(cell_dodt_fields) },
		{ "CELL", "DATA", 24, cell_ref_data_fields, ARRAY_COUNT(cell_ref_data_fields) },
		{ "NPC_", "FLAG", 4, npc_flag_fields, ARRAY_COUNT(npc_flag_fields) },
		{ "NPC_", "NPDT", 12, npc_npdt_12_fields, ARRAY_COUNT(npc_npdt_12_fields) },
		{ "NPC_", "NPDT", 52, npc_npdt_52_fields, ARRAY_COUNT(npc_npdt_52_fields) },
		{ "*", "AIDT", 12, npc_aidt_fields, ARRAY_COUNT(npc_aidt_fields) },
		{ "WEAP", "WPDT", 32, weap_wpdt_fields, ARRAY_COUNT(weap_wpdt_fields) },
		{ "ARMO", "AODT", 24, armo_aodt_fields, ARRAY_COUNT(armo_aodt_fields) },
		{ "ALCH", "ALDT", 12, alch_aldt_fields, ARRAY_COUNT(alch_aldt_fields) },
		{ "ENCH", "ENDT", 16, ench_endt_fields, ARRAY_COUNT(ench_endt_fields) },
		{ "*", "ENAM", 24, enam_fields, ARRAY_COUNT(enam_fields) },
		{ "*", "NPCO", 36, npco_fields, ARRAY_COUNT(npco_fields) },
		{ "BOOK", "BKDT", 20, book_bkdt_fields, ARRAY_COUNT(book_bkdt_fields) },
		{ "CREA", "NPDT", 96, crea_npdt_fields, ARRAY_COUNT(crea_npdt_fields) },
		{ "CREA", "FLAG", 4, crea_flag_fields, ARRAY_COUNT(crea_flag_fields) },
		{ "CONT", "CNDT", 4, cont_cndt_fields, ARRAY_COUNT(cont_cndt_fields) },
		{ "CONT", "FLAG", 4, cont_flag_fields, ARRAY_COUNT(cont_flag_fields) },
		{ "FACT", "FADT", 240, fact_fadt_fields, ARRAY_COUNT(fact_fadt_fields) },
		{ "CLAS", "CLDT", 60, clas_cldt_fields, ARRAY_COUNT(clas_cldt_fields) },
		{ "RACE", "RADT", 140, race_radt_fields, ARRAY_COUNT(race_radt_fields) },
		{ "LEVI", "DATA", 4, levi_data_fields, ARRAY_COUNT(levi_data_fields) },
		{ "LEVC", "DATA", 4, levi_data_fields, ARRAY_COUNT(levi_data_fields) },
		{ "LEVI", "INTV", 2, levi_intv_fields, ARRAY_COUNT(levi_intv_fields) },
		{ "LEVC", "INTV", 2, levi_intv_fields, ARRAY_COUNT(levi_intv_fields) },
		{ "GMST", "STRV", 0, gmst_strv_fields, ARRAY_COUNT(gmst_strv_fields) },
		{ "GMST", "INTV", 4, gmst_intv_fields, ARRAY_COUNT(gmst_intv_fields) },
		{ "GMST", "FLTV", 4, gmst_fltv_fields, ARRAY_COUNT(gmst_fltv_fields) },
		{ "INFO", "DATA", 12, info_data_fields, ARRAY_COUNT(info_data_fields) },
		{ "REGN", "WEAT", 8, regn_weat_fields, ARRAY_COUNT(regn_weat_fields) },
		{ "SPEL", "SPDT", 12, spel_spdt_fields, ARRAY_COUNT(spel_spdt_fields) },
		{ "MGEF", "MEDT", 36, mgef_medt_fields, ARRAY_COUNT(mgef_medt_fields) },
		{ "SKIL", "SKDT", 24, skil_skdt_fields, ARRAY_COUNT(skil_skdt_fields) },
		{ "CLOT", "CTDT", 12, clot_ctdt_fields, ARRAY_COUNT(clot_ctdt_fields) },
		{ "LIGH", "LHDT", 24, ligh_lhdt_fields, ARRAY_COUNT(ligh_lhdt_fields) },
		{ "INGR", "IRDT", 56, ingr_irdt_fields, ARRAY_COUNT(ingr_irdt_fields) },
		{ "SCPT", "SCHD", 52, scpt_schd_fields, ARRAY_COUNT(scpt_schd_fields) },
		{ "MISC", "MCDT", 12, misc_mcdt_fields, ARRAY_COUNT(misc_mcdt_fields) },
		{ "APPA", "AADT", 16, appa_aadt_fields, ARRAY_COUNT(appa_aadt_fields) },
		{ "REPA", "RIDT", 16, repa_ridt_fields, ARRAY_COUNT(repa_ridt_fields) },
		{ "LOCK", "LKDT", 16, repa_ridt_fields, ARRAY_COUNT(repa_ridt_fields) },
		{ "PROB", "PBDT", 16, repa_ridt_fields, ARRAY_COUNT(repa_ridt_fields) },
		{ "BODY", "BYDT", 4, body_bydt_fields, ARRAY_COUNT(body_bydt_fields) },
		{ "*", "AI_W", 14, ai_w_fields, ARRAY_COUNT(ai_w_fields) },
	};
	return schemas;
}

const sub_record_schema_t * find_schema(const std::string & record_type, const std::string & sub_type, size_t data_size)
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
