#include "view_row_order.hpp"

namespace view_row_order {

namespace {

constexpr int tier_stride = 100000;
constexpr int unlisted_position = 50000;

int roster_position(const std::string & type, const std::vector<std::string> & roster)
{
	for (size_t position = 0; position < roster.size(); ++position)
	{
		if (roster[position] == type)
			return static_cast<int>(position);
	}

	return unlisted_position;
}

} // namespace

int structural_tier(const row_kind_input_t & input)
{
	if (input.type == "Record Header")
		return tier_header;

	if (input.is_info_chain)
		return tier_list;

	if (input.composition_tier >= 0)
		return input.composition_tier;

	if (input.is_optional_placeholder)
		return input.has_schema ? tier_data_block : tier_single_value;

	if (input.is_repeatable)
		return tier_list;

	if (input.size == 0 && input.has_children)
		return tier_list;

	if (input.has_children)
		return tier_data_block;

	return tier_single_value;
}

int rank(const row_kind_input_t & input, const std::vector<std::string> & roster)
{
	const int tier = structural_tier(input);
	if (tier == tier_header)
		return -1;

	return tier * tier_stride + roster_position(input.type, roster);
}

} // namespace view_row_order
