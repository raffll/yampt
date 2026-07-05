#pragma once
#include <scanner/conflict_enums.hpp>
#include <utility/color_palette.hpp>
#include <utility/theme_enums.hpp>
#include <algorithm>
#include <QColor>

inline QColor lighter_hsl(const QColor & color, double amount)
{
	float h, s, l, a;
	color.getHslF(&h, &s, &l, &a);
	l = std::min(static_cast<float>(l + (1.0 - l) * amount), 1.0f);
	QColor result;
	result.setHslF(h, s, l, a);
	return result;
}

inline QColor darker_hsl(const QColor & color, double amount)
{
	float h, s, l, a;
	color.getHslF(&h, &s, &l, &a);
	l = std::max(static_cast<float>(l * (1.0 - amount)), 0.0f);
	QColor result;
	result.setHslF(h, s, l, a);
	return result;
}

inline QColor conflict_all_color_raw(conflict_all_t ca, theme_t theme = theme_t::light)
{
	color_name_t name = color_name_t::conflict_all_no_conflict_raw;
	switch (ca)
	{
	case conflict_all_t::no_conflict:
		name = color_name_t::conflict_all_no_conflict_raw;
		break;
	case conflict_all_t::override_benign:
		name = color_name_t::conflict_all_override_benign_raw;
		break;
	case conflict_all_t::conflict:
		name = color_name_t::conflict_all_conflict_raw;
		break;
	default:
		return QColor(255, 255, 255);
	}

	const auto index = static_cast<size_t>(name);
	const auto & rgb = (theme == theme_t::dark) ? dark_palette[index] : light_palette[index];
	return QColor(rgb.red, rgb.green, rgb.blue, rgb.alpha);
}

inline QColor conflict_all_background(conflict_all_t ca, theme_t theme = theme_t::light)
{
	if (ca < conflict_all_t::no_conflict)
		return (theme == theme_t::dark) ? QColor(30, 30, 30) : QColor(255, 255, 255);

	const auto raw = conflict_all_color_raw(ca, theme);

	if (theme == theme_t::dark)
		return darker_hsl(raw, 0.70);

	return lighter_hsl(raw, 0.85);
}

inline QColor conflict_this_foreground(conflict_this_t ct, theme_t theme = theme_t::light)
{
	color_name_t name = color_name_t::conflict_this_master;
	switch (ct)
	{
	case conflict_this_t::master:
		name = color_name_t::conflict_this_master;
		break;
	case conflict_this_t::identical_to_master:
		name = color_name_t::conflict_this_identical;
		break;
	case conflict_this_t::override_wins:
		name = color_name_t::conflict_this_override_wins;
		break;
	case conflict_this_t::conflict_wins:
		name = color_name_t::conflict_this_conflict_wins;
		break;
	case conflict_this_t::conflict_loses:
		name = color_name_t::conflict_this_conflict_loses;
		break;
	case conflict_this_t::deleted:
		name = color_name_t::conflict_this_deleted;
		break;
	default:
		return (theme == theme_t::dark) ? QColor(220, 220, 220) : QColor(0, 0, 0);
	}

	const auto index = static_cast<size_t>(name);
	const auto & rgb = (theme == theme_t::dark) ? dark_palette[index] : light_palette[index];
	return QColor(rgb.red, rgb.green, rgb.blue, rgb.alpha);
}
