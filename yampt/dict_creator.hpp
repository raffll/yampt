#pragma once

#include "includes.hpp"
#include "tools.hpp"
#include "esm_reader.hpp"

class translation_engine_t;

class dict_creator_t
{
public:
	enum class mode_t
	{
		single,
		base,
		base_ordered,
		single_with_base
	};

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

	static bool differs_only_in_numbers_or_punct(const std::string & a, const std::string & b);
	static std::string adapt_translation(
	    const std::string & source,
	    const std::string & matched_source,
	    const std::string & matched_translation);
	static bool is_interior_cell(const std::string & data_content);
	static std::string make_exterior_coord_key(const std::string & data_content);
	static std::string make_cell_key_text(const std::string & fingerprint);

private:
	void make_dict_single();
	void make_dict_base();

	void make_dict_single_gmst();
	void make_dict_single_fnam();
	void make_dict_single_desc();
	void make_dict_single_text();
	void make_dict_single_rnam();
	void make_dict_single_indx();
	void make_dict_single_info();
	void make_dict_single_sctx();
	void make_dict_single_bnam();
	void make_dict_single_dial();
	void make_dict_single_cell();

	void make_dict_base_gmst();
	void make_dict_base_fnam();
	void make_dict_base_desc();
	void make_dict_base_text();
	void make_dict_base_rnam();
	void make_dict_base_indx();
	void make_dict_base_info();
	void make_dict_base_sctx();
	void make_dict_base_bnam();
	void make_dict_base_dial();
	void make_dict_base_cell();

	void make_dict_base_ordered();
	void build_dial_map_ordered();

	void process_gmst_ordered(size_t i);
	void process_fnam_ordered(size_t i);
	void process_desc_ordered(size_t i);
	void process_text_ordered(size_t i);
	void process_rnam_ordered(size_t i);
	void process_indx_ordered(size_t i);
	void process_dial_ordered(size_t i, std::string & dial_type, std::string & dial_foreign_name);
	void process_info_ordered(size_t i, const std::string & dial_type, const std::string & dial_foreign_name);
	void process_sctx_ordered(size_t i);
	void process_bnam_ordered(
	    size_t i,
	    const std::string & dial_type,
	    const std::string & dial_foreign_name,
	    const std::string & info_inam);
	void process_cell_ordered(size_t i);
	void process_cell_default_ordered();
	void process_cell_region_ordered();

	void build_gmst_index();
	void build_fnam_index();
	void build_desc_index();
	void build_text_index();
	void build_rnam_index();
	void build_indx_index();
	void build_npc_index();
	void build_info_index();
	void build_text_match_index();
	void reset_counters();
	void insert_entry_single(
	    const std::string & key_text,
	    const std::string & old_text,
	    const std::string & new_text,
	    tools_t::rec_type_t type);
	void insert_entry_base(
	    const std::string & key_text,
	    const std::string & old_text,
	    const std::string & new_text,
	    tools_t::rec_type_t type,
	    const char * status);
	void insert_entry_single_with_base(
	    const std::string & key_text,
	    const std::string & old_text,
	    const std::string & new_text,
	    tools_t::rec_type_t type);

	void insert_as_untranslated(const std::string & key_text, const std::string & old_text, tools_t::rec_type_t type);
	void insert_with_status(
	    const std::string & key_text,
	    const std::string & old_text,
	    const std::string & new_text,
	    tools_t::rec_type_t type,
	    const char * status);
	void insert_duplicate(
	    const std::string & key_text,
	    const std::string & old_text,
	    const std::string & new_text,
	    tools_t::rec_type_t type,
	    const char * status);
	void insert_via_text_match(const std::string & key_text, const std::string & old_text, tools_t::rec_type_t type);

	std::vector<std::string> make_script_messages(const std::string & script_text);

	void make_dict_cell_exterior();
	void make_dict_cell_interior();
	void make_dict_cell_default();
	void make_dict_cell_region();
	void make_dict_cell_add_missing(
	    const std::vector<std::pair<size_t, std::string>> & missing_cells,
	    const std::string & native_candidates_str = {});

	using fingerprint_index_t = std::unordered_map<std::string, std::set<size_t>>;
	fingerprint_index_t build_cell_fingerprint_index(esm_reader_t & esm_src);
	fingerprint_index_t build_dial_inam_index(esm_reader_t & esm_src);
	static std::string make_cell_fingerprint(esm_reader_t & esm_src);
	void make_dict_cell_interior_heuristic(
	    std::vector<std::pair<size_t, std::string>> & missing_cells,
	    const std::set<size_t> & matched_native_records);

	esm_reader_t esm;
	esm_reader_t esm_ext;
	esm_reader_t & esm_ref;
	const tools_t::dict_t * base_dict = nullptr;
	tools_t::dict_t dict;
	mode_t mode = mode_t::single;
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
	std::unordered_map<std::string, std::string> dial_native_to_foreign;
	std::string cell_native_candidates_str_;
	std::unordered_map<std::string, const tools_t::record_entry_t *> text_match_index_;
	std::unordered_map<std::string, std::string> text_match_conflicts_;
	std::unordered_map<std::string, std::string> text_match_first_;
};
