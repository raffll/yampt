#pragma once

#include <string>
#include <vector>

#include <translation_example.hpp>

namespace translation_example_ops {

std::string format_examples_lines(const std::vector<translation_example_t> & examples);

std::vector<translation_example_t> add_capped(
    const std::vector<translation_example_t> & examples,
    const std::vector<translation_example_t> & pairs);

std::vector<translation_example_t> remove_by_original(
    const std::vector<translation_example_t> & examples,
    const std::string & original);

bool contains_original(const std::vector<translation_example_t> & examples, const std::string & original);

} // namespace translation_example_ops
