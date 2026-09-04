#pragma once

#include <model/view_tree_model.hpp>
#include <string>
#include <vector>

namespace field_binary_resolver
{
	struct resolved_field_t
	{
		bool found = false;
		std::string sub_type;
		size_t sub_size = 0;
		int binary_index = -1;
		int occurrence = 0;
	};

	const view_tree_model_t::view_node_t * find_sub_record_node(
	    const std::vector<const view_tree_model_t::view_node_t *> & ancestors_nearest_first);

	int binary_index_for_column(const view_tree_model_t::view_node_t & sub_record_node, int column);

	resolved_field_t resolve(
	    const std::vector<const view_tree_model_t::view_node_t *> & ancestors_nearest_first,
	    int column,
	    int schema_field_index);

	struct resolved_bit_t
	{
		bool found = false;
		std::string sub_type;
		size_t sub_size = 0;
		int binary_index = -1;
		int field_index = -1;
		int bit_index = -1;
		int occurrence = 0;
	};

	resolved_bit_t resolve_bit(
	    const std::vector<const view_tree_model_t::view_node_t *> & ancestors_nearest_first,
	    int column,
	    int schema_field_index,
	    int bit_index);
}
