#include "utility/char_diff.hpp"
#include "utility/string_utils.hpp"
#include <algorithm>

namespace {

std::vector<diff_segment_t> build_lcs_matrix_over_units(
    const std::vector<std::string> & old_units,
    const std::vector<std::string> & new_units)
{
	const auto old_size = old_units.size();
	const auto new_size = new_units.size();

	std::vector<std::vector<int>> matrix(old_size + 1, std::vector<int>(new_size + 1, 0));

	for (size_t row = 1; row <= old_size; ++row)
	{
		for (size_t col = 1; col <= new_size; ++col)
		{
			if (old_units[row - 1] == new_units[col - 1])
				matrix[row][col] = matrix[row - 1][col - 1] + 1;
			else
				matrix[row][col] = std::max(matrix[row - 1][col], matrix[row][col - 1]);
		}
	}

	std::vector<diff_segment_t> reversed;
	auto row = old_size;
	auto col = new_size;

	while (row > 0 || col > 0)
	{
		if (row > 0 && col > 0 && old_units[row - 1] == new_units[col - 1])
		{
			reversed.push_back({ diff_op_t::unchanged, old_units[row - 1] });
			--row;
			--col;
		}
		else if (col > 0 && (row == 0 || matrix[row][col - 1] >= matrix[row - 1][col]))
		{
			reversed.push_back({ diff_op_t::inserted, new_units[col - 1] });
			--col;
		}
		else
		{
			reversed.push_back({ diff_op_t::deleted, old_units[row - 1] });
			--row;
		}
	}

	std::vector<diff_segment_t> result;
	result.reserve(reversed.size());

	for (auto it = reversed.rbegin(); it != reversed.rend(); ++it)
	{
		if (!result.empty() && result.back().operation == it->operation)
			result.back().text += it->text;
		else
			result.push_back(std::move(*it));
	}

	return result;
}

} // namespace

std::vector<diff_segment_t> compute_char_diff(std::string_view old_text, std::string_view new_text)
{
	if (old_text == new_text)
		return { { diff_op_t::unchanged, std::string(old_text) } };

	if (old_text.empty())
		return { { diff_op_t::inserted, std::string(new_text) } };

	if (new_text.empty())
		return { { diff_op_t::deleted, std::string(old_text) } };

	const auto old_units = string_utils::split_utf8_characters(old_text);
	const auto new_units = string_utils::split_utf8_characters(new_text);

	return build_lcs_matrix_over_units(old_units, new_units);
}
