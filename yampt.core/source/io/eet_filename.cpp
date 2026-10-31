#include "eet_filename.hpp"
#include "../utility/string_utils.hpp"

namespace eet_filename {

namespace {

constexpr std::string_view eet_extension = ".eet";

std::string_view strip_eet_extension(std::string_view filename)
{
	if (filename.size() < eet_extension.size())
		return filename;

	const auto suffix = filename.substr(filename.size() - eet_extension.size());
	if (!string_utils::case_insensitive_equal(suffix, eet_extension))
		return filename;

	return filename.substr(0, filename.size() - eet_extension.size());
}

} // namespace

std::optional<parts_t> parse(std::string_view filename)
{
	const auto leaf = string_utils::extract_filename(filename);
	const auto stem = strip_eet_extension(leaf);

	const auto underscore = stem.find_last_of('_');
	if (underscore == std::string_view::npos)
		return std::nullopt;

	const auto name = stem.substr(0, underscore);
	const auto hash = stem.substr(underscore + 1);

	if (name.empty() || hash.empty())
		return std::nullopt;

	return parts_t { std::string(name), std::string(hash) };
}

} // namespace eet_filename
