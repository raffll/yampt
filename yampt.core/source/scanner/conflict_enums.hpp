#pragma once

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
