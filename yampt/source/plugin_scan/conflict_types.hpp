#pragma once
#include "conflict_enums.hpp"
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

inline QColor conflict_all_color_raw(conflict_all_t ca)
{
	switch (ca)
	{
	case conflict_all_t::no_conflict:
		return QColor(0, 255, 0);
	case conflict_all_t::override_benign:
		return QColor(255, 255, 0);
	case conflict_all_t::conflict:
		return QColor(255, 0, 0);
	default:
		return QColor(255, 255, 255);
	}
}

inline QColor conflict_all_background(conflict_all_t ca)
{
	if (ca < conflict_all_t::no_conflict)
		return QColor(255, 255, 255);

	return lighter_hsl(conflict_all_color_raw(ca), 0.85);
}

inline QColor conflict_this_foreground(conflict_this_t ct)
{
	switch (ct)
	{
	case conflict_this_t::master:
		return QColor(128, 0, 128);
	case conflict_this_t::identical_to_master:
		return QColor(128, 128, 128);
	case conflict_this_t::override_wins:
		return QColor(0, 128, 0);
	case conflict_this_t::conflict_wins:
		return QColor(255, 128, 64);
	case conflict_this_t::conflict_loses:
		return QColor(255, 0, 0);
	case conflict_this_t::deleted:
		return QColor(128, 128, 128);
	default:
		return QColor(0, 0, 0);
	}
}
