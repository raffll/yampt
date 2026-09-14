#pragma once
#include <string>
#include <vector>

namespace view_row_order {

inline constexpr int tier_header = 0;
inline constexpr int tier_single_value = 1;
inline constexpr int tier_data_block = 2;
inline constexpr int tier_list = 3;

struct row_kind_input_t
{
	std::string type;
	size_t size = 0;
	int occurrence = 0;
	bool has_children = false;
	bool is_info_chain = false;
	bool is_optional_placeholder = false;
	bool has_schema = false;
	bool is_repeatable = false;
	int composition_tier = -1;
};

int structural_tier(const row_kind_input_t & input);

int rank(const row_kind_input_t & input, const std::vector<std::string> & roster);

} // namespace view_row_order
