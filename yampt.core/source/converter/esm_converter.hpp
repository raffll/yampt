#pragma once

#include "../io/codepage.hpp"
#include "../io/esm_reader.hpp"
#include "../utility/includes.hpp"
#include "../utility/keyword_trie.hpp"
#include "../utility/tools.hpp"
#include "../merger/dict_merger.hpp"

class esm_converter_t
{
public:
	const auto & is_loaded()
	{
		return esm.is_loaded();
	}

	const auto & get_name()
	{
		return esm.get_name();
	}

	const auto & get_time()
	{
		return esm.get_time();
	}

	const auto & get_records()
	{
		return esm.get_records();
	}

	esm_converter_t(
	    const std::string & path,
	    const dict_merger_t & merger,
	    const bool add_hyperlinks,
	    const std::string & file_suffix,
	    const codepage_t encoding,
	    const bool create_header);

private:
	void convert_esm();
	void reset_counters();
	void convert_record_content(const std::string & new_text);
	void add_null_terminator_if_empty(std::string & new_text);
	bool make_new_text(const tools_t::entry_t & entry, std::string & new_text);
	bool is_identical(const std::string & old_text, const std::string & new_text);
	void print_log_line(const tools_t::rec_type_t type);
	void convert_mast();
	void make_header();
	void convert_cell();
	void convert_pgrd();
	void convert_anam();
	void convert_scvr();
	void convert_dnam();
	void convert_cndt();
	void convert_gmst();
	void convert_fnam();
	void convert_desc();
	void convert_text();
	void convert_rnam();
	void convert_indx();
	void convert_dial();
	void convert_info();
	void convert_bnam();
	void convert_scpt();
	void convert_gmdt();

	bool detect_encoding();
	bool detect_windows_1250_encoding(const std::string & text);

	void build_hyperlink_trie();
	std::string insert_hyperlink_markers(const std::string & text) const;

	esm_reader_t esm;
	const dict_merger_t & merger;

	bool add_hyperlinks;
	const std::string file_suffix;
	const bool create_header;

	keyword_trie_t m_hyperlink_trie;

	int counter_converted = 0;
	int counter_identical = 0;
	int counter_unchanged = 0;
	int counter_all = 0;
	int counter_added = 0;
};
