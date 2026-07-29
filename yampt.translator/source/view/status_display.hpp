#pragma once

#include <utility/status_types.hpp>
#include <QCoreApplication>
#include <QString>

inline QString status_display_name(status_t status)
{
	switch (status)
	{
	case status_t::translated:
		return QCoreApplication::translate("yTranslator", "Translated");
	case status_t::untranslated:
		return QCoreApplication::translate("yTranslator", "Untranslated");
	case status_t::missing:
		return QCoreApplication::translate("yTranslator", "Missing");
	case status_t::duplicate:
		return QCoreApplication::translate("yTranslator", "Duplicate");
	case status_t::mismatch:
		return QCoreApplication::translate("yTranslator", "Mismatch");
	case status_t::heuristic:
		return QCoreApplication::translate("yTranslator", "Heuristic");
	case status_t::to_verify:
		return QCoreApplication::translate("yTranslator", "To Verify");
	case status_t::adapted:
		return QCoreApplication::translate("yTranslator", "Adapted");
	case status_t::changed:
		return QCoreApplication::translate("yTranslator", "Changed");
	case status_t::outdated:
		return QCoreApplication::translate("yTranslator", "Outdated");
	case status_t::reused:
		return QCoreApplication::translate("yTranslator", "Reused");
	case status_t::ambiguous:
		return QCoreApplication::translate("yTranslator", "Ambiguous");
	case status_t::in_progress:
		return QCoreApplication::translate("yTranslator", "In Progress");
	case status_t::model:
		return QCoreApplication::translate("yTranslator", "Generated");
	case status_t::propagated:
		return QCoreApplication::translate("yTranslator", "Propagated");
	case status_t::replaced:
		return QCoreApplication::translate("yTranslator", "Replaced");
	case status_t::error:
		return QCoreApplication::translate("yTranslator", "Error");
	}

	return QCoreApplication::translate("yTranslator", "Error");
}
