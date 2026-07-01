#pragma once

#include <string>
#include <string_view>
#include <vector>

enum class diff_op_t { unchanged, inserted, deleted };

struct diff_segment_t
{
	diff_op_t operation;
	std::string text;
};

std::vector<std::vector<int>> build_lcs_matrix(std::string_view old_text,
                                               std::string_view new_text);

std::vector<diff_segment_t> backtrack_diff(const std::vector<std::vector<int>> & matrix,
                                           std::string_view old_text,
                                           std::string_view new_text);

std::vector<diff_segment_t> compute_char_diff(std::string_view old_text,
                                              std::string_view new_text);
