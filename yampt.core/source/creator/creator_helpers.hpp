#pragma once

#include "creator_context.hpp"

#include <string>
#include <vector>

class creator_helpers_t
{
public:
	static void insert_entry_single(creator_context_t & ctx, const std::string & key_text,
	    const std::string & old_text, const std::string & new_text, rec_type_t type);

	static void insert_entry_base(creator_context_t & ctx, const std::string & key_text,
	    const std::string & old_text, const std::string & new_text, rec_type_t type, status_t status);

	static void insert_entry_single_with_base(creator_context_t & ctx, const std::string & key_text,
	    const std::string & old_text, const std::string & new_text, rec_type_t type);

	static void insert_as_untranslated(creator_context_t & ctx, const std::string & key_text,
	    const std::string & old_text, rec_type_t type);

	static void insert_with_status(creator_context_t & ctx, const std::string & key_text,
	    const std::string & old_text, const std::string & new_text, rec_type_t type, status_t status);

	static void insert_duplicate(creator_context_t & ctx, const std::string & key_text,
	    const std::string & old_text, const std::string & new_text, rec_type_t type, status_t status);

	static void insert_via_text_match(creator_context_t & ctx, const std::string & key_text,
	    const std::string & old_text, rec_type_t type);

	static void insert_changed_entry(creator_context_t & ctx, const std::string & key_text,
	    const std::string & old_text, const record_entry_t & base_entry, rec_type_t type);

	static std::vector<std::string> make_script_messages(const std::string & script_text);

	static void build_npc_index(creator_context_t & ctx);
	static void build_text_match_index(creator_context_t & ctx);
	static void build_gmst_index(creator_context_t & ctx);
	static void build_fnam_index(creator_context_t & ctx);
	static void build_desc_index(creator_context_t & ctx);
	static void build_text_index(creator_context_t & ctx);
	static void build_rnam_index(creator_context_t & ctx);
	static void build_indx_index(creator_context_t & ctx);
	static void build_info_index(creator_context_t & ctx);

	static void load_english_dict(creator_context_t & ctx);
	static status_t determine_status(const creator_context_t & ctx,
	    const std::string & old_text, const std::string & new_text);

	static bool differs_only_in_numbers_or_punct(const std::string & a, const std::string & b);
	static std::string adapt_translation(const std::string & source,
	    const std::string & matched_source, const std::string & matched_translation);

	static void enrich_info_speaker(creator_context_t & ctx, const std::string & key_text, size_t record_index);
};
