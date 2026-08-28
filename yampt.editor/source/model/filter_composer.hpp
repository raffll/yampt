#pragma once

#include "nav_tree_filter.hpp"

namespace filter_composer
{

nav_tree_filter_t::filter_state_t compose_filter(
	bool conflicts_only,
	const nav_tree_filter_t::filter_state_t & advanced,
	const nav_tree_filter_t::filter_state_t & search);

}
