#include "script_parser.hpp"
#include <regex>

script_parser_t::script_parser_t(
    const tools_t::rec_type_t type,
    const dict_merger_t & merger,
    const std::string & script_name,
    const std::string & file_name,
    const std::string & old_script,
    const std::string & old_scdt)
    : type(type)
    , merger(&merger)
    , script_name(script_name)
    , file_name(file_name)
    , old_script(old_script)
    , old_scdt(old_scdt)
    , new_scdt(old_scdt)
{
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
		line_lc = line;
		new_line = line;
		new_text.erase();
		pos = 0;
		keyword_pos = 0;
		keyword.erase();
		error = false;

		transform(line_lc.begin(), line_lc.end(), line_lc.begin(), ::tolower);

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
	auto is_word_char = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };

	size_t search_from = 0;
	while (true)
	{
		pos = line_lc.find(keyword, search_from);
		if (pos == std::string::npos)
			return;

		if (pos > 0 && is_word_char(line_lc[pos - 1]))
		{
			search_from = pos + 1;
			continue;
		}
		if (pos + keyword.size() < line_lc.size() && is_word_char(line_lc[pos + keyword.size()]))
		{
			search_from = pos + 1;
			continue;
		}
		break;
	}

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
	tools_t::add_log(file_name + "\r\n", true);
	tools_t::add_log(script_name + "\r\n", true);
	tools_t::add_log("<<< " + line + "\r\n", true);

	old_text = line.substr(pos);

	tools_t::add_log("1: " + old_text + "\r\n", true);
}

void script_parser_t::extract_text(const int pos_in_expression)
{
	std::regex r1("([\\w\\.\\-\\xD1]+|\".*?\")", std::regex::optimize);
	std::sregex_iterator next(old_text.begin(), old_text.end(), r1);
	std::sregex_iterator end;
	std::smatch found;

	int ctr = -1;
	while (next != end && ctr != pos_in_expression)
	{
		found = *next;
		next++;
		ctr++;
	}

	old_text = found[1].str();
	pos += found.position(1);

	tools_t::add_log("2: " + old_text + "\r\n", true);
}

void script_parser_t::remove_quotes()
{
	std::regex r("\"(.*?)\"", std::regex::optimize);
	std::smatch found;
	std::regex_search(old_text, found, r);
	if (!found.empty())
	{
		old_text = found[1].str();
		pos += 1;
	}

	tools_t::add_log("3: " + old_text + "\r\n", true);
}

void script_parser_t::find_new_text(const tools_t::rec_type_t text_type)
{
	new_text = old_text;
	auto * search = merger->get_dict().at(text_type).find(old_text);
	if (search)
	{
		new_text = search->new_text;
	}
	else
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

	if (new_scdt.empty())
	{
		tools_t::add_log("[error] SCDT is empty\r\n", true);
		error = true;
		return;
	}

	pos_c = new_scdt.find(old_text, pos_c);
	if (pos_c == std::string::npos)
	{
		tools_t::add_log("[error] not found in SCDT\r\n", true);
		error = true;
		return;
	}

	size_t old_size = tools_t::convert_string_byte_array_to_uint(new_scdt.substr(pos_c - 1, 1));

	/* wtf! Sometimes old text can be null terminated */
	while (old_size != old_text.size() && old_size != old_text.size() + 1)
	{
		tools_t::add_log(
		    "[warn] " + std::to_string(old_size) + " != " + std::to_string(old_text.size()) + " " + old_text +
		        " false positive in " + script_name + "\r\n",
		    true);
		error = true;

		pos_c += old_text.size();
		pos_c = new_scdt.find(old_text, pos_c);

		if (pos_c == std::string::npos)
		{
			tools_t::add_log("[error] not found in SCDT\r\n", true);
			error = true;
			return;
		}

		old_size = tools_t::convert_string_byte_array_to_uint(new_scdt.substr(pos_c - 1, 1));
	}

	pos_c -= 1;
	new_scdt.erase(pos_c, 1);
	new_scdt.insert(pos_c, tools_t::convert_uint_to_string_byte_array(new_text.size()).substr(0, 1));
	pos_c += 1;
	new_scdt.erase(pos_c, old_text.size());
	new_scdt.insert(pos_c, new_text);

	if (is_getpccell)
	{
		/* additional getpccell size byte determines
           how many bytes from that byte to the end of expression
		 */
		size_t end_of_expr;
		size_t expr_size;

		if (new_scdt.substr(pos_c + new_text.size(), 1) != " ")
		{
			/* if expression ends exactly when inner text ends */
			end_of_expr = pos_c + new_text.size();
			pos_c = new_scdt.rfind('X', pos_c) - 2;
			expr_size = end_of_expr - pos_c;
		}
		else
		{
			/* if expression ends with equals or inequal signs */
			end_of_expr = pos_c + new_text.size();
			while (end_of_expr < new_scdt.size() && new_scdt[end_of_expr] != '\0')
				end_of_expr++;
			pos_c = new_scdt.rfind('X', pos_c) - 2;
			expr_size = end_of_expr - pos_c - 1;
		}

		new_scdt.erase(pos_c, 1);
		new_scdt.insert(pos_c, tools_t::convert_uint_to_string_byte_array(expr_size).substr(0, 1));
		pos_c += expr_size;
	}
	else
	{
		pos_c += new_text.size();
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
	tools_t::add_log(file_name + "\r\n", true);
	tools_t::add_log(script_name + "\r\n", true);
	tools_t::add_log("<<< " + line + "\r\n", true);

	auto * search = merger->get_dict().at(type).find(script_name + "^" + line);
	if (search)
	{
		if (line != search->new_text.substr(script_name.size() + 1))
		{
			new_line = search->new_text.substr(script_name.size() + 1);
		}
	}

	tools_t::add_log(">>> " + new_line + "\r\n", true);
}

void script_parser_t::convert_message_in_compiled()
{
	if (type != tools_t::rec_type_t::sctx)
		return;

	if (new_scdt.empty())
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

	for (size_t i = 0; i < splitted_line.size(); i++)
	{
		replace_vertical_lines_by_new_line(splitted_line[i]);
		replace_vertical_lines_by_new_line(splitted_new_line[i]);

		pos_c = new_scdt.find(splitted_line[i], pos_c);
		if (pos_c == std::string::npos)
		{
			tools_t::add_log("[error] message not found in SCDT\r\n", true);
			error = true;
			return;
		}

		if (splitted_line[i] == splitted_new_line[i])
		{
			pos_c += splitted_line[i].size();
			continue;
		}

		if (splitted_line[i] == " " || splitted_line[i] == "\t")
		{
			tools_t::add_log("[error] message is one whitespace character\r\n", true);
			error = true;
			return;
		}

		if (i == 0)
		{
			pos_c -= 2;
			new_scdt.erase(pos_c, 2);
			new_scdt.insert(
			    pos_c, tools_t::convert_uint_to_string_byte_array(splitted_new_line[i].size()).substr(0, 2));
			pos_c += 2;
			new_scdt.erase(pos_c, splitted_line[i].size());
			new_scdt.insert(pos_c, splitted_new_line[i]);
			pos_c += splitted_new_line[i].size();
		}
		else
		{
			pos_c -= 1;
			new_scdt.erase(pos_c, 1);
			new_scdt.insert(
			    pos_c, tools_t::convert_uint_to_string_byte_array(splitted_new_line[i].size() + 1).substr(0, 1));
			pos_c += 1;
			new_scdt.erase(pos_c, splitted_line[i].size());
			new_scdt.insert(pos_c, splitted_new_line[i]);
			pos_c += splitted_new_line[i].size();
		}
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
	/* check if last 2 chars are newline and strip them if necessary */
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
		tools_t::add_log("\r\n----------------------------------------------------------\r\n", true);
		tools_t::add_log(tools_t::replace_non_readable_chars_with_dot(new_scdt), true);
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
