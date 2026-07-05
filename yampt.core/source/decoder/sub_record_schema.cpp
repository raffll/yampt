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

static const char * const npc_flags[] = { "Female", "Essential", "Respawn", "Base", "Autocalc" };

static const field_def_t npc_flag_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, npc_flags, ARRAY_COUNT(npc_flags) },
};

static const field_def_t npc_npdt_12_fields[] = {
	{ "Level", field_type_t::u16, 0, 2, nullptr, nullptr },
	{ "Disposition", field_type_t::u8, 2, 1, nullptr, nullptr },
	{ "Reputation", field_type_t::u8, 3, 1, nullptr, nullptr },
	{ "Rank", field_type_t::u8, 4, 1, nullptr, nullptr },
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
	{ "Block", field_type_t::u8, 10, 1, nullptr, nullptr },
	{ "Armorer", field_type_t::u8, 11, 1, nullptr, nullptr },
	{ "Medium Armor", field_type_t::u8, 12, 1, nullptr, nullptr },
	{ "Heavy Armor", field_type_t::u8, 13, 1, nullptr, nullptr },
	{ "Blunt Weapon", field_type_t::u8, 14, 1, nullptr, nullptr },
	{ "Long Blade", field_type_t::u8, 15, 1, nullptr, nullptr },
	{ "Axe", field_type_t::u8, 16, 1, nullptr, nullptr },
	{ "Spear", field_type_t::u8, 17, 1, nullptr, nullptr },
	{ "Athletics", field_type_t::u8, 18, 1, nullptr, nullptr },
	{ "Enchant", field_type_t::u8, 19, 1, nullptr, nullptr },
	{ "Destruction", field_type_t::u8, 20, 1, nullptr, nullptr },
	{ "Alteration", field_type_t::u8, 21, 1, nullptr, nullptr },
	{ "Illusion", field_type_t::u8, 22, 1, nullptr, nullptr },
	{ "Conjuration", field_type_t::u8, 23, 1, nullptr, nullptr },
	{ "Mysticism", field_type_t::u8, 24, 1, nullptr, nullptr },
	{ "Restoration", field_type_t::u8, 25, 1, nullptr, nullptr },
	{ "Alchemy", field_type_t::u8, 26, 1, nullptr, nullptr },
	{ "Unarmored", field_type_t::u8, 27, 1, nullptr, nullptr },
	{ "Security", field_type_t::u8, 28, 1, nullptr, nullptr },
	{ "Sneak", field_type_t::u8, 29, 1, nullptr, nullptr },
	{ "Acrobatics", field_type_t::u8, 30, 1, nullptr, nullptr },
	{ "Light Armor", field_type_t::u8, 31, 1, nullptr, nullptr },
	{ "Short Blade", field_type_t::u8, 32, 1, nullptr, nullptr },
	{ "Marksman", field_type_t::u8, 33, 1, nullptr, nullptr },
	{ "Mercantile", field_type_t::u8, 34, 1, nullptr, nullptr },
	{ "Speechcraft", field_type_t::u8, 35, 1, nullptr, nullptr },
	{ "Hand-to-hand", field_type_t::u8, 36, 1, nullptr, nullptr },
	{ "Health", field_type_t::u16, 38, 2, nullptr, nullptr },
	{ "Magicka", field_type_t::u16, 40, 2, nullptr, nullptr },
	{ "Fatigue", field_type_t::u16, 42, 2, nullptr, nullptr },
	{ "Disposition", field_type_t::u8, 44, 1, nullptr, nullptr },
	{ "Reputation", field_type_t::u8, 45, 1, nullptr, nullptr },
	{ "Rank", field_type_t::u8, 46, 1, nullptr, nullptr },
	{ "Gold", field_type_t::u32, 48, 4, nullptr, nullptr },
};

static const char * const aidt_flags[] = { "Weapon",        "Armor",     "Clothing",    "Books",     "Ingredients",
	                                       "Picks",         "Probes",    "Lights",      "Apparatus", "Repair Items",
	                                       "Miscellaneous", "Spells",    "Magic Items", "Potions",   "Training",
	                                       "Spellmaking",   "Enchanting" };

static const field_def_t npc_aidt_fields[] = {
	{ "Hello", field_type_t::u16, 0, 2, nullptr, nullptr, 0, "Disposition" },
	{ "Fight", field_type_t::u8, 2, 1, nullptr, nullptr, 0, "Disposition" },
	{ "Flee", field_type_t::u8, 3, 1, nullptr, nullptr, 0, "Disposition" },
	{ "Alarm", field_type_t::u8, 4, 1, nullptr, nullptr, 0, "Disposition" },
	{ "Services", field_type_t::flags_u32, 8, 4, nullptr, aidt_flags, ARRAY_COUNT(aidt_flags), nullptr },
};

static const char * const weapon_types[] = {
	"Short Blade 1H",    "Long Blade 1H",   "Long Blade 2H", "Blunt 1H", "Blunt 2H Close",
	"Blunt 2H Wide",     "Spear 2H",        "Axe 1H",        "Axe 2H",   "Marksman Bow",
	"Marksman Crossbow", "Marksman Thrown", "Arrow",         "Bolt",     nullptr
};

static const char * const weapon_flags[] = { "Magical", "Silver" };

static const field_def_t weap_wpdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Type", field_type_t::enum_u16, 8, 2, weapon_types, nullptr },
	{ "Health", field_type_t::u16, 10, 2, nullptr, nullptr },
	{ "Speed", field_type_t::f32, 12, 4, nullptr, nullptr },
	{ "Reach", field_type_t::f32, 16, 4, nullptr, nullptr },
	{ "Enchant Points", field_type_t::u16, 20, 2, nullptr, nullptr },
	{ "Chop Min", field_type_t::u8, 22, 1, nullptr, nullptr },
	{ "Chop Max", field_type_t::u8, 23, 1, nullptr, nullptr },
	{ "Slash Min", field_type_t::u8, 24, 1, nullptr, nullptr },
	{ "Slash Max", field_type_t::u8, 25, 1, nullptr, nullptr },
	{ "Thrust Min", field_type_t::u8, 26, 1, nullptr, nullptr },
	{ "Thrust Max", field_type_t::u8, 27, 1, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 28, 4, nullptr, weapon_flags, ARRAY_COUNT(weapon_flags) },
};

static const char * const armor_types[] = { "Helmet",  "Cuirass",     "Left Pauldron", "Right Pauldron",
	                                        "Greaves", "Boots",       "Left Gauntlet", "Right Gauntlet",
	                                        "Shield",  "Left Bracer", "Right Bracer",  nullptr };

static const field_def_t armo_aodt_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, armor_types, nullptr },
	{ "Weight", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Health", field_type_t::u32, 12, 4, nullptr, nullptr },
	{ "Enchant Points", field_type_t::u32, 16, 4, nullptr, nullptr },
	{ "Armor Rating", field_type_t::u32, 20, 4, nullptr, nullptr },
};

static const char * const alch_flags[] = { "Autocalc" };

static const field_def_t alch_aldt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 8, 4, nullptr, alch_flags, ARRAY_COUNT(alch_flags) },
};

static const char * const ench_types[] = { "Cast Once",
	                                       "Cast on Strike",
	                                       "Cast when Used",
	                                       "Constant Effect",
	                                       nullptr };

static const char * const ench_flags[] = { "Autocalc" };

static const field_def_t ench_endt_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, ench_types, nullptr },
	{ "Cost", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Charge", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 12, 4, nullptr, ench_flags, ARRAY_COUNT(ench_flags) },
};

static const char * const spell_effect_range[] = { "Self", "Touch", "Target", nullptr };

static const char * const attribute_names[] = { "Strength",  "Intelligence", "Willpower", "Agility", "Speed",
	                                            "Endurance", "Personality",  "Luck",      nullptr };

static const char * const skill_names[] = {
	"Block",       "Armorer",     "Medium Armor", "Heavy Armor", "Blunt Weapon", "Long Blade",   "Axe",
	"Spear",       "Athletics",   "Enchant",      "Destruction", "Alteration",   "Illusion",     "Conjuration",
	"Mysticism",   "Restoration", "Alchemy",      "Unarmored",   "Security",     "Sneak",        "Acrobatics",
	"Light Armor", "Short Blade", "Marksman",     "Mercantile",  "Speechcraft",  "Hand-to-hand", nullptr
};

static const char * const effect_names[] = { "Water Breathing",
	                                         "Swift Swim",
	                                         "Water Walking",
	                                         "Shield",
	                                         "Fire Shield",
	                                         "Lightning Shield",
	                                         "Frost Shield",
	                                         "Burden",
	                                         "Feather",
	                                         "Jump",
	                                         "Levitate",
	                                         "Slow Fall",
	                                         "Lock",
	                                         "Open",
	                                         "Fire Damage",
	                                         "Shock Damage",
	                                         "Frost Damage",
	                                         "Drain Attribute",
	                                         "Drain Health",
	                                         "Drain Magicka",
	                                         "Drain Fatigue",
	                                         "Drain Skill",
	                                         "Damage Attribute",
	                                         "Damage Health",
	                                         "Damage Magicka",
	                                         "Damage Fatigue",
	                                         "Damage Skill",
	                                         "Poison",
	                                         "Weakness to Fire",
	                                         "Weakness to Frost",
	                                         "Weakness to Shock",
	                                         "Weakness to Magicka",
	                                         "Weakness to Common Disease",
	                                         "Weakness to Blight Disease",
	                                         "Weakness to Corprus Disease",
	                                         "Weakness to Poison",
	                                         "Weakness to Normal Weapons",
	                                         "Disintegrate Weapon",
	                                         "Disintegrate Armor",
	                                         "Invisibility",
	                                         "Chameleon",
	                                         "Light",
	                                         "Sanctuary",
	                                         "Night Eye",
	                                         "Charm",
	                                         "Paralyze",
	                                         "Silence",
	                                         "Blind",
	                                         "Sound",
	                                         "Calm Humanoid",
	                                         "Calm Creature",
	                                         "Frenzy Humanoid",
	                                         "Frenzy Creature",
	                                         "Demoralize Humanoid",
	                                         "Demoralize Creature",
	                                         "Rally Humanoid",
	                                         "Rally Creature",
	                                         "Dispel",
	                                         "Soultrap",
	                                         "Telekinesis",
	                                         "Mark",
	                                         "Recall",
	                                         "Divine Intervention",
	                                         "Almsivi Intervention",
	                                         "Detect Animal",
	                                         "Detect Enchantment",
	                                         "Detect Key",
	                                         "Spell Absorption",
	                                         "Reflect",
	                                         "Cure Common Disease",
	                                         "Cure Blight Disease",
	                                         "Cure Corprus Disease",
	                                         "Cure Poison",
	                                         "Cure Paralyzation",
	                                         "Restore Attribute",
	                                         "Restore Health",
	                                         "Restore Magicka",
	                                         "Restore Fatigue",
	                                         "Restore Skill",
	                                         "Fortify Attribute",
	                                         "Fortify Health",
	                                         "Fortify Magicka",
	                                         "Fortify Fatigue",
	                                         "Fortify Skill",
	                                         "Fortify Maximum Magicka",
	                                         "Absorb Attribute",
	                                         "Absorb Health",
	                                         "Absorb Magicka",
	                                         "Absorb Fatigue",
	                                         "Absorb Skill",
	                                         "Resist Fire",
	                                         "Resist Frost",
	                                         "Resist Shock",
	                                         "Resist Magicka",
	                                         "Resist Common Disease",
	                                         "Resist Blight Disease",
	                                         "Resist Corprus Disease",
	                                         "Resist Poison",
	                                         "Resist Normal Weapons",
	                                         "Resist Paralysis",
	                                         "Remove Curse",
	                                         "Turn Undead",
	                                         "Summon Scamp",
	                                         "Summon Clannfear",
	                                         "Summon Daedroth",
	                                         "Summon Dremora",
	                                         "Summon Ancestral Ghost",
	                                         "Summon Skeletal Minion",
	                                         "Summon Bonewalker",
	                                         "Summon Greater Bonewalker",
	                                         "Summon Bonelord",
	                                         "Summon Winged Twilight",
	                                         "Summon Hunger",
	                                         "Summon Golden Saint",
	                                         "Summon Flame Atronach",
	                                         "Summon Frost Atronach",
	                                         "Summon Storm Atronach",
	                                         "Fortify Attack",
	                                         "Command Creature",
	                                         "Command Humanoid",
	                                         "Bound Dagger",
	                                         "Bound Longsword",
	                                         "Bound Mace",
	                                         "Bound Battle Axe",
	                                         "Bound Spear",
	                                         "Bound Longbow",
	                                         "Extra Spell",
	                                         "Bound Cuirass",
	                                         "Bound Helm",
	                                         "Bound Boots",
	                                         "Bound Shield",
	                                         "Bound Gloves",
	                                         "Corprus",
	                                         "Vampirism",
	                                         "Summon Centurion Sphere",
	                                         "Sun Damage",
	                                         "Stunted Magicka",
	                                         "Summon Fabricant",
	                                         "Summon Wolf",
	                                         "Summon Bear",
	                                         "Summon Bonewolf",
	                                         "Summon Creature 04",
	                                         "Summon Creature 05",
	                                         nullptr };

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

static const char * const book_scroll[] = { "No", "Yes", nullptr };

static const field_def_t book_bkdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Scroll", field_type_t::enum_u32, 8, 4, book_scroll, nullptr },
	{ "Skill", field_type_t::i8, 12, 1, skill_names, nullptr },
	{ "Enchant Points", field_type_t::u32, 16, 4, nullptr, nullptr },
};

static const char * const creature_types[] = { "Creature", "Daedra", "Undead", "Humanoid", nullptr };

static const field_def_t crea_npdt_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, creature_types, nullptr, 0, nullptr },
	{ "Level", field_type_t::u32, 4, 4, nullptr, nullptr, 0, nullptr },
	{ "Strength", field_type_t::u32, 8, 4, nullptr, nullptr, 0, "Attributes" },
	{ "Intelligence", field_type_t::u32, 12, 4, nullptr, nullptr, 0, "Attributes" },
	{ "Willpower", field_type_t::u32, 16, 4, nullptr, nullptr, 0, "Attributes" },
	{ "Agility", field_type_t::u32, 20, 4, nullptr, nullptr, 0, "Attributes" },
	{ "Speed", field_type_t::u32, 24, 4, nullptr, nullptr, 0, "Attributes" },
	{ "Endurance", field_type_t::u32, 28, 4, nullptr, nullptr, 0, "Attributes" },
	{ "Personality", field_type_t::u32, 32, 4, nullptr, nullptr, 0, "Attributes" },
	{ "Luck", field_type_t::u32, 36, 4, nullptr, nullptr, 0, "Attributes" },
	{ "Health", field_type_t::u32, 40, 4, nullptr, nullptr, 0, "Stats" },
	{ "Magicka", field_type_t::u32, 44, 4, nullptr, nullptr, 0, "Stats" },
	{ "Fatigue", field_type_t::u32, 48, 4, nullptr, nullptr, 0, "Stats" },
	{ "Soul", field_type_t::u32, 52, 4, nullptr, nullptr, 0, "Stats" },
	{ "Combat", field_type_t::u32, 56, 4, nullptr, nullptr, 0, "Skills" },
	{ "Magic", field_type_t::u32, 60, 4, nullptr, nullptr, 0, "Skills" },
	{ "Stealth", field_type_t::u32, 64, 4, nullptr, nullptr, 0, "Skills" },
	{ "Attack 1 Min", field_type_t::u32, 68, 4, nullptr, nullptr, 0, "Attacks" },
	{ "Attack 1 Max", field_type_t::u32, 72, 4, nullptr, nullptr, 0, "Attacks" },
	{ "Attack 2 Min", field_type_t::u32, 76, 4, nullptr, nullptr, 0, "Attacks" },
	{ "Attack 2 Max", field_type_t::u32, 80, 4, nullptr, nullptr, 0, "Attacks" },
	{ "Attack 3 Min", field_type_t::u32, 84, 4, nullptr, nullptr, 0, "Attacks" },
	{ "Attack 3 Max", field_type_t::u32, 88, 4, nullptr, nullptr, 0, "Attacks" },
	{ "Gold", field_type_t::u32, 92, 4, nullptr, nullptr, 0, nullptr },
};

static const field_def_t cont_cndt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
};

static const field_def_t fact_fadt_fields[] = {
	{ "Attribute 1", field_type_t::enum_u32, 0, 4, attribute_names, nullptr },
	{ "Attribute 2", field_type_t::enum_u32, 4, 4, attribute_names, nullptr },
	{ "Rank 1 Attr1", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Rank 1 Attr2", field_type_t::u32, 12, 4, nullptr, nullptr },
	{ "Rank 1 Primary", field_type_t::u32, 16, 4, nullptr, nullptr },
	{ "Rank 1 Favoured", field_type_t::u32, 20, 4, nullptr, nullptr },
	{ "Rank 1 Reputation", field_type_t::u32, 24, 4, nullptr, nullptr },
	{ "Rank 2 Attr1", field_type_t::u32, 28, 4, nullptr, nullptr },
	{ "Rank 2 Attr2", field_type_t::u32, 32, 4, nullptr, nullptr },
	{ "Rank 2 Primary", field_type_t::u32, 36, 4, nullptr, nullptr },
	{ "Rank 2 Favoured", field_type_t::u32, 40, 4, nullptr, nullptr },
	{ "Rank 2 Reputation", field_type_t::u32, 44, 4, nullptr, nullptr },
	{ "Rank 3 Attr1", field_type_t::u32, 48, 4, nullptr, nullptr },
	{ "Rank 3 Attr2", field_type_t::u32, 52, 4, nullptr, nullptr },
	{ "Rank 3 Primary", field_type_t::u32, 56, 4, nullptr, nullptr },
	{ "Rank 3 Favoured", field_type_t::u32, 60, 4, nullptr, nullptr },
	{ "Rank 3 Reputation", field_type_t::u32, 64, 4, nullptr, nullptr },
	{ "Rank 4 Attr1", field_type_t::u32, 68, 4, nullptr, nullptr },
	{ "Rank 4 Attr2", field_type_t::u32, 72, 4, nullptr, nullptr },
	{ "Rank 4 Primary", field_type_t::u32, 76, 4, nullptr, nullptr },
	{ "Rank 4 Favoured", field_type_t::u32, 80, 4, nullptr, nullptr },
	{ "Rank 4 Reputation", field_type_t::u32, 84, 4, nullptr, nullptr },
	{ "Rank 5 Attr1", field_type_t::u32, 88, 4, nullptr, nullptr },
	{ "Rank 5 Attr2", field_type_t::u32, 92, 4, nullptr, nullptr },
	{ "Rank 5 Primary", field_type_t::u32, 96, 4, nullptr, nullptr },
	{ "Rank 5 Favoured", field_type_t::u32, 100, 4, nullptr, nullptr },
	{ "Rank 5 Reputation", field_type_t::u32, 104, 4, nullptr, nullptr },
	{ "Rank 6 Attr1", field_type_t::u32, 108, 4, nullptr, nullptr },
	{ "Rank 6 Attr2", field_type_t::u32, 112, 4, nullptr, nullptr },
	{ "Rank 6 Primary", field_type_t::u32, 116, 4, nullptr, nullptr },
	{ "Rank 6 Favoured", field_type_t::u32, 120, 4, nullptr, nullptr },
	{ "Rank 6 Reputation", field_type_t::u32, 124, 4, nullptr, nullptr },
	{ "Rank 7 Attr1", field_type_t::u32, 128, 4, nullptr, nullptr },
	{ "Rank 7 Attr2", field_type_t::u32, 132, 4, nullptr, nullptr },
	{ "Rank 7 Primary", field_type_t::u32, 136, 4, nullptr, nullptr },
	{ "Rank 7 Favoured", field_type_t::u32, 140, 4, nullptr, nullptr },
	{ "Rank 7 Reputation", field_type_t::u32, 144, 4, nullptr, nullptr },
	{ "Rank 8 Attr1", field_type_t::u32, 148, 4, nullptr, nullptr },
	{ "Rank 8 Attr2", field_type_t::u32, 152, 4, nullptr, nullptr },
	{ "Rank 8 Primary", field_type_t::u32, 156, 4, nullptr, nullptr },
	{ "Rank 8 Favoured", field_type_t::u32, 160, 4, nullptr, nullptr },
	{ "Rank 8 Reputation", field_type_t::u32, 164, 4, nullptr, nullptr },
	{ "Rank 9 Attr1", field_type_t::u32, 168, 4, nullptr, nullptr },
	{ "Rank 9 Attr2", field_type_t::u32, 172, 4, nullptr, nullptr },
	{ "Rank 9 Primary", field_type_t::u32, 176, 4, nullptr, nullptr },
	{ "Rank 9 Favoured", field_type_t::u32, 180, 4, nullptr, nullptr },
	{ "Rank 9 Reputation", field_type_t::u32, 184, 4, nullptr, nullptr },
	{ "Rank 10 Attr1", field_type_t::u32, 188, 4, nullptr, nullptr },
	{ "Rank 10 Attr2", field_type_t::u32, 192, 4, nullptr, nullptr },
	{ "Rank 10 Primary", field_type_t::u32, 196, 4, nullptr, nullptr },
	{ "Rank 10 Favoured", field_type_t::u32, 200, 4, nullptr, nullptr },
	{ "Rank 10 Reputation", field_type_t::u32, 204, 4, nullptr, nullptr },
	{ "Skill 1", field_type_t::i32, 208, 4, skill_names, nullptr },
	{ "Skill 2", field_type_t::i32, 212, 4, skill_names, nullptr },
	{ "Skill 3", field_type_t::i32, 216, 4, skill_names, nullptr },
	{ "Skill 4", field_type_t::i32, 220, 4, skill_names, nullptr },
	{ "Skill 5", field_type_t::i32, 224, 4, skill_names, nullptr },
	{ "Skill 6", field_type_t::i32, 228, 4, skill_names, nullptr },
	{ "Skill 7", field_type_t::i32, 232, 4, skill_names, nullptr },
	{ "Hidden", field_type_t::bool_bit, 236, 0, nullptr, nullptr },
};

static const field_def_t fact_rnam_fields[] = {
	{ "Rank Name", field_type_t::string_fixed, 0, 32, nullptr, nullptr },
};

static const char * const class_specializations[] = { "Combat", "Magic", "Stealth", nullptr };

static const field_def_t clas_cldt_fields[] = {
	{ "Attribute 1", field_type_t::enum_u32, 0, 4, attribute_names, nullptr, 0, nullptr },
	{ "Attribute 2", field_type_t::enum_u32, 4, 4, attribute_names, nullptr, 0, nullptr },
	{ "Specialization", field_type_t::enum_u32, 8, 4, class_specializations, nullptr, 0, nullptr },
	{ "Major 1", field_type_t::enum_u32, 16, 4, skill_names, nullptr, 0, "Major Skills" },
	{ "Major 2", field_type_t::enum_u32, 24, 4, skill_names, nullptr, 0, "Major Skills" },
	{ "Major 3", field_type_t::enum_u32, 32, 4, skill_names, nullptr, 0, "Major Skills" },
	{ "Major 4", field_type_t::enum_u32, 40, 4, skill_names, nullptr, 0, "Major Skills" },
	{ "Major 5", field_type_t::enum_u32, 48, 4, skill_names, nullptr, 0, "Major Skills" },
	{ "Minor 1", field_type_t::enum_u32, 12, 4, skill_names, nullptr, 0, "Minor Skills" },
	{ "Minor 2", field_type_t::enum_u32, 20, 4, skill_names, nullptr, 0, "Minor Skills" },
	{ "Minor 3", field_type_t::enum_u32, 28, 4, skill_names, nullptr, 0, "Minor Skills" },
	{ "Minor 4", field_type_t::enum_u32, 36, 4, skill_names, nullptr, 0, "Minor Skills" },
	{ "Minor 5", field_type_t::enum_u32, 44, 4, skill_names, nullptr, 0, "Minor Skills" },
	{ "Playable", field_type_t::bool_bit, 52, 0, nullptr, nullptr, 0, nullptr },
	{ "Services", field_type_t::flags_u32, 56, 4, nullptr, aidt_flags, ARRAY_COUNT(aidt_flags), nullptr },
};

static const char * const race_flags[] = { "Playable", "Beast" };

static const field_def_t race_radt_fields[] = {
	{ "Bonus Skill 1", field_type_t::enum_u32, 0, 4, skill_names, nullptr },
	{ "Bonus 1", field_type_t::i32, 4, 4, nullptr, nullptr },
	{ "Bonus Skill 2", field_type_t::enum_u32, 8, 4, skill_names, nullptr },
	{ "Bonus 2", field_type_t::i32, 12, 4, nullptr, nullptr },
	{ "Bonus Skill 3", field_type_t::enum_u32, 16, 4, skill_names, nullptr },
	{ "Bonus 3", field_type_t::i32, 20, 4, nullptr, nullptr },
	{ "Bonus Skill 4", field_type_t::enum_u32, 24, 4, skill_names, nullptr },
	{ "Bonus 4", field_type_t::i32, 28, 4, nullptr, nullptr },
	{ "Bonus Skill 5", field_type_t::enum_u32, 32, 4, skill_names, nullptr },
	{ "Bonus 5", field_type_t::i32, 36, 4, nullptr, nullptr },
	{ "Bonus Skill 6", field_type_t::enum_u32, 40, 4, skill_names, nullptr },
	{ "Bonus 6", field_type_t::i32, 44, 4, nullptr, nullptr },
	{ "Bonus Skill 7", field_type_t::enum_u32, 48, 4, skill_names, nullptr },
	{ "Bonus 7", field_type_t::i32, 52, 4, nullptr, nullptr },
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
	{ "Flags", field_type_t::flags_u32, 136, 4, nullptr, race_flags, ARRAY_COUNT(race_flags) },
};

static const char * const levi_flags[] = { "Calc for Each Item", "Calc from All Levels" };
static const char * const levc_flags[] = { "Calc from All Levels" };

static const field_def_t levi_data_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, levi_flags, ARRAY_COUNT(levi_flags) },
};

static const field_def_t levc_data_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, levc_flags, ARRAY_COUNT(levc_flags) },
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

static const field_def_t glob_fnam_fields[] = {
	{ "Type", field_type_t::string_fixed, 0, 1, nullptr, nullptr },
};

static const field_def_t glob_fltv_fields[] = {
	{ "Value", field_type_t::f32, 0, 4, nullptr, nullptr },
};

static const char * const sndg_types[] = { "Left Foot", "Right Foot", "Swim Left", "Swim Right", "Moan",
	                                       "Roar",      "Scream",     "Land",      nullptr };

static const field_def_t sndg_data_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, sndg_types, nullptr },
};

static const char * const land_flags[] = { "Heights & Normals", "Vertex Colors", "Textures" };

static const field_def_t land_data_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, land_flags, ARRAY_COUNT(land_flags) },
};

static const field_def_t land_intv_fields[] = {
	{ "Grid X", field_type_t::i32, 0, 4, nullptr, nullptr },
	{ "Grid Y", field_type_t::i32, 4, 4, nullptr, nullptr },
};

static const field_def_t land_binary_fields[] = {
	{ "Data", field_type_t::binary, 0, 0, nullptr, nullptr },
};

static const char * const info_gender[] = { "Male", "Female", nullptr };

static const char * const info_rank_names[] = { nullptr };

static const char * const info_types[] = { "Topic", "Voice", "Greeting", "Persuasion", "Journal", nullptr };

static const field_def_t info_data_fields[] = {
	{ "Type", field_type_t::enum_u32, 0, 4, info_types, nullptr },
	{ "Disposition", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Rank", field_type_t::i8, 8, 1, info_rank_names, nullptr },
	{ "Gender", field_type_t::i8, 9, 1, info_gender, nullptr },
	{ "PC Rank", field_type_t::i8, 10, 1, info_rank_names, nullptr },
};

static const field_def_t regn_weat_fields[] = {
	{ "Clear", field_type_t::u8, 0, 1, nullptr, nullptr }, { "Cloudy", field_type_t::u8, 1, 1, nullptr, nullptr },
	{ "Foggy", field_type_t::u8, 2, 1, nullptr, nullptr }, { "Overcast", field_type_t::u8, 3, 1, nullptr, nullptr },
	{ "Rain", field_type_t::u8, 4, 1, nullptr, nullptr },  { "Thunder", field_type_t::u8, 5, 1, nullptr, nullptr },
	{ "Ash", field_type_t::u8, 6, 1, nullptr, nullptr },   { "Blight", field_type_t::u8, 7, 1, nullptr, nullptr },
};

static const field_def_t regn_weat_10_fields[] = {
	{ "Clear", field_type_t::u8, 0, 1, nullptr, nullptr }, { "Cloudy", field_type_t::u8, 1, 1, nullptr, nullptr },
	{ "Foggy", field_type_t::u8, 2, 1, nullptr, nullptr }, { "Overcast", field_type_t::u8, 3, 1, nullptr, nullptr },
	{ "Rain", field_type_t::u8, 4, 1, nullptr, nullptr },  { "Thunder", field_type_t::u8, 5, 1, nullptr, nullptr },
	{ "Ash", field_type_t::u8, 6, 1, nullptr, nullptr },   { "Blight", field_type_t::u8, 7, 1, nullptr, nullptr },
	{ "Snow", field_type_t::u8, 8, 1, nullptr, nullptr },  { "Blizzard", field_type_t::u8, 9, 1, nullptr, nullptr },
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
	"Target Skill", "Target Attribute", "No Duration",    "No Magnitude",   "Harmful",       "Continuous VFX",
	"Cast Self",    "Cast Touch",       "Cast Target",    "Spellmaking",    "Enchanting",    "Negative Light",
	"Applied Once", "Stealth",          "Non-Recastable", "Illegal Daedra", "Unreflectable", "Caster Linked",
};

static const field_def_t mgef_medt_fields[] = {
	{ "School", field_type_t::enum_u32, 0, 4, mgef_schools, nullptr },
	{ "Base Cost", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 8, 4, nullptr, mgef_flags, ARRAY_COUNT(mgef_flags) },
	{ "Red", field_type_t::u32, 12, 4, nullptr, nullptr },
	{ "Green", field_type_t::u32, 16, 4, nullptr, nullptr },
	{ "Blue", field_type_t::u32, 20, 4, nullptr, nullptr },
	{ "Size X", field_type_t::f32, 24, 4, nullptr, nullptr },
	{ "Speed", field_type_t::f32, 28, 4, nullptr, nullptr },
	{ "Size Cap", field_type_t::f32, 32, 4, nullptr, nullptr },
};

static const char * const skill_specializations[] = { "Combat", "Magic", "Stealth", nullptr };

static const field_def_t skil_skdt_fields[] = {
	{ "Attribute", field_type_t::enum_u32, 0, 4, attribute_names, nullptr },
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

static const field_def_t cell_nam5_fields[] = {
	{ "Red", field_type_t::u8, 0, 1, nullptr, nullptr },
	{ "Green", field_type_t::u8, 1, 1, nullptr, nullptr },
	{ "Blue", field_type_t::u8, 2, 1, nullptr, nullptr },
	{ "Alpha", field_type_t::u8, 3, 1, nullptr, nullptr },
};

static const field_def_t cell_fltv_fields[] = {
	{ "Lock Level", field_type_t::i32, 0, 4, nullptr, nullptr },
};

static const field_def_t cell_nam9_fields[] = {
	{ "Stack Count", field_type_t::i32, 0, 4, nullptr, nullptr },
};

static const field_def_t cell_dodt_fields[] = {
	{ "X Position", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Y Position", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Z Position", field_type_t::f32, 8, 4, nullptr, nullptr },
	{ "X Rotation", field_type_t::f32, 12, 4, nullptr, nullptr },
	{ "Y Rotation", field_type_t::f32, 16, 4, nullptr, nullptr },
	{ "Z Rotation", field_type_t::f32, 20, 4, nullptr, nullptr },
};

static const char * const cont_flags[] = { "Organic", "Respawns", "_", "Unknown" };

static const field_def_t cont_flag_fields[] = {
	{ "Flags", field_type_t::flags_u32, 0, 4, nullptr, cont_flags, ARRAY_COUNT(cont_flags) },
};

static const char * const crea_flags[] = {
	"Biped", "Respawn", "Weapon and Shield", "Base",        "Swims", "Flies", "Walks", "Essential",
	"_",     "_",       "Skeleton Blood",    "Metal Blood",
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
	{ "Enchant Points", field_type_t::u16, 10, 2, nullptr, nullptr },
};

static const char * const light_flags[] = { "Dynamic",     "Can Carry",    "Negative", "Flicker",   "Fire",
	                                        "Off Default", "Flicker Slow", "Pulse",    "Pulse Slow" };

static const field_def_t ligh_lhdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Time", field_type_t::u32, 8, 4, nullptr, nullptr },
	{ "Radius", field_type_t::u32, 12, 4, nullptr, nullptr },
	{ "Red", field_type_t::u8, 16, 1, nullptr, nullptr },
	{ "Green", field_type_t::u8, 17, 1, nullptr, nullptr },
	{ "Blue", field_type_t::u8, 18, 1, nullptr, nullptr },
	{ "Alpha", field_type_t::u8, 19, 1, nullptr, nullptr },
	{ "Flags", field_type_t::flags_u32, 20, 4, nullptr, light_flags, ARRAY_COUNT(light_flags) },
};

static const field_def_t ingr_irdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Effect 1", field_type_t::i32, 8, 4, effect_names, nullptr },
	{ "Effect 2", field_type_t::i32, 12, 4, effect_names, nullptr },
	{ "Effect 3", field_type_t::i32, 16, 4, effect_names, nullptr },
	{ "Effect 4", field_type_t::i32, 20, 4, effect_names, nullptr },
	{ "Skill 1", field_type_t::i32, 24, 4, skill_names, nullptr },
	{ "Skill 2", field_type_t::i32, 28, 4, skill_names, nullptr },
	{ "Skill 3", field_type_t::i32, 32, 4, skill_names, nullptr },
	{ "Skill 4", field_type_t::i32, 36, 4, skill_names, nullptr },
	{ "Attribute 1", field_type_t::i32, 40, 4, attribute_names, nullptr },
	{ "Attribute 2", field_type_t::i32, 44, 4, attribute_names, nullptr },
	{ "Attribute 3", field_type_t::i32, 48, 4, attribute_names, nullptr },
	{ "Attribute 4", field_type_t::i32, 52, 4, attribute_names, nullptr },
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
	{ "Key", field_type_t::bool_bit, 8, 0, nullptr, nullptr },
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

static const field_def_t lock_lkdt_fields[] = {
	{ "Weight", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Value", field_type_t::u32, 4, 4, nullptr, nullptr },
	{ "Quality", field_type_t::f32, 8, 4, nullptr, nullptr },
	{ "Uses", field_type_t::u32, 12, 4, nullptr, nullptr },
};

static const field_def_t cell_ref_data_fields[] = {
	{ "X Position", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Y Position", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Z Position", field_type_t::f32, 8, 4, nullptr, nullptr },
	{ "X Rotation", field_type_t::f32, 12, 4, nullptr, nullptr },
	{ "Y Rotation", field_type_t::f32, 16, 4, nullptr, nullptr },
	{ "Z Rotation", field_type_t::f32, 20, 4, nullptr, nullptr },
};

static const char * const body_parts[] = { "Head",     "Hair",     "Neck",     "Chest", "Groin", "Hand",
	                                       "Wrist",    "Forearm",  "Upperarm", "Foot",  "Ankle", "Knee",
	                                       "Upperleg", "Clavicle", "Tail",     nullptr };

static const char * const body_vampire[] = { "No", "Yes", nullptr };

static const char * const body_part_types[] = { "Skin", "Clothing", "Armor", nullptr };

static const field_def_t body_bydt_fields[] = {
	{ "Part", field_type_t::enum_u8, 0, 1, body_parts, nullptr },
	{ "Vampire", field_type_t::enum_u8, 1, 1, body_vampire, nullptr },
	{ "Female", field_type_t::bool_bit, 2, 0, nullptr, nullptr },
	{ "Playable", field_type_t::bool_bit, 2, 1, nullptr, nullptr },
	{ "Part Type", field_type_t::enum_u8, 3, 1, body_part_types, nullptr },
};

static const field_def_t ai_w_fields[] = {
	{ "Distance", field_type_t::u16, 0, 2, nullptr, nullptr, 0, nullptr },
	{ "Duration", field_type_t::u16, 2, 2, nullptr, nullptr, 0, nullptr },
	{ "Time of Day", field_type_t::u8, 4, 1, nullptr, nullptr, 0, nullptr },
	{ "Idle 1", field_type_t::u8, 5, 1, nullptr, nullptr, 0, "Idle" },
	{ "Idle 2", field_type_t::u8, 6, 1, nullptr, nullptr, 0, "Idle" },
	{ "Idle 3", field_type_t::u8, 7, 1, nullptr, nullptr, 0, "Idle" },
	{ "Idle 4", field_type_t::u8, 8, 1, nullptr, nullptr, 0, "Idle" },
	{ "Idle 5", field_type_t::u8, 9, 1, nullptr, nullptr, 0, "Idle" },
	{ "Idle 6", field_type_t::u8, 10, 1, nullptr, nullptr, 0, "Idle" },
	{ "Idle 7", field_type_t::u8, 11, 1, nullptr, nullptr, 0, "Idle" },
	{ "Idle 8", field_type_t::u8, 12, 1, nullptr, nullptr, 0, "Idle" },
	{ "Should Repeat", field_type_t::u8, 13, 1, nullptr, nullptr, 0, nullptr },
};

static const field_def_t ai_t_fields[] = {
	{ "X", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Y", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Z", field_type_t::f32, 8, 4, nullptr, nullptr },
	{ "Should Repeat", field_type_t::u8, 12, 1, nullptr, nullptr },
};

static const field_def_t ai_f_fields[] = {
	{ "X", field_type_t::f32, 0, 4, nullptr, nullptr },
	{ "Y", field_type_t::f32, 4, 4, nullptr, nullptr },
	{ "Z", field_type_t::f32, 8, 4, nullptr, nullptr },
	{ "Duration", field_type_t::u16, 12, 2, nullptr, nullptr },
	{ "ID", field_type_t::string_fixed, 14, 32, nullptr, nullptr },
	{ "Should Repeat", field_type_t::u8, 46, 1, nullptr, nullptr },
};

static const field_def_t ai_a_fields[] = {
	{ "ID", field_type_t::string_fixed, 0, 32, nullptr, nullptr },
	{ "Should Repeat", field_type_t::u8, 32, 1, nullptr, nullptr },
};

static const char * const dial_types[] = { "Topic", "Voice", "Greeting", "Persuasion", "Journal", nullptr };

static const field_def_t dial_data_fields[] = {
	{ "Type", field_type_t::enum_u8, 0, 1, dial_types, nullptr },
};

static const field_def_t xscl_fields[] = {
	{ "Scale", field_type_t::f32, 0, 4, nullptr, nullptr },
};

static const field_def_t frmr_fields[] = {
	{ "Object Index", field_type_t::u32, 0, 4, nullptr, nullptr },
};

static const field_def_t mvrf_fields[] = {
	{ "Moved Reference", field_type_t::u32, 0, 4, nullptr, nullptr },
};

static const field_def_t cell_cndt_fields[] = {
	{ "Destination Cell", field_type_t::string_var, 0, 0, nullptr, nullptr },
};

static const field_def_t dele_fields[] = {
	{ "Deleted", field_type_t::u32, 0, 4, nullptr, nullptr },
};

static const field_def_t whgt_fields[] = {
	{ "Water Height", field_type_t::f32, 0, 4, nullptr, nullptr },
};

static const field_def_t nam0_fields[] = {
	{ "Object Count", field_type_t::u32, 0, 4, nullptr, nullptr },
};

static const field_def_t intv_4_fields[] = {
	{ "Value", field_type_t::i32, 0, 4, nullptr, nullptr },
};

static const field_def_t soun_data_fields[] = {
	{ "Volume", field_type_t::u8, 0, 1, nullptr, nullptr },
	{ "Min Range", field_type_t::u8, 1, 1, nullptr, nullptr },
	{ "Max Range", field_type_t::u8, 2, 1, nullptr, nullptr },
};

static const field_def_t pgrd_data_fields[] = {
	{ "Grid X", field_type_t::i32, 0, 4, nullptr, nullptr },
	{ "Grid Y", field_type_t::i32, 4, 4, nullptr, nullptr },
	{ "Granularity", field_type_t::u16, 8, 2, nullptr, nullptr },
	{ "Points", field_type_t::u16, 10, 2, nullptr, nullptr },
};

static const field_def_t regn_snam_fields[] = {
	{ "Sound", field_type_t::string_fixed, 0, 32, nullptr, nullptr },
	{ "Chance", field_type_t::u8, 32, 1, nullptr, nullptr },
};

static const field_def_t regn_cnam_fields[] = {
	{ "Red", field_type_t::u8, 0, 1, nullptr, nullptr },
	{ "Green", field_type_t::u8, 1, 1, nullptr, nullptr },
	{ "Blue", field_type_t::u8, 2, 1, nullptr, nullptr },
	{ "Alpha", field_type_t::u8, 3, 1, nullptr, nullptr },
};

static const field_def_t indx_fields[] = {
	{ "Index", field_type_t::u32, 0, 4, nullptr, nullptr },
};

static const char * const armo_part_names[] = { "Head",           "Hair",
	                                            "Neck",           "Cuirass",
	                                            "Groin",          "Skirt",
	                                            "Right Hand",     "Left Hand",
	                                            "Right Wrist",    "Left Wrist",
	                                            "Shield",         "Right Forearm",
	                                            "Left Forearm",   "Right Upper Arm",
	                                            "Left Upper Arm", "Right Foot",
	                                            "Left Foot",      "Right Ankle",
	                                            "Left Ankle",     "Right Knee",
	                                            "Left Knee",      "Right Upper Leg",
	                                            "Left Upper Leg", "Right Pauldron",
	                                            "Left Pauldron",  "Weapon",
	                                            "Tail",           nullptr };

static const field_def_t armo_indx_fields[] = {
	{ "Part", field_type_t::enum_u8, 0, 1, armo_part_names, nullptr },
};

static const field_def_t text_fields[] = {
	{ "Text", field_type_t::string_var, 0, 0, nullptr, nullptr },
};

static const char * const quest_marker_names[] = { "No", "Yes", nullptr };

static const field_def_t quest_marker_fields[] = {
	{ "Value", field_type_t::enum_u8, 0, 1, quest_marker_names, nullptr },
};

static const field_def_t nnam_chance_fields[] = {
	{ "Value", field_type_t::u8, 0, 1, nullptr, nullptr },
};

static const std::vector<sub_record_schema_t> & build_schemas()
{
	static const std::vector<sub_record_schema_t> schemas = {
		{ "CELL", "DATA", 12, cell_data_fields, ARRAY_COUNT(cell_data_fields) },
		{ "CELL", "AMBI", 16, cell_ambi_fields, ARRAY_COUNT(cell_ambi_fields) },
		{ "*", "DODT", 24, cell_dodt_fields, ARRAY_COUNT(cell_dodt_fields) },
		{ "CELL", "DATA", 24, cell_ref_data_fields, ARRAY_COUNT(cell_ref_data_fields) },
		{ "NPC_", "FLAG", 4, npc_flag_fields, ARRAY_COUNT(npc_flag_fields) },
		{ "NPC_", "NPDT", 12, npc_npdt_12_fields, ARRAY_COUNT(npc_npdt_12_fields) },
		{ "NPC_", "NPDT", 52, npc_npdt_52_fields, ARRAY_COUNT(npc_npdt_52_fields) },
		{ "*", "AIDT", 12, npc_aidt_fields, ARRAY_COUNT(npc_aidt_fields) },
		{ "WEAP", "WPDT", 32, weap_wpdt_fields, ARRAY_COUNT(weap_wpdt_fields) },
		{ "ARMO", "AODT", 24, armo_aodt_fields, ARRAY_COUNT(armo_aodt_fields) },
		{ "ALCH", "ALDT", 12, alch_aldt_fields, ARRAY_COUNT(alch_aldt_fields) },
		{ "ENCH", "ENDT", 16, ench_endt_fields, ARRAY_COUNT(ench_endt_fields) },
		{ "SPEL", "ENAM", 24, enam_fields, ARRAY_COUNT(enam_fields) },
		{ "ENCH", "ENAM", 24, enam_fields, ARRAY_COUNT(enam_fields) },
		{ "ALCH", "ENAM", 24, enam_fields, ARRAY_COUNT(enam_fields) },
		{ "INGR", "ENAM", 24, enam_fields, ARRAY_COUNT(enam_fields) },
		{ "*", "NPCO", 36, npco_fields, ARRAY_COUNT(npco_fields) },
		{ "BOOK", "BKDT", 20, book_bkdt_fields, ARRAY_COUNT(book_bkdt_fields) },
		{ "CREA", "NPDT", 96, crea_npdt_fields, ARRAY_COUNT(crea_npdt_fields) },
		{ "CREA", "FLAG", 4, crea_flag_fields, ARRAY_COUNT(crea_flag_fields) },
		{ "CONT", "CNDT", 4, cont_cndt_fields, ARRAY_COUNT(cont_cndt_fields) },
		{ "CONT", "FLAG", 4, cont_flag_fields, ARRAY_COUNT(cont_flag_fields) },
		{ "FACT", "FADT", 240, fact_fadt_fields, ARRAY_COUNT(fact_fadt_fields) },
		{ "FACT", "RNAM", 32, fact_rnam_fields, ARRAY_COUNT(fact_rnam_fields) },
		{ "CLAS", "CLDT", 60, clas_cldt_fields, ARRAY_COUNT(clas_cldt_fields) },
		{ "RACE", "RADT", 140, race_radt_fields, ARRAY_COUNT(race_radt_fields) },
		{ "LEVI", "DATA", 4, levi_data_fields, ARRAY_COUNT(levi_data_fields) },
		{ "LEVC", "DATA", 4, levc_data_fields, ARRAY_COUNT(levc_data_fields) },
		{ "LEVI", "INTV", 2, levi_intv_fields, ARRAY_COUNT(levi_intv_fields) },
		{ "LEVC", "INTV", 2, levi_intv_fields, ARRAY_COUNT(levi_intv_fields) },
		{ "GMST", "STRV", 0, gmst_strv_fields, ARRAY_COUNT(gmst_strv_fields) },
		{ "GMST", "INTV", 4, gmst_intv_fields, ARRAY_COUNT(gmst_intv_fields) },
		{ "GMST", "FLTV", 4, gmst_fltv_fields, ARRAY_COUNT(gmst_fltv_fields) },
		{ "INFO", "DATA", 12, info_data_fields, ARRAY_COUNT(info_data_fields) },
		{ "REGN", "WEAT", 8, regn_weat_fields, ARRAY_COUNT(regn_weat_fields) },
		{ "REGN", "WEAT", 10, regn_weat_10_fields, ARRAY_COUNT(regn_weat_10_fields) },
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
		{ "LOCK", "LKDT", 16, lock_lkdt_fields, ARRAY_COUNT(lock_lkdt_fields) },
		{ "PROB", "PBDT", 16, lock_lkdt_fields, ARRAY_COUNT(lock_lkdt_fields) },
		{ "BODY", "BYDT", 4, body_bydt_fields, ARRAY_COUNT(body_bydt_fields) },
		{ "*", "AI_W", 14, ai_w_fields, ARRAY_COUNT(ai_w_fields) },
		{ "*", "AI_T", 16, ai_t_fields, ARRAY_COUNT(ai_t_fields) },
		{ "*", "AI_F", 48, ai_f_fields, ARRAY_COUNT(ai_f_fields) },
		{ "*", "AI_E", 48, ai_f_fields, ARRAY_COUNT(ai_f_fields) },
		{ "*", "AI_A", 33, ai_a_fields, ARRAY_COUNT(ai_a_fields) },
		{ "DIAL", "DATA", 1, dial_data_fields, ARRAY_COUNT(dial_data_fields) },
		{ "*", "XSCL", 4, xscl_fields, ARRAY_COUNT(xscl_fields) },
		{ "*", "FRMR", 4, frmr_fields, ARRAY_COUNT(frmr_fields) },
		{ "CELL", "MVRF", 4, mvrf_fields, ARRAY_COUNT(mvrf_fields) },
		{ "CELL", "CNDT", 0, cell_cndt_fields, ARRAY_COUNT(cell_cndt_fields) },
		{ "*", "DELE", 4, dele_fields, ARRAY_COUNT(dele_fields) },
		{ "CELL", "WHGT", 4, whgt_fields, ARRAY_COUNT(whgt_fields) },
		{ "CELL", "NAM0", 4, nam0_fields, ARRAY_COUNT(nam0_fields) },
		{ "*", "INTV", 4, intv_4_fields, ARRAY_COUNT(intv_4_fields) },
		{ "SOUN", "DATA", 3, soun_data_fields, ARRAY_COUNT(soun_data_fields) },
		{ "PGRD", "DATA", 12, pgrd_data_fields, ARRAY_COUNT(pgrd_data_fields) },
		{ "PGRD", "PGRP", 0, land_binary_fields, ARRAY_COUNT(land_binary_fields) },
		{ "PGRD", "PGRC", 0, land_binary_fields, ARRAY_COUNT(land_binary_fields) },
		{ "SCPT", "SCVR", 0, land_binary_fields, ARRAY_COUNT(land_binary_fields) },
		{ "SCPT", "SCDT", 0, land_binary_fields, ARRAY_COUNT(land_binary_fields) },
		{ "REGN", "SNAM", 33, regn_snam_fields, ARRAY_COUNT(regn_snam_fields) },
		{ "REGN", "CNAM", 4, regn_cnam_fields, ARRAY_COUNT(regn_cnam_fields) },
		{ "*", "INDX", 4, indx_fields, ARRAY_COUNT(indx_fields) },
		{ "*", "INDX", 1, armo_indx_fields, ARRAY_COUNT(armo_indx_fields) },
		{ "BOOK", "TEXT", 0, text_fields, ARRAY_COUNT(text_fields) },
		{ "*", "QSTN", 1, quest_marker_fields, ARRAY_COUNT(quest_marker_fields) },
		{ "*", "QSTF", 1, quest_marker_fields, ARRAY_COUNT(quest_marker_fields) },
		{ "*", "QSTR", 1, quest_marker_fields, ARRAY_COUNT(quest_marker_fields) },
		{ "LEVC", "NNAM", 1, nnam_chance_fields, ARRAY_COUNT(nnam_chance_fields) },
		{ "LEVI", "NNAM", 1, nnam_chance_fields, ARRAY_COUNT(nnam_chance_fields) },
		{ "CELL", "NAM5", 4, cell_nam5_fields, ARRAY_COUNT(cell_nam5_fields) },
		{ "CELL", "FLTV", 4, cell_fltv_fields, ARRAY_COUNT(cell_fltv_fields) },
		{ "CELL", "NAM9", 4, cell_nam9_fields, ARRAY_COUNT(cell_nam9_fields) },
		{ "GLOB", "FNAM", 1, glob_fnam_fields, ARRAY_COUNT(glob_fnam_fields) },
		{ "GLOB", "FLTV", 4, glob_fltv_fields, ARRAY_COUNT(glob_fltv_fields) },
		{ "SNDG", "DATA", 4, sndg_data_fields, ARRAY_COUNT(sndg_data_fields) },
		{ "LAND", "DATA", 4, land_data_fields, ARRAY_COUNT(land_data_fields) },
		{ "LAND", "INTV", 8, land_intv_fields, ARRAY_COUNT(land_intv_fields) },
		{ "LAND", "VNML", 0, land_binary_fields, ARRAY_COUNT(land_binary_fields) },
		{ "LAND", "VHGT", 0, land_binary_fields, ARRAY_COUNT(land_binary_fields) },
		{ "LAND", "WNAM", 0, land_binary_fields, ARRAY_COUNT(land_binary_fields) },
		{ "LAND", "VCLR", 0, land_binary_fields, ARRAY_COUNT(land_binary_fields) },
		{ "LAND", "VTEX", 0, land_binary_fields, ARRAY_COUNT(land_binary_fields) },
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

const char * effect_name_by_index(int index)
{
	if (index < 0)
		return nullptr;

	int count = 0;
	while (effect_names[count])
		++count;

	if (index >= count)
		return nullptr;

	return effect_names[index];
}

const char * skill_name_by_index(int index)
{
	if (index < 0)
		return nullptr;

	int count = 0;
	while (skill_names[count])
		++count;

	if (index >= count)
		return nullptr;

	return skill_names[index];
}
