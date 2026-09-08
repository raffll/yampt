#pragma once

#include "../session/merged_patch_name.hpp"
#include <string>
#include <string_view>
#include <QString>

namespace plugin_icon {

struct tier_flags_t
{
	std::string_view filename;
	bool is_overridden = false;
	bool is_excluded = false;
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
	return full_path.find("/overwrite/") != std::string_view::npos ||
	       full_path.find("\\overwrite\\") != std::string_view::npos;
}

inline QString prefix(const tier_flags_t & flags)
{
	QString icons;

	if (flags.filename == merged_patch::filename)
		icons += QString::fromUtf8("\xE2\x9A\x99 ");
	else if (has_esm_extension(flags.filename))
		icons += QString::fromUtf8("\xF0\x9F\x93\x9C ");
	else
		icons += QString::fromUtf8("\xF0\x9F\x93\x84 ");

	if (flags.is_overridden)
		icons += QString::fromUtf8("\xE2\x9A\xA1 ");

	if (flags.is_excluded)
		icons += QString::fromUtf8("\xF0\x9F\x9A\xAB ");
	else if (flags.is_guard)
		icons += QString::fromUtf8("\xF0\x9F\x9B\xA1 ");

	if (flags.is_active)
		icons += QString::fromUtf8("\xE2\xAD\x90 ");

	return icons;
}

} // namespace plugin_icon
