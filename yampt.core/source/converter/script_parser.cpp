#include "script_parser.hpp"
#include "../utility/app_logger.hpp"
#include "../utility/string_utils.hpp"
#include <regex>

namespace {

size_t find_whole_word(const std::string & text_line, const std::string & keyword)
{
	auto is_word_char = [](char value) { return std::isalnum(static_cast<unsigned char>(value)) || value == '_'; };

	size_t search_from = 0;
	while (true)
	{
		const auto found_pos = text_line.find(m_keyword, search_from);
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

bool is_numeric_token(const std::string & token)
{
	if (token.empty())
		return false;

	size_t start = 0;
	if (token[0] == '-' || token[0] == '+')
		start = 1;

	if (start >= token.size())
		return false;

	bool has_digit = false;
	for (size_t index = start; index < token.size(); ++index)
	{
		if (std::isdigit(static_cast<unsigned char>(token[index])))
		{
			has_digit = true;
			continue;
		}

		if (token[index] == '.')
			continue;

		return false;
	}

	return has_digit;
}

std::string extract_cell_name_to_eol(const std::string & text_after_keyword)
{
	auto cell_name = text_after_keyword;

	const auto comment_pos = cell_name.find(';');
	if (comment_pos != std::string::npos)
		cell_name = cell_name.substr(0, comment_pos);

	const auto cr_pos = cell_name.find('\r');
	if (cr_pos != std::string::npos)
		cell_name = cell_name.substr(0, cr_pos);

	const auto lf_pos = cell_name.find('\n');
	if (lf_pos != std::string::npos)
		cell_name = cell_name.substr(0, lf_pos);

	while (!cell_name.empty() && (cell_name.back() == ' ' || cell_name.back() == '\t'))
		cell_name.pop_back();

	return cell_name;
}

std::string extract_cell_name_after_numerics(const std::string & text_after_keyword)
{
	size_t scan_pos = 0;
	const auto text_size = text_after_keyword.size();

	while (scan_pos < text_size)
	{
		while (scan_pos < text_size && (text_after_keyword[scan_pos] == ' ' || text_after_keyword[scan_pos] == '\t' ||
		                                text_after_keyword[scan_pos] == ','))
			++scan_pos;

		if (scan_pos >= text_size)
			break;

		const auto token_start = scan_pos;
		while (scan_pos < text_size && text_after_keyword[scan_pos] != ' ' && text_after_keyword[scan_pos] != '\t' &&
		       text_after_keyword[scan_pos] != ',')
			++scan_pos;

		const auto token = text_after_keyword.substr(token_start, scan_pos - token_start);
		if (!is_numeric_token(token))
		{
			const auto remainder = text_after_keyword.substr(token_start);
			return extract_cell_name_to_eol(remainder);
		}
	}

	return {};
}

} // namespace

script_parser_t::script_parser_t(
    const rec_type_t type,
    const dict_merger_t & merger,
    const std::string & record_key,
    const std::string & source_path,
    const std::string & old_script,
    const std::string & old_scdt)
    : m_type(type)
    , m_merger(&merger)
    , m_record_key(record_key)
    , m_source_path(source_path)
    , m_old_script(old_script)
    , m_old_scdt(old_scdt)
{
	if (m_type == rec_type_t::sctx && !m_old_scdt.empty())
		m_patcher = std::make_unique<scdt_patcher_t>(m_old_scdt);

	convert_script();
	trim_last_new_line_chars();
}

void script_parser_t::convert_script()
{
	std::istringstream ss(m_old_script);
	bool is_end = false;

	while (std::getline(ss, m_line))
	{
		m_is_done = false;
		m_line = string_utils::trim_cr(m_line);
		m_line_lc = string_utils::to_lower(m_line);
		m_new_line = m_line;
		m_new_text.erase();
		m_pos = 0;
		m_keyword_pos = 0;
		m_keyword.erase();
		m_error = false;

		if (m_line_lc == "end" || (m_line_lc.size() > 3 && m_line_lc.substr(0, 4) == "end "))
			is_end = true;

		if (!is_end)
			convert_current_line();

		if (m_error)
			dump_error();

		m_new_script += m_new_line + "\r\n";
	}
}

void script_parser_t::convert_current_line()
{
	try
	{
		if (!m_is_done)
			convert_line("addtopic", 0, rec_type_t::dial);

		if (!m_is_done)
			convert_line_unquoted("showmap", rec_type_t::cell);
		if (!m_is_done)
			convert_line("showmap", 0, rec_type_t::cell);

		if (!m_is_done)
			convert_line_unquoted("centeroncell", rec_type_t::cell);
		if (!m_is_done)
			convert_line("centeroncell", 0, rec_type_t::cell);

		if (!m_is_done)
			convert_line("getpccell", 0, rec_type_t::cell);

		if (!m_is_done)
			convert_line_unquoted("aifollowcell", rec_type_t::cell);
		if (!m_is_done)
			convert_line("aifollowcell", 1, rec_type_t::cell);

		if (!m_is_done)
			convert_line_unquoted("aiescortcell", rec_type_t::cell);
		if (!m_is_done)
			convert_line("aiescortcell", 1, rec_type_t::cell);

		if (!m_is_done)
			convert_line_unquoted("placeitemcell", rec_type_t::cell);
		if (!m_is_done)
			convert_line("placeitemcell", 1, rec_type_t::cell);

		if (!m_is_done)
			convert_line_unquoted("positioncell", rec_type_t::cell);
		if (!m_is_done)
			convert_line("positioncell", 4, rec_type_t::cell);

		if (!m_is_done)
			convert_line();
	}
	catch (...)
	{
		app_logger_t::add_log("[error] unknown error in script parser\r\n");
		app_logger_t::add_log("line: " + m_line + "\r\n");
		m_error = true;
	}
}

void script_parser_t::convert_line(const std::string & keyword, const int pos_in_expression, const rec_type_t text_type)
{
	m_pos = find_whole_word(m_line_lc, keyword);
	if (m_pos == std::string::npos)
		return;

	if (m_line.size() == keyword.size())
		return;

	if (m_line.rfind(";", m_pos) != std::string::npos)
		return;

	m_pos = m_line.find_first_of(" \t,\"", m_pos);
	m_pos = m_line.find_first_not_of(" \t,", m_pos);
	if (m_pos == std::string::npos)
		return;

	trim_line();
	extract_text(pos_in_expression);
	remove_quotes();
	find_new_text(text_type);
	insert_new_text();

	const auto is_getpccell = keyword == "getpccell" ? true : false;
	convert_text_in_compiled(is_getpccell);

	m_is_done = true;
}

void script_parser_t::convert_line_unquoted(const std::string & keyword, const rec_type_t text_type)
{
	m_pos = find_whole_word(m_line_lc, keyword);
	if (m_pos == std::string::npos)
		return;

	if (m_line.size() == keyword.size())
		return;

	if (m_line.rfind(";", m_pos) != std::string::npos)
		return;

	const auto after_keyword = m_line.find_first_of(" \t", m_pos);
	if (after_keyword == std::string::npos)
		return;

	const auto content_start = m_line.find_first_not_of(" \t", after_keyword);
	if (content_start == std::string::npos)
		return;

	const auto comment_pos = m_line.find(';', content_start);
	const auto search_end = (comment_pos != std::string::npos) ? comment_pos : m_line.size();

	if (m_line.find('"', content_start) < search_end)
		return;

	const auto text_after_keyword = m_line.substr(content_start);

	const auto is_showmap_family = (keyword == "showmap" || keyword == "centeroncell");
	const auto cell_name = is_showmap_family ? extract_cell_name_to_eol(text_after_keyword)
	                                         : extract_cell_name_after_numerics(text_after_keyword);

	if (cell_name.empty())
		return;

	m_pos = content_start + (m_line.substr(content_start).find(cell_name));
	m_old_text = cell_name;

	app_logger_t::add_log("\r\n\r\n" + m_source_path + "\r\n" + m_record_key + "\r\n", true);
	app_logger_t::add_log("<<< " + m_line + "\r\n", true);
	app_logger_t::add_log("unquoted: " + m_old_text + "\r\n", true);

	find_new_text(text_type);
	insert_new_text();
	convert_text_in_compiled(false);

	m_is_done = true;
}

void script_parser_t::trim_line()
{
	app_logger_t::add_log("\r\n\r\n", true);
	app_logger_t::add_log(m_source_path + "\r\n", true);
	app_logger_t::add_log(m_record_key + "\r\n", true);
	app_logger_t::add_log("<<< " + m_line + "\r\n", true);

	m_old_text = m_line.substr(m_pos);

	app_logger_t::add_log("1: " + m_old_text + "\r\n", true);
}

void script_parser_t::extract_text(const int pos_in_expression)
{
	const auto result = extract_token_at(m_old_text, pos_in_expression);
	if (!result.found)
	{
		app_logger_t::add_log(
		    "[warning] extract_text: expected parameter at position " + std::to_string(pos_in_expression) +
		        " in: " + m_old_text + "\r\n",
		    true);
		m_error = true;
		return;
	}

	m_old_text = result.value;
	m_pos += result.offset;

	app_logger_t::add_log("2: " + m_old_text + "\r\n", true);
}

void script_parser_t::remove_quotes()
{
	const auto stripped = strip_quotes(m_old_text);
	if (stripped != m_old_text)
	{
		m_old_text = stripped;
		m_pos += 1;
	}

	app_logger_t::add_log("3: " + m_old_text + "\r\n", true);
}

void script_parser_t::find_new_text(const rec_type_t text_type)
{
	m_new_text = m_old_text;

	const auto * search = m_merger->get_dict().at(text_type).find(m_old_text);

	if (search && is_approved_status(search->status))
	{
		m_new_text = search->new_text;
	}
	else if (text_type != rec_type_t::cell)
	{
		for (const auto & elem : m_merger->get_dict().at(text_type).records)
		{
			if (!is_approved_status(elem.status))
				continue;

			if (string_utils::case_insensitive_equal(m_old_text, elem.key_text))
			{
				m_new_text = elem.new_text;
				break;
			}
		}
	}
	else
	{
		for (const auto & elem : m_merger->get_dict().at(text_type).records)
		{
			if (!is_approved_status(elem.status))
				continue;

			if (string_utils::case_insensitive_equal(m_old_text, elem.old_text))
			{
				m_new_text = elem.new_text;
				break;
			}
		}
	}

	app_logger_t::add_log("4: " + m_new_text + "\r\n", true);
	if (m_new_text.size() < 2)
	{
		app_logger_t::add_log("[error] result is too short\r\n", true);
		m_error = true;
	}
}

void script_parser_t::insert_new_text()
{
	if (m_new_text == m_old_text)
	{
		m_pos += m_old_text.size();
		return;
	}

	m_new_line.erase(m_pos, m_old_text.size());
	m_new_line.insert(m_pos, m_new_text);

	app_logger_t::add_log(">>> " + m_new_line + "\r\n", true);
}

void script_parser_t::convert_text_in_compiled(const bool is_getpccell)
{
	if (m_new_text == m_old_text)
		return;

	if (m_type != rec_type_t::sctx)
		return;

	if (!m_patcher || m_patcher->is_empty())
	{
		app_logger_t::add_log("[error] SCDT is empty\r\n", true);
		m_error = true;
		return;
	}

	const auto result = m_patcher->apply_text_patch(m_old_text, m_new_text, is_getpccell);

	if (result.had_false_positive)
	{
		app_logger_t::add_log("[warning] false positive in " + m_record_key + " for: " + m_old_text + "\r\n", true);
	}

	if (!result.success)
	{
		app_logger_t::add_log("[error] not found in SCDT\r\n", true);
		m_error = true;
	}
}

void script_parser_t::convert_line()
{
	find_keyword();

	if (m_keyword_pos == std::string::npos)
		return;

	if (m_line.rfind(";", m_keyword_pos) != std::string::npos)
		return;

	if (m_line.find("\"", m_keyword_pos) == std::string::npos)
		return;

	find_new_message();
	convert_message_in_compiled();

	m_is_done = true;
}

void script_parser_t::find_keyword()
{
	std::map<size_t, std::string> keyword_pos_coll;
	for (const auto & kw : domain_types::script_keywords)
	{
		m_keyword_pos = find_whole_word(m_line_lc, kw);
		if (m_keyword_pos != std::string::npos)
			keyword_pos_coll.insert({ m_keyword_pos, kw });
	}

	if (keyword_pos_coll.empty())
	{
		m_keyword_pos = std::string::npos;
		m_keyword.clear();
		return;
	}

	m_keyword_pos = keyword_pos_coll.begin()->first;
	m_keyword = keyword_pos_coll.begin()->second;
}

void script_parser_t::find_new_message()
{
	app_logger_t::add_log("\r\n\r\n", true);
	app_logger_t::add_log(m_source_path + "\r\n", true);
	app_logger_t::add_log(m_record_key + "\r\n", true);
	app_logger_t::add_log("<<< " + m_line + "\r\n", true);

	auto * search = m_merger->get_dict().at(m_type).find(m_record_key + "^" + m_line);
	if (search && is_approved_status(search->status))
	{
		if (m_line != search->new_text)
		{
			m_new_line = search->new_text;
		}
	}

	app_logger_t::add_log(">>> " + m_new_line + "\r\n", true);
}

void script_parser_t::convert_message_in_compiled()
{
	if (m_type != rec_type_t::sctx)
		return;

	if (!m_patcher || m_patcher->is_empty())
	{
		app_logger_t::add_log("[error] SCDT is empty\r\n", true);
		m_error = true;
		return;
	}

	std::vector<std::string> splitted_line = split_line(m_line);
	std::vector<std::string> splitted_new_line = split_line(m_new_line);

	if (splitted_line.size() != splitted_new_line.size())
	{
		app_logger_t::add_log("[error] incompatible messages\r\n", true);
		m_error = true;
		return;
	}

	for (auto & segment : splitted_line)
		replace_vertical_lines_by_new_line(segment);

	for (auto & segment : splitted_new_line)
		replace_vertical_lines_by_new_line(segment);

	const auto result = m_patcher->apply_message_patch(splitted_line, splitted_new_line);
	if (!result.success)
	{
		app_logger_t::add_log("[error] message not found in SCDT\r\n", true);
		m_error = true;
	}
}

std::vector<std::string> script_parser_t::split_line(const std::string & cur_line) const
{
	std::string cur_line_tr = cur_line.substr(m_keyword_pos);
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

	if (m_keyword == "say" && splitted_line.size() > 0)
	{
		splitted_line.erase(splitted_line.begin());
	}

	return splitted_line;
}

void script_parser_t::trim_last_new_line_chars()
{
	if (m_new_script.size() < 2)
		return;

	size_t last_nl_pos = m_old_script.rfind("\r\n");
	if (last_nl_pos != m_old_script.size() - 2 || last_nl_pos == std::string::npos)
	{
		m_new_script.resize(m_new_script.size() - 2);
	}
}

void script_parser_t::dump_error()
{
	if (m_type == rec_type_t::sctx)
	{
		app_logger_t::add_log("----------------------------------------------------------\r\n", true);
		app_logger_t::add_log(string_utils::replace_non_printable_with_dot(m_old_scdt), true);
		app_logger_t::add_log(
		    "\r\n----------------------------------------------------------"
		    "\r\n",
		    true);
		if (m_patcher)
			app_logger_t::add_log(string_utils::replace_non_printable_with_dot(m_patcher->get_scdt()), true);
	}
	app_logger_t::add_log("\r\n----------------------------------------------------------\r\n", true);
	app_logger_t::add_log(m_old_script, true);
	app_logger_t::add_log("\r\n----------------------------------------------------------\r\n", true);
}

void script_parser_t::replace_vertical_lines_by_new_line(std::string & message)
{
	while (message.find("|") != std::string::npos)
	{
		message.replace(message.find("|"), 1, "\x0A");
	}
}
