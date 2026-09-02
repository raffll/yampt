#include "translation_example_ops.hpp"

namespace translation_example_ops {

bool contains_original(const std::vector<translation_example_t> & examples, const std::string & original)
{
	for (const auto & example : examples)
	{
		if (example.original == original)
			return true;
	}

	return false;
}

std::string format_examples_lines(const std::vector<translation_example_t> & examples)
{
	std::string lines;
	for (const auto & example : examples)
	{
		if (!lines.empty())
			lines += "\n";

		lines += example.original;
		lines += " -> ";
		lines += example.translation;
	}

	return lines;
}

std::vector<translation_example_t> add_capped(
    const std::vector<translation_example_t> & examples,
    const std::vector<translation_example_t> & pairs)
{
	auto result = examples;
	for (const auto & pair : pairs)
	{
		if (static_cast<int>(result.size()) >= max_examples)
			break;

		if (contains_original(result, pair.original))
			continue;

		result.push_back(pair);
	}

	return result;
}

std::vector<translation_example_t> remove_by_original(
    const std::vector<translation_example_t> & examples,
    const std::string & original)
{
	std::vector<translation_example_t> result;
	for (const auto & example : examples)
	{
		if (example.original == original)
			continue;

		result.push_back(example);
	}

	return result;
}

} // namespace translation_example_ops
