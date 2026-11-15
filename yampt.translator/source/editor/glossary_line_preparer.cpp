#include "glossary_line_preparer.hpp"

namespace glossary_line_preparer
{
std::vector<std::string> prepare_lines(
    const std::vector<std::string> & source_lines,
    const std::function<std::string(const std::string &)> & apply_glossary_fn)
{
	std::vector<std::string> prepared_lines;

	for (const auto & source : source_lines)
	{
		if (source.empty())
			prepared_lines.push_back(std::string());
		else
			prepared_lines.push_back(apply_glossary_fn(source));
	}

	return prepared_lines;
}
}
