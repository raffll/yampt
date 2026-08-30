#include "scvr_condition.hpp"

#include <array>
#include <cstdlib>

namespace
{
	constexpr size_t scvr_prefix_size = 5;

	constexpr std::array<const char *, 74> function_names = {
		"Faction Reaction Lowest",
		"Faction Reaction Highest",
		"Rank Requirement",
		"Reputation",
		"Health Percent",
		"PC Reputation",
		"PC Level",
		"PC Health Percent",
		"PC Magicka",
		"PC Fatigue",
		"PC Strength",
		"PC Block",
		"PC Armorer",
		"PC Medium Armor",
		"PC Heavy Armor",
		"PC Blunt Weapon",
		"PC Long Blade",
		"PC Axe",
		"PC Spear",
		"PC Athletics",
		"PC Enchant",
		"PC Destruction",
		"PC Alteration",
		"PC Illusion",
		"PC Conjuration",
		"PC Mysticism",
		"PC Restoration",
		"PC Alchemy",
		"PC Unarmored",
		"PC Security",
		"PC Sneak",
		"PC Acrobatics",
		"PC Light Armor",
		"PC Short Blade",
		"PC Marksman",
		"PC Mercantile",
		"PC Speechcraft",
		"PC Hand To Hand",
		"PC Gender",
		"PC Expelled",
		"PC Common Disease",
		"PC Blight Disease",
		"PC Clothing Modifier",
		"PC Crime Level",
		"Same Sex",
		"Same Race",
		"Same Faction",
		"Faction Rank Difference",
		"Detected",
		"Alarmed",
		"Choice",
		"PC Intelligence",
		"PC Willpower",
		"PC Agility",
		"PC Speed",
		"PC Endurance",
		"PC Personality",
		"PC Luck",
		"PC Corprus",
		"Weather",
		"PC Vampire",
		"Level",
		"Attacked",
		"Talked To PC",
		"PC Health",
		"Creature Target",
		"Friend Hit",
		"Fight",
		"Hello",
		"Alarm",
		"Flee",
		"Should Attack",
		"Werewolf",
		"PC Werewolf Kills",
	};

	struct type_entry_t
	{
		char type_char;
		const char * name;
	};

	constexpr std::array<type_entry_t, 12> type_entries = {{
	    { '1', "Function" },
	    { '2', "Global" },
	    { '3', "Local" },
	    { '4', "Journal" },
	    { '5', "Item" },
	    { '6', "Dead" },
	    { '7', "Not ID" },
	    { '8', "Not Faction" },
	    { '9', "Not Class" },
	    { 'A', "Not Race" },
	    { 'B', "Not Cell" },
	    { 'C', "Not Local" },
	}};

	struct operator_entry_t
	{
		char operator_char;
		const char * symbol;
	};

	constexpr std::array<operator_entry_t, 6> operator_entries = {{
	    { '0', "==" },
	    { '1', "!=" },
	    { '2', ">" },
	    { '3', ">=" },
	    { '4', "<" },
	    { '5', "<=" },
	}};
}

std::string scvr_type_name(char type_char)
{
	for (const auto & entry : type_entries)
	{
		if (entry.type_char == type_char)
			return entry.name;
	}

	return {};
}

char scvr_type_char(const std::string & type_name)
{
	for (const auto & entry : type_entries)
	{
		if (type_name == entry.name)
			return entry.type_char;
	}

	return '\0';
}

char scvr_operator_char(const std::string & operator_symbol)
{
	for (const auto & entry : operator_entries)
	{
		if (operator_symbol == entry.symbol)
			return entry.operator_char;
	}

	return '\0';
}

const std::vector<std::string> & scvr_type_names()
{
	static const std::vector<std::string> names = []
	{
		std::vector<std::string> result;
		for (const auto & entry : type_entries)
			result.emplace_back(entry.name);

		return result;
	}();

	return names;
}

const std::vector<std::string> & scvr_operator_symbols()
{
	static const std::vector<std::string> symbols = []
	{
		std::vector<std::string> result;
		for (const auto & entry : operator_entries)
			result.emplace_back(entry.symbol);

		return result;
	}();

	return symbols;
}

const char * scvr_variable_storage_name(char storage_char)
{
	switch (storage_char)
	{
	case 'f':
		return "Float";
	case 'l':
		return "Long";
	case 's':
		return "Short";
	default:
		return "";
	}
}

bool scvr_type_uses_function(const std::string & type_name)
{
	return type_name == "Function";
}

bool scvr_type_uses_variable_storage(const std::string & type_name)
{
	return type_name == "Global" || type_name == "Local" || type_name == "Not Local";
}

std::string scvr_subject_display(const char * data, size_t size)
{
	if (data == nullptr || size < 4)
		return {};

	const std::string type_name = scvr_type_name(data[1]);

	if (scvr_type_uses_function(type_name))
	{
		const char digits[3] = { data[2], data[3], '\0' };
		const char * name = scvr_function_name(std::atoi(digits));
		return name != nullptr ? name : std::string(digits);
	}

	if (scvr_type_uses_variable_storage(type_name))
		return scvr_variable_storage_name(data[2]);

	return std::string(data + 2, 2);
}

std::string scvr_subject_label(const char * data, size_t size)
{
	if (data == nullptr || size < 2)
		return "Function";

	const std::string type_name = scvr_type_name(data[1]);

	if (scvr_type_uses_function(type_name))
		return "Function";

	if (scvr_type_uses_variable_storage(type_name))
		return "Variable Type";

	return "Marker";
}

const char * scvr_operator_symbol(char comparison_char)
{
	for (const auto & entry : operator_entries)
	{
		if (entry.operator_char == comparison_char)
			return entry.symbol;
	}

	return "?";
}

const char * scvr_function_name(int function_index)
{
	if (function_index < 0 || function_index >= static_cast<int>(function_names.size()))
		return nullptr;

	return function_names[static_cast<size_t>(function_index)];
}

std::string format_scvr_condition(const scvr_condition_t & condition, const std::string & value_text)
{
	if (!condition.valid)
		return {};

	std::string subject = condition.type_name;

	if (!condition.function_name.empty())
		subject += " " + condition.function_name;
	else if (!condition.variable_name.empty())
		subject += " \"" + condition.variable_name + "\"";

	std::string result = subject + " " + condition.operator_symbol;

	if (!value_text.empty())
		result += " " + value_text;

	return result;
}

scvr_condition_t parse_scvr_condition(const char * data, size_t size)
{
	scvr_condition_t condition;

	if (data == nullptr || size < scvr_prefix_size)
		return condition;

	const char index_char = data[0];
	if (index_char >= '0' && index_char <= '9')
		condition.index = index_char - '0';

	const char type_char = data[1];
	condition.type_name = scvr_type_name(type_char);
	if (condition.type_name.empty())
		return condition;

	condition.function_chars.assign(data + 2, 2);

	if (type_char == '1')
	{
		char digits[3] = { data[2], data[3], '\0' };
		const int function_index = std::atoi(digits);
		const char * name = scvr_function_name(function_index);
		if (name != nullptr)
			condition.function_name = name;
	}

	const char comparison_char = data[4];
	condition.operator_symbol = scvr_operator_symbol(comparison_char);

	condition.variable_name.assign(data + scvr_prefix_size, size - scvr_prefix_size);

	condition.valid = true;

	return condition;
}


