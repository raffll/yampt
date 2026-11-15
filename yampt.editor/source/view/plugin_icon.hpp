#pragma once

#include "../session/merged_patch_name.hpp"
#include <utility/plugin_icon_glyphs.hpp>
#include <utility/plugin_icon_tiers.hpp>
#include <QCoreApplication>
#include <QString>

namespace plugin_icon {

inline QString role_icon(const tier_flags_t & flags)
{
	if (flags.is_guard)
		return QString::fromUtf8(glyph::shield);

	if (flags.filename == merged_patch::filename)
		return QString::fromUtf8(glyph::gear);

	if (flags.is_overridden)
		return QString::fromUtf8(glyph::bolt);

	if (has_esm_extension(flags.filename))
		return QString::fromUtf8(glyph::scroll);

	return QString::fromUtf8(glyph::page);
}

inline QString prefix(const tier_flags_t & flags)
{
	QString icons = role_icon(flags) + " ";

	if (flags.is_active)
		icons += QCoreApplication::translate("yEditor", "[Active]") + " ";

	return icons;
}

} // namespace plugin_icon
