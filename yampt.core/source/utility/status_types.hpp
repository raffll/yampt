#pragma once

#include <array>
#include <string_view>

enum class status_t
{
	translated,
	untranslated,
	missing,
	duplicate,
	mismatch,
	heuristic,
	to_verify,
	adapted,
	changed,
	outdated,
	reused,
	ambiguous,
	in_progress,
	model,
	propagated,
	error
};

// clang-format off
inline constexpr std::array<std::pair<status_t, std::string_view>, 16> status_entries
{{
    { status_t::translated,   "translated" },
    { status_t::untranslated, "untranslated" },
    { status_t::missing,      "missing" },
    { status_t::duplicate,    "duplicate" },
    { status_t::mismatch,     "mismatch" },
    { status_t::heuristic,    "heuristic" },
    { status_t::to_verify,    "to_verify" },
    { status_t::adapted,      "adapted" },
    { status_t::changed,      "changed" },
    { status_t::outdated,     "outdated" },
    { status_t::reused,       "reused" },
    { status_t::ambiguous,    "ambiguous" },
    { status_t::in_progress,  "in_progress" },
    { status_t::model,        "model" },
    { status_t::propagated,   "propagated" },
    { status_t::error,        "error" },
}};
// clang-format on

constexpr std::string_view status_to_string(status_t status)
{
	for (const auto & [value, name] : status_entries)
	{
		if (value == status)
			return name;
	}

	return "error";
}

constexpr status_t string_to_status(std::string_view text)
{
	for (const auto & [value, name] : status_entries)
	{
		if (name == text)
			return value;
	}

	return status_t::error;
}

#include <ostream>

inline std::ostream & operator<<(std::ostream & stream, status_t status)
{
	return stream << status_to_string(status);
}
