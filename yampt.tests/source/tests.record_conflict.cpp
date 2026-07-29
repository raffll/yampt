#include <catch2/catch_all.hpp>
#include <scanner/record_conflict.hpp>

TEST_CASE("record_conflict::compute_conflict_all, single value yields only_one", "[u]")
{
	std::vector<std::string> values = { "A" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::only_one);
}

TEST_CASE("record_conflict::compute_conflict_all, empty vector yields only_one", "[u]")
{
	std::vector<std::string> values = {};
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::only_one);
}

TEST_CASE("record_conflict::compute_conflict_all, all same yields no_conflict", "[u]")
{
	std::vector<std::string> values = { "A", "A", "A" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::no_conflict);
}

TEST_CASE("record_conflict::compute_conflict_all, only last differs yields override_benign", "[u]")
{
	std::vector<std::string> values = { "A", "A", "B" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_all, multiple differ yields conflict", "[u]")
{
	std::vector<std::string> values = { "A", "B", "C" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::conflict);
}

TEST_CASE("record_conflict::compute_conflict_all, two values same yields no_conflict", "[u]")
{
	std::vector<std::string> values = { "X", "X" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::no_conflict);
}

TEST_CASE("record_conflict::compute_conflict_all, two values differ yields override_benign", "[u]")
{
	std::vector<std::string> values = { "X", "Y" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_this, single value assigns master", "[u]")
{
	std::vector<std::string> values = { "A" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result.size() == 1);
	REQUIRE(result[0] == conflict_this_t::master);
}

TEST_CASE("record_conflict::compute_conflict_this, empty value assigns unknown", "[u]")
{
	std::vector<std::string> values = { "" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result.size() == 1);
	REQUIRE(result[0] == conflict_this_t::unknown);
}

TEST_CASE("record_conflict::compute_conflict_this, identical to master", "[u]")
{
	std::vector<std::string> values = { "A", "A", "A" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result.size() == 3);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::identical_to_master);
	REQUIRE(result[2] == conflict_this_t::identical_to_master);
}

TEST_CASE("record_conflict::compute_conflict_this, override wins", "[u]")
{
	std::vector<std::string> values = { "A", "A", "B" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result.size() == 3);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::identical_to_master);
	REQUIRE(result[2] == conflict_this_t::override_wins);
}

TEST_CASE("record_conflict::compute_conflict_this, conflict wins and loses", "[u]")
{
	std::vector<std::string> values = { "A", "B", "C" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result.size() == 3);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::conflict_loses);
	REQUIRE(result[2] == conflict_this_t::conflict_wins);
}

TEST_CASE("record_conflict::compute_conflict_this, empty cell in non-master", "[u]")
{
	std::vector<std::string> values = { "A", "", "B" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result.size() == 3);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::unknown);
	REQUIRE(result[2] == conflict_this_t::override_wins);
}

TEST_CASE("record_conflict::compute_conflict_all, parent inherits worst from children", "[u]")
{
	std::vector<conflict_all_t> children = { conflict_all_t::no_conflict,
		                                     conflict_all_t::no_conflict,
		                                     conflict_all_t::override_benign };

	conflict_all_t parent = conflict_all_t::only_one;
	for (const auto & child : children)
	{
		if (child > parent)
			parent = child;
	}

	REQUIRE(parent == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_all, parent inherits conflict from one child", "[u]")
{
	std::vector<conflict_all_t> children = { conflict_all_t::no_conflict,
		                                     conflict_all_t::conflict,
		                                     conflict_all_t::no_conflict };

	conflict_all_t parent = conflict_all_t::only_one;
	for (const auto & child : children)
	{
		if (child > parent)
			parent = child;
	}

	REQUIRE(parent == conflict_all_t::conflict);
}

TEST_CASE("record_conflict::compute_conflict_all, parent stays no_conflict when all children agree", "[u]")
{
	std::vector<conflict_all_t> children = { conflict_all_t::no_conflict,
		                                     conflict_all_t::no_conflict,
		                                     conflict_all_t::no_conflict };

	conflict_all_t parent = conflict_all_t::only_one;
	for (const auto & child : children)
	{
		if (child > parent)
			parent = child;
	}

	REQUIRE(parent == conflict_all_t::no_conflict);
}

TEST_CASE("record_conflict::compute_conflict_all, parent with no children stays only_one", "[u]")
{
	std::vector<conflict_all_t> children = {};

	conflict_all_t parent = conflict_all_t::only_one;
	for (const auto & child : children)
	{
		if (child > parent)
			parent = child;
	}

	REQUIRE(parent == conflict_all_t::only_one);
}

TEST_CASE("record_conflict::compute_conflict_all, parent ignores only_one children", "[u]")
{
	std::vector<conflict_all_t> children = { conflict_all_t::only_one,
		                                     conflict_all_t::no_conflict,
		                                     conflict_all_t::only_one };

	conflict_all_t parent = conflict_all_t::only_one;
	for (const auto & child : children)
	{
		if (child > parent)
			parent = child;
	}

	REQUIRE(parent == conflict_all_t::no_conflict);
}

TEST_CASE("record_conflict::compute_conflict_all, empty merge column excluded yields no_conflict", "[u]")
{
	std::vector<std::string> all_values = { "A", "A", "" };
	int merge_col = 2;

	std::vector<std::string> filtered;
	for (size_t i = 0; i < all_values.size(); ++i)
	{
		if (static_cast<int>(i) != merge_col || !all_values[i].empty())
			filtered.push_back(all_values[i]);
	}

	REQUIRE(record_conflict::compute_conflict_all(filtered) == conflict_all_t::no_conflict);
}

TEST_CASE("record_conflict::compute_conflict_all, populated merge column participates in conflict", "[u]")
{
	std::vector<std::string> all_values = { "A", "A", "B" };
	int merge_col = 2;

	std::vector<std::string> filtered;
	for (size_t i = 0; i < all_values.size(); ++i)
	{
		if (static_cast<int>(i) != merge_col || !all_values[i].empty())
			filtered.push_back(all_values[i]);
	}

	REQUIRE(record_conflict::compute_conflict_all(filtered) == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_all, empty merge column excluded from real conflict", "[u]")
{
	std::vector<std::string> all_values = { "A", "B", "" };
	int merge_col = 2;

	std::vector<std::string> filtered;
	for (size_t i = 0; i < all_values.size(); ++i)
	{
		if (static_cast<int>(i) != merge_col || !all_values[i].empty())
			filtered.push_back(all_values[i]);
	}

	REQUIRE(record_conflict::compute_conflict_all(filtered) == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_all, no merge column leaves values unchanged", "[u]")
{
	std::vector<std::string> all_values = { "A", "B", "C" };
	int merge_col = -1;

	std::vector<std::string> filtered;
	for (size_t i = 0; i < all_values.size(); ++i)
	{
		if (static_cast<int>(i) != merge_col || !all_values[i].empty())
			filtered.push_back(all_values[i]);
	}

	REQUIRE(record_conflict::compute_conflict_all(filtered) == conflict_all_t::conflict);
}

TEST_CASE("record_conflict::compute_conflict_this, empty merge column excluded preserves statuses", "[u]")
{
	std::vector<std::string> all_values = { "A", "A", "" };
	int merge_col = 2;

	std::vector<std::string> filtered;
	for (size_t i = 0; i < all_values.size(); ++i)
	{
		if (static_cast<int>(i) != merge_col || !all_values[i].empty())
			filtered.push_back(all_values[i]);
	}

	auto result = record_conflict::compute_conflict_this(filtered);
	REQUIRE(result.size() == 2);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::identical_to_master);
}

TEST_CASE("record_conflict::compute_conflict_this, populated merge column gets status", "[u]")
{
	std::vector<std::string> all_values = { "A", "A", "B" };
	int merge_col = 2;

	std::vector<std::string> filtered;
	for (size_t i = 0; i < all_values.size(); ++i)
	{
		if (static_cast<int>(i) != merge_col || !all_values[i].empty())
			filtered.push_back(all_values[i]);
	}

	auto result = record_conflict::compute_conflict_this(filtered);
	REQUIRE(result.size() == 3);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::identical_to_master);
	REQUIRE(result[2] == conflict_this_t::override_wins);
}

TEST_CASE("record_conflict::compute_conflict_all, non_existent_value treated as real value", "[u]")
{
	std::vector<std::string> values = { "hello", non_existent_value, "hello" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::conflict);
}

TEST_CASE("record_conflict::compute_conflict_all, first differs from rest yields conflict", "[u]")
{
	std::vector<std::string> values = { "X", "Y", "Y" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_all, four values all same", "[u]")
{
	std::vector<std::string> values = { "A", "A", "A", "A" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::no_conflict);
}

TEST_CASE("record_conflict::compute_conflict_all, four values last differs", "[u]")
{
	std::vector<std::string> values = { "A", "A", "A", "B" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_all, four values middle differs", "[u]")
{
	std::vector<std::string> values = { "A", "B", "A", "A" };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::conflict);
}

TEST_CASE("record_conflict::compute_conflict_this, four values override pattern", "[u]")
{
	std::vector<std::string> values = { "A", "A", "A", "B" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::identical_to_master);
	REQUIRE(result[2] == conflict_this_t::identical_to_master);
	REQUIRE(result[3] == conflict_this_t::override_wins);
}

TEST_CASE("record_conflict::compute_conflict_this, four values conflict pattern", "[u]")
{
	std::vector<std::string> values = { "A", "B", "C", "D" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::conflict_loses);
	REQUIRE(result[2] == conflict_this_t::conflict_loses);
	REQUIRE(result[3] == conflict_this_t::conflict_wins);
}

TEST_CASE("record_conflict::compute_conflict_this, master same as winner others differ", "[u]")
{
	std::vector<std::string> values = { "A", "B", "C", "A" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::conflict_loses);
	REQUIRE(result[2] == conflict_this_t::conflict_loses);
	REQUIRE(result[3] == conflict_this_t::identical_to_master);
}

TEST_CASE("record_conflict::compute_conflict_this, all empty yields all unknown", "[u]")
{
	std::vector<std::string> values = { "", "", "" };
	auto result = record_conflict::compute_conflict_this(values);
	REQUIRE(result[0] == conflict_this_t::unknown);
	REQUIRE(result[1] == conflict_this_t::identical_to_master);
	REQUIRE(result[2] == conflict_this_t::identical_to_master);
}
