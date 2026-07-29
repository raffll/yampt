#include "handler_parser.hpp"
#include "../utility/string_utils.hpp"
#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

static constexpr size_t max_handler_body_length = 10000;

struct known_interface_t
{
	std::string interface_name;
	std::vector<std::string> method_names;
};

struct registration_match_t
{
	std::string interface_name;
	std::string method_name;
	std::string type_argument;
	std::string callback_expression;
	int line_number;
};

enum class alias_kind_t
{
	interfaces_root,
	specific_interface
};

struct alias_entry_t
{
	alias_kind_t alias_kind;
	std::string interface_name;
};

static const std::vector<known_interface_t> & known_interfaces()
{
	static const std::vector<known_interface_t> interfaces = {
		{ "SkillProgression", { "addSkillLevelUpHandler", "addSkillUsedHandler", "addHandlerForType" } },
		{ "ItemUsage", { "addHandlerForType" } },
		{ "Activation", { "addHandlerForType" } },
	};
	return interfaces;
}

static std::vector<std::string> split_lines(const std::string & content)
{
	std::vector<std::string> lines;
	std::istringstream stream(content);
	std::string current_line;

	while (std::getline(stream, current_line))
	{
		if (!current_line.empty() && current_line.back() == '\r')
			current_line.pop_back();

		lines.push_back(std::move(current_line));
	}

	return lines;
}

static void blank_range(std::string & content, size_t start, size_t count)
{
	for (size_t index = start; index < start + count && index < content.size(); ++index)
	{
		if (content[index] != '\n' && content[index] != '\r')
			content[index] = ' ';
	}
}

static size_t skip_block_comment(std::string & content, size_t start_pos)
{
	const auto close_pos = content.find("]]", start_pos + 4);
	if (close_pos == std::string::npos)
	{
		blank_range(content, start_pos, content.size() - start_pos);
		return content.size();
	}

	blank_range(content, start_pos, close_pos + 2 - start_pos);
	return close_pos + 2;
}

static size_t skip_line_comment(std::string & content, size_t start_pos)
{
	const auto newline_pos = content.find('\n', start_pos);
	if (newline_pos == std::string::npos)
	{
		blank_range(content, start_pos, content.size() - start_pos);
		return content.size();
	}

	blank_range(content, start_pos, newline_pos - start_pos);
	return newline_pos;
}

static size_t skip_quoted_string(std::string & content, size_t start_pos)
{
	const char quote_char = content[start_pos];
	size_t index = start_pos + 1;

	while (index < content.size())
	{
		if (content[index] == '\\')
		{
			++index;
		}
		else if (content[index] == quote_char)
		{
			blank_range(content, start_pos + 1, index - start_pos - 1);
			return index + 1;
		}

		++index;
	}

	blank_range(content, start_pos + 1, content.size() - start_pos - 1);
	return content.size();
}

static size_t skip_long_string(std::string & content, size_t start_pos)
{
	const auto close_pos = content.find("]]", start_pos + 2);
	if (close_pos == std::string::npos)
	{
		blank_range(content, start_pos + 2, content.size() - start_pos - 2);
		return content.size();
	}

	blank_range(content, start_pos + 2, close_pos - start_pos - 2);
	return close_pos + 2;
}

static bool is_block_comment_start(const std::string & content, size_t position)
{
	if (position + 3 >= content.size())
		return false;

	return content[position] == '-' && content[position + 1] == '-' && content[position + 2] == '[' &&
	       content[position + 3] == '[';
}

static bool is_line_comment_start(const std::string & content, size_t position)
{
	if (position + 1 >= content.size())
		return false;

	return content[position] == '-' && content[position + 1] == '-';
}

static bool is_long_string_start(const std::string & content, size_t position)
{
	if (position + 1 >= content.size())
		return false;

	return content[position] == '[' && content[position + 1] == '[';
}

static size_t skip_over_quoted_string(const std::string & content, size_t start_pos)
{
	const char quote_char = content[start_pos];
	size_t index = start_pos + 1;

	while (index < content.size())
	{
		if (content[index] == '\\')
			++index;
		else if (content[index] == quote_char)
			return index + 1;

		++index;
	}

	return content.size();
}

static size_t skip_over_long_string(const std::string & content, size_t start_pos)
{
	const auto close_pos = content.find("]]", start_pos + 2);
	if (close_pos == std::string::npos)
		return content.size();

	return close_pos + 2;
}

static std::string strip_comments_only(const std::string & source)
{
	std::string content = source;
	size_t position = 0;

	while (position < content.size())
	{
		if (is_block_comment_start(content, position))
			position = skip_block_comment(content, position);
		else if (is_line_comment_start(content, position))
			position = skip_line_comment(content, position);
		else if (content[position] == '"' || content[position] == '\'')
			position = skip_over_quoted_string(content, position);
		else if (is_long_string_start(content, position))
			position = skip_over_long_string(content, position);
		else
			++position;
	}

	return content;
}

static std::string strip_comments_and_strings(const std::string & source)
{
	std::string content = source;
	size_t position = 0;

	while (position < content.size())
	{
		if (is_block_comment_start(content, position))
			position = skip_block_comment(content, position);
		else if (is_line_comment_start(content, position))
			position = skip_line_comment(content, position);
		else if (content[position] == '"' || content[position] == '\'')
			position = skip_quoted_string(content, position);
		else if (is_long_string_start(content, position))
			position = skip_long_string(content, position);
		else
			++position;
	}

	return content;
}

static std::string extract_parenthesized_content(const std::string & source_line, size_t open_paren_pos)
{
	int depth = 0;
	std::string content;

	for (size_t index = open_paren_pos; index < source_line.size(); ++index)
	{
		if (source_line[index] == '(')
		{
			++depth;
			if (depth > 1)
				content += source_line[index];
		}
		else if (source_line[index] == ')')
		{
			--depth;
			if (depth == 0)
				return content;

			content += source_line[index];
		}
		else if (depth > 0)
		{
			content += source_line[index];
		}
	}

	return content;
}

static bool extract_type_and_callback(const std::string & arguments, registration_match_t & match)
{
	const auto comma_pos = arguments.find(',');
	if (comma_pos == std::string::npos)
	{
		match.type_argument = "";
		match.callback_expression = std::string(string_utils::trim(arguments));
		return true;
	}

	match.type_argument = std::string(string_utils::trim(std::string_view(arguments).substr(0, comma_pos)));
	match.callback_expression = std::string(string_utils::trim(std::string_view(arguments).substr(comma_pos + 1)));
	return true;
}

struct line_context_t
{
	const std::string & line_text;
	int line_number;
};

static bool try_match_direct_pattern(const line_context_t & context, registration_match_t & match)
{
	for (const auto & iface : known_interfaces())
	{
		for (const auto & method : iface.method_names)
		{
			const auto pattern = "." + iface.interface_name + "." + method + "(";
			const auto found_pos = context.line_text.find(pattern);
			if (found_pos == std::string::npos)
				continue;

			const auto paren_pos = found_pos + pattern.size() - 1;
			const auto arguments = extract_parenthesized_content(context.line_text, paren_pos);

			match.interface_name = iface.interface_name;
			match.method_name = method;
			match.line_number = context.line_number;

			if (method == "addHandlerForType")
				extract_type_and_callback(arguments, match);
			else
			{
				match.type_argument = "";
				match.callback_expression = std::string(string_utils::trim(arguments));
			}

			return true;
		}
	}

	return false;
}

using alias_map_t = std::unordered_map<std::string, alias_entry_t>;

static std::string extract_local_variable_name(const std::string & line_text)
{
	const auto trimmed = std::string(string_utils::trim(line_text));
	if (trimmed.rfind("local ", 0) != 0)
		return "";

	const auto after_local = std::string_view(trimmed).substr(6);
	const auto name_end = after_local.find_first_of(" \t=");
	if (name_end == std::string_view::npos)
		return "";

	return std::string(after_local.substr(0, name_end));
}

static bool is_interfaces_require(const std::string & line_text)
{
	const auto single_quote = line_text.find("require('openmw.interfaces')");
	if (single_quote != std::string::npos)
		return true;

	const auto double_quote = line_text.find("require(\"openmw.interfaces\")");
	return double_quote != std::string::npos;
}

static std::string find_rhs_alias_source(const std::string & line_text)
{
	const auto equals_pos = line_text.find('=');
	if (equals_pos == std::string::npos)
		return "";

	const auto right_side = std::string(string_utils::trim(std::string_view(line_text).substr(equals_pos + 1)));

	return right_side;
}

static std::string extract_dot_access_target(const std::string & right_side)
{
	const auto dot_pos = right_side.find('.');
	if (dot_pos == std::string::npos)
		return "";

	const auto after_dot = std::string_view(right_side).substr(dot_pos + 1);
	const auto end_pos = after_dot.find_first_of(" \t.([");
	if (end_pos == std::string_view::npos)
		return std::string(after_dot);

	return std::string(after_dot.substr(0, end_pos));
}

static std::string extract_dot_access_prefix(const std::string & right_side)
{
	const auto dot_pos = right_side.find('.');
	if (dot_pos == std::string::npos)
		return "";

	return std::string(std::string_view(right_side).substr(0, dot_pos));
}

static void try_add_require_alias(const std::string & line_text, alias_map_t & aliases)
{
	if (!is_interfaces_require(line_text))
		return;

	const auto variable_name = extract_local_variable_name(line_text);
	if (variable_name.empty())
		return;

	alias_entry_t alias_entry;
	alias_entry.alias_kind = alias_kind_t::interfaces_root;
	alias_entry.interface_name = "";
	aliases[variable_name] = alias_entry;
}

static void try_add_derived_alias(const std::string & line_text, alias_map_t & aliases)
{
	const auto variable_name = extract_local_variable_name(line_text);
	if (variable_name.empty())
		return;

	const auto right_side = find_rhs_alias_source(line_text);
	if (right_side.empty())
		return;

	const auto prefix = extract_dot_access_prefix(right_side);
	if (prefix.empty())
		return;

	const auto it_found = aliases.find(prefix);
	if (it_found == aliases.end())
		return;

	const auto target_name = extract_dot_access_target(right_side);
	if (target_name.empty())
		return;

	if (it_found->second.alias_kind == alias_kind_t::interfaces_root)
	{
		alias_entry_t alias_entry;
		alias_entry.alias_kind = alias_kind_t::specific_interface;
		alias_entry.interface_name = target_name;
		aliases[variable_name] = alias_entry;
		return;
	}

	alias_entry_t alias_entry;
	alias_entry.alias_kind = alias_kind_t::specific_interface;
	alias_entry.interface_name = it_found->second.interface_name;
	aliases[variable_name] = alias_entry;
}

static alias_map_t build_alias_map(const std::vector<std::string> & lines)
{
	alias_map_t aliases;

	for (const auto & current_line : lines)
	{
		try_add_require_alias(current_line, aliases);
		try_add_derived_alias(current_line, aliases);
	}

	return aliases;
}

struct alias_match_input_t
{
	const line_context_t & context;
	const std::string & alias_name;
	const known_interface_t & iface;
};

static bool try_match_alias_method(const alias_match_input_t & input, registration_match_t & match)
{
	for (const auto & method : input.iface.method_names)
	{
		const auto pattern = input.alias_name + "." + method + "(";
		const auto found_pos = input.context.line_text.find(pattern);
		if (found_pos == std::string::npos)
			continue;

		if (found_pos > 0 && std::isalnum(static_cast<unsigned char>(input.context.line_text[found_pos - 1])))
			continue;

		const auto paren_pos = found_pos + pattern.size() - 1;
		const auto arguments = extract_parenthesized_content(input.context.line_text, paren_pos);

		match.interface_name = input.iface.interface_name;
		match.method_name = method;
		match.line_number = input.context.line_number;

		if (method == "addHandlerForType")
			extract_type_and_callback(arguments, match);
		else
		{
			match.type_argument = "";
			match.callback_expression = std::string(string_utils::trim(arguments));
		}

		return true;
	}

	return false;
}

struct alias_scan_input_t
{
	const line_context_t & context;
	const alias_map_t & aliases;
};

static bool try_match_alias_pattern(const alias_scan_input_t & scan_input, registration_match_t & match)
{
	for (const auto & [alias_name, alias_entry] : scan_input.aliases)
	{
		if (alias_entry.alias_kind == alias_kind_t::interfaces_root)
			continue;

		for (const auto & iface : known_interfaces())
		{
			if (iface.interface_name != alias_entry.interface_name)
				continue;

			const alias_match_input_t input { scan_input.context, alias_name, iface };

			if (try_match_alias_method(input, match))
				return true;
		}
	}

	return false;
}

static bool is_word_boundary_char(char character)
{
	return !std::isalnum(static_cast<unsigned char>(character)) && character != '_';
}

static bool is_word_at_position(const std::string & text, size_t position, const std::string & word)
{
	if (position + word.size() > text.size())
		return false;

	if (text.compare(position, word.size(), word) != 0)
		return false;

	if (position > 0 && !is_word_boundary_char(text[position - 1]))
		return false;

	if (position + word.size() < text.size() && !is_word_boundary_char(text[position + word.size()]))
		return false;

	return true;
}

static size_t find_function_keyword_near(const std::string & content, size_t search_start)
{
	auto position = content.find("function", search_start);

	while (position != std::string::npos)
	{
		if (is_word_at_position(content, position, "function"))
			return position;

		position = content.find("function", position + 1);
	}

	return std::string::npos;
}

static size_t skip_non_code_content(const std::string & content, size_t position)
{
	if (content[position] == '-' && position + 1 < content.size() && content[position + 1] == '-')
	{
		if (position + 3 < content.size() && content[position + 2] == '[' && content[position + 3] == '[')
		{
			const auto close = content.find("]]", position + 4);
			return (close == std::string::npos) ? content.size() : close + 2;
		}

		const auto newline = content.find('\n', position);
		return (newline == std::string::npos) ? content.size() : newline + 1;
	}

	if (content[position] == '"' || content[position] == '\'')
	{
		const char quote = content[position];
		auto index = position + 1;
		while (index < content.size() && content[index] != quote)
		{
			if (content[index] == '\\')
				++index;

			++index;
		}

		return (index < content.size()) ? index + 1 : content.size();
	}

	if (content[position] == '[' && position + 1 < content.size() && content[position + 1] == '[')
	{
		const auto close = content.find("]]", position + 2);
		return (close == std::string::npos) ? content.size() : close + 2;
	}

	return position;
}

static bool is_non_code_start(const std::string & content, size_t position)
{
	if (content[position] == '-' && position + 1 < content.size() && content[position + 1] == '-')
		return true;

	if (content[position] == '"' || content[position] == '\'')
		return true;

	if (content[position] == '[' && position + 1 < content.size() && content[position + 1] == '[')
		return true;

	return false;
}

static size_t find_matching_end(const std::string & content, size_t start_after_function)
{
	int depth = 1;
	size_t position = start_after_function;

	while (position < content.size() && depth > 0)
	{
		if (is_non_code_start(content, position))
		{
			position = skip_non_code_content(content, position);
			continue;
		}

		if (is_word_at_position(content, position, "function"))
		{
			++depth;
			position += 8;
			continue;
		}

		if (is_word_at_position(content, position, "if"))
		{
			++depth;
			position += 2;
			continue;
		}

		if (is_word_at_position(content, position, "do"))
		{
			++depth;
			position += 2;
			continue;
		}

		if (is_word_at_position(content, position, "for"))
		{
			++depth;
			position += 3;
			continue;
		}

		if (is_word_at_position(content, position, "while"))
		{
			++depth;
			position += 5;
			continue;
		}

		if (is_word_at_position(content, position, "end"))
		{
			--depth;
			if (depth == 0)
				return position + 3;

			position += 3;
			continue;
		}

		++position;
	}

	return std::string::npos;
}

static std::string extract_handler_body(const std::string & original_content, size_t registration_line_pos)
{
	const auto function_pos = find_function_keyword_near(original_content, registration_line_pos);
	if (function_pos == std::string::npos)
		return "";

	const auto after_keyword = function_pos + 8;
	const auto end_pos = find_matching_end(original_content, after_keyword);
	if (end_pos == std::string::npos)
		return "";

	const auto body_length = std::min(end_pos - function_pos, max_handler_body_length);
	return original_content.substr(function_pos, body_length);
}

static bool contains_return_false(const std::string & body)
{
	size_t position = 0;

	while (position < body.size())
	{
		position = body.find("return", position);
		if (position == std::string::npos)
			return false;

		if (!is_word_at_position(body, position, "return"))
		{
			++position;
			continue;
		}

		auto after_return = position + 6;
		while (after_return < body.size() && (body[after_return] == ' ' || body[after_return] == '\t'))
			++after_return;

		if (is_word_at_position(body, after_return, "false"))
			return true;

		position = after_return;
	}

	return false;
}

static bool contains_options_mutation(const std::string & body)
{
	size_t position = 0;

	while (position < body.size())
	{
		position = body.find("options.", position);
		if (position == std::string::npos)
			return false;

		if (position > 0 && !is_word_boundary_char(body[position - 1]))
		{
			++position;
			continue;
		}

		auto after_dot = position + 8;
		while (after_dot < body.size() &&
		       (std::isalnum(static_cast<unsigned char>(body[after_dot])) || body[after_dot] == '_'))
			++after_dot;

		if (after_dot == position + 8)
		{
			++position;
			continue;
		}

		while (after_dot < body.size() && (body[after_dot] == ' ' || body[after_dot] == '\t'))
			++after_dot;

		if (after_dot < body.size() && body[after_dot] == '=' &&
		    (after_dot + 1 >= body.size() || body[after_dot + 1] != '='))
			return true;

		position = after_dot;
	}

	return false;
}

static handler_class_t classify_handler_body(const std::string & body)
{
	if (contains_return_false(body))
		return handler_class_t::blocking;

	if (contains_options_mutation(body))
		return handler_class_t::mutating;

	return handler_class_t::passive;
}

static size_t find_return_false_position(const std::string & body)
{
	size_t position = 0;

	while (position < body.size())
	{
		position = body.find("return", position);
		if (position == std::string::npos)
			return std::string::npos;

		if (!is_word_at_position(body, position, "return"))
		{
			++position;
			continue;
		}

		auto after_return = position + 6;
		while (after_return < body.size() && (body[after_return] == ' ' || body[after_return] == '\t'))
			++after_return;

		if (is_word_at_position(body, after_return, "false"))
			return position;

		position = after_return;
	}

	return std::string::npos;
}

static std::string extract_if_condition_before(const std::string & body, size_t return_false_pos)
{
	auto search_pos = return_false_pos;

	while (search_pos > 0)
	{
		auto if_pos = body.rfind("if", search_pos - 1);
		if (if_pos == std::string::npos)
			return "";

		if (!is_word_at_position(body, if_pos, "if"))
		{
			search_pos = if_pos;
			continue;
		}

		const auto after_if = if_pos + 2;
		const auto then_pos = body.find("then", after_if);
		if (then_pos == std::string::npos || then_pos >= return_false_pos)
			return "";

		if (!is_word_at_position(body, then_pos, "then"))
		{
			search_pos = if_pos;
			continue;
		}

		const auto condition = std::string_view(body).substr(after_if, then_pos - after_if);
		return std::string(string_utils::trim(condition));
	}

	return "";
}

static std::string extract_blocking_condition(const std::string & body)
{
	const auto return_pos = find_return_false_position(body);
	if (return_pos == std::string::npos)
		return "";

	return extract_if_condition_before(body, return_pos);
}

static size_t find_line_start_in_original(const std::string & content, int target_line)
{
	int current_line = 1;
	size_t position = 0;

	while (current_line < target_line && position < content.size())
	{
		const auto newline = content.find('\n', position);
		if (newline == std::string::npos)
			return content.size();

		position = newline + 1;
		++current_line;
	}

	return position;
}

struct output_context_t
{
	const std::string & script_path;
	const std::string & mod_name;
};

struct build_context_t
{
	const output_context_t & output;
	const std::string & original_content;
};

static handler_registration_t build_registration(const registration_match_t & match, const build_context_t & context)
{
	handler_registration_t registration;
	registration.interface_name = match.interface_name;
	registration.method_name = match.method_name;
	registration.type_argument = match.type_argument;
	registration.callback_expression = match.callback_expression;
	registration.script_path = context.output.script_path;
	registration.line_number = match.line_number;
	registration.mod_name = context.output.mod_name;

	const auto line_start = find_line_start_in_original(context.original_content, match.line_number);
	registration.handler_body = extract_handler_body(context.original_content, line_start);
	registration.classification = classify_handler_body(registration.handler_body);
	registration.blocking_condition = (registration.classification == handler_class_t::blocking)
	                                      ? extract_blocking_condition(registration.handler_body)
	                                      : "";
	return registration;
}

std::vector<handler_registration_t> handler_parser_t::parse(const parse_input_t & input)
{
	std::vector<handler_registration_t> registrations;
	const auto comment_stripped = strip_comments_only(input.file_content);
	const auto alias_lines = split_lines(comment_stripped);
	const auto aliases = build_alias_map(alias_lines);
	const auto preprocessed = strip_comments_and_strings(input.file_content);
	const auto lines = split_lines(preprocessed);
	const output_context_t output { input.script_path, input.mod_name };
	const build_context_t context { output, input.file_content };

	for (size_t index = 0; index < lines.size(); ++index)
	{
		registration_match_t match;
		const line_context_t line_context { lines[index], static_cast<int>(index + 1) };

		if (try_match_direct_pattern(line_context, match))
		{
			registrations.push_back(build_registration(match, context));
			continue;
		}

		const alias_scan_input_t scan_input { line_context, aliases };

		if (try_match_alias_pattern(scan_input, match))
			registrations.push_back(build_registration(match, context));
	}

	return registrations;
}
