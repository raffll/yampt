#include "view_tree_format.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>

const std::map<std::string, const char *> & sub_record_descriptions()
{
	static const std::map<std::string, const char *> descs = {
		{ "NAME", "ID" },
		{ "FNAM", "Name" },
		{ "MODL", "Model Filename" },
		{ "SCRI", "Script" },
		{ "ITEX", "Icon" },
		{ "ENAM", "Enchantment Effect" },
		{ "ANAM", "Faction/Owner" },
		{ "BNAM", "Script Text" },
		{ "CNAM", "Class" },
		{ "DNAM", "Destination" },
		{ "ONAM", "Actor" },
		{ "RNAM", "Race" },
		{ "INDX", "Index" },
		{ "INTV", "Integer Value" },
		{ "FLTV", "Float Value" },
		{ "STRV", "String Value" },
		{ "INAM", "Info ID" },
		{ "PNAM", "Previous Info" },
		{ "NNAM", "Next Info" },
		{ "SNAM", "Sound" },
		{ "DATA", "Data" },
		{ "FLAG", "Flags" },
		{ "NPDT", "Data" },
		{ "AIDT", "AI Data" },
		{ "WPDT", "Data" },
		{ "AODT", "Data" },
		{ "ALDT", "Data" },
		{ "ENDT", "Data" },
		{ "BKDT", "Data" },
		{ "CNDT", "Data" },
		{ "FADT", "Data" },
		{ "CLDT", "Data" },
		{ "RADT", "Data" },
		{ "SPDT", "Data" },
		{ "WEAT", "Weather" },
		{ "WHGT", "Water Height" },
		{ "AMBI", "Ambient Light" },
		{ "RGNN", "Region Name" },
		{ "DELE", "Deleted" },
		{ "SCVR", "Script Variable" },
		{ "SCHD", "Script Header" },
		{ "SCTX", "Script Source" },
		{ "SCDT", "Compiled" },
		{ "HEDR", "Header" },
		{ "MAST", "Master File" },
		{ "DODT", "Door Destination" },
		{ "FRMR", "Object Reference" },
		{ "XSCL", "Scale" },
		{ "NAM0", "Object Count" },
		{ "NAM5", "Map Color" },
		{ "NPCO", "Item" },
		{ "NPCS", "Spell/Ability" },
		{ "MEDT", "Data" },
		{ "SKDT", "Data" },
		{ "CTDT", "Data" },
		{ "LHDT", "Data" },
		{ "IRDT", "Data" },
		{ "MCDT", "Data" },
		{ "AADT", "Data" },
		{ "RIDT", "Data" },
		{ "LKDT", "Data" },
		{ "PBDT", "Data" },
		{ "KNAM", "Key" },
		{ "TNAM", "Trap" },
		{ "UNAM", "Blocked" },
		{ "AI_W", "AI Wander" },
		{ "AI_T", "AI Travel" },
		{ "AI_F", "AI Follow" },
		{ "AI_E", "AI Escort" },
		{ "AI_A", "AI Activate" },
		{ "GLOB", "Global" },
		{ "DESC", "Description" },
		{ "TEXT", "Text" },
	};
	return descs;
}

std::string format_value(const char * data, size_t size)
{
	bool printable = true;
	for (size_t i = 0; i < size; ++i)
	{
		unsigned char c = static_cast<unsigned char>(data[i]);
		if (c == 0)
			continue;

		if (c < 32 || c > 126)
		{
			printable = false;
			break;
		}
	}

	if (printable)
	{
		size_t len = 0;
		for (size_t i = 0; i < size; ++i)
		{
			if (data[i] == '\0')
				break;

			++len;
		}
		return std::string(data, len);
	}

	char buf[64];
	std::snprintf(buf, sizeof(buf), "<%zu bytes>", size);
	return std::string(buf);
}

static std::string format_flags(uint32_t value, const field_def_t & field, int max_bits)
{
	char buf[32];
	std::snprintf(buf, sizeof(buf), "0x%X", value);
	std::string result = buf;

	if (!field.flag_names || field.flag_count <= 0)
		return result;

	std::string names;
	for (int bit = 0; bit < max_bits && bit < field.flag_count; ++bit)
	{
		if (!(value & (1u << bit)))
			continue;

		if (field.flag_names[bit][0] == '_')
			continue;

		if (!names.empty())
			names += " | ";

		names += field.flag_names[bit];
	}

	if (!names.empty())
		result += " (" + names + ")";

	return result;
}

static std::string format_enum_lookup(uint32_t value, const char * const * enum_names)
{
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%u", value);
	std::string result = buf;

	if (!enum_names)
		return result;

	size_t count = 0;
	while (enum_names[count])
		++count;

	if (value < count)
		result += " (" + std::string(enum_names[value]) + ")";

	return result;
}

static std::string read_fixed_string(const char * ptr, size_t field_size, size_t data_size, size_t field_offset)
{
	size_t len = 0;
	for (size_t i = 0; i < field_size && field_offset + i < data_size; ++i)
	{
		if (ptr[i] == '\0')
			break;

		++len;
	}
	return std::string(ptr, len);
}

std::string decode_field(const field_def_t & field, const char * data, size_t data_size)
{
	if (field.offset + field.size > data_size && field.type != field_type_t::string_var)
		return "";

	const char * ptr = data + field.offset;
	char buf[128];

	switch (field.type)
	{
	case field_type_t::u8:
	{
		uint8_t val = 0;
		std::memcpy(&val, ptr, 1);
		std::snprintf(buf, sizeof(buf), "%u", val);
		return buf;
	}
	case field_type_t::u16:
	{
		uint16_t val = 0;
		std::memcpy(&val, ptr, 2);
		std::snprintf(buf, sizeof(buf), "%u", val);
		return buf;
	}
	case field_type_t::u32:
	{
		uint32_t val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "%u", val);
		return buf;
	}
	case field_type_t::i8:
	{
		int8_t val = 0;
		std::memcpy(&val, ptr, 1);
		std::snprintf(buf, sizeof(buf), "%d", val);
		std::string result = buf;

		if (field.enum_names && val >= 0)
		{
			size_t count = 0;
			while (field.enum_names[count])
				++count;

			if (static_cast<size_t>(val) < count)
				result += " (" + std::string(field.enum_names[val]) + ")";
		}
		else if (field.enum_names && val == -1)
		{
			result += " (None)";
		}

		return result;
	}
	case field_type_t::i16:
	{
		int16_t val = 0;
		std::memcpy(&val, ptr, 2);
		std::snprintf(buf, sizeof(buf), "%d", val);
		return buf;
	}
	case field_type_t::i32:
	{
		int32_t val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "%d", val);
		return buf;
	}
	case field_type_t::f32:
	{
		float val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "%.4f", val);
		return buf;
	}
	case field_type_t::string_fixed:
		return read_fixed_string(ptr, field.size, data_size, field.offset);

	case field_type_t::string_var:
	{
		if (field.offset >= data_size)
			return "";

		const size_t remaining = data_size - field.offset;
		return read_fixed_string(ptr, remaining, data_size, field.offset);
	}
	case field_type_t::flags_u8:
	{
		uint8_t val = 0;
		std::memcpy(&val, ptr, 1);
		return format_flags(val, field, 8);
	}
	case field_type_t::flags_u16:
	{
		uint16_t val = 0;
		std::memcpy(&val, ptr, 2);
		return format_flags(val, field, 16);
	}
	case field_type_t::flags_u32:
	{
		uint32_t val = 0;
		std::memcpy(&val, ptr, 4);
		return format_flags(val, field, 32);
	}
	case field_type_t::enum_u8:
	{
		uint8_t val = 0;
		std::memcpy(&val, ptr, 1);
		return format_enum_lookup(val, field.enum_names);
	}
	case field_type_t::enum_u16:
	{
		uint16_t val = 0;
		std::memcpy(&val, ptr, 2);
		return format_enum_lookup(val, field.enum_names);
	}
	case field_type_t::enum_u32:
	{
		uint32_t val = 0;
		std::memcpy(&val, ptr, 4);
		return format_enum_lookup(val, field.enum_names);
	}
	case field_type_t::raw:
	{
		std::string hex_output;
		constexpr size_t max_hex_bytes = 64;
		const size_t limit = std::min(field.size, max_hex_bytes);
		for (size_t i = 0; i < limit && field.offset + i < data_size; ++i)
		{
			char hbuf[4];
			std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(ptr[i]));
			if (!hex_output.empty())
				hex_output += ' ';

			hex_output += hbuf;
		}

		if (field.size > max_hex_bytes)
			hex_output += " ...";

		return hex_output;
	}
	}

	return "";
}

static const std::map<std::pair<std::string, std::string>, const char *> & context_descriptions()
{
	static const std::map<std::pair<std::string, std::string>, const char *> descs = {
		{ { "ARMO", "BNAM" }, "Male Part Name" }, { { "ARMO", "CNAM" }, "Female Part Name" },
		{ { "CLOT", "BNAM" }, "Male Part Name" }, { { "CLOT", "CNAM" }, "Female Part Name" },
		{ { "CELL", "ANAM" }, "Owner" },          { { "CELL", "BNAM" }, "Global/Rank" },
		{ { "NPC_", "ANAM" }, "Faction" },        { { "NPC_", "BNAM" }, "Head Model" },
		{ { "NPC_", "CNAM" }, "Class" },          { { "INFO", "ANAM" }, "Cell" },
	};
	return descs;
}

static const std::map<std::string, const char *> & record_display_names()
{
	static const std::map<std::string, const char *> names = {
		{ "ACTI", "Activator" },
		{ "ALCH", "Potion" },
		{ "APPA", "Apparatus" },
		{ "ARMO", "Armor" },
		{ "BOOK", "Book" },
		{ "CELL", "Cell" },
		{ "CLAS", "Class" },
		{ "CLOT", "Clothing" },
		{ "CONT", "Container" },
		{ "CREA", "Creature" },
		{ "ENCH", "Enchantment" },
		{ "FACT", "Faction" },
		{ "GLOB", "Global" },
		{ "INFO", "Info" },
		{ "INGR", "Ingredient" },
		{ "LEVI", "Leveled Item" },
		{ "LEVC", "Leveled Creature" },
		{ "LIGH", "Light" },
		{ "LOCK", "Lockpick" },
		{ "MGEF", "Magic Effect" },
		{ "MISC", "Misc Item" },
		{ "NPC_", "NPC" },
		{ "PROB", "Probe" },
		{ "RACE", "Race" },
		{ "REGN", "Region" },
		{ "REPA", "Repair Item" },
		{ "SCPT", "Script" },
		{ "SKIL", "Skill" },
		{ "SPEL", "Spell" },
		{ "WEAP", "Weapon" },
	};
	return names;
}

static std::string build_schema_label(
    const std::string & sub_type,
    const std::string & record_type,
    const std::map<std::string, const char *> & descs)
{
	const auto & names = record_display_names();
	auto rit = names.find(record_type);
	const auto & parent_name = (rit != names.end()) ? std::string(rit->second) : record_type;

	auto it = descs.find(sub_type);
	if (it != descs.end())
		return sub_type + " - " + parent_name + " " + it->second;

	return sub_type + " - " + parent_name + " Data";
}

std::string make_sub_label(const std::string & sub_type, const std::string & record_type, size_t data_size)
{
	const auto & ctx_descs = context_descriptions();
	auto ctx_it = ctx_descs.find({ record_type, sub_type });
	if (ctx_it != ctx_descs.end())
		return sub_type + " - " + ctx_it->second;

	const auto & descs = sub_record_descriptions();
	const auto * schema = find_schema(record_type, sub_type, data_size);

	if (schema)
		return build_schema_label(sub_type, record_type, descs);

	auto it = descs.find(sub_type);
	if (it != descs.end())
		return sub_type + " - " + it->second;

	return sub_type;
}
