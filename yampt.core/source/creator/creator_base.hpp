#pragma once

#include "creator_context.hpp"

#include <set>
#include <unordered_map>
#include <vector>

class creator_base_t
{
public:
	explicit creator_base_t(creator_context_t & context);
	void run();

private:
	void make_gmst();
	void make_fnam();
	void make_desc();
	void make_text();
	void make_rnam();
	void make_indx();
	void make_info();
	void make_sctx();
	void make_bnam();
	void make_dial();
	void make_cell();

	void insert_entry_base(const std::string & key_text, const std::string & old_text,
	    const std::string & new_text, rec_type_t type, status_t status);

	void build_sctx_schd_index(std::unordered_map<std::string, size_t> & schd_index);
	void match_sctx_messages(const std::string & script_name,
	    const std::vector<std::string> & native_messages,
	    const std::unordered_map<std::string, size_t> & schd_index);
	void match_bnam_native_infos(const std::string & info_key,
	    const std::vector<std::string> & native_messages);
	void collect_bnam_missing_topics(const std::set<std::string> & matched_foreign_topics);

	creator_context_t & m_ctx;
};
