#include "omwscripts_parser.hpp"
#include "../utility/string_utils.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

struct parse_context_t
{
	std::string base_directory;
	omwscripts_result_t & result;
};

struct line_parts_t
{
	std::string tags_text;
	std::string script_path;
};

static bool is_blank_or_comment(const std::string & line)
{
	const auto trimmed = string_utils::trim(line);
	if (trimmed.empty())
		return true;

	return trimmed.front() == '#';
}

static std::vector<std::string> split_context_tags(const std::string & tags_text)
{
	std::vector<std::string> context_tags;
	std::istringstream stream(tags_text);
	std::string token;

	while (std::getline(stream, token, ','))
	{
		const auto trimmed = std::string(string_utils::trim(token));
		if (!trimmed.empty())
			context_tags.push_back(trimmed);
	}

	return context_tags;
}

static bool has_lua_extension(const std::string & script_path)
{
	if (script_path.size() < 4)
		return false;

	const auto extension = string_utils::to_lower(script_path.substr(script_path.size() - 4));
	return extension == ".lua";
}

static bool try_split_line(const std::string & line, line_parts_t & parts)
{
	const auto colon_pos = line.find(':');
	if (colon_pos == std::string::npos)
		return false;

	parts.tags_text = std::string(string_utils::trim(line.substr(0, colon_pos)));
	parts.script_path = std::string(string_utils::trim(line.substr(colon_pos + 1)));
	return true;
}

static void add_malformed_warning(int line_number, parse_context_t & context)
{
	context.result.warnings.push_back(
	    "[warning] malformed line " + std::to_string(line_number) + " in \"" + context.result.source_file +
	    "\": missing colon separator");
}

static void add_extension_warning(int line_number, parse_context_t & context)
{
	context.result.warnings.push_back(
	    "[warning] malformed line " + std::to_string(line_number) + " in \"" + context.result.source_file +
	    "\": path does not end in .lua");
}

static void check_script_exists(const line_parts_t & parts, parse_context_t & context)
{
	const auto full_path = context.base_directory + "/" + parts.script_path;
	if (std::filesystem::exists(full_path))
		return;

	context.result.warnings.push_back(
	    "[warning] script path \"" + parts.script_path + "\" referenced in \"" + context.result.source_file +
	    "\" does not exist");
}

static void append_entry(const line_parts_t & parts, int line_number, parse_context_t & context)
{
	omwscripts_entry_t entry;
	entry.context_tags = split_context_tags(parts.tags_text);
	entry.script_path = parts.script_path;
	entry.line_number = line_number;
	context.result.entries.push_back(std::move(entry));
}

static void parse_line(const std::string & line, int line_number, parse_context_t & context)
{
	if (is_blank_or_comment(line))
		return;

	line_parts_t parts;
	if (!try_split_line(line, parts))
	{
		add_malformed_warning(line_number, context);
		return;
	}

	if (!has_lua_extension(parts.script_path))
	{
		add_extension_warning(line_number, context);
		return;
	}

	check_script_exists(parts, context);
	append_entry(parts, line_number, context);
}

static std::string extract_base_directory(const std::string & file_path)
{
	const auto normalized = string_utils::normalize_path(file_path);
	const auto slash_pos = normalized.find_last_of('/');
	if (slash_pos == std::string::npos)
		return ".";

	return normalized.substr(0, slash_pos);
}

omwscripts_result_t omwscripts_parser_t::parse(const std::string & file_path, const std::string & mod_name)
{
	omwscripts_result_t result;
	result.source_file = file_path;
	result.mod_name = mod_name;

	std::ifstream file_stream(string_utils::utf8_to_path(file_path));
	if (!file_stream.is_open())
	{
		result.warnings.push_back("[error] cannot open \"" + file_path + "\"");
		return result;
	}

	parse_context_t context { extract_base_directory(file_path), result };
	std::string line;
	int line_number = 0;

	while (std::getline(file_stream, line))
	{
		++line_number;
		const auto trimmed_line = string_utils::trim_cr(line);
		parse_line(trimmed_line, line_number, context);
	}

	return result;
}
