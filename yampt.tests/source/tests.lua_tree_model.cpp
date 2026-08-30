#include <catch2/catch_all.hpp>
#include <model/lua_tree_model.hpp>

TEST_CASE("lua_tree_model_t::set_scan_result, empty result produces zero rows", "[u]")
{
	lua_tree_model_t model;

	lua_scan_result_t result;
	model.set_scan_result(result);

	REQUIRE(model.rowCount(QModelIndex()) == 0);
}

TEST_CASE("lua_tree_model_t::set_scan_result, registrations grouped by mod name", "[u]")
{
	lua_tree_model_t model;

	lua_scan_result_t result;

	handler_registration_t reg_a;
	reg_a.mod_name = "ModAlpha";
	reg_a.interface_name = "I.AI";
	reg_a.method_name = "addHandler";
	reg_a.script_path = "scripts/a.lua";
	reg_a.line_number = 10;

	handler_registration_t reg_b;
	reg_b.mod_name = "ModAlpha";
	reg_b.interface_name = "I.Combat";
	reg_b.method_name = "addHandler";
	reg_b.script_path = "scripts/b.lua";
	reg_b.line_number = 20;

	handler_registration_t reg_c;
	reg_c.mod_name = "ModBeta";
	reg_c.interface_name = "I.UI";
	reg_c.method_name = "addHandler";
	reg_c.script_path = "scripts/c.lua";
	reg_c.line_number = 5;

	result.registrations = { reg_a, reg_b, reg_c };
	model.set_scan_result(result);

	REQUIRE(model.rowCount(QModelIndex()) == 2);

	const auto group_0 = model.index(0, 0, QModelIndex());
	const auto group_1 = model.index(1, 0, QModelIndex());

	REQUIRE(model.rowCount(group_0) + model.rowCount(group_1) == 3);
}

TEST_CASE("lua_tree_model_t::clear, resets to empty", "[u]")
{
	lua_tree_model_t model;

	lua_scan_result_t result;
	handler_registration_t reg;
	reg.mod_name = "TestMod";
	reg.interface_name = "I.AI";
	reg.method_name = "handler";
	reg.script_path = "test.lua";
	reg.line_number = 1;
	result.registrations = { reg };

	model.set_scan_result(result);
	REQUIRE(model.rowCount(QModelIndex()) == 1);

	model.clear();
	REQUIRE(model.rowCount(QModelIndex()) == 0);
}

TEST_CASE("lua_tree_model_t::node_at, returns registration index for leaf", "[u]")
{
	lua_tree_model_t model;

	lua_scan_result_t result;
	handler_registration_t reg;
	reg.mod_name = "TestMod";
	reg.interface_name = "I.AI";
	reg.method_name = "addHandler";
	reg.type_argument = "NPC";
	reg.script_path = "test.lua";
	reg.line_number = 42;
	result.registrations = { reg };

	model.set_scan_result(result);

	const auto group_index = model.index(0, 0, QModelIndex());
	const auto leaf_index = model.index(0, 0, group_index);

	const auto info = model.node_at(leaf_index);
	REQUIRE(info.registration_idx == 0);
	REQUIRE_FALSE(info.is_group);
}

TEST_CASE("lua_tree_model_t::node_at, returns is_group for group node", "[u]")
{
	lua_tree_model_t model;

	lua_scan_result_t result;
	handler_registration_t reg;
	reg.mod_name = "TestMod";
	reg.interface_name = "I.AI";
	reg.method_name = "handler";
	reg.script_path = "test.lua";
	reg.line_number = 1;
	result.registrations = { reg };

	model.set_scan_result(result);

	const auto group_index = model.index(0, 0, QModelIndex());
	const auto info = model.node_at(group_index);

	REQUIRE(info.is_group);
	REQUIRE(info.registration_idx == -1);
}

TEST_CASE("lua_tree_model_t::set_scan_result, conflicts mode filters to conflicting only", "[u]")
{
	lua_tree_model_t model;

	lua_scan_result_t result;

	handler_registration_t reg_conflict;
	reg_conflict.mod_name = "ModA";
	reg_conflict.interface_name = "I.AI";
	reg_conflict.method_name = "addHandler";
	reg_conflict.type_argument = "NPC";
	reg_conflict.script_path = "a.lua";
	reg_conflict.line_number = 10;

	handler_registration_t reg_conflict_2;
	reg_conflict_2.mod_name = "ModB";
	reg_conflict_2.interface_name = "I.AI";
	reg_conflict_2.method_name = "addHandler";
	reg_conflict_2.type_argument = "NPC";
	reg_conflict_2.script_path = "b.lua";
	reg_conflict_2.line_number = 20;

	handler_registration_t reg_clean;
	reg_clean.mod_name = "ModC";
	reg_clean.interface_name = "I.UI";
	reg_clean.method_name = "show";
	reg_clean.script_path = "c.lua";
	reg_clean.line_number = 5;

	result.registrations = { reg_conflict, reg_conflict_2, reg_clean };

	handler_conflict_t conflict;
	conflict.interface_name = "I.AI";
	conflict.method_name = "addHandler";
	conflict.type_argument = "NPC";
	conflict.severity = conflict_severity_t::blocking;
	conflict.registrations = { reg_conflict, reg_conflict_2 };
	result.conflicts = { conflict };

	model.set_scan_result(result);

	int total_leaves = 0;
	for (int g = 0; g < model.rowCount(QModelIndex()); ++g)
	{
		const auto group_idx = model.index(g, 0, QModelIndex());
		total_leaves += model.rowCount(group_idx);
	}

	REQUIRE(total_leaves == 2);
}
