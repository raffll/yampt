#pragma once

#include "../merger/dict_merger.hpp"
#include "../utility/includes.hpp"
#include "../utility/domain_types.hpp"
#include "scdt_patcher.hpp"

class script_parser_t
{
public:
	void convert_script();

	std::string get_new_script()
	{
		return new_script;
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

	const rec_type_t type;
	const dict_merger_t * merger;
	const std::string record_key;
	const std::string source_path;
	const std::string old_script;
	const std::string old_scdt;

	std::string new_script;
	std::unique_ptr<scdt_patcher_t> m_patcher;

	bool is_done = false;
	std::string line;
	std::string line_lc;
	std::string old_text;
	std::string new_line;
	std::string new_text;
	size_t pos = 0;
	size_t keyword_pos = 0;
	std::string keyword;
	bool error = false;
};
