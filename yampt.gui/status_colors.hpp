#pragma once

#include <QColor>
#include <string>

inline QColor get_status_color(const std::string & status)
{
	if (status == "untranslated")
		return QColor(166, 166, 166);

	if (status == "missing")
		return QColor(242, 140, 89);

	if (status == "duplicate")
		return QColor(242, 230, 102);

	if (status == "coords")
		return QColor(102, 204, 204);

	if (status == "fingerprint")
		return QColor(102, 191, 217);

	if (status == "heuristic")
		return QColor(153, 179, 230);

	if (status == "info")
		return QColor(115, 217, 191);

	if (status == "exact")
		return QColor(128, 230, 217);

	if (status == "wilderness")
		return QColor(77, 153, 77);

	if (status == "region")
		return QColor(153, 153, 89);

	if (status == "matched")
		return QColor(140, 200, 170);

	if (status == "error")
		return QColor(242, 102, 102);

	if (status == "identical")
		return QColor(140, 191, 140);

	if (status == "translated")
		return QColor(128, 230, 128);

	if (status == "reused")
		return QColor(128, 217, 179);

	if (status == "adapted")
		return QColor(179, 140, 217);

	if (status == "changed")
		return QColor(242, 179, 102);

	if (status == "in_progress")
		return QColor(102, 153, 242);

	if (status == "mismatch")
		return QColor(230, 115, 140);

	return QColor(217, 217, 217);
}
