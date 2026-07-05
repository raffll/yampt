#pragma once

#include <set>
#include <string>
#include <vector>

struct match_result_t
{
	int score = 0;
	int score_orig = 0;
	int score_model = 0;
	int count = 0;
	size_t index = 0;
	std::string name;
};

std::vector<std::string> split_words(const std::string & text);

int count_shared_words(
    const std::vector<std::string> & source,
    const std::vector<std::string> & target);

std::vector<std::string> build_compare_words(
    const std::vector<std::string> & translated_words,
    const std::vector<std::string> & original_words);

match_result_t compute_best_match(
    const std::vector<std::string> & compare_words,
    const std::vector<std::string> & original_words,
    const std::vector<std::string> & translated_words,
    const std::vector<std::pair<size_t, std::string>> & candidates,
    const std::set<size_t> & matched_set);

bool check_all_same_name(
    const std::vector<std::string> & compare_words,
    const std::vector<std::pair<size_t, std::string>> & candidates,
    const std::set<size_t> & matched_set,
    const match_result_t & result);
