#pragma once

#include "../io/esm_reader.hpp"
#include "../utility/tools.hpp"
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class translation_engine_t;

class cell_matcher_t
{
public:
	struct counters_t
	{
		int created = 0;
		int missing = 0;
		int doubled = 0;
		int identical = 0;
	};

	using fingerprint_index_t = std::unordered_map<std::string, std::set<size_t>>;
	using determine_status_fn = std::function<status_t(const std::string &, const std::string &)>;

	cell_matcher_t(
	    esm_reader_t & esm_native,
	    esm_reader_t & esm_foreign,
	    translation_engine_t * translation_engine,
	    tools_t::dict_t & output_dict,
	    determine_status_fn determine_status);

	void match_exterior_cells();
	void match_interior_cells();
	void match_default_cell_name();
	void match_region_names();

	const counters_t & get_counters() const;

	static bool is_interior_cell(const std::string & data_content);
	static std::string make_exterior_coord_key(const std::string & data_content);
	static std::string make_cell_key_text(const std::string & fingerprint);
	static std::string make_cell_fingerprint(esm_reader_t & esm_src);

private:
	fingerprint_index_t build_cell_fingerprint_index(esm_reader_t & esm_src);
	void match_interior_cells_heuristic(
	    std::vector<std::pair<size_t, std::string>> & missing_cells,
	    const std::set<size_t> & matched_native_records);
	void add_missing_cells(
	    const std::vector<std::pair<size_t, std::string>> & missing_cells,
	    const std::string & native_candidates_str = {});
	void insert_entry(
	    const std::string & key_text,
	    const std::string & old_text,
	    const std::string & new_text,
	    status_t status);

	esm_reader_t & m_esm_native;
	esm_reader_t & m_esm_foreign;
	translation_engine_t * m_translation_engine;
	tools_t::dict_t & m_output_dict;
	determine_status_fn m_determine_status;
	counters_t m_counters;
	std::string m_native_candidates_str;
};
