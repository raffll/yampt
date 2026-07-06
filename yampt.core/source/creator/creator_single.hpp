#pragma once

#include "creator_context.hpp"

class creator_single_t
{
public:
	explicit creator_single_t(creator_context_t & context);
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

	creator_context_t & m_ctx;
	bool m_with_base;
};
