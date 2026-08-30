#include "filter_composer.hpp"

#include <scanner/conflict_enums.hpp>

namespace filter_composer
{

nav_tree_filter_t::filter_state_t compose_filter(
	bool conflicts_only,
	const nav_tree_filter_t::filter_state_t & advanced,
	const nav_tree_filter_t::filter_state_t & search)
{
	auto result = advanced;

	result.filter_by_id = search.filter_by_id;
	result.id_text = search.id_text;
	result.filter_by_name = search.filter_by_name;
	result.name_text = search.name_text;
	result.search_case_sensitive = search.search_case_sensitive;
	result.search_regex = search.search_regex;

	if (!conflicts_only)
		return result;

	result.filter_conflict_all = true;
	result.conflict_all_set = {conflict_all_t::conflict, conflict_all_t::override_benign};

	return result;
}

}
