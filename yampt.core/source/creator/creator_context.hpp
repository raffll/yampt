#pragma once

#include "../io/esm_reader.hpp"
#include "../utility/domain_types.hpp"
#include "text_match_index.hpp"

#include <memory>
#include <set>
#include <unordered_map>

class Hunspell;
class translation_engine_t;

enum class base_mode_t
{
	full,
	partial
};

struct creator_context_t
{
	esm_reader_t esm;
	esm_reader_t esm_ext;
	esm_reader_t & esm_ref;
	const dict_t * base_dict = nullptr;
	dict_t dict;
	translation_engine_t * translation_engine = nullptr;
	base_mode_t base_mode = base_mode_t::full;
	std::string dictionary_aff_path;
	std::unique_ptr<Hunspell> english_dict;

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
	text_match_index_t text_match_index;

	creator_context_t(const std::string & path)
	    : esm(path)
	    , esm_ref(esm)
	{
		dict = domain_types_t::initialize_dict();
	}

	creator_context_t(const std::string & native_path, const std::string & foreign_path)
	    : esm(foreign_path)
	    , esm_ext(native_path)
	    , esm_ref(esm_ext)
	{
		dict = domain_types_t::initialize_dict();
	}

	void reset_counters()
	{
		counter_created = 0;
		counter_missing = 0;
		counter_doubled = 0;
		counter_identical = 0;
		counter_all = 0;
	}

	bool is_loaded() const
	{
		return esm.is_loaded();
	}

	bool is_both_loaded() const
	{
		return esm.is_loaded() && esm_ext.is_loaded();
	}
};
