#include <catch2/catch_test_macros.hpp>
#include <model/filter_composer.hpp>

TEST_CASE("filter_composer::compose_filter, shortcut only sets conflict all", "[u]")
{
	const nav_tree_filter_t::filter_state_t advanced;
	const nav_tree_filter_t::filter_state_t search;

	const auto result = filter_composer::compose_filter(true, advanced, search);

	REQUIRE(result.filter_conflict_all == true);
	REQUIRE(result.conflict_all_set == std::set<conflict_all_t>{ conflict_all_t::conflict, conflict_all_t::override_benign });
	REQUIRE(result.filter_conflict_this == false);
	REQUIRE(result.filter_by_type == false);
	REQUIRE(result.filter_by_id == false);
	REQUIRE(result.filter_by_name == false);
	REQUIRE(result.filter_deleted == false);
	REQUIRE(result.filter_lua_severity == false);
	REQUIRE(result.filter_lua_interface == false);
}

TEST_CASE("filter_composer::compose_filter, advanced conflict all preserved when shortcut off", "[u]")
{
	nav_tree_filter_t::filter_state_t advanced;
	advanced.filter_conflict_all = true;
	advanced.conflict_all_set = { conflict_all_t::conflict };

	const nav_tree_filter_t::filter_state_t search;

	const auto result = filter_composer::compose_filter(false, advanced, search);

	REQUIRE(result.filter_conflict_all == true);
	REQUIRE(result.conflict_all_set == std::set<conflict_all_t>{ conflict_all_t::conflict });
}

TEST_CASE("filter_composer::compose_filter, shortcut overrides advanced conflict all while other dims survive", "[u]")
{
	nav_tree_filter_t::filter_state_t advanced;
	advanced.filter_conflict_all = true;
	advanced.conflict_all_set = { conflict_all_t::conflict };
	advanced.filter_conflict_this = true;
	advanced.conflict_this_set = { conflict_this_t::master, conflict_this_t::override_wins };
	advanced.filter_by_type = true;
	advanced.type_set = { "NPC_", "CELL" };

	const nav_tree_filter_t::filter_state_t search;

	const auto result = filter_composer::compose_filter(true, advanced, search);

	REQUIRE(result.filter_conflict_all == true);
	REQUIRE(result.conflict_all_set == std::set<conflict_all_t>{ conflict_all_t::conflict, conflict_all_t::override_benign });
	REQUIRE(result.filter_conflict_this == true);
	REQUIRE(result.conflict_this_set == std::set<conflict_this_t>{ conflict_this_t::master, conflict_this_t::override_wins });
	REQUIRE(result.filter_by_type == true);
	REQUIRE(result.type_set == std::set<std::string>{ "CELL", "NPC_" });
}

TEST_CASE("filter_composer::compose_filter, advanced and search fields both present", "[u]")
{
	nav_tree_filter_t::filter_state_t advanced;
	advanced.filter_by_type = true;
	advanced.type_set = { "ARMO" };

	nav_tree_filter_t::filter_state_t search;
	search.filter_by_id = true;
	search.id_text = "iron";
	search.search_case_sensitive = true;

	const auto result = filter_composer::compose_filter(false, advanced, search);

	REQUIRE(result.filter_by_type == true);
	REQUIRE(result.type_set == std::set<std::string>{ "ARMO" });
	REQUIRE(result.filter_by_id == true);
	REQUIRE(result.id_text == "iron");
	REQUIRE(result.search_case_sensitive == true);
}

TEST_CASE("filter_composer::compose_filter, all empty equals default constructed state", "[u]")
{
	const nav_tree_filter_t::filter_state_t advanced;
	const nav_tree_filter_t::filter_state_t search;

	const auto result = filter_composer::compose_filter(false, advanced, search);

	REQUIRE(result == nav_tree_filter_t::filter_state_t{});
}
