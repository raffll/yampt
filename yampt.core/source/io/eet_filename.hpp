#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace eet_filename {

struct parts_t
{
	std::string name;
	std::string hash;
};

std::optional<parts_t> parse(std::string_view filename);

} // namespace eet_filename
