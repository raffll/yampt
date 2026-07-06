#pragma once
#include "conflict_enums.hpp"
#include <string>
#include <vector>

inline const std::string non_existent_value { "\x00_NE", 4 };

struct conflict_policy_t
{
	bool skip_non_existent = false;
	bool ignore_conflict = false;
};

class record_conflict_t
{
public:
	static conflict_all_t compute_conflict_all(const std::vector<std::string> & values);
	static conflict_all_t compute_conflict_all_skip_empty(const std::vector<std::string> & values);
	static std::vector<conflict_this_t> compute_conflict_this(const std::vector<std::string> & values);
	static std::vector<conflict_this_t> compute_conflict_this_skip_empty(const std::vector<std::string> & values);
	static conflict_policy_t find_conflict_policy(const std::string & record_type, const std::string & sub_type);
};
