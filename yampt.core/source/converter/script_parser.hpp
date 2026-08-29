#pragma once

#include "../merger/dict_merger.hpp"
#include "../utility/domain_types.hpp"
#include "scdt_patcher.hpp"
#include <map>
#include <memory>
#include <string>

namespace script_token {

struct token_result_t
{
	std::string value;
	size_t offset = 0;
	bool found = false;
};

token_result_t extract_token_at(const std::string & text_input, int position);

} // namespace script_token

class script_parser_t
{
public:
	void convert_script();

	std::string get_new_script()
	{
		return m_new_script;
	}

	std::string get_new_scdt()
	{
		if (m_patcher)
			return m_patcher->get_scdt();

		return {};
	}

	script_parser_t(
	    const rec_type_t type,
	    const dict_merger_t & merger,
	    const std::string & record_key,
	    const std::string & source_path,
	    const std::string & old_script,
	    const std::string & old_scdt = "");

private:
	void convert_current_line();
	void convert_line(const std::string & keyword, const int pos_in_expression, const rec_type_t text_type);
	void convert_line_unquoted(const std::string & keyword, const rec_type_t text_type);
	void trim_line();
	void extract_text(const int pos_in_expression);
	void remove_quotes();
	void find_new_text(const rec_type_t text_type);
	void insert_new_text();
	void convert_text_in_compiled(const bool is_getpccell);
	void convert_line();
	void find_keyword();
	void find_new_message();
	void convert_message_in_compiled();
	std::vector<std::string> split_line(const std::string & cur_line) const;
	void trim_last_new_line_chars();
	void dump_error();
	void replace_vertical_lines_by_new_line(std::string & message);

	const rec_type_t m_type;
	const dict_merger_t * m_merger;
	const std::string m_record_key;
	const std::string m_source_path;
	const std::string m_old_script;
	const std::string m_old_scdt;

	std::string m_new_script;
	std::unique_ptr<scdt_patcher_t> m_patcher;

	bool m_is_done = false;
	std::string m_line;
	std::string m_line_lc;
	std::string m_old_text;
	std::string m_new_line;
	std::string m_new_text;
	size_t m_pos = 0;
	size_t m_keyword_pos = 0;
	std::string m_keyword;
	bool m_error = false;
};
