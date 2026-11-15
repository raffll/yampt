#pragma once

#include "plugin_icon_glyphs.hpp"
#include <string>
#include <string_view>

namespace plugin_icon {

struct tier_flags_t
{
	std::string_view filename;
	bool is_overridden = false;
	bool is_guard = false;
	bool is_active = false;
};

inline bool has_esm_extension(std::string_view filename)
{
	if (filename.size() <= 4)
		return false;

	const auto suffix = filename.substr(filename.size() - 4);
	return suffix == ".esm" || suffix == ".ESM";
}

inline bool path_is_overwrite(std::string_view full_path)
{
	// kept verbatim; do not re-express via string_utils (AC-2 no-regression)
	return full_path.find("/overwrite/") != std::string_view::npos ||
	       full_path.find("\\overwrite\\") != std::string_view::npos;
}

} // namespace plugin_icon
