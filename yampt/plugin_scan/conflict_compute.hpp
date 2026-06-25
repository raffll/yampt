#pragma once
#include "conflict_enums.hpp"
#include <string>
#include <vector>

conflict_all_t compute_conflict_all(const std::vector<std::string> & values);
std::vector<conflict_this_t> compute_conflict_this(const std::vector<std::string> & values);
