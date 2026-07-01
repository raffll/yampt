#include "script_parser.hpp"
#include "../utility/string_utils.hpp"
#include <regex>

namespace {

size_t find_whole_word(const std::string & text_line, const std::string & keyword)
{
	auto is_word_char = [](char value) { return std::isalnum(static_cast<unsigned char>(value)) || value == '_'; };

	size_t search_from = 0;
	while (true)
	{
		const auto found_pos = text_line.find(keyword, search_from);
		if (found_pos == std::string::npos)
			return std::string::npos;

		if (found_pos > 0 && is_word_char(text_line[found_pos - 1]))
		{
			search_from = found_pos + 1;
			continue;
		}

		const auto after_pos = found_pos + keyword.size();
		if (after_pos < text_line.size() && is_word_char(text_line[after_pos]))
		{
			search_from = found_pos + 1;
			continue;
		}

		return found_pos;
	}
}

struct token_result_t
{
	std::string value;
	size_t offset = 0;
	bool found = false;
};

token_result_t extract_token_at(const std::string & text_input, const int position)
{
	static const std::regex token_regex("([\\w\\.\\-\\xD1]+|\".*?\")", std::regex::optimize);

	std::sregex_iterator it_current(text_input.begin(), text_input.end(), token_regex);
	std::sregex_iterator it_end;
	std::smatch match_result;

	int counter = -1;
	while (it_current != it_end && counter != position)
	{
		match_result = *it_current;
		++it_current;
		++counter;
	}

	if (counter != position || match_result.empty())
		return {};

	return { match_result[1].str(), static_cast<size_t>(match_result.position(1)), true };
}

std::string strip_quotes(const std::string & text_input)
{
	static const std::regex quote_regex("\"(.*?)\"", std::regex::optimize);

	std::smatch match_result;
	std::regex_search(text_input, match_result, quote_regex);
	if (!match_result.empty())
		return match_result[1].str();

	return text_input;
}

} // namespace

script_parser_t::script_parser_t(
    const tools_t::rec_type_t type,
    const dict_merger_t & merger,
    const std::string & record_key,
    const std::string & source_path,
    const std::string & old_script,
    const std::string & old_scdt)
    : type(type)
    , merger(&merger)
    , record_key(record_key)
    , source_path(source_path)
    , old_script(old_script)
    , old_scdt(old_scdt)
{
	if (type == tools_t::rec_type_t::sctx && !old_scdt.empty())
		m_patcher = std::make_unique<scdt_patcher_t>(old_scdt);

	convert_script();
	trim_last_new_line_chars();
}

void script_parser_t::convert_script()
{
	std::istringstream ss(old_script);
	bool is_end = false;

	while (std::getline(ss, line))
	{
		is_done = false;
		line = tools_t::trim_cr(line);
		line_lc = string_utils::to_lower(line);
		new_line = line;
		new_text.erase();
		pos = 0;
		keyword_pos = 0;
		keyword.erase();
		error = false;

		if (line_lc == "end" || (line_lc.size() > 3 && line_lc.substr(0, 4) == "end "))
		{
			is_end = true;
		}

		if (!is_end)
		{
			try
			{
				if (!is_done)
					convert_line("addtopic", 0, tools_t::rec_type_t::dial);

				if (!is_done)
					convert_line("showmap", 0, tools_t::rec_type_t::cell);

				if (!is_done)
					convert_line("centeroncell", 0, tools_t::rec_type_t::cell);

				if (!is_done)
					convert_line("getpccell", 0, tools_t::rec_type_t::cell);

				if (!is_done)
					convert_line("aifollowcell", 1, tools_t::rec_type_t::cell);

				if (!is_done)
					convert_line("aiescortcell", 1, tools_t::rec_type_t::cell);

				if (!is_done)
					convert_line("placeitemcell", 1, tools_t::rec_type_t::cell);

				if (!is_done)
					convert_line("positioncell", 4, tools_t::rec_type_t::cell);

				if (!is_done)
					convert_line();
			}
			catch (...)
			{
				tools_t::add_log("[error] unknown error in script parser\r\n");
				tools_t::add_log("line: " + line + "\r\n");
				error = true;
			}
		}

		if (error)
			dump_error();

		new_script += new_line + "\r\n";
	}
}

void script_parser_t::convert_line(
    const std::string & keyword,
    const int pos_in_expression,
    const tools_t::rec_type_t text_type)
{
	pos = find_whole_word(line_lc, keyword);
	if (pos == std::string::npos)
		return;

	if (line.size() == keyword.size())
		return;

	if (line.rfind(";", pos) != std::string::npos)
		return;

	pos = line.find_first_of(" \t,\"", pos);
	pos = line.find_first_not_of(" \t,", pos);
	if (pos == std::string::npos)
		return;

	trim_line();
	extract_text(pos_in_expression);
	remove_quotes();
	find_new_text(text_type);
	insert_new_text();

	const auto is_getpccell = keyword == "getpccell" ? true : false;
	convert_text_in_compiled(is_getpccell);

	is_done = true;
}

void script_parser_t::trim_line()
{
	tools_t::add_log("\r\n\r\n", true);
	tools_t::add_log(source_path + "\r\n", true);
	tools_t::add_log(record_key + "\r\n", true);
	tools_t::add_log("<<< " + line + "\r\n", true);

	old_text = line.substr(pos);

	tools_t::add_log("1: " + old_text + "\r\n", true);
}

void script_parser_t::extract_text(const int pos_in_expression)
{
	const auto result = extract_token_at(old_text, pos_in_expression);
	if (!result.found)
	{
		tools_t::add_log(
		    "[warning] extract_text: expected parameter at position " + std::to_string(pos_in_expression) +
		        " in: " + old_text + "\r\n",
		    true);
		error = true;
		return;
	}

	old_text = result.value;
	pos += result.offset;

	tools_t::add_log("2: " + old_text + "\r\n", true);
}

void script_parser_t::remove_quotes()
{
	const auto stripped = strip_quotes(old_text);
	if (stripped != old_text)
	{
		old_text = stripped;
		pos += 1;
	}

	tools_t::add_log("3: " + old_text + "\r\n", true);
}

void script_parser_t::find_new_text(const tools_t::rec_type_t text_type)
{
	new_text = old_text;

	const auto * search = merger->get_dict().at(text_type).find(old_text);

	if (search)
	{
		new_text = search->new_text;
	}
	else if (text_type != tools_t::rec_type_t::cell)
	{
		for (const auto & elem : merger->get_dict().at(text_type).records)
		{
			if (tools_t::case_insensitive_string_cmp(old_text, elem.key_text))
			{
				new_text = elem.new_text;
				break;
			}
		}
	}
	else
	{
		for (const auto & elem : merger->get_dict().at(text_type).records)
		{
			if (tools_t::case_insensitive_string_cmp(old_text, elem.old_text))
			{
				new_text = elem.new_text;
				break;
			}
		}
	}

	tools_t::add_log("4: " + new_text + "\r\n", true);
	if (new_text.size() < 2)
	{
		tools_t::add_log("[error] result is too short\r\n", true);
		error = true;
	}
}

void script_parser_t::insert_new_text()
{
	new_line.erase(pos, old_text.size());
	new_line.insert(pos, new_text);

	tools_t::add_log(">>> " + new_line + "\r\n", true);
}

void script_parser_t::convert_text_in_compiled(const bool is_getpccell)
{
	if (type != tools_t::rec_type_t::sctx)
		return;

	if (!m_patcher || m_patcher->is_empty())
	{
		tools_t::add_log("[error] SCDT is empty\r\n", true);
		error = true;
		return;
	}

	const auto result = m_patcher->apply_text_patch(old_text, new_text, is_getpccell);

	if (result.had_false_positive)
	{
		tools_t::add_log("[warning] false positive in " + record_key + " for: " + old_text + "\r\n", true);
	}

	if (!result.success)
	{
		tools_t::add_log("[error] not found in SCDT\r\n", true);
		error = true;
	}
}

void script_parser_t::convert_line()
{
	find_keyword();

	if (keyword_pos == std::string::npos)
		return;

	if (line.rfind(";", keyword_pos) != std::string::npos)
		return;

	if (line.find("\"", keyword_pos) == std::string::npos)
		return;

	find_new_message();
	convert_message_in_compiled();

	is_done = true;
}

void script_parser_t::find_keyword()
{
	std::map<size_t, std::string> keyword_pos_coll;
	for (const auto & keyword : tools_t::keywords)
	{
		keyword_pos = line_lc.find(keyword);
		keyword_pos_coll.insert({ keyword_pos, keyword });
	}

	keyword_pos = keyword_pos_coll.begin()->first;
	keyword = keyword_pos_coll.begin()->second;
}

void script_parser_t::find_new_message()
{
	tools_t::add_log("\r\n\r\n", true);
	tools_t::add_log(source_path + "\r\n", true);
	tools_t::add_log(record_key + "\r\n", true);
	tools_t::add_log("<<< " + line + "\r\n", true);

	auto * search = merger->get_dict().at(type).find(record_key + "^" + line);
	if (search)
	{
		if (line != search->new_text)
		{
			new_line = search->new_text;
		}
	}

	tools_t::add_log(">>> " + new_line + "\r\n", true);
}

void script_parser_t::convert_message_in_compiled()
{
	if (type != tools_t::rec_type_t::sctx)
		return;

	if (!m_patcher || m_patcher->is_empty())
	{
		tools_t::add_log("[error] SCDT is empty\r\n", true);
		error = true;
		return;
	}

	std::vector<std::string> splitted_line = split_line(line);
	std::vector<std::string> splitted_new_line = split_line(new_line);

	if (splitted_line.size() != splitted_new_line.size())
	{
		tools_t::add_log("[error] incompatible messages\r\n", true);
		error = true;
		return;
	}

	for (auto & segment : splitted_line)
		replace_vertical_lines_by_new_line(segment);

	for (auto & segment : splitted_new_line)
		replace_vertical_lines_by_new_line(segment);

	const auto result = m_patcher->apply_message_patch(splitted_line, splitted_new_line);
	if (!result.success)
	{
		tools_t::add_log("[error] message not found in SCDT\r\n", true);
		error = true;
	}
}

std::vector<std::string> script_parser_t::split_line(const std::string & cur_line) const
{
	std::string cur_line_tr = cur_line.substr(keyword_pos);
	if (cur_line_tr.find(";") != std::string::npos)
	{
		cur_line_tr = cur_line_tr.substr(0, cur_line_tr.find(";"));
	}

	std::vector<std::string> splitted_line;
	std::regex re("\"(.*?)\"", std::regex::optimize);
	std::sregex_iterator next(cur_line_tr.begin(), cur_line_tr.end(), re);
	std::sregex_iterator end;
	std::smatch found;
	while (next != end)
	{
		found = *next;
		splitted_line.push_back(found[1].str());
		next++;
	}

	/* special case if say keyword */
	/* first parameter is sound file name, so we don't need it */
	if (keyword == "say" && splitted_line.size() > 0)
	{
		splitted_line.erase(splitted_line.begin());
	}

	return splitted_line;
}

void script_parser_t::trim_last_new_line_chars()
{
	if (new_script.size() < 2)
		return;

	size_t last_nl_pos = old_script.rfind("\r\n");
	if (last_nl_pos != old_script.size() - 2 || last_nl_pos == std::string::npos)
	{
		new_script.resize(new_script.size() - 2);
	}
}

void script_parser_t::dump_error()
{
	if (type == tools_t::rec_type_t::sctx)
	{
		tools_t::add_log("----------------------------------------------------------\r\n", true);
		tools_t::add_log(tools_t::replace_non_readable_chars_with_dot(old_scdt), true);
		tools_t::add_log(
		    "\r\n----------------------------------------------------------"
		    "\r\n",
		    true);
		if (m_patcher)
			tools_t::add_log(tools_t::replace_non_readable_chars_with_dot(m_patcher->get_scdt()), true);
	}
	tools_t::add_log("\r\n----------------------------------------------------------\r\n", true);
	tools_t::add_log(old_script, true);
	tools_t::add_log("\r\n----------------------------------------------------------\r\n", true);
}

void script_parser_t::replace_vertical_lines_by_new_line(std::string & message)
{
	while (message.find("|") != std::string::npos)
	{
		message.replace(message.find("|"), 1, "\x0A");
	}
}
