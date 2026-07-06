#pragma once

#include "creator_context.hpp"

#include <string>
#include <vector>

namespace creator_helpers {

void insert_entry_single(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, const std::string & new_text, rec_type_t type);

void insert_entry_base(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, const std::string & new_text, rec_type_t type, status_t status);

void insert_entry_single_with_base(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, const std::string & new_text, rec_type_t type);

void insert_as_untranslated(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, rec_type_t type);

void insert_with_status(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, const std::string & new_text, rec_type_t type, status_t status);

void insert_duplicate(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, const std::string & new_text, rec_type_t type, status_t status);

void insert_via_text_match(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, rec_type_t type);

void insert_changed_entry(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, const record_entry_t & base_entry, rec_type_t type);

void insert_unapproved_changed(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, const record_entry_t & base_entry, rec_type_t type);

void insert_adapted_entry(creator_context_t & ctx, const std::string & key_text,
    const std::string & old_text, const record_entry_t & base_entry, rec_type_t type);

std::vector<std::string> make_script_messages(const std::string & script_text);

void build_npc_index(creator_context_t & ctx);
void build_text_match_index(creator_context_t & ctx);
void build_gmst_index(creator_context_t & ctx);
void build_fnam_index(creator_context_t & ctx);
void build_desc_index(creator_context_t & ctx);
void build_text_index(creator_context_t & ctx);
void build_rnam_index(creator_context_t & ctx);
void build_indx_index(creator_context_t & ctx);
void build_info_index(creator_context_t & ctx);

void load_english_dict(creator_context_t & ctx);
status_t determine_status(const creator_context_t & ctx,
    const std::string & old_text, const std::string & new_text);
bool is_proper_noun(const creator_context_t & ctx, const std::string & text);

bool differs_only_in_numbers_or_punct(const std::string & a, const std::string & b);
std::string adapt_translation(const std::string & source,
    const std::string & matched_source, const std::string & matched_translation);

void enrich_info_speaker(creator_context_t & ctx, const std::string & key_text, size_t record_index);

} // namespace creator_helpers
