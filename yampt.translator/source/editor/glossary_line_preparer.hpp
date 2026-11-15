#pragma once

#include <functional>
#include <string>
#include <vector>

namespace glossary_line_preparer
{
std::vector<std::string> prepare_lines(
    const std::vector<std::string> & source_lines,
    const std::function<std::string(const std::string &)> & apply_glossary_fn);
}
