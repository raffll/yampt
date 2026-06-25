#pragma once
#include <QColor>
#include <algorithm>

// View tree coloring matrix (xEdit reproduction):
//
// BACKGROUND (per row, from row_conflict_all):
//   unknown          -> white (no paint)
//   only_one         -> white (no paint)
//   no_conflict      -> light green (lime lightened 85%)
//   override_benign  -> light yellow (lightened 85%)
//   conflict         -> light red (lightened 85%)
//
// TEXT COLOR (per cell, from conflict_this):
//   unknown              -> black     (0, 0, 0)       single plugin / unresolved
//   master               -> purple    (128, 0, 128)   first plugin (defines the record)
//   identical_to_master  -> gray      (128, 128, 128) same value as master
//   override_wins        -> green     (0, 128, 0)     overrides master, wins by load order
//   conflict_wins        -> orange    (255, 128, 64)  conflicts, wins by load order
//   conflict_loses       -> red       (255, 0, 0)     conflicts, loses
//   deleted              -> gray      (128, 128, 128) record deleted
//
// COMBINED (row scenario):
//   1 plugin               -> bg: white,  text: black (all cols)
//   2 plugins, all same    -> bg: green,  text: purple (master) + gray (identical)
//   2 plugins, last differs-> bg: yellow, text: purple (master) + green (override_wins)
//   3+ plugins, all same   -> bg: green,  text: purple (master) + gray (identical)
//   3+ plugins, last differs->bg: yellow, text: purple + gray + green (last)
//   3+ plugins, multi differ->bg: red,    text: purple + orange (wins) + red (loses)
//
// SPECIAL:
//   single plugin (column_names_.size() <= 1) -> skip ForegroundRole (black)
//   label column (col 0) -> worst conflict_this across all plugin columns
//   empty cell in painted row -> background lighter by +0.08 (factor 0.93)

enum class conflict_all_t
{
	unknown,
	only_one,
	no_conflict,
	override_benign,
	conflict
};

enum class conflict_this_t
{
	unknown,
	master,
	identical_to_master,
	override_wins,
	conflict_wins,
	conflict_loses,
	deleted
};

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
