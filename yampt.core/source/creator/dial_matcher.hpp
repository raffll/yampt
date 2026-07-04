#pragma once

#include "../io/esm_reader.hpp"
#include "../utility/tools.hpp"
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class translation_engine_t;

class dial_matcher_t
{
public:
	struct counters_t
	{
		int created = 0;
		int missing = 0;
		int doubled = 0;
	};

	using fingerprint_index_t = std::unordered_map<std::string, std::set<size_t>>;
	using determine_status_fn = std::function<status_t(const std::string &, const std::string &)>;

	dial_matcher_t(
	    esm_reader_t & esm_native,
	    esm_reader_t & esm_foreign,
	    translation_engine_t * translation_engine,
	    tools_t::dict_t & output_dict,
	    determine_status_fn determine_status);

	fingerprint_index_t build_inam_index(esm_reader_t & esm_source);
	void match_topics();

	const std::unordered_map<std::string, std::string> & get_native_to_foreign() const;
	const counters_t & get_counters() const;

private:
	void match_by_inam(
	    const fingerprint_index_t & native_inam_index,
	    std::vector<std::pair<size_t, std::string>> & unmatched_foreign,
	    std::set<size_t> & matched_native_records);
	void match_by_translation(
	    std::vector<std::pair<size_t, std::string>> & unmatched_foreign,
	    const std::set<size_t> & matched_native_records);
	void report_unmatched(
	    const std::vector<std::pair<size_t, std::string>> & unmatched_foreign,
	    const std::set<size_t> & matched_native_records);
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
	std::unordered_map<std::string, std::string> m_native_to_foreign;
};
