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

	if (status == "mismatch")
		return QColor(230, 115, 140);

	if (status == "matched" || status == "fingerprint" || status == "coords" || status == "heuristic" ||
	    status == "exact" || status == "info" || status == "wilderness" || status == "region")
		return QColor(140, 200, 170);

	if (status == "error")
		return QColor(242, 102, 102);

	if (status == "identical" || status == "translated")
		return QColor(128, 230, 128);

	if (status == "reused")
		return QColor(128, 217, 179);

	if (status == "adapted")
		return QColor(179, 140, 217);

	if (status == "changed")
		return QColor(242, 179, 102);

	if (status == "in_progress")
		return QColor(102, 153, 242);

	if (status == "model")
		return QColor(100, 180, 220);

	if (status == "propagated")
		return QColor(180, 230, 230);

	return QColor(217, 217, 217);
}
