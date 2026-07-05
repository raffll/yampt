#include "record_conflict.hpp"

conflict_all_t compute_conflict_all(const std::vector<std::string> & values)
{
	if (values.size() <= 1)
		return conflict_all_t::only_one;

	bool all_same = true;
	for (size_t i = 1; i < values.size(); ++i)
	{
		if (values[i] != values[0])
		{
			all_same = false;
			break;
		}
	}

	if (all_same)
		return conflict_all_t::no_conflict;

	const auto & winner = values.back();
	for (size_t i = 0; i < values.size() - 1; ++i)
	{
		if (values[i] != values[0] && values[i] != winner)
			return conflict_all_t::conflict;
	}

	return conflict_all_t::override_benign;
}

conflict_all_t compute_conflict_all_skip_empty(const std::vector<std::string> & values)
{
	std::vector<std::string> present;
	for (const auto & value : values)
	{
		if (!value.empty())
			present.push_back(value);
	}

	if (present.size() <= 1)
		return conflict_all_t::only_one;

	bool all_same = true;
	for (size_t i = 1; i < present.size(); ++i)
	{
		if (present[i] != present[0])
		{
			all_same = false;
			break;
		}
	}

	if (all_same)
		return conflict_all_t::no_conflict;

	const auto & winner = present.back();
	for (size_t i = 0; i < present.size() - 1; ++i)
	{
		if (present[i] != present[0] && present[i] != winner)
			return conflict_all_t::conflict;
	}

	return conflict_all_t::override_benign;
}

std::vector<conflict_this_t> compute_conflict_this(const std::vector<std::string> & values)
{
	std::vector<conflict_this_t> result(values.size(), conflict_this_t::unknown);

	if (values.empty())
		return result;

	if (values.size() == 1)
	{
		result[0] = values[0].empty() ? conflict_this_t::unknown : conflict_this_t::master;
		return result;
	}

	result[0] = values[0].empty() ? conflict_this_t::unknown : conflict_this_t::master;

	const auto & winner = values.back();
	bool is_override = true;

	for (size_t i = 0; i < values.size() - 1; ++i)
	{
		if (values[i].empty())
			continue;

		if (values[i] != values[0] && values[i] != winner)
		{
			is_override = false;
			break;
		}
	}

	for (size_t i = 1; i < values.size(); ++i)
	{
		if (values[i].empty() && values[0].empty())
		{
			result[i] = conflict_this_t::identical_to_master;
			continue;
		}

		if (values[i].empty())
		{
			result[i] = conflict_this_t::unknown;
			continue;
		}

		if (values[i] == values[0])
		{
			result[i] = conflict_this_t::identical_to_master;
			continue;
		}

		if (is_override)
			result[i] = conflict_this_t::override_wins;
		else if (i == values.size() - 1)
			result[i] = conflict_this_t::conflict_wins;
		else
			result[i] = conflict_this_t::conflict_loses;
	}

	return result;
}

std::vector<conflict_this_t> compute_conflict_this_skip_empty(const std::vector<std::string> & values)
{
	std::vector<conflict_this_t> result(values.size(), conflict_this_t::unknown);

	if (values.empty())
		return result;

	std::string first_present;
	size_t first_idx = SIZE_MAX;
	std::string last_present;
	size_t last_idx = SIZE_MAX;

	for (size_t i = 0; i < values.size(); ++i)
	{
		if (values[i].empty())
			continue;

		if (first_idx == SIZE_MAX)
		{
			first_present = values[i];
			first_idx = i;
		}

		last_present = values[i];
		last_idx = i;
	}

	if (first_idx == SIZE_MAX)
		return result;

	if (first_idx == last_idx)
	{
		result[first_idx] = conflict_this_t::master;
		return result;
	}

	result[first_idx] = conflict_this_t::master;

	bool is_override = true;
	for (size_t i = 0; i < values.size(); ++i)
	{
		if (values[i].empty())
			continue;

		if (values[i] != first_present && values[i] != last_present)
		{
			is_override = false;
			break;
		}
	}

	for (size_t i = 0; i < values.size(); ++i)
	{
		if (i == first_idx)
			continue;

		if (values[i].empty())
			continue;

		if (values[i] == first_present)
		{
			result[i] = conflict_this_t::identical_to_master;
			continue;
		}

		if (is_override)
			result[i] = conflict_this_t::override_wins;
		else if (i == last_idx)
			result[i] = conflict_this_t::conflict_wins;
		else
			result[i] = conflict_this_t::conflict_loses;
	}

	return result;
}
