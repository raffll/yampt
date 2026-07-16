#pragma once

#include <utility/status_types.hpp>
#include <QString>

inline QString status_display_name(status_t status)
{
	switch (status)
	{
	case status_t::translated:
		return QStringLiteral("Translated");
	case status_t::untranslated:
		return QStringLiteral("Untranslated");
	case status_t::missing:
		return QStringLiteral("Missing");
	case status_t::duplicate:
		return QStringLiteral("Duplicate");
	case status_t::mismatch:
		return QStringLiteral("Mismatch");
	case status_t::heuristic:
		return QStringLiteral("Heuristic");
	case status_t::to_verify:
		return QStringLiteral("To Verify");
	case status_t::adapted:
		return QStringLiteral("Adapted");
	case status_t::changed:
		return QStringLiteral("Changed");
	case status_t::outdated:
		return QStringLiteral("Outdated");
	case status_t::reused:
		return QStringLiteral("Reused");
	case status_t::ambiguous:
		return QStringLiteral("Ambiguous");
	case status_t::in_progress:
		return QStringLiteral("In Progress");
	case status_t::model:
		return QStringLiteral("Generated");
	case status_t::propagated:
		return QStringLiteral("Propagated");
	case status_t::error:
		return QStringLiteral("Error");
	}

	return QStringLiteral("Error");
}
