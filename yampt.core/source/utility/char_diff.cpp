#include "utility/char_diff.hpp"
#include <algorithm>

std::vector<std::vector<int>> build_lcs_matrix(std::string_view old_text, std::string_view new_text)
{
	const auto old_size = old_text.size();
	const auto new_size = new_text.size();

	std::vector<std::vector<int>> matrix(old_size + 1, std::vector<int>(new_size + 1, 0));

	for (size_t row = 1; row <= old_size; ++row)
	{
		for (size_t col = 1; col <= new_size; ++col)
		{
			if (old_text[row - 1] == new_text[col - 1])
				matrix[row][col] = matrix[row - 1][col - 1] + 1;
			else
				matrix[row][col] = std::max(matrix[row - 1][col], matrix[row][col - 1]);
		}
	}

	return matrix;
}

static std::vector<diff_segment_t> collapse_segments(std::vector<diff_segment_t> & reversed)
{
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

std::vector<diff_segment_t> backtrack_diff(
    const std::vector<std::vector<int>> & matrix,
    std::string_view old_text,
    std::string_view new_text)
{
	std::vector<diff_segment_t> reversed;
	auto row = old_text.size();
	auto col = new_text.size();

	while (row > 0 || col > 0)
	{
		if (row > 0 && col > 0 && old_text[row - 1] == new_text[col - 1])
		{
			reversed.push_back({ diff_op_t::unchanged, std::string(1, old_text[row - 1]) });
			--row;
			--col;
		}
		else if (col > 0 && (row == 0 || matrix[row][col - 1] >= matrix[row - 1][col]))
		{
			reversed.push_back({ diff_op_t::inserted, std::string(1, new_text[col - 1]) });
			--col;
		}
		else
		{
			reversed.push_back({ diff_op_t::deleted, std::string(1, old_text[row - 1]) });
			--row;
		}
	}

	return collapse_segments(reversed);
}

std::vector<diff_segment_t> compute_char_diff(std::string_view old_text, std::string_view new_text)
{
	if (old_text == new_text)
		return { { diff_op_t::unchanged, std::string(old_text) } };

	if (old_text.empty())
		return { { diff_op_t::inserted, std::string(new_text) } };

	if (new_text.empty())
		return { { diff_op_t::deleted, std::string(old_text) } };

	const auto & matrix = build_lcs_matrix(old_text, new_text);
	return backtrack_diff(matrix, old_text, new_text);
}
