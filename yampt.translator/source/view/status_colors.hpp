#pragma once

#include <utility/status_types.hpp>
#include <QColor>

inline QColor get_status_color(status_t status)
{
	switch (status)
	{
	case status_t::untranslated:
		return QColor(166, 166, 166);
	case status_t::missing:
		return QColor(242, 140, 89);
	case status_t::duplicate:
		return QColor(242, 230, 102);
	case status_t::mismatch:
		return QColor(230, 115, 140);
	case status_t::error:
		return QColor(242, 102, 102);
	case status_t::translated:
		return QColor(128, 230, 128);
	case status_t::reused:
		return QColor(128, 217, 179);
	case status_t::adapted:
		return QColor(179, 140, 217);
	case status_t::changed:
		return QColor(242, 179, 102);
	case status_t::outdated:
		return QColor(220, 140, 80);
	case status_t::in_progress:
		return QColor(102, 153, 242);
	case status_t::model:
		return QColor(100, 180, 220);
	case status_t::propagated:
		return QColor(180, 230, 230);
	case status_t::heuristic:
		return QColor(100, 180, 160);
	case status_t::to_verify:
		return QColor(180, 200, 180);
	case status_t::ambiguous:
		return QColor(230, 180, 60);
	}

	return QColor(217, 217, 217);
}
