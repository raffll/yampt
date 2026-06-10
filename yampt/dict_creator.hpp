#pragma once

#include "includes.hpp"
#include "tools.hpp"
#include "esm_reader.hpp"

class translation_engine_t;

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

	dict_creator_t(
	    const std::string & path,
	    const std::string & path_ext,
	    translation_engine_t * translation_engine = nullptr);

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
	void build_text_match_index();
	void reset_counters();
	std::string translate_dialog_topic(std::string to_translate);
	void insert_entry(
	    const std::string & key_text,
	    const std::string & old_text,
	    const std::string & new_text,
	    tools_t::rec_type_t type);
	std::vector<std::string> make_script_messages(const std::string & script_text);
	void print_log_line(const tools_t::rec_type_t type);

	static bool differs_only_in_numbers_or_punct(const std::string & a, const std::string & b);
	static std::string adapt_translation(
	    const std::string & source,
	    const std::string & matched_source,
	    const std::string & matched_translation);

	void make_dict_cell_exterior();
	void make_dict_cell_interior();
	void make_dict_cell_default();
	void make_dict_cell_regn();
	void make_dict_cell_add_missing(const std::vector<std::pair<size_t, std::string>> & missing_cells);

	void make_dict_gmst();
	void make_dict_fnam();
	void make_dict_desc();
	void make_dict_text();
	void make_dict_rnam();
	void make_dict_indx();
	void make_dict_dial();
	void make_dict_info();
	void make_dict_script(const ids & ids);

	static bool is_interior_cell(const std::string & data_content);
	static std::string make_exterior_coord_key(const std::string & data_content);
	using cell_index_t = std::unordered_map<std::string, size_t>;
	cell_index_t build_cell_index(esm_reader_t & esm_src, std::set<std::string> & duplicates);
	static std::string make_cell_fingerprint(esm_reader_t & esm_src);
	static std::string make_cell_key_text(const std::string & fingerprint);
	void make_dict_cell_interior_heuristic(
	    std::vector<std::pair<size_t, std::string>> & missing_cells,
	    const std::set<size_t> & matched_native_records);

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
	translation_engine_t * translation_engine_ = nullptr;

	int counter_created = 0;
	int counter_missing = 0;
	int counter_doubled = 0;
	int counter_identical = 0;
	int counter_all = 0;

	std::unordered_map<std::string, size_t> gmst_index;
	std::unordered_map<std::string, size_t> fnam_index;
	std::unordered_map<std::string, size_t> desc_index;
	std::unordered_map<std::string, size_t> text_index;
	std::unordered_map<std::string, std::string> rnam_index;
	std::unordered_map<std::string, size_t> indx_index;
	std::unordered_map<std::string, size_t> npc_index;
	std::unordered_map<std::string, size_t> info_index;
	std::unordered_map<std::string, std::vector<const tools_t::record_entry_t *>> text_match_index_;
};
