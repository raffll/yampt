#pragma once

#include "creator_context.hpp"
#include <string>

class creator_ordered_t
{
public:
	explicit creator_ordered_t(creator_context_t & context);
	void run();

private:
	void build_dial_map();
	void process_gmst(size_t index);
	void process_fnam(size_t index);
	void process_desc(size_t index);
	void process_text(size_t index);
	void process_rnam(size_t index);
	void process_indx(size_t index);
	void process_dial(size_t index, std::string & dial_type, std::string & dial_foreign_name);
	void process_info(size_t index, const std::string & dial_type, const std::string & dial_foreign_name);
	void attach_speaker_metadata(const std::string & key_text, size_t record_index);
	void process_sctx(size_t index);
	void process_bnam(
	    size_t index,
	    const std::string & dial_type,
	    const std::string & dial_foreign_name,
	    const std::string & info_inam);
	void process_cell(size_t index);
	void process_cell_default();
	void process_cell_region();

	void insert_entry_base(
	    const std::string & key_text,
	    const std::string & old_text,
	    const std::string & new_text,
	    rec_type_t type,
	    status_t status);

	creator_context_t & m_ctx;
};
