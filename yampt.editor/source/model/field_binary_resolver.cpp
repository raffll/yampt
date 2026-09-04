
#include <model/field_binary_resolver.hpp>

namespace field_binary_resolver
{
	const view_tree_model_t::view_node_t * find_sub_record_node(
	    const std::vector<const view_tree_model_t::view_node_t *> & ancestors_nearest_first)
	{
		for (const auto * node : ancestors_nearest_first)
		{
			if (node == nullptr)
				continue;

			if (node->type.empty())
				continue;

			return node;
		}

		return nullptr;
	}

	int binary_index_for_column(const view_tree_model_t::view_node_t & sub_record_node, int column)
	{
		if (column < 0 || column >= static_cast<int>(sub_record_node.binary_ranges.size()))
			return -1;

		return sub_record_node.binary_ranges[column].start;
	}

	resolved_field_t resolve(
	    const std::vector<const view_tree_model_t::view_node_t *> & ancestors_nearest_first,
	    int column,
	    int schema_field_index)
	{
		if (schema_field_index < 0)
			return {};

		const auto * sub_record_node = find_sub_record_node(ancestors_nearest_first);
		if (sub_record_node == nullptr)
			return {};

		const int binary_index = binary_index_for_column(*sub_record_node, column);
		if (binary_index < 0)
			return {};

		return { true, sub_record_node->type, sub_record_node->size, binary_index };
	}

	resolved_bit_t resolve_bit(
	    const std::vector<const view_tree_model_t::view_node_t *> & ancestors_nearest_first,
	    int column,
	    int schema_field_index,
	    int bit_index)
	{
		if (bit_index < 0)
			return {};

		const auto field = resolve(ancestors_nearest_first, column, schema_field_index);
		if (!field.found)
			return {};

		return { true, field.sub_type, field.sub_size, field.binary_index, schema_field_index, bit_index };
	}
}
