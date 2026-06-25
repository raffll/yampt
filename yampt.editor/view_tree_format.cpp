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
		{ "NPDT", "NPC Data" },
		{ "AIDT", "AI Data" },
		{ "WPDT", "Weapon Data" },
		{ "AODT", "Armor Data" },
		{ "ALDT", "Potion Data" },
		{ "ENDT", "Enchantment Data" },
		{ "BKDT", "Book Data" },
		{ "CNDT", "Container Data" },
		{ "FADT", "Faction Data" },
		{ "CLDT", "Class Data" },
		{ "RADT", "Race Data" },
		{ "SPDT", "Spell Data" },
		{ "WEAT", "Weather" },
		{ "WHGT", "Water Height" },
		{ "AMBI", "Ambient Light" },
		{ "RGNN", "Region Name" },
		{ "DELE", "Deleted" },
		{ "SCVR", "Script Variable" },
		{ "SCHD", "Script Header" },
		{ "SCTX", "Script Source" },
		{ "SCDT", "Script Data" },
		{ "HEDR", "Header" },
		{ "MAST", "Master File" },
		{ "DODT", "Door Destination" },
		{ "FRMR", "Object Reference" },
		{ "XSCL", "Scale" },
		{ "NAM0", "Object Count" },
		{ "NAM5", "Map Color" },
		{ "NPCO", "Item" },
		{ "NPCS", "Spell/Ability" },
		{ "MEDT", "Effect Data" },
		{ "SKDT", "Skill Data" },
		{ "CTDT", "Clothing Data" },
		{ "LHDT", "Light Data" },
		{ "IRDT", "Ingredient Data" },
		{ "MCDT", "Misc Item Data" },
		{ "AADT", "Apparatus Data" },
		{ "RIDT", "Repair Data" },
		{ "LKDT", "Lock Data" },
		{ "PBDT", "Probe Data" },
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
	{
		size_t len = 0;
		for (size_t i = 0; i < field.size && field.offset + i < data_size; ++i)
		{
			if (ptr[i] == '\0')
				break;

			++len;
		}
		return std::string(ptr, len);
	}
	case field_type_t::string_var:
	{
		if (field.offset >= data_size)
			return "";

		size_t remaining = data_size - field.offset;
		size_t len = 0;
		for (size_t i = 0; i < remaining; ++i)
		{
			if (ptr[i] == '\0')
				break;

			++len;
		}
		return std::string(ptr, len);
	}
	case field_type_t::flags_u8:
	{
		uint8_t val = 0;
		std::memcpy(&val, ptr, 1);
		std::snprintf(buf, sizeof(buf), "0x%X", val);
		std::string result = buf;

		if (field.flag_names && field.flag_count > 0)
		{
			std::string names;
			for (int bit = 0; bit < 8 && bit < field.flag_count; ++bit)
			{
				if (!(val & (1u << bit)))
					continue;

				if (field.flag_names[bit][0] == '_')
					continue;

				if (!names.empty())
					names += " | ";

				names += field.flag_names[bit];
			}

			if (!names.empty())
				result += " (" + names + ")";
		}

		return result;
	}
	case field_type_t::flags_u16:
	{
		uint16_t val = 0;
		std::memcpy(&val, ptr, 2);
		std::snprintf(buf, sizeof(buf), "0x%X", val);
		std::string result = buf;

		if (field.flag_names && field.flag_count > 0)
		{
			std::string names;
			for (int bit = 0; bit < 16 && bit < field.flag_count; ++bit)
			{
				if (!(val & (1u << bit)))
					continue;

				if (field.flag_names[bit][0] == '_')
					continue;

				if (!names.empty())
					names += " | ";

				names += field.flag_names[bit];
			}

			if (!names.empty())
				result += " (" + names + ")";
		}

		return result;
	}
	case field_type_t::flags_u32:
	{
		uint32_t val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "0x%X", val);
		std::string result = buf;

		if (field.flag_names && field.flag_count > 0)
		{
			std::string names;
			for (int bit = 0; bit < 32 && bit < field.flag_count; ++bit)
			{
				if (!(val & (1u << bit)))
					continue;

				if (field.flag_names[bit][0] == '_')
					continue;

				if (!names.empty())
					names += " | ";

				names += field.flag_names[bit];
			}

			if (!names.empty())
				result += " (" + names + ")";
		}

		return result;
	}
	case field_type_t::enum_u8:
	{
		uint8_t val = 0;
		std::memcpy(&val, ptr, 1);
		std::snprintf(buf, sizeof(buf), "%u", val);
		std::string result = buf;

		if (field.enum_names)
		{
			size_t count = 0;
			while (field.enum_names[count])
				++count;

			if (val < count)
				result += " (" + std::string(field.enum_names[val]) + ")";
		}

		return result;
	}
	case field_type_t::enum_u16:
	{
		uint16_t val = 0;
		std::memcpy(&val, ptr, 2);
		std::snprintf(buf, sizeof(buf), "%u", val);
		std::string result = buf;

		if (field.enum_names)
		{
			size_t count = 0;
			while (field.enum_names[count])
				++count;

			if (val < count)
				result += " (" + std::string(field.enum_names[val]) + ")";
		}

		return result;
	}
	case field_type_t::enum_u32:
	{
		uint32_t val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "%u", val);
		std::string result = buf;

		if (field.enum_names)
		{
			size_t count = 0;
			while (field.enum_names[count])
				++count;

			if (val < count)
				result += " (" + std::string(field.enum_names[val]) + ")";
		}

		return result;
	}
	case field_type_t::raw:
	{
		std::string hex;
		size_t limit = std::min(field.size, static_cast<size_t>(64));
		for (size_t i = 0; i < limit && field.offset + i < data_size; ++i)
		{
			char hbuf[4];
			std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(ptr[i]));
			if (!hex.empty())
				hex += ' ';

			hex += hbuf;
		}

		if (field.size > 64)
			hex += " ...";

		return hex;
	}
	}

	return "";
}

std::string make_sub_label(const std::string & sub_type, const std::string & record_type, size_t data_size)
{
	const auto & descs = sub_record_descriptions();
	auto it = descs.find(sub_type);

	const sub_record_schema_t * schema = find_schema(record_type, sub_type, data_size);

	if (schema)
	{
		std::string parent_name;

		static const std::map<std::string, const char *> record_names = {
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

		auto rit = record_names.find(record_type);
		if (rit != record_names.end())
			parent_name = rit->second;
		else
			parent_name = record_type;

		if (it != descs.end())
			return sub_type + " - " + parent_name + " " + it->second;

		return sub_type + " - " + parent_name + " Data";
	}

	if (it != descs.end())
		return sub_type + " - " + it->second;

	return sub_type;
}
