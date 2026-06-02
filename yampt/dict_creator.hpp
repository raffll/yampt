#pragma once

#include "includes.hpp"
#include "tools.hpp"
#include "esm_reader.hpp"

class dict_creator_t
{
public:
	const auto & get_name()
	{
		return esm.get_name();
	}

	const auto & get_dict() const
	{
		return dict;
	}

	dict_creator_t(const std::string & plugin_path, const tools_t::dict_t * base_dict = nullptr);

	dict_creator_t(const std::string & path, const std::string & path_ext);

private:
	struct pattern_t
	{
		std::string str;
		size_t pos;
		bool missing;
	};

	using patterns_ext_t = std::vector<pattern_t>;
	using patterns = std::map<std::string, size_t>;

	struct ids
	{
		const std::string & rec_id;
		const std::string & key_id;
		const std::string & val_id;
		const tools_t::rec_type_t type;
	};

	void make_dict();
	void build_indexes();
	void reset_counters();
	std::string translate_dialog_topic(std::string to_translate);
	void insert_entry(
	    const std::string & key_text,
	    const std::string & old_text,
	    const std::string & new_text,
	    tools_t::rec_type_t type);
	std::vector<std::string> make_script_messages(const std::string & script_text);
	void print_log_line(const tools_t::rec_type_t type);

	void make_dict_cell();
	void make_dict_cell_default();
	void make_dict_cell_regn();
	void make_dict_gmst();
	void make_dict_fnam();
	void make_dict_desc();
	void make_dict_text();
	void make_dict_rnam();
	void make_dict_indx();
	void make_dict_dial();
	void make_dict_flag();
	void make_dict_info();
	void make_dict_script(const ids & ids);
	void make_dict_fnam_glossary();

	void make_dict_cell_unordered();
	void make_dict_cell_unordered_default();
	void make_dict_cell_unordered_regn();
	patterns_ext_t make_dict_cell_unordered_patterns_ext();
	patterns make_dict_cell_unordered_patterns();
	std::string make_dict_cell_unordered_pattern(esm_reader_t & esm_cur);
	void make_dict_cell_unordered_add_missing(const patterns_ext_t & patterns_ext);

	void make_dict_dial_unordered();
	patterns_ext_t make_dict_dial_unordered_patterns_ext();
	patterns make_dict_dial_unordered_patterns();
	std::string make_dict_dial_unordered_pattern(esm_reader_t & esm_cur, size_t i);
	void make_dict_dial_unordered_add_missing(const patterns_ext_t & patterns_ext);

	void make_dict_script_unordered(const ids & ids);
	patterns_ext_t make_dict_unordered_patterns_ext(const ids & ids);
	patterns make_dict_unordered_patterns(const ids & ids);

	esm_reader_t esm;
	esm_reader_t esm_ext;
	esm_reader_t & esm_ref;
	const tools_t::dict_t * base_dict = nullptr;
	tools_t::dict_t dict;
	bool is_make_mode = false;

	int counter_created = 0;
	int counter_missing = 0;
	int counter_doubled = 0;
	int counter_identical = 0;
	int counter_all = 0;

	std::unordered_map<std::string, size_t> gmst_index;
	std::unordered_map<std::string, size_t> fnam_index;
	std::unordered_map<std::string, size_t> desc_index;
	std::unordered_map<std::string, size_t> text_index;
	std::unordered_map<std::string, size_t> rnam_index;
	std::unordered_map<std::string, size_t> indx_index;
	std::unordered_map<std::string, size_t> flag_index;
	std::unordered_map<std::string, size_t> info_index;
};
