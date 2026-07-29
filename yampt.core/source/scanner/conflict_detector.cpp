#include "conflict_detector.hpp"
#include <map>
#include <set>
#include <tuple>

struct grouping_key_t
{
	std::string interface_name;
	std::string method_name;
	std::string type_argument;

	bool operator<(const grouping_key_t & other) const
	{
		return std::tie(interface_name, method_name, type_argument) <
		       std::tie(other.interface_name, other.method_name, other.type_argument);
	}
};

static bool has_multiple_mods(const std::vector<handler_registration_t> & registrations)
{
	std::set<std::string> distinct_mods;
	for (const auto & registration : registrations)
		distinct_mods.insert(registration.mod_name);

	return distinct_mods.size() >= 2;
}

static conflict_severity_t determine_severity(const std::vector<handler_registration_t> & registrations)
{
	for (const auto & registration : registrations)
	{
		if (registration.classification == handler_class_t::blocking)
			return conflict_severity_t::blocking;
	}

	for (const auto & registration : registrations)
	{
		if (registration.classification == handler_class_t::mutating)
			return conflict_severity_t::mutating;
	}

	return conflict_severity_t::overlapping;
}

std::vector<handler_conflict_t> conflict_detector_t::detect(
    const std::vector<handler_registration_t> & all_registrations)
{
	std::map<grouping_key_t, std::vector<handler_registration_t>> groups;

	for (const auto & registration : all_registrations)
	{
		grouping_key_t group_key;
		group_key.interface_name = registration.interface_name;
		group_key.method_name = registration.method_name;
		group_key.type_argument = registration.type_argument;
		groups[group_key].push_back(registration);
	}

	std::vector<handler_conflict_t> conflicts;

	for (const auto & [group_key, registrations] : groups)
	{
		if (!has_multiple_mods(registrations))
			continue;

		handler_conflict_t conflict;
		conflict.interface_name = group_key.interface_name;
		conflict.method_name = group_key.method_name;
		conflict.type_argument = group_key.type_argument;
		conflict.severity = determine_severity(registrations);
		conflict.registrations = registrations;
		conflicts.push_back(std::move(conflict));
	}

	return conflicts;
}
